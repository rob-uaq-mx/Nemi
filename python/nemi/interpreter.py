"""Tree-walking interpreter for Nemi.

The :class:`Interpreter` evaluates a :class:`nemi.ast_nodes.Program`. Dispatch
is table-driven (a dict from node type to handler method), which mirrors a C++
visitor and keeps each rule short and testable.

Key semantic decisions (spec §8, §11, §12, §16):

* **Numeric tower** — ``int`` (bignum), ``Fraction`` (from ``/`` on integers,
  keeping ``⌊n/2⌋`` exact for bignum), ``float`` (reals / inexact ``√``).
* **``+`` / ``·`` polysemy** — logical OR/AND when *both* operands are
  booleans, arithmetic otherwise (§11). The corpus's Warshall uses ``∨``/``∧``
  directly, which work uniformly over booleans and 0/1 bits.
* **``∧`` / ``∨`` short-circuit** (§12) and preserve operand kind: bits in →
  bit (0/1) out, booleans in → boolean out.
* **Parameter passing** — scalars by value, arrays/strings by reference (§12);
  this falls out of Python's object model (immutable scalars vs. shared
  :class:`Array`).
* **``intercambia(a, b)``** — a primitive that swaps two *l-values* in the
  caller's frame, so ``mcd`` can swap its by-value scalar parameters.
"""

import math
import sys
from fractions import Fraction

from . import ast_nodes as ast
from .errors import ExecutionError
from .tokens import TokenKind as T
from .values import Array, Conjunto, format_value, from_python, is_truthy

# How deep Nemi's own call stack may go before we report a friendly overflow
# (spec §16: "recursion without a base case"). Well below Python's own limit.
_MAX_CALL_DEPTH = 8000


class ReturnSignal(Exception):
    """Internal control-flow exception carrying a ``regresa`` value."""

    __slots__ = ("value",)

    def __init__(self, value):
        self.value = value


class Environment:
    """A single call frame: a flat name → value mapping.

    Nemi has no nested lexical scopes (functions do not close over each other),
    so one frame per call is enough. Kept as a class rather than a bare dict to
    localise the "undefined variable" diagnostic.
    """

    __slots__ = ("_vars",)

    def __init__(self):
        self._vars = {}

    def define(self, name, value):
        self._vars[name] = value

    def get(self, name):
        try:
            return self._vars[name]
        except KeyError:
            raise ExecutionError(f"variable no definida: {name}")

    def has(self, name):
        return name in self._vars


def _is_number(value):
    """True for a Nemi *arithmetic* number (int/Fraction/float, not bool)."""
    return isinstance(value, (int, Fraction, float)) and not isinstance(value, bool)


# Operator -> Nemi symbol, for _expr_to_source below (afirma's error message
# quotes the failing expression; §21.2 needs the source text, and since the
# lexer/parser don't track raw text spans, this reconstructs equivalent Nemi
# syntax from the AST instead -- simpler than plumbing byte offsets through
# every token, and it's always exactly consistent with how the expression was
# actually parsed).
_OP_SYMBOLS = {
    T.EQ: "=", T.NE: "≠", T.LT: "<", T.LE: "≤", T.GT: ">", T.GE: "≥",
    T.IN: "∈", T.WITHIN: "∈", T.NOTIN: "∉", T.SUBSETEQ: "⊆", T.SUBSET: "⊂",
    T.PLUS: "+", T.MINUS: "−", T.TIMES: "·", T.DIVIDE: "/", T.MOD: "mod",
    T.AND: "∧", T.OR: "∨", T.NOT: "¬", T.SQRT: "√",
}


