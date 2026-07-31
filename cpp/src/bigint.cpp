// BigInt implementation — schoolbook algorithms over base-1e9 limbs.
// See bigint.hpp for the representation invariants.
#include "nemi/bigint.hpp"

#include <stdexcept>

namespace nemi {

namespace {
constexpr std::uint64_t kBase = 1000000000ULL;  // 1e9
}

// -- construction -----------------------------------------------------------
BigInt::BigInt(long long value) {
    negative_ = value < 0;
    // Take the absolute value via unsigned arithmetic so LLONG_MIN (whose
    // magnitude does not fit in a signed long long) does not overflow.
    unsigned long long mag =
        negative_ ? static_cast<unsigned long long>(-(value + 1)) + 1ULL
                  : static_cast<unsigned long long>(value);
    while (mag > 0) {
        mag_.push_back(static_cast<std::uint32_t>(mag % kBase));
        mag /= kBase;
    }
    trim();
}

BigInt BigInt::from_decimal(const std::string& digits) {
    BigInt result;
    const BigInt ten(10);
    for (char c : digits) {
        int d = c - '0';
        result = result * ten + BigInt(static_cast<long long>(d));
    }
    return result;
}

void BigInt::trim() {
    while (!mag_.empty() && mag_.back() == 0) mag_.pop_back();
    if (mag_.empty()) negative_ = false;  // canonical zero
}

// -- magnitude helpers (unsigned arithmetic on the limb vectors) ------------
int BigInt::cmp_mag(const std::vector<std::uint32_t>& a,
                    const std::vector<std::uint32_t>& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (std::size_t i = a.size(); i-- > 0;) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

std::vector<std::uint32_t> BigInt::add_mag(const std::vector<std::uint32_t>& a,
                                           const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> out;
    std::size_t n = a.size() > b.size() ? a.size() : b.size();
    out.reserve(n + 1);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < n || carry; ++i) {
        std::uint64_t sum = carry;
        if (i < a.size()) sum += a[i];
        if (i < b.size()) sum += b[i];
        out.push_back(static_cast<std::uint32_t>(sum % kBase));
        carry = sum / kBase;
    }
    return out;  // add_mag on canonical inputs never yields a spurious top zero
}

// Precondition: magnitude(a) >= magnitude(b).
std::vector<std::uint32_t> BigInt::sub_mag(const std::vector<std::uint32_t>& a,
                                           const std::vector<std::uint32_t>& b) {
    std::vector<std::uint32_t> out;
    out.reserve(a.size());
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        std::int64_t diff = static_cast<std::int64_t>(a[i]) - borrow -
                            (i < b.size() ? static_cast<std::int64_t>(b[i]) : 0);
        if (diff < 0) {
            diff += static_cast<std::int64_t>(kBase);
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.push_back(static_cast<std::uint32_t>(diff));
    }
    while (!out.empty() && out.back() == 0) out.pop_back();
    return out;
}

std::vector<std::uint32_t> BigInt::mul_mag(const std::vector<std::uint32_t>& a,
                                           const std::vector<std::uint32_t>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<std::uint64_t> acc(a.size() + b.size(), 0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < b.size(); ++j) {
            // a[i]*b[j] < 1e18, plus acc/carry (each < ~1e9): fits in uint64.
            std::uint64_t cur = acc[i + j] +
                                static_cast<std::uint64_t>(a[i]) * b[j] + carry;
            acc[i + j] = cur % kBase;
            carry = cur / kBase;
        }
        std::size_t k = i + b.size();
        while (carry) {
            std::uint64_t cur = acc[k] + carry;
            acc[k] = cur % kBase;
            carry = cur / kBase;
            ++k;
        }
    }
    std::vector<std::uint32_t> out(acc.size());
    for (std::size_t i = 0; i < acc.size(); ++i) out[i] = static_cast<std::uint32_t>(acc[i]);
    while (!out.empty() && out.back() == 0) out.pop_back();
    return out;
}

// Schoolbook long division: for each limb of `a` (most significant first),
// grow the running remainder by one limb and binary-search the largest
// quotient digit d in [0, BASE) with b*d <= remainder.
void BigInt::divmod_mag(const std::vector<std::uint32_t>& a,
                        const std::vector<std::uint32_t>& b,
                        std::vector<std::uint32_t>& q,
                        std::vector<std::uint32_t>& r) {
    q.assign(a.size(), 0);
    r.clear();
    for (std::size_t idx = a.size(); idx-- > 0;) {
        r.insert(r.begin(), a[idx]);
        while (!r.empty() && r.back() == 0) r.pop_back();  // keep r canonical

        std::uint32_t lo = 0, hi = static_cast<std::uint32_t>(kBase - 1);
        while (lo < hi) {
            std::uint32_t mid = lo + (hi - lo + 1) / 2;
            std::vector<std::uint32_t> trial = mul_mag(b, std::vector<std::uint32_t>{mid});
            if (cmp_mag(trial, r) <= 0) lo = mid; else hi = mid - 1;
        }
        q[idx] = lo;
        if (lo != 0) {
            std::vector<std::uint32_t> sub = mul_mag(b, std::vector<std::uint32_t>{lo});
            r = sub_mag(r, sub);
        }
    }
    while (!q.empty() && q.back() == 0) q.pop_back();
}

// -- arithmetic ---------------------------------------------------------
BigInt BigInt::operator-() const {
    BigInt result = *this;
    if (!result.mag_.empty()) result.negative_ = !result.negative_;
    return result;
}

