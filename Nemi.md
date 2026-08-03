# Nemi — el pseudolenguaje de algoritmos

> **Especificación versión 0.2** · véase el historial de versiones en §24.

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

> **Extensión v0.2 (§20).** La biblioteca estándar amplía esta gramática con
> literales de lista/conjunto (`lista_lit`, `conjunto_lit` en `primaria`), los
> operadores de conjunto `∈ ∉ ⊆ ⊂` en `op_comp`, el ciclo `ciclo_cada`
> (`para cada … en …`) en `instrucción`, y las instrucciones `afirma`/`traza`.
> Las producciones exactas están en §20–§21. Esta sección §10 documenta el
> **núcleo (v0.1)**; las adiciones son opcionales para un intérprete que solo
> ejecute el corpus §17.

## 11. Tipos y operadores

| Tipo | Notas |
|---|---|
| Entero | **bignum** obligatorio. |
| Real | punto flotante (o racional); aparece en piso/techo y complejidad. |
| Booleano / bit | `0` y `1`. |
| Sucesión / arreglo | **índice base 1**, mutable. |
| Matriz | 2D, base 1 (`W[i][j]`). |
| Cadena | sucesión de símbolos; indexable en base 1. |
| Conjunto *(v0.2)* | no ordenado, sin duplicados; lo añade la biblioteca estándar (§20.2). |

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
   el análisis (cierres explícitos)? **Resuelta:** cierres explícitos.
2. ¿Se desea un **modo booleano** explícito para la Unidad 4, o basta con resolver
   `+`/`·` por tipo? **Resuelta:** basta el despacho por tipo (no hay modo booleano
   separado).
3. ¿El intérprete debe **ejecutar** los algoritmos geométricos (trominos) o basta
   con el núcleo aritmético/estructural? **Resuelta:** las acciones en prosa `«…»`
   son no ejecutables (§14); basta el núcleo aritmético/estructural.
4. ¿Se quiere E/S interactiva (`leer`, `imprime`) además de `regresa`? **Resuelta:**
   sí para salida (`imprime`, §21.1); no hay entrada interactiva.
5. ¿Índices siempre base 1, o configurable? **Resuelta:** siempre base 1, no
   configurable.

> **Nota.** Todas las preguntas de §18 quedaron resueltas y las respuestas están
> reflejadas arriba (las 2, 3 y 5 ya se documentaban en el `README` del repositorio;
> la 4 se especifica en §21.1). La biblioteca estándar y las extensiones que exige
> se especifican en §19–§23.

---

## 19. Biblioteca estándar de Nemi (`stdlib`)

### 19.1 Motivación

El curso reutiliza sin cesar un puñado de funciones (`C(n,r)`, `factorial`,
`exp_mod`, `mcd`, generadores de permutaciones/combinaciones, operaciones de
conjuntos…). En vez de que cada estudiante las reescriba —con el riesgo de errar
un `⌊ ⌋` o una condición—, se provee una **biblioteca estándar**: un conjunto de
archivos `.nemi` que se traen con `incluye` y funciones de núcleo (primitivas)
provistas por el intérprete. Objetivo pedagógico: que el estudiante pueda
**verificar su tarea en una línea** (`imprime(C(52,5))`) y que las definiciones
de la biblioteca sean **legibles** (material de estudio), no cajas negras.

### 19.2 Arquitectura en dos capas

1. **Núcleo (primitivas del intérprete).** Cosas que no se pueden —o no conviene—
   escribir en el propio Nemi: el tipo `Conjunto` y sus operaciones, listas
   dinámicas (`agrega`), E/S (`imprime`), aserciones (`afirma`), manejo de
   cadenas, copia profunda. Se especifican en §20 y §21. Esto **amplía** el
   catálogo tentativo de §15.
2. **Biblioteca en Nemi (`stdlib/*.nemi`).** Todo lo demás se escribe en el
   propio lenguaje sobre el núcleo, y se distribuye como archivos `.nemi`
   incluibles. El código completo y sus pruebas están en §22.

