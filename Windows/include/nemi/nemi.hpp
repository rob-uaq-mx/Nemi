// Public facade for embedding the interpreter. Mirrors the helpers exported
// from nemi/__init__.py (load / load_files / run_call).
#ifndef NEMI_NEMI_HPP
#define NEMI_NEMI_HPP

#include <string>
#include <vector>

#include "nemi/interpreter.hpp"
#include "nemi/value.hpp"

namespace nemi {

// Parse source text into a ready-to-use interpreter. `source_path`, if given,
// tags error locations with a file name and is the base for resolving any
// relative `incluye "ruta"` directive in `source` (otherwise: the current
// working directory).
Interpreter load(const std::string& source, const std::string& source_path = "");

// Parse and merge several .nemi files into one interpreter. Each file's
// `incluye "ruta"` directives resolve relative to its own directory.
Interpreter load_files(const std::vector<std::string>& paths);

// Evaluate a single call/expression (e.g. "máximo([3,9,4],3)") against the
// loaded definitions and return the resulting value.
Value run_call(Interpreter& interp, const std::string& call_text);

}  // namespace nemi

#endif  // NEMI_NEMI_HPP
