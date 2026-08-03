# Guía de portación a C++ (paso a paso, para principiantes)

> Esta guía cubre solo el **núcleo v0.1** (Fases 0–6 de `backlog.md`); el
> port de la biblioteca común v0.2 (`Conjunto`, `afirma`/`traza`, `bibcom/`,
> …) que vino después no tiene una guía paso a paso equivalente — su
> historia fase por fase, con decisiones y bugs reales encontrados, está en
> [`../backlog_v0.2.md`](../backlog_v0.2.md).

Esta guía acompaña al [`backlog.md`](backlog.md). Su objetivo es que **alguien
con poca experiencia** (o un asistente automático sencillo) pueda completar el
intérprete de Nemi en C++ traduciendo, pieza por pieza, la implementación de
referencia en Python (`../python/nemi/`).

> **Regla de oro:** por cada archivo de C++ que edites, ten abierto al lado el
> archivo `.py` equivalente. La estructura es casi 1:1. Cuando dudes de una
> regla del lenguaje, la fuente de verdad es [`../Nemi.md`](../Nemi.md).

## Contenido
- [0. Preparación](#0-preparación)
- [1. El panorama y las estructuras de datos](#1-el-panorama-y-las-estructuras-de-datos)
- [2. Cómo trabajar y verificar (incremental)](#2-cómo-trabajar-y-verificar-incremental)
- [Fase 1 — Lexer](#fase-1--lexer-srclexercpp)
- [Fase 2 — Parser](#fase-2--parser-srcparsercpp)
- [Fase 3 — Evaluador](#fase-3--evaluador-srcinterpretercpp)
- [Fase 4 — Números grandes (bignum)](#fase-4--números-grandes-bignum)
- [Fase 5 — Paridad con Python](#fase-5--paridad-con-python)
- [Fase 6 — Pruebas y cierre](#fase-6--pruebas-y-cierre)
- [Apéndice: errores comunes](#apéndice-errores-comunes)

---

## 0. Preparación

Necesitas: un compilador de C++17 (clang++ o g++ ≥ 7), CMake y, opcionalmente,
Ninja. Comprueba que el esqueleto compila **antes** de tocar nada:

```console
$ cd cpp
$ cmake -S . -B build -G Ninja
$ cmake --build build
$ ctest --test-dir build            # debe pasar (todo en PENDING)
```

Ten a mano la versión Python para comparar salidas:

```console
$ cd ../python
$ python -m nemi ../examples/maximo.nemi --lexemas   # verás los tokens esperados
```

---

## 1. El panorama y las estructuras de datos

Un intérprete es una tubería de tres etapas. Cada etapa transforma una
estructura de datos en la siguiente:

```
  texto  ──Lexer──▶  vector<Token>  ──Parser──▶  AST (árbol)  ──Interpreter──▶  Value
 (string)             (arreglo)                   (punteros)                    (resultado)
```

Como estudiante de **Estructuras de datos**, reconocerás cada pieza:

| Concepto del curso | Dónde aparece en Nemi |
|---|---|
| **Arreglo dinámico** (`std::vector`) | la lista de tokens que produce el lexer |
| **Árbol** | el AST; cada nodo tiene hijos (`unique_ptr<Expr>`) |
| **Recursión** | el parser (descenso recursivo) y las funciones Nemi recursivas |
| **Pila (stack)** | la pila de llamadas del intérprete (una `Environment` por llamada) |
| **Tabla hash / diccionario** | `Environment` (`unordered_map<string, Value>`) y las tablas de palabras clave |
| **Matriz** | el algoritmo de Warshall (`Array` de `Array`) |
| **Variante etiquetada** (`std::variant`) | `Value` (un valor que puede ser entero, real, cadena, arreglo…) |

Antes de escribir código, **lee** estos archivos ya terminados para entender los
tipos con los que trabajarás (son cortos):
- `include/nemi/token.hpp` — qué es un `Token`.
- `include/nemi/ast.hpp` — los nodos del árbol y el patrón *visitor*.
- `include/nemi/value.hpp` y `include/nemi/array.hpp` — los valores en ejecución.

---

## 2. Cómo trabajar y verificar (incremental)

**No intentes hacerlo todo de golpe.** El truco: el archivo de pruebas
`tests/corpus_test.cpp` corre el corpus y, para cada caso que aún no funciona,
imprime **PENDING** con el mensaje del stub que se disparó. Ese mensaje es tu
brújula:

- Al principio verás: `PENDING … TODO: implement Lexer::tokenize`.
- Cuando termines el lexer y el parser, el mensaje **cambiará** a
  `TODO: implement Interpreter::visit(...)`. ¡Eso significa que avanzaste!
- Cuando termines un `visit`, algunos casos pasarán de **PENDING** a **PASS**.

Ciclo de trabajo:
```console
$ cmake --build build && ctest --test-dir build --output-on-failure
```
Repite: edita un pedacito → compila → mira qué mensaje PENDING cambió o qué caso
pasó a PASS. Marca la casilla correspondiente en `backlog.md`.

---

## Fase 1 — Lexer (`src/lexer.cpp`)

**Objetivo:** implementar `Lexer::tokenize()`, que convierte el texto fuente en
un `std::vector<Token>` terminado en un token `Eof`.

**Idea clave:** recorres el texto **una sola vez** con un cursor (un índice).
En cada paso miras el carácter actual y decides qué tipo de token empieza ahí.

### 1.1 Problema del UTF-8 (leélo primero)
Nemi usa símbolos como `←`, `≤`, `√` y letras acentuadas (`función`). En UTF-8
esos caracteres ocupan **varios bytes**. Para no pelear con bytes, convierte
todo el texto a *code points* (`char32_t`) **una vez** al inicio, y trabaja
sobre un `std::u32string`. Copia estos dos ayudantes al principio de
`lexer.cpp` (son mecánicos, no es lo que se evalúa):

```cpp
// UTF-8 (bytes) -> code points
static std::u32string decode_utf8(const std::string& b) {
    std::u32string out; size_t i = 0;
    while (i < b.size()) {
        unsigned char c = b[i]; char32_t cp; int len;
        if (c < 0x80)      { cp = c;        len = 1; }
        else if (c < 0xE0) { cp = c & 0x1F; len = 2; }
        else if (c < 0xF0) { cp = c & 0x0F; len = 3; }
        else               { cp = c & 0x07; len = 4; }
        for (int k = 1; k < len && i + k < b.size(); ++k)
            cp = (cp << 6) | (b[i + k] & 0x3F);
        out.push_back(cp); i += len;
    }
    return out;
}
// code points -> UTF-8 (para guardar el texto de un identificador/cadena en el Token)
static std::string encode_utf8(const std::u32string& s) {
    std::string out;
    for (char32_t cp : s) {
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0|(cp>>6)); out += (char)(0x80|(cp&0x3F)); }
        else if (cp < 0x10000) { out += (char)(0xE0|(cp>>12)); out += (char)(0x80|((cp>>6)&0x3F)); out += (char)(0x80|(cp&0x3F)); }
        else { out += (char)(0xF0|(cp>>18)); out += (char)(0x80|((cp>>12)&0x3F)); out += (char)(0x80|((cp>>6)&0x3F)); out += (char)(0x80|(cp&0x3F)); }
    }
    return out;
}
```

En el constructor del lexer (en `lexer.hpp`) guarda `u32src_ = decode_utf8(source)`
y añade un índice `size_t pos_`, y `int line_ = 1, col_ = 1;`.

### 1.2 Las tablas (cópialas: son datos, no lógica)
Traducidas directamente de `_KEYWORDS`, `_TWO_CHAR` y `_ONE_CHAR` de `lexer.py`:

`KEYWORDS` va con claves `std::string` (UTF-8), porque `word_token` convierte la
palabra a `std::string` de todos modos (ver 1.4). `TWO_CHAR`/`ONE_CHAR` operan
sobre *code points* crudos, antes de decidir codificar nada:

```cpp
using K = TokenKind;
static const std::unordered_map<std::string, K> KEYWORDS = {
    {"función",K::Function},{"funcion",K::Function},{"procedimiento",K::Procedure},
    {"para",K::For},{"hasta",K::To},{"repite",K::Repeat},{"mientras",K::While},
    {"si",K::If},{"entonces",K::Then},{"alt",K::Else},{"alternativamente",K::Else},
    {"regresa",K::Return},{"fin",K::End},{"mod",K::Mod},
    {"y",K::And},{"o",K::Or},{"no",K::Not},   // ← solo español (nada de and/or/not)
};
static const std::unordered_map<std::u32string, K> TWO_CHAR = {
    {U"<-",K::Assign},{U"!=",K::Ne},{U"<=",K::Le},{U">=",K::Ge},
};
static const std::unordered_map<char32_t, K> ONE_CHAR = {
    {U'←',K::Assign},{U'=',K::Eq},{U'≠',K::Ne},{U'<',K::Lt},{U'≤',K::Le},
    {U'>',K::Gt},{U'≥',K::Ge},{U'+',K::Plus},{U'−',K::Minus},{U'-',K::Minus},
    {U'·',K::Times},{U'*',K::Times},{U'/',K::Divide},{U'∧',K::And},{U'∨',K::Or},
    {U'¬',K::Not},{U'(',K::LParen},{U')',K::RParen},{U'[',K::LBracket},{U']',K::RBracket},
    {U'⌊',K::LFloor},{U'⌋',K::RFloor},{U'⌈',K::LCeil},{U'⌉',K::RCeil},{U'√',K::Sqrt},{U',',K::Comma},
};
```

### 1.3 Predicados sencillos
`std::isalpha` no sirve para code points Unicode. **Cuidado con la tentación de
usar `c >= 0x80` para decir "es letra":** los símbolos operadores de Nemi
(`←`, `≤`, `√`, `⌊`, …) **también** son code points `≥ 0x80` — con esa regla,
`is_letter` se los "tragaría" en `word_token` antes de que la tabla de
operadores llegue a verlos, y `≤` dejaría de reconocerse. La solución correcta
es una **lista explícita** de las letras acentuadas que sí usa el español:

```cpp
static bool is_digit(char32_t c){ return c >= U'0' && c <= U'9'; }
static bool is_letter(char32_t c){
    if ((c>=U'a'&&c<=U'z') || (c>=U'A'&&c<=U'Z') || c==U'_') return true;
    static const std::u32string acentuadas = U"áéíóúüñÁÉÍÓÚÜÑ";
    return acentuadas.find(c) != std::u32string::npos;
}
```

### 1.4 El bucle principal (traduce `tokenize` / `_skip_trivia` / `_next_token`)
Pseudocódigo (compáralo con `lexer.py`):

```
tokenize():
    tokens = []
    repite:
        salta espacios y comentarios   # ' ' \t \r \n ; y desde '#' o '▷' hasta fin de línea
        si llegaste al final: agrega Token(Eof); regresa tokens
        tokens.push_back( next_token() )

next_token():
    guarda la ubicación (line_, col_) de inicio
    c = carácter actual
    si c == '"':            regresa string_token()
    si c == '«':            regresa prose_token()
    si is_digit(c):         regresa number_token()
    si is_letter(c):        regresa word_token()
    # operadores: primero prueba de 2 caracteres, luego de 1
    par = substring de 2 code points desde pos_
    si par está en TWO_CHAR: avanza 2; regresa Token(TWO_CHAR[par])
    si c está en ONE_CHAR:   avanza 1; regresa Token(ONE_CHAR[c])
    error léxico: "unexpected character"
```

Detalles de cada escáner (todos avanzan el cursor code point a code point):
- **`number_token`**: acumula dígitos. Si viene `'.'` seguido de dígito, sigue y
  produce `Token(Real, valor)` con `std::stod`; si no, `Token(Int, valor)` con
  `std::stoll` (más adelante, con bignum, será tu tipo `Integer`).
- **`word_token`**: acumula `is_letter`/`is_digit`. Convierte a `std::string` con
  `encode_utf8`. Si la palabra está en `KEYWORDS`, produce ese `TokenKind`
  (sin valor). Si no, `Token(Ident, palabra)`.
- **`string_token`**: consume la `"` inicial, acumula hasta la `"` final (soporta
  `\"` y `\\` opcionalmente), guarda el texto (UTF-8) en `Token(String, …)`.
- **`prose_token`**: consume `«`, acumula hasta `»`, guarda `Token(Prose, texto)`.
- **comentarios**: al ver `#` o `▷`, avanza hasta el fin de línea.

> **No olvides** actualizar `line_`/`col_` al avanzar: si el carácter es `'\n'`,
> `line_++, col_=1`; si no, `col_++`. Así los errores dirán la línea correcta.

### 1.5 Cómo verificar la Fase 1
```console
$ cmake --build build
$ ./build/nemi ../examples/maximo.nemi --tokens      # (opción en inglés por ahora)
Function
Ident
LParen
...
```
Compara con la versión Python (`python -m nemi ../examples/maximo.nemi --lexemas`).
Deben coincidir los tipos. En `corpus_test`, los mensajes PENDING deben dejar de
mencionar `Lexer::tokenize`: la mayoría pasan a `Parser::parse_expression` (cada
caso de valor evalúa su expresión con `run_call`, que tiene su propio lexer+parser
independiente de `load_files`); los dos casos que llaman directo a la API C++
(`inserción_por_orden`, `Warshall`) seguirán en "front end not ported yet" hasta
que **también** el parser esté (porque esos dependen de que `load_files` cargue
el corpus completo, que usa `parse_program`). Si ves ese cambio, ¡el lexer
funciona!

---

## Fase 2 — Parser (`src/parser.cpp`)

**Objetivo:** convertir el `vector<Token>` en un **árbol** (`Program`, con
`definitions` y `main`). Técnica: **descenso recursivo** — una función por regla
de la gramática (§10).

**Idea clave:** cada función "consume" los tokens que le tocan y **devuelve un
nodo** (`unique_ptr<...>`). Si algo no cuadra, lanza `ParseError`.

### 2.1 Cursor (cópialo; traduce los helpers de `parser.py`)
```cpp
const Token& peek() const { return tokens_[i_]; }
const Token& advance() { const Token& t = tokens_[i_]; if (t.kind!=TokenKind::Eof) ++i_; return t; }
bool check(TokenKind k) const { return peek().kind == k; }
bool match(TokenKind k) { if (check(k)) { advance(); return true; } return false; }
const Token& expect(TokenKind k, const char* what) {
    if (!check(k)) throw ParseError(std::string("expected ")+what, peek().location);
    return advance();
}
```

### 2.2 Nivel superior y sentencias
```
parse_program():
    Program p
    mientras no sea Eof:
        si peek es Function o Procedure: p.definitions.push_back(definition())
        si no:                          p.main.push_back(statement())
    regresa p

statement():   # despacha según el primer token (ver _statement en parser.py)
    For      -> for_loop()
    While    -> while_loop()
    If       -> if_stmt()
    Return   -> return_stmt()
    Prose    -> nodo Prose
    Ident    -> assign_or_call()
    otro     -> ParseError
```

Fíjate en `assign_or_call()`: lee un `Ident`; si sigue `(` es una **llamada**
(sentencia de expresión); si no, es una **asignación** (`lugar ← expr`, donde
`lugar` puede llevar índices `[..]`).

En `if_stmt()`, tras el bloque `then`, si `match(Else)` (la palabra `alt`) lee el
bloque alternativo. Un bloque termina cuando `peek().kind` es `End`, `Else` o `Eof`.

Para construir nodos usa `std::make_unique`:
```cpp
auto node = std::make_unique<Binary>();
node->op = op; node->left = std::move(left); node->right = std::move(right);
```

### 2.3 La torre de expresiones (lo más importante)
Traduce estas funciones **en este orden** (de menor a mayor precedencia). Cada
una llama a la siguiente:

```
expression()  -> logic_or()
logic_or()    : izq = logic_and();  mientras Or:  izq = Binary(Or, izq, logic_and())
logic_and()   : izq = logic_not(); mientras And: izq = Binary(And, izq, logic_not())
logic_not()   : si Not: return Unary(Not, comparison());  si no: comparison()
comparison()  : izq = sum();  si viene un comparador (= ≠ < ≤ > ≥): return Binary(op, izq, sum())
sum()         : izq = product(); mientras + o −: izq = Binary(op, izq, product())
product()     : izq = unary();   mientras · / mod: izq = Binary(op, izq, unary())
unary()       : si − o ¬: return Unary(op, postfix());  si no: postfix()
postfix()     : e = primary(); mientras '[': e = Index(e, expression()) tras ']'
primary()     : número | cadena | '[' arreglo ']' | Ident (Variable o Call) |
                '(' expresión ')' | ⌊ e ⌋ | ⌈ e ⌉ | √ unary()
```

> **Por qué este orden importa (concepto):** al poner `+`/`−` en un nivel
> "más abajo" que `·`/`/`, garantizas que `a + b · c` se agrupe como
> `a + (b · c)`. Es exactamente la **precedencia de operadores** que ves en
> matemáticas. La asociatividad izquierda sale del bucle `mientras`.

### 2.4 Cómo verificar la Fase 2
Aún no hay evaluador, así que la señal es indirecta pero clara: al recompilar y
correr `ctest`, **los 15 casos** (ya no solo 13) deben mostrar mensajes PENDING
de `Interpreter::visit(...)` — incluidos ahora los dos que llaman a la API C++
directamente (`inserción_por_orden`, `Warshall`), que antes decían "front end
not ported yet" porque dependían de que `load_files` (y por tanto
`parse_program`) funcionara. Si los 15 muestran `visit(...)`, el lexer **y** el
parser ya funcionan de punta a punta sobre todo el corpus.

---

## Fase 3 — Evaluador (`src/interpreter.cpp`)

**Objetivo:** rellenar los 10 métodos `visit(...)` que hoy llaman a `todo(...)`.
La maquinaria (pila de llamadas, `regresa`, literales) ya está hecha: úsala.

**Cómo funciona el "registro de resultado":** los `visit` de expresión no
devuelven nada; en su lugar **guardan** el resultado en `result_`. El helper
`eval(expr)` hace `expr.accept(*this); return result_;`. Entonces, dentro de un
`visit`, para obtener el valor de un sub-nodo escribes `Value v = eval(*hijo);`
y al final `result_ = ...;`.

Traduce cada método desde `interpreter.py` (nombres casi idénticos):

1. **`visit(ArrayLiteral)`** → construye un `Array` evaluando cada elemento;
   `result_ = std::make_shared<Array>(items);`.
2. **`visit(Index)`** → `eval(base)`, `eval(index)`; si es `Array` usa `get`
   (base-1); si es cadena, devuelve el carácter (recuerda base-1).
3. **`visit(Unary)`** → según `op`: `Minus` (negar), `Not` (negación lógica),
   `Sqrt`, `LFloor` (¡caso especial `⌊√n⌋` exacto con `isqrt` para enteros!),
   `LCeil`.
4. **`visit(Binary)`** → **primero** `And`/`Or` con **corto-circuito** (evalúa el
   izquierdo; solo evalúa el derecho si hace falta). Luego el resto: `+ − · /`,
   `mod`, comparaciones. Reglas finas (mira `interpreter.py`):
   - `+`/`·`: si **ambos** operandos son `bool`, son OR/AND; si no, aritméticos.
   - `∧`/`∨`/`¬` "preservan el tipo": si algún operando es entero (bit 0/1),
     devuelven bit `0/1`; si son `bool`, devuelven `bool` (helper `logic_result`).
   - `/` entre dos enteros = **racional exacto** (no `double`); ver Fase 4.
   - división/`mod` por cero → `ExecutionError`.
5. **`visit(Assign)`** → si no hay índices, `env_->define(nombre, valor)`; con
   índices, navega el `Array`/matriz hasta la celda y usa `set`.
6. **`visit(ForLoop)`** → `i = eval(start)`; `mientras i <= end`: define la
   variable, ejecuta el cuerpo, `i = i + 1` (inclusive, paso +1).
7. **`visit(WhileLoop)`** → `mientras is_truthy(eval(cond))`: ejecuta el cuerpo.
8. **`visit(If)`** → si `is_truthy(cond)` ejecuta `then_body`, si no y hay
   `else_body`, ejecútalo.
9. **`visit(ExprStatement)`** → evalúa la llamada y **descarta** el resultado.
10. **`visit(Call)`** — el más grande. En orden:
    - si el nombre es `intercambia`: caso especial que intercambia dos
      *l-values* (variables/celdas) — mira `_swap`/`_get_lvalue`/`_set_lvalue`.
    - evalúa los argumentos; si el nombre es una función de usuario, llama a
      `invoke(def, args)` (ya existe); si es una primitiva
      (`piso/techo/raíz/abs/long/mod/imprime`), ejecútala.

> **Consejo:** implementa en este orden y verás casos pasar a PASS pronto:
> `Binary`+`If`+`Return`+`Call` te dan `factorial`/`fib`; añade `ForLoop`+`Assign`
> para `factorial_iter`/`máximo`; `Index` para `busca_texto`/`máximo`; el
> intercambio para `mcd`; y `∧`/`∨` para `warshall`/`insercion`.

### Cómo verificar la Fase 3
Cada `visit` que completes hace que uno o más casos del corpus pasen de PENDING
a **PASS**. Meta de la fase: los 13 casos de valor + los 2 de procedimiento en
verde (los que caben en 64 bits) — **verificado: los 15 pasan** con la
implementación de referencia.

---

## Fase 4 — Números grandes (bignum)

Los enteros de Nemi son de **precisión arbitraria** (§8): `factorial(100)` tiene
158 dígitos. Antes de esta fase, `include/nemi/numeric.hpp` usaba `long long`
(se desborda) y un `Rational` de mentira.

**Decisión tomada en la implementación de referencia: un `BigInt` propio, desde
cero** (`include/nemi/bigint.hpp` + `src/bigint.cpp`), en vez de Boost.Multiprecision
o GMP. Motivo: cero dependencias externas (el proyecto sigue compilando con solo
un compilador C++17 y CMake — nada de red ni de gestor de paquetes durante
`cmake configure`), y de paso es un ejercicio genuino de estructuras de datos
(vector dinámico de "dígitos" en base 10⁹, suma/resta con acarreo, multiplicación
escolar, división larga). Si prefieres una librería externa, Boost.Multiprecision
(`cpp_int`/`cpp_rational`, vía CMake `FetchContent`) o GMP (`mpz_class`/`mpq_class`)
siguen siendo alternativas válidas — solo cambiarías `numeric.hpp` y
`bigint.hpp`/`.cpp`; el resto del intérprete no se entera.

Representación elegida: signo + `std::vector<uint32_t>` en base 1,000,000,000
(1e9), little-endian. La base 1e9 (en vez de, p. ej., base 2³²) hace que
parsear/formatear decimal sea trivial (cada "dígito" del vector YA son 9 dígitos
decimales, sin conversión de base), y que `limb*limb` (hasta ~1e18) quepa
holgadamente en un acumulador `uint64_t` durante la multiplicación escolar.

Piezas necesarias en `BigInt`: constructor desde `long long` (**impícito** —
así `x + 1`, `n - 1`, `i == 0` funcionan sin envolver todo en `Integer(...)`,
igual que el `int` de Python) y desde una cadena de dígitos decimales
(`from_decimal`, para el lexer); `+ − * / %`; **división/módulo de piso**
(`floor_div`/`mod_floor`, con la semántica de Python: el resto tiene el signo
del divisor) separadas de la división/resto truncantes (estilo C++) que sirven
de base; comparaciones; `to_decimal()`/`to_double()`; `isqrt` (Newton) y `gcd`
(Euclides, necesario para normalizar `Rational`).

> ⚠️ **Bug real que apareció al implementar esto (déjate enseñar por él):** la
> primera versión de `isqrt` sembraba la iteración de Newton así:
> `BigInt x(static_cast<long long>(std::sqrt(n.to_double())) + 1);` — para una
> `n` cuya raíz tiene más de ~19 dígitos, `std::sqrt(n.to_double())` da un
> `double` que **no cabe en `long long`**, y convertir un `double` fuera de
> rango a un entero con signo es **comportamiento indefinido** en C++. El
> resultado no era un crash limpio: era una semilla basura que dejaba al bucle
> de corrección final (`while ((x+1)*(x+1) <= n) x += 1;`, que avanza de a uno)
> intentando subir de casi cero hasta un número de 31 dígitos — en la práctica,
> un **cuelgue indefinido**, no un error. La lección: nunca hagas pasar el
> tamaño de un bignum por un tipo nativo de 64 bits, ni siquiera "solo para
> sembrar una estimación". La solución fue construir la semilla directamente
> como potencia de diez a partir de la **cantidad de dígitos decimales** de
> `n` (si `n` tiene `d` dígitos, `10^⌈d/2⌉ ≥ √n` siempre) — sin pasar nunca por
> `double`/`long long`. Si implementas tu propio `isqrt`, ten cuidado con este
> mismo patrón.

**Añade un caso de estrés** a `tests/corpus_test.cpp` que compruebe
`factorial(100)` contra su valor exacto (cópialo, con cuidado — es un literal de
158 dígitos y es fácil trastocar un dígito al partirlo en varias líneas de C++;
verifícalo programáticamente, no a ojo, contra el mismo literal usado en
`python/tests/run_corpus.py`).

> **Por qué importa el `Rational`:** `⌊n / 2⌋` (en `exp_rápida`/`exp_mod`) debe
> ser exacto para enteros enormes. Si usas `double`, pierdes precisión y RSA
> falla. Por eso `/` entre enteros produce un racional exacto, y `⌊⌋` recupera el
> entero.

### Cómo verificar la Fase 4
`ctest` debe seguir en verde, y el caso de estrés `factorial(100)` debe pasar
(158 dígitos exactos). Prueba también a mano: `⌊n/2⌋` para una `n` de más de 30
dígitos debe coincidir dígito a dígito con la misma cuenta en la versión Python.

---

## Fase 5 — Paridad con Python  ✅ (hecho, 2026-07-26)

1. **Instrucciones de nivel superior (cuerpo de guión).** El AST ya traía
   `Block main;` desde la Fase 2. Se añadió `Interpreter::run_program()`
   (declarado en `interpreter.hpp`, implementado en `interpreter.cpp` justo
   después de `has_function`): ejecuta `program_.main` en una `Environment`
   nueva, guardando/restaurando `env_` (mismo patrón que `evaluate`/`invoke`)
   y atrapando `ReturnSignal` para que un `regresa` suelto simplemente termine
   el guión. `load_files`/`load` no necesitaron cambios: como `nemi.cpp`
   concatena el **texto fuente** de todos los archivos antes de parsear, el
   `main` fusionado sale gratis.
2. **CLI en español** (`apps/main.cpp`): `--call/--tokens/--ast` →
   `--llama/--lexemas/--asa`, más `-a/--ayuda`; se llama `run_program()` tras
   cargar, antes del bucle de `--llama`.
3. **`--asa`**: en vez de solo trasladar el `repr` crudo de Python, se escribió
   un *pretty-printer* recursivo (dispatch con `dynamic_cast` sobre cada tipo
   de `Expr`/`Stmt`) que imprime el AST **como código Nemi legible** (con
   `alt`, `⌊⌋`, `∨`/`∧`, etc.), no como volcado de estructuras internas — más
   útil para depurar que un repr genérico. Nota: igual que la versión Python,
   solo imprime `definitions`, no el `main` de nivel superior (así es también
   el alcance de `--asa`/`--arbol` en `cli.py`).

### Cómo verificar la Fase 5
- `nemi ../examples/guion_factorial.nemi` (sin `--llama`) debe imprimir
  exactamente lo mismo que `python -m nemi` con el mismo archivo (usa
  `imprime(...)`, no auto-print de la llamada suelta).
- `nemi --ayuda` y `python -m nemi --ayuda` deben coincidir en contenido
  (salvo el prefijo "usage:"/"uso:", que Python deja en inglés por ser parte
  de `argparse`).
- `nemi archivo.nemi --asa` no debe fallar en ningún archivo del corpus, y
  debe leerse como el `.nemi` original (con `si`/`alt`/`fin`, no como un dump
  de C++).

> ⚠️ **Otro gotcha real de C++ que apareció aquí:** al escribir el texto de
> `--ayuda` con acentos usando escapes `\xNN` a mano, el compilador falló con
> *"hex escape sequence out of range"* — ver el apéndice de errores comunes
> más abajo (`\xC3\xBAa` se lee como un solo escape porque `a` es un dígito
> hexadecimal válido).

> ⚠️ **Aviso descubierto al probar el CLI (Windows específicamente):** pasar un
> identificador con tildes (p. ej. `--call "máximo(...)"`) como argumento de
> línea de comandos en Windows puede llegar **corrompido** a `main(int, char**)`
> — Windows decodifica `argv` con la página de códigos ANSI activa, no UTF-8,
> a diferencia de Linux/macOS (UTF-8 nativo) o de Python en Windows (que sí usa
> `GetCommandLineW` internamente). Esto **no** es un bug del lexer/parser: leer
> un archivo `.nemi` con `función máximo(...)` funciona perfecto (la lectura de
> archivo sí es UTF-8 real); el problema es específico de argumentos pasados
> por la terminal. Lo confirmarás así: el mismo `índice fuera de rango` con un
> nombre de función ASCII da el mensaje correcto, pero con `máximo` en el
> argumento de línea de comandos, el error menciona un carácter irreconocible.
> Arreglo correcto (queda pendiente, ver `backlog.md`): usar
> `wmain(int argc, wchar_t** argv)` + `WideCharToMultiByte` (o
> `GetCommandLineW`/`CommandLineToArgvW`) para decodificar `argv` como UTF-8
> de verdad, en vez del `main(int, char**)` portable estándar.

---

## Fase 6 — Pruebas y cierre

- Corre `ctest`: `corpus_test` debe leer **16/16 PASS** — ✅ ya verificado
  (los 15 casos del §17 + el caso de estrés bignum de la Fase 4).
- (Opcional) escribe pruebas unitarias del lexer y del parser.
- Actualiza la tabla de estado en `README.md` y marca todo en `backlog.md`.
- Idealmente, un CI que compile y corra **ambas** suites (Python y C++).
- Pendiente real: arreglar el `argv` UTF-8 en Windows (`wmain`), ver Fase 5.

---

## Apéndice: errores comunes

- **"undefined reference"/enlazado:** olvidaste añadir tu nuevo `.cpp` a
  `CMakeLists.txt` (lista de fuentes de la librería `nemi`). Los 6 actuales ya
  están; si divides en más archivos, agrégalos.
- **`std::bad_variant_access`:** leíste un `Value`/`Token` como el tipo
  equivocado. Comprueba primero con `std::holds_alternative<T>(v)` o
  `std::get_if<T>(&v)`.
- **Símbolos raros en pantalla (Windows):** el CLI ya fuerza UTF-8; si compilas
  con MSVC añade `/utf-8` (ya está en `CMakeLists.txt`).
- **`bool` es un `int` en C++… no tanto:** aquí `bool` y `int` **son distintos**;
  aprovéchalo para la polisemia `+`/`·` (OR/AND solo si **ambos** son `bool`).
- **Punteros colgantes en el AST:** usa siempre `std::make_unique` y `std::move`
  al pasar hijos; nunca copies un `unique_ptr`.
- **Bucle infinito en el lexer:** asegúrate de que **cada** rama avanza el cursor
  (`pos_`); si no, `tokenize` nunca termina.
- **El caso no pasa a PASS:** vuelve a leer la función equivalente en
  `../python/nemi/*.py` línea por línea; casi siempre es una regla fina (base-1,
  corto-circuito, inclusivo) que se te escapó.
- **"hex escape sequence out of range" al escribir UTF-8 a mano con `\xNN`:**
  las secuencias `\x` en C++ consumen **todos** los dígitos hexadecimales que
  encuentren después, sin límite de longitud — así que `"\xC3\xBAa"` (para
  "úa") intenta leer `\xBAa` como un solo escape (`0xBAa`, fuera de rango para
  un `char`), porque `a` **es** un dígito hexadecimal válido (`a`-`f`). Pasó de
  verdad al escribir el texto de ayuda del CLI en la Fase 5. Arreglo: corta el
  literal justo después del escape con literales adyacentes (se concatenan
  solos): `"\xC3\xBA" "a"` en vez de `"\xC3\xBAa"`. Ocurre con cualquier letra
  `a`-`f`/`A`-`F` que siga a un escape `\xNN` (aquí picó dos veces: `\xBAa` en
  "evalúa" y `\xA1ctico` en "sintáctico", porque `c` también es dígito hex).