**Regla de oro para el implementador:** si una función de §22 aparece con cuerpo
Nemi, se implementa **cargando ese archivo**, no en el lenguaje anfitrión. Solo lo
listado en §20–§21 es primitivo.

### 19.3 Módulos

| Archivo | Unidad | Contenido |
|---|---|---|
| `teoria_numeros.nemi` | 2 | `mcd`, `mcm`, `primo`, `exp_mod`, `euclides_extendido`, `mod_inv` |
| `conteo.nemi` | 3 | `factorial`, `P`, `C`, `pascal`, `multinomial`, `genera_combinaciones`, `genera_permutaciones` |
| `conjuntos.nemi` | 1 | `complemento`, `subconjunto_propio`, `potencia`, `producto_cartesiano` |
| `relaciones.nemi` | 1 | `es_refleja`, `es_simetrica`, `es_transitiva`, `es_antisimetrica`, `composicion`, `cerradura_transitiva` |
| `booleana.nemi` | 4 | `nand`, `nor`, `xor`, `xnor`, `minterminos`, `equivalentes`, `evalua` |
| `cadenas.nemi` | 1–2 | `invierte`, `prefijo`, `sufijo`, `a_binario`, `desde_base` |

Un archivo `stdlib.nemi` que solo hace `incluye` de todos los anteriores permite
`incluye "stdlib.nemi"` de una vez.

---

## 20. Extensiones del lenguaje requeridas por la biblioteca

Estas son **adiciones al núcleo** (no estaban en la Parte II). Cada una indica su
prioridad: **[REQ]** = requerida por la biblioteca; **[OPC]** = mejora, no
bloquea.

### 20.1 Literales de lista y de conjunto [REQ]

La gramática de §10 usaba `[ ]` solo para **indexar**; los literales `[1,2,3]`
—que ya aparecen en las anotaciones de prueba de §17 y en las llamadas del
corpus— no estaban formalizados. La versión 0.2 **extiende** `primaria` (§10) con
estas dos producciones (la nota al pie de §10 remite aquí):

```ebnf
primaria    ::= número | ident | llamada | cadena
              | "(" expresión ")"
              | "⌊" expresión "⌋" | "⌈" expresión "⌉"
              | "√" expr_unaria
              | lista_lit | conjunto_lit ;          (* nuevo *)
lista_lit   ::= "[" [ args ] "]" ;                  (* [], [1,2,3], [[0,1],[1,0]] *)
conjunto_lit::= "{" [ args ] "}" | "∅" ;            (* {}, {1,2,3}, ∅ *)
```

- Una **lista** (arreglo) es ordenada, mutable, indexada en base 1; puede anidar
  (matrices = listas de listas). `[]` es la lista vacía.
- Un **conjunto** es no ordenado y **sin duplicados**. `{}` y `∅` son el conjunto
  vacío. Al construir `{3,1,2,1}` los duplicados se colapsan → `{1,2,3}`.

> **Doble uso del glifo `∅` (decidido: se acepta y se documenta).** El mismo
> símbolo `∅` cumple dos papeles según el contexto: como **entrada**, es el
> literal del conjunto vacío (arriba); como **salida**, es la representación
> impresa del valor «sin valor» (lo que devuelve un procedimiento o un `regresa`
> sin expresión; véase la tabla de §21.1). No hay ambigüedad de análisis
> sintáctico —uno ocurre en el código fuente, el otro solo en la salida de
> `imprime`—, pero conviene que el estudiante lo sepa: si ve `∅` impreso por
> `imprime(unProcedimiento())`, significa «no hubo valor», no «conjunto vacío»
> (un conjunto vacío se imprime `{}`, §21.1).
- Los elementos de una lista o conjunto pueden ser enteros, reales, bits, cadenas,
  **listas** u **otros conjuntos** (necesario para pares ordenados `[a,b]` en
  relaciones y para el conjunto potencia).

### 20.2 Tipo `Conjunto` y sus primitivas [REQ]

La versión 0.2 **añade el tipo `Conjunto`** (ya listado como fila *v0.2* en la
tabla de §11). Operaciones primitivas:

