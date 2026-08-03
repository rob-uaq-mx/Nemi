"""The Nemi lexer: turns UTF-8 source text into a list of tokens.

Design notes (see docs/ARCHITECTURE.md):

* The language is whitespace-insensitive: blocks are delimited by explicit
  ``fin`` closers (spec §8/§13), so newlines and indentation are discarded.
* Every operator has a canonical Unicode form and an ASCII keyboard
  equivalent (spec §9). Both lex to the same ``TokenKind``; the distinction
  is erased here so the parser never has to care.
* Identifiers may contain any Unicode letter (so ``función`` would lex as a
  word — it is then looked up in the keyword table — and ``máximo``/``raíz``
  are valid identifiers).
"""

from .errors import LexicalError, SourceLocation
from .tokens import Token, TokenKind

# Word -> keyword/operator kind. Both accented and unaccented spellings, and
# English logical words, are accepted for robustness (spec §9).
_KEYWORDS = {
    "función": TokenKind.FUNCTION, "funcion": TokenKind.FUNCTION,
    "procedimiento": TokenKind.PROCEDURE,
    "para": TokenKind.FOR,
    "hasta": TokenKind.TO,
    "repite": TokenKind.REPEAT,
    "cada": TokenKind.EACH,
    "en": TokenKind.WITHIN,
    "mientras": TokenKind.WHILE,
    "si": TokenKind.IF,
    "entonces": TokenKind.THEN,
    "alt": TokenKind.ELSE, "alternativamente": TokenKind.ELSE,  # else branch
    "regresa": TokenKind.RETURN,
    "fin": TokenKind.END,
    "incluye": TokenKind.INCLUDE,
    "afirma": TokenKind.ASSERT,
    "traza": TokenKind.TRACE,
    "mod": TokenKind.MOD,
    "y": TokenKind.AND,
    "o": TokenKind.OR,
    "no": TokenKind.NOT,
}

# Two-character ASCII operators, checked before single characters.
_TWO_CHAR = {
    "<-": TokenKind.ASSIGN,
    "!=": TokenKind.NE,
    "<=": TokenKind.LE,
    ">=": TokenKind.GE,
}

# Single characters (Unicode canonical forms and remaining ASCII equivalents).
_ONE_CHAR = {
    "←": TokenKind.ASSIGN,
    "=": TokenKind.EQ,
    "≠": TokenKind.NE,
    "<": TokenKind.LT,
    "≤": TokenKind.LE,
    ">": TokenKind.GT,
    "≥": TokenKind.GE,
    "∈": TokenKind.IN,
    "∉": TokenKind.NOTIN,
    "⊆": TokenKind.SUBSETEQ,
    "⊂": TokenKind.SUBSET,
    "+": TokenKind.PLUS,
    "−": TokenKind.MINUS, "-": TokenKind.MINUS,
    "·": TokenKind.TIMES, "*": TokenKind.TIMES,
    "/": TokenKind.DIVIDE,
    "∧": TokenKind.AND,
    "∨": TokenKind.OR,
    "¬": TokenKind.NOT,
    "(": TokenKind.LPAREN,
    ")": TokenKind.RPAREN,
    "[": TokenKind.LBRACKET,
    "]": TokenKind.RBRACKET,
    "⌊": TokenKind.LFLOOR,
    "⌋": TokenKind.RFLOOR,
    "⌈": TokenKind.LCEIL,
    "⌉": TokenKind.RCEIL,
    "√": TokenKind.SQRT,
    ",": TokenKind.COMMA,
    "{": TokenKind.LBRACE,
    "}": TokenKind.RBRACE,
    "∅": TokenKind.EMPTYSET,
}

_COMMENT_CHARS = ("#", "▷")


