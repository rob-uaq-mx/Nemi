// Interpreter implementation — ported from nemi/interpreter.py (Fase 3 of
// backlog.md). The call machinery, dispatch, and `regresa` control flow were
// already wired up (Fase 0); this fills in the ten `visit(...)` overloads:
// arithmetic/comparison/logic (spec §11), indexing, loops, assignment, and
// calls (user functions, primitives, `intercambia`).
//
// Most helpers below are free functions in the anonymous namespace rather
// than Interpreter methods: they operate purely on Value/Integer/Rational and
// never touch `env_` or `eval()`, so there is no need to burden them with
// interpreter state (mirrors nemi/interpreter.py's module-level `_PRIMITIVES`
// helpers, which are similarly free of real dependency on `self`).
#include "nemi/interpreter.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "nemi/array.hpp"
#include "nemi/conjunto.hpp"
#include "nemi/errors.hpp"
#include "nemi/pretty.hpp"

namespace nemi {
namespace {

// How deep Nemi's own call stack may go before reporting a friendly overflow.
constexpr int kMaxCallDepth = 8000;

// Internal control-flow carrier for `regresa` (spec §12).
struct ReturnSignal {
    Value value;
};

// ---------------------------------------------------------------------
// Numeric tower helpers (spec §8/§11): Integer/Rational/Real, promoting to
// the "highest" representation present (Integer < Rational < Real), mirroring
// Python's automatic int -> Fraction -> float promotion.
// ---------------------------------------------------------------------
bool is_number(const Value& v) {
    return std::holds_alternative<Integer>(v) || std::holds_alternative<Rational>(v) ||
           std::holds_alternative<Real>(v);
}

const Value& require_number(const Value& v) {
    if (!is_number(v)) {
        throw ExecutionError("se esperaba un número, se obtuvo " + format_value(v));
    }
    return v;
}

bool is_value_zero(const Value& v) {
    if (auto i = std::get_if<Integer>(&v)) return i->is_zero();
    if (auto r = std::get_if<Rational>(&v)) return r->num.is_zero();
    if (auto d = std::get_if<Real>(&v)) return *d == 0.0;
    return false;  // not a number; callers validate is_number() first
}

double as_real(const Value& v) {
    if (auto i = std::get_if<Integer>(&v)) return i->to_double();
    if (auto r = std::get_if<Rational>(&v)) return r->to_double();
    return std::get<Real>(v);
}

Rational as_rational(const Value& v) {
    if (auto i = std::get_if<Integer>(&v)) return Rational(*i, Integer(1));
    return std::get<Rational>(v);
}

// +, -, * for two already-validated numeric values.
Value arith_combine(TokenKind op, const Value& left, const Value& right) {
    bool real_mode = std::holds_alternative<Real>(left) || std::holds_alternative<Real>(right);
    bool rat_mode = !real_mode && (std::holds_alternative<Rational>(left) ||
                                   std::holds_alternative<Rational>(right));

    if (real_mode) {
        double a = as_real(left), b = as_real(right);
        if (op == TokenKind::Plus) return a + b;
        if (op == TokenKind::Minus) return a - b;
        return a * b;  // Times
    }
    if (rat_mode) {
        Rational a = as_rational(left), b = as_rational(right);
        if (op == TokenKind::Plus) return a + b;
        if (op == TokenKind::Minus) return a - b;
        return a * b;
    }
    Integer a = std::get<Integer>(left), b = std::get<Integer>(right);
    if (op == TokenKind::Plus) return a + b;
    if (op == TokenKind::Minus) return a - b;
    return a * b;
}

Value require_arith(const Value& left, const Value& right, const char* symbol, TokenKind op) {
    if (!is_number(left) || !is_number(right)) {
        throw ExecutionError(std::string("el operador '") + symbol + "' requiere números, se obtuvo " +
                             format_value(left) + " y " + format_value(right));
    }
    return arith_combine(op, left, right);
}

Value divide_values(const Value& left, const Value& right) {
    if (!is_number(left) || !is_number(right)) {
        throw ExecutionError("el operador '/' requiere números");
    }
    if (is_value_zero(right)) throw ExecutionError("división entre cero");
    if (std::holds_alternative<Real>(left) || std::holds_alternative<Real>(right)) {
        return as_real(left) / as_real(right);
    }
    if (std::holds_alternative<Integer>(left) && std::holds_alternative<Integer>(right)) {
        return Rational(std::get<Integer>(left), std::get<Integer>(right));  // always exact
    }
    return as_rational(left) / as_rational(right);
}

Value modulo_values(const Value& left, const Value& right) {
    // Deliberately NOT is_number(): `mod` is integers-only, and since bool is
    // its own Value alternative (not an Integer), holds_alternative<Integer>
    // already excludes booleans with no extra check needed.
    if (!std::holds_alternative<Integer>(left) || !std::holds_alternative<Integer>(right)) {
        throw ExecutionError("el operador 'mod' requiere enteros");
    }
    const Integer& a = std::get<Integer>(left);
    const Integer& b = std::get<Integer>(right);
    if (b.is_zero()) throw ExecutionError("módulo entre cero");
    return a.mod_floor(b);  // floor-mod, matching Python's `%`
}

// values_equal (structural equality, incl. Array/Conjunto) lives in
// value.cpp now -- shared with Conjunto's own idempotent insertion
// (conjunto.cpp), so both agree on what "the same element" means.

Value compare_values(TokenKind op, const Value& left, const Value& right) {
    if (op == TokenKind::Eq) return values_equal(left, right);
    if (op == TokenKind::Ne) return !values_equal(left, right);

    // Set operators (spec §20.2, v0.2): ∈/∉ take any left operand and a
    // Conjunto on the right; ⊆/⊂ need a Conjunto on both sides. Within
    // ("en") is a word-only synonym for In (∈, no ASCII glyph otherwise),
    // reused from "para cada ... en ..." loop syntax -- see is_comparison.
    if (op == TokenKind::In || op == TokenKind::Within || op == TokenKind::NotIn) {
        auto* set = std::get_if<SetPtr>(&right);
        if (!set) {
            throw ExecutionError(
                std::string("'") + (op == TokenKind::NotIn ? "\xE2\x88\x89" : "\xE2\x88\x88") +
                "' requiere un conjunto a la derecha, se obtuvo " + format_value(right));
        }
        bool member = (*set)->contains(left);
        return op != TokenKind::NotIn ? member : !member;
    }
    if (op == TokenKind::SubsetEq || op == TokenKind::Subset) {
        auto* a = std::get_if<SetPtr>(&left);
        auto* b = std::get_if<SetPtr>(&right);
        if (!a || !b) {
            throw ExecutionError(
                std::string("'") + (op == TokenKind::SubsetEq ? "\xE2\x8A\x86" : "\xE2\x8A\x82") +
                "' requiere dos conjuntos, se obtuvo " + format_value(left) + " y " + format_value(right));
        }
        return op == TokenKind::SubsetEq ? (*a)->is_subset(**b) : (*a)->is_proper_subset(**b);
    }

    if (is_number(left) && is_number(right)) {
        if (std::holds_alternative<Real>(left) || std::holds_alternative<Real>(right)) {
            double a = as_real(left), b = as_real(right);
            if (op == TokenKind::Lt) return a < b;
            if (op == TokenKind::Le) return a <= b;
            if (op == TokenKind::Gt) return a > b;
            if (op == TokenKind::Ge) return a >= b;
        }
        Rational a = as_rational(left), b = as_rational(right);
        if (op == TokenKind::Lt) return a < b;
        if (op == TokenKind::Le) return a <= b;
        if (op == TokenKind::Gt) return a > b;
        if (op == TokenKind::Ge) return a >= b;
    }
    if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
        const auto& a = std::get<std::string>(left);
        const auto& b = std::get<std::string>(right);
        if (op == TokenKind::Lt) return a < b;
        if (op == TokenKind::Le) return a <= b;
        if (op == TokenKind::Gt) return a > b;
        if (op == TokenKind::Ge) return a >= b;
    }
    throw ExecutionError("no se pueden ordenar " + format_value(left) + " y " + format_value(right));
}

bool is_comparison(TokenKind k) {
    return k == TokenKind::Eq || k == TokenKind::Ne || k == TokenKind::Lt ||
           k == TokenKind::Le || k == TokenKind::Gt || k == TokenKind::Ge ||
           k == TokenKind::In || k == TokenKind::Within || k == TokenKind::NotIn ||
           k == TokenKind::SubsetEq || k == TokenKind::Subset;
}

// Return a 0/1 bit if any operand is an Integer, else a plain bool. Keeps
// ∧/∨/¬ closed over 0/1 bits (Warshall's matrix entries) while returning
// proper booleans for comparison-based conditions.
Value logic_result(bool result, const Value& a) {
    if (std::holds_alternative<Integer>(a)) return result ? Integer(1) : Integer(0);
    return result;
}
Value logic_result(bool result, const Value& a, const Value& b) {
    if (std::holds_alternative<Integer>(a) || std::holds_alternative<Integer>(b)) {
        return result ? Integer(1) : Integer(0);
    }
    return result;
}

Value negate_value(const Value& v) {
    if (auto i = std::get_if<Integer>(&v)) return -(*i);
    if (auto r = std::get_if<Rational>(&v)) return -(*r);
    if (auto d = std::get_if<Real>(&v)) return -(*d);
    throw ExecutionError("no se puede negar " + format_value(v));
}

Value sqrt_value(const Value& v) {
    require_number(v);
    if (auto i = std::get_if<Integer>(&v)) {
        if (i->is_negative()) throw ExecutionError("√ de un número negativo");
        Integer root = Integer::isqrt(*i);
        if (root * root == *i) return root;   // perfect square: exact
        return std::sqrt(i->to_double());
    }
    if (auto r = std::get_if<Rational>(&v)) {
        if (r->num.is_negative()) throw ExecutionError("√ de un número negativo");
        return std::sqrt(r->to_double());
    }
    double d = std::get<Real>(v);
    if (d < 0) throw ExecutionError("√ de un número negativo");
    return std::sqrt(d);
}

// NOTE (known limitation, spec §16 edge case not exercised by the corpus):
// floor/ceil on a Real go through a `long long` intermediate, unlike Python's
// math.floor/ceil on a float (which produce an arbitrary-precision int). A
// double already only carries ~15-17 significant digits, so this only bites
// for magnitudes beyond ~9.2e18 — well past anything the acceptance corpus
// or realistic algorithm inputs exercise.
Value floor_value(const Value& v) {
    require_number(v);
    if (auto i = std::get_if<Integer>(&v)) return *i;
    if (auto r = std::get_if<Rational>(&v)) return r->floor();
    double d = std::get<Real>(v);
    return Integer(static_cast<long long>(std::floor(d)));
}
Value ceil_value(const Value& v) {
    require_number(v);
    if (auto i = std::get_if<Integer>(&v)) return *i;
    if (auto r = std::get_if<Rational>(&v)) return r->ceil();
    double d = std::get<Real>(v);
    return Integer(static_cast<long long>(std::ceil(d)));
}

// ---------------------------------------------------------------------
// Indexing (base-1, spec §8/§11): Array cells and string characters.
// ---------------------------------------------------------------------
Value index_get(const Value& container, const Value& key) {
    if (auto a = std::get_if<ArrayPtr>(&container)) {
        if (!std::holds_alternative<Integer>(key)) {
            throw ExecutionError("el índice de arreglo debe ser un entero, se obtuvo " + format_value(key));
        }
        return (*a)->get(std::get<Integer>(key));
    }
    if (auto s = std::get_if<std::string>(&container)) {
        if (!std::holds_alternative<Integer>(key)) {
            throw ExecutionError("el índice de cadena debe ser un entero, se obtuvo " + format_value(key));
        }
        Integer idx = std::get<Integer>(key);
        Integer size(static_cast<long long>(s->size()));
        if (idx < Integer(1) || idx > size) {
            throw ExecutionError("índice " + idx.to_decimal() + " fuera de rango 1.." +
                                 std::to_string(s->size()));
        }
        std::size_t pos = static_cast<std::size_t>((idx - Integer(1)).to_llong());
        return std::string(1, (*s)[pos]);
    }
    throw ExecutionError("el valor no es indexable: " + format_value(container));
}

void index_set(const Value& container, const Value& key, Value value) {
    if (auto a = std::get_if<ArrayPtr>(&container)) {
        if (!std::holds_alternative<Integer>(key)) {
            throw ExecutionError("el índice de arreglo debe ser un entero, se obtuvo " + format_value(key));
        }
        (*a)->set(std::get<Integer>(key), std::move(value));
        return;
    }
    if (std::holds_alternative<std::string>(container)) {
        throw ExecutionError("las cadenas son inmutables; no se puede asignar a s[i]");
    }
    throw ExecutionError("el valor no es indexable: " + format_value(container));
}

// ---------------------------------------------------------------------
// Primitives (spec §15). `intercambia` is handled in the interpreter (needs
// l-values, not evaluated arguments) — see Interpreter::swap_call.
// ---------------------------------------------------------------------
void check_arity(const char* name, const std::vector<Value>& args, std::size_t n) {
    if (args.size() != n) {
        throw ExecutionError(std::string(name) + " espera " + std::to_string(n) +
                             " argumento(s), se obtuvo " + std::to_string(args.size()));
    }
}

Value prim_floor(const std::vector<Value>& args) {
    check_arity("piso", args, 1);
    return floor_value(args[0]);
}
Value prim_ceil(const std::vector<Value>& args) {
    check_arity("techo", args, 1);
    return ceil_value(args[0]);
}
Value prim_sqrt(const std::vector<Value>& args) {
    check_arity("raíz", args, 1);
    return sqrt_value(args[0]);
}
Value prim_abs(const std::vector<Value>& args) {
    check_arity("abs", args, 1);
    const Value& v = require_number(args[0]);
    if (auto i = std::get_if<Integer>(&v)) return i->is_negative() ? -(*i) : *i;
    if (auto r = std::get_if<Rational>(&v)) return r->num.is_negative() ? -(*r) : *r;
    double d = std::get<Real>(v);
    return d < 0 ? -d : d;
}
Value prim_mod(const std::vector<Value>& args) {
    check_arity("mod", args, 2);
    return modulo_values(args[0], args[1]);
}
Value prim_length(const std::vector<Value>& args) {
    check_arity("long", args, 1);
    if (auto a = std::get_if<ArrayPtr>(&args[0])) {
        return Integer(static_cast<long long>((*a)->length()));
    }
    if (auto s = std::get_if<std::string>(&args[0])) {
        return Integer(static_cast<long long>(s->size()));
    }
    if (auto c = std::get_if<SetPtr>(&args[0])) {
        return Integer(static_cast<long long>((*c)->cardinality()));
    }
    throw ExecutionError("long espera un arreglo, cadena o conjunto");
}

// -- String primitives (spec §22.6, v0.2) ----------------------------------
// long(s) and indexing s[i] (base 1, a length-1 string) already exist
// above/in index_get; these three round out what cadenas.nemi needs.
Value prim_concatena(const std::vector<Value>& args) {
    check_arity("concatena", args, 2);
    auto s = std::get_if<std::string>(&args[0]);
    auto t = std::get_if<std::string>(&args[1]);
    if (!s || !t) {
        throw ExecutionError("concatena espera dos cadenas, se obtuvo " +
                             format_value(args[0]) + ", " + format_value(args[1]));
    }
    return *s + *t;
}

Value prim_texto(const std::vector<Value>& args) {
    check_arity("texto", args, 1);
    if (!is_number(args[0])) {
        throw ExecutionError("texto espera un número, se obtuvo " + format_value(args[0]));
    }
    return format_value(args[0]);  // same rendering imprime/afirma already use
}

Value prim_valor(const std::vector<Value>& args) {
    check_arity("valor", args, 1);
    auto s = std::get_if<std::string>(&args[0]);
    if (!s || s->size() != 1 || (*s)[0] < '0' || (*s)[0] > '9') {
        throw ExecutionError("valor espera un carácter de un solo dígito, se obtuvo " +
                             format_value(args[0]));
    }
    return Integer(static_cast<long long>((*s)[0] - '0'));
}

// -- Conjunto primitives (spec §20.2, v0.2) --------------------------------
const SetPtr& require_set(const char* name, const Value& v) {
    auto s = std::get_if<SetPtr>(&v);
    if (!s) throw ExecutionError(std::string(name) + " espera un conjunto, se obtuvo " + format_value(v));
    return *s;
}

Value prim_pertenece(const std::vector<Value>& args) {
    check_arity("pertenece", args, 2);
    return require_set("pertenece", args[1])->contains(args[0]);
}
Value prim_subconjunto(const std::vector<Value>& args) {
    check_arity("subconjunto", args, 2);
    return require_set("subconjunto", args[0])->is_subset(*require_set("subconjunto", args[1]));
}
Value prim_union(const std::vector<Value>& args) {
    check_arity("union", args, 2);
    return std::make_shared<Conjunto>(
        require_set("union", args[0])->set_union(*require_set("union", args[1])));
}
Value prim_interseccion(const std::vector<Value>& args) {
    check_arity("interseccion", args, 2);
    return std::make_shared<Conjunto>(
        require_set("interseccion", args[0])->intersection(*require_set("interseccion", args[1])));
}
Value prim_diferencia(const std::vector<Value>& args) {
    check_arity("diferencia", args, 2);
    return std::make_shared<Conjunto>(
        require_set("diferencia", args[0])->difference(*require_set("diferencia", args[1])));
}
Value prim_cardinalidad(const std::vector<Value>& args) {
    check_arity("cardinalidad", args, 1);
    return Integer(static_cast<long long>(require_set("cardinalidad", args[0])->cardinality()));
}

// -- Dynamic lists (spec §20.4, v0.2) --------------------------------------
Value prim_agrega(const std::vector<Value>& args) {
    check_arity("agrega", args, 2);
    const Value& a = args[0];
    if (auto arr = std::get_if<ArrayPtr>(&a)) {
        (*arr)->append(args[1]);  // grows the list, mutating by reference
        return Value();
    }
    if (auto set = std::get_if<SetPtr>(&a)) {
        (*set)->add(args[1]);  // idempotent insert (§20.2)
        return Value();
    }
    throw ExecutionError("agrega espera un arreglo o conjunto, se obtuvo " + format_value(a));
}

Value deep_copy(const Value& v) {
    if (auto a = std::get_if<ArrayPtr>(&v)) {
        std::vector<Value> items;
        items.reserve((*a)->items().size());
        for (const auto& item : (*a)->items()) items.push_back(deep_copy(item));
        return std::make_shared<Array>(std::move(items));
    }
    if (auto s = std::get_if<SetPtr>(&v)) {
        std::vector<Value> items;
        items.reserve((*s)->items().size());
        for (const auto& item : (*s)->items()) items.push_back(deep_copy(item));
        return std::make_shared<Conjunto>(std::move(items));
    }
    return v;  // scalars are already value types -- copying Value copies them
}
Value prim_copia(const std::vector<Value>& args) {
    check_arity("copia", args, 1);
    return deep_copy(args[0]);
}

long long require_nonneg_int(const char* name, const Value& v) {
    auto i = std::get_if<Integer>(&v);
    if (!i || i->is_negative()) {
        throw ExecutionError(std::string(name) + " espera un entero no negativo, se obtuvo " +
                             format_value(v));
    }
    return i->to_llong();
}

Value prim_arreglo_cero(const std::vector<Value>& args) {
    check_arity("arreglo_cero", args, 1);
    long long n = require_nonneg_int("arreglo_cero", args[0]);
    return std::make_shared<Array>(std::vector<Value>(static_cast<std::size_t>(n), Value(Integer(0))));
}

Value prim_matriz_cero(const std::vector<Value>& args) {
    check_arity("matriz_cero", args, 2);
    long long m = require_nonneg_int("matriz_cero", args[0]);
    long long n = require_nonneg_int("matriz_cero", args[1]);
    std::vector<Value> rows;
    rows.reserve(static_cast<std::size_t>(m));
    for (long long i = 0; i < m; ++i) {
        rows.push_back(std::make_shared<Array>(
            std::vector<Value>(static_cast<std::size_t>(n), Value(Integer(0)))));
    }
    return std::make_shared<Array>(std::move(rows));
}

Value prim_print(const std::vector<Value>& args) {
    std::string line;
    for (std::size_t k = 0; k < args.size(); ++k) {
        if (k) line += ' ';
        if (auto s = std::get_if<std::string>(&args[k])) line += *s;  // raw, no quotes
        else line += format_value(args[k]);
    }
    std::cout << line << "\n";
    return Value();
}

using PrimitiveFn = Value (*)(const std::vector<Value>&);

const std::unordered_map<std::string, PrimitiveFn>& primitives() {
    static const std::unordered_map<std::string, PrimitiveFn> table = {
        {"piso", prim_floor},
        {"techo", prim_ceil},
        {"raíz", prim_sqrt},
        {"raiz", prim_sqrt},
        {"abs", prim_abs},
        {"mod", prim_mod},
        {"long", prim_length},
        {"concatena", prim_concatena},
        {"texto", prim_texto},
        {"valor", prim_valor},
        {"imprime", prim_print},
        {"pertenece", prim_pertenece},
        {"subconjunto", prim_subconjunto},
        {"union", prim_union},
        {"interseccion", prim_interseccion},
        {"diferencia", prim_diferencia},
        {"cardinalidad", prim_cardinalidad},
        {"agrega", prim_agrega},
        {"copia", prim_copia},
        {"arreglo_cero", prim_arreglo_cero},
        {"matriz_cero", prim_matriz_cero},
    };
    return table;
}

}  // namespace

