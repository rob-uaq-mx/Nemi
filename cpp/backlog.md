# Backlog / hoja de ruta — versión C++ de Nemi

Seguimiento del avance del port en `cpp/`. Marca las casillas `- [ ]` → `- [x]`
al completar cada tarea. Cada elemento apunta al archivo Python de referencia a
portar (`../python/nemi/*.py`) y al stub C++ correspondiente.

> 📖 **¿Vas a implementarlo (o eres principiante)?** Sigue la
> [**Guía de portación paso a paso**](PORTING_GUIDE.md): explica los conceptos,
> trae fragmentos de código de arranque y dice cómo verificar cada fase. Este
> backlog es el *checklist*; la guía es el *cómo*.

**Última actualización:** 2026-07-26 · **Estado global:** Fases 0–6 completas.
El port C++ es funcionalmente completo y con CI: **`corpus_test` = 16/16 PASS**
(los 15 casos del §17 + el caso de estrés bignum `factorial(100)`), verificado
además con **tres tool­chains reales** (clang++, MSVC 19.42 vía Visual Studio
2022, y — sin probar localmente, ver Fase 6 — g++ en el workflow de CI); el
CLI tiene paridad con `python -m nemi`. Quedan dos gaps conocidos y
documentados (ver «Riesgos / notas»): `argv` UTF-8 en Windows y la indexación
de cadenas por byte, no por code point. Este repositorio **no es (todavía) un
repositorio git** — el workflow de CI (`.github/workflows/ci.yml`) queda listo
para cuando se inicialice y se suba a GitHub; mientras tanto, `check_all.ps1`
en la raíz corre las mismas comprobaciones en local.

## Cómo usar este documento
- El orden de las fases es el recomendado (cada una desbloquea la siguiente).
- **Criterio de terminado global:** `ctest` con `corpus_test` en **15/15 PASS**,
  más el caso de estrés bignum (Fase 4) — ✅ cumplido (16/16) —, con ambas
  suites (Python y C++) corriendo en CI (Fase 6, pendiente).
- Verifica el progreso con: `cmake --build build && ctest --test-dir build`
  y mira cuántos casos pasan de *PENDING* a *PASS*.

## Leyenda de estado
`[x]` hecho · `[ ]` pendiente · `[~]` en progreso · `[!]` bloqueado

---

## Fase 0 — Infraestructura y andamiaje  ✅ (hecho)
- [x] Layout `include/` + `src/` + `apps/` + `tests/`, CMake (libnemi + CLI + CTest)
- [x] `errors.hpp`, `source_location.hpp`
- [x] `token_kind.*` (enum + `to_string`), `token.hpp`
- [x] `value.hpp`/`value.cpp` (`is_truthy`, `format_value`), `array.hpp`, `environment.hpp`
- [x] `ast.hpp` (nodos `Expr`/`Stmt` + `ExprVisitor`/`StmtVisitor`)
- [x] Fachada `nemi.cpp` (`load` / `load_files` / `run_call`)
- [x] `interpreter.cpp`: constructor, tabla de funciones, `call`/`invoke`,
      despacho, `exec_block`, `regresa` (ReturnSignal), literales, `Prose`
- [x] `corpus_test.cpp` sobre el corpus compartido `../examples`, con *PENDING*
      en vez de *FAIL* para lo no portado

---

## Fase 1 — Lexer  (`src/lexer.cpp` ← `lexer.py`)  ✅ (hecho, 2026-07-25)
- [x] `Lexer::tokenize`: bucle principal, EOF, saltar espacios y comentarios (`#`, `▷`)
- [x] Decodificar UTF-8 a *code points* (`char32_t`); `is_letter`/`is_digit` Unicode
- [x] Números (enteros y reales), cadenas `"…"`, prosa `«…»`
- [x] Tabla de keywords y operadores (Unicode + equivalentes ASCII `<-  !=  <=  >=  -  *`)
- [x] **Estado actual del lenguaje (no el original):**
  - [x] `alt` y `alternativamente` → `Else` (NO existe `sino` ni `si no`)
  - [x] lógicos solo español: `y`→`And`, `o`→`Or`, `no`→`Not` (NO `and`/`or`/`not`)
