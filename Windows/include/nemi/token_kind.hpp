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
    Each,        // cada      (v0.2 §20.3, "para cada ... en ...")
    Within,      // en        (v0.2 §20.3 -- distinct from In/∈ above, which
                  //            is set membership; both read "in" in English)
    While,       // mientras
    If,          // si
    Then,        // entonces  (optional sugar)
    Else,        // alt / alternativamente
    Return,      // regresa
    End,         // fin
    Include,     // incluye
    Assert,      // afirma    (v0.2 §21.2, autoverificación)
    Trace,       // traza     (v0.2 §21.3 [OPC], herramienta de comprensión)

    // Operators (the Spanish logical words y/o/no map to And/Or/Not)
    Assign,      // ←  <-
    Eq,          // =
    Ne,          // ≠  !=
    Lt,          // <
    Le,          // ≤  <=
    Gt,          // >
    Ge,          // ≥  >=
    In,          // ∈   (pertenece; v0.2 §20.2)
    NotIn,       // ∉   (v0.2 §20.2)
    SubsetEq,    // ⊆   (subconjunto; v0.2 §20.2)
    Subset,      // ⊂   (subconjunto propio; v0.2 §20.2)
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
    LBrace, RBrace,      // {  }   (v0.2 §20.1, literal de conjunto)
    EmptySet,            // ∅      (v0.2 §20.1: literal de conjunto vacío --
                          //         distinto del uso de ∅ como texto impreso
                          //         de "sin valor", ver value.cpp)

    // Literals and names
    Int, Real, String, Ident,
    Prose,               // « ... »

    Eof
};

const char* to_string(TokenKind kind);

}  // namespace nemi

#endif  // NEMI_TOKEN_KIND_HPP
