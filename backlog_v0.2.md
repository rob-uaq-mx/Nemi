# Backlog — biblioteca estándar de Nemi (spec v0.2, `Nemi.md` §19–§23)

Seguimiento del trabajo para implementar la biblioteca estándar y las
extensiones del núcleo que introdujo la versión 0.2 de la especificación.
Ver también [`Notas.md`](Notas.md) (revisión de la propuesta contra el
código) y el propio [`Nemi.md`](Nemi.md) §19–§24.

**Estado global** (al 2026-08-02):

| Fase | `python/` | `cpp/` + `Windows/` | Casos nuevos |
|---|---|---|---|
| 0 — ya funcionaba | ✅ | ✅ | — (verificación) |
| 1 — `Conjunto` y literales | ✅ | ✅ | 17 |
| 2 — `para cada … en …` | ✅ | ✅ | 5 |
| 3 — listas dinámicas | ✅ | ✅ | 8 |
| 4 — `afirma` | ✅ | ✅ | 8 |
| 5 — `traza` [OPC] | ✅ | ✅ | 5 |
| 6 — primitivas de cadena | ✅ | ✅ | 10 |
| 7 — los 6 módulos reales | ✅ | ✅ | 8 |
| 8 — paridad e integración | ✅ | ✅ | — (integración, sin casos propios) |

**v0.2 completa de punta a punta.** `python/tests/run_corpus.py`: **87/87**.
`{cpp,Windows}/tests/corpus_test.exe`: **82/82** cada uno. CLI real y
`WinNemi.exe` verificados tras cada port. §19–§23 completas en las tres
implementaciones; `README.md`/`cpp/README.md`/`Windows/README.md`
actualizados.

## Cómo usar este documento
- Mismo formato que `cpp/backlog.md`/`Windows/backlog.md`: casillas
  `- [ ]` → `- [x]`, orden de fases recomendado (cada una desbloquea la
  siguiente).
- **Orden de implementación sugerido:** Python primero como referencia (igual
  que se hizo con el port v0.1 a C++), luego portar a `cpp/`, luego
  sincronizar `Windows/` — el patrón que ya siguió todo este proyecto.
- **Criterio de terminado:** las líneas `afirma` de los 6 módulos de §22
  corren y pasan en las tres implementaciones, con un `corpus_test`/
  `run_corpus.py` actualizado que las incluya (mismo patrón que el corpus §17).

## Leyenda de estado
`[x]` hecho · `[ ]` pendiente · `[~]` en progreso · `[!]` bloqueado

---

## Fase 0 — Ya resuelto sin saberlo (verificado, no hace falta trabajo)

Encontrado durante la revisión de `Notas.md`: varias piezas que §20 pide como
"requeridas" **ya funcionan hoy** en las tres implementaciones. No requieren
ningún cambio, solo falta declarar que existen:

- [x] Literales de lista `[1,2,3]`, `[]`, anidados (`parser.cpp`/`parser.py`,
      ya documentado en `docs/ARCHITECTURE.md`).
- [x] Igualdad estructural de listas (`values_equal` en `interpreter.cpp` /
      equivalente en Python) — compara recursivamente, no por referencia.
- [x] Igualdad entre tipos distintos → `falso`, sin error (ya es el
      comportamiento por defecto).
- [x] Indexación de cadenas `s[i]` (base 1) — ya existe, la usa todo
      `cadenas.nemi` (§22.6).

---

## Fase 1 — Tipo `Conjunto` y literales (`Nemi.md` §20.1–§20.2)  ✅ (hecho, 2026-08-01, las tres implementaciones)
- [x] **`python/`** Nuevo caso de valor `Conjunto` (`values.py`): lista interna
      mantenida siempre en **orden canónico**, sin duplicados (inserción
      idempotente vía igualdad estructural `==`, O(n) por inserción — no usa
      el `set` nativo de Python porque los elementos pueden ser `Array`
      (mutables, no *hashable*)).
- [x] **`python/`** Literal `{ }` / `{1,2,3}` — tokens `LBRACE`/`RBRACE`
      (`tokens.py`, `lexer.py`), producción `_set_literal` en el parser
      (`conjunto_lit` de `primaria`), nodo AST `SetLiteral` (`ast_nodes.py`).
- [x] **`python/`** Literal `∅` como conjunto vacío — token `EMPTYSET`
      dedicado, distinto del uso de `∅` en `format_value` para `monostate`;
      verificado con pruebas explícitas que **no** se confunden (`∅` de
      "sin valor" imprime `∅`, `∅` literal de conjunto imprime `{}`).
- [x] **`python/`** Orden canónico (`_canonical_key` en `values.py`): números
      (incluye bool) por valor ascendente, cadenas lexicográfico, compuestos
      (`Array`/`Conjunto`) después, por su `format_value`.
- [x] **`python/`** Igualdad estructural para `Conjunto` (`__eq__`, mismos
      elementos sin importar orden de inserción — como el orden canónico es
      determinista, comparar las listas internas ya ordenadas basta).
- [x] **`python/`** Primitivas: `pertenece`, `subconjunto`, `union`,
      `interseccion`, `diferencia`, `cardinalidad`; `long` extendido para
      aceptar `Conjunto`.
- [x] **`python/`** Operadores `∈ ∉ ⊆ ⊂` en el nivel de `_COMPARISONS`/
      `op_comp` del parser y `_COMPARATORS` del intérprete (misma precedencia
      que `=`/`≠`/etc., tal como pide §20.2).
- [x] **`python/`** 18 pruebas nuevas en `python/tests/run_corpus.py`
      ("extra: Conjunto (v0.2)") — **44/44** en total, sin regresión.
- [x] **`cpp/`** Mismo diseño, adaptado a C++: nuevo `SetPtr =
      std::shared_ptr<Conjunto>` en el `Value` variant (`value.hpp`), clase
      `Conjunto` en `include/nemi/conjunto.hpp`/`src/conjunto.cpp` (mirroring
      `Array`), nuevos `TokenKind::{LBrace,RBrace,EmptySet,In,NotIn,SubsetEq,
      Subset}`. **Refactor de paso:** `values_equal` (antes local a
      `interpreter.cpp`) se movió a `value.cpp`/`value.hpp` (función
      exportada) para que `Conjunto::add`/`contains` (`conjunto.cpp`) y el
      operador `=` compartan la misma noción de igualdad estructural — antes
      había una sola copia porque solo `interpreter.cpp` la necesitaba; ahora
      dos módulos distintos la necesitan. 17 casos nuevos en
      `tests/corpus_test.cpp` — **38/38**, sin regresión. CLI real probado.
