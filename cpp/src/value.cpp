// Fully implemented (pure, no evaluation logic): truthiness and formatting.
#include "nemi/value.hpp"

#include <string>

#include "nemi/array.hpp"
#include "nemi/errors.hpp"

namespace nemi {

bool is_truthy(const Value& value) {
    if (auto b = std::get_if<bool>(&value)) return *b;
    if (auto i = std::get_if<Integer>(&value)) return *i != 0;
    if (auto r = std::get_if<Rational>(&value)) return r->num != 0;
    if (auto d = std::get_if<Real>(&value)) return *d != 0.0;
    throw ExecutionError("value is not a boolean condition: " + format_value(value));
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
    return "?";
}

}  // namespace nemi
