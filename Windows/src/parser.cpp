// Parser implementation — ported from nemi/parser.py (Fase 2 of backlog.md).
//
// Recursive descent, one method per grammar production (§10); one token of
// lookahead. See PORTING_GUIDE.md "Fase 2" for the design walkthrough.
#include "nemi/parser.hpp"

#include "nemi/errors.hpp"

namespace nemi {
namespace {

// A block ends where a comparison operator could start (§10 op_comp).
bool is_comparison(TokenKind k) {
    return k == TokenKind::Eq || k == TokenKind::Ne || k == TokenKind::Lt ||
           k == TokenKind::Le || k == TokenKind::Gt || k == TokenKind::Ge ||
           k == TokenKind::In || k == TokenKind::Within || k == TokenKind::NotIn ||
           k == TokenKind::SubsetEq || k == TokenKind::Subset;
}
// TokenKind::Within ("en") is a word-only synonym for In (∈) here -- no
// ASCII-symbol equivalent otherwise, unlike !=/<=/>=. Reused in two grammar
// positions: this one (membership) and right after "cada" in
// parse_for_each() (loop syntax) -- no ambiguity because parse_for_each
// consumes it by direct position, never through this general production.

ExprPtr make_binary(TokenKind op, ExprPtr left, ExprPtr right, SourceLocation loc) {
    auto node = std::make_unique<Binary>();
    node->op = op;
    node->left = std::move(left);
    node->right = std::move(right);
    node->location = loc;
    return node;
}

ExprPtr make_unary(TokenKind op, ExprPtr operand, SourceLocation loc) {
    auto node = std::make_unique<Unary>();
    node->op = op;
    node->operand = std::move(operand);
    node->location = loc;
    return node;
}

}  // namespace

// -- token cursor -----------------------------------------------------------
const Token& Parser::peek() const { return tokens_[i_]; }

const Token& Parser::advance() {
    const Token& tok = tokens_[i_];
    if (tok.kind != TokenKind::Eof) ++i_;
    return tok;
}

bool Parser::check(TokenKind kind) const { return peek().kind == kind; }

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenKind kind, const char* what) {
    const Token& tok = peek();
    if (tok.kind != kind) {
        throw ParseError(std::string("se esperaba ") + what + ", se encontró " +
                         to_string(tok.kind), tok.location);
    }
    return advance();
}

// -- program / definitions ---------------------------------------------------
Program Parser::parse_program() {
    // programa ::= { definición | instrucción }
    // Definitions and top-level statements may interleave; we sort them into
    // `definitions` (hoisted) and `main` (run in source order, Fase 5).
    Program program;
    while (!check(TokenKind::Eof)) {
        if (check(TokenKind::Function) || check(TokenKind::Procedure)) {
            program.definitions.push_back(parse_definition());
        } else {
            program.main.push_back(parse_statement());
        }
    }
    return program;
}

DefinitionPtr Parser::parse_definition() {
    const Token& tok = peek();
    bool is_function;
    if (tok.kind == TokenKind::Function) {
        is_function = true;
    } else if (tok.kind == TokenKind::Procedure) {
        is_function = false;
    } else {
        throw ParseError("se esperaba 'función' o 'procedimiento'", tok.location);
    }
    advance();
    std::string name = expect(TokenKind::Ident, "nombre de definición").string_value();
    expect(TokenKind::LParen, "'('");
    std::vector<std::string> params = parse_params();
    expect(TokenKind::RParen, "')'");
    Block body = parse_block();
    expect(TokenKind::End, "'fin'");
    // The closing keyword must match the opener.
    TokenKind closer = is_function ? TokenKind::Function : TokenKind::Procedure;
    expect(closer, "'función'/'procedimiento' después de 'fin'");

    auto def = std::make_unique<Definition>();
    def->name = std::move(name);
    def->params = std::move(params);
    def->body = std::move(body);
    def->is_function = is_function;
    def->location = tok.location;
    return def;
}

std::vector<std::string> Parser::parse_params() {
    std::vector<std::string> params;
    if (check(TokenKind::Ident)) {
        params.push_back(advance().string_value());
        while (match(TokenKind::Comma)) {
            params.push_back(expect(TokenKind::Ident, "nombre de parámetro").string_value());
        }
    }
    return params;
}

