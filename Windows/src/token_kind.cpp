#include "nemi/token_kind.hpp"

namespace nemi {

const char* to_string(TokenKind kind) {
    switch (kind) {
        case TokenKind::Function:  return "Function";
        case TokenKind::Procedure: return "Procedure";
        case TokenKind::For:       return "For";
        case TokenKind::To:        return "To";
        case TokenKind::Repeat:    return "Repeat";
        case TokenKind::While:     return "While";
        case TokenKind::If:        return "If";
        case TokenKind::Then:      return "Then";
        case TokenKind::Else:      return "Else";
        case TokenKind::Return:    return "Return";
        case TokenKind::End:       return "End";
        case TokenKind::Include:   return "Include";
        case TokenKind::Assign:    return "Assign";
        case TokenKind::Eq:        return "Eq";
        case TokenKind::Ne:        return "Ne";
        case TokenKind::Lt:        return "Lt";
        case TokenKind::Le:        return "Le";
        case TokenKind::Gt:        return "Gt";
        case TokenKind::Ge:        return "Ge";
        case TokenKind::Plus:      return "Plus";
        case TokenKind::Minus:     return "Minus";
        case TokenKind::Times:     return "Times";
        case TokenKind::Divide:    return "Divide";
        case TokenKind::Mod:       return "Mod";
        case TokenKind::And:       return "And";
        case TokenKind::Or:        return "Or";
        case TokenKind::Not:       return "Not";
        case TokenKind::LParen:    return "LParen";
        case TokenKind::RParen:    return "RParen";
        case TokenKind::LBracket:  return "LBracket";
        case TokenKind::RBracket:  return "RBracket";
        case TokenKind::LFloor:    return "LFloor";
        case TokenKind::RFloor:    return "RFloor";
        case TokenKind::LCeil:     return "LCeil";
        case TokenKind::RCeil:     return "RCeil";
        case TokenKind::Sqrt:      return "Sqrt";
        case TokenKind::Comma:     return "Comma";
        case TokenKind::Int:       return "Int";
        case TokenKind::Real:      return "Real";
        case TokenKind::String:    return "String";
        case TokenKind::Ident:     return "Ident";
        case TokenKind::Prose:     return "Prose";
        case TokenKind::Eof:       return "Eof";
    }
    return "?";
}

}  // namespace nemi
