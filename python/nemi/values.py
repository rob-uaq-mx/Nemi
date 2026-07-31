"""Runtime value model for Nemi.

The value types are:

    * ``int``       — arbitrary-precision integer (bignum, spec §8/§11).
    * ``Fraction``  — exact rational, produced by ``/`` on two integers so that
                      ``⌊n / 2⌋`` stays exact for bignum ``n``.
    * ``float``     — real literals and non-exact ``√``.
    * ``bool``      — boolean; ``0`` / ``1`` integers also serve as bits (§11).
    * ``str``       — a string; indexable base-1, yields one-character strings.
    * ``Array``     — a mutable, base-1 sequence; matrices are Arrays of Arrays.

Only :class:`Array` is a custom type; the rest are Python built-ins whose
semantics already match Nemi (notably unbounded ``int``). In a C++ port these
become a tagged ``Value`` variant over ``BigInt``, ``Rational``, ``double``,
``bool``, ``std::string`` and ``Array``.
"""

from fractions import Fraction

from .errors import ExecutionError


class Array:
    """A mutable sequence with **1-based** indexing (spec §8/§11).

    Internally backed by a 0-based Python list; the base-1 convention is
    applied at the boundary in :meth:`get` / :meth:`set`. Arrays are reference
    types: passing one to a function shares it (parameter passing §12), which
    is what lets ``inserción_por_orden`` sort in place.
    """

    __slots__ = ("_items",)

    def __init__(self, items=None):
        self._items = list(items) if items is not None else []

    def length(self):
        return len(self._items)

    def _checked_index(self, index):
        if not isinstance(index, int) or isinstance(index, bool):
            raise ExecutionError(f"array index must be an integer, got {index!r}")
        if index < 1 or index > len(self._items):
            raise ExecutionError(
                f"index {index} out of range 1..{len(self._items)}"
            )
        return index - 1

    def get(self, index):
        return self._items[self._checked_index(index)]

    def set(self, index, value):
        self._items[self._checked_index(index)] = value

    def items(self):
        """The underlying items, in order (read-only view for formatting)."""
        return self._items

    def __eq__(self, other):
        return isinstance(other, Array) and self._items == other._items

    def __repr__(self):
        return f"Array({self._items!r})"


# --------------------------------------------------------------------------
# Conversions between Nemi values and plain Python objects (host boundary)
# --------------------------------------------------------------------------
def from_python(obj):
    """Wrap a Python object as a Nemi value (lists become Arrays, recursively)."""
    if isinstance(obj, list):
        return Array([from_python(x) for x in obj])
    return obj


def to_python(value):
    """Unwrap a Nemi value to a plain Python object (Arrays become lists)."""
    if isinstance(value, Array):
        return [to_python(x) for x in value.items()]
    return value


# --------------------------------------------------------------------------
# Truthiness and formatting
# --------------------------------------------------------------------------
def is_truthy(value):
    """Interpret a value as a condition (spec §12).

    Booleans map directly; a bit/number is true iff non-zero. Strings and
    arrays are not conditions.
    """
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, Fraction, float)):
        return value != 0
    raise ExecutionError(f"value is not a boolean condition: {format_value(value)}")


def format_value(value):
    """Human-readable rendering used by the CLI and ``imprime``."""
    if isinstance(value, bool):
        return "verdadero" if value else "falso"
    if isinstance(value, Fraction):
        return str(value.numerator) if value.denominator == 1 else str(value)
    if isinstance(value, float):
        return repr(value)
    if isinstance(value, str):
        return f'"{value}"'
    if isinstance(value, Array):
        return "[" + ", ".join(format_value(x) for x in value.items()) + "]"
    if value is None:
        return "∅"  # a procedure / value-less return
    return str(value)