// ==========================================================================
// Interpreter: construction, calls, dispatch (Fase 0 — unchanged)
// ==========================================================================
Interpreter::Interpreter(Program program) : program_(std::move(program)) {
    for (auto& def : program_.definitions) {
        auto [it, inserted] = functions_.emplace(def->name, def.get());
        (void)it;
        if (!inserted) {
            throw ExecutionError("definición duplicada: " + def->name);
        }
    }
}

bool Interpreter::has_function(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

void Interpreter::run_program() {
    Environment env;
    Environment* saved = env_;
    env_ = &env;
    try {
        exec_block(program_.main);
    } catch (ReturnSignal&) {
        // a stray top-level `regresa` simply ends the script
    } catch (...) {
        env_ = saved;
        throw;
    }
    env_ = saved;
}

Value Interpreter::call(const std::string& name, std::vector<Value> args) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw ExecutionError("función o procedimiento no definido: " + name);
    }
    return invoke(*it->second, args);
}

Value Interpreter::evaluate(Expr& expr) {
    Environment frame;
    Environment* saved = env_;
    env_ = &frame;
    Value result;
    try {
        result = eval(expr);
    } catch (...) {
        env_ = saved;
        throw;
    }
    env_ = saved;
    return result;
}

Value Interpreter::invoke(Definition& def, std::vector<Value>& args) {
    if (args.size() != def.params.size()) {
        throw ExecutionError(def.name + " espera " +
                             std::to_string(def.params.size()) +
                             " argumento(s), se obtuvo " + std::to_string(args.size()),
                             def.location);
    }

    // traza (v0.2 §21.3 [OPC]): entry/return are printed at the caller's
    // depth, so a pair brackets the callee's body (which itself prints one
    // level deeper -- see visit(Assign&)). Formatted before args are moved
    // into the frame below.
    if (trace_depth_) {
        std::string args_text;
        for (std::size_t k = 0; k < args.size(); ++k) {
            if (k) args_text += ", ";
            args_text += format_value(args[k]);
        }
        std::cout << std::string(2 * depth_, ' ') << "\xE2\x86\x92 "
                  << def.name << "(" << args_text << ")\n";
    }

    if (++depth_ > kMaxCallDepth) {
        --depth_;
        throw ExecutionError("desbordamiento de la pila de llamadas en " + def.name +
                             " (¿recursión sin caso base?)");
    }

    Environment frame;
    for (std::size_t k = 0; k < def.params.size(); ++k) {
        frame.define(def.params[k], std::move(args[k]));
    }

    Environment* saved = env_;
    env_ = &frame;
    Value result;  // monostate: fell off the end (a value-less return)
    try {
        exec_block(def.body);
    } catch (ReturnSignal& sig) {
        result = std::move(sig.value);
    } catch (...) {
        env_ = saved;
        --depth_;
        throw;
    }
    env_ = saved;
    --depth_;

    if (trace_depth_) {
        std::cout << std::string(2 * depth_, ' ') << "\xE2\x86\x90 "
                  << format_value(result) << "\n";
    }
    return result;
}