def _expr_to_source(expr):
    """Render an expression back to (roughly) the Nemi syntax it came from."""
    if isinstance(expr, ast.IntLiteral):
        return str(expr.value)
    if isinstance(expr, ast.RealLiteral):
        return str(expr.value)
    if isinstance(expr, ast.StringLiteral):
        return f'"{expr.value}"'
    if isinstance(expr, ast.Variable):
        return expr.name
    if isinstance(expr, ast.ArrayLiteral):
        return "[" + ", ".join(_expr_to_source(e) for e in expr.elements) + "]"
    if isinstance(expr, ast.SetLiteral):
        if not expr.elements:
            return "∅"
        return "{" + ", ".join(_expr_to_source(e) for e in expr.elements) + "}"
    if isinstance(expr, ast.Index):
        return f"{_expr_to_source(expr.base)}[{_expr_to_source(expr.index)}]"
    if isinstance(expr, ast.Call):
        args = ", ".join(_expr_to_source(a) for a in expr.args)
        return f"{expr.name}({args})"
    if isinstance(expr, ast.Unary):
        if expr.op is T.LFLOOR:
            return f"⌊{_expr_to_source(expr.operand)}⌋"
        if expr.op is T.LCEIL:
            return f"⌈{_expr_to_source(expr.operand)}⌉"
        symbol = _OP_SYMBOLS.get(expr.op, expr.op.name)
        return f"({symbol} {_expr_to_source(expr.operand)})"
    if isinstance(expr, ast.Binary):
        symbol = _OP_SYMBOLS.get(expr.op, expr.op.name)
        return f"({_expr_to_source(expr.left)} {symbol} {_expr_to_source(expr.right)})"
    return "?expr?"


