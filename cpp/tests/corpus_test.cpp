// Acceptance suite (spec §17) — the C++ counterpart of tests/run_corpus.py.
//
// It loads the SAME examples/ corpus shared with the Python implementation and
// asserts the same verified results. Until the lexer/parser/evaluator are
// ported, the stubs throw std::logic_error; such cases are reported as PENDING
// (not FAIL), so this file doubles as a live TODO list. A C++ port is complete
// when every case reads PASS.
//
// NEMI_EXAMPLES_DIR and NEMI_BIBCOM_DIR are injected by CMake (point at
// ../examples and ../bibcom respectively).
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "nemi/array.hpp"
#include "nemi/ast.hpp"
#include "nemi/conjunto.hpp"
#include "nemi/errors.hpp"
#include "nemi/nemi.hpp"

#ifndef NEMI_EXAMPLES_DIR
#  define NEMI_EXAMPLES_DIR "."
#endif
#ifndef NEMI_BIBCOM_DIR
#  define NEMI_BIBCOM_DIR "."
#endif

namespace {

std::string example(const std::string& name) {
    return std::string(NEMI_EXAMPLES_DIR) + "/" + name;
}

std::string bibcom(const std::string& name) {
    return std::string(NEMI_BIBCOM_DIR) + "/" + name;
}

// Runs `fn` with std::cout redirected into a buffer, returns what it wrote.
// Used by the traza (v0.2 §21.3) checks, which assert on printed trace lines.
std::string capture_stdout(const std::function<void()>& fn) {
    std::ostringstream buf;
    std::streambuf* saved = std::cout.rdbuf(buf.rdbuf());
    try {
        fn();
    } catch (...) {
        std::cout.rdbuf(saved);
        throw;
    }
    std::cout.rdbuf(saved);
    return buf.str();
}

// All corpus files, loaded into one interpreter (definitions share a program).
std::vector<std::string> all_examples() {
    static const char* names[] = {
        "maximo.nemi", "busca_texto.nemi", "insercion_por_orden.nemi",
        "es_primo.nemi", "mcd.nemi", "factorial.nemi", "fib.nemi",
        "factorial_iter.nemi", "fib_iter.nemi", "exp_rapida.nemi",
        "exp_mod.nemi", "warshall.nemi", "enlosa.nemi",
    };
    std::vector<std::string> paths;
    for (const char* n : names) paths.push_back(example(n));
    return paths;
}

struct Harness {
    int passed = 0, failed = 0, pending = 0;

    // Run `produce`, format its result, compare to `expected`.
    void check(const std::string& label,
               const std::function<nemi::Value()>& produce,
               const std::string& expected) {
        try {
            std::string got = nemi::format_value(produce());
            if (got == expected) {
                std::cout << "  [PASS] " << label << ": " << got << "\n";
                ++passed;
            } else {
                std::cout << "  [FAIL] " << label << ": got " << got
                          << ", expected " << expected << "\n";
                ++failed;
            }
        } catch (const std::logic_error& todo) {
            std::cout << "  [PENDING] " << label << ": " << todo.what() << "\n";
            ++pending;
        } catch (const std::exception& err) {
            std::cout << "  [FAIL] " << label << ": threw " << err.what() << "\n";
            ++failed;
        }
    }
};

}  // namespace