| Primitiva | Resultado |
|---|---|
| `x ∈ A` | bit `1`/`0` (pertenencia). También `x ∉ A`. Alternativas para `∈`: la palabra **`en`** (`x en A`) o la función `pertenece(x, A)` — ver nota siguiente |
| `A ⊆ B` | bit (subconjunto). Alias: `subconjunto(A, B)` |
| `A ⊂ B` | bit (subconjunto propio) |
| `union(A, B)` | `A ∪ B` |
| `interseccion(A, B)` | `A ∩ B` |
| `diferencia(A, B)` | `A ∖ B` |
| `cardinalidad(A)` | número de elementos (entero). También `long(A)` |
| `agrega(A, x)` | inserta `x` en `A` **in situ** (idempotente: si ya está, no cambia); muta por referencia, sin valor útil de retorno |

Los operadores de comparación de conjuntos `∈ ∉ ⊆ ⊂` **extienden** `op_comp`
(§10), pues **producen un bit** y encajan en el nivel de las comparaciones.
`∈` tiene además la palabra **`en`** como equivalente directo, sin necesitar
teclado Unicode: `x en A` ≡ `x ∈ A`. Reutiliza la misma palabra clave que
introduce `para cada … en …` (§20.3) sin ambigüedad de gramática — la
producción `ciclo_cada` consume `en` en una posición fija, justo después de
`cada ident`, nunca a través de `op_comp`, así que ambos usos conviven en la
misma gramática sin conflicto. Los demás operadores de conjunto no tienen
palabra equivalente: para `∉ ⊆ ⊂`, sin teclado Unicode, se usan las funciones
`pertenece` y `subconjunto` (`∉` también se obtiene negando: `¬pertenece(x, A)`).
(Las operaciones binarias `∪ ∩ ∖` se ofrecen como **funciones**, no
operadores, para no ampliar la precedencia; `union`, `interseccion`,
`diferencia`.)

**Igualdad estructural [REQ].** `=` y `≠` operan por **valor** sobre datos
compuestos:
- **conjuntos:** iguales si tienen exactamente los mismos elementos (sin importar
  orden), comparando recursivamente;
- **listas:** iguales si tienen la misma longitud y elementos iguales **en el
  mismo orden**;
- **cadenas:** iguales carácter a carácter.

**Orden canónico [REQ].** Para impresión determinista (§21.1) e iteración
(§20.3), un conjunto se recorre en orden canónico: números en orden ascendente;
cadenas en orden lexicográfico; elementos compuestos (listas/conjuntos) después de
los escalares, ordenados por su representación impresa. (El objetivo es solo
**reproducibilidad**, no un orden matemático profundo.)

### 20.3 Ciclo `para cada … en …` [REQ]

Itera sobre los elementos de un conjunto (en orden canónico) o de una lista (en
orden de índice). Nuevas palabras clave: **`cada`**, **`en`**.

```ebnf
ciclo_cada  ::= "para" "cada" ident "en" expresión [ "repite" ]
                bloque "fin" "para" ;
```

```
para cada x en A repite
    ⟨usa x⟩
fin para
```

No se debe mutar la colección que se está recorriendo dentro del ciclo (para eso,
constrúyase otra colección y fusiónese después; véase `potencia` en §22.3).

### 20.4 Listas dinámicas: `agrega`, `copia`, ceros [REQ]

| Primitiva | Semántica |
|---|---|
| `agrega(a, x)` | si `a` es **lista**, **anexa** `x` al final (crece la longitud); muta por referencia. (Sobre **conjunto**: inserta, §20.2.) |
| `long(a)` | longitud de una lista / de una cadena / cardinalidad de un conjunto |
| `copia(a)` | **copia profunda** (recursiva) de una lista/matriz/conjunto; indispensable para guardar instantáneas de un arreglo que luego se muta |
| `arreglo_cero(n)` | lista de `n` ceros, `[0,0,…,0]` (base 1) |
| `matriz_cero(m, n)` | matriz `m×n` de ceros |

Ejemplo del patrón «acumular resultados» (usado por los generadores de §22.2):

```
res ← []
agrega(res, copia(s))        ▷ copia: s se mutará en la siguiente iteración
```

