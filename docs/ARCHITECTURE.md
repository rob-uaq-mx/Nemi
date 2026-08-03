# Nemi interpreter — architecture & C++ portability guide

> **Scope note:** this document covers the **v0.1 core** (`Nemi.md` §1–§18) —
> lexer, parser, AST, evaluator, the value model. The **v0.2 common library**
> (`Conjunto`, `para cada`, dynamic lists, `afirma`/`traza`, string
> primitives, and the 6 real modules in [`../bibcom`](../bibcom)) was added
> on top afterwards; see [`../backlog_v0.2.md`](../backlog_v0.2.md) (Fase
> 0–8) for its own design decisions, in the same spirit as §8 below — those
> weren't folded into this file to avoid re-deriving a second time what the
> phase-by-phase backlog already records in more detail.

This document describes how the Python reference interpreter is structured and
how each piece maps onto the C++ ports (`cpp/`, `Windows/` — complete for the
core language, see `cpp/backlog.md`/`Windows/backlog.md` for status). The
implementation is deliberately a **classic three-stage tree-walking
interpreter** with no clever Python-only tricks, so the translation is
mechanical.

```
source text ──▶ Lexer ──▶ [Token] ──▶ Parser ──▶ AST ──▶ Interpreter ──▶ Value
   (UTF-8)                                                   (evaluator)
```

The single source of truth for the *language* is [`../Nemi.md`](../Nemi.md);
section references below (§9, §10, …) point into it.

---

## Repository layout

The Python reference and the two C++ ports are siblings that share the spec
(`Nemi.md`), the acceptance corpus (`examples/`), the common library
(`bibcom/`), and this document:

```
python/nemi/   the reference interpreter (the module map below)
python/tests/  the acceptance runner (run_corpus.py)
cpp/           the C++ port (see cpp/README.md)
Windows/       the C++ port + WinNemi.exe editor/console (see Windows/README.md)
examples/      the shared §17 corpus (all three suites run it)
bibcom/        the shared v0.2 common library, §22 (all three suites run it)
```

## 1. Module map (Python reference)

| Module (`python/`) | Responsibility | Spec |
|---|---|---|
| `nemi/errors.py` | Exception hierarchy + `SourceLocation` | §16 |
| `nemi/tokens.py` | `TokenKind` enum, `Token` value type | §9 |
| `nemi/lexer.py` | UTF-8 text → token list | §9 |
| `nemi/ast_nodes.py` | AST node classes (`Statement` / `Expr` trees) | §10 |
| `nemi/parser.py` | tokens → `Program` (recursive descent) | §10, §11 |
| `nemi/values.py` | `Array` + truthiness / formatting / conversions | §8, §11 |
| `nemi/interpreter.py` | AST → values (evaluator, operators, primitives) | §12–§16 |
| `nemi/cli.py`, `__main__.py` | `python -m nemi` front end | — |
| `nemi/__init__.py` | public API (`load`, `load_files`, `run_call`, …) | — |

Each stage depends only on the ones above it; there are no cycles. In C++ these
become headers/translation units with the same layering (see `cpp/`).

---

## 2. Lexer (`lexer.py`)

Single pass, one character of lookahead, no backtracking.

* **Whitespace-insensitive.** Blocks are closed explicitly with `fin …` (§8,
  §13), so newlines/indentation are pure trivia and discarded. This is the one
  decision that most simplifies both parser and port — there is no INDENT/DEDENT
  machinery.
* **Canonicalisation.** Operators have a Unicode form and, for most, an ASCII
  keyboard equivalent (§9). Both lex to the *same* `TokenKind` (`<-`≡`←`,
  `!=`≡`≠`, `*`≡`·`, `-`≡`−`, …), so no later stage sees the difference. The
  logical connectives are Spanish only: the words `y`/`o`/`no` and the symbols
  `∧`/`∨`/`¬` (there is no `and`/`or`/`not`).
* **Unicode identifiers.** A "letter" is anything `str.isalpha()` accepts, so
  `función` lexes as a word (then found in the keyword table) and `máximo`,
  `raíz`, `exp_rápida` are valid identifiers.
* **Keywords carry no payload.** Every keyword (including `alt`, the dedicated
  else keyword) lexes to a bare `TokenKind`; the lexer needs no lookahead or
  special tagging.

