# `bibcom/` — biblioteca común de Nemi

Seis módulos `.nemi` con funciones ya escritas y **verificadas** (cada
archivo trae sus propias líneas `afirma` — ver `Nemi.md` §19–§22 para el
diseño y `backlog_v0.2.md` Fase 7 para cómo se verificaron). Si tu tarea o
tus notas necesitan algo de esta tabla, **no lo reescribas**: inclúyelo.

Si nunca usaste `afirma`/`Conjunto`/`para cada`/`incluye`, el
[manual](../manual/00_indice.md) los explica desde cero — en particular
[`manual/10_biblioteca_comun.md`](../manual/10_biblioteca_comun.md) es la
introducción a este mismo directorio.

## Cómo usarla

```
incluye "bibcom.nemi"        # trae los 6 módulos de una vez
```

o, si solo necesitas uno:

```
incluye "teoria_numeros.nemi"
```

`incluye` resuelve la ruta relativa al directorio del archivo que la escribe
(`Nemi.md` §10/§12) — así que tu propio archivo `.nemi` necesita estar en
`bibcom/`, tener una ruta relativa correcta hacia ahí, o pasar la carpeta con
`-I` al ejecutar `nemi` (p. ej. `nemi mi_archivo.nemi -I ruta/a/bibcom`) —
esta última es la forma más cómoda si tu archivo vive en otra carpeta. Ver
[`manual/10_biblioteca_comun.md`](../manual/10_biblioteca_comun.md) para
ejemplos con rutas.

Los ejemplos de la columna "Resultado" de las tablas de abajo son las líneas
`afirma` reales de cada archivo — puedes verificarlos tú mismo:

```console
$ cd python
$ python -m nemi ../bibcom/teoria_numeros.nemi --llama "mcd(504, 396)"
36
```

## `teoria_numeros.nemi` (Unidad 2)

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `mcd(a, b)` | Máximo común divisor (Euclides) | `mcd(504, 396)` | `36` |
| `mcm(a, b)` | Mínimo común múltiplo | `mcm(4, 6)` | `12` |
| `primo(n)` | `1` si `n` es primo, `0` si no | `primo(97)` | `1` |
| `exp_mod(a, n, z)` | `aⁿ mod z` (exponenciación rápida, para RSA) | `exp_mod(572, 29, 713)` | `113` |
| `euclides_extendido(a, b)` | `[d, s, t]` con `s·a + t·b = d` (`d = mcd(a,b)`) | `euclides_extendido(240, 46)[1]` | `2` |
| `mod_inv(a, m)` | Inverso de `a` módulo `m`, o `−1` si no existe | `mod_inv(3, 7)` | `5` |

## `conteo.nemi` (Unidad 3)

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `factorial(n)` | `n!` | `factorial(5)` | `120` |
| `P(n, r)` | Permutaciones de `r` de entre `n` (`n!/(n−r)!`) | `P(10, 4)` | `5040` |
| `C(n, r)` | Combinaciones de `r` de entre `n` (`n! / (r!(n−r)!)`, forma iterativa) | `C(52, 5)` | `2598960` |
| `pascal(n)` | Renglón `n` del triángulo de Pascal: `[C(n,0), …, C(n,n)]` | `pascal(4)` | `[1, 4, 6, 4, 1]` |
| `multinomial(n, ks)` | Coeficiente multinomial (`ks` = `[n1, …, nt]`, suman `n`) | `multinomial(11, [1, 4, 4, 2])` | `34650` |
| `genera_combinaciones(n, r)` | Lista de las `C(n,r)` combinaciones, orden lexicográfico | `long(genera_combinaciones(6, 4))` | `15` |
| `genera_permutaciones(n)` | Lista de las `n!` permutaciones, orden lexicográfico | `long(genera_permutaciones(4))` | `24` |

## `conjuntos.nemi` (Unidad 1)

Sobre el tipo `Conjunto` del núcleo (`Nemi.md` §20.2) — ver también
`pertenece`/`subconjunto`/`union`/`interseccion`/`diferencia`/`cardinalidad`,
que son primitivas del intérprete, no de este archivo.

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `complemento(A, U)` | `U \ A`, el complemento de `A` respecto al universo `U` | `complemento({1}, {1,2,3})` | `{2, 3}` |
| `subconjunto_propio(A, B)` | `verdadero` si `A ⊆ B` y `A ≠ B` | `subconjunto_propio({1}, {1,2})` | `verdadero` |
| `potencia(A)` | Conjunto potencia `P(A)` (todos los subconjuntos) | `cardinalidad(potencia({1, 2, 3}))` | `8` |
| `producto_cartesiano(A, B)` | Conjunto de pares `[a, b]` con `a ∈ A`, `b ∈ B` | `cardinalidad(producto_cartesiano({1, 2}, {3, 4, 5}))` | `6` |