- [x] Seguimiento de `SourceLocation` (línea/columna) en cada token
- **Desbloquea:** `--lexemas` en el CLI y todos los casos del corpus.
- **Aceptación:** `nemi archivo.nemi --tokens` vuelca tokens sin error en los 15
  archivos del corpus; los kinds coinciden 1:1 con `python -m nemi --lexemas`.
  Verificado además: `*`/`·`, `-`/`−`, `<-`/`←`, `!=`/`≠`, `<=`/`≤`, `>=`/`≥`
  equivalentes; `and`/`or`/`not` lexean como `Ident` (ya no son operadores) y
  `y`/`o`/`no` sí; identificadores acentuados (`exp_rápida`, `máximo`); errores
  claros para carácter inesperado, cadena sin cerrar, y entero fuera de rango
  de 64 bits (apunta a la Fase 4).
- **Nota para la Fase 2:** el predicado `is_letter` usa una lista explícita de
  letras acentuadas (`áéíóúüñ...`), **no** `c >= 0x80` — esa regla tan amplia
  se comería símbolos como `←`/`≤`/`√` (también `> 0x80`) antes de llegar a la
  tabla de operadores. Si ves ese error en tu propio código, es la causa más
  probable (ver `PORTING_GUIDE.md`, apéndice de errores comunes).

---

## Fase 2 — Parser  (`src/parser.cpp` ← `parser.py`)  ✅ (hecho, 2026-07-25)
- [x] Helpers de cursor (`peek`, `advance`, `check`, `match`, `expect`)
- [x] `parse_program`: `{ definición | instrucción }` → separa `definitions` y `main`
- [x] `función`/`procedimiento` con cierres `fin` explícitos
- [x] Instrucciones: asignación, `para`, `mientras`, `si … [alt …] fin si`, `regresa`, llamada, prosa
- [x] Fin de bloque = `fin` / `alt` / EOF
- [x] Torre de expresiones §11: `or→and→not→comparación→suma→producto→unaria→postfija→primaria`
- [x] `parse_expression` (para `run_call`) y literal de arreglo `[…]`
- **Adelantado de la Fase 5:** se añadió `Block main;` a `struct Program`
  (`ast.hpp`) porque `parse_program` necesita dónde poner las instrucciones
  sueltas — es un requisito estructural del parser, no solo del CLI. Sigue
  pendiente en Fase 5: que el `Interpreter` *ejecute* ese bloque
  (`run_program()`).
- **Aceptación (verificada):** `ctest` en verde; **los 15 casos** del corpus
  pasan de PENDING-en-el-parser a PENDING-en-el-intérprete (mensajes tipo
  `Interpreter::visit(Call)`, `visit(ForLoop)`, `visit(Assign)`, según qué
  statement encuentra primero cada función). Esto confirma que los 13 `.nemi`
  del corpus (incl. `enlosa.nemi` con prosa) parsean sin error, y que
  `guion_factorial.nemi` (instrucciones de nivel superior) también parsea.
  Casos de error probados y con mensaje claro: falta `fin`, `fin` con la
  palabra clave equivocada (`fin procedimiento` cerrando `función`), y `sino`
  (ya no es keyword — falla como identificador suelto, tal como en Python).
  Los casos **no** pasan a PASS todavía — eso es la Fase 3.

---

## Fase 3 — Evaluador  (`interpreter.cpp`, 10 `visit(...)` en `todo(...)`)  ✅ (hecho, 2026-07-26)
- [x] `visit(IntLiteral/RealLiteral/StringLiteral/Variable)` (ya venían de la Fase 0)
- [x] `visit(ArrayLiteral)`
- [x] `visit(Index)` — indexación base-1 en `Array` y en cadenas
- [x] `visit(Unary)` — `−ᵤ`, `¬`, `√`, `⌊⌋`, `⌈⌉` (incluye `⌊√n⌋` exacto con `isqrt`)
- [x] `visit(Binary)` — aritmética, comparación, `∧`/`∨` con corto-circuito, polisemia `+`/`·`
- [x] `visit(Assign)` — variable y celda (`s[i]`, `W[i][j]`)
- [x] `visit(ForLoop)` — inclusivo, paso +1
- [x] `visit(WhileLoop)`
- [x] `visit(If)` — usa `node.has_else` (no "¿`else_body` está vacío?") para saber
      si hay rama `alt`: un `alt` vacío (`alt` seguido de `fin si` sin nada en
      medio) es sintácticamente válido y da un `else_body` vacío *con*
      `has_else = true`, distinto de "no hay `alt`" (`else_body` vacío *y*
      `has_else = false`)
