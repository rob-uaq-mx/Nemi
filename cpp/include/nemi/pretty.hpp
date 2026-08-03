// Renders an expression back to (roughly) the Nemi syntax it came from.
// Used by `afirma`'s failure message (spec §21.2, v0.2 -- interpreter.cpp
// needs this at *runtime*, not just for diagnostics) and by the CLI's
// `--asa` pretty-printer (apps/main.cpp). Mirrors nemi/interpreter.py's
// `_expr_to_source`.
//
// Reconstructing from the AST (rather than recording raw source text spans
// in the lexer/parser) avoids plumbing byte offsets through every token in
// all three implementations, and the result is always exactly consistent
// with how the expression was actually parsed.
#ifndef NEMI_PRETTY_HPP
#define NEMI_PRETTY_HPP

#include <ostream>
#include <string>

#include "nemi/ast.hpp"

namespace nemi {

// Writes directly to a stream (what the recursive statement pretty-printer
// in apps/main.cpp wants, to avoid building a throwaway string per
// sub-expression).
void print_expr_source(std::ostream& os, const Expr& expr);

// Convenience wrapper for a single expression (what afirma's error message
// wants).
std::string expr_to_source(const Expr& expr);

}  // namespace nemi

#endif  // NEMI_PRETTY_HPP
