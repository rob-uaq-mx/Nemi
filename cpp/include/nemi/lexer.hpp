// Lexer: UTF-8 source text -> token list (spec §9). Mirrors nemi/lexer.py.
//
// Single pass, one code point of lookahead, whitespace-insensitive (blocks use
// explicit `fin` closers). Unicode and ASCII operator spellings lex to the same
// TokenKind. See ARCHITECTURE.md §2 and cpp/PORTING_GUIDE.md Fase 1 for the
// UTF-8 handling approach: the source is decoded to char32_t code points once
// in the constructor, so the scanner never has to reason about UTF-8 bytes.
#ifndef NEMI_LEXER_HPP
#define NEMI_LEXER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "nemi/source_location.hpp"
#include "nemi/token.hpp"

namespace nemi {

class Lexer {
public:
    // `file`, if given, tags every token's SourceLocation so later errors
    // (and multi-file features like `incluye`, see loader.hpp) point at the
    // file that actually has the problem instead of a bare line number.
    explicit Lexer(const std::string& source, std::string file = "");

    // Scan the whole input; the stream always ends with an Eof token.
    std::vector<Token> tokenize();

private:
    std::u32string src_;   // the source, decoded to Unicode code points
    std::string file_;     // tag applied to every token's SourceLocation
    std::size_t pos_ = 0;  // index into src_
    int line_ = 1;
    int col_ = 1;

    // -- cursor helpers ---------------------------------------------------
    bool at_end() const;
    char32_t peek(std::size_t ahead = 0) const;
    char32_t advance();
    SourceLocation loc() const;

    // -- scanning -----------------------------------------------------------
    void skip_trivia();               // whitespace + '#'/'▷' comments
    Token next_token();
    Token scan_number(SourceLocation start);
    Token scan_word(SourceLocation start);     // identifier or keyword
    Token scan_string(SourceLocation start);   // "..."
    Token scan_prose(SourceLocation start);    // «...»
};

}  // namespace nemi

#endif  // NEMI_LEXER_HPP