class Interpreter:
    """Evaluates a program and exposes :meth:`call` as the host entry point."""

    def __init__(self, program):
        self._functions = {}
        for defn in program.definitions:
            if defn.name in self._functions:
                raise ExecutionError(f"definición duplicada: {defn.name}")
            self._functions[defn.name] = defn
        self._main = program.main
        self._depth = 0
        # traza (spec §21.3, v0.2 [OPC]): a counter, not a bool, so a nested
        # `traza` inside an already-traced call doesn't turn tracing off when
        # it finishes -- the outer traza's dynamic extent keeps it active.
        self._trace_depth = 0
        # Nemi recursion uses several Python frames per call; lift the host
        # limit so we reach our own friendlier _MAX_CALL_DEPTH first.
        if sys.getrecursionlimit() < 200000:
            sys.setrecursionlimit(200000)

        self._stmt_dispatch = {
            ast.Assign: self._exec_assign,
            ast.ForLoop: self._exec_for,
            ast.ForEach: self._exec_for_each,
            ast.WhileLoop: self._exec_while,
            ast.If: self._exec_if,
            ast.Return: self._exec_return,
            ast.ExprStatement: self._exec_expr_statement,
            ast.Prose: self._exec_prose,
            ast.Assert: self._exec_assert,
            ast.Trace: self._exec_trace,
        }
        self._expr_dispatch = {
            ast.IntLiteral: lambda n, e: n.value,
            ast.RealLiteral: lambda n, e: n.value,
            ast.StringLiteral: lambda n, e: n.value,
            ast.ArrayLiteral: self._eval_array_literal,
            ast.SetLiteral: self._eval_set_literal,
            ast.Variable: lambda n, e: e.get(n.name),
            ast.Index: self._eval_index,
            ast.Call: self._eval_call,
            ast.Unary: self._eval_unary,
            ast.Binary: self._eval_binary,
        }

    # ==================================================================
    # Public API
    # ==================================================================
    def has_function(self, name):
        return name in self._functions

    def run_program(self):
        """Execute the top-level statements (the script body) in a global frame.

        Definitions are hoisted, so top-level code may call any function in the
        loaded source regardless of order. Statement values are discarded — a
        bare call like ``factorial(100)`` is evaluated for effect and NOT
        printed (script semantics, à la Python); use ``imprime(...)`` to
        produce output. A stray top-level ``regresa`` simply ends the script.
        """
        env = Environment()
        try:
            self._exec_block(self._main, env)
        except ReturnSignal:
            pass
        return None

    def call(self, name, python_args):
        """Call a Nemi function with plain Python arguments; returns a Nemi value.

        Lists are wrapped as base-1 :class:`Array`s (and shared, so a procedure
        that mutates a parameter mutates the caller's list-derived Array).
        """
        defn = self._functions.get(name)
        if defn is None:
            raise ExecutionError(f"función o procedimiento no definido: {name}")
        args = [from_python(a) for a in python_args]
        return self._invoke(defn, args)

    # ==================================================================
    # Invocation
    # ==================================================================
    def _invoke(self, defn, args):
        if len(args) != len(defn.params):
            raise ExecutionError(
                f"{defn.name} espera {len(defn.params)} argumento(s), "
                f"se obtuvo {len(args)}",
                defn.location,
            )
        frame = Environment()
        for name, value in zip(defn.params, args):
            frame.define(name, value)

        # traza (spec §21.3, v0.2 [OPC]): entry/return are printed at the
        # caller's depth, so a pair brackets the callee's body (which itself
        # prints one level deeper -- see _exec_assign).
        if self._trace_depth:
            indent = "  " * self._depth
            args_text = ", ".join(format_value(a) for a in args)
            print(f"{indent}→ {defn.name}({args_text})")

        self._depth += 1
        if self._depth > _MAX_CALL_DEPTH:
            self._depth -= 1
            raise ExecutionError(
                f"desbordamiento de la pila de llamadas en {defn.name} "
                f"(¿recursión sin caso base?)"
            )
        try:
            self._exec_block(defn.body, frame)
            result = None  # fell off the end (a value-less return)
        except ReturnSignal as sig:
            result = sig.value
        finally:
            self._depth -= 1

        if self._trace_depth:
            indent = "  " * self._depth
            print(f"{indent}← {format_value(result)}")
        return result

    def _exec_block(self, statements, env):
        for stmt in statements:
            self._stmt_dispatch[type(stmt)](stmt, env)

    # ==================================================================
    # Statements
    # ==================================================================
    def _exec_assign(self, node, env):
        value = self._eval(node.value, env)
        if not node.indices:
            env.define(node.name, value)
            place = node.name
        else:
            container = env.get(node.name)
            keys = [self._eval(i, env) for i in node.indices]
            for key in keys[:-1]:
                container = self._index_get(container, key)
            self._index_set(container, keys[-1], value)
            place = node.name + "".join(f"[{format_value(k)}]" for k in keys)

        if self._trace_depth:
            # one level deeper than the enclosing call's entry/return (spec
            # §21.3's example shows "  lugar ← valor" with extra indent).
            indent = "  " * self._depth
            print(f"{indent}  {place} ← {format_value(value)}")

    def _exec_for(self, node, env):
        start = self._eval(node.start, env)
        end = self._eval(node.end, env)
        if not _is_number(start) or not _is_number(end):
            raise ExecutionError("los límites de 'para' deben ser numéricos", node.location)
        i = start
        while i <= end:                 # inclusive, step +1 (spec §12)
            env.define(node.var, i)
            self._exec_block(node.body, env)
            i = i + 1

    def _exec_for_each(self, node, env):
        # ForEach (spec §20.3, v0.2): a Conjunto iterates in canonical order
        # (already how Conjunto stores its elements), an Array in index
        # order. Mutating the collection while iterating it is explicitly
        # undefined by the spec, so this just walks the live sequence with
        # no defensive copy -- Python's own iteration semantics apply.
        collection = self._eval(node.collection, env)
        if isinstance(collection, Array):
            items = collection.items()
        elif isinstance(collection, Conjunto):
            items = collection.items()
        else:
            raise ExecutionError(
                f"'para cada' requiere un arreglo o conjunto, se obtuvo {format_value(collection)}",
                node.location,
            )
        for item in items:
            env.define(node.var, item)
            self._exec_block(node.body, env)

    def _exec_while(self, node, env):
        while is_truthy(self._eval(node.condition, env)):
            self._exec_block(node.body, env)

    def _exec_if(self, node, env):
        if is_truthy(self._eval(node.condition, env)):
            self._exec_block(node.then_body, env)
        elif node.else_body is not None:
            self._exec_block(node.else_body, env)

    def _exec_return(self, node, env):
        value = self._eval(node.value, env) if node.value is not None else None
        raise ReturnSignal(value)

    def _exec_expr_statement(self, node, env):
        self._eval_call(node.call, env)  # result discarded

    def _exec_prose(self, node, env):
        pass  # non-executable prose action (spec §14)

    def _exec_assert(self, node, env):
        # afirma expr [, "mensaje"] (spec §21.2, v0.2): self-check the
        # student runs against known values. Reuses is_truthy (same rule as
        # si/mientras) rather than requiring a strict 0/1 bit.
        value = self._eval(node.condition, env)
        if not is_truthy(value):
            text = f"afirma falsa: {_expr_to_source(node.condition)}"
            if node.message is not None:
                text += f" -- {node.message}"
            raise ExecutionError(text, node.location)

    def _exec_trace(self, node, env):
        # traza expr (spec §21.3, v0.2 [OPC]): runs `expression` (typically a
        # call) with entry/return/assignment tracing active for its dynamic
        # extent; the expression's value is discarded (traza is run for its
        # printed side effects, same as an ExprStatement).
        self._trace_depth += 1
        try:
            self._eval(node.expression, env)
        finally:
            self._trace_depth -= 1

    # ==================================================================
    # Expressions
    # ==================================================================
    def _eval(self, node, env):
        return self._expr_dispatch[type(node)](node, env)

    def _eval_array_literal(self, node, env):
        return Array([self._eval(e, env) for e in node.elements])

    def _eval_set_literal(self, node, env):
        # Conjunto's constructor is idempotent (spec §20.1: {3,1,2,1} -> {1,2,3}).
        return Conjunto(self._eval(e, env) for e in node.elements)

    def _eval_index(self, node, env):
        container = self._eval(node.base, env)
        key = self._eval(node.index, env)
        return self._index_get(container, key)

    def _index_get(self, container, key):
        if isinstance(container, Array):
            return container.get(key)
        if isinstance(container, str):
            idx = self._string_index(container, key)
            return container[idx]
        raise ExecutionError(f"el valor no es indexable: {format_value(container)}")

    def _index_set(self, container, key, value):
        if isinstance(container, Array):
            container.set(key, value)
            return
        if isinstance(container, str):
            raise ExecutionError("las cadenas son inmutables; no se puede asignar a s[i]")
        raise ExecutionError(f"el valor no es indexable: {format_value(container)}")

    @staticmethod
    def _string_index(s, key):
        if not isinstance(key, int) or isinstance(key, bool):
            raise ExecutionError(f"el índice de cadena debe ser un entero, se obtuvo {key!r}")
        if key < 1 or key > len(s):
            raise ExecutionError(f"índice {key} fuera de rango 1..{len(s)}")
        return key - 1

    # -- unary ----------------------------------------------------------
    def _eval_unary(self, node, env):
        op = node.op
        if op is T.MINUS:
            return self._negate(self._eval(node.operand, env))
        if op is T.NOT:
            v = self._eval(node.operand, env)
            return self._logic_result(not is_truthy(v), v)
        if op is T.SQRT:
            return self._sqrt(self._eval(node.operand, env))
        if op is T.LFLOOR:
            return self._floor(node.operand, env)
        if op is T.LCEIL:
            return math.ceil(self._require_real(self._eval(node.operand, env)))
        raise ExecutionError(f"operador unario desconocido {op.name}", node.location)

    def _floor(self, operand, env):
        # Exact ⌊√n⌋ for a non-negative integer n (keeps primality/bignum sound).
        if isinstance(operand, ast.Unary) and operand.op is T.SQRT:
            x = self._eval(operand.operand, env)
            if isinstance(x, int) and not isinstance(x, bool) and x >= 0:
                return math.isqrt(x)
        return math.floor(self._require_real(self._eval(operand, env)))

    @staticmethod
    def _require_real(value):
        if _is_number(value):
            return value
        raise ExecutionError(
            f"se esperaba un número, se obtuvo {format_value(value)}"
        )

    def _negate(self, value):
        if not _is_number(value):
            raise ExecutionError(f"no se puede negar {format_value(value)}")
        return -value

    def _sqrt(self, value):
        v = self._require_real(value)
        if v < 0:
            raise ExecutionError("√ de un número negativo")
        if isinstance(v, int):
            root = math.isqrt(v)
            return root if root * root == v else math.sqrt(v)
        return math.sqrt(float(v))

    # -- binary ---------------------------------------------------------
    def _eval_binary(self, node, env):
        op = node.op

        # Logical connectives short-circuit (spec §12).
        if op is T.AND:
            left = self._eval(node.left, env)
            if not is_truthy(left):
                return self._logic_result(False, left)
            right = self._eval(node.right, env)
            return self._logic_result(is_truthy(right), left, right)
        if op is T.OR:
            left = self._eval(node.left, env)
            if is_truthy(left):
                return self._logic_result(True, left)
            right = self._eval(node.right, env)
            return self._logic_result(is_truthy(right), left, right)

        left = self._eval(node.left, env)
        right = self._eval(node.right, env)

        if op is T.PLUS:
            if isinstance(left, bool) and isinstance(right, bool):
                return left or right          # polysemy: bit + bit ↦ OR
            return self._arith(left, right, lambda a, b: a + b, "+")
        if op is T.MINUS:
            return self._arith(left, right, lambda a, b: a - b, "−")
        if op is T.TIMES:
            if isinstance(left, bool) and isinstance(right, bool):
                return left and right         # polysemy: bit · bit ↦ AND
            return self._arith(left, right, lambda a, b: a * b, "·")
        if op is T.DIVIDE:
            return self._divide(left, right)
        if op is T.MOD:
            return self._modulo(left, right)
        if op in _COMPARATORS:
            return self._compare(op, left, right)
        raise ExecutionError(f"operador binario desconocido {op.name}", node.location)

    @staticmethod
    def _arith(left, right, fn, symbol):
        if _is_number(left) and _is_number(right):
            return fn(left, right)
        raise ExecutionError(
            f"el operador '{symbol}' requiere números, se obtuvo "
            f"{format_value(left)} y {format_value(right)}"
        )

    @staticmethod
    def _divide(left, right):
        if not (_is_number(left) and _is_number(right)):
            raise ExecutionError("el operador '/' requiere números")
        if right == 0:
            raise ExecutionError("división entre cero")
        if isinstance(left, int) and isinstance(right, int):
            return Fraction(left, right)      # exact; ⌊·⌋ recovers integers
        return left / right

    @staticmethod
    def _modulo(left, right):
        if not (isinstance(left, int) and isinstance(right, int)) \
                or isinstance(left, bool) or isinstance(right, bool):
            raise ExecutionError("el operador 'mod' requiere enteros")
        if right == 0:
            raise ExecutionError("módulo entre cero")
        return left % right

    @staticmethod
    def _compare(op, left, right):
        if op is T.EQ:
            return left == right
        if op is T.NE:
            return left != right
        if op is T.IN or op is T.WITHIN or op is T.NOTIN:
            if not isinstance(right, Conjunto):
                raise ExecutionError(
                    f"'{'∉' if op is T.NOTIN else '∈'}' requiere un conjunto a la "
                    f"derecha, se obtuvo {format_value(right)}"
                )
            member = right.contains(left)
            return member if op is not T.NOTIN else not member
        if op is T.SUBSETEQ or op is T.SUBSET:
            symbol = "⊆" if op is T.SUBSETEQ else "⊂"
            if not (isinstance(left, Conjunto) and isinstance(right, Conjunto)):
                raise ExecutionError(
                    f"'{symbol}' requiere dos conjuntos, se obtuvo {format_value(left)} "
                    f"y {format_value(right)}"
                )
            return left.is_subset(right) if op is T.SUBSETEQ else left.is_proper_subset(right)
        try:
            if op is T.LT:
                return left < right
            if op is T.LE:
                return left <= right
            if op is T.GT:
                return left > right
            if op is T.GE:
                return left >= right
        except TypeError:
            raise ExecutionError(
                f"no se pueden ordenar {format_value(left)} y {format_value(right)}"
            )
        raise ExecutionError(f"comparación desconocida {op.name}")

    @staticmethod
    def _logic_result(result, *operands):
        """Return a bit (0/1) if any operand is an int, else a boolean.

        This keeps ``∧``/``∨``/``¬`` closed over 0/1 bits (as Warshall needs)
        while returning proper booleans for comparison-based conditions.
        """
        if any(isinstance(o, int) and not isinstance(o, bool) for o in operands):
            return 1 if result else 0
        return result

    # ==================================================================
    # Calls and primitives
    # ==================================================================
    def _eval_call(self, node, env):
        name = node.name

        # 'intercambia' is special: it takes l-values and swaps them in place.
        if name == "intercambia":
            return self._swap(node.args, env)

        args = [self._eval(a, env) for a in node.args]

        defn = self._functions.get(name)
        if defn is not None:
            return self._invoke(defn, args)

        primitive = _PRIMITIVES.get(name)
        if primitive is not None:
            return primitive(self, args)

        raise ExecutionError(
            f"función o procedimiento no definido: {name}", node.location
        )

    # -- l-value helpers (for intercambia) ------------------------------
    def _get_lvalue(self, node, env):
        if isinstance(node, ast.Variable):
            return env.get(node.name)
        if isinstance(node, ast.Index):
            container = self._eval(node.base, env)
            key = self._eval(node.index, env)
            return self._index_get(container, key)
        raise ExecutionError("intercambia espera variables o celdas de arreglo")

    def _set_lvalue(self, node, env, value):
        if isinstance(node, ast.Variable):
            env.define(node.name, value)
            return
        if isinstance(node, ast.Index):
            container = self._eval(node.base, env)
            key = self._eval(node.index, env)
            self._index_set(container, key, value)
            return
        raise ExecutionError("intercambia espera variables o celdas de arreglo")

    def _swap(self, args, env):
        if len(args) != 2:
            raise ExecutionError("intercambia espera exactamente 2 argumentos")
        a, b = args
        va = self._get_lvalue(a, env)
        vb = self._get_lvalue(b, env)
        self._set_lvalue(a, env, vb)
        self._set_lvalue(b, env, va)
        return None