- [x] **`Windows/`** Mismos archivos sincronizados desde `cpp/`; `interpreter.
      {hpp,cpp}` y `parser.{hpp,cpp}` se **fusionaron a mano** (no se
      copiaron tal cual) porque ya tenían las adiciones de `run_line`/
      `parse_single_statement`/`is_at_end` específicas de la consola de
      WinNemi (sesión anterior) — no perderlas era el punto. **38/38** en
      `corpus_test.exe`; `WinNemi.exe` compila y arranca sin problemas
      (verificado: lanzar y cerrar limpio).

## Fase 2 — `para cada … en …` (`Nemi.md` §20.3)  ✅ (hecho, 2026-08-01, las tres implementaciones)
- [x] **`python/`** Palabras clave nuevas: `cada` → `TokenKind.EACH`, `en` →
      `TokenKind.WITHIN` (**no** `TokenKind.IN` — ese nombre ya lo usa `∈` de
      la Fase 1; son conceptos distintos aunque ambos se lean "in" en
      inglés). Se revisó el corpus §17 y los ejemplos existentes por si
      `cada`/`en` se usaban como identificador — no se encontró ningún caso,
      seguro reservarlas.
- [x] **`python/`** Nueva producción `ciclo_cada` en el parser
      (`_for_each`) + nodo AST `ForEach` (`ast_nodes.py`). Un token de
      lookahead extra en `_statement()` distingue `para cada …` de
      `para ident ← … hasta …` (ambos empiezan con `para`).
- [x] **`python/`** Interpretación (`_exec_for_each`): itera `Array` en
      orden de índice, `Conjunto` en orden canónico (ya garantizado por como
      `Conjunto` se almacena internamente, Fase 1); cualquier otro tipo
      lanza `ExecutionError`. Mutar la colección durante la iteración queda
      explícitamente indefinido por la spec (§20.3) — no se defiende contra
      eso a propósito, se itera la secuencia viva.
- [x] **`python/`** 5 pruebas nuevas en `run_corpus.py`
      ("extra: para cada (v0.2)") — **49/49** en total, sin regresión. El
      orden canónico vs. de índice se verificó codificando el orden de
      visita como un solo número (`v ← v·10 + elem`), ya que `agrega`
      (Fase 3) todavía no existe para construir una lista real de lo visto.
- [x] **`cpp/`** Mismo diseño: nuevos `TokenKind::{Each,Within}` — **`Within`**
      en vez de `In` para no chocar con el `In`/`∈` ya usado en la Fase 1
      (ambos se leen "in" en inglés, pero son conceptos distintos). Nodo
      `ForEach : Stmt`, un token de lookahead extra en `parse_statement()`
      para distinguir `para cada …` de `para ident ← …`. 5 casos nuevos en
      `corpus_test.cpp` — **43/43**, sin regresión. CLI real probado.
- [x] **`Windows/`** `interpreter.{hpp,cpp}` y `parser.{hpp,cpp}` de nuevo
      fusionados a mano (no copiados) por las mismas adiciones de `run_line`
      de la consola de WinNemi; el resto de archivos sí se copiaron tal
      cual. **43/43** en `corpus_test.exe`; `WinNemi.exe` compila y arranca
      sin problemas (verificado de nuevo).

## Fase 3 — Listas dinámicas (`Nemi.md` §20.4)  ✅ (hecho, 2026-08-01, las tres implementaciones)
- [x] **`python/`** `Array.append` nuevo en `values.py` (antes `Array` solo
      tenía `get`/`set`/`length`, tamaño fijo). `agrega(a, x)`: sobre lista,
      anexa al final mutando por referencia (`a.append(x)`); sobre conjunto,
      inserta de forma idempotente reusando `Conjunto.add` (Fase 1) — mismo
      primitivo, dos comportamientos según el tipo de `a`.
- [x] **`python/`** `copia(a)`: copia profunda recursiva. Los escalares
      (int/Fraction/float/bool/str) ya son inmutables en Python, así que
      `_deep_copy` los regresa tal cual; solo `Array`/`Conjunto` se
      reconstruyen recursivamente.
- [x] **`python/`** `arreglo_cero(n)`, `matriz_cero(m, n)` — validan que
      `n`/`m` sean enteros no negativos (mismo patrón de error que el resto
      de primitivas).
- [x] **`python/`** 8 pruebas nuevas en `run_corpus.py`
      ("extra: listas dinámicas (v0.2)") — **57/57** en total, sin
      regresión. Cubren mutación por referencia, idempotencia en conjuntos,
      independencia de `copia` (incluso anidada), y los dos casos de error.
- [x] **`cpp/`** Mismo diseño: `Array::append` nuevo (mirroring
      `Array.append`), primitivas `prim_agrega`/`prim_copia`/
      `prim_arreglo_cero`/`prim_matriz_cero` (`deep_copy` recursivo sobre
      `Value`, análogo a `_deep_copy`). 8 casos nuevos en `corpus_test.cpp`
      — **51/51**, sin regresión.
- [x] **`Windows/`** `interpreter.cpp` fusionado a mano de nuevo (mismo
      motivo que las Fases 1–2: las adiciones de `run_line`); el resto se
      copió tal cual. **51/51** en `corpus_test.exe`.
      **Nota de depuración:** el primer intento de probar el CLI
      (`nemi.exe`) falló con `undefined function or procedure: agrega`
      aunque `corpus_test.exe` ya pasaba — el vínculo incremental de MSVC no
      había re-enlazado `nemi.exe` contra el `nemi.lib` recién compilado
      (`nemi.exe` quedó con una marca de tiempo **anterior** a `nemi.lib`,
      visible con `ls -la`). Se resolvió borrando `nemi.exe` y forzando un
      vínculo completo (`cmake --build ... --target nemi-cli`); no era un
      bug de código. Verificar marcas de tiempo build vs. lib es la manera
      rápida de detectar este problema si vuelve a aparecer.

## Fase 4 — `afirma` (`Nemi.md` §21.2)  ✅ completa en las tres implementaciones (2026-08-02)
- [x] **`python/`** Palabra clave `afirma` → `TokenKind.ASSERT`; producción
      `_assert` en el parser (`aserción ::= "afirma" expresión [ "," cadena
      ]` — el mensaje es literalmente un token `STRING`, no una expresión
      general, así que se extrae directo sin evaluar nada).
- [x] **`python/`** Nodo AST `Assert` + `_exec_assert`: evalúa la condición
      con `is_truthy` (misma regla que `si`/`mientras`, no una comprobación
      estricta de bit 0/1); si es falsa, lanza `ExecutionError` con
      `node.location` — el archivo+línea correctos llegan gratis, incluso a
      través de `incluye`, porque ya es el mismo mecanismo de `SourceLocation`
      de la sesión de `incluye`. Verificado explícitamente con una prueba
      dedicada (`examples/prueba_afirma.nemi`).
