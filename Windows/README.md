# Nemi — C++ port

A C++17 implementation of the Nemi interpreter, mirroring the Python
reference package ([`../python/nemi`](../python/nemi)) documented in
[`../docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md), one C++ file per Python
module. The lexer, parser, evaluator, a from-scratch bignum/rational numeric
tower, and a Spanish-flag CLI matching `python -m nemi` are all implemented
and pass the full acceptance corpus.

**Progress tracking:** the phased roadmap and checklist for the v0.1 core
live in [`backlog.md`](backlog.md) — all 6 phases are done. What remains are
two known, documented gaps (Windows `argv` UTF-8 decoding, byte- vs
code-point-based string indexing) — see `backlog.md` "Riesgos / notas".

On top of that core, the v0.2 **common library** (`Nemi.md` §19–§23 —
`Conjunto`, `para cada`, dynamic lists, `afirma`/`traza`, string primitives,
and the 6 real modules in [`../bibcom`](../bibcom)) is also complete; see
[`../backlog_v0.2.md`](../backlog_v0.2.md) at the repo root for that phased
history. `tests/corpus_test.cpp` now covers both: **82/82** (16 from the
v0.1 §17 corpus + everything v0.2 added).

Verified with **three real compilers**: clang++ and MSVC (Visual Studio 2022,
`cl.exe` 19.42) locally — both give the identical result — plus g++ in CI
(`.github/workflows/ci.yml`). `check_all.ps1` at the repo root runs the
Python + C++ suites locally in one shot.

## Build & test

```console
$ cmake -S . -B build -G Ninja        # or your favourite generator
$ cmake --build build
$ ctest --test-dir build --output-on-failure
```

The acceptance test (`tests/corpus_test.cpp`) loads the **shared** corpus in
[`../examples`](../examples) and the **shared** common library in
[`../bibcom`](../bibcom) — the same files the Python suite uses — and
asserts the same §17/§22 results, plus a bignum stress case
(`factorial(100)`, 158 digits). **82/82 pass.**

```console
$ ./build/nemi ../examples/factorial.nemi --llama "factorial(100)"
93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000

$ ./build/nemi ../examples/guion_factorial.nemi   # runs the script body (imprime output)
$ ./build/nemi ../examples/factorial.nemi --asa   # pretty-prints the AST as readable Nemi
$ ./build/nemi ../bibcom/bibcom.nemi --llama "C(52, 5)"   # the v0.2 common library
2598960
```

`-I DIR` (repeatable) adds a fallback directory searched for `incluye`
targets not found next to the file that includes them — e.g. pointing at an
installed `bibcom/` without copying it or writing a relative path back to
the install directory (see [`installer/README.md`](installer/README.md)).

## WinNemi (editor + consola)

`WinNemi.exe` es un editor de texto tipo Bloc de notas para archivos `.nemi`
(modernizado desde el PopPad de Petzold) con una consola interactiva
integrada (`CmdLineCtl`), compilado por el mismo `CMakeLists.txt` cuando
`WIN32` está definido. Para empaquetarlo en un instalador standalone (sin
dependencia del Visual C++ Redistributable), ver
[`installer/README.md`](installer/README.md).

`WinNemi.exe` resuelve `incluye` igual que el CLI, más dos cosas propias de
la GUI (sin `-I`, que es solo de línea de comandos): detecta sola una
carpeta `bibcom/` junto a su propio `.exe` (instalada o portátil, sin
configurar nada), y el menú **Ejecutar → Rutas de inclusión...** abre un
diálogo para agregar o quitar carpetas propias (persisten en `WinNemi.ini`,
junto al `.exe`) — útil para que un estudiante tenga su propia biblioteca de
funciones reutilizables.

La consola solo acepta líneas después de correr el archivo al menos una vez
(**Ejecutar → Ejecutar archivo**, F5) — es lo que carga el intérprete con
las funciones/procedimientos del buffer. **Ejecutar → Reiniciar sesión**
(Ctrl+Mayús+F5) limpia solo las variables sueltas que hayas escrito en la
consola, sin recargar el archivo ni perder esas funciones — más ligero que
volver a pulsar F5 cuando solo quieres una consola limpia.

El menú **Ayuda** abre el [manual](../manual/00_indice.md) (`manual/*.md`,
`Nemi.md` y `bibcom/README.md` convertidos a HTML con `pandoc` en
[`../docs/`](../docs/) — ver [`devtools/README.md`](devtools/README.md)) en
el navegador predeterminado, buscando `manual\00_indice.html` junto al
propio `.exe`. El instalador lo deja ahí; en un build de desarrollo sin
instalar hay que copiar `docs\manual\`, `docs\Nemi.html` y
`docs\bibcom\README.html` junto a `WinNemi.exe` a mano para probarlo (igual
que ya pasa con `bibcom/`) — sin esa carpeta, **Ayuda** muestra un aviso en
vez de fallar.

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