# --------------------------------------------------------------------------
# Value-returning primitives (spec §15). ``intercambia`` is handled in the
# interpreter because it needs l-values rather than evaluated arguments.
# --------------------------------------------------------------------------
_COMPARATORS = frozenset(
    {T.EQ, T.NE, T.LT, T.LE, T.GT, T.GE, T.IN, T.WITHIN, T.NOTIN, T.SUBSETEQ, T.SUBSET}
)


def _prim_floor(interp, args):
    _arity("piso", args, 1)
    return math.floor(interp._require_real(args[0]))


def _prim_ceil(interp, args):
    _arity("techo", args, 1)
    return math.ceil(interp._require_real(args[0]))


def _prim_sqrt(interp, args):
    _arity("raíz", args, 1)
    return interp._sqrt(args[0])


def _prim_abs(interp, args):
    _arity("abs", args, 1)
    return abs(interp._require_real(args[0]))


def _prim_mod(interp, args):
    _arity("mod", args, 2)
    return interp._modulo(args[0], args[1])


def _prim_length(interp, args):
    _arity("long", args, 1)
    value = args[0]
    if isinstance(value, Array):
        return value.length()
    if isinstance(value, str):
        return len(value)
    if isinstance(value, Conjunto):
        return value.cardinality()
    raise ExecutionError("long espera un arreglo, cadena o conjunto")