- [x] **Decisión sobre "el texto fuente de la expresión":** se optó por
      reconstruir un pretty-print desde el AST (`_expr_to_source`, nueva
      función en `interpreter.py`) en vez de guardar rangos de texto crudo
      en el lexer/parser — no requiere tocar el lexer (que no rastrea
      offsets de bytes) en ninguna de las tres implementaciones, y el
      resultado es *siempre* consistente con cómo se parseó de verdad la
      expresión. Mismo enfoque que ya usaba `--asa` en C++, pero ahora vive
      en el núcleo (`interpreter.py`), no solo en el CLI, porque `afirma`
      lo necesita en tiempo de ejecución.
- [x] **`python/`** 8 pruebas nuevas en `run_corpus.py`
      ("extra: afirma (v0.2)") — **65/65** en total, sin regresión. Cubren
      caso verdadero, falso con/sin mensaje, igualdad estructural de listas,
      nivel superior (guión), y el archivo correcto vía `incluye`.
- [x] **`cpp/`** Se extrajo el pretty-printer de expresiones que antes vivía
      solo en `apps/main.cpp` (`op_symbol`/`print_expr`) hacia una nueva
      pareja de archivos compartidos por la librería, `include/nemi/pretty.hpp`
      + `src/pretty.cpp` (`nemi::print_expr_source`/`nemi::expr_to_source`),
      const-correctos (`dynamic_cast<const X*>`). `apps/main.cpp` ahora usa
      `using nemi::print_expr_source;` en vez de tener su propia copia — cero
      lógica duplicada. `TokenKind::Assert`, el nodo AST `Assert`
      (`ExprPtr condition; std::optional<std::string> message;`),
      `Parser::parse_assert()` e `Interpreter::visit(Assert&)` son un mirror
      directo de la versión Python. `print_stmt` (`--asa`) también gana un
      caso para `Assert`.
- [x] **`cpp/`** 8 pruebas nuevas en `tests/corpus_test.cpp` (mismos casos que
      `run_corpus.py`) — **59/59** en total, sin regresión. Verificado además
      con el CLI real (`nemi.exe` sobre `examples/prueba_afirma.nemi` y
      `--asa` sobre una función con `afirma`).
- [x] **`Windows/`** Todos los archivos aditivos (`token_kind.*`, `lexer.cpp`,
      `ast.hpp`, los nuevos `pretty.hpp`/`pretty.cpp`, `apps/main.cpp`,
      `tests/corpus_test.cpp`) se copiaron tal cual de `cpp/` tras confirmar
      con `diff` que no había divergencia previa de WinNemi en esos archivos.
      `parser.hpp/.cpp` e `interpreter.hpp/.cpp` se fusionaron a mano (mismo
      patrón que Fases 1–3) para conservar `run_line`/`parse_single_statement`/
      `is_at_end`. `CMakeLists.txt` ganó una línea (`src/pretty.cpp`) en la
      lista de fuentes de `add_library(nemi ...)`. **59/59** en
      `corpus_test.exe` con MSVC real; sin el bug de vínculo incremental esta
      vez (marca de tiempo de `nemi.exe` posterior a `nemi.lib`, verificado).
      CLI verificado end-to-end y `WinNemi.exe` (el editor+consola, ya
      presente en este árbol) se lanzó y cerró sin problemas tras el rebuild.

## Fase 5 — `traza` [OPC, baja prioridad] (`Nemi.md` §21.3)  ✅ completa en las tres implementaciones (2026-08-02)
- [x] **`python/`** Palabra clave `traza` → `TokenKind.TRACE`; producción
      `_trace` en el parser (`rastreo ::= "traza" expresión` — mismo patrón
      minimalista que `_assert`, sin cadena opcional).
- [x] **`python/`** Nodo AST `Trace` (`expression: Expr`) + `_exec_trace`:
      activa el rastreo (`self._trace_depth += 1`), evalúa la expresión (el
      valor se descarta, igual que un `ExprStatement`) y lo desactiva en un
      `finally`. Se usa un **contador**, no un booleano, para que un `traza`
      anidado dentro de una llamada ya rastreada no apague el rastreo al
      terminar — la extensión dinámica del `traza` exterior sigue activa.
- [x] **`python/`** Rastreo de entrada/retorno en `_invoke`: con
      `_trace_depth > 0`, imprime `→ nombre(args)` **antes** de incrementar
      `self._depth` (con sangría `"  " * self._depth`, es decir, la
      profundidad de la pila *antes* de esta llamada) y `← valor` **después**
      de decrementarlo — así ambas líneas de un mismo par quedan a la misma
      sangría, y las llamadas anidadas quedan más adentro. Si el cuerpo lanza
      una excepción que no sea `ReturnSignal` (p. ej. un `afirma` roto), la
      línea de entrada ya se imprimió pero la de retorno **no** — es
      justamente lo esperado (la llamada abortó, no regresó).
- [x] **`python/`** Rastreo de asignación en `_exec_assign`: imprime
      `  lugar ← valor` con una sangría extra respecto a la entrada/retorno
      de la función que la contiene (`"  " * self._depth + "  "`), tal como
      lo muestra el ejemplo de la spec §21.3. `lugar` incluye los índices ya
      evaluados (`arr[1]`, no la expresión fuente del índice) para reflejar
      la celda realmente escrita.
- [x] **Decisión de diseño:** se reutiliza `self._depth` (ya existente para
      el guardia de recursión) como métrica de "profundidad de la pila" para
      la sangría — la spec no especifica si debe ser relativa al punto donde
      empezó el `traza` o absoluta; se optó por la absoluta (más simple, un
      único contador, sin estado adicional que sincronizar entre
      `_invoke`/`_exec_assign`/`_exec_trace`).