BigInt BigInt::operator+(const BigInt& other) const {
    BigInt result;
    if (negative_ == other.negative_) {
        result.mag_ = add_mag(mag_, other.mag_);
        result.negative_ = negative_;
    } else {
        int c = cmp_mag(mag_, other.mag_);
        if (c == 0) return BigInt();
        if (c > 0) {
            result.mag_ = sub_mag(mag_, other.mag_);
            result.negative_ = negative_;
        } else {
            result.mag_ = sub_mag(other.mag_, mag_);
            result.negative_ = other.negative_;
        }
    }
    result.trim();
    return result;
}

BigInt BigInt::operator-(const BigInt& other) const { return *this + (-other); }

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt result;
    result.mag_ = mul_mag(mag_, other.mag_);
    result.negative_ = (negative_ != other.negative_) && !result.mag_.empty();
    return result;
}

BigInt BigInt::operator/(const BigInt& other) const {
    if (other.is_zero()) throw std::domain_error("BigInt division by zero");
    std::vector<std::uint32_t> q, r;
    divmod_mag(mag_, other.mag_, q, r);
    BigInt result;
    result.mag_ = std::move(q);
    result.negative_ = (negative_ != other.negative_) && !result.mag_.empty();
    return result;
}

BigInt BigInt::operator%(const BigInt& other) const {
    if (other.is_zero()) throw std::domain_error("BigInt modulo by zero");
    std::vector<std::uint32_t> q, r;
    divmod_mag(mag_, other.mag_, q, r);
    BigInt result;
    result.mag_ = std::move(r);
    result.negative_ = negative_ && !result.mag_.empty();  // sign of the dividend
    return result;
}

BigInt BigInt::floor_div(const BigInt& other) const {
    BigInt q = *this / other;
    BigInt r = *this % other;
    if (!r.is_zero() && (r.negative_ != other.negative_)) q = q - BigInt(1);
    return q;
}

BigInt BigInt::mod_floor(const BigInt& other) const {
    BigInt r = *this % other;
    if (!r.is_zero() && (r.negative_ != other.negative_)) r = r + other;
    return r;
}

// -- comparisons ----------------------------------------------------------
bool BigInt::operator==(const BigInt& other) const {
    return negative_ == other.negative_ && mag_ == other.mag_;
}
bool BigInt::operator!=(const BigInt& other) const { return !(*this == other); }

bool BigInt::operator<(const BigInt& other) const {
    if (negative_ != other.negative_) return negative_;  // negative < non-negative
    int c = cmp_mag(mag_, other.mag_);
    return negative_ ? c > 0 : c < 0;  // among negatives, larger magnitude = smaller value
}
bool BigInt::operator<=(const BigInt& other) const { return *this < other || *this == other; }
bool BigInt::operator>(const BigInt& other) const { return other < *this; }
bool BigInt::operator>=(const BigInt& other) const { return !(*this < other); }

// -- misc -------------------------------------------------------------------
bool BigInt::is_zero() const { return mag_.empty(); }
bool BigInt::is_negative() const { return negative_; }

std::string BigInt::to_decimal() const {
    if (mag_.empty()) return "0";
    std::string out;
    if (negative_) out += '-';
    out += std::to_string(mag_.back());  // most-significant limb: no zero-padding
    for (auto it = mag_.rbegin() + 1; it != mag_.rend(); ++it) {
        std::string chunk = std::to_string(*it);
        out += std::string(9 - chunk.size(), '0') + chunk;  // zero-pad to 9 digits
    }
    return out;
}

double BigInt::to_double() const {
    double result = 0.0;
    for (std::size_t i = mag_.size(); i-- > 0;) {
        result = result * 1e9 + static_cast<double>(mag_[i]);
    }
    return negative_ ? -result : result;
}

long long BigInt::to_llong() const {
    // Only meaningful for values that actually fit; callers (e.g. array
    // indexing) range-check against a machine-sized bound first.
    long long result = 0;
    for (std::size_t i = mag_.size(); i-- > 0;) {
        result = result * static_cast<long long>(kBase) + static_cast<long long>(mag_[i]);
    }
    return negative_ ? -result : result;
}

BigInt BigInt::isqrt(const BigInt& n) {
    if (n.is_negative()) throw std::domain_error("isqrt of a negative number");
    if (n.is_zero()) return BigInt();

    // Seed the Newton iteration with a power-of-ten guaranteed to be >= the
    // true root: a d-digit n satisfies sqrt(n) < 10^ceil(d/2), so 10^ceil(d/2)
    // is always a safe (if loose) upper bound. This must NOT go through
    // `double`/`long long`: for a root with more than ~19 digits, casting an
    // out-of-range double to long long is undefined behaviour, and the
    // resulting garbage seed can make the correction loop below (which
    // walks one unit at a time) effectively never terminate.
    std::string digits = n.to_decimal();  // n > 0 here, so no sign to strip
    std::size_t half = (digits.size() + 1) / 2;  // ceil(digit_count / 2)
    BigInt x = BigInt::from_decimal("1" + std::string(half, '0'));

    const BigInt two(2);
    while (true) {
        BigInt y = (x + n / x) / two;
        if (!(y < x)) break;  // converged
        x = y;
    }
    // Defensive correction (handles the rare off-by-one Newton can leave).
    while (x * x > n) x = x - BigInt(1);
    while ((x + BigInt(1)) * (x + BigInt(1)) <= n) x = x + BigInt(1);
    return x;
}

BigInt BigInt::gcd(BigInt a, BigInt b) {
    a.negative_ = false;
    b.negative_ = false;
    while (!b.is_zero()) {
        BigInt r = a % b;  // a, b non-negative here: truncating % == floor-mod
        a = b;
        b = r;
    }
    return a;
}

}  // namespace nemi