Value Interpreter::eval(Expr& expr) {
    expr.accept(*this);
    return std::move(result_);
}

void Interpreter::exec(Stmt& stmt) { stmt.accept(*this); }

void Interpreter::exec_block(const Block& block) {
    for (const auto& stmt : block) exec(*stmt);
}

// ==========================================================================
// Expression visitors
// ==========================================================================
void Interpreter::visit(IntLiteral& node) { result_ = node.value; }
void Interpreter::visit(RealLiteral& node) { result_ = node.value; }
void Interpreter::visit(StringLiteral& node) { result_ = node.value; }
void Interpreter::visit(Variable& node) { result_ = env_->get(node.name); }

void Interpreter::visit(ArrayLiteral& node) {
    std::vector<Value> items;
    items.reserve(node.elements.size());
    for (auto& e : node.elements) items.push_back(eval(*e));
    result_ = std::make_shared<Array>(std::move(items));
}

void Interpreter::visit(SetLiteral& node) {
    // Conjunto's constructor is idempotent (spec §20.1: {3,1,2,1} -> {1,2,3}).
    std::vector<Value> items;
    items.reserve(node.elements.size());
    for (auto& e : node.elements) items.push_back(eval(*e));
    result_ = std::make_shared<Conjunto>(std::move(items));
}

