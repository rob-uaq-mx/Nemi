"""Abstract syntax tree node classes.

Two small class hierarchies rooted at :class:`Statement` and :class:`Expr`,
mirroring the grammar in spec §10. Nodes are plain data holders (no behaviour)
so the tree can be walked by any number of visitors; the interpreter is one
such visitor. This shape ports directly to a C++ class hierarchy with a
``Node`` base and a visitor interface.

Operators are represented by :class:`nemi.tokens.TokenKind` values rather than
strings, so the interpreter switches on a closed enum.
"""

from dataclasses import dataclass, field
from typing import List, Optional

from .tokens import TokenKind


# --------------------------------------------------------------------------
# Top level
# --------------------------------------------------------------------------
@dataclass
class Program:
    """A whole source unit.

    ``definitions`` holds the ``función``/``procedimiento`` definitions;
    ``main`` holds any top-level statements (the "script body"), executed in
    source order after all definitions are registered (spec §10). Definitions
    are hoisted, so top-level code may call a function defined later in the
    file.
    """
    definitions: List["Definition"] = field(default_factory=list)
    main: List["Statement"] = field(default_factory=list)


@dataclass
class Definition:
    """A ``función`` or ``procedimiento``.

    ``is_function`` is True for ``función`` (returns a value) and False for
    ``procedimiento`` (executes actions). The distinction is mostly
    documentary at runtime: both share the same call machinery.
    """
    name: str
    params: List[str]
    body: List["Statement"]
    is_function: bool
    location: object = None


# --------------------------------------------------------------------------
# Statements
# --------------------------------------------------------------------------
class Statement:
    """Marker base class for statements."""


@dataclass
class Assign(Statement):
    """``place ← value``. ``indices`` is empty for a plain variable."""
    name: str
    indices: List["Expr"]
    value: "Expr"
    location: object = None


@dataclass
class ForLoop(Statement):
    """``para var ← start hasta end [repite] … fin para`` (inclusive, step +1)."""
    var: str
    start: "Expr"
    end: "Expr"
    body: List[Statement]
    location: object = None


@dataclass
class ForEach(Statement):
    """``para cada var en coleccion [repite] … fin para`` (spec §20.3, v0.2).

    Iterates a Conjunto in canonical order, or an Array in index order.
    """
    var: str
    collection: "Expr"
    body: List[Statement]
    location: object = None


@dataclass
class WhileLoop(Statement):
    """``mientras cond … fin mientras``."""
    condition: "Expr"
    body: List[Statement]
    location: object = None


@dataclass
class If(Statement):
    """``si cond [entonces] … [alt …] fin si``. ``else_body`` may be None."""
    condition: "Expr"
    then_body: List[Statement]
    else_body: Optional[List[Statement]]
    location: object = None


@dataclass
class Return(Statement):
    """``regresa [expr]``. ``value`` is None for a bare return."""
    value: Optional["Expr"]
    location: object = None


@dataclass
class ExprStatement(Statement):
    """A call used for its effect, e.g. ``intercambia(a, b)``."""
    call: "Call"
    location: object = None


@dataclass
class Prose(Statement):
    """A non-executable prose action ``« … »`` (spec §14). Evaluated as a no-op."""
    text: str
    location: object = None


@dataclass
class Assert(Statement):
    """``afirma expr [, "mensaje"]`` (spec §21.2, v0.2): self-check the
    student can run against known values. ``message`` is the optional
    literal string's value, or ``None`` if omitted (the grammar takes a
    ``cadena`` here, not a general expression)."""
    condition: "Expr"
    message: Optional[str]
    location: object = None


@dataclass
class Trace(Statement):
    """``traza expr`` (spec §21.3, v0.2 [OPC]): runs ``expression`` (typically
    a call) with entry/return/assignment tracing active for its dynamic
    extent."""
    expression: "Expr"
    location: object = None


# --------------------------------------------------------------------------
# Expressions
# --------------------------------------------------------------------------
class Expr:
    """Marker base class for expressions."""


@dataclass
class IntLiteral(Expr):
    value: int
    location: object = None


@dataclass
class RealLiteral(Expr):
    value: float
    location: object = None


@dataclass
class StringLiteral(Expr):
    value: str
    location: object = None


@dataclass
class ArrayLiteral(Expr):
    """``[e1, e2, …]`` — an extension beyond §10 so hosts/tests can build inputs."""
    elements: List[Expr]
    location: object = None


@dataclass
class SetLiteral(Expr):
    """``{e1, e2, …}`` or ``∅`` (spec §20.1, v0.2). Duplicates collapse at
    evaluation time (the ``Conjunto`` constructor is idempotent), not here --
    ``elements`` is empty only for ``{}``/``∅`` written literally."""
    elements: List[Expr]
    location: object = None


@dataclass
class Variable(Expr):
    name: str
    location: object = None


@dataclass
class Index(Expr):
    """Postfix indexing ``base[index]`` (base-1)."""
    base: Expr
    index: Expr
    location: object = None


@dataclass
class Call(Expr):
    """``name(arg, …)`` — user function/procedure or a built-in primitive."""
    name: str
    args: List[Expr]
    location: object = None


@dataclass
class Unary(Expr):
    """A prefix operator: MINUS, NOT, or the bracket operators SQRT/⌊⌋/⌈⌉.

    Floor and ceil carry their operand directly (the closing bracket is
    consumed by the parser); ``op`` is LFLOOR / LCEIL / SQRT / MINUS / NOT.
    """
    op: TokenKind
    operand: Expr
    location: object = None


@dataclass
class Binary(Expr):
    """An infix operator (arithmetic, comparison, or logical)."""
    op: TokenKind
    left: Expr
    right: Expr
    location: object = None
