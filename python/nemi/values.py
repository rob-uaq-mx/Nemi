"""Runtime value model for Nemi.

The value types are:

    * ``int``       — arbitrary-precision integer (bignum, spec §8/§11).
    * ``Fraction``  — exact rational, produced by ``/`` on two integers so that
                      ``⌊n / 2⌋`` stays exact for bignum ``n``.
    * ``float``     — real literals and non-exact ``√``.
    * ``bool``      — boolean; ``0`` / ``1`` integers also serve as bits (§11).
    * ``str``       — a string; indexable base-1, yields one-character strings.
    * ``Array``     — a mutable, base-1 sequence; matrices are Arrays of Arrays.
    * ``Conjunto``  — an unordered set with no duplicates (spec §20.2, v0.2).

:class:`Array` and :class:`Conjunto` are custom types; the rest are Python
built-ins whose semantics already match Nemi (notably unbounded ``int``). In a
C++ port these become a tagged ``Value`` variant over ``BigInt``, ``Rational``,
``double``, ``bool``, ``std::string``, ``Array`` and ``Conjunto``.
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
            raise ExecutionError(f"el índice de arreglo debe ser un entero, se obtuvo {index!r}")
        if index < 1 or index > len(self._items):
            raise ExecutionError(
                f"índice {index} fuera de rango 1..{len(self._items)}"
            )
        return index - 1

    def get(self, index):
        return self._items[self._checked_index(index)]

    def set(self, index, value):
        self._items[self._checked_index(index)] = value

    def append(self, value):
        """Grow by one element at the end (§20.4 v0.2 ``agrega`` on a lista)."""
        self._items.append(value)

    def items(self):
        """The underlying items, in order (read-only view for formatting)."""
        return self._items

    def __eq__(self, other):
        return isinstance(other, Array) and self._items == other._items

    def __repr__(self):
        return f"Array({self._items!r})"


def _canonical_key(value):
    """Sort key for the canonical order of §20.2: numbers ascending, then
    strings lexicographic, then compound values (Array/Conjunto) sorted by
    their printed representation. Only used for deterministic iteration and
    printing of a Conjunto -- not a claim of a "true" mathematical order.
    """
    if isinstance(value, (bool, int, Fraction, float)):
        return (0, float(value))
    if isinstance(value, str):
        return (1, value)
    return (2, format_value(value))  # Array / Conjunto


class Conjunto:
    """An unordered set with no duplicates (spec §20.2, v0.2).

    Elements can be any Nemi value, including Arrays and other Conjuntos, so
    membership can't use Python's own hash-based ``set`` (Arrays are mutable
    and unhashable). Instead this keeps a flat list in **canonical order**
    (:func:`_canonical_key`) and does structural-equality membership checks
    (O(n) per insert) -- fine at the scale a teaching language runs at, and
    it makes equality/iteration trivial: two Conjuntos with the same elements
    always end up with identical, order-independent internal lists.
    """

    __slots__ = ("_items",)

    def __init__(self, items=None):
        self._items = []
        if items is not None:
            for x in items:
                self.add(x)

    def add(self, x):
        """Idempotent insert (§20.2/§20.4 ``agrega`` on a set)."""
        if not self.contains(x):
            self._items.append(x)
            self._items.sort(key=_canonical_key)

    def contains(self, x):
        return any(x == existing for existing in self._items)

    def items(self):
        """Elements in canonical order (read-only view for formatting/iteration)."""
        return self._items

    def cardinality(self):
        return len(self._items)

    def union(self, other):
        result = Conjunto(self._items)
        for x in other._items:
            result.add(x)
        return result

    def intersection(self, other):
        return Conjunto(x for x in self._items if other.contains(x))

    def difference(self, other):
        return Conjunto(x for x in self._items if not other.contains(x))

    def is_subset(self, other):
        return all(other.contains(x) for x in self._items)

    def is_proper_subset(self, other):
        return self.is_subset(other) and self.cardinality() < other.cardinality()

    def __eq__(self, other):
        return isinstance(other, Conjunto) and self._items == other._items

    def __repr__(self):
        return f"Conjunto({self._items!r})"


# --------------------------------------------------------------------------
# Conversions between Nemi values and plain Python objects (host boundary)
# --------------------------------------------------------------------------
def from_python(obj):
    """Wrap a Python object as a Nemi value (lists become Arrays, recursively;
    Python ``set``/``frozenset`` become Conjuntos)."""
    if isinstance(obj, list):
        return Array([from_python(x) for x in obj])
    if isinstance(obj, (set, frozenset)):
        return Conjunto(from_python(x) for x in obj)
    return obj


def to_python(value):
    """Unwrap a Nemi value to a plain Python object (Arrays become lists,
    Conjuntos become lists too -- Nemi elements aren't necessarily hashable,
    so a Python ``set`` isn't always representable; the list is in canonical
    order)."""
    if isinstance(value, Array):
        return [to_python(x) for x in value.items()]
    if isinstance(value, Conjunto):
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
    raise ExecutionError(f"el valor no es una condición booleana: {format_value(value)}")


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
    if isinstance(value, Conjunto):
        # Canonical order (§20.2); the *empty* set prints "{}", never "∅" --
        # that glyph is reserved for "no value" below (§20.1/§21.1 double
        # use of ∅, resolved: same glyph, different meaning by context).
        return "{" + ", ".join(format_value(x) for x in value.items()) + "}"
    if value is None:
        return "∅"  # a procedure / value-less return
    return str(value)
