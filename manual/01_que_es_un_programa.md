# 1. ¿Qué es un programa?

## Un algoritmo es una receta

Antes de tocar una computadora: un **algoritmo** es una lista de pasos,
escritos en orden, que si los sigues exactamente como están (sin usar tu
criterio para "arreglar" ninguno) te llevan de un punto de partida a un
resultado. Una receta de cocina es un algoritmo. Las instrucciones para armar
un mueble son un algoritmo. "Para saber si un número es par, divídelo entre 2
y ve si sobra algo" es un algoritmo.

Un **programa** es un algoritmo escrito en un lenguaje que una computadora
puede seguir **sin ambigüedad**. Esa es la diferencia con una receta de
cocina: una receta dice "hornea hasta que esté dorado" y confía en que tú
sabes juzgar "dorado". Una computadora no puede juzgar nada que no esté
definido con precisión exacta — por eso los lenguajes de programación son
más rígidos que el español de todos los días.

**Ejecutar** un programa significa: la computadora sigue tus pasos, en el
orden en que los escribiste, uno a la vez, sin saltarse ninguno y sin
adivinar lo que quisiste decir.

## Tu primer programa

Crea un archivo de texto llamado `hola.nemi` con esta única línea:

```
imprime("Hola, Nemi")
```

Ejecútalo:

```console
$ python -m nemi hola.nemi
Hola, Nemi
```

(Si usas WinNemi: pega esa línea en el editor y presiona **F5**. El
resultado aparece en la consola de abajo.)

Acabas de escribir y ejecutar un programa. `imprime(...)` es una
**instrucción**: le dice a Nemi "muestra esto en pantalla". Todo lo que
pongas entre los paréntesis, entre comillas, aparece tal cual — el texto
`"Hola, Nemi"` se llama una **cadena** (una secuencia de caracteres).

## Varias instrucciones, en orden

```
imprime("Empezando...")
imprime("2 + 2 =", 2 + 2)
imprime("Listo.")
```

```console
$ python -m nemi varias.nemi
Empezando...
2 + 2 = 4
Listo.
```

Tres cosas para notar:

1. Las instrucciones se ejecutan **en el orden en que están escritas**, de
   arriba hacia abajo. Nemi nunca "adivina" que quieres ejecutar la tercera
   línea antes que la segunda.
2. `imprime` acepta **varias cosas separadas por comas** y las muestra
   separadas por un espacio, todas en la misma línea de salida.
3. `2 + 2` no está entre comillas, así que Nemi lo **calcula** (te da `4`),
   en vez de imprimir literalmente el texto `2 + 2`. Esta es la diferencia
   fundamental entre una cadena (`"..."`, texto tal cual) y una expresión que
   Nemi evalúa.

## ¿Por qué esto importa para matemáticas discretas?

Todo el curso de matemáticas discretas está lleno de afirmaciones del tipo
"si sigues este proceso, en algún momento llegas a este resultado" —
exactamente la definición de algoritmo de arriba. El algoritmo de Euclides
para el máximo común divisor, generar todas las permutaciones de un
conjunto, decidir si un número es primo: todos son "sigue estos pasos
exactos". La ventaja de escribirlos en Nemi en vez de solo leerlos en el
libro es que puedes **comprobar** que entendiste el proceso completo, sin
zonas grises — si te falta un paso o lo pusiste en el orden equivocado, Nemi
no "lo arregla" ni adivina: simplemente no te da el resultado correcto, o
truena con un error (como el que acabas de provocar si intentaste el
ejercicio 2 de abajo) — cualquiera de las dos es información real sobre tu
programa, no un obstáculo.

## Ejercicios

1. Escribe un programa que imprima tu nombre en una línea, y en la siguiente
   línea el resultado de `10 mod 3` (el residuo de dividir 10 entre 3) junto
   con un texto que diga qué es.
2. ¿Qué pasa si quitas las comillas de `"Hola, Nemi"` y dejas `imprime(Hola,
   Nemi)`? Pruébalo y lee el mensaje de error — no te lo expliques, solo
   obsérvalo; en el capítulo 2 vas a entender exactamente qué significa.
3. Escribe un programa con cinco instrucciones `imprime` distintas y
   confirma que salen en el orden en que las escribiste (parece obvio, pero
   vale la pena verlo con tus propios ojos una vez).

Siguiente: [**Capítulo 2 — Variables y tipos**](02_variables_y_tipos.md)
