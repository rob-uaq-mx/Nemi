# Nemi — C++ port

A C++17 implementation of the Nemi interpreter, mirroring the Python
reference package ([`../python/nemi`](../python/nemi)) documented in
[`../docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md), one C++ file per Python
module. The lexer, parser, evaluator, a from-scratch bignum/rational numeric
tower, and a Spanish-flag CLI matching `python -m nemi` are all implemented
and pass the full acceptance corpus.

**Progress tracking:** the phased roadmap and checklist live in
[`backlog.md`](backlog.md) — all 6 phases are done. What remains are two
known, documented gaps (Windows `argv` UTF-8 decoding, byte- vs
code-point-based string indexing) — see `backlog.md` "Riesgos / notas".

Verified with **three real compilers**: clang++ and MSVC (Visual Studio 2022,
`cl.exe` 19.42) locally — both give the identical 16/16 result — plus g++ in
CI (`.github/workflows/ci.yml`; untested locally, only an ancient MinGW g++
6.3 was available on the dev machine). This repo is not a git repository yet,
so the CI workflow hasn't actually run — `check_all.ps1` at the repo root runs
the same two checks locally, no git required.

## Build & test

```console
$ cmake -S . -B build -G Ninja        # or your favourite generator
$ cmake --build build
$ ctest --test-dir build --output-on-failure
```

The acceptance test (`tests/corpus_test.cpp`) loads the **shared** corpus in
[`../examples`](../examples) — the same files the Python suite uses — and
asserts the same §17 results, plus a bignum stress case (`factorial(100)`,
158 digits). **16/16 pass.**

```console
$ ./build/nemi ../examples/factorial.nemi --llama "factorial(100)"
93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000

$ ./build/nemi ../examples/guion_factorial.nemi   # runs the script body (imprime output)
$ ./build/nemi ../examples/factorial.nemi --asa   # pretty-prints the AST as readable Nemi
```

## Layout

```
cpp/
├── CMakeLists.txt          libnemi (static) + nemi CLI + CTest
├── include/nemi/           public headers (one class per header)
│   ├── source_location.hpp errors.hpp
│   ├── bigint.hpp numeric.hpp    (BigInt + Rational, no external deps)
│   ├── token_kind.hpp token.hpp
│   ├── lexer.hpp ast.hpp parser.hpp
│   ├── value.hpp array.hpp environment.hpp interpreter.hpp
│   └── nemi.hpp            facade: load / load_files / run_call
├── src/                    implementations
├── apps/main.cpp           the `nemi` CLI (== python -m nemi: --llama/--lexemas/--asa/--ayuda)
└── tests/corpus_test.cpp   §17 acceptance suite + bignum stress case, over ../examples
```

## Status of each translation unit

| File | State |
|---|---|
| `errors.hpp`, `source_location.hpp` | complete |
| `token_kind.*`, `token.hpp` | complete |
| `bigint.hpp`/`.cpp`, `numeric.hpp`/`rational.cpp` | complete — from-scratch bignum, no GMP/Boost |
| `lexer.hpp`/`.cpp` | complete |
| `ast.hpp`, `parser.hpp`/`.cpp` | complete |
| `value.hpp`/`.cpp`, `array.hpp`, `environment.hpp` | complete |
| `interpreter.hpp`/`.cpp` | complete: all 10 `visit(...)` + call machinery + `run_program()` |
| `nemi.cpp` (facade) | complete |
| `apps/main.cpp` (CLI) | complete: Spanish flags, script-body execution, AST pretty-printer |

## What's left: two known gaps (everything else is done)

1. **Known Windows-specific gap:** UTF-8 identifiers passed as `--llama`
   command-line arguments can get mangled by Windows' ANSI-codepage `argv`
   decoding (`wmain` + `GetCommandLineW` would fix it) — reading `.nemi`
   *files* is unaffected (real UTF-8 there). See `backlog.md` Fase 5.
2. **Known semantic gap:** string indexing (`s[i]`) is byte-based
   (`std::string` holds raw UTF-8), while Python's `str[i]` is code-point
   based — differs only for non-ASCII string contents, which the acceptance
   corpus never exercises.
3. Unit tests for the lexer and parser in isolation were deliberately skipped
   (`corpus_test.cpp` already exercises both end to end, and each stage was
   hand-verified extensively while building it — see `backlog.md` Fase 6).

See [`PORTING_GUIDE.md`](PORTING_GUIDE.md) for the step-by-step walkthrough of
how this was built, including real bugs hit along the way (an
integer-overflow-driven infinite loop in the first `isqrt` implementation, a
`\xNN` hex-escape gotcha in the CLI help text) — useful reading even now that
the port is complete, as a record of what to watch for in a similar project.
