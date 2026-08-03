# 5. Funciones

## Función Nemi ↔ función matemática

En matemáticas, una función toma una o más entradas y produce **una**
salida: `f(x) = x²` toma `x` y da `x²`. Una función en Nemi hace exactamente
eso — es la misma idea, ejecutable:

```
función cuadrado(x)
    regresa x · x
fin función

imprime(cuadrado(5))
imprime(cuadrado(7))
```

```console
25
49
```

Piezas:

- `función NOMBRE(parámetros)` — empieza la definición. `x` es un
  **parámetro**: una variable que solo existe dentro de esta función, y que
  toma el valor que le pases cuando la llames (`cuadrado(5)` hace que, por
  dentro, `x` valga `5`).
- `regresa VALOR` — termina la función inmediatamente y entrega `VALOR`
  como resultado. Es exactamente el "= x²" de la notación matemática.
- `fin función` — obligatorio, como todos los `fin ...` que ya conoces.

Una vez definida, `cuadrado(5)` se usa como cualquier otro valor: puedes
imprimirlo, guardarlo en una variable, usarlo dentro de una cuenta más
grande (`cuadrado(3) + cuadrado(4)`).

## Funciones con varios parámetros

```
función area_rectangulo(base, altura)
    regresa base · altura
fin función

imprime(area_rectangulo(4, 5))
```

Los parámetros se llenan **en el orden en que los escribes** al llamar la
función: el primer valor que pasas va al primer parámetro, y así.

## `procedimiento`: cuando no hay un valor que regresar

A veces quieres que algo **suceda** (imprimir, modificar algo), no que se
calcule un valor. Para eso existe `procedimiento` — es igual que `función`
pero sin `regresa` (o con un `regresa` sin valor, para cortar antes de
tiempo):

```
procedimiento saluda(nombre)
    imprime("Hola,", nombre)
fin procedimiento

saluda("Ana")
saluda("Luis")
```

```console
Hola, Ana
Hola, Luis
```

La diferencia entre `función` y `procedimiento` es de **intención**, para
que quien lea tu código sepa qué esperar: una función se llama por su
resultado (`imprime(cuadrado(5))` tiene sentido), un procedimiento se llama
por su efecto (`saluda("Ana")` no regresa nada útil que imprimir).

## Recursión: una función que se llama a sí misma

Esta es la idea que más conecta programación con matemáticas discretas. Una
definición **recursiva** define algo en términos de una versión más pequeña
de sí mismo, más un caso base donde para:

> `factorial(n) = n · factorial(n − 1)`, y `factorial(0) = 1`.

Eso se traduce a Nemi casi palabra por palabra:

```
función factorial(n)
    si n = 0
        regresa 1
    fin si
    regresa n · factorial(n - 1)
fin función

imprime(factorial(5))
```

```console
120
```

Cómo lo ejecuta Nemi (síguelo con `factorial(3)` en tu cabeza): para
calcular `factorial(3)` necesita `factorial(2)`, que necesita `factorial(1)`,
que necesita `factorial(0)` — y ese último caso **no** necesita llamar a
nadie más, regresa `1` directo (es el "caso base"). Con eso, `factorial(1)`
puede terminar (`1 · 1 = 1`), luego `factorial(2)` (`2 · 1 = 2`), luego
`factorial(3)` (`3 · 2 = 6`).

**Toda función recursiva necesita un caso base** — si `factorial` no tuviera
el `si n = 0 ... regresa 1`, se llamaría a sí misma para siempre (con `n`
cada vez más negativo) y nunca terminaría. Es el equivalente, en recursión,
del ciclo infinito del capítulo 4.

Si alguna vez quieres **ver** una recursión ejecutarse paso a paso en vez de
imaginártela, adelántate a leer sobre `traza` en el
[capítulo 8](08_verificacion.md) — muestra exactamente esta secuencia de
llamadas en pantalla.

## Ejercicios

1. Escribe `función es_par(n)` que regrese `1` si `n` es par y `0` si no
   (usa `mod`, capítulo 2), y pruébala con varios números.
2. Escribe la función matemática `f(n) = n² + 1` como una función Nemi y
   evalúala en `f(0)`, `f(1)`, `f(2)`, `f(3)`.
3. Escribe `función suma_hasta(n)` **recursiva** que calcule `1 + 2 + ... +
   n` (pista: `suma_hasta(n) = n + suma_hasta(n − 1)`, con caso base
   `suma_hasta(0) = 0`). Compara `suma_hasta(10)` con el resultado del
   ejemplo "Acumular un resultado" del capítulo 4 (la suma de 1 a 10) —
   deben coincidir: son el mismo cálculo, uno con `para` y otro recursivo.
4. ¿Qué pasa si llamas `factorial(-1)` con la función de este capítulo?
   Antes de correrlo, piensa por qué (revisa el caso base).

Siguiente: [**Capítulo 6 — Arreglos**](06_arreglos.md)
