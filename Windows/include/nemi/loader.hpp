// Loader: resolves `incluye "ruta.nemi"` directives (spec §10/§12) by
// recursively splicing each included file's own tokens into the stream in
// place -- so the result reads as if the included text had been pasted right
// there, except every token still carries its *own* true origin (file+line),
// so a later lexer/parser/interpreter error points at the file that actually
// has the problem, not at some fused line count.
//
// Kept separate from Lexer on purpose: Lexer stays pure text -> tokens (no
// filesystem access, easy to unit-test), and this module owns the file I/O,
// path resolution, and recursion. Mirrors nemi/loader.py.
#ifndef NEMI_LOADER_HPP
#define NEMI_LOADER_HPP

#include <string>
#include <vector>

#include "nemi/token.hpp"

namespace nemi {

// Tokenizes `source` (tagging its tokens with `base_path`, if given) and
// splices in the tokens of any file named by an `incluye "ruta"` directive,
// recursively. Relative `incluye` paths resolve against the directory of
// `base_path`; if `base_path` is empty (an in-memory source with no file of
// its own, e.g. a `--llama` expression), they resolve against the current
// working directory. If not found there, each directory in `search_paths`
// (the `-I` CLI flag) is tried in order. Throws LexicalError for a missing
// string operand after `incluye`, a file that can't be opened, or a cyclic
// inclusion.
std::vector<Token> tokenize_with_includes(
    const std::string& source, const std::string& base_path = "",
    const std::vector<std::string>& search_paths = {});

// Reads `path` from disk (UTF-8 bytes, no BOM handling -- matches every
// .nemi file in examples/) and tokenizes it with tokenize_with_includes.
// Throws LexicalError if `path` can't be opened.
std::vector<Token> tokenize_file_with_includes(
    const std::string& path, const std::vector<std::string>& search_paths = {});

}  // namespace nemi

#endif  // NEMI_LOADER_HPP
