"""Resolves ``incluye "ruta.nemi"`` directives (spec §10/§12).

Recursively splices each included file's own tokens into the stream in
place, so the result reads as if the included text had been pasted right
there -- except every token still carries its *own* true origin (file+line),
so a later lexer/parser/interpreter error points at the file that actually
has the problem, not at some fused line count.

Kept separate from lexer.py on purpose: Lexer stays pure text -> tokens (no
filesystem access, easy to unit-test), and this module owns the file I/O,
path resolution, and recursion. Mirrors cpp/include/nemi/loader.hpp.
"""

import os

from .errors import LexicalError
from .lexer import Lexer
from .tokens import TokenKind


def _read_file(path):
    try:
        with open(path, encoding="utf-8") as handle:
            return handle.read()
    except OSError as exc:
        raise LexicalError(f"no se pudo abrir el archivo: {path}") from exc


def _resolve_include(base_dir, rel, search_paths):
    """Find the file an `incluye "rel"` should splice in.

    Tries `base_dir` first (today's only behavior, always wins if the file
    is there); then each directory in `search_paths`, in order (the `-I`
    CLI flag). Falls through to the `base_dir` candidate if nothing exists,
    so the caller's usual open()-fails error path fires unchanged.
    """
    candidate = os.path.join(base_dir, rel)
    if os.path.isfile(candidate):
        return candidate
    for directory in search_paths:
        candidate = os.path.join(directory, rel)
        if os.path.isfile(candidate):
            return candidate
    return os.path.join(base_dir, rel)


def tokenize_with_includes(source, base_path="", search_paths=()):
    """Tokenize `source`, splicing in `incluye "ruta"` targets recursively.

    Relative `incluye` paths resolve against the directory of `base_path`;
    if `base_path` is empty (an in-memory source with no file of its own,
    e.g. a `--llama` expression), they resolve against the current working
    directory. If not found there, each directory in `search_paths` (the
    `-I` CLI flag) is tried in order.
    """
    active = [os.path.realpath(base_path)] if base_path else []
    base_dir = os.path.dirname(base_path) if base_path else "."
    return _expand(source, base_dir or ".", base_path, active, search_paths)


def tokenize_file_with_includes(path, search_paths=()):
    """Read `path` from disk and tokenize it with tokenize_with_includes."""
    source = _read_file(path)
    active = [os.path.realpath(path)]
    base_dir = os.path.dirname(path) or "."
    return _expand(source, base_dir, path, active, search_paths)


def _expand(source, base_dir, file_tag, active, search_paths=()):
    tokens = Lexer(source, file_tag).tokenize()
    result = []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok.kind is not TokenKind.INCLUDE:
            result.append(tok)
            i += 1
            continue

        if i + 1 >= len(tokens) or tokens[i + 1].kind is not TokenKind.STRING:
            raise LexicalError(
                "se esperaba una cadena con la ruta después de 'incluye'",
                tok.location,
            )
        rel = tokens[i + 1].value
        i += 2

        resolved = _resolve_include(base_dir, rel, search_paths)
        key = os.path.realpath(resolved)
        if key in active:
            raise LexicalError(
                f"inclusión cíclica: '{rel}' ya se está incluyendo", tok.location
            )

        included_source = None
        try:
            with open(resolved, encoding="utf-8") as handle:
                included_source = handle.read()
        except OSError as exc:
            raise LexicalError(
                f"no se pudo abrir el archivo incluido: {rel}", tok.location
            ) from exc

        active.append(key)
        included = _expand(
            included_source, os.path.dirname(resolved) or ".", resolved, active,
            search_paths,
        )
        active.pop()

        # Drop the included file's own trailing EOF: it is spliced into the
        # middle of another stream, not the end of the whole program.
        if included and included[-1].kind is TokenKind.EOF:
            included = included[:-1]
        result.extend(included)
    return result
