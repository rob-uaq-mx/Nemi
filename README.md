# Nemi

**Nemi** (del náhuatl *nemi*, «andar, ejecutarse») es un pequeño pseudolenguaje
para describir y **ejecutar** algoritmos, con palabras clave en español y notación
matemática **UTF-8**. Nació del pseudocódigo usado en unas notas de *Matemáticas
Discretas*; este repositorio busca convertirlo en un **intérprete** funcional.

> **Estado:** especificación completa, con **dos intérpretes completos y
> equivalentes**: uno en Python 3 y otro en C++17 (`cpp/`, ver
> [`cpp/backlog.md`](cpp/backlog.md) — las 6 fases del port están hechas y
> verificadas, 16/16 en el corpus de aceptación). La especificación es
> [`Nemi.md`](Nemi.md); el diseño y las notas de portabilidad a C++ están en
> [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Contenido del repositorio

Las dos implementaciones son hermanas y **comparten** la especificación y el
corpus de ejemplos de la raíz:

```
Nemi/
├── README.md          ← este archivo
├── Nemi.md            ← especificación completa (guía + spec del intérprete)
├── examples/          ← corpus de programas .nemi con salidas verificadas (COMPARTIDO)
├── docs/              ← ARCHITECTURE.md (diseño + guía de portación a C++)
├── python/            ← intérprete de referencia en Python 3
│   ├── nemi/          ←   el paquete (lexer, parser, evaluador, CLI)
│   └── tests/         ←   suite de aceptación del §17 (run_corpus.py)
└── cpp/               ← intérprete completo en C++17 (ver cpp/README.md)
```

## Uso (Python)

El paquete `nemi` vive en `python/`, así que ejecuta el CLI **desde ese
directorio** (o añade `python/` al `PYTHONPATH`). El corpus está en la raíz,
por lo que desde `python/` se referencia como `../examples/...`:

```console
$ cd python
$ python -m nemi ../examples/maximo.nemi --llama "máximo([3, 9, 4], 3)"
9

$ python -m nemi ../examples/exp_mod.nemi --llama "exp_mod(572, 29, 713)"
113
```

`python -m nemi ARCHIVO.nemi [...] [--llama "expr"] [--lexemas] [--asa]`. Cada
`--llama` evalúa una expresión (admite literales de arreglo `[...]` y de cadena)
contra las definiciones cargadas e imprime el resultado; `--lexemas` vuelca los
tokens y `--asa` el árbol sintáctico abstracto. `ARCHIVO.nemi` puede ser
cualquier ruta (relativa o absoluta) a tu archivo. En Windows, si la consola no
muestra bien el UTF-8, usa `set PYTHONUTF8=1`. (Opciones en español porque Nemi
es un lenguaje en español; ejecuta `python -m nemi --ayuda` para verlas.)

Como biblioteca (con `python/` en el path):

```python
import nemi
interp = nemi.load_files(["../examples/mcd.nemi"])
print(interp.call("mcd", [504, 396]))   # 36
```

## Uso (C++)

El intérprete en `cpp/` es completo (lexer, parser, evaluador, bignum propio,
CLI en español) y pasa `ctest` con **16/16**. Ver [`cpp/README.md`](cpp/README.md):

```console
$ cmake -S cpp -B cpp/build -G Ninja
$ cmake --build cpp/build
$ ctest --test-dir cpp/build

$ ./cpp/build/nemi ../examples/factorial.nemi --llama "factorial(100)"
```

Para correr **ambas** suites (Python + C++) de una sola vez en local, sin
necesitar git: `.\check_all.ps1` desde la raíz del repo. El repo aún no es un
repositorio git; `.github/workflows/ci.yml` queda listo para cuando se
inicialice y se suba.

## Nemi en un vistazo

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

Características y decisiones de diseño (todas detalladas en `Nemi.md`):

- **UTF-8**: símbolos `←  =  ≠  <  ≤  >  ≥  +  −  ·  /  mod  ¬  ∧  ∨  ⌊ ⌋  ⌈ ⌉  √`
  y palabras clave acentuadas (`función`, `regresa`, …). Se aceptan equivalentes de
  teclado (`<-`, `!=`, `<=`, `>=`, `*`). Los conectores lógicos son en español:
  `y`/`o`/`no` o `∧`/`∨`/`¬` (no `and`/`or`/`not`).
- **Asignación** `←`, **igualdad** `=` (tokens distintos, sin ambigüedad).
- **Bloques con cierres explícitos**: `fin si`, `fin para`, `fin mientras`,
  `fin función`, `fin procedimiento`.
- **Índices base 1** (`s[1]` es el primer elemento).
- **Enteros de precisión arbitraria** (bignum): requerido por RSA y factoriales.
- **Producto explícito** con `·` (sin yuxtaposición `xy`).
- **`+` y `·` polisémicos**: sobre bits/booleanos son OR/AND; sobre números son
  suma/producto. Se resuelven por tipo de operando.
- **Arreglos/matrices/cadenas por referencia** (hay algoritmos que los mutan).
- **Acciones en prosa** entre `« … »`: pasos en lenguaje natural **no ejecutables**
  por el núcleo (p. ej. geometría del enlosado con trominos).
- **`incluye "ruta.nemi"`**: empalma otro archivo `.nemi` en el punto donde
  aparece (recursivo, detecta ciclos, los errores señalan el archivo correcto) —
  permite una pequeña "biblioteca estándar" de funciones reutilizables
  (`examples/biblioteca.nemi` + `examples/usa_biblioteca.nemi`).

La **gramática EBNF**, los tipos, la precedencia de operadores, la semántica y los
casos límite están en `Nemi.md` (Parte II).

## Corpus de ejemplos (suite de aceptación)

Los archivos de `examples/` provienen de las notas del curso y sus resultados ya
están **verificados** en el texto. Son ideales como pruebas de regresión.

| Archivo | Qué calcula | Prueba (entrada → salida) |
|---|---|---|
| `maximo.nemi` | Máximo de una sucesión | `máximo([3,9,4], 3)` → `9` |
| `busca_texto.nemi` | Buscar patrón en texto | `busca_texto("001",3,"010001",6)` → `4` |
| `insercion_por_orden.nemi` | Ordenamiento por inserción (in situ) | `[34,20,19,5]` → `[5,19,20,34]` |
| `es_primo.nemi` | Primalidad (0 = primo, o un factor) | `es_primo(97)` → `0`; `es_primo(51)` → `3` |
| `mcd.nemi` | MCD (Euclides) | `mcd(504,396)` → `36` |
| `factorial.nemi` | Factorial recursivo | `factorial(5)` → `120` |
| `fib.nemi` | Fibonacci recursivo | `fib(5)` → `5` |
| `factorial_iter.nemi` | Factorial iterativo | `factorial_iter(6)` → `720` |
| `fib_iter.nemi` | Fibonacci iterativo | `fib_iter(10)` → `55` |
| `exp_rapida.nemi` | Exponenciación rápida | `exp_rápida(2,10)` → `1024` |
| `exp_mod.nemi` | Exponenciación modular (RSA) | `exp_mod(572,29,713)` → `113` |
| `warshall.nemi` | Cerradura transitiva (Warshall) | matriz 4×4 → `R⁺` (Ej. 1.164) |
| `enlosa.nemi` | Enlosado con trominos | usa acciones en prosa (ver `Nemi.md §14`) |

> **Nota:** los nombres de archivo son ASCII por portabilidad, pero su **contenido
> es UTF-8** (incluye `función`, `←`, `≤`, `∧`, `⌊ ⌋`, …). Los identificadores de
> Nemi admiten letras Unicode, por eso funciones como `máximo` o `exp_rápida`
> conservan sus acentos.

## Estado de la implementación

El intérprete de referencia (en `python/nemi/`) cubre el **núcleo formal**
completo: lexer/parser de la sintaxis UTF-8 (`Nemi.md §10`), enteros bignum +
racionales exactos + reales, arreglos/matrices/cadenas base 1 por referencia,
despacho por tipo de `+`/`·`, `∧`/`∨` con corto-circuito, `⌊⌋`/`⌈⌉`/`√` (con
`⌊√n⌋` exacto), pila de llamadas para recursión, y las primitivas de
`Nemi.md §15` (`mod`, `piso`, `techo`, `raíz`, `abs`, `long`, `intercambia`,
`imprime`). La versión en `cpp/` implementa exactamente lo mismo (incluido su
propio bignum, sin dependencias externas) y da los mismos resultados.

Decisiones tomadas sobre las **preguntas abiertas** (`Nemi.md §18`): se
priorizaron los **cierres explícitos** para el análisis; `+`/`·` se resuelven
**por tipo** (no hay modo booleano aparte); las **acciones en prosa** `« … »` se
tratan como **no-ops** (por eso `enlosa` solo prueba la estructura de
recursión); se añadió `imprime` como E/S de conveniencia; los **índices son
base 1**. El detalle de arquitectura y la guía de portación a **C++** están en
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

### Pruebas

```console
$ python python/tests/run_corpus.py
...
21/21 checks passed.
```

Cada salida esperada proviene de las notas del curso (`Nemi.md §17`), más
comprobaciones extra de bignum y de las grafías ASCII de los operadores.