# -- String primitives (spec §22.6, v0.2) ----------------------------------
# `long(s)` and indexing `s[i]` (base 1, a length-1 string) already exist
# above/in _index_get; these three round out what cadenas.nemi needs.
def _prim_concatena(interp, args):
    _arity("concatena", args, 2)
    s, t = args
    if not isinstance(s, str) or not isinstance(t, str):
        raise ExecutionError(
            f"concatena espera dos cadenas, se obtuvo {format_value(s)}, {format_value(t)}"
        )
    return s + t


def _prim_texto(interp, args):
    _arity("texto", args, 1)
    value = args[0]
    if not _is_number(value):
        raise ExecutionError(f"texto espera un número, se obtuvo {format_value(value)}")
    return format_value(value)  # same rendering imprime/afirma already use


def _prim_valor(interp, args):
    _arity("valor", args, 1)
    value = args[0]
    if not isinstance(value, str) or len(value) != 1 or not ("0" <= value <= "9"):
        raise ExecutionError(
            f"valor espera un carácter de un solo dígito, se obtuvo {format_value(value)}"
        )
    return ord(value) - ord("0")


# -- Conjunto primitives (spec §20.2, v0.2) --------------------------------
def _require_set(name, value):
    if not isinstance(value, Conjunto):
        raise ExecutionError(f"{name} espera un conjunto, se obtuvo {format_value(value)}")
    return value


