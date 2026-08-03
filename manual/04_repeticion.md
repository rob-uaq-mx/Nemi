# 4. Repetición

## `para`: repetir un número conocido de veces

```
para i ← 1 hasta 5
    imprime(i)
fin para
```

```console
1
2
3
4
5
```

`para VARIABLE ← INICIO hasta FIN` crea una variable (aquí `i`) que empieza
en `INICIO`, ejecuta el bloque, le suma 1, lo vuelve a ejecutar, y así hasta
que `VARIABLE` pase de `FIN` (incluye el `FIN`: el ejemplo de arriba sí
imprime el 5). Usa `para` cuando **sabes de antemano** cuántas vueltas vas a
dar — por ejemplo, "para cada número del 1 al 10".

### Acumular un resultado

El patrón más común: una variable que empieza en un valor "neutro" (0 para
sumar, 1 para multiplicar) y se va actualizando en cada vuelta.

```
suma ← 0
para i ← 1 hasta 10
    suma ← suma + i
fin para
imprime("La suma de 1 a 10 es", suma)
```

```console
La suma de 1 a 10 es 55
```

Esto es exactamente la idea de una sumatoria `∑` que ves en clase — `para` es
cómo se ejecuta una sumatoria paso a paso en vez de solo escribirla.

## `mientras`: repetir mientras algo sea cierto

Cuando **no** sabes de antemano cuántas vueltas vas a necesitar (depende del
resultado de cada paso), usas `mientras`:

```
n ← 20
pasos ← 0
mientras n ≠ 1
    si n mod 2 = 0
        n ← ⌊n / 2⌋
    alt
        n ← 3 · n + 1
    fin si
    pasos ← pasos + 1
fin mientras
imprime("Se tardó", pasos, "pasos")
```

```console
Se tardó 7 pasos
```

Este es el proceso de Collatz: si el número es par, divídelo entre 2; si es
impar, multiplícalo por 3 y súmale 1; repite hasta llegar a 1. Nadie sabe de
antemano cuántos pasos va a tardar un número dado (es un problema abierto de
matemáticas) — por eso `mientras`, y no `para`, es la herramienta correcta
aquí.

`mientras CONDICIÓN ... fin mientras` revisa la condición **antes** de cada
vuelta (incluida la primera): si nunca es cierta, el bloque nunca se
ejecuta ni una vez; si nunca deja de ser cierta, el programa nunca termina
(un "ciclo infinito" — le va a pasar a todo mundo alguna vez, y la señal es
que tu programa no responde; hay que interrumpirlo y revisar la condición).

## Un tropiezo casi garantizado: `/` no te regresa un entero

Fíjate que arriba usamos `⌊n / 2⌋` (**piso** de `n / 2`), no `n / 2` a
secas. Recuerda del capítulo 2: `/` siempre te da el resultado **exacto**,
que Nemi guarda como fracción aunque se imprima como si fuera un entero.
Si luego intentas usar `mod` sobre ese resultado, truena:

```
n ← 20 / 2
imprime(n)          # se ve como "10"...
imprime(n mod 2)     # ...pero esto falla: el operador 'mod' requiere enteros
```

`n` **se imprime** como `10`, pero por dentro sigue siendo "la fracción
exacta 10/1", no un entero — y `mod` exige enteros de verdad. `⌊ ⌋` (piso)
sí te regresa un entero genuino, listo para volver a usarse con `mod`,
como índice de un arreglo (capítulo 6), etc. Regla práctica: si vas a seguir
haciendo aritmética entera con un resultado de `/`, envuélvelo en `⌊ ⌋`.

## Ejercicios

1. Con `para`, calcula el producto de los números del 1 al 6 (esto es
   `6!`, pero calcúlalo con el ciclo, no con una función todavía — esas
   vienen en el capítulo 5).
2. Con `mientras`, cuenta cuántas veces puedes dividir 100 entre 2 antes de
   que deje de ser divisible exactamente (usa `mod` para checar, y `⌊ ⌋`
   para dividir).
3. Modifica el ejemplo de Collatz para que además imprima cada valor de `n`
   por el que pasa (no solo el conteo final).
4. ¿Qué pasa si escribes `mientras 1 = 2` seguido de una instrucción y
   `fin mientras`? ¿Y si escribes `mientras 1 = 1`? No corras el segundo
   caso sin pensarlo primero — ¿por qué sería un problema?

Siguiente: [**Capítulo 5 — Funciones**](05_funciones.md)
