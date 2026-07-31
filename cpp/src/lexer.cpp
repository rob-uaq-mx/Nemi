// Lexer implementation — ported from nemi/lexer.py (Fase 1 of backlog.md).
//
// Single pass over the source, decoded once to Unicode code points
// (std::u32string) so the scanner never has to reason about UTF-8 byte
// lengths. See PORTING_GUIDE.md "Fase 1" for the design walkthrough.
#include "nemi/lexer.hpp"

#include <stdexcept>
#include <unordered_map>

#include "nemi/errors.hpp"

namespace nemi {
namespace {

// -- UTF-8 <-> code points ---------------------------------------------
std::u32string decode_utf8(const std::string& bytes) {
    std::u32string out;
    std::size_t i = 0;
    while (i < bytes.size()) {
        unsigned char c = static_cast<unsigned char>(bytes[i]);
        char32_t cp;
        int len;
        if (c < 0x80)      { cp = c;        len = 1; }
        else if (c < 0xE0) { cp = c & 0x1F; len = 2; }
        else if (c < 0xF0) { cp = c & 0x0F; len = 3; }
        else               { cp = c & 0x07; len = 4; }
        for (int k = 1; k < len && i + k < bytes.size(); ++k) {
            cp = (cp << 6) | (static_cast<unsigned char>(bytes[i + k]) & 0x3F);
        }
        out.push_back(cp);
        i += static_cast<std::size_t>(len);
    }
    return out;
}

std::string encode_utf8(const std::u32string& s) {
    std::string out;
    for (char32_t cp : s) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

// -- character classes ---------------------------------------------------
bool is_digit(char32_t c) { return c >= U'0' && c <= U'9'; }

bool is_letter(char32_t c) {
    if ((c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_') {
        return true;
    }
    // Accented Spanish letters used in Nemi identifiers (función, máximo,
    // raíz, ...). Deliberately an explicit whitelist rather than a blanket
    // "c >= 0x80" rule: several *operator* symbols (←, ≤, √, ⌊, ...) also
    // live above 0x80 and must NOT be swallowed by the identifier scanner.
    static const std::u32string kAccented = U"áéíóúüñÁÉÍÓÚÜÑ";
    return kAccented.find(c) != std::u32string::npos;
}

// -- keyword / operator tables (spec §9) ---------------------------------
using K = TokenKind;

const std::unordered_map<std::string, TokenKind>& keywords() {
    static const std::unordered_map<std::string, TokenKind> table = {
        {"función", K::Function}, {"funcion", K::Function},
        {"procedimiento", K::Procedure},
        {"para", K::For},
        {"hasta", K::To},
        {"repite", K::Repeat},
        {"mientras", K::While},
        {"si", K::If},
        {"entonces", K::Then},
        {"alt", K::Else}, {"alternativamente", K::Else},
        {"regresa", K::Return},
        {"fin", K::End},
        {"incluye", K::Include},
        {"mod", K::Mod},
        {"y", K::And},
        {"o", K::Or},
        {"no", K::Not},
    };
    return table;
}

const std::unordered_map<std::u32string, TokenKind>& two_char_ops() {
    static const std::unordered_map<std::u32string, TokenKind> table = {
        {U"<-", K::Assign},
        {U"!=", K::Ne},
        {U"<=", K::Le},
        {U">=", K::Ge},
    };
    return table;
}

const std::unordered_map<char32_t, TokenKind>& one_char_ops() {
    static const std::unordered_map<char32_t, TokenKind> table = {
        {U'←', K::Assign},
        {U'=', K::Eq},
        {U'≠', K::Ne},
        {U'<', K::Lt},
        {U'≤', K::Le},
        {U'>', K::Gt},
        {U'≥', K::Ge},
        {U'+', K::Plus},
        {U'−', K::Minus}, {U'-', K::Minus},
        {U'·', K::Times}, {U'*', K::Times},
        {U'/', K::Divide},
        {U'∧', K::And},
        {U'∨', K::Or},
        {U'¬', K::Not},
        {U'(', K::LParen},
        {U')', K::RParen},
        {U'[', K::LBracket},
        {U']', K::RBracket},
        {U'⌊', K::LFloor},
        {U'⌋', K::RFloor},
        {U'⌈', K::LCeil},
        {U'⌉', K::RCeil},
        {U'√', K::Sqrt},
        {U',', K::Comma},
    };
    return table;
}

bool is_comment_start(char32_t c) { return c == U'#' || c == U'▷'; }

// Integer is BigInt (Fase 4): no overflow, unlike the earlier std::stoll-based
// placeholder — an integer literal of any length just works.
Integer parse_integer(const std::string& digits) {
    return Integer::from_decimal(digits);
}

}  // namespace

Lexer::Lexer(const std::string& source, std::string file)
    : src_(decode_utf8(source)), file_(std::move(file)) {}

// -- cursor helpers -------------------------------------------------------
bool Lexer::at_end() const { return pos_ >= src_.size(); }

char32_t Lexer::peek(std::size_t ahead) const {
    std::size_t i = pos_ + ahead;
    return i < src_.size() ? src_[i] : U'\0';
}

char32_t Lexer::advance() {
    char32_t c = src_[pos_++];
    if (c == U'\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

SourceLocation Lexer::loc() const { return SourceLocation{line_, col_, file_}; }

// -- main loop --------------------------------------------------------------
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skip_trivia();
        if (at_end()) {
            tokens.push_back(Token{TokenKind::Eof, std::monostate{}, loc()});
            return tokens;
        }
        tokens.push_back(next_token());
    }
}

void Lexer::skip_trivia() {
    while (!at_end()) {
        char32_t c = peek();
        if (c == U' ' || c == U'\t' || c == U'\r' || c == U'\n') {
            advance();
        } else if (is_comment_start(c)) {
            while (!at_end() && peek() != U'\n') advance();
        } else {
            return;
        }
    }
}

Token Lexer::next_token() {
    SourceLocation start = loc();
    char32_t c = peek();

    if (c == U'"') return scan_string(start);
    if (c == U'«') return scan_prose(start);
    if (is_digit(c)) return scan_number(start);
    if (is_letter(c)) return scan_word(start);

    // Two-character ASCII operators are checked before single characters.
    std::u32string pair;
    pair.push_back(c);
    pair.push_back(peek(1));
    const auto& two = two_char_ops();
    auto two_it = two.find(pair);
    if (two_it != two.end()) {
        advance();
        advance();
        return Token{two_it->second, std::monostate{}, start};
    }

    const auto& one = one_char_ops();
    auto one_it = one.find(c);
    if (one_it != one.end()) {
        advance();
        return Token{one_it->second, std::monostate{}, start};
    }

    throw LexicalError(
        "unexpected character '" + encode_utf8(std::u32string(1, c)) + "'",
        start);
}

// -- token scanners -----------------------------------------------------
Token Lexer::scan_number(SourceLocation start) {
    std::u32string digits;
    while (!at_end() && is_digit(peek())) digits.push_back(advance());

    // A real number: digits '.' digits (a trailing dot with no digit after
    // it is left for next_token, which will report it as unexpected).
    if (peek() == U'.' && is_digit(peek(1))) {
        digits.push_back(advance());  // the dot
        while (!at_end() && is_digit(peek())) digits.push_back(advance());
        return Token{TokenKind::Real, std::stod(encode_utf8(digits)), start};
    }
    return Token{TokenKind::Int, parse_integer(encode_utf8(digits)), start};
}

Token Lexer::scan_word(SourceLocation start) {
    std::u32string word;
    while (!at_end() && (is_letter(peek()) || is_digit(peek()))) {
        word.push_back(advance());
    }
    std::string text = encode_utf8(word);
    const auto& kw = keywords();
    auto it = kw.find(text);
    if (it != kw.end()) return Token{it->second, std::monostate{}, start};
    return Token{TokenKind::Ident, text, start};
}

Token Lexer::scan_string(SourceLocation start) {
    advance();  // opening quote
    std::u32string chars;
    while (true) {
        if (at_end()) throw LexicalError("unterminated string literal", start);
        char32_t c = advance();
        if (c == U'"') break;
        if (c == U'\\' && !at_end()) {
            chars.push_back(advance());  // minimal escape support
        } else {
            chars.push_back(c);
        }
    }
    return Token{TokenKind::String, encode_utf8(chars), start};
}

Token Lexer::scan_prose(SourceLocation start) {
    advance();  // opening «
    std::u32string chars;
    while (true) {
        if (at_end()) {
            throw LexicalError("unterminated prose action « … »", start);
        }
        char32_t c = advance();
        if (c == U'»') break;
        chars.push_back(c);
    }
    std::string text = encode_utf8(chars);
    const char* ws = " \t\r\n";  // mirror Python's str.strip()
    auto b = text.find_first_not_of(ws);
    if (b == std::string::npos) return Token{TokenKind::Prose, std::string(), start};
    auto e = text.find_last_not_of(ws);
    return Token{TokenKind::Prose, text.substr(b, e - b + 1), start};
}

}  // namespace nemi
