// Token: a kind, its literal payload, and a source location.
// Mirrors nemi/tokens.py's Token. The payload is a tagged union: Integer for
// Int, Real for Real, string for String/Ident/Prose, monostate otherwise.
#ifndef NEMI_TOKEN_HPP
#define NEMI_TOKEN_HPP

#include <string>
#include <variant>

#include "nemi/numeric.hpp"
#include "nemi/source_location.hpp"
#include "nemi/token_kind.hpp"

namespace nemi {

using TokenValue = std::variant<std::monostate, Integer, Real, std::string>;

struct Token {
    TokenKind kind;
    TokenValue value;
    SourceLocation location;

    // Convenience accessors (throw std::bad_variant_access if wrong kind).
    Integer int_value() const { return std::get<Integer>(value); }
    Real real_value() const { return std::get<Real>(value); }
    const std::string& string_value() const { return std::get<std::string>(value); }
};

}  // namespace nemi

#endif  // NEMI_TOKEN_HPP