### 20.5 Retorno de varios valores mediante lista [REQ, sin tipo nuevo]

`regresa` sigue devolviendo **un** valor. Cuando una función necesita entregar
varios (p. ej. `euclides_extendido → (d, s, t)`), **devuelve una lista** y quien
llama la desempaca por índice:

```
t ← euclides_extendido(240, 46)   ▷ t = [d, s, t]
d ← t[1]                          ▷ base 1
```

No se introduce un tipo «tupla»: una lista de longitud fija cumple el papel. Un
**par ordenado** (para relaciones/producto cartesiano) es una lista de dos
elementos `[a, b]`.

### 20.6 Funciones de primera clase [OPC]

Permitir pasar una **función como argumento** habilita azúcar agradable como
`tabla_verdad(mayoria, 3)`. **No es requerida:** el módulo `booleana.nemi` (§22.5)
trabaja sobre **arreglos de tabla de verdad** (un arreglo de `2ⁿ` bits), lo que
evita por completo el orden superior y es, además, pedagógicamente más claro (el
estudiante construye la tabla, que es el concepto). Si se implementa, especifíquese
por separado; el resto de la biblioteca **no** depende de ello.

---

## 21. `imprime`, `afirma` y `traza`

### 21.1 `imprime` [REQ] — resuelve la pregunta abierta §18.4

`imprime(e₁, …, e_k)` evalúa sus argumentos, imprime sus representaciones
**separadas por un espacio** y termina con **salto de línea**. `imprime()` (sin
argumentos) imprime una línea en blanco. Formato por tipo:

| Tipo | Representación |
|---|---|
| Entero / real | decimal (`120`, `8.3`) |
| Bit | `0` / `1` |
| Cadena | su contenido, **sin** comillas (`0011`) |
| Lista | `[a, b, c]` (recursivo: `[[0, 1], [1, 0]]`) |
| Conjunto | `{a, b, c}` en **orden canónico** (§20.2); vacío: `{}` |
| Matriz | como lista de listas (una convención; opcionalmente una rejilla por renglones) |
| Sin valor | `∅` — resultado de un procedimiento o de un `regresa` sin expresión. Es el comportamiento ya presente en las tres implementaciones. **Ojo:** mismo glifo que el literal de conjunto vacío en el código fuente (§20.1); el conjunto vacío **impreso** es `{}`, no `∅`. |

Es E/S por efecto: como dice §12, el valor de una instrucción suelta se descarta,
así que **solo** `imprime` produce salida.

### 21.2 `afirma` [REQ] — autoverificación

Instrucción nueva para que el estudiante compruebe su código contra los valores
del libro. Palabra clave **`afirma`**.

```ebnf
instrucción ::= … | aserción ;
aserción    ::= "afirma" expresión [ "," cadena ] ;
```

Semántica: evalúa la expresión (debe dar un bit). Si es `1`, no hace nada. Si es
`0`, **aborta** el programa con un mensaje que incluye el **archivo y la línea**
(§12 conserva el origen real incluso a través de `incluye`), el **texto fuente**
de la expresión y la cadena opcional. Ejemplo:

```
afirma C(8, 4) = 70
afirma mcd(504, 396) = 36, "Euclides"
```

Como `=` es igualdad estructural (§20.2), también sirve para listas y conjuntos:
`afirma pascal(4) = [1, 4, 6, 4, 1]`.

### 21.3 `traza` [OPC] — herramienta de comprensión

Ejecuta una expresión (típicamente una llamada) con **rastreo** activado durante
su extensión dinámica. Palabra clave **`traza`**.

```ebnf
instrucción ::= … | rastreo ;
rastreo     ::= "traza" expresión ;
```

Semántica sugerida: mientras el rastreo está activo, cada **entrada** a una
función imprime `→ f(args)`, cada **retorno** imprime `← valor`, y cada
**asignación** ejecutada imprime `  lugar ← valor`, todo con sangría según la
profundidad de la pila. Convierte a Nemi en herramienta de *entendimiento* (p.
ej. ver iterar a Euclides), no solo de cálculo. Es de menor prioridad que §21.1–2.

