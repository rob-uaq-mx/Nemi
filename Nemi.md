# Nemi — el pseudolenguaje de algoritmos

**Nemi** (del náhuatl *nemi*, «andar, ejecutarse») es el pseudolenguaje empleado
en las notas del curso para presentar algoritmos. Este documento lo describe y
sirve además como **especificación** para construir su intérprete. Los archivos de
código fuente usan la extensión **`.nemi`**. El documento consta de dos partes:

- **Parte I — Guía de lectura.** Cómo leer los algoritmos tal como aparecen en el
  libro (para estudiantes).
- **Parte II — Especificación del intérprete.** Sintaxis concreta (UTF-8),
  gramática, semántica, decisiones de diseño y corpus de prueba (para quien
  implemente el intérprete).

El lenguaje es **UTF-8**: usa símbolos matemáticos (`←`, `≤`, `≥`, `≠`, `∧`, `∨`,
`¬`, `⌊ ⌋`, `⌈ ⌉`, `√`, `·`) y palabras clave en español con acentos
(`función`, `regresa`, …).

---
---

# Parte I — Guía de lectura

El pseudocódigo es un lenguaje intermedio entre el lenguaje natural y un lenguaje
de programación: es lo bastante preciso para no ser ambiguo, pero no depende de la
sintaxis de ningún lenguaje concreto. En el libro, los algoritmos se componen con
los paquetes LaTeX `algorithm` / `algpseudocode`, con palabras clave **en
español**.

## 1. Estructura general

Cada algoritmo aparece en un recuadro rotulado **«Algoritmo N»** con un título.
Su cuerpo tiene:

- **líneas numeradas**, para referirse a ellas en el texto y en las pruebas de
  corrección;
- **sangría (indentación)** que indica el alcance de cada estructura de control;
- un encabezado de **función** (devuelve un valor) o **procedimiento** (ejecuta
  acciones):

```
función nombre(parámetros)
    ⟨cuerpo⟩

procedimiento nombre(parámetros)
    ⟨cuerpo⟩
```

## 2. Palabras clave

| Palabra clave | Significado |
|---|---|
| **función** / **procedimiento** | Encabezado del algoritmo |
| **para … hasta … repite** | Ciclo con contador (recorre un rango) |
| **mientras** | Ciclo condicional (se repite mientras la condición sea verdadera) |
| **si … entonces / alt** | Condicional (`alt` = rama alternativa/«si no») |
| **regresa** | Devuelve un valor y termina |
| **y**, **o**, **no** | Conectores lógicos |

## 3. Asignación

Guarda un valor en una variable; se escribe con `←` (o `=` en el libro):

```
grande ← s₁            resultado ← 1
```

No confundir con la **igualdad** de una condición (p. ej. `si n = 0`), donde `=`
es una comparación.

## 4. Estructuras de control

**Ciclo `para`:** ejecuta el cuerpo con `i = a, a+1, …, b` (inclusive):

```
para i ← 2 hasta n repite
    ⟨instrucciones⟩
```

**Ciclo `mientras`:** repite mientras la condición sea verdadera (evaluada antes
de cada iteración):

```
mientras b ≠ 0
    r ← a mod b
    a ← b
    b ← r
```

**Condicional:**

```
si n = 0 entonces
    regresa 1
alt
    regresa n · factorial(n − 1)
```

## 5. Recursión, salida y comentarios

Una función se **invoca** por su nombre con argumentos entre paréntesis; si se
invoca a sí misma, es **recursión** (requiere un **caso base**). **regresa**
entrega el resultado y detiene la función. Los **comentarios** (al margen, con
`▷`) no se ejecutan.

## 6. Operadores y notación

- **Aritméticos:** `+`, `−`, `·`, `/`.
- **Enteros:** `a mod b` (residuo), `⌊x⌋` (piso), `⌈x⌉` (techo), `√x`.
- **Comparación:** `=`, `≠`, `<`, `≤`, `>`, `≥`.
- **Lógicos / booleanos:** `∧` (y/AND), `∨` (o/OR), `¬` (no/NOT).
- **Subíndices / arreglos:** `s₁, …, sₙ` (índice desde 1).