void Interpreter::visit(Index& node) {
    Value container = eval(*node.base);
    Value key = eval(*node.index);
    result_ = index_get(container, key);
}

void Interpreter::visit(Unary& node) {
    TokenKind op = node.op;
    if (op == TokenKind::Minus) {
        result_ = negate_value(eval(*node.operand));
        return;
    }
    if (op == TokenKind::Not) {
        Value v = eval(*node.operand);
        result_ = logic_result(!is_truthy(v), v);
        return;
    }
    if (op == TokenKind::Sqrt) {
        result_ = sqrt_value(eval(*node.operand));
        return;
    }
    if (op == TokenKind::LFloor) {
        // Exact ⌊√n⌋ for a non-negative integer n (keeps primality/bignum
        // sound). Mirrors nemi/interpreter.py's `_floor`: this evaluates the
        // √'s inner operand once for the fast-path check; if it doesn't
        // qualify, it falls through to a *second*, independent evaluation of
        // the whole operand below (matching the Python reference exactly,
        // including its quirk of a possible double evaluation in that rare
        // fallback case).
        if (auto* inner = dynamic_cast<Unary*>(node.operand.get())) {
            if (inner->op == TokenKind::Sqrt) {
                Value x = eval(*inner->operand);
                if (auto* i = std::get_if<Integer>(&x)) {
                    if (!i->is_negative()) {
                        result_ = Integer::isqrt(*i);
                        return;
                    }
                }
            }
        }
        result_ = floor_value(eval(*node.operand));
        return;
    }
    if (op == TokenKind::LCeil) {
        result_ = ceil_value(eval(*node.operand));
        return;
    }
    throw ExecutionError(std::string("operador unario desconocido ") + to_string(op), node.location);
}

