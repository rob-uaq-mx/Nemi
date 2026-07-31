// Abstract syntax tree (spec §10). Mirrors nemi/ast_nodes.py.
//
// Two class hierarchies, Expr and Stmt, each visited by a corresponding
// Visitor. Visitors return void; the interpreter stashes an expression's value
// in an internal register (see interpreter.hpp). This keeps the AST decoupled
// from the runtime Value type. Children are owned via unique_ptr — the tree is
// strict ownership, no sharing.
#ifndef NEMI_AST_HPP
#define NEMI_AST_HPP

#include <memory>
#include <string>
#include <vector>

#include "nemi/numeric.hpp"
#include "nemi/source_location.hpp"
#include "nemi/token_kind.hpp"

namespace nemi {

// ---- forward declarations -------------------------------------------------
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using Block = std::vector<StmtPtr>;

struct IntLiteral;
struct RealLiteral;
struct StringLiteral;
struct ArrayLiteral;
struct Variable;
struct Index;
struct Call;
struct Unary;
struct Binary;

struct Assign;
struct ForLoop;
struct WhileLoop;
struct If;
struct Return;
struct ExprStatement;
struct Prose;

// ---- visitor interfaces ---------------------------------------------------
struct ExprVisitor {
    virtual ~ExprVisitor() = default;
    virtual void visit(IntLiteral&) = 0;
    virtual void visit(RealLiteral&) = 0;
    virtual void visit(StringLiteral&) = 0;
    virtual void visit(ArrayLiteral&) = 0;
    virtual void visit(Variable&) = 0;
    virtual void visit(Index&) = 0;
    virtual void visit(Call&) = 0;
    virtual void visit(Unary&) = 0;
    virtual void visit(Binary&) = 0;
};

struct StmtVisitor {
    virtual ~StmtVisitor() = default;
    virtual void visit(Assign&) = 0;
    virtual void visit(ForLoop&) = 0;
    virtual void visit(WhileLoop&) = 0;
    virtual void visit(If&) = 0;
    virtual void visit(Return&) = 0;
    virtual void visit(ExprStatement&) = 0;
    virtual void visit(Prose&) = 0;
};

// ---- bases ----------------------------------------------------------------
struct Expr {
    SourceLocation location;
    virtual ~Expr() = default;
    virtual void accept(ExprVisitor& v) = 0;
};

struct Stmt {
    SourceLocation location;
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) = 0;
};

// Boilerplate for the accept override.
#define NEMI_EXPR_ACCEPT \
    void accept(ExprVisitor& v) override { v.visit(*this); }
#define NEMI_STMT_ACCEPT \
    void accept(StmtVisitor& v) override { v.visit(*this); }

// ---- expression nodes -----------------------------------------------------
struct IntLiteral : Expr {
    Integer value;
    explicit IntLiteral(Integer v) : value(v) {}
    NEMI_EXPR_ACCEPT
};

struct RealLiteral : Expr {
    Real value;
    explicit RealLiteral(Real v) : value(v) {}
    NEMI_EXPR_ACCEPT
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
    NEMI_EXPR_ACCEPT
};

struct ArrayLiteral : Expr {
    std::vector<ExprPtr> elements;
    NEMI_EXPR_ACCEPT
};

struct Variable : Expr {
    std::string name;
    explicit Variable(std::string n) : name(std::move(n)) {}
    NEMI_EXPR_ACCEPT
};

struct Index : Expr {         // base[index], base-1
    ExprPtr base;
    ExprPtr index;
    NEMI_EXPR_ACCEPT
};

struct Call : Expr {          // name(arg, ...)
    std::string name;
    std::vector<ExprPtr> args;
    NEMI_EXPR_ACCEPT
};

struct Unary : Expr {         // op operand; op in {Minus, Not, Sqrt, LFloor, LCeil}
    TokenKind op;
    ExprPtr operand;
    NEMI_EXPR_ACCEPT
};

struct Binary : Expr {        // left op right
    TokenKind op;
    ExprPtr left;
    ExprPtr right;
    NEMI_EXPR_ACCEPT
};

// ---- statement nodes ------------------------------------------------------
struct Assign : Stmt {        // place ← value; indices empty for a bare variable
    std::string name;
    std::vector<ExprPtr> indices;
    ExprPtr value;
    NEMI_STMT_ACCEPT
};

struct ForLoop : Stmt {       // para var ← start hasta end ... (inclusive, +1)
    std::string var;
    ExprPtr start;
    ExprPtr end;
    Block body;
    NEMI_STMT_ACCEPT
};

struct WhileLoop : Stmt {
    ExprPtr condition;
    Block body;
    NEMI_STMT_ACCEPT
};

struct If : Stmt {            // else_body empty when absent
    ExprPtr condition;
    Block then_body;
    Block else_body;
    bool has_else = false;
    NEMI_STMT_ACCEPT
};

struct Return : Stmt {        // value null for a bare `regresa`
    ExprPtr value;            // may be nullptr
    NEMI_STMT_ACCEPT
};

struct ExprStatement : Stmt { // a call used for effect, e.g. intercambia(a,b)
    ExprPtr call;             // always a Call
    NEMI_STMT_ACCEPT
};

struct Prose : Stmt {         // « ... » non-executable (spec §14)
    std::string text;
    NEMI_STMT_ACCEPT
};

#undef NEMI_EXPR_ACCEPT
#undef NEMI_STMT_ACCEPT

// ---- top level ------------------------------------------------------------
struct Definition {
    std::string name;
    std::vector<std::string> params;
    Block body;
    bool is_function = true;   // función vs. procedimiento
    SourceLocation location;
};
using DefinitionPtr = std::unique_ptr<Definition>;

struct Program {
    std::vector<DefinitionPtr> definitions;
    // Top-level statements (the "script body"), executed in source order
    // after all definitions are registered — spec §10, Nemi.md §12. Wired up
    // by the parser (Fase 2); Interpreter::run_program() (Fase 5) executes it.
    Block main;
};

}  // namespace nemi

#endif  // NEMI_AST_HPP