// -- blocks and statements ---------------------------------------------------
bool Parser::at_block_end() const {
    TokenKind k = peek().kind;
    return k == TokenKind::End || k == TokenKind::Else || k == TokenKind::Eof;
}

Block Parser::parse_block() {
    Block statements;
    while (!at_block_end()) {
        statements.push_back(parse_statement());
    }
    return statements;
}

StmtPtr Parser::parse_statement() {
    TokenKind kind = peek().kind;
    if (kind == TokenKind::For) {
        // Disambiguate 'para cada ... en ...' from 'para ident <- ... hasta
        // ...' (one token of extra lookahead; Eof always follows any real
        // token, so i_ + 1 is always in bounds here).
        if (tokens_[i_ + 1].kind == TokenKind::Each) return parse_for_each();
        return parse_for();
    }
    if (kind == TokenKind::While) return parse_while();
    if (kind == TokenKind::If) return parse_if();
    if (kind == TokenKind::Return) return parse_return();
    if (kind == TokenKind::Prose) {
        const Token& tok = advance();
        auto node = std::make_unique<Prose>();
        node->text = tok.string_value();
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::Assert) return parse_assert();
    if (kind == TokenKind::Trace) return parse_trace();
    if (kind == TokenKind::Ident) return parse_assign_or_call();

    const Token& tok = peek();
    throw ParseError(std::string(to_string(tok.kind)) +
                     " inesperado al inicio de la instrucción", tok.location);
}

