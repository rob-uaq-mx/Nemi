// Acceptance suite (spec §17) — the C++ counterpart of tests/run_corpus.py.
//
// It loads the SAME examples/ corpus shared with the Python implementation and
// asserts the same verified results. Until the lexer/parser/evaluator are
// ported, the stubs throw std::logic_error; such cases are reported as PENDING
// (not FAIL), so this file doubles as a live TODO list. A C++ port is complete
// when every case reads PASS.
//
// NEMI_EXAMPLES_DIR is injected by CMake (points at ../examples).
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "nemi/array.hpp"
#include "nemi/ast.hpp"
#include "nemi/errors.hpp"
#include "nemi/nemi.hpp"

#ifndef NEMI_EXAMPLES_DIR
#  define NEMI_EXAMPLES_DIR "."
#endif

namespace {

std::string example(const std::string& name) {
    return std::string(NEMI_EXAMPLES_DIR) + "/" + name;
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
    }

    std::cout << "\n" << h.passed << " passed, " << h.failed << " failed, "
              << h.pending << " pending.\n";
    return h.failed == 0 ? 0 : 1;
}
