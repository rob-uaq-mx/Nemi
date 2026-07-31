// BigInt: an arbitrary-precision (bignum) signed integer, implemented from
// scratch — no external dependency (no Boost/GMP). Fase 4 of backlog.md.
//
// Representation: sign + magnitude, base 1,000,000,000 (1e9), stored as a
// little-endian std::vector<uint32_t> (mag_[0] is the least-significant
// "chunk" of 9 decimal digits). Zero is always represented as an empty
// magnitude with negative_ == false (the canonical zero).
//
// Base 1e9 is chosen deliberately: it makes decimal parsing/formatting
// trivial (each limb IS 9 decimal digits, no base conversion needed), and
// limb*limb products (< 1e18) fit safely in a uint64_t accumulator during
// schoolbook multiplication — see bigint.cpp.
#ifndef NEMI_BIGINT_HPP
#define NEMI_BIGINT_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace nemi {

class BigInt {
public:
    // Implicit on purpose: lets ordinary code write `x + 1`, `n - 1`,
    // `i == 0`, etc. without an explicit BigInt(...) wrapper everywhere,
    // exactly like Python's plain `int` used pervasively in interpreter.py.
    BigInt(long long value = 0);

    // Parses a run of ASCII digits '0'..'9' (no sign, as produced by the
    // lexer's number scanner) into a BigInt.
    static BigInt from_decimal(const std::string& digits);

    BigInt operator-() const;
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;

    // Truncating division/remainder (quotient rounds toward zero; remainder
    // takes the sign of the dividend) — the usual C/C++ convention. Nemi's
    // `mod` operator wants *floor* semantics instead; use mod_floor/floor_div
    // for that (they are built on top of these).
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;

    // Floor division / floor modulo (Python's `//` and `%` semantics: the
    // remainder has the same sign as the divisor, or is zero).
    BigInt floor_div(const BigInt& other) const;
    BigInt mod_floor(const BigInt& other) const;

    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    bool operator<(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;

    bool is_zero() const;
    bool is_negative() const;

    std::string to_decimal() const;
    double to_double() const;      // may lose precision for very large values
    long long to_llong() const;    // only meaningful for values that actually fit

    static BigInt isqrt(const BigInt& n);        // floor(sqrt(n)), n >= 0
    static BigInt gcd(BigInt a, BigInt b);        // non-negative result

private:
    bool negative_ = false;          // canonical zero always has negative_ == false
    std::vector<std::uint32_t> mag_; // base 1e9, little-endian, no leading zero limb

    void trim();  // drop spurious leading (most-significant) zero limbs

    static int cmp_mag(const std::vector<std::uint32_t>& a,
                       const std::vector<std::uint32_t>& b);
    static std::vector<std::uint32_t> add_mag(const std::vector<std::uint32_t>& a,
                                              const std::vector<std::uint32_t>& b);
    static std::vector<std::uint32_t> sub_mag(const std::vector<std::uint32_t>& a,
                                              const std::vector<std::uint32_t>& b);
    static std::vector<std::uint32_t> mul_mag(const std::vector<std::uint32_t>& a,
                                              const std::vector<std::uint32_t>& b);
    static void divmod_mag(const std::vector<std::uint32_t>& a,
                           const std::vector<std::uint32_t>& b,
                           std::vector<std::uint32_t>& q,
                           std::vector<std::uint32_t>& r);
};

}  // namespace nemi

#endif  // NEMI_BIGINT_HPP