---

## 22. Módulos de la biblioteca — código fuente y pruebas

Todo el código siguiente es Nemi **puro** sobre el núcleo (§20–§21). Las líneas
`afirma` son la **suite de aceptación** del módulo; sus valores esperados están
verificados contra las notas del curso y contra el intérprete.

### 22.1 `teoria_numeros.nemi`

```
# teoria_numeros.nemi — Unidad 2 (solo núcleo aritmético)

función mcd(a, b)
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

función mcm(a, b)
    regresa ⌊(a · b) / mcd(a, b)⌋
fin función

función primo(n)
    si n < 2
        regresa 0
    fin si
    para d ← 2 hasta ⌊√n⌋
        si n mod d = 0
            regresa 0
        fin si
    fin para
    regresa 1
fin función

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

función euclides_extendido(a, b)      # regresa [d, s, t] con s·a + t·b = d
    si b = 0
        regresa [a, 1, 0]
    fin si
    r ← euclides_extendido(b, a mod b)
    regresa [r[1], r[3], r[2] − ⌊a / b⌋ · r[3]]
fin función

función mod_inv(a, m)                  # inverso de a módulo m, o −1 si no existe
    r ← euclides_extendido(a mod m, m)
    si r[1] ≠ 1
        regresa −1
    fin si
    regresa r[2] mod m
fin función

# --- pruebas ---
afirma mcd(504, 396) = 36
afirma mcm(4, 6) = 12
afirma primo(97) = 1
afirma primo(51) = 0
afirma exp_mod(7, 13, 11) = 2
afirma exp_mod(572, 29, 713) = 113
afirma mod_inv(3, 7) = 5
afirma euclides_extendido(240, 46)[1] = 2
```

### 22.2 `conteo.nemi`

```
# conteo.nemi — Unidad 3

función factorial(n)
    resultado ← 1
    para i ← 2 hasta n
        resultado ← resultado · i
    fin para
    regresa resultado
fin función

función P(n, r)
    resultado ← 1
    para i ← 0 hasta r − 1
        resultado ← resultado · (n − i)
    fin para
    regresa resultado
fin función

función C(n, r)                        # forma iterativa: evita factoriales enormes
    resultado ← 1
    para i ← 1 hasta r
        resultado ← ⌊(resultado · (n − r + i)) / i⌋
    fin para
    regresa resultado
fin función

función pascal(n)                      # regresa el renglón n: [C(n,0), …, C(n,n)]
    fila ← [1]
    para k ← 1 hasta n
        agrega(fila, C(n, k))
    fin para
    regresa fila
fin función

función multinomial(n, ks)             # ks = [n1, …, nt] con suma n
    resultado ← factorial(n)
    para cada k en ks repite
        resultado ← ⌊resultado / factorial(k)⌋
    fin para
    regresa resultado
fin función

función genera_combinaciones(n, r)     # lista de las C(n,r) combinaciones (orden lexicográfico)
    s ← arreglo_cero(r)
    para i ← 1 hasta r
        s[i] ← i
    fin para
    res ← []
    agrega(res, copia(s))
    para c ← 2 hasta C(n, r)
        m ← r
        tope ← n
        mientras s[m] = tope
            m ← m − 1
            tope ← tope − 1
        fin mientras
        s[m] ← s[m] + 1
        para j ← m + 1 hasta r
            s[j] ← s[j − 1] + 1
        fin para
        agrega(res, copia(s))
    fin para
    regresa res
fin función

función genera_permutaciones(n)        # lista de las n! permutaciones (orden lexicográfico)
    s ← arreglo_cero(n)
    para i ← 1 hasta n
        s[i] ← i
    fin para
    res ← []
    agrega(res, copia(s))
    para c ← 2 hasta factorial(n)
        m ← n − 1
        mientras s[m] > s[m + 1]
            m ← m − 1
        fin mientras
        k ← n
        mientras s[m] > s[k]
            k ← k − 1
        fin mientras
        tmp ← s[m]
        s[m] ← s[k]
        s[k] ← tmp
        p ← m + 1
        q ← n
        mientras p < q
            tmp ← s[p]
            s[p] ← s[q]
            s[q] ← tmp
            p ← p + 1
            q ← q − 1
        fin mientras
        agrega(res, copia(s))
    fin para
    regresa res
fin función

# --- pruebas ---
afirma factorial(5) = 120
afirma P(10, 4) = 5040
afirma C(52, 5) = 2598960
afirma C(8, 4) = 70
afirma pascal(4) = [1, 4, 6, 4, 1]
afirma multinomial(11, [1, 4, 4, 2]) = 34650
afirma long(genera_combinaciones(6, 4)) = 15
afirma long(genera_permutaciones(4)) = 24
```