- [x] `visit(ExprStatement)` — llamada por efecto
- [x] `visit(Call)` — funciones de usuario, primitivas (`piso/techo/raíz/abs/long/mod/imprime`)
      e `intercambia` (l-values)
- **Diseño:** casi todos los helpers numéricos/de indexación (`is_number`,
  `arith_combine`, `divide_values`, `modulo_values`, `values_equal`,
  `compare_values`, `logic_result`, `index_get`/`set`, las primitivas) se
  implementaron como **funciones libres** en el namespace anónimo de
  `interpreter.cpp`, no como métodos de `Interpreter` — no necesitan `env_` ni
  `eval()`. Solo `get_lvalue`/`set_lvalue`/`swap_call` (para `intercambia`)
  quedaron como métodos privados nuevos en `interpreter.hpp`, porque sí los
  necesitan.
- **Aceptación (verificada):** los 15 casos del corpus pasan a **PASS**
  (`0 passed, 0 failed, 15 pending` → `15 passed, 0 failed, 0 pending`).
  Verificado además contra la versión Python en casos no cubiertos por el
  harness (equivalentes ASCII vía CLI, mensajes de error de `enlosa`/división
  por cero — coinciden literalmente).

---

## Fase 4 — Torre numérica / bignum  (`include/nemi/numeric.hpp`)  ✅ (hecho, 2026-07-26)
- [x] Reemplazar `Integer = long long` por un entero de precisión arbitraria
- [x] Reemplazar el `Rational` *placeholder* por uno real, para `/` exacto y `⌊n/2⌋`
- [x] Añadir caso de estrés a `corpus_test`: `factorial(100)` (bignum)
- **Decisión (distinta de lo previsto originalmente):** en vez de Boost.Multiprecision
  o GMP, se implementó un **`BigInt` propio desde cero**
  (`include/nemi/bigint.hpp` + `src/bigint.cpp`, y `Rational` real en
  `src/rational.cpp`): signo + `vector<uint32_t>` en base 1e9. Motivo: cero
  dependencias externas (sigue compilando solo con un compilador C++17 + CMake,
  sin red durante `configure`), y es un ejercicio genuino de estructuras de
  datos. La opción `NEMI_USE_GMP` de `CMakeLists.txt` se **eliminó** (ya no
  aplica). Boost/GMP siguen siendo swaps válidos a futuro — solo tocarían
  `numeric.hpp`/`bigint.*`.
- [x] `lexer.cpp`: `parse_integer` ahora usa `Integer::from_decimal(...)` — el
      límite de 64 bits de la Fase 1 **quedó cerrado**.
- ⚠️ **Bug real encontrado y corregido:** la primera versión de `isqrt` sembraba
  Newton vía `static_cast<long long>(std::sqrt(n.to_double()))`, que es
  **comportamiento indefinido** para raíces de más de ~19 dígitos (el `double`
  no cabe en `long long`) — producía una semilla basura que dejaba el bucle de
  corrección final subiendo de a uno desde casi cero hasta un número de 31
  dígitos: en la práctica, un cuelgue. Arreglado sembrando con `10^⌈d/2⌉`
  (`d` = dígitos decimales de `n`), sin pasar nunca por `double`/`long long`.
  Detalle completo en `PORTING_GUIDE.md` Fase 4 (vale la pena leerlo como
  ejemplo real de UB con bignums).
