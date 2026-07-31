// CLI front end — equivalent of `python -m nemi`. Options are Spanish, since
// Nemi itself is a Spanish-keyword language (mirrors python/nemi/cli.py):
//
//   nemi ARCHIVO.nemi [ARCHIVO2.nemi ...] [--llama "expr" ...]
//   nemi ARCHIVO.nemi --lexemas   # dump the token stream
//   nemi ARCHIVO.nemi --asa       # dump the parsed definitions (AST)
//
// Loading a file runs its top-level statements (the "script body") in source
// order; a bare top-level call is evaluated for effect but not printed, so
// use `imprime(...)` to produce output. Each `--llama` expression is then
// evaluated against the loaded definitions and its result printed.
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "nemi/ast.hpp"
#include "nemi/loader.hpp"
#include "nemi/nemi.hpp"
#include "nemi/parser.hpp"

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace {

void force_utf8() {
#if defined(_WIN32)
    // Windows consoles default to the OEM/ANSI code page; Nemi I/O is UTF-8.
    // NOTE: this covers file reads and console *output* only. UTF-8
    // identifiers passed as command-line arguments (e.g. --llama "máximo(…)")
    // can still arrive mangled on Windows, since argv itself is decoded with
    // the ANSI code page by the C runtime before main() ever runs — see
    // backlog.md Fase 5 / PORTING_GUIDE.md for the full explanation and fix
    // (wmain + GetCommandLineW).
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

int usage() {
    std::cerr << "uso: nemi ARCHIVO.nemi [...] [--llama \"expr\"] [--lexemas] [--asa]\n";
    return 2;
}

void print_help() {
    std::cout <<
        "uso: nemi [-a] [--llama EXPRESI\xC3\x93N] [--lexemas] [--asa]\n"
        "          ARCHIVO.nemi [ARCHIVO.nemi ...]\n\n"
        "Intérprete de Nemi\n\n"
        "positional arguments:\n"
        "  ARCHIVO.nemi       uno o más archivos fuente .nemi\n\n"
        "options:\n"
        "  -a, --ayuda        muestra esta ayuda y termina\n"
        "  --llama EXPRESI\xC3\x93N  eval\xC3\xBA" "a una expresi\xC3\xB3n de llamada contra las\n"
        "                     definiciones cargadas\n"
        "  --lexemas          imprime los lexemas (tokens) y termina\n"
        "  --asa              imprime el \xC3\xA1rbol sint\xC3\xA1" "ctico abstracto (ASA) y termina\n";
}

// ---- --asa: a small, readable pretty-printer for the parsed AST -----------
// (Diagnostic-only; not exercised by the acceptance corpus. Mirrors the
// Python CLI's --asa in scope: prints only `definitions`, not the top-level
// `main` script body — see PORTING_GUIDE.md if you want to extend it.)

// Renders an operator as its Nemi symbol (e.g. Eq -> "=") rather than the
// bare enumerator name, so the dump reads like Nemi source, not C++ internals.
const char* op_symbol(nemi::TokenKind op) {
    using nemi::TokenKind;
    switch (op) {
        case TokenKind::Eq: return "=";
        case TokenKind::Ne: return "\xE2\x89\xA0";       // ≠
        case TokenKind::Lt: return "<";
        case TokenKind::Le: return "\xE2\x89\xA4";       // ≤
        case TokenKind::Gt: return ">";
        case TokenKind::Ge: return "\xE2\x89\xA5";       // ≥
        case TokenKind::Plus: return "+";
        case TokenKind::Minus: return "\xE2\x88\x92";    // −
        case TokenKind::Times: return "\xC2\xB7";        // ·
        case TokenKind::Divide: return "/";
        case TokenKind::Mod: return "mod";
        case TokenKind::And: return "\xE2\x88\xA7";      // ∧
        case TokenKind::Or: return "\xE2\x88\xA8";       // ∨
        case TokenKind::Not: return "\xC2\xAC";          // ¬
        case TokenKind::Sqrt: return "\xE2\x88\x9A";     // √
        default: return nemi::to_string(op);
    }
}

void print_expr(std::ostream& os, nemi::Expr& e);

void print_expr(std::ostream& os, nemi::Expr& e) {
    using namespace nemi;
    if (auto* n = dynamic_cast<IntLiteral*>(&e)) { os << n->value.to_decimal(); return; }
    if (auto* n = dynamic_cast<RealLiteral*>(&e)) { os << n->value; return; }
    if (auto* n = dynamic_cast<StringLiteral*>(&e)) { os << '"' << n->value << '"'; return; }
    if (auto* n = dynamic_cast<Variable*>(&e)) { os << n->name; return; }
    if (auto* n = dynamic_cast<ArrayLiteral*>(&e)) {
        os << "[";
        for (std::size_t i = 0; i < n->elements.size(); ++i) {
            if (i) os << ", ";
            print_expr(os, *n->elements[i]);
        }
        os << "]";
        return;
    }
    if (auto* n = dynamic_cast<Index*>(&e)) {
        print_expr(os, *n->base);
        os << "[";
        print_expr(os, *n->index);
        os << "]";
        return;
    }
    if (auto* n = dynamic_cast<Call*>(&e)) {
        os << n->name << "(";
        for (std::size_t i = 0; i < n->args.size(); ++i) {
            if (i) os << ", ";
            print_expr(os, *n->args[i]);
        }
        os << ")";
        return;
    }
    if (auto* n = dynamic_cast<Unary*>(&e)) {
        if (n->op == TokenKind::LFloor) {
            os << "\xE2\x8C\x8A";  // ⌊
            print_expr(os, *n->operand);
            os << "\xE2\x8C\x8B";  // ⌋
            return;
        }
        if (n->op == TokenKind::LCeil) {
            os << "\xE2\x8C\x88";  // ⌈
            print_expr(os, *n->operand);
            os << "\xE2\x8C\x89";  // ⌉
            return;
        }
        os << "(" << op_symbol(n->op) << " ";
        print_expr(os, *n->operand);
        os << ")";
        return;
    }
    if (auto* n = dynamic_cast<Binary*>(&e)) {
        os << "(";
        print_expr(os, *n->left);
        os << " " << op_symbol(n->op) << " ";
        print_expr(os, *n->right);
        os << ")";
        return;
    }
    os << "?expr?";
}

void print_indent(std::ostream& os, int indent) {
    for (int i = 0; i < indent; ++i) os << "    ";
}

void print_stmt(std::ostream& os, nemi::Stmt& s, int indent);

void print_block(std::ostream& os, const nemi::Block& block, int indent) {
    for (const auto& s : block) print_stmt(os, *s, indent);
}

void print_stmt(std::ostream& os, nemi::Stmt& s, int indent) {
    using namespace nemi;
    print_indent(os, indent);
    if (auto* n = dynamic_cast<Assign*>(&s)) {
        os << n->name;
        for (auto& idx : n->indices) { os << "["; print_expr(os, *idx); os << "]"; }
        os << " <- ";
        print_expr(os, *n->value);
        os << "\n";
        return;
    }
    if (auto* n = dynamic_cast<ForLoop*>(&s)) {
        os << "para " << n->var << " <- ";
        print_expr(os, *n->start);
        os << " hasta ";
        print_expr(os, *n->end);
        os << "\n";
        print_block(os, n->body, indent + 1);
        print_indent(os, indent);
        os << "fin para\n";
        return;
    }
    if (auto* n = dynamic_cast<WhileLoop*>(&s)) {
        os << "mientras ";
        print_expr(os, *n->condition);
        os << "\n";
        print_block(os, n->body, indent + 1);
        print_indent(os, indent);
        os << "fin mientras\n";
        return;
    }
    if (auto* n = dynamic_cast<If*>(&s)) {
        os << "si ";
        print_expr(os, *n->condition);
        os << "\n";
        print_block(os, n->then_body, indent + 1);
        if (n->has_else) {
            print_indent(os, indent);
            os << "alt\n";
            print_block(os, n->else_body, indent + 1);
        }
        print_indent(os, indent);
        os << "fin si\n";
        return;
    }
    if (auto* n = dynamic_cast<Return*>(&s)) {
        os << "regresa";
        if (n->value) {
            os << " ";
            print_expr(os, *n->value);
        }
        os << "\n";
        return;
    }
    if (auto* n = dynamic_cast<ExprStatement*>(&s)) {
        print_expr(os, *n->call);
        os << "\n";
        return;
    }
    if (auto* n = dynamic_cast<Prose*>(&s)) {
        os << "\xC2\xAB " << n->text << " \xC2\xBB\n";  // « … »
        return;
    }
    os << "?stmt?\n";
}

void print_definition(std::ostream& os, nemi::Definition& def) {
    os << (def.is_function ? "funcion " : "procedimiento ") << def.name << "(";
    for (std::size_t i = 0; i < def.params.size(); ++i) {
        if (i) os << ", ";
        os << def.params[i];
    }
    os << ")\n";
    print_block(os, def.body, 1);
    os << (def.is_function ? "fin funcion" : "fin procedimiento") << "\n\n";
}

}  // namespace

int main(int argc, char** argv) {
    force_utf8();

    std::vector<std::string> files;
    std::vector<std::string> calls;
    bool dump_lexemes = false;
    bool dump_asa = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--llama") {
            if (++i >= argc) return usage();
            calls.emplace_back(argv[i]);
        } else if (arg == "--lexemas") {
            dump_lexemes = true;
        } else if (arg == "--asa") {
            dump_asa = true;
        } else if (arg == "-a" || arg == "--ayuda") {
            print_help();
            return 0;
        } else {
            files.emplace_back(arg);
        }
    }

    if (files.empty()) return usage();

    try {
        if (dump_lexemes) {
            for (const auto& path : files) {
                for (const auto& tok : nemi::tokenize_file_with_includes(path)) {
                    std::cout << nemi::to_string(tok.kind) << '\n';
                }
            }
            return 0;
        }

        if (dump_asa) {
            for (const auto& path : files) {
                nemi::Parser parser(nemi::tokenize_file_with_includes(path));
                nemi::Program program = parser.parse_program();
                for (auto& def : program.definitions) print_definition(std::cout, *def);
            }
            return 0;
        }

        nemi::Interpreter interp = nemi::load_files(files);
        interp.run_program();  // execute any top-level statements (the script body)
        for (const auto& call : calls) {
            nemi::Value result = nemi::run_call(interp, call);
            std::cout << nemi::format_value(result) << '\n';
        }
        return 0;
    } catch (const std::exception& err) {
        std::cerr << "nemi: " << err.what() << '\n';
        return 1;
    }
}
