"""Acceptance test runner for the Nemi corpus (spec §17).

Self-contained (no pytest dependency): run

    python python/tests/run_corpus.py

from anywhere. Exits non-zero if any case fails. Every expected value is the
one *already verified in the course notes* (spec §17), plus a few extra checks
for behaviour the corpus exercises but does not spell out (bignum,
ASCII-equivalent operators, the non-executable prose actions of `enlosa`).
"""

import os
import sys

# Layout: <repo>/python/tests/run_corpus.py, with the corpus shared at
# <repo>/examples. Make the `nemi` package importable and locate the corpus.
_PKG_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # <repo>/python
_REPO_ROOT = os.path.dirname(_PKG_ROOT)                                  # <repo>
sys.path.insert(0, _PKG_ROOT)

import nemi  # noqa: E402  (import after sys.path tweak)
from nemi.values import to_python  # noqa: E402

EXAMPLES = os.path.join(_REPO_ROOT, "examples")


def _example(name):
    return os.path.join(EXAMPLES, name)


class Runner:
    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, label, actual, expected):
        ok = actual == expected
        mark = "PASS" if ok else "FAIL"
        print(f"  [{mark}] {label}: got {actual!r}, expected {expected!r}")
        if ok:
            self.passed += 1
        else:
            self.failed += 1

    def check_raises(self, label, fn, exc=nemi.ExecutionError):
        try:
            fn()
        except exc as err:
            print(f"  [PASS] {label}: raised {type(err).__name__} ({err})")
            self.passed += 1
        except Exception as err:  # noqa: BLE001
            print(f"  [FAIL] {label}: raised {type(err).__name__}, expected {exc.__name__}")
            self.failed += 1
        else:
            print(f"  [FAIL] {label}: did not raise")
            self.failed += 1


def transitive_closure_reference(matrix):
    """Independent reference for Warshall: reachability by paths of length ≥ 1."""
    n = len(matrix)
    result = [[0] * n for _ in range(n)]
    for start in range(n):
        # BFS over edges; a node counts as reachable only via a real edge.
        stack = [j for j in range(n) if matrix[start][j]]
        seen = set()
        while stack:
            node = stack.pop()
            if node in seen:
                continue
            seen.add(node)
            result[start][node] = 1
            for j in range(n):
                if matrix[node][j] and j not in seen:
                    stack.append(j)
    return result