## 7. Ejemplo completo

```
Algoritmo 1: Encontrar el valor máximo en una sucesión

1  función máximo(s, n)
2      grande ← s₁
3      para i ← 2 hasta n repite
4          si sᵢ > grande entonces
5              grande ← sᵢ
6      regresa grande
```

Se inicializa `grande` con el primer término (2); el ciclo recorre el resto (3) y
actualiza `grande` cuando encuentra uno mayor (4–5). El **invariante** es
«`grande` es el mayor entre `s₁, …, sᵢ`»; al terminar con `i = n`, `grande` es el
máximo.

---
---

# Parte II — Especificación del intérprete

**Audiencia:** quien implemente un intérprete de este lenguaje. Aquí se fijan la
sintaxis concreta, la gramática, la semántica y las decisiones de diseño.

> **Punto de partida.** Hasta ahora el lenguaje solo existía como **notación
> matemática compuesta con LaTeX**; no tenía una sintaxis concreta en texto. Esta
> Parte II **define** esa sintaxis concreta, en **UTF-8**.

## 8. Decisiones de diseño (resumen)

| Cuestión | Decisión |
|---|---|
| Codificación | **UTF-8**. Se usan símbolos Unicode (`←`, `≤`, `≥`, `≠`, `∧`, `∨`, `¬`, `⌊⌋`, `⌈⌉`, `√`, `·`, `−`) y palabras acentuadas (`función`). |
| Asignación vs. igualdad | `←` **solo** asigna; `=` **solo** compara. Elimina la ambigüedad del libro (que usa `=` para ambas). |
| Delimitación de bloques | **Cierres explícitos**: `fin si`, `fin para`, `fin mientras`, `fin función`, `fin procedimiento`. (El libro usa `[noend]`, sin cierres, apoyado en la sangría; para el intérprete los cierres son más robustos.) |
| Índices | **Base 1**: `s[1]` es el primer elemento. Es la convención del texto. |
| Enteros | **Precisión arbitraria (bignum)**: RSA y factoriales lo exigen. |
| Producto | **Explícito con `·`**; no se admite yuxtaposición (`xy`), que sería ambigua con los identificadores multiletra. |
| `+` y `·` polisémicos | Se resuelven **por tipo de operando** (véase §11). |
| Parámetros | Escalares por valor; **arreglos/matrices/cadenas por referencia** (hay algoritmos que los mutan). |
| Acciones en prosa | Se escriben entre guillemets `« … »`; **no** pertenecen al núcleo ejecutable (véase §14). |

## 9. Estructura léxica (tokens)

- **Palabras clave:** `función`, `procedimiento`, `para`, `hasta`, `repite`,
  `mientras`, `si`, `entonces`, `alt` (o `alternativamente`), `regresa`, `fin`,
  `incluye`, `y`, `o`, `no`. Las palabras `repite` y `entonces` son **opcionales** (azúcar sintáctico);
  el intérprete debe aceptarlas y también su ausencia.
- **Identificadores:** `letra (letra | dígito | '_')*`, donde *letra* incluye
  letras Unicode (por tanto `á`, `ñ`, etc. son válidas). Se permiten subíndices
  como parte del nombre si se escriben con `_` (`c_0`, `x_k`) o se usa indexación
  con corchetes (preferido).
- **Números:** enteros (`0`, `42`, `3628800`) y reales (`8.3`). Las constantes
  `0` y `1` sirven también como bits.