int main() {
    Harness h;

    bool frontend_ready = true;
    nemi::Interpreter interp = [&] {
        try {
            return nemi::load_files(all_examples());
        } catch (const std::logic_error&) {
            // Lexer/parser not ported yet: build an empty interpreter directly
            // (bypassing the front end) so the per-case checks still run and
            // report PENDING uniformly.
            frontend_ready = false;
            return nemi::Interpreter(nemi::Program{});
        }
    }();

    auto call = [&](const std::string& text) {
        return [&interp, text] { return nemi::run_call(interp, text); };
    };

    // Direct-API checks (procedures) can't rely on the front end throwing a
    // logic_error, so gate them until the front end is ported.
    auto api_check = [&](const std::string& label,
                         const std::function<nemi::Value()>& fn,
                         const std::string& expected) {
        if (!frontend_ready) {
            std::cout << "  [PENDING] " << label << ": front end not ported yet\n";
            ++h.pending;
            return;
        }
        h.check(label, fn, expected);
    };

    // -- value-returning corpus (via run_call, spec §17) ------------------
    h.check("máximo([3,9,4],3)", call("máximo([3, 9, 4], 3)"), "9");
    h.check("busca_texto", call("busca_texto(\"001\", 3, \"010001\", 6)"), "4");
    h.check("es_primo(97)", call("es_primo(97)"), "0");
    h.check("es_primo(51)", call("es_primo(51)"), "3");
    h.check("mcd(504,396)", call("mcd(504, 396)"), "36");
    h.check("mcd(105,30)", call("mcd(105, 30)"), "15");
    h.check("factorial(5)", call("factorial(5)"), "120");
    h.check("fib(5)", call("fib(5)"), "5");
    h.check("factorial_iter(6)", call("factorial_iter(6)"), "720");
    h.check("fib_iter(10)", call("fib_iter(10)"), "55");
    h.check("exp_rápida(2,10)", call("exp_rápida(2, 10)"), "1024");
    h.check("exp_mod(572,29,713)", call("exp_mod(572, 29, 713)"), "113");
    h.check("exp_mod(3,13,7)", call("exp_mod(3, 13, 7)"), "3");

    // -- Fase 4 stress case: factorial(100) needs true bignum (158 digits,
    // far past 64 bits). Literal copied verbatim from
    // ../../python/tests/run_corpus.py so both suites check the same value.
    h.check("factorial(100) is bignum-correct", call("factorial(100)"),
            "9332621544394415268169923885626670049071596826438162146859296389521759"
            "9993229915608941463976156518286253697920827223758251185210916864000000000000000000000000");

    // -- inserción_por_orden: in-place procedure (uses the C++ call API) --
    api_check("inserción_por_orden in place", [&] {
        auto arr = std::make_shared<nemi::Array>(
            std::vector<nemi::Value>{nemi::Integer{34}, nemi::Integer{20},
                                     nemi::Integer{19}, nemi::Integer{5}});
        interp.call("inserción_por_orden", {arr, nemi::Integer{4}});
        return nemi::Value{arr};  // inspect the mutated array
    }, "[5, 19, 20, 34]");

    // -- Warshall: boolean matrix (transitive closure) --------------------
    api_check("Warshall closure", [&] {
        auto row = [](std::initializer_list<int> bits) {
            std::vector<nemi::Value> r;
            for (int b : bits) r.push_back(nemi::Integer{b});
            return std::make_shared<nemi::Array>(std::move(r));
        };
        auto matrix = std::make_shared<nemi::Array>(std::vector<nemi::Value>{
            row({0, 1, 0, 0}), row({0, 0, 1, 0}),
            row({0, 0, 0, 1}), row({0, 0, 0, 0})});
        return interp.call("Warshall", {matrix, nemi::Integer{4}});
    }, "[[0, 1, 1, 1], [0, 0, 1, 1], [0, 0, 0, 1], [0, 0, 0, 0]]");

    // -- incluye: file inclusion (lets Nemi have a "standard library") ----
    auto expect_error = [&](const std::string& label,
                            const std::function<void()>& fn,
                            const std::string& needle) {
        try {
            fn();
            std::cout << "  [FAIL] " << label << ": no lanz\xC3\xB3 ninguna excepci\xC3\xB3n\n";
            ++h.failed;
        } catch (const nemi::NemiError& err) {
            std::string what = err.what();
            if (what.find(needle) != std::string::npos) {
                std::cout << "  [PASS] " << label << ": " << what << "\n";
                ++h.passed;
            } else {
                std::cout << "  [FAIL] " << label << ": " << what
                          << " (no contiene '" << needle << "')\n";
                ++h.failed;
            }
        }
    };

    if (frontend_ready) {
        nemi::Interpreter inc = nemi::load_files({example("usa_biblioteca.nemi")});
        api_check("doble(21) via incluye", [&] {
            return inc.call("doble", {nemi::Integer{21}});
        }, "42");
        api_check("es_par(4) via incluye", [&] {
            return inc.call("es_par", {nemi::Integer{4}});
        }, "verdadero");

        expect_error("incluye missing file", [&] {
            nemi::load("incluye \"no_existe_de_verdad.nemi\"\n",
                      example("marcador_ficticio.nemi"));
        }, "no_existe_de_verdad.nemi");

        expect_error("incluye cyclic file", [&] {
            nemi::load_files({example("incluye_ciclo_a.nemi")});
        }, "incluye_ciclo_b.nemi");

        expect_error("incluye error names the broken file", [&] {
            nemi::load("incluye \"biblioteca_rota.nemi\"\n",
                      example("marcador_ficticio2.nemi"));
        }, "biblioteca_rota.nemi");

        // -I / search_paths: fallback search path when the file isn't next
        // to the one including it (see README.md, "-I / --incluye-dir")
        std::vector<std::string> incluye_dir_externo{example("incluye_dir_externo")};
        nemi::Interpreter via_dir = nemi::load_files(
            {example("usa_incluye_dir.nemi")}, incluye_dir_externo);
        api_check("triple(7) via -I", [&] {
            return via_dir.call("triple", {nemi::Integer{7}});
        }, "21");

        expect_error("incluye not found without matching -I", [&] {
            nemi::load_files({example("usa_incluye_dir.nemi")});
        }, "triple.nemi");
    }

    // -- Conjunto (v0.2 §20.1-§20.2) ---------------------------------------
    {
        nemi::Interpreter cset = nemi::load(
            "función literal_con_dup() regresa {3, 1, 2, 1} fin función\n"
            "función pertenece_si() regresa 2 \xE2\x88\x88 {1, 2, 3} fin función\n"
            "función pertenece_igual_uno() regresa (2 \xE2\x88\x88 {1, 2, 3}) = 1 fin función\n"
            "función pertenece_no() regresa 5 \xE2\x88\x88 {1, 2, 3} fin función\n"
            "función pertenece_si_en() regresa 2 en {1, 2, 3} fin función\n"
            "función pertenece_no_en() regresa 5 en {1, 2, 3} fin función\n"
            "función no_pertenece() regresa 5 \xE2\x88\x89 {1, 2, 3} fin función\n"
            "función vacio_literal() regresa \xE2\x88\x85 fin función\n"
            "función es_subconjunto() regresa {1, 2} \xE2\x8A\x86 {1, 2, 3} fin función\n"
            "función no_propio_de_si() regresa {1, 2, 3} \xE2\x8A\x82 {1, 2, 3} fin función\n"
            "función igualdad_sin_orden() regresa {1, 2, 3} = {3, 2, 1} fin función\n"
            "función anidado() regresa {1, 2} \xE2\x88\x88 {{1, 2}, {3, 4}} fin función\n"
            "función u() regresa union({1,2}, {2,3}) fin función\n"
            "función i() regresa interseccion({1,2,3}, {2,3,4}) fin función\n"
            "función d() regresa diferencia({1,2,3}, {2}) fin función\n"
            "función c() regresa cardinalidad({1,2,3}) fin función\n"
            "función p() regresa pertenece(2, {1,2,3}) fin función\n"
            "función s() regresa subconjunto({1,2}, {1,2,3}) fin función\n"
            "función l() regresa long({1,2,3}) fin función\n"
            "función sinvalor()\n"
            "    regresa\n"
            "fin función\n");

        auto ccall = [&](const std::string& name) {
            return [&cset, name] { return cset.call(name, {}); };
        };
        h.check("{3,1,2,1} collapses to {1,2,3}", ccall("literal_con_dup"), "{1, 2, 3}");
        h.check("2 \xE2\x88\x88 {1,2,3}", ccall("pertenece_si"), "verdadero");
        // regression (found by bibcom/conjuntos.nemi, v0.2 §22.3): `=` must
        // treat a bit-producing bool as equal to the number 1, mirroring
        // Python's bool-is-an-int semantics (values_equal in value.cpp).
        h.check("(2 \xE2\x88\x88 {1,2,3}) = 1", ccall("pertenece_igual_uno"), "verdadero");
        h.check("5 \xE2\x88\x88 {1,2,3} is false", ccall("pertenece_no"), "falso");
        h.check("2 en {1,2,3} (ASCII word for \xE2\x88\x88)", ccall("pertenece_si_en"), "verdadero");
        h.check("5 en {1,2,3} is false", ccall("pertenece_no_en"), "falso");
        h.check("5 \xE2\x88\x89 {1,2,3}", ccall("no_pertenece"), "verdadero");
        h.check("\xE2\x88\x85 literal is an empty conjunto", ccall("vacio_literal"), "{}");
        h.check("{1,2} \xE2\x8A\x86 {1,2,3}", ccall("es_subconjunto"), "verdadero");
        h.check("{1,2,3} \xE2\x8A\x82 {1,2,3} is false (not proper)",
                ccall("no_propio_de_si"), "falso");
        h.check("conjunto equality ignores insertion order", ccall("igualdad_sin_orden"), "verdadero");
        h.check("nested conjunto membership", ccall("anidado"), "verdadero");
        h.check("union({1,2},{2,3})", ccall("u"), "{1, 2, 3}");
        h.check("interseccion({1,2,3},{2,3,4})", ccall("i"), "{2, 3}");
        h.check("diferencia({1,2,3},{2})", ccall("d"), "{1, 3}");
        h.check("cardinalidad({1,2,3})", ccall("c"), "3");
        h.check("pertenece(2, {1,2,3})", ccall("p"), "verdadero");
        h.check("subconjunto({1,2}, {1,2,3})", ccall("s"), "verdadero");
        h.check("long extended to conjunto", ccall("l"), "3");
        // ∅ keeps its two meanings straight (§20.1): a procedure/regresa
        // with no value formats as ∅, distinct from the empty-set literal
        // (checked above -- "{}" via vacio_literal), never confused.
        h.check("no-value result formats as \xE2\x88\x85", ccall("sinvalor"), "\xE2\x88\x85");
    }

    // -- para cada ... en ... (v0.2 §20.3) ---------------------------------
    {
        nemi::Interpreter feach = nemi::load(
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
            // Encodes visitation order as a single number (elem 3, then 1,
            // then 2 -> 312) since `agrega` (Fase 3) isn't implemented yet
            // to collect a real list of what was seen.
            "función orden_lista()\n"
            "    v <- 0\n"
            "    para cada elem en [3, 1, 2] repite\n"
            "        v <- v * 10 + elem\n"
            "    fin para\n"
            "    regresa v\n"
            "fin función\n"
            "función orden_conjunto()\n"
            "    v <- 0\n"
            "    para cada elem en {3, 1, 2} repite\n"
            "        v <- v * 10 + elem\n"
            "    fin para\n"
            "    regresa v\n"
            "fin función\n");

        api_check("para cada over a list sums all elements", [&] {
            auto arr = std::make_shared<nemi::Array>(std::vector<nemi::Value>{
                nemi::Integer{1}, nemi::Integer{2}, nemi::Integer{3}, nemi::Integer{4}});
            return feach.call("suma_lista", {arr});
        }, "10");
        api_check("para cada over a conjunto sums all elements", [&] {
            auto set = std::make_shared<nemi::Conjunto>(std::vector<nemi::Value>{
                nemi::Integer{1}, nemi::Integer{2}, nemi::Integer{3}, nemi::Integer{4}});
            return feach.call("suma_conjunto", {set});
        }, "10");
        h.check("para cada over a list visits in index order",
                [&] { return feach.call("orden_lista", {}); }, "312");
        h.check("para cada over a conjunto visits in canonical order",
                [&] { return feach.call("orden_conjunto", {}); }, "123");

        expect_error("para cada over a non-collection raises", [&] {
            nemi::load(
                "función f()\n"
                "    para cada elem en 5 repite\n"
                "        z <- elem\n"
                "    fin para\n"
                "fin función\n")
                .call("f", {});
        }, "para cada");
    }

    // -- listas dinámicas: agrega/copia/arreglo_cero/matriz_cero (v0.2 §20.4) --
    {
        nemi::Interpreter dyn = nemi::load(
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
            "    regresa original\n"
            "fin función\n"
            "función copia_profunda()\n"
            "    interno <- [1, 2]\n"
            "    original <- [interno]\n"
            "    copiado <- copia(original)\n"
            "    copiado[1][1] <- 99\n"
            "    regresa original\n"
            "fin función\n"
            "función arreglo_cero_prueba(n)\n"
            "    regresa arreglo_cero(n)\n"
            "fin función\n"
            "función matriz_cero_prueba(m, n)\n"
            "    regresa matriz_cero(m, n)\n"
            "fin función\n");

        api_check("agrega grows a list, mutating by reference", [&] {
            auto arr = std::make_shared<nemi::Array>(std::vector<nemi::Value>{
                nemi::Integer{1}, nemi::Integer{2}});
            return dyn.call("agrega_lista", {arr, nemi::Integer{3}});
        }, "[1, 2, 3]");
        api_check("agrega on a conjunto is idempotent", [&] {
            auto set = std::make_shared<nemi::Conjunto>(std::vector<nemi::Value>{
                nemi::Integer{1}, nemi::Integer{2}});
            return dyn.call("agrega_conjunto", {set, nemi::Integer{2}});
        }, "{1, 2}");
        h.check("copia leaves the original untouched",
                [&] { return dyn.call("copia_independiente", {}); }, "[1, 2, 3]");
        h.check("copia is deep (nested Array untouched too)",
                [&] { return dyn.call("copia_profunda", {}); }, "[[1, 2]]");
        h.check("arreglo_cero(5)",
                [&] { return dyn.call("arreglo_cero_prueba", {nemi::Integer{5}}); },
                "[0, 0, 0, 0, 0]");
        h.check("matriz_cero(2,3)",
                [&] { return dyn.call("matriz_cero_prueba", {nemi::Integer{2}, nemi::Integer{3}}); },
                "[[0, 0, 0], [0, 0, 0]]");

        expect_error("agrega on a scalar raises", [&] {
            dyn.call("agrega_lista", {nemi::Integer{5}, nemi::Integer{1}});
        }, "agrega");
        expect_error("arreglo_cero(-1) raises", [&] {
            dyn.call("arreglo_cero_prueba", {nemi::Integer{-1}});
        }, "arreglo_cero");
    }

    // -- afirma (v0.2 §21.2) ------------------------------------------------
    {
        nemi::Interpreter assert_interp = nemi::load(
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
            "fin función\n");

        h.check("afirma true does nothing",
                [&] { return assert_interp.call("prueba_ok", {}); }, "1");

        expect_error("afirma false without message raises", [&] {
            assert_interp.call("prueba_falsa_sin_mensaje", {});
        }, "afirma");

        expect_error("afirma false message is included in the error", [&] {
            assert_interp.call("prueba_falsa_con_mensaje", {});
        }, "mensaje de prueba");

        expect_error("afirma error quotes the failing expression", [&] {
            assert_interp.call("prueba_falsa_con_mensaje", {});
        }, "1 = 2");

        expect_error("afirma with structural list equality raises when unequal", [&] {
            assert_interp.call("prueba_listas", {});
        }, "afirma");

        expect_error("afirma error normalizes 'en' back to \xE2\x88\x88 in the message", [&] {
            nemi::load("afirma 5 en {1, 2, 3}\n").run_program();
        }, "5 \xE2\x88\x88 {1, 2, 3}");

        // afirma at the top-level script body (not inside a función)
        try {
            nemi::load("función doble(x)\n    regresa x * 2\nfin funci\xC3\xB3n\nafirma doble(21) = 42\n")
                .run_program();
            std::cout << "  [PASS] afirma at top level (true) doesn't abort\n";
            ++h.passed;
        } catch (const nemi::NemiError& err) {
            std::cout << "  [FAIL] afirma at top level (true) doesn't abort: threw "
                      << err.what() << "\n";
            ++h.failed;
        }

        expect_error("afirma at top level (false) aborts the script", [&] {
            nemi::load("afirma 1 = 2\n").run_program();
        }, "afirma");

        // error location must point at the file with the failing afirma, even
        // when reached via incluye (spec §12/§21.2)
        expect_error("afirma error via incluye names the right file", [&] {
            nemi::load("incluye \"prueba_afirma.nemi\"\n",
                      example("marcador_ficticio3.nemi"))
                .run_program();
        }, "prueba_afirma.nemi");
    }

    // -- traza (v0.2 §21.3 [OPC]) --------------------------------------------
    {
        auto check_text = [&](const std::string& label, const std::string& got,
                              const std::string& expected) {
            if (got == expected) {
                std::cout << "  [PASS] " << label << "\n";
                ++h.passed;
            } else {
                std::cout << "  [FAIL] " << label << ": got " << got
                          << ", expected " << expected << "\n";
                ++h.failed;
            }
        };

        check_text(
            "traza prints nested entry/return, indented by call depth",
            capture_stdout([] {
                nemi::load(
                    "funci\xC3\xB3n factorial(n)\n"
                    "    si n = 0\n"
                    "        regresa 1\n"
                    "    fin si\n"
                    "    regresa n * factorial(n - 1)\n"
                    "fin funci\xC3\xB3n\n"
                    "traza factorial(3)\n")
                    .run_program();
            }),
            "\xE2\x86\x92 factorial(3)\n"
            "  \xE2\x86\x92 factorial(2)\n"
            "    \xE2\x86\x92 factorial(1)\n"
            "      \xE2\x86\x92 factorial(0)\n"
            "      \xE2\x86\x90 1\n"
            "    \xE2\x86\x90 1\n"
            "  \xE2\x86\x90 2\n"
            "\xE2\x86\x90 6\n");

        check_text(
            "traza prints each assignment, indented one level past entry/return",
            capture_stdout([] {
                nemi::load(
                    "funci\xC3\xB3n mcd(a, b)\n"
                    "    mientras b != 0\n"
                    "        t <- b\n"
                    "        b <- a mod b\n"
                    "        a <- t\n"
                    "    fin mientras\n"
                    "    regresa a\n"
                    "fin funci\xC3\xB3n\n"
                    "traza mcd(12, 18)\n")
                    .run_program();
            }),
            "\xE2\x86\x92 mcd(12, 18)\n"
            "    t \xE2\x86\x90 18\n"
            "    b \xE2\x86\x90 12\n"
            "    a \xE2\x86\x90 18\n"
            "    t \xE2\x86\x90 12\n"
            "    b \xE2\x86\x90 6\n"
            "    a \xE2\x86\x90 12\n"
            "    t \xE2\x86\x90 6\n"
            "    b \xE2\x86\x90 0\n"
            "    a \xE2\x86\x90 6\n"
            "\xE2\x86\x90 6\n");

        check_text(
            "traza of a non-call expression prints nothing (no call/assign happens)",
            capture_stdout([] { nemi::load("traza 2 + 2\n").run_program(); }),
            "");

        check_text(
            "nested traza (already active) doesn't disturb the outer trace: both "
            "the inner `traza f(n)` and the untraced `f(n)` after it get traced, "
            "since the outer traza's dynamic extent is still active for both",
            capture_stdout([] {
                nemi::load(
                    "funci\xC3\xB3n f(n)\n"
                    "    regresa n + 1\n"
                    "fin funci\xC3\xB3n\n"
                    "funci\xC3\xB3n g(n)\n"
                    "    traza f(n)\n"
                    "    regresa f(n) * 2\n"
                    "fin funci\xC3\xB3n\n"
                    "traza g(5)\n")
                    .run_program();
            }),
            "\xE2\x86\x92 g(5)\n"
            "  \xE2\x86\x92 f(5)\n"
            "  \xE2\x86\x90 6\n"
            "  \xE2\x86\x92 f(5)\n"
            "  \xE2\x86\x90 6\n"
            "\xE2\x86\x90 12\n");

        check_text(
            "traza prints entry but no return line when the call aborts",
            capture_stdout([] {
                try {
                    nemi::load(
                        "funci\xC3\xB3n falla(n)\n"
                        "    afirma n > 10\n"
                        "    regresa n\n"
                        "fin funci\xC3\xB3n\n"
                        "traza falla(3)\n")
                        .run_program();
                } catch (const nemi::NemiError&) {
                    // expected: the assert aborts the traced call
                }
            }),
            "\xE2\x86\x92 falla(3)\n");
    }

    // -- primitivas de cadena (v0.2 §22.6) -----------------------------------
    {
        nemi::Interpreter strings_interp = nemi::load("");  // primitives only

        h.check("concatena/texto/valor",
                [&] { return nemi::run_call(strings_interp, "concatena(\"ab\", \"cd\")"); },
                "\"abcd\"");
        h.check("texto(5)", [&] { return nemi::run_call(strings_interp, "texto(5)"); },
                "\"5\"");
        h.check("texto of a Rational renders as p/q (same as format_value)",
                [&] { return nemi::run_call(strings_interp, "texto(3 / 2)"); }, "\"3/2\"");
        h.check("texto of a whole-number Rational drops the /1",
                [&] { return nemi::run_call(strings_interp, "texto(4 / 2)"); }, "\"2\"");
        h.check("valor('7')", [&] { return nemi::run_call(strings_interp, "valor(\"7\")"); },
                "7");

        expect_error("concatena raises on a non-string argument", [&] {
            nemi::run_call(strings_interp, "concatena(\"a\", 1)");
        }, "concatena");
        expect_error("texto raises on a non-number argument", [&] {
            nemi::run_call(strings_interp, "texto(\"5\")");
        }, "texto");
        expect_error("valor raises on a non-digit character", [&] {
            nemi::run_call(strings_interp, "valor(\"x\")");
        }, "valor");
        expect_error("valor raises on a multi-character string", [&] {
            nemi::run_call(strings_interp, "valor(\"12\")");
        }, "valor");

        // the §22.6 module code itself, verbatim, as its own acceptance suite:
        // if any of its `afirma` lines is false, run_program() throws.
        try {
            nemi::load(
                "funci\xC3\xB3n invierte(s)\n"
                "    r <- \"\"\n"
                "    para i <- 1 hasta long(s)\n"
                "        r <- concatena(r, s[long(s) - i + 1])\n"
                "    fin para\n"
                "    regresa r\n"
                "fin funci\xC3\xB3n\n"
                "funci\xC3\xB3n prefijo(s, k)\n"
                "    r <- \"\"\n"
                "    para i <- 1 hasta k\n"
                "        r <- concatena(r, s[i])\n"
                "    fin para\n"
                "    regresa r\n"
                "fin funci\xC3\xB3n\n"
                "funci\xC3\xB3n sufijo(s, k)\n"
                "    r <- \"\"\n"
                "    para i <- long(s) - k + 1 hasta long(s)\n"
                "        r <- concatena(r, s[i])\n"
                "    fin para\n"
                "    regresa r\n"
                "fin funci\xC3\xB3n\n"
                "funci\xC3\xB3n a_binario(n)\n"
                "    si n = 0\n"
                "        regresa \"0\"\n"
                "    fin si\n"
                "    r <- \"\"\n"
                "    mientras n > 0\n"
                "        r <- concatena(texto(n mod 2), r)\n"
                "        n <- \xE2\x8C\x8A n / 2 \xE2\x8C\x8B\n"
                "    fin mientras\n"
                "    regresa r\n"
                "fin funci\xC3\xB3n\n"
                "funci\xC3\xB3n desde_base(s, b)\n"
                "    v <- 0\n"
                "    para i <- 1 hasta long(s)\n"
                "        v <- v * b + valor(s[i])\n"
                "    fin para\n"
                "    regresa v\n"
                "fin funci\xC3\xB3n\n"
                "afirma invierte(\"abc\") = \"cba\"\n"
                "afirma prefijo(\"discreta\", 4) = \"disc\"\n"
                "afirma sufijo(\"discreta\", 3) = \"eta\"\n"
                "afirma a_binario(13) = \"1101\"\n"
                "afirma desde_base(\"1101\", 2) = 13\n")
                .run_program();
            std::cout << "  [PASS] cadenas.nemi (\xC2\xA7" "22.6) module runs with all afirma true\n";
            ++h.passed;
        } catch (const nemi::NemiError& err) {
            std::cout << "  [FAIL] cadenas.nemi (\xC2\xA7" "22.6) module runs with all afirma true: "
                      << err.what() << "\n";
            ++h.failed;
        }
    }

    // -- bibcom/*.nemi (v0.2 §19.3, §22) -------------------------------------
    // The actual library files (not inline copies): loading each one and
    // running it is the acceptance suite the spec describes -- if any of its
    // `afirma` lines were wrong, run_program() would throw.
    {
        static const char* modules[] = {
            "teoria_numeros", "conteo", "conjuntos", "relaciones", "booleana", "cadenas",
        };
        for (const char* module : modules) {
            std::string label = std::string("bibcom/") + module +
                                 ".nemi runs standalone with all afirma true";
            try {
                nemi::load_files({bibcom(std::string(module) + ".nemi")}).run_program();
                std::cout << "  [PASS] " << label << "\n";
                ++h.passed;
            } catch (const nemi::NemiError& err) {
                std::cout << "  [FAIL] " << label << ": " << err.what() << "\n";
                ++h.failed;
            }
        }

        try {
            nemi::load_files({bibcom("bibcom.nemi")}).run_program();
            std::cout << "  [PASS] bibcom.nemi aggregator (incluye of all six) "
                         "runs with no afirma failing\n";
            ++h.passed;
        } catch (const nemi::NemiError& err) {
            std::cout << "  [FAIL] bibcom.nemi aggregator (incluye of all six) "
                         "runs with no afirma failing: " << err.what() << "\n";
            ++h.failed;
        }
    }

    std::cout << "\n" << h.passed << " passed, " << h.failed << " failed, "
              << h.pending << " pending.\n";
    return h.failed == 0 ? 0 : 1;
}
