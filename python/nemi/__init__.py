"""Nemi — an interpreter for the Nemi algorithm pseudo-language.

Public API
----------
``load(source)``          parse source text into an :class:`Interpreter`.
``load_files(paths)``     parse and merge several ``.nemi`` files.
``run_call(interp, text)``parse and evaluate a single call expression.
``Interpreter``           the evaluator (``interp.call(name, args)``).
``Array``                 the base-1 array value type.
``format_value``          render a Nemi value as text.

Errors are subclasses of :class:`NemiError` (``ParseError`` / ``ExecutionError``
/ ``LexError``).
"""

from .errors import (
    ExecutionError,
    LexicalError as LexError,
    NemiError,
    ParseError,
)
from .interpreter import Interpreter
from .lexer import tokenize
from .loader import tokenize_file_with_includes, tokenize_with_includes
from .parser import Parser, parse
from .values import Array, format_value, from_python, to_python

__all__ = [
    "load", "load_files", "run_call",
    "Interpreter", "Array", "format_value", "from_python", "to_python",
    "NemiError", "ParseError", "ExecutionError", "LexError",
    "tokenize", "parse",
    "tokenize_with_includes", "tokenize_file_with_includes",
]

__version__ = "0.1.0"


def load(source, source_path="", include_paths=()):
    """Parse Nemi source text and return a ready-to-use :class:`Interpreter`.

    ``source_path``, if given, tags error locations with a file name and is
    the base for resolving any relative ``incluye "ruta"`` directive in
    ``source`` (otherwise: the current working directory). ``include_paths``
    lists extra directories (the ``-I`` CLI flag) tried, in order, if an
    ``incluye`` target isn't found next to its own file.
    """
    return Interpreter(
        parse(tokenize_with_includes(source, source_path, include_paths))
    )


def load_files(paths, include_paths=()):
    """Parse and merge several source files into one :class:`Interpreter`.

    Each file's ``incluye "ruta"`` directives resolve relative to its own
    directory, falling back to ``include_paths`` (the ``-I`` CLI flag) in
    order if not found there.
    """
    from .ast_nodes import Program

    merged = Program()
    for path in paths:
        program = parse(tokenize_file_with_includes(path, include_paths))
        merged.definitions.extend(program.definitions)
        merged.main.extend(program.main)  # top-level statements run in file order
    return Interpreter(merged)


def run_call(interp, call_text):
    """Evaluate a single call/expression (e.g. ``"máximo([3,9,4],3)"``).

    Uses the same expression grammar as the language, so array and string
    literals are available for building inputs. Returns the resulting value.
    """
    from .interpreter import Environment

    tokens = tokenize(call_text)
    expr = Parser(tokens)._expression()  # parse just one expression
    return interp._eval(expr, Environment())
