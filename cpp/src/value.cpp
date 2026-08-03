// Fully implemented (pure, no evaluation logic): truthiness and formatting.
#include "nemi/value.hpp"

#include <string>

#include "nemi/array.hpp"
#include "nemi/conjunto.hpp"
#include "nemi/errors.hpp"

namespace nemi {

bool is_truthy(const Value& value) {
    if (auto b = std::get_if<bool>(&value)) return *b;
    if (auto i = std::get_if<Integer>(&value)) return *i != 0;
    if (auto r = std::get_if<Rational>(&value)) return r->num != 0;
    if (auto d = std::get_if<Real>(&value)) return *d != 0.0;
    throw ExecutionError("el valor no es una condición booleana: " + format_value(value));
}

std::string format_value(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "\xE2\x88\x85";  // ∅
    }
    if (auto b = std::get_if<bool>(&value)) {
        return *b ? "verdadero" : "falso";
    }
    if (auto i = std::get_if<Integer>(&value)) {
        return i->to_decimal();
    }
    if (auto r = std::get_if<Rational>(&value)) {
        if (r->den == Integer(1)) return r->num.to_decimal();
        return r->num.to_decimal() + "/" + r->den.to_decimal();
    }
    if (auto d = std::get_if<Real>(&value)) {
        return std::to_string(*d);
    }
    if (auto s = std::get_if<std::string>(&value)) {
        return "\"" + *s + "\"";
    }
    if (auto a = std::get_if<ArrayPtr>(&value)) {
        std::string out = "[";
        const auto& items = (*a)->items();
        for (std::size_t k = 0; k < items.size(); ++k) {
            if (k) out += ", ";
            out += format_value(items[k]);
        }
        return out + "]";
    }
    if (auto s = std::get_if<SetPtr>(&value)) {
        // Canonical order (§20.2); the *empty* set prints "{}", never "∅" --
        // that glyph is reserved for "no value" above (§20.1/§21.1: double
        // use of ∅, resolved -- same glyph, different meaning by context).
        std::string out = "{";
        const auto& items = (*s)->items();
        for (std::size_t k = 0; k < items.size(); ++k) {
            if (k) out += ", ";
            out += format_value(items[k]);
        }
        return out + "}";
    }
    return "?";
}

// Structural equality (spec §20.2, v0.2): numbers compare across
// Integer/Rational/Real, and Array/Conjunto compare element-wise (not by
// reference/identity) -- mismatched types are simply not equal, mirroring
// Python's `==`. Shared by the `=`/`≠` operators (interpreter.cpp) and
// Conjunto's own idempotent insertion (conjunto.cpp).
bool values_equal(const Value& a, const Value& b) {
    bool a_num = std::holds_alternative<Integer>(a) || std::holds_alternative<Rational>(a) ||
                std::holds_alternative<Real>(a);
    bool b_num = std::holds_alternative<Integer>(b) || std::holds_alternative<Rational>(b) ||
                std::holds_alternative<Real>(b);

    // A bool compares equal to the number 0/1 (mirrors Python's bool-is-an-
    // int semantics: `True == 1`). The spec's own membership/subset
    // operators "producen un bit" (§20.2), so student code legitimately
    // writes `afirma (x ∈ A) = 1` (v0.2 §22.3) even though this interpreter
    // formats that bit as verdadero/falso, not 0/1.
    if (std::holds_alternative<bool>(a) && b_num) {
        return values_equal(Integer(std::get<bool>(a) ? 1 : 0), b);
    }
    if (a_num && std::holds_alternative<bool>(b)) {
        return values_equal(a, Integer(std::get<bool>(b) ? 1 : 0));
    }

    if (a_num && b_num) {
        auto as_real = [](const Value& v) -> double {
            if (auto i = std::get_if<Integer>(&v)) return i->to_double();
            if (auto r = std::get_if<Rational>(&v)) return r->to_double();
            return std::get<Real>(v);
        };
        if (std::holds_alternative<Real>(a) || std::holds_alternative<Real>(b)) {
            return as_real(a) == as_real(b);
        }
        auto as_rational = [](const Value& v) -> Rational {
            if (auto i = std::get_if<Integer>(&v)) return Rational(*i, Integer(1));
            return std::get<Rational>(v);
        };
        if (std::holds_alternative<Rational>(a) || std::holds_alternative<Rational>(b)) {
            return as_rational(a) == as_rational(b);
        }
        return std::get<Integer>(a) == std::get<Integer>(b);
    }
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) {
        return std::get<bool>(a) == std::get<bool>(b);
    }
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
        return std::get<std::string>(a) == std::get<std::string>(b);
    }
    if (std::holds_alternative<ArrayPtr>(a) && std::holds_alternative<ArrayPtr>(b)) {
        const auto& ia = std::get<ArrayPtr>(a)->items();
        const auto& ib = std::get<ArrayPtr>(b)->items();
        if (ia.size() != ib.size()) return false;
        for (std::size_t k = 0; k < ia.size(); ++k) {
            if (!values_equal(ia[k], ib[k])) return false;
        }
        return true;
    }
    if (std::holds_alternative<SetPtr>(a) && std::holds_alternative<SetPtr>(b)) {
        // Both sides are already kept in canonical order internally, so a
        // same-length elementwise compare is enough -- no need to search.
        const auto& ia = std::get<SetPtr>(a)->items();
        const auto& ib = std::get<SetPtr>(b)->items();
        if (ia.size() != ib.size()) return false;
        for (std::size_t k = 0; k < ia.size(); ++k) {
            if (!values_equal(ia[k], ib[k])) return false;
        }
        return true;
    }
    if (std::holds_alternative<std::monostate>(a) && std::holds_alternative<std::monostate>(b)) {
        return true;
    }
    return false;  // mismatched types: not equal
}

}  // namespace nemi