**C++ notes.** Read the file as UTF-8 bytes and iterate by **code point**, not
`char`. Recommended: decode to `std::u32string` (or use a tiny UTF-8 decoder)
so `√`, `≤`, `⌊` are single units and identifier scanning can call an
`is_letter(char32_t)` predicate. The keyword/operator tables become
`std::unordered_map<std::u32string, TokenKind>` and
`std::unordered_map<char32_t, TokenKind>`.

---

## 3. Tokens & AST (`tokens.py`, `ast_nodes.py`)

* `TokenKind` is an `enum.Enum` → C++ `enum class TokenKind`.
* `Token` holds `{kind, value, location}`. `value` is a small tagged payload
  (int / double / string / none) → in C++ a `std::variant` or a struct with a
  discriminated union.
* AST nodes are plain **data classes** with no behaviour, split into two trees
  rooted at `Statement` and `Expr`. Operators are stored as `TokenKind` values,
  not strings, so the evaluator switches over a closed enum.

**C++ notes.** Model the AST as a class hierarchy with a `Node` base and a
`Visitor` interface (`visit(Assign&)`, `visit(Binary&)`, …). The Python
type→handler dispatch dicts (see §5) become virtual `accept(Visitor&)` calls or
a `switch` on a node-kind enum. Use `std::unique_ptr<Expr>` for children and
`std::vector<StmtPtr>` for blocks; ownership is a strict tree, so no shared
pointers are needed for the AST itself.

---

## 4. Parser (`parser.py`)

Textbook recursive descent: one method per grammar production (§10), one token
of lookahead. Expression parsing is the precedence tower of §11, lowest to
highest:

```
or → and → not → comparison → sum → product → unary → postfix → primary
```

One point any port must reproduce: **block termination.** `_block()` reads
statements until it sees `fin`, the else keyword `alt`, or EOF. Because closers
are explicit, the parser never guesses scope from layout. (`alt` is a dedicated
keyword, so — unlike an English `else`/`if` split — there is no ambiguity with a
negated condition such as `si ¬ terminado`; the lexer needs no special tagging.)

One deliberate **extension beyond §10**: array literals `[a, b, c]` are accepted
as a primary. The grammar only used `[` for postfix indexing; a *leading* `[`
is unambiguous and lets host code / the test driver build array and matrix
inputs (and lets `--llama` pass them). Nothing else changed.

**C++ notes.** The cursor helpers (`_peek`, `_advance`, `_check`, `_match`,
`_expect`) map 1:1 to methods over a `std::vector<Token>` index. Throw
`ParseError` with the offending token's `SourceLocation`.

---

## 5. Interpreter (`interpreter.py`)

A tree walker. Dispatch is table-driven: `_stmt_dispatch` and `_expr_dispatch`
map a node type to a handler method. (In C++ this is a visitor or a `switch` on
node kind — the handlers themselves port line for line.)

### 5.1 Values and the numeric tower

See `values.py`. The runtime value is one of:

| Nemi value | Python type | C++ (`cpp/`) |
|---|---|---|
| integer (bignum) | `int` | `BigInt` — from-scratch, no external dependency (`cpp/include/nemi/bigint.hpp`) |
| exact rational | `fractions.Fraction` | `Rational` over `BigInt` (`cpp/include/nemi/numeric.hpp`) |
| real | `float` | `double` |
| boolean / bit | `bool` (0/1 `int` also act as bits) | `bool` + `BigInt` (genuinely distinct types, unlike Python) |
| string | `str` (base-1 indexing) | `std::string` (UTF-8 bytes; indexing is byte-based, not code-point-based — fine for the ASCII-heavy corpus, a known simplification) |
| array / matrix | `Array` (base-1, by reference) | `std::shared_ptr<Array>` |

The one non-obvious choice: **`/` on two integers yields a `Fraction`, not a
float.** This keeps `⌊n / 2⌋` exact for bignum `n` (RSA-scale `exp_mod` would
otherwise lose precision). Floors/ceils then recover an integer. The C++ port's
`Rational` (built on its own `BigInt`, not Boost.Multiprecision or GMP — see
`cpp/backlog.md` Fase 4 for why) reproduces this exactly, verified against the
Python reference for integers past 30 digits.

`bool` is a subclass of `int` in Python, so the helper `_is_number()` explicitly
excludes it; a C++ port that keeps them as distinct types gets this for free.

### 5.2 Operator semantics (the subtle rules)

