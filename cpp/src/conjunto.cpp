#include "nemi/conjunto.hpp"

#include <algorithm>

namespace nemi {
namespace {

// Sort rank for the canonical order of §20.2: numbers ascending, then
// strings lexicographic, then compound values (Array/Conjunto) sorted by
// their printed representation. Only used for deterministic iteration and
// printing of a Conjunto -- not a claim of a "true" mathematical order.
int canonical_rank(const Value& v) {
    if (std::holds_alternative<bool>(v) || std::holds_alternative<Integer>(v) ||
        std::holds_alternative<Rational>(v) || std::holds_alternative<Real>(v)) {
        return 0;
    }
    if (std::holds_alternative<std::string>(v)) return 1;
    return 2;  // ArrayPtr / SetPtr
}

double canonical_numeric(const Value& v) {
    if (auto b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
    if (auto i = std::get_if<Integer>(&v)) return i->to_double();
    if (auto r = std::get_if<Rational>(&v)) return r->to_double();
    return std::get<Real>(v);
}

bool canonical_less(const Value& a, const Value& b) {
    int ra = canonical_rank(a), rb = canonical_rank(b);
    if (ra != rb) return ra < rb;
    if (ra == 0) return canonical_numeric(a) < canonical_numeric(b);
    if (ra == 1) return std::get<std::string>(a) < std::get<std::string>(b);
    return format_value(a) < format_value(b);
}

}  // namespace

Conjunto::Conjunto(std::vector<Value> items) {
    for (auto& x : items) add(x);
}

void Conjunto::add(const Value& x) {
    if (contains(x)) return;
    items_.push_back(x);
    std::sort(items_.begin(), items_.end(), canonical_less);
}

bool Conjunto::contains(const Value& x) const {
    for (const auto& existing : items_) {
        if (values_equal(existing, x)) return true;
    }
    return false;
}

Conjunto Conjunto::set_union(const Conjunto& other) const {
    Conjunto result(items_);
    for (const auto& x : other.items_) result.add(x);
    return result;
}

Conjunto Conjunto::intersection(const Conjunto& other) const {
    Conjunto result;
    for (const auto& x : items_) {
        if (other.contains(x)) result.add(x);
    }
    return result;
}

Conjunto Conjunto::difference(const Conjunto& other) const {
    Conjunto result;
    for (const auto& x : items_) {
        if (!other.contains(x)) result.add(x);
    }
    return result;
}

bool Conjunto::is_subset(const Conjunto& other) const {
    for (const auto& x : items_) {
        if (!other.contains(x)) return false;
    }
    return true;
}

bool Conjunto::is_proper_subset(const Conjunto& other) const {
    return is_subset(other) && cardinality() < other.cardinality();
}

}  // namespace nemi
