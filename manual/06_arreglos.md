# 6. Arreglos

## Una lista de valores, con un solo nombre

Un **arreglo** guarda varios valores bajo un solo nombre, en orden:

```
notas ← [8, 9, 7, 10, 6]
imprime(notas)
imprime(notas[1])
imprime(notas[3])
```

```console
[8, 9, 7, 10, 6]
8
7
```

`notas[1]` es el **primer** elemento — en Nemi los arreglos empiezan a
contarse en **1**, no en 0. Esto es a propósito: es como cuenta la
matemática de toda la vida ("el primer término de la sucesión", `a₁`, no
`a₀`), y es como está escrito el pseudocódigo del curso. Si vienes de otro
lenguaje de programación donde el primer elemento es `[0]`, este es el
detalle que más te va a traicionar al principio — tenlo presente.

## Modificar un elemento

```
notas[1] ← 10
imprime(notas)
```

```console
[10, 9, 7, 10, 6]
```

## Recorrer un arreglo con `para`

Combina lo que ya sabes: un arreglo tiene un tamaño (`long(arreglo)`, "cuán
largo es"), así que puedes recorrerlo con un `para` de 1 hasta ese tamaño:

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

notas ← [8, 9, 7, 10, 6]
imprime(máximo(notas, long(notas)))
```

```console
10
```

Esta es exactamente `máximo.nemi` del corpus de ejemplos del proyecto — el
mismo algoritmo que verías descrito en prosa en unas notas de clase, pero
ejecutable.

## Los arreglos son "por referencia"

Este es otro detalle importante: cuando pasas un arreglo a una función y esa
función lo modifica, el cambio **sí se nota afuera** (a diferencia de un
número o una cadena, que se pasan "por valor" — una copia).

```
procedimiento duplica_primero(s)
    s[1] ← s[1] · 2
fin procedimiento

números ← [5, 10, 15]
duplica_primero(números)
imprime(números)
```

```console
[10, 10, 15]
```

`números` cambió **aunque `duplica_primero` nunca hizo `regresa`** — porque
`s` dentro del procedimiento y `números` afuera son, en la práctica, la
misma tabla en memoria, solo con dos nombres distintos. Esto es justo lo que
hace posible que existan procedimientos como "ordena este arreglo en su
lugar" sin tener que regresar un arreglo nuevo cada vez.

## Matrices: arreglos de arreglos

Una matriz es un arreglo cuyos elementos son, a su vez, arreglos:

```
tabla ← [[1, 2], [3, 4]]
imprime(tabla[1][2])
imprime(tabla[2][1])
```

```console
2
3
```

`tabla[1]` es el primer renglón (`[1, 2]`); `tabla[1][2]` es el segundo
elemento de ese renglón (`2`). Sigue siendo base 1 en ambos índices, como
`Rᵢⱼ` en notación matemática de matrices.

## Cuando el índice se sale del rango

```
notas ← [8, 9, 7]
imprime(notas[0])
```

```console
nemi: índice 0 fuera de rango 1..3
```

Nemi no te deja leer una posición que no existe — ni `[0]` (recuerda: base
1) ni `[4]` en un arreglo de 3 elementos. Es un error, no un `∅` silencioso;
mejor enterarte de inmediato que arrastrar un bug.

## Ejercicios

1. Escribe `función suma_arreglo(s, n)` que sume todos los elementos de `s`
   (usa el patrón acumulador del capítulo 4, adaptado a recorrer un
   arreglo).
2. Escribe un procedimiento que reciba un arreglo y un índice, e
   intercambie ese elemento con el siguiente (pista: vas a necesitar una
   variable temporal para no perder un valor al sobreescribirlo — o revisa
   si `intercambia(a, b)` te sirve directamente).
3. Con la matriz `[[1, 2, 3], [4, 5, 6], [7, 8, 9]]`, escribe un programa
   que imprima la suma de la diagonal (`tabla[1][1] + tabla[2][2] +
   tabla[3][3]`).
4. ¿Qué crees que pasa si haces `otro ← notas` (sin corchetes) y luego
   modificas `otro[1]`? ¿Cambia también `notas`? Pruébalo — la respuesta se
   conecta directo con la sección "por referencia" de arriba.

Siguiente: [**Capítulo 7 — Conjuntos**](07_conjuntos.md)
