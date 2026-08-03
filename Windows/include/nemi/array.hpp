// Array: a mutable sequence with 1-based indexing (spec §8/§11). Mirrors
// nemi/values.py's Array. Reference type (held via shared_ptr in Value), so
// passing one to a function shares it — this is what lets a procedure sort its
// argument in place (§12).
#ifndef NEMI_ARRAY_HPP
#define NEMI_ARRAY_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "nemi/errors.hpp"
#include "nemi/numeric.hpp"
#include "nemi/value.hpp"

namespace nemi {

class Array {
public:
    Array() = default;
    explicit Array(std::vector<Value> items) : items_(std::move(items)) {}

    std::size_t length() const { return items_.size(); }

    const Value& get(Integer index) const {
        return items_[checked(index)];
    }

    void set(Integer index, Value value) {
        items_[checked(index)] = std::move(value);
    }

    // Grow by one element at the end (§20.4 v0.2 `agrega` on a lista).
    void append(Value value) { items_.push_back(std::move(value)); }

    const std::vector<Value>& items() const { return items_; }

private:
    // Map a 1-based index to a 0-based offset, bounds-checked (spec §16).
    // The bound comparison happens entirely in BigInt space, so a huge
    // out-of-range index is rejected correctly before any narrowing cast.
    std::size_t checked(Integer index) const {
        Integer size(static_cast<long long>(items_.size()));
        if (index < Integer(1) || index > size) {
            throw ExecutionError("índice " + index.to_decimal() +
                                 " fuera de rango 1.." +
                                 std::to_string(items_.size()));
        }
        return static_cast<std::size_t>((index - Integer(1)).to_llong());
    }

    std::vector<Value> items_;
};

}  // namespace nemi

#endif  // NEMI_ARRAY_HPP
