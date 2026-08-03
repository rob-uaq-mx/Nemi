#include "nemi/loader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "nemi/errors.hpp"
#include "nemi/lexer.hpp"

namespace nemi {
namespace {

namespace fs = std::filesystem;

std::string read_file_utf8(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw LexicalError("no se pudo abrir el archivo: " + path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Used only to detect cycles (two different-looking relative paths that name
// the same file), so an inexact-but-safe fallback is fine when the file
// can't be resolved for some reason (weakly_canonical doesn't require the
// file to exist, but can still fail on some malformed paths).
std::string canonical_key(const fs::path& path) {
    std::error_code ec;
    fs::path result = fs::weakly_canonical(path, ec);
    if (ec) return fs::absolute(path).lexically_normal().string();
    return result.string();
}

// Finds the file an `incluye "rel"` should splice in. Tries `base_dir`
// first (today's only behavior, always wins if the file is there); then
// each directory in `search_paths`, in order (the `-I` CLI flag). Falls
// through to the `base_dir` candidate if nothing exists, so the caller's
// usual ifstream-fails error path fires unchanged.
fs::path resolve_include(const fs::path& base_dir, const std::string& rel,
                          const std::vector<std::string>& search_paths) {
    fs::path candidate = base_dir / rel;
    std::error_code ec;
    if (fs::is_regular_file(candidate, ec)) return candidate;
    for (const auto& dir : search_paths) {
        fs::path alt = fs::path(dir) / rel;
        if (fs::is_regular_file(alt, ec)) return alt;
    }
    return candidate;
}

// Tokenizes `source` (tagged as `file_tag`) and replaces every
// `incluye "ruta"` pair with the (recursively expanded) tokens of the file it
// names, resolved relative to `base_dir` (falling back to `search_paths`, in
// order). `active` holds the canonical path of every file currently being
// expanded, an ancestor chain used to reject cycles (A includes B includes A).
std::vector<Token> expand(const std::string& source, const fs::path& base_dir,
                           const std::string& file_tag,
                           std::vector<std::string>& active,
                           const std::vector<std::string>& search_paths) {
    std::vector<Token> tokens = Lexer(source, file_tag).tokenize();

    std::vector<Token> result;
    result.reserve(tokens.size());

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].kind != TokenKind::Include) {
            result.push_back(std::move(tokens[i]));
            continue;
        }

        SourceLocation at = tokens[i].location;
        if (i + 1 >= tokens.size() || tokens[i + 1].kind != TokenKind::String) {
            throw LexicalError(
                "se esperaba una cadena con la ruta despu\xC3\xA9s de 'incluye'", at);
        }
        std::string rel = tokens[i + 1].string_value();
        ++i;  // also consume the string token

        fs::path resolved = resolve_include(base_dir, rel, search_paths);
        std::string key = canonical_key(resolved);
        for (const auto& seen : active) {
            if (seen == key) {
                throw LexicalError(
                    "inclusi\xC3\xB3n c\xC3\xAD" "clica: '" + rel + "' ya se est\xC3\xA1 incluyendo",
                    at);
            }
        }

        std::string included_source;
        {
            std::ifstream in(resolved, std::ios::binary);
            if (!in) {
                throw LexicalError(
                    "no se pudo abrir el archivo incluido: " + rel, at);
            }
            std::ostringstream buffer;
            buffer << in.rdbuf();
            included_source = buffer.str();
        }

        active.push_back(key);
        std::vector<Token> included =
            expand(included_source, resolved.parent_path(), resolved.string(), active,
                   search_paths);
        active.pop_back();

        // Drop the included file's own trailing Eof: it is spliced into the
        // middle of another stream, not the end of the whole program.
        if (!included.empty() && included.back().kind == TokenKind::Eof)
            included.pop_back();
        for (auto& t : included) result.push_back(std::move(t));
    }
    return result;
}

}  // namespace

std::vector<Token> tokenize_with_includes(const std::string& source,
                                           const std::string& base_path,
                                           const std::vector<std::string>& search_paths) {
    std::vector<std::string> active;
    fs::path base_dir = base_path.empty() ? fs::current_path()
                                           : fs::path(base_path).parent_path();
    if (base_dir.empty()) base_dir = fs::current_path();
    if (!base_path.empty()) active.push_back(canonical_key(base_path));
    return expand(source, base_dir, base_path, active, search_paths);
}

std::vector<Token> tokenize_file_with_includes(
    const std::string& path, const std::vector<std::string>& search_paths) {
    std::string source = read_file_utf8(path);
    std::vector<std::string> active{canonical_key(path)};
    fs::path base_dir = fs::path(path).parent_path();
    if (base_dir.empty()) base_dir = fs::current_path();
    return expand(source, base_dir, path, active, search_paths);
}

}  // namespace nemi