- **Cadenas:** entre comillas dobles: `"001"`.
- **Operadores y signos:** `←  =  ≠  <  ≤  >  ≥  +  −  ·  /  mod  ¬  ∧  ∨
  ⌊ ⌋ ⌈ ⌉ √  ( )  [ ]  ,`.
  Se aceptan también estos equivalentes de teclado: `<-`(←), `!=`(≠), `<=`(≤),
  `>=`(≥), `-`(−), `*`(·). Los conectores lógicos se escriben en **español**,
  como palabra `y`/`o`/`no` o como símbolo `∧`/`∨`/`¬` (no se admiten `and`,
  `or`, `not`).
- **Comentarios:** desde `▷` (o `#`) hasta el fin de línea; se ignoran.
- **Acciones en prosa:** `« … »` (texto libre no ejecutable).

## 10. Gramática (EBNF)

```ebnf
programa       ::= { definición | instrucción | inclusión } ;
inclusión      ::= "incluye" cadena ;
definición     ::= función | procedimiento ;
función        ::= "función" ident "(" [ params ] ")" bloque "fin" "función" ;
procedimiento  ::= "procedimiento" ident "(" [ params ] ")" bloque "fin" "procedimiento" ;
params         ::= ident { "," ident } ;

bloque         ::= { instrucción } ;
instrucción    ::= asignación | ciclo_para | ciclo_mientras
                 | condicional | retorno | llamada | acción_prosa ;

asignación     ::= lugar "←" expresión ;
lugar          ::= ident { "[" expresión "]" } ;

ciclo_para     ::= "para" ident "←" expresión "hasta" expresión [ "repite" ]
                   bloque "fin" "para" ;
ciclo_mientras ::= "mientras" expresión bloque "fin" "mientras" ;
condicional    ::= "si" expresión [ "entonces" ] bloque
                   [ "alt" bloque ] "fin" "si" ;
retorno        ::= "regresa" [ expresión ] ;
llamada        ::= ident "(" [ args ] ")" ;
args           ::= expresión { "," expresión } ;
acción_prosa   ::= "«" texto "»" ;

expresión      ::= expr_o ;
expr_o         ::= expr_y   { ("∨" | "o")  expr_y } ;
expr_y         ::= expr_no  { ("∧" | "y")  expr_no } ;
expr_no        ::= [ "¬" | "no" ] expr_comp ;
expr_comp      ::= expr_suma [ op_comp expr_suma ] ;
op_comp        ::= "=" | "≠" | "<" | "≤" | ">" | "≥" ;
expr_suma      ::= expr_prod  { ("+" | "−") expr_prod } ;
expr_prod      ::= expr_unaria { ("·" | "/" | "mod") expr_unaria } ;
expr_unaria    ::= [ "−" | "¬" ] expr_postfija ;
expr_postfija  ::= primaria { "[" expresión "]" } ;
primaria       ::= número | ident | llamada | cadena
                 | "(" expresión ")"
                 | "⌊" expresión "⌋" | "⌈" expresión "⌉"
                 | "√" expr_unaria ;
```

## 11. Tipos y operadores

| Tipo | Notas |
|---|---|
| Entero | **bignum** obligatorio. |
| Real | punto flotante (o racional); aparece en piso/techo y complejidad. |
| Booleano / bit | `0` y `1`. |
| Sucesión / arreglo | **índice base 1**, mutable. |
| Matriz | 2D, base 1 (`W[i][j]`). |
| Cadena | sucesión de símbolos; indexable en base 1. |

**Precedencia** (de mayor a menor): `¬`/`no` y unario `−`  ▸  `·` `/` `mod`  ▸
`+` `−`  ▸  comparaciones  ▸  `∧`/`y`  ▸  `∨`/`o`.

> **Polisemia de `+` y `·` (crítico).** En la **Unidad 4** (álgebra booleana y
> circuitos), `+` = **OR**, `·` = **AND** y `¬`/barra = **NOT**. En las
> **Unidades 2–3**, `+` y `·` son suma y producto usuales. Resolución
> recomendada: **despacho por tipo** — si ambos operandos son `bit/booleano`,
> `+ ↦ OR` y `· ↦ AND`; si son numéricos, son aritméticos. (Alternativa:
> operadores separados para el modo booleano.)