void Interpreter::visit(Binary& node) {
    TokenKind op = node.op;

    // Logical connectives short-circuit (spec §12).
    if (op == TokenKind::And) {
        Value left = eval(*node.left);
        if (!is_truthy(left)) { result_ = logic_result(false, left); return; }
        Value right = eval(*node.right);
        result_ = logic_result(is_truthy(right), left, right);
        return;
    }
    if (op == TokenKind::Or) {
        Value left = eval(*node.left);
        if (is_truthy(left)) { result_ = logic_result(true, left); return; }
        Value right = eval(*node.right);
        result_ = logic_result(is_truthy(right), left, right);
        return;
    }

    Value left = eval(*node.left);
    Value right = eval(*node.right);

    if (op == TokenKind::Plus) {
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
            result_ = std::get<bool>(left) || std::get<bool>(right);  // bit+bit -> OR
            return;
        }
        result_ = require_arith(left, right, "+", op);
        return;
    }
    if (op == TokenKind::Minus) {
        result_ = require_arith(left, right, "\xE2\x88\x92", op);  // −
        return;
    }
    if (op == TokenKind::Times) {
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
            result_ = std::get<bool>(left) && std::get<bool>(right);  // bit·bit -> AND
            return;
        }
        result_ = require_arith(left, right, "\xC2\xB7", op);  // ·
        return;
    }
    if (op == TokenKind::Divide) { result_ = divide_values(left, right); return; }
    if (op == TokenKind::Mod) { result_ = modulo_values(left, right); return; }
    if (is_comparison(op)) { result_ = compare_values(op, left, right); return; }

    throw ExecutionError(std::string("operador binario desconocido ") + to_string(op), node.location);
}

