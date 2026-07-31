// Numeric tower typedefs (spec §8/§11), isolated behind aliases so the
// underlying big-integer / rational implementation can be swapped without
// touching the evaluator.
//
//   Integer   arbitrary-precision integer (bignum)   — REQUIRED by RSA/factorials
//   Rational  EXACT rational, produced by `/` on two integers so that
//             ⌊n/2⌋ stays exact for bignum n
//   Real      floating point
//
// Fase 4 of backlog.md: `Integer` is a from-scratch arbitrary-precision type
// (see bigint.hpp) rather than Boost.Multiprecision or GMP — no external
// dependency, so the project keeps building with nothing but a C++17
// compiler and CMake. (A GMP/Boost backend remains a valid future swap: only
// this file and bigint.hpp/.cpp would need to change.)
#ifndef NEMI_NUMERIC_HPP
#define NEMI_NUMERIC_HPP

#include "nemi/bigint.hpp"

namespace nemi {

using Integer = BigInt;
using Real = double;

// An exact rational number, always kept normalized: den > 0 and
// gcd(|num|, den) == 1. Produced by `/` on two Integers (spec §11) so that
// e.g. `⌊n / 2⌋` stays exact even for bignum n.
struct Rational {
    Integer num;
    Integer den;

    explicit Rational(Integer n = Integer(0), Integer d = Integer(1));

    bool is_integer() const;   // den == 1
    double to_double() const;  // may lose precision for very large values
    Integer floor() const;     // exact floor division
    Integer ceil() const;      // exact ceiling division

    Rational operator-() const;
    Rational operator+(const Rational& other) const;
    Rational operator-(const Rational& other) const;
    Rational operator*(const Rational& other) const;
    Rational operator/(const Rational& other) const;

    bool operator==(const Rational& other) const;
    bool operator!=(const Rational& other) const;
    bool operator<(const Rational& other) const;
    bool operator<=(const Rational& other) const;
    bool operator>(const Rational& other) const;
    bool operator>=(const Rational& other) const;

private:
    void normalize();
};

}  // namespace nemi

#endif  // NEMI_NUMERIC_HPP