### 22.3 `conjuntos.nemi`

```
# conjuntos.nemi — Unidad 1 (sobre las primitivas de Conjunto, §20.2)

función complemento(A, U)
    regresa diferencia(U, A)
fin función

función subconjunto_propio(A, B)
    regresa subconjunto(A, B) ∧ ¬(A = B)
fin función

función potencia(A)                    # conjunto potencia P(A)
    res ← { ∅ }                        # conjunto que contiene al conjunto vacío
    para cada x en A repite
        nuevos ← ∅                     # se acumula aparte para no mutar 'res' al iterar
        para cada S en res repite
            agrega(nuevos, union(S, {x}))
        fin para
        para cada T en nuevos repite
            agrega(res, T)
        fin para
    fin para
    regresa res
fin función

función producto_cartesiano(A, B)      # conjunto de pares [a, b]
    res ← ∅
    para cada a en A repite
        para cada b en B repite
            agrega(res, [a, b])
        fin para
    fin para
    regresa res
fin función

# --- pruebas ---
afirma cardinalidad({3, 1, 2, 1}) = 3
afirma (2 ∈ {1, 2, 3}) = 1
afirma union({1, 2}, {2, 3}) = {1, 2, 3}
afirma cardinalidad(potencia({1, 2, 3})) = 8
afirma cardinalidad(producto_cartesiano({1, 2}, {3, 4, 5})) = 6
```

### 22.4 `relaciones.nemi`

```
# relaciones.nemi — Unidad 1 (relación = matriz cero-uno n×n, base 1)

función es_refleja(R, n)
    para i ← 1 hasta n
        si R[i][i] = 0
            regresa 0
        fin si
    fin para
    regresa 1
fin función

función es_simetrica(R, n)
    para i ← 1 hasta n
        para j ← 1 hasta n
            si R[i][j] ≠ R[j][i]
                regresa 0
            fin si
        fin para
    fin para
    regresa 1
fin función

función es_antisimetrica(R, n)
    para i ← 1 hasta n
        para j ← 1 hasta n
            si i ≠ j ∧ R[i][j] = 1 ∧ R[j][i] = 1
                regresa 0
            fin si
        fin para
    fin para
    regresa 1
fin función

función es_transitiva(R, n)
    para i ← 1 hasta n
        para j ← 1 hasta n
            para k ← 1 hasta n
                si R[i][j] = 1 ∧ R[j][k] = 1 ∧ R[i][k] = 0
                    regresa 0
                fin si
            fin para
        fin para
    fin para
    regresa 1
fin función

función composicion(R, S, n)           # producto booleano de matrices: S∘R
    T ← matriz_cero(n, n)
    para i ← 1 hasta n
        para j ← 1 hasta n
            para k ← 1 hasta n
                si R[i][k] = 1 ∧ S[k][j] = 1
                    T[i][j] ← 1
                fin si
            fin para
        fin para
    fin para
    regresa T
fin función

función cerradura_transitiva(R, n)     # algoritmo de Warshall (véase §17.12)
    W ← copia(R)
    para k ← 1 hasta n
        para i ← 1 hasta n
            para j ← 1 hasta n
                W[i][j] ← W[i][j] ∨ (W[i][k] ∧ W[k][j])
            fin para
        fin para
    fin para
    regresa W
fin función

# --- pruebas ---   (R = {(1,2),(2,3)} sobre {1,2,3})
afirma es_transitiva([[0,1,0],[0,0,1],[0,0,0]], 3) = 0
afirma es_refleja([[0,1,0],[0,0,1],[0,0,0]], 3) = 0
afirma cerradura_transitiva([[0,1,0],[0,0,1],[0,0,0]], 3)[1][3] = 1
```