def main():
    r = Runner()

    # -- 17.1 máximo --------------------------------------------------
    print("§17.1 máximo")
    m = nemi.load_files([_example("maximo.nemi")])
    r.check("máximo([3,9,4],3)", to_python(m.call("máximo", [[3, 9, 4], 3])), 9)

    # -- 17.2 busca_texto --------------------------------------------
    print("§17.2 busca_texto")
    bt = nemi.load_files([_example("busca_texto.nemi")])
    r.check("busca_texto('001',3,'010001',6)",
            to_python(bt.call("busca_texto", ["001", 3, "010001", 6])), 4)
    r.check("busca_texto miss", to_python(bt.call("busca_texto", ["999", 3, "010001", 6])), 0)

    # -- 17.3 inserción_por_orden (in-place procedure) ---------------
    print("§17.3 inserción_por_orden")
    ins = nemi.load_files([_example("insercion_por_orden.nemi")])
    arr = nemi.Array([34, 20, 19, 5])
    ins.call("inserción_por_orden", [arr, 4])
    r.check("[34,20,19,5] sorted in place", to_python(arr), [5, 19, 20, 34])

    # -- 17.4 es_primo ------------------------------------------------
    print("§17.4 es_primo")
    ep = nemi.load_files([_example("es_primo.nemi")])
    r.check("es_primo(97)", to_python(ep.call("es_primo", [97])), 0)
    r.check("es_primo(51)", to_python(ep.call("es_primo", [51])), 3)

    # -- 17.5 mcd (uses intercambia) ---------------------------------
    print("§17.5 mcd")
    mcd = nemi.load_files([_example("mcd.nemi")])
    r.check("mcd(504,396)", to_python(mcd.call("mcd", [504, 396])), 36)
    r.check("mcd(105,30)", to_python(mcd.call("mcd", [105, 30])), 15)
    r.check("mcd(396,504) [triggers swap]", to_python(mcd.call("mcd", [396, 504])), 36)

    # -- 17.6 / 17.8 factorial ---------------------------------------
    print("§17.6/17.8 factorial")
    fac = nemi.load_files([_example("factorial.nemi")])
    r.check("factorial(5)", to_python(fac.call("factorial", [5])), 120)
    faci = nemi.load_files([_example("factorial_iter.nemi")])
    r.check("factorial_iter(6)", to_python(faci.call("factorial_iter", [6])), 720)
    # bignum: 100! is far beyond 64-bit
    r.check("factorial(100) is bignum-correct",
            to_python(fac.call("factorial", [100])),
            93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000)

    # -- 17.7 / 17.9 fibonacci ---------------------------------------
    print("§17.7/17.9 fibonacci")
    fib = nemi.load_files([_example("fib.nemi")])
    r.check("fib(5)", to_python(fib.call("fib", [5])), 5)
    fibi = nemi.load_files([_example("fib_iter.nemi")])
    r.check("fib_iter(10)", to_python(fibi.call("fib_iter", [10])), 55)

    # -- 17.10 exp_rápida --------------------------------------------
    print("§17.10 exp_rápida")
    er = nemi.load_files([_example("exp_rapida.nemi")])
    r.check("exp_rápida(2,10)", to_python(er.call("exp_rápida", [2, 10])), 1024)

    # -- 17.11 exp_mod (RSA-style, needs bignum) ---------------------
    print("§17.11 exp_mod")
    em = nemi.load_files([_example("exp_mod.nemi")])
    r.check("exp_mod(572,29,713)", to_python(em.call("exp_mod", [572, 29, 713])), 113)
    r.check("exp_mod(3,13,7)", to_python(em.call("exp_mod", [3, 13, 7])), 3)

    # -- 17.12 Warshall (boolean matrix) -----------------------------
    print("§17.12 Warshall")
    war = nemi.load_files([_example("warshall.nemi")])
    matrix = [
        [0, 1, 0, 0],
        [0, 0, 1, 0],
        [0, 0, 0, 1],
        [0, 0, 0, 0],
    ]
    result = to_python(war.call("Warshall", [matrix, 4]))
    r.check("Warshall transitive closure",
            result, transitive_closure_reference(matrix))

    # -- 17.13 enlosa (prose actions ⇒ not executable past the base) --
    print("§17.13 enlosa (non-executable body)")
    enl = nemi.load_files([_example("enlosa.nemi")])
    r.check("enlosa parses (defined)", enl.has_function("enlosa"), True)
    r.check_raises("enlosa(4,·) hits undefined m1",
                   lambda: enl.call("enlosa", [4, 0]))

    # -- Extra: ASCII-equivalent operators lex to the same language ---
    # Uses <-, !=, <=, *, - (keyboard equivalents) plus the Spanish word 'y'.
    print("extra: ASCII-equivalent operators")
    ascii_src = (
        "función prueba(n)\n"
        "    resultado <- 1\n"
        "    para i <- 2 hasta n\n"
        "        si i != 0 y i <= n\n"
        "            resultado <- resultado * i\n"
        "        fin si\n"
        "    fin para\n"
        "    regresa resultado - 0\n"
        "fin función\n"
    )
    ascii_interp = nemi.load(ascii_src)
    r.check("ASCII ops factorial(6)=720", to_python(ascii_interp.call("prueba", [6])), 720)

    # -- extra: incluye (file inclusion; lets Nemi have a "standard library")
    print("extra: incluye")
    inc = nemi.load_files([_example("usa_biblioteca.nemi")])
    r.check("doble(21) via incluye", to_python(inc.call("doble", [21])), 42)
    r.check("es_par(4) via incluye", to_python(inc.call("es_par", [4])), True)

    r.check_raises(
        "incluye missing file",
        lambda: nemi.load(
            'incluye "no_existe_de_verdad.nemi"\n',
            _example("marcador_ficticio.nemi"),
        ),
        nemi.LexError,
    )

    r.check_raises(
        "incluye cyclic file",
        lambda: nemi.load_files([_example("incluye_ciclo_a.nemi")]),
        nemi.LexError,
    )

    # error location must point at the file that actually has the problem
    try:
        nemi.load(
            'incluye "biblioteca_rota.nemi"\n', _example("marcador_ficticio2.nemi")
        )
        r.check("incluye error names the broken file", False, True)
    except nemi.NemiError as err:
        r.check(
            "incluye error names the broken file",
            "biblioteca_rota.nemi" in str(err),
            True,
        )

    print()
    total = r.passed + r.failed
    print(f"{r.passed}/{total} checks passed.")
    return 0 if r.failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