## 12. Semántica de las instrucciones

- **Asignación** `lugar ← e`: evalúa `e` y lo almacena en la variable o celda
  (`s[i]`, `W[i][j]`).
- **`para i ← a hasta b`**: `i` toma `a, a+1, …, b` (paso `+1`, **inclusive**). Si
  `a > b`, el cuerpo no se ejecuta. `i` no debería modificarse dentro del cuerpo.
- **`mientras C`**: evalúa `C` **antes** de cada iteración; `∧`/`∨` con
  **corto-circuito**.
- **`si C … alt …`**: la rama `alt` (alternativa) es opcional; se admite anidamiento.
- **`regresa e`**: evalúa `e`, termina la función y devuelve el valor. En
  procedimientos, `regresa` sin valor termina anticipadamente.
- **Llamada / recursión:** `f(args)`; requiere pila de llamadas.

**Paso de parámetros.** Escalares por valor; **arreglos/matrices/cadenas por
referencia** (o con semántica observacionalmente equivalente). Justificación:
`inserción_por_orden(s, n)` ordena `s` in situ, y `mcd` intercambia sus
parámetros.

**Instrucciones de nivel superior (cuerpo del guión).** Además de definiciones,
un programa puede contener **instrucciones sueltas** a nivel superior (§10). Se
ejecutan **en orden de aparición** en un entorno global, *después* de registrar
todas las definiciones (**hoisting**: una instrucción de nivel superior puede
llamar a una función definida más abajo en el archivo). El **valor** de una
instrucción suelta se **descarta** —una llamada como `factorial(100)` se evalúa
por su efecto, **no** se imprime (semántica de guión, à la Python)—; para
producir salida se usa `imprime(…)`. Un `regresa` a nivel superior termina el
guión.

**Inclusión de archivos (`incluye`).** `incluye "ruta.nemi"` empalma las
definiciones e instrucciones de otro archivo `.nemi` en el punto donde
aparece, como si su texto se hubiera pegado ahí (la ruta se resuelve relativa
al directorio del archivo que incluye, o al directorio de trabajo si el
archivo aún no se ha guardado en disco). Es recursivo (un archivo incluido
puede a su vez incluir otros) y **detecta ciclos** (A incluye B incluye A es
un error, no un cuelgue). A diferencia de un simple empalme de texto, cada
token conserva su archivo y línea de origen real, así que un error dentro de
un archivo incluido señala **ese** archivo, no el que lo incluye. Esto permite
tener una pequeña "biblioteca estándar": un archivo con funciones de uso
común que otros archivos reutilizan con `incluye`, en vez de copiarlas.

## 13. Bloques

Con **cierres explícitos** (`fin si`, `fin para`, `fin mientras`,
`fin función`, `fin procedimiento`), el análisis no depende de la sangría. Aun
así, se recomienda sangrar por legibilidad. (Si en cambio se prefiere regla de
sangría estilo Python, hay que prohibir mezclar tabuladores y espacios; los
cierres explícitos son la opción recomendada aquí.)

## 14. Acciones en prosa (no ejecutables)

Algunos algoritmos del libro contienen pasos redactados en lenguaje natural.
Deben ir entre `« … »` y **no** forman parte del núcleo ejecutable. Opciones para
tratarlas: (a) rechazarlas como «no soportadas»; (b) mapearlas a **primitivas**
provistas por el entorno. Ejemplos hallados:

- `« intercambia a y b »` → primitiva `intercambia(a, b)`.
- `« dividir el tablero en cuatro subtableros de (n/2)×(n/2) »` (enlosado con
  trominos) — geometría de alto nivel; fuera del núcleo.
- `« colocar un tromino en el centro … »`, `« enlosar con un tromino derecho »`.

Recomendación: implementar un **núcleo formal** (aritmética, arreglos, control de
flujo, recursión) que cubra la mayoría de los algoritmos y dejar los pasos
geométricos como primitivas o fuera de alcance.