### 22.5 `booleana.nemi`

```
# booleana.nemi — Unidad 4
# Una función booleana de n variables se representa como un arreglo t de 2^n
# bits: t[k+1] es el valor sobre la entrada k (0 ≤ k < 2^n), donde k en binario
# es (x1 x2 … xn) con x1 el bit más significativo (convención de mintérmino m_k).

función nand(a, b)
    regresa ¬(a ∧ b)
fin función

función nor(a, b)
    regresa ¬(a ∨ b)
fin función

función xor(a, b)
    regresa (a ∧ ¬b) ∨ (¬a ∧ b)
fin función

función xnor(a, b)
    regresa ¬xor(a, b)
fin función

función filas(n)                       # 2^n
    f ← 1
    para i ← 1 hasta n
        f ← f · 2
    fin para
    regresa f
fin función

función minterminos(t, n)              # lista de k con t[k+1] = 1
    res ← []
    para k ← 0 hasta filas(n) − 1
        si t[k + 1] = 1
            agrega(res, k)
        fin si
    fin para
    regresa res
fin función

función equivalentes(t1, t2, n)        # ¿misma tabla de verdad?
    para k ← 0 hasta filas(n) − 1
        si t1[k + 1] ≠ t2[k + 1]
            regresa 0
        fin si
    fin para
    regresa 1
fin función

función evalua(t, bits, n)             # valor de t sobre la entrada (bits[1..n])
    k ← 0
    para i ← 1 hasta n
        k ← k · 2 + bits[i]
    fin para
    regresa t[k + 1]
fin función

# --- pruebas ---
afirma xor(1, 1) = 0
afirma nand(1, 1) = 0
afirma nor(0, 0) = 1
afirma minterminos([0, 1, 1, 0], 2) = [1, 2]           # XOR de 2 variables
afirma equivalentes([0, 1, 1, 0], [0, 1, 1, 0], 2) = 1
afirma minterminos([0,0,0,1,0,1,1,1], 3) = [3, 5, 6, 7] # mayoría: c = ∑m(3,5,6,7)
afirma evalua([0,0,0,1,0,1,1,1], [1, 1, 0], 3) = 1
```

### 22.6 `cadenas.nemi`

Requiere las primitivas de cadena: `long(s)`, indexación `s[i]` (base 1,
devuelve un carácter = cadena de longitud 1), `concatena(s, t)`,
`texto(x)` (número → cadena) y `valor(c)` (carácter dígito → entero).

```
# cadenas.nemi — Unidades 1–2

función invierte(s)
    r ← ""
    para i ← 1 hasta long(s)
        r ← concatena(r, s[long(s) − i + 1])
    fin para
    regresa r
fin función

función prefijo(s, k)                  # primeros k caracteres
    r ← ""
    para i ← 1 hasta k
        r ← concatena(r, s[i])
    fin para
    regresa r
fin función

función sufijo(s, k)                   # últimos k caracteres
    r ← ""
    para i ← long(s) − k + 1 hasta long(s)
        r ← concatena(r, s[i])
    fin para
    regresa r
fin función

función a_binario(n)                   # entero ≥ 0 → cadena binaria
    si n = 0
        regresa "0"
    fin si
    r ← ""
    mientras n > 0
        r ← concatena(texto(n mod 2), r)
        n ← ⌊n / 2⌋
    fin mientras
    regresa r
fin función

función desde_base(s, b)               # cadena de dígitos en base b → entero
    v ← 0
    para i ← 1 hasta long(s)
        v ← v · b + valor(s[i])
    fin para
    regresa v
fin función

# --- pruebas ---
afirma invierte("abc") = "cba"
afirma prefijo("discreta", 4) = "disc"
afirma sufijo("discreta", 3) = "eta"
afirma a_binario(13) = "1101"
afirma desde_base("1101", 2) = 13
```

---

## 23. Casos límite adicionales (biblioteca y extensiones)

