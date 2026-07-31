// Rational implementation — exact arithmetic built on top of BigInt.
#include "nemi/numeric.hpp"

#include <stdexcept>
#include <utility>

namespace nemi {

Rational::Rational(Integer n, Integer d) : num(std::move(n)), den(std::move(d)) {
    normalize();
}

void Rational::normalize() {
    if (den.is_zero()) throw std::domain_error("Rational: zero denominator");
    if (den.is_negative()) {
        num = -num;
        den = -den;
    }
    Integer abs_num = num.is_negative() ? -num : num;
    Integer g = Integer::gcd(abs_num, den);
    if (!g.is_zero() && !(g == Integer(1))) {
        num = num / g;  // exact: g divides both num and den by construction
        den = den / g;
    }
}

bool Rational::is_integer() const { return den == Integer(1); }

double Rational::to_double() const { return num.to_double() / den.to_double(); }

Integer Rational::floor() const { return num.floor_div(den); }

Integer Rational::ceil() const {
    Integer q = num.floor_div(den);
    Integer r = num.mod_floor(den);
    return r.is_zero() ? q : q + Integer(1);
}

Rational Rational::operator-() const { return Rational(-num, den); }

Rational Rational::operator+(const Rational& other) const {
    return Rational(num * other.den + other.num * den, den * other.den);
}

Rational Rational::operator-(const Rational& other) const {
    return Rational(num * other.den - other.num * den, den * other.den);
}

Rational Rational::operator*(const Rational& other) const {
    return Rational(num * other.num, den * other.den);
}

Rational Rational::operator/(const Rational& other) const {
    if (other.num.is_zero()) throw std::domain_error("Rational division by zero");
    return Rational(num * other.den, den * other.num);
}

bool Rational::operator==(const Rational& other) const {
    return num == other.num && den == other.den;  // both sides always normalized
}
bool Rational::operator!=(const Rational& other) const { return !(*this == other); }

bool Rational::operator<(const Rational& other) const {
    // den and other.den are both positive (normalize()), so cross-multiplying
    // preserves the inequality direction without losing precision to double.
    return (num * other.den) < (other.num * den);
}
bool Rational::operator<=(const Rational& other) const { return *this < other || *this == other; }
bool Rational::operator>(const Rational& other) const { return other < *this; }
bool Rational::operator>=(const Rational& other) const { return !(*this < other); }

}  // namespace nemi