## `relaciones.nemi` (Unidad 1)

Una relación en `{1, …, n}` se representa como matriz cero-uno `n×n`, base 1
(`R[i][j] = 1` si `i` está relacionado con `j`).

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `es_refleja(R, n)` | `1` si `R[i][i] = 1` para todo `i` | `es_refleja([[0,1,0],[0,0,1],[0,0,0]], 3)` | `0` |
| `es_simetrica(R, n)` | `1` si `R[i][j] = R[j][i]` siempre | `es_simetrica([[0,1],[1,0]], 2)` | `1` |
| `es_antisimetrica(R, n)` | `1` si nunca `R[i][j] = R[j][i] = 1` con `i ≠ j` | `es_antisimetrica([[1,1],[0,1]], 2)` | `1` |
| `es_transitiva(R, n)` | `1` si `R[i][j] ∧ R[j][k] ⟹ R[i][k]` siempre | `es_transitiva([[0,1,0],[0,0,1],[0,0,0]], 3)` | `0` |
| `composicion(R, S, n)` | `S∘R` (producto booleano de matrices) | `composicion([[0,1],[0,0]], [[0,0],[1,0]], 2)` | `[[1, 0], [0, 0]]` |
| `cerradura_transitiva(R, n)` | Cerradura transitiva (algoritmo de Warshall) | `cerradura_transitiva([[0,1,0],[0,0,1],[0,0,0]], 3)[1][3]` | `1` |

## `booleana.nemi` (Unidad 4)

Una función booleana de `n` variables se representa como un arreglo `t` de
`2ⁿ` bits: `t[k+1]` es el valor sobre la entrada `k` (`0 ≤ k < 2ⁿ`), donde
`k` en binario es `(x1 x2 … xn)` con `x1` el bit más significativo
(convención de mintérmino `mₖ`).

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `nand(a, b)` | `¬(a ∧ b)` | `nand(1, 1)` | `0` |
| `nor(a, b)` | `¬(a ∨ b)` | `nor(0, 0)` | `1` |
| `xor(a, b)` | O exclusivo | `xor(1, 1)` | `0` |
| `xnor(a, b)` | `¬xor(a, b)` | `xnor(1, 1)` | `1` |
| `filas(n)` | `2ⁿ` (tamaño de la tabla de verdad) | `filas(3)` | `8` |
| `minterminos(t, n)` | Lista de `k` con `t[k+1] = 1` | `minterminos([0, 1, 1, 0], 2)` | `[1, 2]` |
| `equivalentes(t1, t2, n)` | `1` si `t1`/`t2` son la misma tabla de verdad | `equivalentes([0,1,1,0], [0,1,1,0], 2)` | `1` |
| `evalua(t, bits, n)` | Valor de `t` sobre la entrada `bits[1..n]` | `evalua([0,0,0,1,0,1,1,1], [1, 1, 0], 3)` | `1` |

## `cadenas.nemi` (Unidades 1–2)

Usa las primitivas de cadena del núcleo (`long`, `s[i]`, `concatena`,
`texto`, `valor` — `Nemi.md` §22.6).

| Función | Qué hace | Ejemplo | Resultado |
|---|---|---|---|
| `invierte(s)` | Invierte una cadena | `invierte("abc")` | `"cba"` |
| `prefijo(s, k)` | Primeros `k` caracteres | `prefijo("discreta", 4)` | `"disc"` |
| `sufijo(s, k)` | Últimos `k` caracteres | `sufijo("discreta", 3)` | `"eta"` |
| `a_binario(n)` | Entero ≥ 0 → cadena binaria | `a_binario(13)` | `"1101"` |
| `desde_base(s, b)` | Cadena de dígitos en base `b` → entero | `desde_base("1101", 2)` | `13` |

---

Fuente completa de cada módulo: los propios archivos `.nemi` en este
directorio (código corto, léelos directamente — son el material de estudio,
no cajas negras, como dice `Nemi.md` §19.1).
