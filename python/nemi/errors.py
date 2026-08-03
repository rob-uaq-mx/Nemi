"""Exception hierarchy for the Nemi interpreter.

Kept deliberately small and closed. In a C++ port these map naturally to a
base ``NemiError`` class with derived ``LexicalError`` / ``ParseError`` /
``ExecutionError`` classes, each carrying an optional source location.
"""


class SourceLocation:
    """A 1-based (line, column) position in a source file.

    A plain value object. ``column`` may be ``None`` when only the line is
    known (or vice versa). Kept separate from the exception so the same type
    can later annotate tokens and AST nodes.
    """

    __slots__ = ("line", "column", "file")

    def __init__(self, line=None, column=None, file=None):
        self.line = line
        self.column = column
        self.file = file  # None/empty == unknown/in-memory source (e.g. --llama text)

    def __str__(self):
        if self.line is None:
            return "<unknown>"
        prefix = f"{self.file}, " if self.file else ""
        if self.column is None:
            return f"{prefix}{self.line}"
        return f"{prefix}{self.line}:{self.column}"

    def __repr__(self):
        return f"@{self.line}:{self.column}"


class NemiError(Exception):
    """Base class for every error raised by the interpreter."""

    def __init__(self, message, location=None):
        self.message = message
        self.location = location
        if location is not None and location.line is not None:
            super().__init__(f"{message} ({location})")
        else:
            super().__init__(message)


class LexicalError(NemiError):
    """Raised by the lexer for malformed tokens."""


class ParseError(NemiError):
    """Raised by the parser for grammar violations."""


class ExecutionError(NemiError):
    """Raised during evaluation (type errors, division by zero, ...)."""
