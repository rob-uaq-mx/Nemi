// Interpreter: walks the AST and evaluates it (spec §12–§16). Mirrors
// nemi/interpreter.py. Implements ExprVisitor/StmtVisitor; because visitors
// return void, an evaluated expression's value is stashed in `result_` and
// picked up by eval().
#ifndef NEMI_INTERPRETER_HPP
#define NEMI_INTERPRETER_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "nemi/ast.hpp"
#include "nemi/environment.hpp"
#include "nemi/value.hpp"

namespace nemi {

class Interpreter : private ExprVisitor, private StmtVisitor {
public:
    explicit Interpreter(Program program);

    bool has_function(const std::string& name) const;

    // Execute the top-level statements (the "script body") in a global frame.
    // Definitions are hoisted, so top-level code may call any function in the
    // loaded source regardless of order. Statement values are discarded — a
    // bare call like `factorial(100)` is evaluated for effect and NOT printed
    // (script semantics, à la Python); use `imprime(...)` for output. A stray
    // top-level `regresa` simply ends the script. Mirrors
    // nemi/interpreter.py's `run_program`.
    void run_program();

    // Host entry point: call a Nemi function/procedure with runtime values.
    // Arrays are shared, so a procedure mutating a parameter mutates the
    // caller's Array (spec §12).
    Value call(const std::string& name, std::vector<Value> args);

    // Evaluate a standalone expression against a fresh frame (used by run_call).
    Value evaluate(Expr& expr);

private:
    // Owned AST and the name -> definition index built over it.
    Program program_;
    std::unordered_map<std::string, Definition*> functions_;

    // Evaluation state.
    Environment* env_ = nullptr;   // current call frame
    Value result_;                 // register for the last evaluated expression
    int depth_ = 0;                // call depth (recursion guard, spec §16)
    int trace_depth_ = 0;          // traza (v0.2 §21.3 [OPC]) activation
                                    // counter, not a bool: a nested traza
                                    // inside an already-traced call must not
                                    // turn tracing off when it finishes

    // Core helpers.
    Value invoke(Definition& def, std::vector<Value>& args);
    Value eval(Expr& expr);        // expr.accept(*this); return result_
    void exec(Stmt& stmt);
    void exec_block(const Block& block);

    // l-value helpers for `intercambia` (needs env_/eval(), so these stay
    // methods; most other value-level helpers are free functions in
    // interpreter.cpp since they don't need interpreter state).
    Value get_lvalue(Expr& node);
    void set_lvalue(Expr& node, Value value);
    Value swap_call(const std::vector<ExprPtr>& args);

    // ExprVisitor overrides.
    void visit(IntLiteral&) override;
    void visit(RealLiteral&) override;
    void visit(StringLiteral&) override;
    void visit(ArrayLiteral&) override;
    void visit(SetLiteral&) override;
    void visit(Variable&) override;
    void visit(Index&) override;
    void visit(Call&) override;
    void visit(Unary&) override;
    void visit(Binary&) override;

    // StmtVisitor overrides.
    void visit(Assign&) override;
    void visit(ForLoop&) override;
    void visit(ForEach&) override;
    void visit(WhileLoop&) override;
    void visit(If&) override;
    void visit(Return&) override;
    void visit(ExprStatement&) override;
    void visit(Prose&) override;
    void visit(Assert&) override;
    void visit(Trace&) override;
};

}  // namespace nemi

#endif  // NEMI_INTERPRETER_HPP