* **`+` / `·` polysemy (§11).** If *both* operands are booleans, `+`↦OR and
  `·`↦AND; otherwise arithmetic. Implemented in `_eval_binary`. (The corpus's
  Warshall uses `∨`/`∧` explicitly, so it does not even rely on this, but the
  rule is honoured.)
* **`∧` / `∨` short-circuit (§12)** and are *kind-preserving*: `_logic_result`
  returns a 0/1 **bit** when any operand is an integer (Warshall’s matrix
  entries) and a **boolean** when operands are booleans (comparison results).
  `¬` follows the same rule.
* **`⌊√n⌋` is exact.** `_floor` special-cases a floor directly wrapping a `√`
  of a non-negative integer and uses `math.isqrt` (→ `BigInt::isqrt`, a Newton
  iteration, in C++ — see `cpp/PORTING_GUIDE.md` Fase 4 for a real
  integer-overflow bug this hit and how it was fixed). Plain `√` returns an
  exact integer for perfect squares and a `double` otherwise.
* **Errors (§16).** Division/modulo by zero, out-of-range indices (base 1),
  non-numeric operands, and undefined names all raise `ExecutionError` with a
  message; the CLI turns these into a non-zero exit.

### 5.3 Calls, scope and parameter passing (§12)

* **One flat frame per call** (`Environment`). Nemi functions do not nest or
  close over each other, so there is no scope chain — a single
  `name → value` map per invocation. → `std::unordered_map<std::string, Value>`.
* **`regresa`** unwinds via a `ReturnSignal` exception caught in `_invoke`. In
  C++ either throw a small `ReturnSignal`, or have statement execution return a
  `Completion { Normal | Return(value) }` and check it in the block loop (the
  latter avoids exceptions on the hot path).
* **Parameter passing** falls out of the value model: scalars are immutable
  (by value), `Array` is shared (by reference), so `inserción_por_orden` sorts
  its argument in place. In C++, pass scalars by value and arrays as
  `shared_ptr<Array>`.
* **`intercambia(a, b)`** is the one primitive that needs *l-values*, not
  evaluated arguments — it swaps two variables/cells in the caller's frame so
  `mcd` can swap its by-value parameters. Handled specially in `_eval_call`
  before argument evaluation.
* **Recursion depth** is bounded (`_MAX_CALL_DEPTH`) to report §16's
  "recursion without a base case" cleanly instead of crashing. A C++ port
  should keep an explicit depth counter for the same reason.

### 5.4 Prose actions (§14)

`« … »` parses to a `Prose` node and executes as a **no-op**. This is why
`enlosa` runs its base case but fails past it (its recursive calls reference
undefined `m1…m4`), exactly matching the spec's "not executable by the core".
A port could instead dispatch prose text to host primitives; the seam is the
`_exec_prose` handler.

---

## 6. Host boundary & CLI

* `values.from_python` / `to_python` convert between Nemi values and plain
  Python objects (lists ⇄ `Array`) at the API edge — the place a C++ port would
  expose its own embedding API.
* `cli.py` forces UTF-8 on stdout/stderr (Windows consoles default to cp1252),
  loads/merges files, and evaluates each `--call`. `--tokens` / `--ast` dump
  intermediate stages for study.

---

## 7. Testing

`python/tests/run_corpus.py` is a dependency-free runner asserting every §17 expected
output (all pre-verified in the course notes), plus bignum and ASCII-operator
checks, and that `enlosa` is defined but not executable past its base case
(87/87 checks as of the v0.2 common-library additions — see the scope note at
the top of this file). Keep this file as the **conformance suite** for any
reimplementation: a C++ port is correct when it reproduces the same results
(`{cpp,Windows}/tests/corpus_test.cpp`, 82/82 each).

---

## 8. Deviations & decisions recorded

Answers chosen for the open questions in §18, all reflected in code:

1. Concrete syntax favours **explicit closers** over layout (robust parsing).
2. `+`/`·` resolved **by operand type**; no separate boolean mode.
3. Geometric/prose steps are **out of the core** (no-op prose actions).
4. Added **`imprime`** (and the API/`--call`) as convenience I/O beyond
   `regresa`.
5. Indices are **base 1**, not configurable.

Small, documented extension: **array literals** `[…]` in the expression grammar
(host/tests input construction). Everything else follows `Nemi.md` as written.
