// Value: the runtime value variant (spec §8/§11). Mirrors nemi/values.py.
//
//   monostate  the value-less result (∅) of a procedure / bare `regresa`
//   Integer    bignum
//   Rational   exact rational (result of `/` on integers)
//   Real       floating point
//   bool       boolean; 0/1 integers also serve as bits
//   string     base-1 indexable, yields one-character strings
//   ArrayPtr   shared, base-1 array (matrices are arrays of arrays)
//   SetPtr     shared, unordered set with no duplicates (v0.2 §20.2)
#ifndef NEMI_VALUE_HPP
#define NEMI_VALUE_HPP

#include <memory>
#include <string>
#include <variant>

#include "nemi/numeric.hpp"

namespace nemi {

class Array;
using ArrayPtr = std::shared_ptr<Array>;

class Conjunto;
using SetPtr = std::shared_ptr<Conjunto>;

using Value = std::variant<
    std::monostate,
    Integer,
    Rational,
    Real,
    bool,
    std::string,
    ArrayPtr,
    SetPtr>;

// Interpret a value as a condition (spec §12): booleans directly, numbers by
// non-zeroness. Strings/arrays are not conditions (throws ExecutionError).
bool is_truthy(const Value& value);

// Human-readable rendering used by the CLI and `imprime`.
std::string format_value(const Value& value);

// Structural equality (spec §20.2, v0.2): numbers compare across
// Integer/Rational/Real, and Array/Conjunto compare element-wise (not by
// reference/identity) -- mismatched types are simply not equal. Shared by
// the `=`/`≠` operators (interpreter.cpp) and Conjunto's own idempotent
// insertion (conjunto.cpp), so both agree on what "the same element" means.
bool values_equal(const Value& a, const Value& b);

}  // namespace nemi

#endif  // NEMI_VALUE_HPP