- [x] **`python/`** 5 pruebas nuevas en `run_corpus.py` ("extra: traza
      (v0.2)"), capturando stdout con `contextlib.redirect_stdout` y
      comparando el texto exacto — **70/70** en total, sin regresión. Cubren:
      recursión anidada (`factorial`, verifica sangría por profundidad),
      asignaciones dentro de un bucle (`mcd`, verifica sangría extra),
      `traza` de una expresión que no es llamada (no imprime nada), `traza`
      anidado (una llamada ya trazada implícitamente por la extensión
      dinámica del `traza` exterior, no solo por el `traza` interior
      explícito), y abortar a medio camino (entrada impresa, sin línea de
      retorno). No hizo falta tocar `cli.py`: `--asa`/`--lexemas` imprimen
      vía `repr()` automático del `@dataclass Trace`/`Token(TRACE)`, no un
      pretty-printer manual, así que el nodo nuevo salió gratis.
- [x] **`cpp/`** `TokenKind::Trace`, palabra clave `"traza"` en el lexer, nodo
      AST `Trace` (`ExprPtr expression`) y `Parser::parse_trace()` — mirrors
      directos de la versión Python. `Interpreter` gana un contador
      `trace_depth_` (mismo motivo que `_trace_depth` en Python: nested
      `traza` no debe apagar el rastreo del `traza` exterior).
      `Interpreter::invoke` imprime entrada/retorno alrededor de
      `exec_block`/`ReturnSignal` (con `std::string(2 * depth_, ' ')` como
      sangría); `visit(Assign&)` se reestructuró para calcular `place`
      (incluye índices ya evaluados) y el texto del valor **antes** de mover
      `value` a `env_->define`/`index_set`, y emite la línea de traza después.
      `visit(Trace&)` activa/desactiva el contador en un `try/catch(...)`
      (no `finally` en C++, así que el `catch(...)` decrementa y relanza).
      `print_stmt` (`--asa`, en `apps/main.cpp`) gana un caso para `Trace`.
- [x] **`cpp/`** 5 pruebas nuevas en `tests/corpus_test.cpp` (mismos casos
      que `run_corpus.py`), usando un nuevo helper `capture_stdout` (redirige
      `std::cout.rdbuf()` a un `std::ostringstream`, restaura incluso si la
      llamada lanza) — **64/64** en total, sin regresión. Verificado además
      con el CLI real (`nemi.exe` ejecutando un `traza factorial(3)` de
      nivel superior, y `--asa` sobre una función con `traza`).
- [x] **`Windows/`** Todos los archivos aditivos (`token_kind.*`, `lexer.cpp`,
      `ast.hpp`, `apps/main.cpp`, `tests/corpus_test.cpp`) se copiaron tal
      cual de `cpp/` tras confirmar con `diff` que no había divergencia
      previa de WinNemi en esos archivos. `parser.hpp/.cpp` e
      `interpreter.hpp/.cpp` se fusionaron a mano (mismo patrón que Fases
      1–4) para conservar `run_line`/`parse_single_statement`/`is_at_end`.
      No hizo falta tocar `CMakeLists.txt` (no hay archivos nuevos esta
      fase). **64/64** en `corpus_test.exe` con MSVC real (tras un reintento
      por el error transitorio de bloqueo de archivos de Dropbox de
      siempre); marca de tiempo de `nemi.exe` posterior a `nemi.lib`,
      verificado. CLI verificado end-to-end y `WinNemi.exe` se lanzó y cerró
      sin problemas tras el rebuild.

## Fase 6 — Primitivas de cadena (`Nemi.md` §22.6)  ✅ completa en las tres implementaciones (2026-08-02)
- [x] **`python/`** `concatena(s, t)`: exige que ambos argumentos sean
      cadenas (si no, `ExecutionError`); regresa `s + t`. No hay operador
      `+` para cadenas (`_arith` sigue exigiendo números) — `concatena` es
      la única vía, tal como lo introduce la spec.
- [x] **`python/`** `texto(x)`: exige un número (`_is_number`, no booleano);
      regresa `format_value(x)` — **la misma función** que ya usan
      `imprime`/`afirma`, así que `texto(3/2)` da `"3/2"` (Fraction con
      denominador ≠ 1), `texto(4/2)` da `"2"` (colapsa a entero), y
      `texto(5)` da `"5"`, consistente con cómo la spec construye
      `a_binario` dígito a dígito con `n mod 2`.
- [x] **`python/`** `valor(c)`: exige una cadena de longitud exactamente 1
      cuyo carácter esté en `'0'..'9'` (comparación de código de carácter
      explícita, no `str.isdigit()` — ese método acepta dígitos Unicode no
      ASCII como superíndices, que no son "un dígito" en el sentido de la
      spec); regresa `ord(c) - ord('0')`.
- [x] **`python/`** `long(s)` e indexación `s[i]` (base 1, cadena de
      longitud 1) ya existían de v0.1 — no fue necesario tocarlos, solo
      documentarlos como parte del conjunto de primitivas que `cadenas.nemi`
      necesita.
- [x] **`python/`** 10 pruebas nuevas en `run_corpus.py` ("extra: primitivas
      de cadena (v0.2)") — **80/80** en total, sin regresión. Cubren los
      casos base, los casos de error de cada primitiva, y — la prueba más
      importante — **el código de `cadenas.nemi` (§22.6) copiado
      literalmente de la spec** (`invierte`, `prefijo`, `sufijo`,
      `a_binario`, `desde_base` + sus 5 `afirma`), cargado y ejecutado con
      `run_program()`: si cualquier `afirma` fuera falso, la prueba lo
      reportaría. Es la primera vez que ese código de la spec corre de
      verdad (antes solo se había revisado a mano, ver `Notas.md` §7).
- [x] **`cpp/`** `prim_concatena`/`prim_texto`/`prim_valor`, junto a
      `prim_length` (misma sección "String primitives"), registradas en la
      tabla `primitives()`. Sin cambios de lexer/parser/AST — son primitivas
      normales, resueltas por `Call` igual que `piso`/`raíz`/etc. `texto`
      reutiliza `nemi::format_value` (la misma que usan `imprime`/`afirma`);
      `valor` compara el código de carácter explícitamente (`'0'..'9'`), no
      una función tipo `isdigit` de la librería estándar de C, por la misma
      razón que en Python: evitar dígitos no-ASCII.
- [x] **`cpp/`** 10 pruebas nuevas en `tests/corpus_test.cpp` (mismos casos
      que `run_corpus.py`, incluido el módulo `cadenas.nemi` §22.6 copiado
      literalmente y ejecutado con `run_program()`) — **74/74** en total,
      sin regresión. Verificado además con el CLI real.
- [x] **`Windows/`** `tests/corpus_test.cpp` se copió tal cual de `cpp/`
      (con `tr -d '\r'` para conservar el fin de línea LF que ya usaba ese
      archivo en `Windows/`, ya que la copia de `cpp/` traía CRLF).
      `src/interpreter.cpp` se fusionó a mano (mismo patrón que Fases 1–5)
      para conservar `run_line`; el resto de la sección de primitivas de
      cadena se agregó igual que en `cpp/`. **74/74** en `corpus_test.exe`
      con MSVC real. **Reapareció el bug de vínculo incremental de MSVC**
      (documentado por primera vez en la Fase 3): `nemi.exe` quedó con una
      marca de tiempo anterior a `nemi.lib` pese a que el log de compilación
      mostraba `nemi-cli.vcxproj -> ...nemi.exe`; el CLI fallaba con
      `undefined function or procedure: concatena` aunque `corpus_test.exe`
      ya pasaba. Mismo arreglo de siempre: borrar `nemi.exe` y forzar
      `cmake --build build --config Debug --target nemi-cli` (que esta vez
      sí imprimió explícitamente "no se encontró ... o no lo generó el
      último vínculo incremental; ejecutando vínculo completo"). CLI
      verificado end-to-end tras el arreglo y `WinNemi.exe` se lanzó y
      cerró sin problemas.

## Fase 7 — Los 6 módulos de la biblioteca (`Nemi.md` §22)  ✅ completa en las tres implementaciones (2026-08-02)
- [x] **Decisión de ubicación:** nuevo directorio (hermano de `examples/`),
      no dentro de `examples/` — el propósito es distinto (biblioteca
      reusable vs. corpus de aceptación de §17). Compartido entre las tres
      implementaciones, igual que `examples/`.
      **Renombrado 2026-08-02:** `stdlib/` → **`bibcom/`** ("biblioteca
      común", a petición del usuario) — igual el archivo agregador,
      `stdlib.nemi` → `bibcom.nemi`. `Nemi.md` sigue usando el nombre
      `stdlib`/`stdlib.nemi` en su propio texto (§19.2/§19.3); no se tocó el
      spec, el renombrado es solo de la carpeta/archivo en el repo de
      implementación.
- [x] Creados los 6 archivos reales — `teoria_numeros.nemi`, `conteo.nemi`,
      `conjuntos.nemi`, `relaciones.nemi`, `booleana.nemi`, `cadenas.nemi` —
      más `bibcom.nemi` (agregador: solo `incluye` de los seis, spec §19.3).
      Código de §22 copiado tal cual (mismos comentarios, mismos nombres de
      variable), sin ninguna corrección — ver el siguiente punto.
- [x] **Primera corrida real de §22 — sin errores.** Los 6 módulos y el
      agregador cargan y corren (`run_program()`) sin que ningún `afirma`
      aborte, tanto por separado como los 6 juntos vía `bibcom.nemi`. Esto
      confirma lo que `Notas.md` §7 ya sospechaba por revisión a mano
      (Euclides extendido, generadores de combinaciones/permutaciones,
      módulos booleano y de cadenas): el código de la spec era correcto
      desde el principio, sin necesidad de tocar ni una línea. Ningún choque
      de nombres entre los 6 módulos (se verificó la lista completa de
      funciones de cada uno antes de escribirlos).
      **Nota técnica:** `potencia(A)` (conjuntos.nemi) construye un
      `Conjunto` de `Conjunto`s (el conjunto potencia) usando `agrega`
      sobre un conjunto anidado — ejercita la igualdad estructural
      recursiva de `Conjunto.__eq__`/`_canonical_key` (Fase 1) con
      profundidad 2, un caso que las pruebas propias de Fase 1 no habían
      cubierto; pasó sin cambios.
- [x] **`python/`** 7 pruebas nuevas en `run_corpus.py` ("extra: bibcom/*.nemi
      (v0.2)"): las 6 correspondientes a cada módulo cargado por separado
      (`load_files` sobre el archivo real en `bibcom/`, no una copia
      embebida) más una para el agregador `bibcom.nemi` completo (ejercita
      `incluye` en cadena, 6 niveles) — **87/87** en total, sin regresión
      (reverificado tras el renombrado). Además de la prueba embebida de
      `cadenas.nemi` que ya existía de la Fase 6 (código inline, no el
      archivo real).
- [x] Verificado también con el CLI real:
      `python -m nemi bibcom/bibcom.nemi --llama "C(52, 5)"` → `2598960`.
- [x] **`cpp/`** No había archivos `.nemi` que portar (`bibcom/` ya es
      compartido entre las tres implementaciones, igual que `examples/`).
      Se añadió `NEMI_BIBCOM_DIR` en `tests/CMakeLists.txt` (mismo patrón
      que `NEMI_EXAMPLES_DIR`) y 8 pruebas equivalentes a
      `tests/corpus_test.cpp` (las 6 por módulo + el agregador +
      un caso de regresión, ver el siguiente punto).
- [x] **Bug real encontrado y corregido en `cpp/` (no en Python):**
      `bibcom/conjuntos.nemi` línea 38 —
      `afirma (2 ∈ {1, 2, 3}) = 1` — fallaba en `cpp/`/`Windows/` pero no en
      `python/`. Causa: `values_equal` en `value.cpp` no comparaba `bool`
      (lo que produce `∈`/`⊆`/`⊂`, spec §20.2: "producen un bit") contra un
      `Integer` — caían por el `if`/`if` de números y de bool por separado,
      terminando en "tipos distintos → no son iguales". En Python, `bool`
      es subtipo de `int`, así que `True == 1` es verdadero de forma nativa
      sin ningún código especial; en C++ los dos viven en alternativas
      distintas del `std::variant` y no había ningún puente entre ellas.
      **Arreglo:** dos casos nuevos al principio de `values_equal` que
      tratan un `bool` como `Integer(0)`/`Integer(1)` cuando el otro lado es
      un número genuino, antes de la rama numérica normal. Es exactamente el
      tipo de discrepancia entre implementaciones que la "primera corrida
      real" de la Fase 7 estaba buscando — encontrada por el propio código
      de la spec (§22.3), no por una prueba escrita a propósito. Se agregó
      una prueba de regresión explícita en la sección Conjunto de
      `corpus_test.cpp` (`pertenece_igual_uno`) para que no se repita en
      silencio. `python/` no necesitó ningún cambio (ya se comportaba
      correctamente por la semántica nativa de `bool`).
- [x] **`Windows/`** `src/value.cpp` (el arreglo de `values_equal`) y
      `tests/CMakeLists.txt` se fusionaron a mano (diffs limpios,
      confirmados con `diff` antes de tocar nada); `tests/corpus_test.cpp`
      se copió tal cual de `cpp/` (con `tr -d '\r'` para conservar LF, mismo
      patrón que Fases 5–6). **82/82** en `corpus_test.exe` con MSVC real.
      **Reapareció otra vez el bug de vínculo incremental de MSVC**
      (tercera vez consecutiva, Fases 3/6/7): mismo síntoma
      (`nemi.exe` con marca de tiempo anterior a `nemi.lib`, CLI fallando
      con el `afirma` de `conjuntos.nemi` pese a que `corpus_test.exe` ya
      pasaba), mismo arreglo (borrar `nemi.exe` y forzar
      `cmake --build build --config Debug --target nemi-cli`). CLI
      verificado end-to-end tras el arreglo
      (`nemi.exe bibcom\bibcom.nemi --llama "C(52, 5)"` → `2598960`) y
      `WinNemi.exe` se lanzó y cerró sin problemas.

## Fase 8 — Paridad e integración de pruebas  ✅ completa (2026-08-02)
- [x] Fase 1 ya portada a `cpp/`/`Windows/` de una vez (no se acumuló para el
      final) — ver el detalle en la propia Fase 1 arriba.
- [x] Portar Fases 2–7 de Python a `cpp/`, luego sincronizar a `Windows/` —
      hecho fase por fase a medida que se completaba cada una en Python (no
      se acumuló para el final, mismo patrón que la Fase 1), no como un
      trabajo aparte al llegar aquí. Ver el detalle de cada port en la
      sección de su propia fase arriba (2–7).
- [x] Agregar un caso a `python/tests/run_corpus.py` y a
      `{cpp,Windows}/tests/corpus_test.cpp` que cargue `bibcom.nemi` (via
      `incluye`) y confirme que corre sin abortar ningún `afirma` — igual
      que ya existe para `usa_biblioteca.nemi` (`incluye`, sesión anterior).
      Python hecho como parte de la Fase 7; `cpp/`/`Windows/` hecho al
      portar `corpus_test.cpp` (mismo día, tras el renombrado `stdlib/` →
      `bibcom/`).
- [x] Actualizado `README.md` / `Windows/README.md` / `cpp/README.md`:
      mencionan las tres implementaciones (antes `README.md` raíz no
      mencionaba `Windows/` en absoluto), el directorio `bibcom/`
      compartido, los conteos actuales de pruebas (87/87 Python, 82/82 cada
      C++) y enlazan a `backlog_v0.2.md`. También se corrigió una nota
      obsoleta en `README.md` raíz ("el repo aún no es un repositorio git"
      — ya lo es, con remoto en GitHub desde la sesión de configuración
      inicial).
- [x] Auditados el resto de los `.md` del repo (2026-08-02, a petición del
      usuario): `cpp/backlog.md`/`Windows/backlog.md` (nota apuntando a este
      backlog + corrección del estado de git), `docs/ARCHITECTURE.md` (nota
      de alcance v0.1-only al inicio + conteos actualizados),
      `cpp/PORTING_GUIDE.md`/`Windows/PORTING_GUIDE.md` (puntero, sin tocar
      el contenido histórico) y `para-llm-mates-discretas.md` (agregada una
      sección entera sobre `bibcom/`/`afirma`, más una corrección: `∪`/`∩`/
      `∖` **no** son sintaxis Nemi válida, solo la notación matemática que
      usa la spec en prosa — en código siempre son `union`/`interseccion`/
      `diferencia`). `Nemi.md` no se tocó (spec compartida con el otro
      colaborador).
- [x] **Bug de empaquetado encontrado en la auditoría y corregido:**
      `Windows/installer/WinNemi.iss` empaquetaba `examples/*.nemi` y
      `Nemi.md` pero **no** `bibcom/*.nemi` — quien instalara `WinNemi.exe`
      vía el instalador no tenía la biblioteca común disponible. Se agregó
      una línea `Source:` más (mismo patrón que `examples/`). Verificado de
      punta a punta: `make_installer.bat` completo (reconfigura, compila
      Release, `ctest` 82/82, empaqueta con Inno Setup 6 — el log de
      compresión confirma los 7 archivos de `bibcom/`), instalación
      silenciosa (`/VERYSILENT`), confirmado en disco
      (`%LocalAppData%\Programs\WinNemi\bibcom\*.nemi`, los 7 archivos),
      `WinNemi.exe` instalado se lanzó y cerró sin problemas, y
      desinstalación silenciosa limpia (no queda nada en
      `%LocalAppData%\Programs\WinNemi`). `Windows/installer/README.md`
      también actualizado: nota sobre cómo `incluye` resuelve rutas
      relativas al archivo que incluye, no a la carpeta de instalación —
      así que usar `bibcom/` desde un script guardado en otro lado requiere
      copiar la carpeta o usar una ruta relativa de vuelta a la instalación.

**Resumen de v0.2:** las tres implementaciones (`python/`, `cpp/`,
`Windows/`) están a la par en el núcleo v0.1 y en la biblioteca común v0.2
completa (§19–§23). `python/tests/run_corpus.py`: **87/87**.
`{cpp,Windows}/tests/corpus_test.exe`: **82/82** cada uno. CLI real y
`WinNemi.exe` verificados end-to-end en cada port. Sin trabajo pendiente en
el backlog v0.2 — cualquier extensión futura (más módulos, `∀`/`∃`
cuantificadores u otras ideas de `Notas.md`) empezaría un backlog nuevo.

### Addendum — `en` como palabra para pertenencia (2026-08-02)

A petición del usuario tras escribir el [manual](manual/00_indice.md), `en`
(`TokenKind::Within`) ahora también funciona como sinónimo de `∈` a nivel de
comparación (`x en A`, idéntico a `x ∈ A` o `pertenece(x, A)`) — antes solo
existía como parte fija de `para cada VAR en COLECCIÓN repite`. No hay
ambigüedad de gramática: `parse_for_each` consume el token `en` por posición
fija justo después de `cada VAR`, nunca a través de la producción general de
comparación, así que ambos usos conviven sin conflicto. Cambios: `parser.py`/
`.cpp` (`_COMPARISONS`/`is_comparison` gana `WITHIN`), `interpreter.py`/`.cpp`
(`_compare`/`compare_values` tratan `WITHIN` igual que `IN`; `_OP_SYMBOLS`/
`op_symbol` normalizan `en` de vuelta a `∈` en los mensajes de `afirma`, igual
que ya pasa con `!=`→`≠`). Portado y verificado en las tres implementaciones
el mismo día: **90/90** en Python, **85/85** en cada C++. `manual/07_conjuntos.md`
y `manual/referencia_rapida.md` actualizados (ya no dicen que `∈` carece de
equivalente ASCII). `Nemi.md` **no** se tocó — es la spec compartida con el
otro colaborador; si se quiere reflejar ahí (§9 tabla de equivalentes ASCII,
§20.2), pendiente de que el usuario lo pida explícitamente.

### Addendum — opción de CLI `-I` (directorio de inclusión) (2026-08-02)

A petición del usuario: `incluye "ruta.nemi"` solo resolvía relativo al
directorio del archivo que la escribe (`Nemi.md` §10/§12, sin cambios — es
puramente una opción de la herramienta CLI, no del lenguaje), lo cual ya era
un punto de fricción documentado en `bibcom/README.md` y
`Windows/installer/README.md` (un script guardado fuera de la carpeta de
instalación no encontraba `bibcom/` sin copiarla o escribir una ruta
relativa larga de vuelta). Se agregó `-I DIR` (repetible) a las tres CLI:
si el archivo no está junto al que lo incluye, se prueba cada `DIR` de `-I`
en el orden dado. Cambio puramente aditivo (parámetro nuevo con default
vacío en `tokenize_with_includes`/`tokenize_file_with_includes`/`load`/
`load_files`, en las tres implementaciones) — ninguna llamada existente
(`WinNemi.cpp` incluida) necesitó tocarse. `Windows/src/loader.cpp`,
`Windows/src/nemi.cpp`, `Windows/include/nemi/{loader,nemi}.hpp` y
`Windows/apps/main.cpp` son copias idénticas de `cpp/`, así que se portaron
por copia directa (sin fusión) una vez verificados ahí; `Windows/apps/
WinNemi.cpp`/`.h`/`.rc` (la GUI) no se tocaron — no tiene hoy superficie de
CLI para `-I`. Fixture nueva: `examples/incluye_dir_externo/triple.nemi` +
`examples/usa_incluye_dir.nemi`. Verificado en las tres implementaciones el
mismo día: **92/92** en Python, **87/87** en cada C++ (dos casos nuevos cada
una), más prueba manual de las tres CLI con `-I` (con y sin la ruta,
resultado correcto en ambos). Documentado en `README.md`/`cpp/README.md`/
`Windows/README.md` (uso del CLI), `bibcom/README.md` y
`Windows/installer/README.md` (como alternativa a copiar `bibcom/`), y
`manual/10_biblioteca_comun.md`. `Nemi.md` no se tocó (opción de CLI, no de
lenguaje).

### Addendum — WinNemi: `bibcom/` implícito + rutas de inclusión editables (2026-08-02)

A petición del usuario, sobre `WinNemi.exe` (la GUI, que no tiene `-I` por no
tener línea de comandos): (1) ahora detecta sola una carpeta `bibcom/` junto
a su propio `.exe` (`WinNemiStateImplicitBibcomDir`, vía
`GetModuleFileNameW` + `GetFileAttributesW`), sin configurar nada; (2) nuevo
menú **Ejecutar → Rutas de inclusión...** (`IDM_INCLUDE_PATHS`) abre un
diálogo (`WinNemiIncludeDlg.h`/`.cpp`, recurso `IncludesDlg` en `WinNemi.rc`)
donde se agregan carpetas propias con un selector de carpeta
(`SHBrowseForFolderW`) o se quitan de una lista — para que un estudiante
tenga su propia biblioteca de funciones reutilizables, no solo `bibcom/`.
La lista persiste en `WinNemi.ini` (mismo archivo que ya usa
`WinNemiState.cpp` para posición/maximizado), sección `[Incluye]`, clave
`Rutas=` separada por `;`. Orden de resolución al "Ejecutar" (`RunCurrentBuffer`
en `WinNemi.cpp`): directorio del propio archivo (sin cambios) → rutas
configuradas por el usuario, en el orden de la lista → `bibcom/` implícito,
al final — mismo criterio "lo tuyo gana" que ya se usó para `-I`. El
instalador (`WinNemi.iss`, sección `[INI]` nueva) siembra `WinNemi.ini` con
`bibcom/` como primera entrada visible, no porque haga falta para que
resuelva (ya lo cubre la detección implícita) sino para que el diálogo no
aparezca vacío la primera vez y el estudiante entienda cómo agregar la suya.
Build limpio con MSVC, `ctest` sigue en verde (no se tocó `nemi`/
`corpus_test`); verificado end-to-end vía introspección Win32 (sin captura
de pantalla en este entorno): ejecución real de un script sin ruta guardada
que resuelve `triple.nemi` vía la carpeta configurada y `mcd` vía `bibcom/`
implícito, diálogo abierto/cerrado con Aceptar/Cancelar confirmando
persistencia correcta en ambos casos (Cancelar descarta, Aceptar guarda de
inmediato), y una segunda ejecución tras quitar la carpeta configurada
fallando con el mismo mensaje de error de siempre. Documentado en
`Windows/README.md` y `Windows/installer/README.md`. `Nemi.md` no se tocó
(feature de la app `WinNemi.exe`, no del lenguaje).

### Addendum — WinNemi: "Ayuda" abre el manual (Markdown → HTML con pandoc) (2026-08-02)

A petición del usuario: `IDM_HELP` era un placeholder ("Ayuda no disponible
todavía."); el manual para principiantes (`manual/*.md`, 10 capítulos +
referencia rápida) existía pero no estaba enlazado desde ningún lado.
Nuevo `Windows/devtools/manual_to_html.py` (mismo patrón que
`ascii_to_bmp.py`) convierte cada `manual/*.md` a HTML autocontenido vía
`pandoc` (probado con 3.8.2.1), reescribiendo los enlaces al mismo
directorio (`01_que_es_un_programa.md` → `.html`, etc.) — los dos enlaces
que salen de `manual/` (`../Nemi.md`, `../bibcom/README.md`) se dejan tal
cual, esos archivos no se convierten (fuera de alcance: son referencia
técnica, no material del tutorial). `Windows/devtools/manual.css` da un
estilo mínimo, incrustado en cada `.html` con `--embed-resources`. El
resultado (`manual/html/*.html`) se versiona en el repo, igual que
`toolbar_std.bmp` — se regenera a mano, `make_installer.bat` no lo toca.
`WinNemiState.h`/`.cpp` ganó `WinNemiStateExeDir()` (export delgado del
`ExeDir()` que ya existía, privado, reutilizado ahora por dos features);
`WinNemi.cpp`'s `IDM_HELP` arma `manual\00_indice.html` junto al `.exe` y lo
abre con `ShellExecuteW` (en el navegador predeterminado, no en un control
embebido — mismo estilo clásico ligero del resto de la app), o muestra un
aviso si la carpeta no está (dev build sin `manual/html/` copiado a mano, o
instalación corrupta) en vez de fallar. `Windows/installer/WinNemi.iss`
empaqueta `manual/html/*.html` en `{app}\manual`, mismo patrón que
`examples/`/`bibcom/`. Build limpio con MSVC, `ctest` sigue en verde (no se
tocó `nemi`/`corpus_test`); verificado end-to-end vía introspección Win32
(sin captura de pantalla en este entorno): ejecución real de `IDM_HELP` con
`manual/html/` copiado junto al `.exe` de desarrollo, confirmando que
aparece un proceso de navegador nuevo y que `WinNemi.exe` sigue respondiendo
después (sin colgarse), y que no aparece la ventana de "no encontrado".
Documentado en `Windows/devtools/README.md` y `Windows/README.md`.
`Nemi.md` no se tocó (feature de la app `WinNemi.exe`, no del lenguaje).

### Addendum — localización al español de lexer/parser/runtime (2026-08-02)

A petición del otro colaborador (`Notas.md`): los mensajes de error del
lexer, parser y runtime seguían en inglés en las tres implementaciones,
mientras que `incluye`/CLI/`afirma` ya estaban en español. Se tradujo la
tabla completa que trajo la nota (~48 mensajes) en `python/nemi/{lexer,
parser,interpreter,values}.py`, `cpp/src/{lexer,parser,interpreter,value}.cpp`
+ `cpp/include/nemi/{array,environment}.hpp` (dos mensajes vivían ahí, no en
`interpreter.cpp` como asumía la nota) y sus espejos en `Windows/` — vía un
script de reemplazo de subcadena exacta con verificación de conteo (no
`sed`/regex), revisado línea por línea antes de correr las pruebas. Un
mensaje adicional que la nota no traía (`unknown comparison` en Python, sin
equivalente en C++) se tradujo igual, mismo patrón que "operador unario/
binario desconocido". Bug real encontrado al portar a C++: `to_string(
TokenKind)` devuelve `const char*`, así que las dos reordenaciones del
parser (mover el nombre del token de sufijo a prefijo) necesitaron un
`std::string(...)` explícito que no hacía falta en el texto original.
Las pruebas de las tres suites no necesitaron ningún cambio (ninguna
compara el texto en inglés traducido). Actualizadas las 4 citas de mensajes
en `manual/{referencia_rapida,04_repeticion,06_arreglos,08_verificacion}.md`
(la última se corrigió después, ver el punto siguiente) y el typo de acento
en `03_decisiones.md`. Respondido en `Notas.md` para el otro colaborador.

**Seguimiento el mismo día:** el sufijo de ubicación `"line N, col M"` que
se agrega a *todo* error (incluidos los que ya estaban en español) seguía
en inglés — se dejó fuera a propósito en la ronda anterior, pero al
preguntárselo directamente al usuario se decidió que no había razón real
para mantenerlo así. Cambiado a `archivo, N:M` (más corto, sin necesidad de
traducir, y ya existía como convención en `SourceLocation.__repr__` de
Python, solo que no se usaba en el mensaje visible) en
`python/nemi/errors.py` y `cpp/include/nemi/source_location.hpp` +
su espejo en `Windows/` (archivo idéntico, confirmado por diff). Verificado:
Python 92/92, `ctest` en verde en `cpp/build` y `Windows/build`, salida real
de CLI confirmada en las tres implementaciones. `manual/08_verificacion.md`
(única cita restante del formato viejo) actualizada y HTML regenerado.

### Addendum — acentos en `unión`/`intersección` (2026-08-03)

El usuario reportó que las primitivas de conjuntos `union`/`interseccion`
deberían llevar acento. Confirmado con un agente Explore + verificación
directa: son primitivas reales (no solo prosa), registradas en las tres
tablas de despacho; las hermanas `pertenece`/`subconjunto`/`diferencia`/
`cardinalidad` ya estaban bien. Arreglado **de forma aditiva**, siguiendo un
patrón ya existente en el propio código para `raíz`/`raiz` (√, verificado en
`cpp/src/interpreter.cpp:490-491` y su espejo en `Windows/`, y en
`python/nemi/interpreter.py:787-788`): las tablas de primitivas ganan las
llaves `unión`/`intersección` (canónicas, usadas también en los mensajes de
error internos de aridad/tipo) apuntando a las mismas funciones, sin quitar
`union`/`interseccion` como alias — así ningún código existente (`bibcom/`,
tareas de estudiantes) se rompe. Actualizados a la forma acentuada como la
enseñada: `Nemi.md` (tabla §20.2, prosa, el listado de `conjuntos.nemi`
citado en §22.3, y §23), `bibcom/conjuntos.nemi` (debe quedar idéntico a lo
que cita `Nemi.md`), `bibcom/README.md`, `manual/07_conjuntos.md`,
`manual/referencia_rapida.md`, `para-llm-mates-discretas.md`. Las menciones
en `backlog_v0.2.md` de fases anteriores (Fase 1, addendum de `en`) se
dejaron tal cual — son registro histórico de lo que era cierto en su
momento, no documentación viva. Se agregó un checkeo nuevo por cada
implementación confirmando que la forma acentuada funciona, junto a las
pruebas ya existentes de `union`/`interseccion` (que siguen pasando sin
cambios gracias al alias). Verificado: Python 94/94, `cpp/build` 89/89,
`Windows/build` 89/89 — los tres en verde; prueba manual de CLI en las tres
implementaciones confirmando ambas grafías y que el mensaje de error de
aridad usa la forma acentuada sin importar cuál se haya escrito. Un detalle
del entorno, no del código: el build de `Windows/` chocó de nuevo con el bug
ya conocido del enlazador incremental de MSVC (`nemi.exe` quedó desactualizado
pese a que el log decía éxito) — mismo arreglo de siempre, borrar el `.exe` y
forzar el enlace completo. HTML regenerado (`docs/Nemi.html`,
`docs/manual/{07_conjuntos,referencia_rapida}.html`).

---

## Riesgos / notas

- **Tres implementaciones que mantener en paralelo** es el mayor costo de
  este trabajo, no la dificultad de cada feature en sí — cada fase son en
  realidad 3 ports (Python, `cpp/`, `Windows/`), como ya pasó con
  `incluye`/`imprime`.
- **`∅` (Fase 1):** la spec ya decidió aceptar el doble uso (`Nemi.md` §20.1)
  — no hay que volver a discutirlo, solo implementarlo con cuidado.
- **`para cada`/`cada`/`en` (Fase 2):** revisar el corpus §17 y los ejemplos
  existentes en `examples/` por si algún identificador se llama `cada` o
  `en` antes de reservarlos como palabra clave (poco probable, pero barato
  de verificar con un `grep`).
- **Orden de fases:** 1–3 y 6 no tienen dependencias entre sí y podrían
  hacerse en cualquier orden (o en paralelo); 4 (`afirma`) es independiente
  de 1–3 también. Solo la Fase 7 depende de que 1–6 estén completas.