def _prim_pertenece(interp, args):
    _arity("pertenece", args, 2)
    return _require_set("pertenece", args[1]).contains(args[0])


def _prim_subconjunto(interp, args):
    _arity("subconjunto", args, 2)
    a = _require_set("subconjunto", args[0])
    b = _require_set("subconjunto", args[1])
    return a.is_subset(b)


def _prim_union(interp, args):
    _arity("union", args, 2)
    a = _require_set("union", args[0])
    b = _require_set("union", args[1])
    return a.union(b)


def _prim_interseccion(interp, args):
    _arity("interseccion", args, 2)
    a = _require_set("interseccion", args[0])
    b = _require_set("interseccion", args[1])
    return a.intersection(b)


def _prim_diferencia(interp, args):
    _arity("diferencia", args, 2)
    a = _require_set("diferencia", args[0])
    b = _require_set("diferencia", args[1])
    return a.difference(b)


def _prim_cardinalidad(interp, args):
    _arity("cardinalidad", args, 1)
    return _require_set("cardinalidad", args[0]).cardinality()


# -- Dynamic lists (spec §20.4, v0.2) --------------------------------------
def _prim_agrega(interp, args):
    _arity("agrega", args, 2)
    a, x = args
    if isinstance(a, Array):
        a.append(x)  # grows the list, mutating by reference
        return None
    if isinstance(a, Conjunto):
        a.add(x)  # idempotent insert (§20.2)
        return None
    raise ExecutionError(f"agrega espera un arreglo o conjunto, se obtuvo {format_value(a)}")


