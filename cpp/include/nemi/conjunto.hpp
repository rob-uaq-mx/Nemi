// Conjunto: an unordered set with no duplicates (spec §20.2, v0.2). Mirrors
// nemi/values.py's Conjunto. Reference type (held via SetPtr/shared_ptr in
// Value), so `agrega` on a set shared by two variables is visible to both.
//
// Elements can be any Nemi value, including Arrays and other Conjuntos, so
// membership can't use a hash-based container (Arrays are mutable and have
// no stable hash). Instead this keeps a flat vector in **canonical order**
// (spec §20.2: numbers ascending, then strings lexicographic, then compound
// values by their printed form) and does structural-equality membership
// checks (O(n) per insert) -- fine at the scale a teaching language runs at,
// and it makes equality trivial: two Conjuntos with the same elements always
// end up with identical, order-independent internal vectors.
#ifndef NEMI_CONJUNTO_HPP
#define NEMI_CONJUNTO_HPP

#include <vector>

#include "nemi/value.hpp"

namespace nemi {

class Conjunto {
public:
    Conjunto() = default;
    explicit Conjunto(std::vector<Value> items);  // de-duplicates + sorts

    // Idempotent insert (§20.2/§20.4 `agrega` on a set): a no-op if an
    // element structurally equal to `x` is already present.
    void add(const Value& x);

    bool contains(const Value& x) const;

    // Elements in canonical order (read-only view for formatting/iteration).
    const std::vector<Value>& items() const { return items_; }

    std::size_t cardinality() const { return items_.size(); }

    Conjunto set_union(const Conjunto& other) const;
    Conjunto intersection(const Conjunto& other) const;
    Conjunto difference(const Conjunto& other) const;
    bool is_subset(const Conjunto& other) const;
    bool is_proper_subset(const Conjunto& other) const;

private:
    std::vector<Value> items_;
};

}  // namespace nemi

#endif  // NEMI_CONJUNTO_HPP