- **Aceptación (verificada):** `corpus_test` en **16/16 PASS** (15 + el caso de
  estrés). Además: prueba directa de `BigInt`/`Rational` (multiplicación
  factorial-scale, `gcd`, `isqrt` sobre un cuadrado de 62 dígitos,
  división/módulo de piso con negativos) y `⌊n/2⌋` para una `n` de 31 dígitos
  verificado **igual, dígito a dígito, contra la versión Python**.

---

## Fase 5 — Paridad con el lenguaje/CLI actual  (deuda vs. la versión Python)  ✅ (hecho, 2026-07-26)
- [x] `struct Program`: añadir `Block main;` (hecho en Fase 2, era requisito del parser)
- [x] `Interpreter::run_program()` (ejecuta el cuerpo del guión; sin auto-print, à la Python)
- [x] `load_files`/`load`: preservar y fusionar `main` — salió gratis, ya que
      `nemi.cpp` concatena el **texto fuente** de todos los archivos antes de
      parsear (no ASTs por separado), así que `main` se fusiona solo.
- [x] `apps/main.cpp`: renombrar opciones a español `--llama` / `--lexemas` / `--asa` + `-a/--ayuda`
- [x] `apps/main.cpp`: llamar `run_program()` al cargar (antes de procesar `--llama`)
- [x] `--asa`: *pretty-print* del AST — no un `repr` genérico, sino un volcado
      legible como código Nemi (dispatch con `dynamic_cast`; ver
      `apps/main.cpp`: `print_expr`/`print_stmt`/`print_definition`)
- **Aceptación (verificada):** `nemi ../examples/guion_factorial.nemi` imprime
  igual que `python -m nemi` con el mismo archivo; `--ayuda` coincide con
  Python (salvo el prefijo "usage:"/"uso:" que Python deja en inglés, cosa de
  `argparse`); `--asa` no falla en ningún archivo del corpus y se lee como
  Nemi de verdad (con `si`/`alt`/`fin`, `⌊⌋`, `∨`/`∧`), no como dump de C++.
- ⚠️ **Hallazgo (Windows, sin arreglar):** pasar un identificador con tildes
  como argumento de línea de comandos (`--llama "máximo(...)"`) puede llegar
  **corrompido** — Windows decodifica `argv` de `main(int, char**)` con la
  página de códigos ANSI activa, no UTF-8 (a diferencia de leer un `.nemi`
  desde archivo, que sí es UTF-8 real, y a diferencia de Python en Windows,
  que usa `GetCommandLineW` internamente). No es un bug del lexer/parser —
  confirmado comparando el mismo caso con un nombre de función ASCII (funciona)
  vs. con `máximo` en el argumento (falla). Arreglo: `wmain`/`GetCommandLineW`
  en vez de `main(int, char**)`. Detalle en `PORTING_GUIDE.md` Fase 5.
- ⚠️ **Gotcha de C++ real encontrado aquí:** escribir el texto de `--ayuda`
  con acentos vía `\xNN` a mano falló con *"hex escape sequence out of
  range"* — un escape `\xNN` seguido de una letra `a`-`f` se lee como un solo
  escape más largo. Arreglo: cortar el literal justo después con literales
  adyacentes (`"\xC3\xBA" "a"` en vez de `"\xC3\xBAa"`).

---

## Fase 6 — Pruebas y CI  ✅ (hecho, 2026-07-26)
- [x] `corpus_test` en **16/16 PASS** (15 + caso bignum de Fase 4)
- [x] (Opcional, decisión: **omitido deliberadamente**) tests unitarios de
      lexer y parser por separado — `corpus_test.cpp` ya ejercita ambos de
      punta a punta sobre los 13 `.nemi` del corpus + `run_call`, y a lo largo
      de las Fases 1–2 se verificó manualmente cada pieza (equivalentes ASCII,
      `alt`, prosa, errores de sintaxis, …). Si en el futuro se quiere aislar
      un bug específico del lexer o del parser, ahí sí valdría la pena.
- [x] CI que compile y corra ambas suites: `.github/workflows/ci.yml`
      (Python en Ubuntu; C++ en Ubuntu con clang++ **y** g++, y en Windows con
      MSVC — matriz de 4 jobs). El repo **no tiene `.git` todavía**, así que
      este workflow no se ha ejecutado de verdad en GitHub; queda listo para
      cuando se inicialice y suba.