void Interpreter::visit(Call& node) {
    const std::string& name = node.name;

    // 'intercambia' is special: it takes l-values and swaps them in place.
    if (name == "intercambia") {
        result_ = swap_call(node.args);
        return;
    }

    std::vector<Value> args;
    args.reserve(node.args.size());
    for (auto& a : node.args) args.push_back(eval(*a));

    auto fit = functions_.find(name);
    if (fit != functions_.end()) {
        result_ = invoke(*fit->second, args);
        return;
    }

    const auto& prims = primitives();
    auto pit = prims.find(name);
    if (pit != prims.end()) {
        result_ = pit->second(args);
        return;
    }

    throw ExecutionError("función o procedimiento no definido: " + name, node.location);
}

// -- l-value helpers (for intercambia) ------------------------------
Value Interpreter::get_lvalue(Expr& node) {
    if (auto* v = dynamic_cast<Variable*>(&node)) {
        return env_->get(v->name);
    }
    if (auto* idx = dynamic_cast<Index*>(&node)) {
        Value container = eval(*idx->base);
        Value key = eval(*idx->index);
        return index_get(container, key);
    }
    throw ExecutionError("intercambia espera variables o celdas de arreglo");
}

void Interpreter::set_lvalue(Expr& node, Value value) {
    if (auto* v = dynamic_cast<Variable*>(&node)) {
        env_->define(v->name, std::move(value));
        return;
    }
    if (auto* idx = dynamic_cast<Index*>(&node)) {
        Value container = eval(*idx->base);
        Value key = eval(*idx->index);
        index_set(container, key, std::move(value));
        return;
    }
    throw ExecutionError("intercambia espera variables o celdas de arreglo");
}

