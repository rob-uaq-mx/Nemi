// TokenKind: the closed set of lexical categories (spec §9).
// Mirrors nemi/tokens.py's TokenKind enum. The Spanish keywords belong to the
// language; the enumerator names are English.
#ifndef NEMI_TOKEN_KIND_HPP
#define NEMI_TOKEN_KIND_HPP

namespace nemi {

enum class TokenKind {
    // Keywords
    Function,    // función
    Procedure,   // procedimiento
    For,         // para
    To,          // hasta
    Repeat,      // repite    (optional sugar)
    While,       // mientras
    If,          // si
    Then,        // entonces  (optional sugar)
    Else,        // alt / alternativamente
    Return,      // regresa
    End,         // fin
    Include,     // incluye

    // Operators (the Spanish logical words y/o/no map to And/Or/Not)
    Assign,      // ←  <-
    Eq,          // =
    Ne,          // ≠  !=
    Lt,          // <
    Le,          // ≤  <=
    Gt,          // >
    Ge,          // ≥  >=
    Plus,        // +
    Minus,       // −  -
    Times,       // ·  *
    Divide,      // /
    Mod,         // mod
    And,         // ∧  or the word  y
    Or,          // ∨  or the word  o
    Not,         // ¬  or the word  no

    // Bracketing
    LParen, RParen,
    LBracket, RBracket,
    LFloor, RFloor,      // ⌊ ⌋
    LCeil, RCeil,        // ⌈ ⌉
    Sqrt,                // √
    Comma,

    // Literals and names
    Int, Real, String, Ident,
    Prose,               // « ... »

    Eof
};

const char* to_string(TokenKind kind);

}  // namespace nemi

#endif  // NEMI_TOKEN_KIND_HPP
