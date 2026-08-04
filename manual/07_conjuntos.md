# 7. Conjuntos

## El tipo `Conjunto`

Un **conjunto** es una colección de valores **sin orden** y **sin
repeticiones** — exactamente la definición de teoría de conjuntos. Se
escribe con llaves:

```
A ← {1, 2, 3, 2, 1}
imprime(A)
```

```console
{1, 2, 3}
```

Nota que los `2` y el segundo `1` desaparecieron: un conjunto no tiene
repetidos, así que Nemi los colapsa automáticamente al crearlo — igual que
`{1, 2, 3, 2, 1}` en notación matemática es literalmente el mismo conjunto
que `{1, 2, 3}`.

## Pertenencia: `∈`

```
A ← {1, 2, 3}
imprime(3 ∈ A)
imprime(5 en A)
imprime(pertenece(3, A))
```

```console
verdadero
falso
verdadero
```

`∈` no tiene un equivalente ASCII de un solo carácter (a diferencia de
`≤`/`≠`) — si no puedes escribir el símbolo, tienes dos alternativas
idénticas: la palabra **`en`** (`x en A`, tal cual, tan corto como el
símbolo) o la función `pertenece(x, A)`. `∉` (no pertenece) solo tiene la
forma simbólica — no hay palabra equivalente para la negación.

## Subconjuntos y operaciones

```
A ← {1, 2, 3}
B ← {2, 3, 4}
imprime(A ⊆ {1, 2, 3, 4})   # ¿A es subconjunto?
imprime(unión(A, B))
imprime(intersección(A, B))
imprime(diferencia(A, B))
imprime(cardinalidad(A))     # tamaño del conjunto
```

```console
verdadero
{1, 2, 3, 4}
{2, 3}
{1}
3
```

`unión`/`intersección`/`diferencia` son las operaciones ∪/∩/∖ de teoría de
conjuntos, pero se escriben como **funciones** (no hay un símbolo `∪` que
puedas teclear directo en Nemi — es una decisión del lenguaje para no
saturar el teclado de símbolos poco comunes).

## `para cada`: recorrer un conjunto (o un arreglo)

Ya viste `para VARIABLE ← INICIO hasta FIN` en el capítulo 4, que sirve para
contar. `para cada` es distinto: recorre **los elementos** de una colección,
sin que te importe en qué posición está cada uno (los conjuntos ni siquiera
tienen "posición" — no están ordenados).

```
A ← {1, 2, 3, 4, 5}
suma ← 0
para cada x en A repite
    suma ← suma + x
fin para
imprime(suma)
```

```console
15
```

`para cada` también funciona sobre arreglos (capítulo 6), recorriendo cada
elemento en orden.

Nota que la palabra `en` aparece en dos construcciones que se **parecen**
pero no son lo mismo: `x en A` (sección anterior) es una **expresión** que
da `verdadero`/`falso`; `para cada x en A repite` es una **instrucción
completa** que repite un bloque. Nemi distingue una de otra por dónde
aparece `en` en la línea (justo después de `para cada`, o no), así que
nunca hay ambigüedad real — pero si algo no compila como esperabas, vale la
pena confirmar cuál de las dos querías escribir.

## El conjunto vacío: `∅` vs. `{}`

`∅` (o `{}`, son lo mismo **como literal de entrada**) es el conjunto sin
elementos. Pero cuidado: cuando Nemi **imprime** algo, `∅` significa otra
cosa — "esta llamada no regresó ningún valor" (por ejemplo, un
`procedimiento` sin `regresa`):

```
procedimiento nada()
fin procedimiento
imprime(nada())    # ∅  -- "no hay valor que mostrar"
imprime(∅)          # {} -- el conjunto vacío, impreso siempre como {}
```

```console
∅
{}
```

Es el mismo glifo con dos significados según el contexto (al escribir tu
código, `∅` siempre es el conjunto vacío; al leer una salida impresa, `∅`
siempre es "sin valor", nunca el conjunto vacío). La propia especificación
de Nemi señala esta ambigüedad explícitamente — no es que se te esté
escapando algo, es una decisión de diseño documentada.

## ¿Por qué esto vale la pena?

Antes de esta biblioteca, para trabajar con conjuntos en pseudocódigo tenías
que simularlos con arreglos y revisar duplicados a mano — perdiendo de vista
la idea matemática detrás. Con `Conjunto` de verdad, el código se lee (y se
escribe) casi igual que la notación de la clase: `A ⊆ B`, `x ∈ A`,
`|A|` (`cardinalidad(A)`). El capítulo 10 te muestra `conjuntos.nemi` y
`relaciones.nemi` de la biblioteca común, que ya traen construidas
`potencia` (conjunto potencia), `producto_cartesiano`, y las propiedades de
relaciones (reflexiva, simétrica, transitiva) sobre exactamente este tipo.

## Ejercicios

1. Dado `A ← {1, 2, 3}` y `B ← {3, 4, 5}`, calcula `A ∪ B`, `A ∩ B`, y
   `A \ B` usando las funciones de este capítulo, sin adivinar el
   resultado antes de correrlo.
2. Escribe una función que reciba un conjunto y regrese `verdadero` si
   `∅` (el conjunto vacío) es subconjunto de él. (Pista: matemáticamente
   siempre es cierto — ¿tu función lo confirma para cualquier conjunto que
   le pases?)
3. Usa `para cada` para contar cuántos elementos de un conjunto son pares
   (necesitas una variable acumuladora, como en el capítulo 4, más `mod`
   del capítulo 2).
4. ¿Qué imprime `imprime({1, 2} = {2, 1})`? Piensa en la definición de
   conjunto (sin orden) antes de correrlo.

Siguiente: [**Capítulo 8 — Verificar tu trabajo**](08_verificacion.md)
