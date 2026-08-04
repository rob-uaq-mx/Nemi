"""Acceptance test runner for the Nemi corpus (spec §17).

Self-contained (no pytest dependency): run

    python python/tests/run_corpus.py

from anywhere. Exits non-zero if any case fails. Every expected value is the
one *already verified in the course notes* (spec §17), plus a few extra checks
for behaviour the corpus exercises but does not spell out (bignum,
ASCII-equivalent operators, the non-executable prose actions of `enlosa`).
"""

import contextlib
import io
import os
import sys

# Layout: <repo>/python/tests/run_corpus.py, with the corpus shared at
# <repo>/examples. Make the `nemi` package importable and locate the corpus.
_PKG_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # <repo>/python
_REPO_ROOT = os.path.dirname(_PKG_ROOT)                                  # <repo>
sys.path.insert(0, _PKG_ROOT)

import nemi  # noqa: E402  (import after sys.path tweak)
from nemi.values import format_value, to_python  # noqa: E402

EXAMPLES = os.path.join(_REPO_ROOT, "examples")
BIBCOM = os.path.join(_REPO_ROOT, "bibcom")


def _example(name):
    return os.path.join(EXAMPLES, name)


def _bibcom(name):
    return os.path.join(BIBCOM, name)


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

    # -I / include_paths: fallback search path when the file isn't next to
    # the one including it (see README.md, "-I / --incluye-dir")
    incluye_dir_externo = _example("incluye_dir_externo")
    via_dir = nemi.load_files(
        [_example("usa_incluye_dir.nemi")], include_paths=[incluye_dir_externo]
    )
    r.check("triple(7) via -I", to_python(via_dir.call("triple", [7])), 21)

    r.check_raises(
        "incluye not found without matching -I",
        lambda: nemi.load_files([_example("usa_incluye_dir.nemi")]),
        nemi.LexError,
    )

    # -- extra: Conjunto (v0.2 §20.1-§20.2) --------------------------------
    print("extra: Conjunto (v0.2)")
    conjunto_src = """
función literal_con_dup()
    regresa {3, 1, 2, 1}
fin función
función pertenece_si()
    regresa 2 ∈ {1, 2, 3}
fin función
función pertenece_no()
    regresa 5 ∈ {1, 2, 3}
fin función
función pertenece_si_en()
    regresa 2 en {1, 2, 3}
fin función
función pertenece_no_en()
    regresa 5 en {1, 2, 3}
fin función
función no_pertenece()
    regresa 5 ∉ {1, 2, 3}
fin función
función vacio_literal()
    regresa ∅
fin función
función es_subconjunto()
    regresa {1, 2} ⊆ {1, 2, 3}
fin función
función no_es_subconjunto_propio_de_si()
    regresa {1, 2, 3} ⊂ {1, 2, 3}
fin función
función igualdad_sin_importar_orden()
    regresa {1, 2, 3} = {3, 2, 1}
fin función
función conjunto_anidado()
    regresa {1, 2} ∈ {{1, 2}, {3, 4}}
fin función
"""
    cset = nemi.load(conjunto_src)
    r.check(
        "{3,1,2,1} collapses to {1,2,3}",
        to_python(cset.call("literal_con_dup", [])),
        [1, 2, 3],
    )
    r.check("2 ∈ {1,2,3}", cset.call("pertenece_si", []), True)
    r.check("5 ∈ {1,2,3} is false", cset.call("pertenece_no", []), False)
    r.check("2 en {1,2,3} (ASCII word for ∈)", cset.call("pertenece_si_en", []), True)
    r.check("5 en {1,2,3} is false", cset.call("pertenece_no_en", []), False)
    r.check("5 ∉ {1,2,3}", cset.call("no_pertenece", []), True)
    r.check("∅ literal is a Conjunto", to_python(cset.call("vacio_literal", [])), [])
    r.check("{1,2} ⊆ {1,2,3}", cset.call("es_subconjunto", []), True)
    r.check(
        "{1,2,3} ⊂ {1,2,3} is false (not proper)",
        cset.call("no_es_subconjunto_propio_de_si", []),
        False,
    )
    r.check(
        "conjunto equality ignores insertion order",
        cset.call("igualdad_sin_importar_orden", []),
        True,
    )
    r.check("nested conjunto membership", cset.call("conjunto_anidado", []), True)

    # ∅ keeps its two meanings straight (§20.1): printed "no value" vs. the
    # empty-set literal print as "{}", never confused with each other.
    no_value_interp = nemi.load("procedimiento p() regresa fin procedimiento")
    r.check(
        "no-value result formats as ∅",
        format_value(no_value_interp.call("p", [])),
        "∅",
    )
    r.check(
        "empty-set literal formats as {}, not ∅",
        format_value(cset.call("vacio_literal", [])),
        "{}",
    )

    # primitives: pertenece/subconjunto/union/interseccion/diferencia/cardinalidad
    prim_interp = nemi.load(
        "función u() regresa union({1,2}, {2,3}) fin función\n"
        "función i() regresa interseccion({1,2,3}, {2,3,4}) fin función\n"
        "función d() regresa diferencia({1,2,3}, {2}) fin función\n"
        "función c() regresa cardinalidad({1,2,3}) fin función\n"
        "función p() regresa pertenece(2, {1,2,3}) fin función\n"
        "función s() regresa subconjunto({1,2}, {1,2,3}) fin función\n"
        "función l() regresa long({1,2,3}) fin función\n"
        "función u2() regresa unión({1,2}, {2,3}) fin función\n"
        "función i2() regresa intersección({1,2,3}, {2,3,4}) fin función\n"
    )
    r.check("union({1,2},{2,3})", to_python(prim_interp.call("u", [])), [1, 2, 3])
    r.check("interseccion({1,2,3},{2,3,4})", to_python(prim_interp.call("i", [])), [2, 3])
    r.check("diferencia({1,2,3},{2})", to_python(prim_interp.call("d", [])), [1, 3])
    r.check("cardinalidad({1,2,3})", prim_interp.call("c", []), 3)
    r.check("pertenece(2, {1,2,3})", prim_interp.call("p", []), True)
    r.check("subconjunto({1,2}, {1,2,3})", prim_interp.call("s", []), True)
    r.check("long extended to conjunto", prim_interp.call("l", []), 3)
    # unión/intersección (with the required accent) are aliases of the same
    # primitives -- union/interseccion (unaccented) keep working too.
    r.check("unión({1,2},{2,3})", to_python(prim_interp.call("u2", [])), [1, 2, 3])
    r.check("intersección({1,2,3},{2,3,4})", to_python(prim_interp.call("i2", [])), [2, 3])

    # -- extra: para cada ... en ... (v0.2 §20.3) ---------------------------
    print("extra: para cada (v0.2)")
    foreach_interp = nemi.load(
        "función suma_lista(s)\n"
        "    total <- 0\n"
        "    para cada elem en s repite\n"
        "        total <- total + elem\n"
        "    fin para\n"
        "    regresa total\n"
        "fin función\n"
        "función suma_conjunto(c)\n"
        "    total <- 0\n"
        "    para cada elem en c repite\n"
        "        total <- total + elem\n"
        "    fin para\n"
        "    regresa total\n"
        "fin función\n"
        # Encodes visitation order as a single number (elem 3, then 1, then
        # 2 -> 312) since `agrega` (Fase 3) isn't implemented yet to collect
        # a real list of what was seen.
        "función orden_lista(s)\n"
        "    v <- 0\n"
        "    para cada elem en s repite\n"
        "        v <- v * 10 + elem\n"
        "    fin para\n"
        "    regresa v\n"
        "fin función\n"
        "función orden_conjunto(c)\n"
        "    v <- 0\n"
        "    para cada elem en c repite\n"
        "        v <- v * 10 + elem\n"
        "    fin para\n"
        "    regresa v\n"
        "fin función\n"
    )
    r.check(
        "para cada over a list sums all elements",
        to_python(foreach_interp.call("suma_lista", [[1, 2, 3, 4]])),
        10,
    )
    r.check(
        "para cada over a conjunto sums all elements",
        to_python(foreach_interp.call("suma_conjunto", [{1, 2, 3, 4}])),
        10,
    )
    r.check(
        "para cada over a list visits in index order",
        to_python(foreach_interp.call("orden_lista", [[3, 1, 2]])),
        312,
    )
    r.check(
        "para cada over a conjunto visits in canonical order",
        to_python(foreach_interp.call("orden_conjunto", [{3, 1, 2}])),
        123,
    )
    r.check_raises(
        "para cada over a non-collection raises",
        lambda: nemi.load(
            "función f()\n"
            "    para cada elem en 5 repite\n"
            "        z <- elem\n"
            "    fin para\n"
            "fin función\n"
        ).call("f", []),
        nemi.ExecutionError,
    )

    # -- extra: listas dinámicas -- agrega/copia/arreglo_cero/matriz_cero (v0.2 §20.4)
    print("extra: listas dinámicas (v0.2)")
    dyn_interp = nemi.load(
        "función agrega_lista(s, x)\n"
        "    agrega(s, x)\n"
        "    regresa s\n"
        "fin función\n"
        "función agrega_conjunto(c, x)\n"
        "    agrega(c, x)\n"
        "    regresa c\n"
        "fin función\n"
        "función copia_independiente()\n"
        "    original <- [1, 2, 3]\n"
        "    copiado <- copia(original)\n"
        "    copiado[1] <- 99\n"
        "    regresa original\n"  # unaffected if copia is truly independent
        "fin función\n"
        "función copia_profunda()\n"
        "    interno <- [1, 2]\n"
        "    original <- [interno]\n"
        "    copiado <- copia(original)\n"
        "    copiado[1][1] <- 99\n"
        "    regresa original\n"  # unaffected even through the nested Array
        "fin función\n"
        "función arreglo_cero_prueba(n)\n"
        "    regresa arreglo_cero(n)\n"
        "fin función\n"
        "función matriz_cero_prueba(m, n)\n"
        "    regresa matriz_cero(m, n)\n"
        "fin función\n"
    )
    r.check(
        "agrega grows a list, mutating by reference",
        to_python(dyn_interp.call("agrega_lista", [nemi.Array([1, 2]), 3])),
        [1, 2, 3],
    )
    r.check(
        "agrega on a conjunto is idempotent (already-present element)",
        to_python(dyn_interp.call("agrega_conjunto", [{1, 2}, 2])),
        [1, 2],
    )
    r.check(
        "copia leaves the original untouched",
        to_python(dyn_interp.call("copia_independiente", [])),
        [1, 2, 3],
    )
    r.check(
        "copia is deep (nested Array untouched too)",
        to_python(dyn_interp.call("copia_profunda", [])),
        [[1, 2]],
    )
    r.check(
        "arreglo_cero(5)", to_python(dyn_interp.call("arreglo_cero_prueba", [5])), [0] * 5
    )
    r.check(
        "matriz_cero(2,3)",
        to_python(dyn_interp.call("matriz_cero_prueba", [2, 3])),
        [[0, 0, 0], [0, 0, 0]],
    )
    r.check_raises(
        "agrega on a scalar raises",
        lambda: dyn_interp.call("agrega_lista", [5, 1]),
        nemi.ExecutionError,
    )
    r.check_raises(
        "arreglo_cero(-1) raises",
        lambda: dyn_interp.call("arreglo_cero_prueba", [-1]),
        nemi.ExecutionError,
    )

    # -- extra: afirma (v0.2 §21.2) -----------------------------------------
    print("extra: afirma (v0.2)")
    assert_interp = nemi.load(
        "función C(n, r)\n"
        "    resultado <- 1\n"
        "    para i <- 1 hasta r\n"
        "        resultado <- (resultado * (n - r + i)) / i\n"
        "    fin para\n"
        "    regresa resultado\n"
        "fin función\n"
        "función prueba_ok()\n"
        "    afirma C(8, 4) = 70\n"
        "    regresa 1\n"
        "fin función\n"
        "función prueba_falsa_sin_mensaje()\n"
        "    afirma 1 = 2\n"
        "fin función\n"
        "función prueba_falsa_con_mensaje()\n"
        "    afirma 1 = 2, \"mensaje de prueba\"\n"
        "fin función\n"
        "función prueba_listas()\n"
        "    afirma [1, 2, 3] = [1, 2, 4]\n"
        "fin función\n"
    )
    r.check("afirma true does nothing", assert_interp.call("prueba_ok", []), 1)
    r.check_raises(
        "afirma false without message raises",
        lambda: assert_interp.call("prueba_falsa_sin_mensaje", []),
        nemi.ExecutionError,
    )

    try:
        assert_interp.call("prueba_falsa_con_mensaje", [])
        r.check("afirma false message is included in the error", False, True)
    except nemi.ExecutionError as err:
        r.check(
            "afirma false message is included in the error",
            "mensaje de prueba" in str(err),
            True,
        )

    try:
        assert_interp.call("prueba_falsa_con_mensaje", [])
        r.check("afirma error quotes the failing expression", False, True)
    except nemi.ExecutionError as err:
        r.check(
            "afirma error quotes the failing expression",
            "1 = 2" in str(err),
            True,
        )

    r.check_raises(
        "afirma with structural list equality raises when unequal",
        lambda: assert_interp.call("prueba_listas", []),
        nemi.ExecutionError,
    )

    try:
        nemi.load('afirma 5 en {1, 2, 3}\n').run_program()
        r.check("afirma error normalizes 'en' back to ∈ in the message", False, True)
    except nemi.ExecutionError as err:
        r.check(
            "afirma error normalizes 'en' back to ∈ in the message",
            "5 ∈ {1, 2, 3}" in str(err),
            True,
        )

    # afirma at the top-level script body (not inside a función)
    top_ok = nemi.load(
        "función doble(x)\n    regresa x * 2\nfin función\nafirma doble(21) = 42\n"
    )
    try:
        top_ok.run_program()
        r.check("afirma at top level (true) doesn't abort", True, True)
    except nemi.ExecutionError:
        r.check("afirma at top level (true) doesn't abort", False, True)

    r.check_raises(
        "afirma at top level (false) aborts the script",
        lambda: nemi.load("afirma 1 = 2\n").run_program(),
        nemi.ExecutionError,
    )

    # error location must point at the file with the failing afirma, even
    # when reached via incluye (spec §12/§21.2)
    try:
        nemi.load('incluye "prueba_afirma.nemi"\n', _example("marcador_ficticio3.nemi")).run_program()
        r.check("afirma error via incluye names the right file", False, True)
    except nemi.ExecutionError as err:
        r.check(
            "afirma error via incluye names the right file",
            "prueba_afirma.nemi" in str(err),
            True,
        )

    # -- extra: traza (v0.2 §21.3 [OPC]) -------------------------------------
    print("extra: traza (v0.2)")

    def _traced_output(fn):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            fn()
        return buf.getvalue()

    r.check(
        "traza prints nested entry/return, indented by call depth",
        _traced_output(lambda: nemi.load(
            "función factorial(n)\n"
            "    si n = 0\n"
            "        regresa 1\n"
            "    fin si\n"
            "    regresa n * factorial(n - 1)\n"
            "fin función\n"
            "traza factorial(3)\n"
        ).run_program()),
        "→ factorial(3)\n"
        "  → factorial(2)\n"
        "    → factorial(1)\n"
        "      → factorial(0)\n"
        "      ← 1\n"
        "    ← 1\n"
        "  ← 2\n"
        "← 6\n",
    )

    r.check(
        "traza prints each assignment, indented one level past entry/return",
        _traced_output(lambda: nemi.load(
            "función mcd(a, b)\n"
            "    mientras b != 0\n"
            "        t <- b\n"
            "        b <- a mod b\n"
            "        a <- t\n"
            "    fin mientras\n"
            "    regresa a\n"
            "fin función\n"
            "traza mcd(12, 18)\n"
        ).run_program()),
        "→ mcd(12, 18)\n"
        "    t ← 18\n"
        "    b ← 12\n"
        "    a ← 18\n"
        "    t ← 12\n"
        "    b ← 6\n"
        "    a ← 12\n"
        "    t ← 6\n"
        "    b ← 0\n"
        "    a ← 6\n"
        "← 6\n",
    )

    r.check(
        "traza of a non-call expression prints nothing (no call/assign happens)",
        _traced_output(lambda: nemi.load("traza 2 + 2\n").run_program()),
        "",
    )

    r.check(
        "nested traza (already active) doesn't disturb the outer trace: both "
        "the inner `traza f(n)` and the untraced `f(n)` after it get traced, "
        "since the outer traza's dynamic extent is still active for both",
        _traced_output(lambda: nemi.load(
            "función f(n)\n"
            "    regresa n + 1\n"
            "fin función\n"
            "función g(n)\n"
            "    traza f(n)\n"
            "    regresa f(n) * 2\n"
            "fin función\n"
            "traza g(5)\n"
        ).run_program()),
        "→ g(5)\n"
        "  → f(5)\n"
        "  ← 6\n"
        "  → f(5)\n"
        "  ← 6\n"
        "← 12\n",
    )

    def _traced_error():
        try:
            nemi.load(
                "función falla(n)\n"
                "    afirma n > 10\n"
                "    regresa n\n"
                "fin función\n"
                "traza falla(3)\n"
            ).run_program()
        except nemi.ExecutionError:
            pass

    r.check(
        "traza prints entry but no return line when the call aborts",
        _traced_output(_traced_error),
        "→ falla(3)\n",
    )

    # -- extra: primitivas de cadena (v0.2 §22.6) ----------------------------
    print("extra: primitivas de cadena (v0.2)")
    strings_interp = nemi.load("")  # no definitions needed; primitives only

    r.check(
        "concatena/texto/valor",
        to_python(nemi.run_call(strings_interp, 'concatena("ab", "cd")')),
        "abcd",
    )
    r.check("texto(5)", to_python(nemi.run_call(strings_interp, "texto(5)")), "5")
    r.check(
        "texto of a Fraction renders as p/q (same as format_value)",
        to_python(nemi.run_call(strings_interp, "texto(3 / 2)")),
        "3/2",
    )
    r.check(
        "texto of a whole-number Fraction drops the /1",
        to_python(nemi.run_call(strings_interp, "texto(4 / 2)")),
        "2",
    )
    r.check("valor('7')", to_python(nemi.run_call(strings_interp, 'valor("7")')), 7)

    r.check_raises(
        "concatena raises on a non-string argument",
        lambda: nemi.run_call(strings_interp, 'concatena("a", 1)'),
        nemi.ExecutionError,
    )
    r.check_raises(
        "texto raises on a non-number argument",
        lambda: nemi.run_call(strings_interp, 'texto("5")'),
        nemi.ExecutionError,
    )
    r.check_raises(
        "valor raises on a non-digit character",
        lambda: nemi.run_call(strings_interp, 'valor("x")'),
        nemi.ExecutionError,
    )
    r.check_raises(
        "valor raises on a multi-character string",
        lambda: nemi.run_call(strings_interp, 'valor("12")'),
        nemi.ExecutionError,
    )

    # the §22.6 module code itself, verbatim, as its own acceptance suite:
    # if any of its `afirma` lines is false, run_program() raises.
    try:
        nemi.load(
            "función invierte(s)\n"
            "    r <- \"\"\n"
            "    para i <- 1 hasta long(s)\n"
            "        r <- concatena(r, s[long(s) - i + 1])\n"
            "    fin para\n"
            "    regresa r\n"
            "fin función\n"
            "función prefijo(s, k)\n"
            "    r <- \"\"\n"
            "    para i <- 1 hasta k\n"
            "        r <- concatena(r, s[i])\n"
            "    fin para\n"
            "    regresa r\n"
            "fin función\n"
            "función sufijo(s, k)\n"
            "    r <- \"\"\n"
            "    para i <- long(s) - k + 1 hasta long(s)\n"
            "        r <- concatena(r, s[i])\n"
            "    fin para\n"
            "    regresa r\n"
            "fin función\n"
            "función a_binario(n)\n"
            "    si n = 0\n"
            "        regresa \"0\"\n"
            "    fin si\n"
            "    r <- \"\"\n"
            "    mientras n > 0\n"
            "        r <- concatena(texto(n mod 2), r)\n"
            "        n <- ⌊ n / 2 ⌋\n"
            "    fin mientras\n"
            "    regresa r\n"
            "fin función\n"
            "función desde_base(s, b)\n"
            "    v <- 0\n"
            "    para i <- 1 hasta long(s)\n"
            "        v <- v * b + valor(s[i])\n"
            "    fin para\n"
            "    regresa v\n"
            "fin función\n"
            "afirma invierte(\"abc\") = \"cba\"\n"
            "afirma prefijo(\"discreta\", 4) = \"disc\"\n"
            "afirma sufijo(\"discreta\", 3) = \"eta\"\n"
            "afirma a_binario(13) = \"1101\"\n"
            "afirma desde_base(\"1101\", 2) = 13\n"
        ).run_program()
        r.check("cadenas.nemi (§22.6) module runs with all afirma true", True, True)
    except nemi.ExecutionError as err:
        r.check(f"cadenas.nemi (§22.6) module runs with all afirma true: {err}", False, True)

    # -- extra: bibcom/*.nemi (v0.2 §19.3, §22) -------------------------------
    # The actual library files (not inline copies): loading each one and
    # running it is the acceptance suite the spec describes -- if any of its
    # `afirma` lines were wrong, run_program() would raise.
    print("extra: bibcom/*.nemi (v0.2)")
    for module in [
        "teoria_numeros", "conteo", "conjuntos", "relaciones", "booleana", "cadenas",
    ]:
        try:
            nemi.load_files([_bibcom(f"{module}.nemi")]).run_program()
            r.check(f"bibcom/{module}.nemi runs standalone with all afirma true", True, True)
        except nemi.ExecutionError as err:
            r.check(f"bibcom/{module}.nemi runs standalone with all afirma true: {err}", False, True)

    try:
        nemi.load_files([_bibcom("bibcom.nemi")]).run_program()
        r.check("bibcom.nemi aggregator (incluye of all six) runs with no afirma failing", True, True)
    except nemi.ExecutionError as err:
        r.check(
            f"bibcom.nemi aggregator (incluye of all six) runs with no afirma failing: {err}",
            False, True,
        )

    print()
    total = r.passed + r.failed
    print(f"{r.passed}/{total} checks passed.")
    return 0 if r.failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