Value Interpreter::swap_call(const std::vector<ExprPtr>& args) {
    if (args.size() != 2) throw ExecutionError("intercambia espera exactamente 2 argumentos");
    Value va = get_lvalue(*args[0]);
    Value vb = get_lvalue(*args[1]);
    set_lvalue(*args[0], std::move(vb));
    set_lvalue(*args[1], std::move(va));
    return Value();
}

// ==========================================================================
// Statement visitors
// ==========================================================================
void Interpreter::visit(Assign& node) {
    Value value = eval(*node.value);
    std::string trace_value_text = trace_depth_ ? format_value(value) : std::string();
    std::string place = node.name;
    if (node.indices.empty()) {
        env_->define(node.name, std::move(value));
    } else {
        Value container = env_->get(node.name);
        std::vector<Value> keys;
        keys.reserve(node.indices.size());
        for (auto& idx_expr : node.indices) keys.push_back(eval(*idx_expr));
        for (std::size_t k = 0; k + 1 < keys.size(); ++k) {
            container = index_get(container, keys[k]);
        }
        if (trace_depth_) {
            for (auto& key : keys) place += "[" + format_value(key) + "]";
        }
        index_set(container, keys.back(), std::move(value));
    }

    // traza (v0.2 §21.3 [OPC]): one level deeper than the enclosing call's
    // entry/return (spec's example shows "  lugar ← valor" with extra indent).
    if (trace_depth_) {
        std::cout << std::string(2 * depth_, ' ') << "  " << place
                  << " \xE2\x86\x90 " << trace_value_text << "\n";
    }
}

