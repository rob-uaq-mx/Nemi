# 2. Variables y tipos

## Variables: guardar un valor con un nombre

Una **variable** es un nombre que apunta a un valor. En matemáticas escribes
"sea `x = 5`" y luego usas `x` en vez de repetir `5`. En Nemi es casi igual,
salvo por un detalle importante: se usa la flecha **`←`** para asignar, no el
signo `=`.

```
x ← 5
imprime(x)
x ← x + 1
imprime(x)
```

```console
5
6
```

El punto es la mecánica de la última línea: `x ← x + 1` primero lee el valor
**actual** de `x` (5), le suma 1 (da 6), y **eso** pasa a ser el nuevo valor
de `x`. No es una ecuación ("x igual a x más uno" no tiene solución) — es una
instrucción: "calcula el lado derecho con el valor de ahora, y guárdalo".

Si tu teclado no tiene `←`, escribe `<-` (dos caracteres, guion bajo... no,
menos y mayor-que: `<` seguido de `-`). Nemi los trata exactamente igual:

```
x <- 5        # exactamente lo mismo que  x ← 5
```

## `=` no es asignación — es comparación

Este es el error más común al empezar. En Nemi:

- **`←`** (o `<-`) — asigna: "guarda este valor en esta variable".
- **`=`** — compara: "¿son iguales estos dos valores?" y te da `verdadero` o
  `falso` como respuesta, **sin cambiar nada**.

```
imprime(5 = 5)     # verdadero -- es una PREGUNTA, no una asignación
imprime(5 = 6)     # falso
```

Si escribes `x = 5` en vez de `x ← 5`, Nemi no se va a quejar de inmediato
(`=` es una expresión válida en muchos contextos) pero **no** vas a haber
guardado nada en `x`. Vas a ver esto en detalle en el capítulo 3, donde `=`
se usa constantemente para tomar decisiones.

## Los tipos que vas a usar

Nemi distingue estos tipos de valor:

| Tipo | Ejemplos | Para qué |
|---|---|---|
| **Entero** | `0`, `42`, `−7` | Contar, indexar, aritmética exacta (sin límite de tamaño) |
| **Real** | `3.5`, `0.1` | Cuando de verdad necesitas decimales (no exactos) |
| **Cadena** | `"hola"`, `"1101"` | Texto |
| **Bit** | `0`, `1` | Lógica (verdadero/falso *como número*, ver capítulo 3) |

No necesitas *declarar* el tipo de una variable — Nemi lo deduce del valor
que le asignes, y puedes reasignarle un valor de otro tipo después si
quieres (no es lo más ordenado, pero el lenguaje no te lo impide).

### Los enteros no tienen límite de tamaño

A diferencia de casi cualquier otro lenguaje, los enteros en Nemi **no
pierden precisión** por más grandes que sean — esto importa mucho en
matemáticas discretas, donde `factorial(100)` tiene 158 dígitos y necesitas
el número exacto, no una aproximación:

```console
$ python -m nemi ../examples/factorial.nemi --llama "factorial(100)"
93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000
```

## Aritmética

| Operador | Símbolo | ASCII | Ejemplo | Resultado |
|---|---|---|---|---|
| Suma | `+` | `+` | `2 + 2` | `4` |
| Resta | `−` | `-` | `10 - 4` | `6` |
| Producto | `·` | `*` | `2 · 3` | `6` |
| División | `/` | `/` | `7 / 2` | `7/2` |
| Residuo | `mod` | `mod` | `7 mod 2` | `1` |

La fila de división merece una pausa: `7 / 2` te da **`7/2` exacto**, no
`3.5`. Nemi no redondea entre enteros — si de verdad quieres la parte entera,
existen `⌊ ⌋` (piso) y `⌈ ⌉` (techo), que verás cuando te hagan falta. Esta
exactitud es a propósito: en un algoritmo como el de Euclides, redondear un
paso de más arruina el resultado final, y con enteros gigantescos (como
arriba) ni siquiera podrías notar el error a simple vista.

Cuando la división **sí** es exacta, el resultado se **imprime** como
entero solo, sin el `/1`:

```
imprime(6 / 2)   # 3, no 3/1
```

(ojo: que se *imprima* como `3` no significa que Nemi lo trate exactamente
igual que el entero `3` para todo — el capítulo 4 tiene un ejemplo concreto
donde esa diferencia importa.)

`mod` es el residuo de la división entera — la base de buena parte de
teoría de números (dos enteros son "congruentes módulo n" si tienen el mismo
residuo al dividir entre n; esto reaparece en criptografía RSA más adelante
en el curso).

## Ejercicios

1. Escribe un programa que guarde tu edad en una variable, la imprima, le
   sume 5, y vuelva a imprimirla con un mensaje que diga "en 5 años tendré".
2. Sin correrlo primero: ¿qué crees que imprime `imprime(9 / 3)`? ¿Y
   `imprime(10 / 3)`? Corre el programa y compara con lo que pensaste.
3. Calcula, usando `mod`, si el año actual es bisiesto sabiendo que la regla
   simplificada es "divisible entre 4" (no te preocupes por la excepción de
   los siglos). Pista: "divisible entre 4" es lo mismo que "el residuo de
   dividir entre 4 es 0".
4. ¿Qué da `imprime(1 / 3)`? ¿Y si luego escribes `imprime(1 / 3 + 1 / 3 +
   1 / 3)`? Este ejercicio es la razón por la que Nemi no usa decimales para
   la división entre enteros.

Siguiente: [**Capítulo 3 — Decisiones**](03_decisiones.md)