## 15. Biblioteca de primitivas sugerida

`mod(a,b)` · `piso(x)` = ⌊x⌋ · `techo(x)` = ⌈x⌉ · `raíz(x)` = √x · `abs(x)` ·
`long(s)` = longitud de sucesión/cadena · `intercambia(x,y)` · concatenación y
subcadenas · operaciones de matrices cero-uno (producto booleano, `Aᵏ`,
disyunción entrada a entrada) · conversión entre bases (binario/decimal/hexadecimal).
Decidir cuáles son primitivas del lenguaje y cuáles se definen en el propio
pseudolenguaje.

## 16. Casos límite y errores a definir

- `mod` o `/` con divisor `0`.
- Índice fuera de rango (recordar: base 1).
- Función que no ejecuta ningún `regresa` en algún camino.
- Recursión sin caso base (desbordamiento de pila).
- Operación entre tipos incompatibles (p. ej. `+` entre entero y bit): definir
  regla o error.
- Comparación entre tipos distintos.

---

## 17. Corpus de prueba (transcrito a la sintaxis concreta)

Los algoritmos de las notas, ya con entradas y **salidas verificadas** en el
texto, forman una suite de aceptación. A continuación se transcriben a la sintaxis
concreta de la Parte II.

### 17.1 Máximo de una sucesión  ·  `máximo([3,9,4],3) = 9`
```
función máximo(s, n)
    grande ← s[1]
    para i ← 2 hasta n
        si s[i] > grande
            grande ← s[i]
        fin si
    fin para
    regresa grande
fin función
```

### 17.2 Búsqueda de texto  ·  `busca_texto("001",3,"010001",6) = 4`
```
función busca_texto(p, m, t, n)
    para i ← 1 hasta n − m + 1
        j ← 1
        mientras t[i + j − 1] = p[j]
            j ← j + 1
            si j > m
                regresa i          ▷ patrón encontrado en la posición i
            fin si
        fin mientras
    fin para
    regresa 0                       ▷ no aparece
fin función
```

### 17.3 Inserción por orden (in situ)  ·  `[34,20,19,5] → [5,19,20,34]`
```
procedimiento inserción_por_orden(s, n)
    para i ← 2 hasta n
        val ← s[i]
        j ← i − 1
        mientras j ≥ 1 ∧ val < s[j]
            s[j + 1] ← s[j]
            j ← j − 1
        fin mientras
        s[j + 1] ← val
    fin para
fin procedimiento
```

### 17.4 Prueba de primalidad  ·  `es_primo(97) = 0` (primo); `es_primo(51) = 3`
```
función es_primo(n)
    para d ← 2 hasta ⌊√n⌋
        si n mod d = 0
            regresa d              ▷ compuesto; d es un factor
        fin si
    fin para
    regresa 0                       ▷ primo
fin función
```

### 17.5 Máximo común divisor (Euclides)  ·  `mcd(504,396) = 36`
```
función mcd(a, b)                   ▷ a, b ≥ 0, no ambos cero
    si a < b
        intercambia(a, b)
    fin si
    mientras b ≠ 0
        r ← a mod b
        a ← b
        b ← r
    fin mientras
    regresa a
fin función
```

### 17.6 Factorial (recursivo)  ·  `factorial(5) = 120`
```
función factorial(n)
    si n = 0
        regresa 1
    alt
        regresa n · factorial(n − 1)
    fin si
fin función
```

### 17.7 Fibonacci (recursivo)  ·  `fib(5) = 5`
```
función fib(n)
    si n = 0 ∨ n = 1
        regresa n
    alt
        regresa fib(n − 1) + fib(n − 2)
    fin si
fin función
```

### 17.8 Factorial (iterativo)  ·  `factorial_iter(6) = 720`
```
función factorial_iter(n)
    resultado ← 1
    para i ← 2 hasta n
        resultado ← resultado · i
    fin para
    regresa resultado
fin función
```

