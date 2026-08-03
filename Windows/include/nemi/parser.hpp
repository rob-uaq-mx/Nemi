// Parser: tokens -> Program (spec §10, §11). Mirrors nemi/parser.py.
//
// Recursive descent, one method per production, one token of lookahead. The
// expression tower is (lowest to highest):
//   or → and → not → comparison → sum → product → unary → postfix → primary
// Blocks end at `fin`, the else keyword `alt`, or EOF. See ARCHITECTURE.md §4
// for the array-literal-primary note.
#ifndef NEMI_PARSER_HPP
#define NEMI_PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "nemi/ast.hpp"
#include "nemi/token.hpp"

namespace nemi {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    // Parse a whole compilation unit: programa ::= { definición | instrucción }.
    Program parse_program();

    // Parse a single expression (used by run_call to evaluate `--llama` text).
    ExprPtr parse_expression();

    // Parse exactly one instrucción (statement — assignment, a bare call,
    // etc.; NOT a función/procedimiento definition), requiring it to consume
    // the whole input. Used by Interpreter::run_line for one-line console
    // input such as `r ← factorial(5)`, which isn't valid as a bare
    // expression (the ← isn't an expression operator).
    StmtPtr parse_single_statement();

    // True once the cursor has reached the end of the token stream. Lets a
    // caller that tried parse_expression() confirm the whole input was
    // consumed (a trailing `← ...` after a variable, for instance, is not an
    // error to parse_expression() by itself — it just stops early).
    bool is_at_end() const;

private:
    std::vector<Token> tokens_;
    std::size_t i_ = 0;

    // -- token cursor -----------------------------------------------------
    const Token& peek() const;
    const Token& advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);                       // advances iff check() is true
    const Token& expect(TokenKind kind, const char* what);

    // -- program / definitions ---------------------------------------------
    DefinitionPtr parse_definition();
    std::vector<std::string> parse_params();

    // -- blocks and statements ---------------------------------------------
    bool at_block_end() const;   // 'fin' | 'alt' | Eof
    Block parse_block();
    StmtPtr parse_statement();
    StmtPtr parse_for();
    StmtPtr parse_for_each();   // para cada var en coleccion ... (spec §20.3, v0.2)
    StmtPtr parse_while();
    StmtPtr parse_if();
    StmtPtr parse_return();
    StmtPtr parse_assign_or_call();   // ident (call) or ident{[idx]} '←' expr
    StmtPtr parse_assert();   // afirma expr [, "mensaje"] (spec §21.2, v0.2)
    StmtPtr parse_trace();    // traza expr (spec §21.3, v0.2 [OPC])

    // -- expressions (precedence tower, low to high) ------------------------
    ExprPtr parse_logic_or();
    ExprPtr parse_logic_and();
    ExprPtr parse_logic_not();
    ExprPtr parse_comparison();
    ExprPtr parse_sum();
    ExprPtr parse_product();
    ExprPtr parse_unary();
    ExprPtr parse_postfix();
    ExprPtr parse_primary();
    ExprPtr parse_array_literal();
    ExprPtr parse_set_literal();   // {e1, e2, ...} or ∅ (spec §20.1, v0.2)
    ExprPtr finish_call(const std::string& name, SourceLocation location);
};

}  // namespace nemi

#endif  // NEMI_PARSER_HPP
