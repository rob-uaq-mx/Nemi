"""Command-line front end: ``python -m nemi``.

Because Nemi is a Spanish-keyword language, the CLI options are Spanish too:

    python -m nemi FILE.nemi [FILE2.nemi ...] [--llama "expr" ...]
    python -m nemi FILE.nemi --lexemas       # dump the token stream
    python -m nemi FILE.nemi --asa            # dump the parsed definitions (AST)

Loading a file runs its top-level statements (the "script body") in source
order; a bare top-level call is evaluated for effect but not printed, so use
``imprime(...)`` to produce output. Each ``--llama`` expression is then
evaluated against the loaded definitions and its result printed; array/string
literals may be used to build arguments, e.g.

    python -m nemi ../examples/maximo.nemi --llama "máximo([3, 9, 4], 3)"
"""

import argparse
import sys

from . import load_files, run_call
from .errors import NemiError
from .loader import tokenize_file_with_includes
from .parser import parse
from .values import format_value


def _force_utf8():
    """Windows consoles default to cp1252; Nemi source and output are UTF-8."""
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            reconfigure(encoding="utf-8")


def _build_parser():
    parser = argparse.ArgumentParser(
        prog="nemi", description="Intérprete de Nemi", add_help=False
    )
    parser.add_argument(
        "-a", "--ayuda", action="help",
        help="muestra esta ayuda y termina",
    )
    parser.add_argument(
        "archivos", nargs="+", metavar="ARCHIVO.nemi",
        help="uno o más archivos fuente .nemi",
    )
    parser.add_argument(
        "--llama", action="append", default=[], metavar="EXPRESIÓN",
        help="evalúa una expresión de llamada contra las definiciones cargadas",
    )
    parser.add_argument(
        "--lexemas", action="store_true",
        help="imprime los lexemas (tokens) y termina",
    )
    parser.add_argument(
        "--asa", action="store_true",
        help="imprime el árbol sintáctico abstracto (ASA) y termina",
    )
    return parser


def main(argv=None):
    _force_utf8()
    parser = _build_parser()
    args = parser.parse_args(argv)

    try:
        if args.lexemas:
            for path in args.archivos:
                for tok in tokenize_file_with_includes(path):
                    print(tok)
            return 0

        if args.asa:
            for path in args.archivos:
                for defn in parse(tokenize_file_with_includes(path)).definitions:
                    print(defn)
            return 0

        interp = load_files(args.archivos)
        interp.run_program()  # execute any top-level statements (the script body)
        for expr in args.llama:
            result = run_call(interp, expr)
            print(format_value(result))
        return 0

    except NemiError as err:
        print(f"nemi: {err}", file=sys.stderr)
        return 1
    except OSError as err:
        print(f"nemi: {err}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