### 17.9 Fibonacci (iterativo)  ·  `fib_iter(10) = 55`
```
función fib_iter(n)
    si n = 0 ∨ n = 1
        regresa n
    fin si
    anterior ← 0
    actual ← 1
    para i ← 2 hasta n
        siguiente ← anterior + actual
        anterior ← actual
        actual ← siguiente
    fin para
    regresa actual
fin función
```

### 17.10 Exponenciación rápida  ·  `exp_rápida(2,10) = 1024`
```
función exp_rápida(a, n)
    resultado ← 1
    x ← a
    mientras n > 0
        si n mod 2 = 1
            resultado ← resultado · x
        fin si
        x ← x · x
        n ← ⌊n / 2⌋
    fin mientras
    regresa resultado
fin función
```

### 17.11 Exponenciación modular  ·  `exp_mod(572,29,713) = 113`; `exp_mod(3,13,7) = 3`
```
función exp_mod(a, n, z)
    resultado ← 1
    x ← a mod z
    mientras n > 0
        si n mod 2 = 1
            resultado ← (resultado · x) mod z
        fin si
        x ← (x · x) mod z
        n ← ⌊n / 2⌋
    fin mientras
    regresa resultado
fin función
```

### 17.12 Algoritmo de Warshall (cerradura transitiva)
Entrada: matriz cero-uno `A` (n×n). Salida: matriz de `R⁺` (véase Ejemplo 1.164
en `unidad1.tex`; la traza 4×4 W₀…W₄ es la prueba esperada).
```
función Warshall(A, n)              ▷ A es una matriz cero-uno n×n
    W ← A
    para k ← 1 hasta n
        para i ← 1 hasta n
            para j ← 1 hasta n
                W[i][j] ← W[i][j] ∨ (W[i][k] ∧ W[k][j])
            fin para
        fin para
    fin para
    regresa W
fin función
```

### 17.13 Enlosado con trominos (usa acciones en prosa)
Ilustra los pasos **no ejecutables** (§14): probar solo la estructura de
recursión, o proveer primitivas geométricas.
```
procedimiento enlosa(n, L)          ▷ n potencia de 2; L: cuadro faltante
    si n = 2
        « enlosar el bloque 2×2 con un tromino derecho »
        regresa
    fin si
    « dividir el tablero en cuatro subtableros de (n/2)×(n/2) »
    « colocar un tromino central que cubra un cuadro de cada subtablero sin L »
    enlosa(n / 2, m1)
    enlosa(n / 2, m2)
    enlosa(n / 2, m3)
    enlosa(n / 2, m4)
fin procedimiento
```

---

## 18. Preguntas abiertas para el autor del curso

1. ¿La sintaxis concreta debe **parecerse al PDF** (sangría, símbolos) o priorizar
   el análisis (cierres explícitos)? Aquí se eligieron cierres explícitos.
2. ¿Se desea un **modo booleano** explícito para la Unidad 4, o basta con resolver
   `+`/`·` por tipo?
3. ¿El intérprete debe **ejecutar** los algoritmos geométricos (trominos) o basta
   con el núcleo aritmético/estructural?
4. ¿Se quiere E/S interactiva (`leer`, `imprime`) además de `regresa`?
5. ¿Índices siempre base 1, o configurable?

---

### Resumen para el siguiente implementador

El artefacto de partida es esta especificación de **Nemi**. Tareas: (1) implementar
el lexer y parser de la **sintaxis UTF-8** de la Parte II (gramática §10); (2) respetar las
decisiones de §8 (asignación `←` vs. igualdad `=`, bloques con `fin`, base 1,
bignum, paso por referencia de arreglos, `+`/`·` por tipo); (3) fijar el
tratamiento de las **acciones en prosa** (§14); y (4) validar con el **corpus del
§17**, cuyas salidas ya están verificadas en las notas.
