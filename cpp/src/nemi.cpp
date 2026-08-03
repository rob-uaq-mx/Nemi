// Facade implementation: parse, merge, and evaluate. This layer is real; it
// only depends on Lexer/Parser/Interpreter (whose bodies are being ported).
#include "nemi/nemi.hpp"

#include "nemi/lexer.hpp"
#include "nemi/loader.hpp"
#include "nemi/parser.hpp"

namespace nemi {

Interpreter load(const std::string& source, const std::string& source_path,
                  const std::vector<std::string>& search_paths) {
    Parser parser(tokenize_with_includes(source, source_path, search_paths));
    return Interpreter(parser.parse_program());
}

Interpreter load_files(const std::vector<std::string>& paths,
                        const std::vector<std::string>& search_paths) {
    // Tokenize each file separately (each resolves its own `incluye`s
    // relative to its own directory, and its tokens keep their true file so
    // errors point at the right place), then concatenate the token streams
    // -- definitions from all files still share one program (hoisting).
    std::vector<Token> merged;
    if (paths.empty()) {
        merged = tokenize_with_includes("", "", search_paths);
    } else {
        for (std::size_t i = 0; i < paths.size(); ++i) {
            std::vector<Token> tokens =
                tokenize_file_with_includes(paths[i], search_paths);
            bool is_last = (i + 1 == paths.size());
            if (!is_last && !tokens.empty() && tokens.back().kind == TokenKind::Eof)
                tokens.pop_back();
            for (auto& t : tokens) merged.push_back(std::move(t));
        }
    }
    Parser parser(std::move(merged));
    return Interpreter(parser.parse_program());
}

Value run_call(Interpreter& interp, const std::string& call_text) {
    Lexer lexer(call_text);
    Parser parser(lexer.tokenize());
    ExprPtr expr = parser.parse_expression();
    return interp.evaluate(*expr);
}

}  // namespace nemi
