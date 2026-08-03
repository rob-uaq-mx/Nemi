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
// working directory). `search_paths` lists extra directories (the `-I` CLI
// flag) tried, in order, if an `incluye` target isn't found next to its own
// file.
Interpreter load(const std::string& source, const std::string& source_path = "",
                  const std::vector<std::string>& search_paths = {});

// Parse and merge several .nemi files into one interpreter. Each file's
// `incluye "ruta"` directives resolve relative to its own directory, falling
// back to `search_paths` (the `-I` CLI flag), in order, if not found there.
Interpreter load_files(const std::vector<std::string>& paths,
                        const std::vector<std::string>& search_paths = {});

// Evaluate a single call/expression (e.g. "máximo([3,9,4],3)") against the
// loaded definitions and return the resulting value.
Value run_call(Interpreter& interp, const std::string& call_text);

}  // namespace nemi

#endif  // NEMI_NEMI_HPP