void Interpreter::visit(ForLoop& node) {
    Value start = eval(*node.start);
    Value end = eval(*node.end);
    if (!is_number(start) || !is_number(end)) {
        throw ExecutionError("los límites de 'para' deben ser numéricos", node.location);
    }
    Value i = start;
    while (is_truthy(compare_values(TokenKind::Le, i, end))) {  // inclusive, step +1
        env_->define(node.var, i);
        exec_block(node.body);
        i = arith_combine(TokenKind::Plus, i, Integer(1));
    }
}

void Interpreter::visit(ForEach& node) {
    // spec §20.3, v0.2: a Conjunto iterates in canonical order (already how
    // it stores its elements), an Array in index order. Mutating the
    // collection while iterating it is explicitly undefined by the spec, so
    // this just walks the live sequence with no defensive copy.
    Value collection = eval(*node.collection);
    if (auto a = std::get_if<ArrayPtr>(&collection)) {
        for (const auto& item : (*a)->items()) {
            env_->define(node.var, item);
            exec_block(node.body);
        }
        return;
    }
    if (auto c = std::get_if<SetPtr>(&collection)) {
        for (const auto& item : (*c)->items()) {
            env_->define(node.var, item);
            exec_block(node.body);
        }
        return;
    }
    throw ExecutionError(
        "'para cada' requiere un arreglo o conjunto, se obtuvo " + format_value(collection),
        node.location);
}

void Interpreter::visit(WhileLoop& node) {
    while (is_truthy(eval(*node.condition))) {
        exec_block(node.body);
    }
}

void Interpreter::visit(If& node) {
    if (is_truthy(eval(*node.condition))) {
        exec_block(node.then_body);
    } else if (node.has_else) {
        exec_block(node.else_body);
    }
}

void Interpreter::visit(Return& node) {
    Value value;  // monostate for a bare `regresa`
    if (node.value) value = eval(*node.value);
    throw ReturnSignal{std::move(value)};
}

void Interpreter::visit(ExprStatement& node) {
    eval(*node.call);  // result discarded
}

void Interpreter::visit(Prose&) { /* non-executable (spec §14) — no-op */ }

void Interpreter::visit(Assert& node) {
    // afirma expr [, "mensaje"] (spec §21.2, v0.2): reuses is_truthy (same
    // rule as si/mientras) rather than requiring a strict 0/1 bit.
    Value value = eval(*node.condition);
    if (!is_truthy(value)) {
        std::string text = "afirma falsa: " + expr_to_source(*node.condition);
        if (node.message) text += " -- " + *node.message;
        throw ExecutionError(text, node.location);
    }
}

void Interpreter::visit(Trace& node) {
    // traza expr (spec §21.3, v0.2 [OPC]): runs `expression` (typically a
    // call) with entry/return/assignment tracing active for its dynamic
    // extent; the expression's value is discarded (traza is run for its
    // printed side effects, same as an ExprStatement). A counter, not a
    // bool, so a nested traza already inside a traced call doesn't turn
    // tracing off when it finishes.
    ++trace_depth_;
    try {
        eval(*node.expression);
    } catch (...) {
        --trace_depth_;
        throw;
    }
    --trace_depth_;
}

}  // namespace nemi
