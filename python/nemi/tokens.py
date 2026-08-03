"""Token kinds and the ``Token`` value type.

``TokenKind`` is an ``enum.Enum`` so it maps directly onto a C++
``enum class TokenKind``. Note that the *keywords are Spanish* because they
belong to the Nemi language itself; the enum *names* are English so the
implementation reads naturally.
"""

import enum


class TokenKind(enum.Enum):
    # --- Keywords -------------------------------------------------------
    FUNCTION = "function"      # función
    PROCEDURE = "procedure"    # procedimiento
    FOR = "for"                # para
    TO = "to"                  # hasta
    REPEAT = "repeat"          # repite   (optional syntactic sugar)
    EACH = "each"              # cada     (v0.2 §20.3, "para cada ... en ...")
    WITHIN = "within"          # en       (v0.2 §20.3, "para cada ... en ...";
                                #           also usable as an ASCII-friendly
                                #           word for membership, a synonym of
                                #           IN/∈ at comparison level -- see
                                #           _COMPARISONS in parser.py)
    WHILE = "while"            # mientras
    IF = "if"                  # si
    THEN = "then"              # entonces (optional syntactic sugar)
    ELSE = "else"              # alt / alternativamente
    RETURN = "return"          # regresa
    END = "end"                # fin
    INCLUDE = "include"        # incluye
    ASSERT = "assert"          # afirma   (v0.2 §21.2, autoverificación)
    TRACE = "trace"            # traza    (v0.2 §21.3 [OPC], herramienta de
                                #           comprensión)

    # --- Operators (logical words map here too: y/o/no, and/or/not) -----
    ASSIGN = "assign"          # ←  or  <-
    EQ = "eq"                  # =
    NE = "ne"                  # ≠  or  !=
    LT = "lt"                  # <
    LE = "le"                  # ≤  or  <=
    GT = "gt"                  # >
    GE = "ge"                  # ≥  or  >=
    IN = "in"                  # ∈  (pertenece; v0.2 §20.2)
    NOTIN = "notin"            # ∉  (v0.2 §20.2)
    SUBSETEQ = "subseteq"      # ⊆  (subconjunto; v0.2 §20.2)
    SUBSET = "subset"          # ⊂  (subconjunto propio; v0.2 §20.2)
    PLUS = "plus"              # +
    MINUS = "minus"            # −  or  -
    TIMES = "times"            # ·  or  *
    DIVIDE = "divide"          # /
    MOD = "mod"                # mod
    AND = "and"                # ∧  or the word  y
    OR = "or"                  # ∨  or the word  o
    NOT = "not"                # ¬  or the word  no

    # --- Bracketing operators ------------------------------------------
    LPAREN = "lparen"          # (
    RPAREN = "rparen"          # )
    LBRACKET = "lbracket"      # [
    RBRACKET = "rbracket"      # ]
    LFLOOR = "lfloor"          # ⌊
    RFLOOR = "rfloor"          # ⌋
    LCEIL = "lceil"            # ⌈
    RCEIL = "rceil"            # ⌉
    SQRT = "sqrt"              # √
    COMMA = "comma"            # ,
    LBRACE = "lbrace"          # {   (v0.2 §20.1, literal de conjunto)
    RBRACE = "rbrace"          # }   (v0.2 §20.1)
    EMPTYSET = "emptyset"      # ∅   (v0.2 §20.1: literal de conjunto vacío --
                                #      distinto del uso de ∅ como texto impreso
                                #      de "sin valor", ver values.format_value)

    # --- Literals and names --------------------------------------------
    INT = "int"                # 42
    REAL = "real"              # 8.3
    STRING = "string"          # "001"
    IDENT = "ident"            # máximo, s, W ...
    PROSE = "prose"            # « ... »  (non-executable prose action)

    EOF = "eof"


class Token:
    """A lexical token: a kind, its literal value, and a source location.

    ``value`` holds the semantic payload: an ``int`` for INT, ``float`` for
    REAL, ``str`` for STRING/IDENT/PROSE, and ``None`` for pure operators and
    keywords (the kind is enough).
    """

    __slots__ = ("kind", "value", "location")

    def __init__(self, kind, value, location):
        self.kind = kind
        self.value = value
        self.location = location

    def __repr__(self):
        if self.value is None:
            return f"Token({self.kind.name})"
        return f"Token({self.kind.name}, {self.value!r})"
