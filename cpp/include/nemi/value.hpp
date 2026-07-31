// Value: the runtime value variant (spec §8/§11). Mirrors nemi/values.py.
//
//   monostate  the value-less result (∅) of a procedure / bare `regresa`
//   Integer    bignum
//   Rational   exact rational (result of `/` on integers)
//   Real       floating point
//   bool       boolean; 0/1 integers also serve as bits
//   string     base-1 indexable, yields one-character strings
//   ArrayPtr   shared, base-1 array (matrices are arrays of arrays)
#ifndef NEMI_VALUE_HPP
#define NEMI_VALUE_HPP

#include <memory>
#include <string>
#include <variant>

#include "nemi/numeric.hpp"

namespace nemi {

class Array;
using ArrayPtr = std::shared_ptr<Array>;

using Value = std::variant<
    std::monostate,
    Integer,
    Rational,
    Real,
    bool,
    std::string,
    ArrayPtr>;

// Interpret a value as a condition (spec §12): booleans directly, numbers by
// non-zeroness. Strings/arrays are not conditions (throws ExecutionError).
bool is_truthy(const Value& value);

// Human-readable rendering used by the CLI and `imprime`.
std::string format_value(const Value& value);

}  // namespace nemi

#endif  // NEMI_VALUE_HPP
