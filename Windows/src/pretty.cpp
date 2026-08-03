#include "nemi/pretty.hpp"

#include <sstream>

namespace nemi {
namespace {

// Renders an operator as its Nemi symbol (e.g. Eq -> "=") rather than the
// bare enumerator name, so the output reads like Nemi source, not C++
// internals.
const char* op_symbol(TokenKind op) {
    switch (op) {
        case TokenKind::Eq: return "=";
        case TokenKind::Ne: return "\xE2\x89\xA0";       // ≠
        case TokenKind::Lt: return "<";
        case TokenKind::Le: return "\xE2\x89\xA4";       // ≤
        case TokenKind::Gt: return ">";
        case TokenKind::Ge: return "\xE2\x89\xA5";       // ≥
        case TokenKind::In: return "\xE2\x88\x88";       // ∈
        case TokenKind::Within: return "\xE2\x88\x88";   // ∈ ("en", word synonym)
        case TokenKind::NotIn: return "\xE2\x88\x89";    // ∉
        case TokenKind::SubsetEq: return "\xE2\x8A\x86"; // ⊆
        case TokenKind::Subset: return "\xE2\x8A\x82";   // ⊂
        case TokenKind::Plus: return "+";
        case TokenKind::Minus: return "\xE2\x88\x92";    // −
        case TokenKind::Times: return "\xC2\xB7";        // ·
        case TokenKind::Divide: return "/";
        case TokenKind::Mod: return "mod";
        case TokenKind::And: return "\xE2\x88\xA7";      // ∧
        case TokenKind::Or: return "\xE2\x88\xA8";       // ∨
        case TokenKind::Not: return "\xC2\xAC";          // ¬
        case TokenKind::Sqrt: return "\xE2\x88\x9A";     // √
        default: return to_string(op);
    }
}

}  // namespace

void print_expr_source(std::ostream& os, const Expr& e) {
    if (auto* n = dynamic_cast<const IntLiteral*>(&e)) { os << n->value.to_decimal(); return; }
    if (auto* n = dynamic_cast<const RealLiteral*>(&e)) { os << n->value; return; }
    if (auto* n = dynamic_cast<const StringLiteral*>(&e)) { os << '"' << n->value << '"'; return; }
    if (auto* n = dynamic_cast<const Variable*>(&e)) { os << n->name; return; }
    if (auto* n = dynamic_cast<const ArrayLiteral*>(&e)) {
        os << "[";
        for (std::size_t i = 0; i < n->elements.size(); ++i) {
            if (i) os << ", ";
            print_expr_source(os, *n->elements[i]);
        }
        os << "]";
        return;
    }
    if (auto* n = dynamic_cast<const SetLiteral*>(&e)) {
        if (n->elements.empty()) { os << "\xE2\x88\x85"; return; }  // ∅
        os << "{";
        for (std::size_t i = 0; i < n->elements.size(); ++i) {
            if (i) os << ", ";
            print_expr_source(os, *n->elements[i]);
        }
        os << "}";
        return;
    }
    if (auto* n = dynamic_cast<const Index*>(&e)) {
        print_expr_source(os, *n->base);
        os << "[";
        print_expr_source(os, *n->index);
        os << "]";
        return;
    }
    if (auto* n = dynamic_cast<const Call*>(&e)) {
        os << n->name << "(";
        for (std::size_t i = 0; i < n->args.size(); ++i) {
            if (i) os << ", ";
            print_expr_source(os, *n->args[i]);
        }
        os << ")";
        return;
    }
    if (auto* n = dynamic_cast<const Unary*>(&e)) {
        if (n->op == TokenKind::LFloor) {
            os << "\xE2\x8C\x8A";  // ⌊
            print_expr_source(os, *n->operand);
            os << "\xE2\x8C\x8B";  // ⌋
            return;
        }
        if (n->op == TokenKind::LCeil) {
            os << "\xE2\x8C\x88";  // ⌈
            print_expr_source(os, *n->operand);
            os << "\xE2\x8C\x89";  // ⌉
            return;
        }
        os << "(" << op_symbol(n->op) << " ";
        print_expr_source(os, *n->operand);
        os << ")";
        return;
    }
    if (auto* n = dynamic_cast<const Binary*>(&e)) {
        os << "(";
        print_expr_source(os, *n->left);
        os << " " << op_symbol(n->op) << " ";
        print_expr_source(os, *n->right);
        os << ")";
        return;
    }
    os << "?expr?";
}

std::string expr_to_source(const Expr& expr) {
    std::ostringstream ss;
    print_expr_source(ss, expr);
    return ss.str();
}

}  // namespace nemi
