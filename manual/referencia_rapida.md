# Referencia rápida de Nemi

Cheat-sheet — sin explicaciones, solo sintaxis. Si algo aquí no te hace
sentido, está explicado con calma en el [manual](00_indice.md) (columna
"capítulo") o en [`Nemi.md`](../Nemi.md) (spec completa).

## Estructura de un programa

```
función NOMBRE(p1, p2)      procedimiento NOMBRE(p1, p2)
    ...                         ...
    regresa VALOR                # sin regresa, o "regresa" sin valor
fin función                  fin procedimiento

# instrucciones sueltas (fuera de toda función) se ejecutan de arriba a abajo
```

Comentarios: `# esto es un comentario, hasta el final de la línea`.

## Asignación y comparación

| | Símbolo | ASCII | Capítulo |
|---|---|---|---|
| Asignar | `←` | `<-` | 2 |
| Igual a | `=` | `=` | 2 |
| Distinto de | `≠` | `!=` | 3 |
| Menor / mayor | `<` `>` | `<` `>` | 3 |
| Menor o igual / mayor o igual | `≤` `≥` | `<=` `>=` | 3 |

## Aritmética

| Operación | Símbolo | ASCII | Nota |
|---|---|---|---|
| Suma | `+` | `+` | bit+bit = OR (cap. 3) |
| Resta | `−` | `-` | |
| Producto | `·` | `*` | bit·bit = AND (cap. 3) |
| División exacta | `/` | `/` | da fracción, no decimal (cap. 2) |
| Residuo | `mod` | `mod` | exige enteros |
| Piso | `⌊x⌋` | — | entero ≤ x, el más grande |
| Techo | `⌈x⌉` | — | entero ≥ x, el más chico |
| Raíz | `√x` | — | exacta si x es cuadrado perfecto |

## Lógica

| Operación | Símbolo | Palabra | Nota |
|---|---|---|---|
| Y | `∧` | `y` | cap. 3 |
| O | `∨` | `o` | cap. 3 |
| No | `¬` | `no` | cap. 3 |

## Bloques de control

```
si CONDICIÓN                 mientras CONDICIÓN
    ...                          ...
alt                          fin mientras
    ...
fin si                       para VAR ← INICIO hasta FIN
                                  ...
                              fin para

para cada VAR en COLECCIÓN repite
    ...
fin para
```

`alt` es opcional. `entonces` (después de la condición de `si`) es opcional
y no cambia nada. `repite` (después de `para cada ... en ...`) también es
opcional.

## Funciones

```
función f(x, y) ... regresa x + y fin función
f(3, 4)                          # llamar

intercambia(a, b)                 # swap de dos variables/celdas

afirma EXPR                       # aborta si EXPR es falsa
afirma EXPR, "mensaje"

traza EXPR                        # ejecuta EXPR mostrando cada llamada/regreso/asignación
```

## Arreglos y matrices (base 1)

```
s ← [10, 20, 30]      s[1]           →  10 (primer elemento)
m ← [[1,2],[3,4]]     m[2][1]        →  3
s[1] ← 99                            # modificar
long(s)                              # tamaño
agrega(s, 40)                        # crece el arreglo, por referencia
copia(s)                             # copia independiente (profunda)
arreglo_cero(n)                      # arreglo de n ceros
matriz_cero(m, n)                    # matriz m×n de ceros
```

## Conjuntos

```
A ← {1, 2, 3}          ∅ o {}         # conjunto vacío (como literal)
x ∈ A     x en A     pertenece(x, A)   # pertenencia (las tres son iguales)
x ∉ A                                 # no pertenencia
A ⊆ B     subconjunto(A, B)           # subconjunto
A ⊂ B                                 # subconjunto propio
unión(A, B)    intersección(A, B)    diferencia(A, B)
cardinalidad(A)                       # tamaño
para cada x en A repite ... fin para  # recorrer
```

`∅` **impreso** (no como literal) significa "sin valor", no conjunto
vacío — un conjunto vacío se imprime como `{}` (cap. 7).

## Cadenas

```
s ← "hola"
long(s)          s[1]            concatena(s, "!")
texto(42)         # número -> cadena, "42"
valor("5")        # dígito -> número, 5
```

## Primitivas sueltas

| Primitiva | Qué hace |
|---|---|
| `imprime(v1, v2, ...)` | Muestra valores separados por espacio + salto de línea |
| `piso(x)` / `techo(x)` | Alias función de `⌊x⌋` / `⌈x⌉` |
| `raíz(x)` / `raiz(x)` | Alias función de `√x` |
| `abs(x)` | Valor absoluto |
| `long(v)` | Tamaño (arreglo, conjunto o cadena) |

## Errores comunes (mensaje → qué significa)

| Mensaje | Causa típica |
|---|---|
| `variable no definida: X` | Usaste `X` antes de asignarle algo (¿te faltaron las comillas de una cadena?) |
| `índice N fuera de rango 1..M` | Índice fuera de rango — recuerda, empieza en 1 |
| `el operador 'mod' requiere enteros` | Le diste a `mod` algo que no es entero (¿viene de una `/` sin `⌊ ⌋`? cap. 4) |
| `afirma falsa: ...` | Un `afirma` que escribiste tú mismo no se cumplió (cap. 8) |
| `desbordamiento de la pila de llamadas en F` | `F` es recursiva y nunca llega a su caso base (cap. 5) |

---
[**← Volver al índice del manual**](00_indice.md)
