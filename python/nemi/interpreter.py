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
from .values import Array, format_value, from_python, is_truthy

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
            raise ExecutionError(f"undefined variable: {name}")

    def has(self, name):
        return name in self._vars


def _is_number(value):
    """True for a Nemi *arithmetic* number (int/Fraction/float, not bool)."""
    return isinstance(value, (int, Fraction, float)) and not isinstance(value, bool)


class Interpreter:
    """Evaluates a program and exposes :meth:`call` as the host entry point."""

    def __init__(self, program):
        self._functions = {}
        for defn in program.definitions:
            if defn.name in self._functions:
                raise ExecutionError(f"duplicate definition: {defn.name}")
            self._functions[defn.name] = defn
        self._main = program.main
        self._depth = 0
        # Nemi recursion uses several Python frames per call; lift the host
        # limit so we reach our own friendlier _MAX_CALL_DEPTH first.
        if sys.getrecursionlimit() < 200000:
            sys.setrecursionlimit(200000)

        self._stmt_dispatch = {
            ast.Assign: self._exec_assign,
            ast.ForLoop: self._exec_for,
            ast.WhileLoop: self._exec_while,
            ast.If: self._exec_if,
            ast.Return: self._exec_return,
            ast.ExprStatement: self._exec_expr_statement,
            ast.Prose: self._exec_prose,
        }
        self._expr_dispatch = {
            ast.IntLiteral: lambda n, e: n.value,
            ast.RealLiteral: lambda n, e: n.value,
            ast.StringLiteral: lambda n, e: n.value,
            ast.ArrayLiteral: self._eval_array_literal,
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
            raise ExecutionError(f"undefined function or procedure: {name}")
        args = [from_python(a) for a in python_args]
        return self._invoke(defn, args)

    # ==================================================================
    # Invocation
    # ==================================================================
    def _invoke(self, defn, args):
        if len(args) != len(defn.params):
            raise ExecutionError(
                f"{defn.name} expects {len(defn.params)} argument(s), "
                f"got {len(args)}",
                defn.location,
            )
        frame = Environment()
        for name, value in zip(defn.params, args):
            frame.define(name, value)

        self._depth += 1
        if self._depth > _MAX_CALL_DEPTH:
            self._depth -= 1
            raise ExecutionError(
                f"call stack overflow in {defn.name} "
                f"(recursion without a base case?)"
            )
        try:
            self._exec_block(defn.body, frame)
        except ReturnSignal as sig:
            return sig.value
        finally:
            self._depth -= 1
        return None  # fell off the end (a value-less return)

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
            return
        container = env.get(node.name)
        keys = [self._eval(i, env) for i in node.indices]
        for key in keys[:-1]:
            container = self._index_get(container, key)
        self._index_set(container, keys[-1], value)

    def _exec_for(self, node, env):
        start = self._eval(node.start, env)
        end = self._eval(node.end, env)
        if not _is_number(start) or not _is_number(end):
            raise ExecutionError("'para' bounds must be numeric", node.location)
        i = start
        while i <= end:                 # inclusive, step +1 (spec §12)
            env.define(node.var, i)
            self._exec_block(node.body, env)
            i = i + 1

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

    # ==================================================================
    # Expressions
    # ==================================================================
    def _eval(self, node, env):
        return self._expr_dispatch[type(node)](node, env)

    def _eval_array_literal(self, node, env):
        return Array([self._eval(e, env) for e in node.elements])

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
        raise ExecutionError(f"value is not indexable: {format_value(container)}")

    def _index_set(self, container, key, value):
        if isinstance(container, Array):
            container.set(key, value)
            return
        if isinstance(container, str):
            raise ExecutionError("strings are immutable; cannot assign to s[i]")
        raise ExecutionError(f"value is not indexable: {format_value(container)}")

    @staticmethod
    def _string_index(s, key):
        if not isinstance(key, int) or isinstance(key, bool):
            raise ExecutionError(f"string index must be an integer, got {key!r}")
        if key < 1 or key > len(s):
            raise ExecutionError(f"index {key} out of range 1..{len(s)}")
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
        raise ExecutionError(f"unknown unary operator {op.name}", node.location)

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
            f"expected a number, got {format_value(value)}"
        )

    def _negate(self, value):
        if not _is_number(value):
            raise ExecutionError(f"cannot negate {format_value(value)}")
        return -value

    def _sqrt(self, value):
        v = self._require_real(value)
        if v < 0:
            raise ExecutionError("√ of a negative number")
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
        raise ExecutionError(f"unknown binary operator {op.name}", node.location)

    @staticmethod
    def _arith(left, right, fn, symbol):
        if _is_number(left) and _is_number(right):
            return fn(left, right)
        raise ExecutionError(
            f"operator '{symbol}' needs numbers, got "
            f"{format_value(left)} and {format_value(right)}"
        )

    @staticmethod
    def _divide(left, right):
        if not (_is_number(left) and _is_number(right)):
            raise ExecutionError("operator '/' needs numbers")
        if right == 0:
            raise ExecutionError("division by zero")
        if isinstance(left, int) and isinstance(right, int):
            return Fraction(left, right)      # exact; ⌊·⌋ recovers integers
        return left / right

    @staticmethod
    def _modulo(left, right):
        if not (isinstance(left, int) and isinstance(right, int)) \
                or isinstance(left, bool) or isinstance(right, bool):
            raise ExecutionError("operator 'mod' needs integers")
        if right == 0:
            raise ExecutionError("modulo by zero")
        return left % right

    @staticmethod
    def _compare(op, left, right):
        if op is T.EQ:
            return left == right
        if op is T.NE:
            return left != right
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
                f"cannot order {format_value(left)} and {format_value(right)}"
            )
        raise ExecutionError(f"unknown comparison {op.name}")

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
            f"undefined function or procedure: {name}", node.location
        )

    # -- l-value helpers (for intercambia) ------------------------------
    def _get_lvalue(self, node, env):
        if isinstance(node, ast.Variable):
            return env.get(node.name)
        if isinstance(node, ast.Index):
            container = self._eval(node.base, env)
            key = self._eval(node.index, env)
            return self._index_get(container, key)
        raise ExecutionError("intercambia expects variables or array cells")

    def _set_lvalue(self, node, env, value):
        if isinstance(node, ast.Variable):
            env.define(node.name, value)
            return
        if isinstance(node, ast.Index):
            container = self._eval(node.base, env)
            key = self._eval(node.index, env)
            self._index_set(container, key, value)
            return
        raise ExecutionError("intercambia expects variables or array cells")

    def _swap(self, args, env):
        if len(args) != 2:
            raise ExecutionError("intercambia expects exactly 2 arguments")
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
_COMPARATORS = frozenset({T.EQ, T.NE, T.LT, T.LE, T.GT, T.GE})


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
    raise ExecutionError("long expects an array or string")


def _prim_print(interp, args):
    rendered = " ".join(
        v if isinstance(v, str) else format_value(v) for v in args
    )
    print(rendered)
    return None


def _arity(name, args, n):
    if len(args) != n:
        raise ExecutionError(f"{name} expects {n} argument(s), got {len(args)}")


_PRIMITIVES = {
    "piso": _prim_floor,
    "techo": _prim_ceil,
    "raíz": _prim_sqrt,
    "raiz": _prim_sqrt,
    "abs": _prim_abs,
    "mod": _prim_mod,
    "long": _prim_length,
    "imprime": _prim_print,
}