Complementan §16.

- **`afirma` falso:** abortar con archivo, línea, texto de la expresión y mensaje.
- **`agrega` sobre algo que no es lista ni conjunto:** error de tipo.
- **`∈`, `union`, `⊆`, etc. con un operando que no es conjunto:** error de tipo.
- **Igualdad entre tipos distintos** (p. ej. lista `=` conjunto): resultado `0`
  (no error), salvo que se prefiera error; **fijar y documentar** la elección.
- **Iterar (`para cada`) y mutar** la misma colección: comportamiento no definido;
  la biblioteca lo evita acumulando en otra colección (véase `potencia`).
- **`copia` superficial vs. profunda:** `copia` es **profunda**; sin ella, guardar
  `s` en una lista y seguir mutando `s` corrompería lo guardado.
- **Orden canónico** de conjuntos con elementos heterogéneos: debe ser total y
  determinista (§20.2), aunque la elección concreta es libre.
- **`mod_inv` sin inverso** (`mcd(a,m) ≠ 1`): devuelve `−1` (convención de la
  biblioteca), no error.

---

### Resumen para el siguiente implementador

El artefacto de partida es esta especificación de **Nemi**. Tareas: (1) implementar
el lexer y parser de la **sintaxis UTF-8** de la Parte II (gramática §10); (2) respetar las
decisiones de §8 (asignación `←` vs. igualdad `=`, bloques con `fin`, base 1,
bignum, paso por referencia de arreglos, `+`/`·` por tipo); (3) fijar el
tratamiento de las **acciones en prosa** (§14); (4) validar con el **corpus del
§17**, cuyas salidas ya están verificadas en las notas; y (5) implementar las
**extensiones del núcleo** de §20–§21 (tipo `Conjunto`, listas dinámicas,
`para cada`, igualdad estructural, `imprime`, `afirma`) y cargar la **biblioteca
estándar** de §22, cuyas líneas `afirma` son su suite de aceptación.

---

## 24. Historial de versiones

El documento se versiona con `mayor.menor`. Cambios incompatibles con
implementaciones previas suben la versión **menor** hasta llegar a la 1.0 (primer
intérprete que pase todo el corpus §17 **y** la biblioteca §22).

| Versión | Alcance |
|---|---|
| **0.1** | Especificación inicial del núcleo: Parte I (guía de lectura) y Parte II §8–§18 (léxico, gramática EBNF, tipos, semántica, `incluye`, acciones en prosa, casos límite, corpus de prueba §17). Define la sintaxis concreta UTF-8, `←` vs `=`, bloques con `fin`, base 1, bignum, paso por referencia y polisemia `+`/`·`. |
| **0.2** | *(esta versión)* Añade la **biblioteca estándar** y las extensiones que exige: §19 (arquitectura en dos capas y catálogo de módulos), §20 (literales de lista/conjunto, tipo `Conjunto`, `para cada`, listas dinámicas, igualdad estructural, retorno vía lista, funciones de primera clase [opcional]), §21 (`imprime` —resuelve §18.4—, `afirma`, `traza`), §22 (código fuente y pruebas de `teoria_numeros`, `conteo`, `conjuntos`, `relaciones`, `booleana`, `cadenas`) y §23 (casos límite de la biblioteca). No cambia nada de 0.1; solo agrega. **Aclaraciones (rev. tras revisión del implementador):** se resolvieron todas las preguntas abiertas de §18 (2/3/5 según el `README` del repo); §10 y §11 llevan ahora una nota que remite a las extensiones de §20 (antes §20 decía «se añaden a §10/§11» sin que esas secciones lo reflejaran); y se documentó el **doble uso del glifo `∅`** (literal de conjunto vacío en entrada vs. «sin valor» en la salida de `imprime`), decisión: aceptarlo y advertirlo (§20.1, fila nueva en §21.1). **Aclaración adicional (rev. tras implementación):** `∈` gana la palabra **`en`** como equivalente ASCII directo (`x en A`), reutilizando sin ambigüedad la palabra clave de `para cada … en …` (§20.3); ver §20.2. |