class Lexer:
    """Scans a source string once and produces a token list.

    Single-pass, single character of lookahead. The public entry point is
    :meth:`tokenize`, which always terminates the stream with an ``EOF`` token.
    """

    def __init__(self, source, file=None):
        self._src = source
        self._file = file  # tags every token's SourceLocation (see loader.py)
        self._pos = 0
        self._line = 1
        self._col = 1

    # -- low-level cursor helpers ---------------------------------------
    def _at_end(self):
        return self._pos >= len(self._src)

    def _peek(self, ahead=0):
        i = self._pos + ahead
        return self._src[i] if i < len(self._src) else ""

    def _advance(self):
        ch = self._src[self._pos]
        self._pos += 1
        if ch == "\n":
            self._line += 1
            self._col = 1
        else:
            self._col += 1
        return ch

    def _loc(self):
        return SourceLocation(self._line, self._col, self._file)

    # -- main loop ------------------------------------------------------
    def tokenize(self):
        tokens = []
        while True:
            self._skip_trivia()
            if self._at_end():
                tokens.append(Token(TokenKind.EOF, None, self._loc()))
                return tokens
            tokens.append(self._next_token())

    def _skip_trivia(self):
        """Consume whitespace and comments (comment runs to end of line)."""
        while not self._at_end():
            ch = self._peek()
            if ch in " \t\r\n":
                self._advance()
            elif ch in _COMMENT_CHARS:
                while not self._at_end() and self._peek() != "\n":
                    self._advance()
            else:
                return

    def _next_token(self):
        start = self._loc()
        ch = self._peek()

        if ch == '"':
            return self._string(start)
        if ch == "«":
            return self._prose(start)
        if ch.isdigit():
            return self._number(start)
        if ch.isalpha() or ch == "_":
            return self._word(start)

        # two-character ASCII operators
        pair = ch + self._peek(1)
        if pair in _TWO_CHAR:
            self._advance()
            self._advance()
            return Token(_TWO_CHAR[pair], None, start)

        if ch in _ONE_CHAR:
            self._advance()
            return Token(_ONE_CHAR[ch], None, start)

        raise LexicalError(f"carácter inesperado {ch!r}", start)

    # -- token scanners -------------------------------------------------
    def _number(self, start):
        digits = []
        while not self._at_end() and self._peek().isdigit():
            digits.append(self._advance())
        # a real number: digits '.' digits (a trailing dot is not consumed)
        if self._peek() == "." and self._peek(1).isdigit():
            digits.append(self._advance())  # the dot
            while not self._at_end() and self._peek().isdigit():
                digits.append(self._advance())
            return Token(TokenKind.REAL, float("".join(digits)), start)
        return Token(TokenKind.INT, int("".join(digits)), start)

    def _word(self, start):
        chars = []
        while not self._at_end():
            ch = self._peek()
            if ch.isalnum() or ch == "_":
                chars.append(self._advance())
            else:
                break
        word = "".join(chars)
        kind = _KEYWORDS.get(word)
        if kind is not None:
            return Token(kind, None, start)
        return Token(TokenKind.IDENT, word, start)

    def _string(self, start):
        self._advance()  # opening quote
        chars = []
        while True:
            if self._at_end():
                raise LexicalError("literal de cadena sin cerrar", start)
            ch = self._advance()
            if ch == '"':
                break
            if ch == "\\" and not self._at_end():
                # minimal escape support: \" and \\
                nxt = self._advance()
                chars.append(nxt)
            else:
                chars.append(ch)
        return Token(TokenKind.STRING, "".join(chars), start)

    def _prose(self, start):
        self._advance()  # opening «
        chars = []
        while True:
            if self._at_end():
                raise LexicalError("acción en prosa « … » sin cerrar", start)
            ch = self._advance()
            if ch == "»":
                break
            chars.append(ch)
        return Token(TokenKind.PROSE, "".join(chars).strip(), start)


def tokenize(source, file=None):
    """Convenience wrapper: ``tokenize(source) -> list[Token]``."""
    return Lexer(source, file).tokenize()