StmtPtr Parser::parse_for() {
    const Token& tok = advance();  // para
    std::string var = expect(TokenKind::Ident, "variable de ciclo").string_value();
    expect(TokenKind::Assign, "'←'");
    ExprPtr start = parse_expression();
    expect(TokenKind::To, "'hasta'");
    ExprPtr end = parse_expression();
    match(TokenKind::Repeat);  // optional 'repite'
    Block body = parse_block();
    expect(TokenKind::End, "'fin'");
    expect(TokenKind::For, "'para' después de 'fin'");

    auto node = std::make_unique<ForLoop>();
    node->var = std::move(var);
    node->start = std::move(start);
    node->end = std::move(end);
    node->body = std::move(body);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_for_each() {
    // ciclo_cada ::= "para" "cada" ident "en" expresión [ "repite" ]
    //                bloque "fin" "para"   (spec §20.3, v0.2)
    const Token& tok = advance();  // para
    advance();  // cada
    std::string var = expect(TokenKind::Ident, "variable de ciclo").string_value();
    expect(TokenKind::Within, "'en'");
    ExprPtr collection = parse_expression();
    match(TokenKind::Repeat);  // optional 'repite'
    Block body = parse_block();
    expect(TokenKind::End, "'fin'");
    expect(TokenKind::For, "'para' después de 'fin'");

    auto node = std::make_unique<ForEach>();
    node->var = std::move(var);
    node->collection = std::move(collection);
    node->body = std::move(body);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_while() {
    const Token& tok = advance();  // mientras
    ExprPtr condition = parse_expression();
    Block body = parse_block();
    expect(TokenKind::End, "'fin'");
    expect(TokenKind::While, "'mientras' después de 'fin'");

    auto node = std::make_unique<WhileLoop>();
    node->condition = std::move(condition);
    node->body = std::move(body);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_if() {
    const Token& tok = advance();  // si
    ExprPtr condition = parse_expression();
    match(TokenKind::Then);  // optional 'entonces'
    Block then_body = parse_block();
    Block else_body;
    bool has_else = false;
    if (match(TokenKind::Else)) {  // 'alt'
        has_else = true;
        else_body = parse_block();
    }
    expect(TokenKind::End, "'fin'");
    expect(TokenKind::If, "'si' después de 'fin'");

    auto node = std::make_unique<If>();
    node->condition = std::move(condition);
    node->then_body = std::move(then_body);
    node->else_body = std::move(else_body);
    node->has_else = has_else;
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_return() {
    const Token& tok = advance();  // regresa
    ExprPtr value;  // null = bare 'regresa'
    if (!at_block_end()) {
        value = parse_expression();
    }
    auto node = std::make_unique<Return>();
    node->value = std::move(value);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_assign_or_call() {
    const Token& tok = peek();
    std::string name = advance().string_value();  // Ident
    if (check(TokenKind::LParen)) {
        ExprPtr call = finish_call(name, tok.location);
        auto node = std::make_unique<ExprStatement>();
        node->call = std::move(call);
        node->location = tok.location;
        return node;
    }
    // Otherwise a place (assignment target): ident { '[' expr ']' }.
    std::vector<ExprPtr> indices;
    while (match(TokenKind::LBracket)) {
        indices.push_back(parse_expression());
        expect(TokenKind::RBracket, "']'");
    }
    expect(TokenKind::Assign, "'←' (asignación) o '(' (llamada)");
    ExprPtr value = parse_expression();

    auto node = std::make_unique<Assign>();
    node->name = std::move(name);
    node->indices = std::move(indices);
    node->value = std::move(value);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_assert() {
    // aserción ::= "afirma" expresión [ "," cadena ]   (spec §21.2, v0.2)
    const Token& tok = advance();  // afirma
    ExprPtr condition = parse_expression();
    std::optional<std::string> message;
    if (match(TokenKind::Comma)) {
        message = expect(TokenKind::String, "cadena de mensaje de la aserción").string_value();
    }

    auto node = std::make_unique<Assert>();
    node->condition = std::move(condition);
    node->message = std::move(message);
    node->location = tok.location;
    return node;
}

StmtPtr Parser::parse_trace() {
    // rastreo ::= "traza" expresión   (spec §21.3, v0.2 [OPC])
    const Token& tok = advance();  // traza
    ExprPtr expression = parse_expression();

    auto node = std::make_unique<Trace>();
    node->expression = std::move(expression);
    node->location = tok.location;
    return node;
}

// -- expressions (precedence tower) ------------------------------------------
ExprPtr Parser::parse_expression() { return parse_logic_or(); }

StmtPtr Parser::parse_single_statement() {
    StmtPtr stmt = parse_statement();
    if (!check(TokenKind::Eof)) {
        throw ParseError("entrada inesperada después de la instrucción", peek().location);
    }
    return stmt;
}

bool Parser::is_at_end() const { return check(TokenKind::Eof); }

ExprPtr Parser::parse_logic_or() {
    ExprPtr left = parse_logic_and();
    while (check(TokenKind::Or)) {
        const Token& op = advance();
        ExprPtr right = parse_logic_and();
        left = make_binary(TokenKind::Or, std::move(left), std::move(right), op.location);
    }
    return left;
}

ExprPtr Parser::parse_logic_and() {
    ExprPtr left = parse_logic_not();
    while (check(TokenKind::And)) {
        const Token& op = advance();
        ExprPtr right = parse_logic_not();
        left = make_binary(TokenKind::And, std::move(left), std::move(right), op.location);
    }
    return left;
}

ExprPtr Parser::parse_logic_not() {
    // expr_no ::= [ "¬" | "no" ] expr_comp
    if (check(TokenKind::Not)) {
        const Token& op = advance();
        ExprPtr operand = parse_comparison();
        return make_unary(TokenKind::Not, std::move(operand), op.location);
    }
    return parse_comparison();
}

ExprPtr Parser::parse_comparison() {
    ExprPtr left = parse_sum();
    if (is_comparison(peek().kind)) {
        const Token& op = advance();
        ExprPtr right = parse_sum();
        return make_binary(op.kind, std::move(left), std::move(right), op.location);
    }
    return left;
}

ExprPtr Parser::parse_sum() {
    ExprPtr left = parse_product();
    while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
        const Token& op = advance();
        ExprPtr right = parse_product();
        left = make_binary(op.kind, std::move(left), std::move(right), op.location);
    }
    return left;
}

ExprPtr Parser::parse_product() {
    ExprPtr left = parse_unary();
    while (peek().kind == TokenKind::Times || peek().kind == TokenKind::Divide ||
           peek().kind == TokenKind::Mod) {
        const Token& op = advance();
        ExprPtr right = parse_unary();
        left = make_binary(op.kind, std::move(left), std::move(right), op.location);
    }
    return left;
}

ExprPtr Parser::parse_unary() {
    // expr_unaria ::= [ "−" | "¬" ] expr_postfija
    if (peek().kind == TokenKind::Minus || peek().kind == TokenKind::Not) {
        const Token& op = advance();
        ExprPtr operand = parse_postfix();
        return make_unary(op.kind, std::move(operand), op.location);
    }
    return parse_postfix();
}

ExprPtr Parser::parse_postfix() {
    ExprPtr expr = parse_primary();
    while (match(TokenKind::LBracket)) {
        ExprPtr index = parse_expression();
        expect(TokenKind::RBracket, "']'");
        SourceLocation loc = expr->location;
        auto node = std::make_unique<Index>();
        node->base = std::move(expr);
        node->index = std::move(index);
        node->location = loc;
        expr = std::move(node);
    }
    return expr;
}

ExprPtr Parser::parse_primary() {
    const Token& tok = peek();
    TokenKind kind = tok.kind;

    if (kind == TokenKind::Int) {
        advance();
        auto node = std::make_unique<IntLiteral>(tok.int_value());
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::Real) {
        advance();
        auto node = std::make_unique<RealLiteral>(tok.real_value());
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::String) {
        advance();
        auto node = std::make_unique<StringLiteral>(tok.string_value());
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::LBracket) {
        return parse_array_literal();
    }
    if (kind == TokenKind::LBrace) {
        return parse_set_literal();
    }
    if (kind == TokenKind::EmptySet) {
        advance();
        auto node = std::make_unique<SetLiteral>();
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::Ident) {
        advance();
        if (check(TokenKind::LParen)) {
            return finish_call(tok.string_value(), tok.location);
        }
        auto node = std::make_unique<Variable>(tok.string_value());
        node->location = tok.location;
        return node;
    }
    if (kind == TokenKind::LParen) {
        advance();
        ExprPtr expr = parse_expression();
        expect(TokenKind::RParen, "')'");
        return expr;
    }
    if (kind == TokenKind::LFloor) {
        advance();
        ExprPtr expr = parse_expression();
        expect(TokenKind::RFloor, "'⌋'");
        return make_unary(TokenKind::LFloor, std::move(expr), tok.location);
    }
    if (kind == TokenKind::LCeil) {
        advance();
        ExprPtr expr = parse_expression();
        expect(TokenKind::RCeil, "'⌉'");
        return make_unary(TokenKind::LCeil, std::move(expr), tok.location);
    }
    if (kind == TokenKind::Sqrt) {
        advance();
        ExprPtr operand = parse_unary();  // √ binds like a unary prefix (√ expr_unaria)
        return make_unary(TokenKind::Sqrt, std::move(operand), tok.location);
    }

    throw ParseError(std::string(to_string(kind)) + " inesperado en la expresión",
                     tok.location);
}

ExprPtr Parser::parse_array_literal() {
    const Token& tok = expect(TokenKind::LBracket, "'['");
    std::vector<ExprPtr> elements;
    if (!check(TokenKind::RBracket)) {
        elements.push_back(parse_expression());
        while (match(TokenKind::Comma)) {
            elements.push_back(parse_expression());
        }
    }
    expect(TokenKind::RBracket, "']'");

    auto node = std::make_unique<ArrayLiteral>();
    node->elements = std::move(elements);
    node->location = tok.location;
    return node;
}

ExprPtr Parser::parse_set_literal() {
    // conjunto_lit ::= "{" [ args ] "}" | "∅"  (spec §20.1, v0.2)
    const Token& tok = expect(TokenKind::LBrace, "'{'");
    std::vector<ExprPtr> elements;
    if (!check(TokenKind::RBrace)) {
        elements.push_back(parse_expression());
        while (match(TokenKind::Comma)) {
            elements.push_back(parse_expression());
        }
    }
    expect(TokenKind::RBrace, "'}'");

    auto node = std::make_unique<SetLiteral>();
    node->elements = std::move(elements);
    node->location = tok.location;
    return node;
}

ExprPtr Parser::finish_call(const std::string& name, SourceLocation location) {
    expect(TokenKind::LParen, "'('");
    std::vector<ExprPtr> args;
    if (!check(TokenKind::RParen)) {
        args.push_back(parse_expression());
        while (match(TokenKind::Comma)) {
            args.push_back(parse_expression());
        }
    }
    expect(TokenKind::RParen, "')'");

    auto node = std::make_unique<Call>();
    node->name = name;
    node->args = std::move(args);
    node->location = location;
    return node;
}

}  // namespace nemi