def _deep_copy(value):
    if isinstance(value, Array):
        return Array([_deep_copy(x) for x in value.items()])
    if isinstance(value, Conjunto):
        return Conjunto(_deep_copy(x) for x in value.items())
    return value  # scalars (int/Fraction/float/bool/str) are already immutable


def _prim_copia(interp, args):
    _arity("copia", args, 1)
    return _deep_copy(args[0])


def _require_nonneg_int(name, value):
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ExecutionError(f"{name} espera un entero no negativo, se obtuvo {format_value(value)}")
    return value


def _prim_arreglo_cero(interp, args):
    _arity("arreglo_cero", args, 1)
    n = _require_nonneg_int("arreglo_cero", args[0])
    return Array([0] * n)


def _prim_matriz_cero(interp, args):
    _arity("matriz_cero", args, 2)
    m = _require_nonneg_int("matriz_cero", args[0])
    n = _require_nonneg_int("matriz_cero", args[1])
    return Array([Array([0] * n) for _ in range(m)])


def _prim_print(interp, args):
    rendered = " ".join(
        v if isinstance(v, str) else format_value(v) for v in args
    )
    print(rendered)
    return None


def _arity(name, args, n):
    if len(args) != n:
        raise ExecutionError(f"{name} espera {n} argumento(s), se obtuvo {len(args)}")


_PRIMITIVES = {
    "piso": _prim_floor,
    "techo": _prim_ceil,
    "raíz": _prim_sqrt,
    "raiz": _prim_sqrt,
    "abs": _prim_abs,
    "mod": _prim_mod,
    "long": _prim_length,
    "concatena": _prim_concatena,
    "texto": _prim_texto,
    "valor": _prim_valor,
    "imprime": _prim_print,
    "pertenece": _prim_pertenece,
    "subconjunto": _prim_subconjunto,
    "union": _prim_union,
    "interseccion": _prim_interseccion,
    "diferencia": _prim_diferencia,
    "cardinalidad": _prim_cardinalidad,
    "agrega": _prim_agrega,
    "copia": _prim_copia,
    "arreglo_cero": _prim_arreglo_cero,
    "matriz_cero": _prim_matriz_cero,
}