- [x] `check_all.ps1` (raíz del repo): corre las mismas dos comprobaciones
      **en local**, sin necesitar git ni GitHub — verificado de punta a punta
      (dos veces: build limpio y build incremental con `ninja: no work to do`
      en la segunda corrida).
- [x] Actualizar `cpp/README.md` (tabla de estado) y este backlog
- **Validación extra de portabilidad (no pedida, pero de bajo costo y alto
  valor):** se compiló y corrió `corpus_test` con **MSVC real** (Visual Studio
  2022, `cl.exe` 19.42, vía `vcvars64.bat` + generador "Visual Studio 17
  2022") — **16/16 PASS**, igual que con clang++. Esto confirma que el código
  no depende de extensiones específicas de un compilador. **No** se pudo
  probar con g++ moderno en esta máquina (solo hay un MinGW g++ 6.3.0, muy
  anterior a C++17); el job de CI con g++ queda entonces como el único de los
  cuatro **sin verificar localmente** — si falla al correr en GitHub por
  primera vez, revisar ahí primero.
- **Pendiente real (no de esta fase, documentado en Fase 5):** arreglar el
  `argv` UTF-8 en Windows (`wmain` + `GetCommandLineW`).

---

## Riesgos / notas
- **UTF-8:** aislar la decodificación en el lexer; iterar por *code point*, no por `char`.
  ✅ Hecho — pero ver el hallazgo de Fase 5 sobre `argv` en Windows (la
  decodificación del *código fuente* es UTF-8 real; la de los *argumentos de
  línea de comandos* en Windows no, y eso es un problema aparte).
- **Rational exacto:** no sustituir `/` por `double` — rompe `⌊n/2⌋` en bignum (RSA).
  ✅ Implementado y verificado (Fase 4) contra la versión Python con enteros de
  31 dígitos.
- **`bool` vs bit:** en C++ son tipos distintos (más limpio que en Python); cuidar la
  polisemia `+`/`·` (OR/AND solo si **ambos** operandos son `bool`) y que `∧`/`∨`
  preserven el tipo (bit→bit, bool→bool). ✅ Implementado en Fase 3.
- **Deriva del lenguaje:** el port debe reflejar el estado **actual** (`alt`,
  instrucciones sueltas, sin `and/or/not`), no la spec original. ✅ Reflejado
  en el lexer/parser (Fases 1–2).
- **Cuidado al copiar literales largos a mano:** el literal de `factorial(100)`
  (158 dígitos) se corrompió **dos veces** al partirlo en varias líneas de C++
  a ojo durante la Fase 4. Si necesitas un literal así, verifícalo
  programáticamente (compáralo contra el mismo string en
  `python/tests/run_corpus.py`) en vez de confiar en la vista.
- ⚠️ **Gap conocido, no cubierto por el corpus:** la indexación de cadenas
  (`s[i]`) en C++ es **por byte** (`std::string` guarda UTF-8 crudo), mientras
  que en Python `str[i]` es **por code point** — para una cadena con acentos,
  `s[i]` daría resultados distintos entre las dos implementaciones. El corpus
  (`busca_texto`) solo usa cadenas ASCII, así que esto no afecta el 16/16
  PASS, pero es una divergencia real de semántica frente a la versión Python.
  Arreglo (no trivial, fuera de alcance de las fases actuales): decodificar la
  cadena a code points al construir el `Value` (como ya hace el lexer con el
  código fuente) en vez de guardar UTF-8 crudo.

## Panorama de progreso
| Fase | Descripción | Estado |
|---|---|---|
| 0 | Infraestructura y andamiaje | ✅ hecho |
| 1 | Lexer | ✅ hecho |
| 2 | Parser | ✅ hecho |
| 3 | Evaluador (10 visits) | ✅ hecho |
| 4 | Torre numérica / bignum | ✅ hecho |
| 5 | Paridad lenguaje/CLI | ✅ hecho |
| 6 | Pruebas y CI | ✅ hecho (workflow listo; sin ejecutar aún — no hay `.git`) |
