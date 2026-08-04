# 8. Verificar tu trabajo

## El problema que resuelve `afirma`

Hasta ahora, para saber si tu función está bien, la llamabas con `imprime` y
comparabas el resultado **a mano** contra lo que esperabas. Funciona, pero
tiene un problema: si mañana cambias la función y rompes algo, no te vas a
dar cuenta a menos que vuelvas a comparar a mano — y a la tercera o cuarta
función de tu tarea, ya no lo vas a hacer.

`afirma` deja la comprobación **escrita en el propio archivo**, para que se
revise sola cada vez que lo corras:

```
función cuadrado(x)
    regresa x · x
fin función

afirma cuadrado(5) = 25
imprime("Todo bien")
```

```console
Todo bien
```

Si la condición de `afirma` es cierta, no pasa nada — el programa sigue
normal (por eso ves "Todo bien"). El poder está en lo que pasa cuando **no**
lo es:

```
función cuadrado(x)
    regresa x · x
fin función

afirma cuadrado(5) = 30, "el cuadrado de 5 debería ser 25"
imprime("Todo bien")
```

```console
nemi: afirma falsa: (cuadrado(5) = 30) -- el cuadrado de 5 debería ser 25 (tarea.nemi, 4:1)
```

El programa **se detiene ahí mismo** — nunca llega al `imprime("Todo
bien")` — y el mensaje te dice exactamente qué esperabas (`cuadrado(5) =
30`), el mensaje que le pusiste, y el archivo y la línea exactos donde
falló. `afirma EXPRESIÓN, "mensaje"` — el mensaje entre comillas es
opcional, pero ayuda mucho cuando tienes varios `afirma` en el mismo
archivo y necesitas saber cuál específicamente falló.

## El flujo recomendado para tu tarea

En vez de escribir tu función y luego, en otra ventana, ir probando valores
sueltos, escribe la función **y** las comprobaciones en el mismo archivo,
usando los casos que ya conoces la respuesta (los del libro, los que el
profesor dio como ejemplo):

```
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

afirma mcd(504, 396) = 36
afirma mcd(105, 30) = 15
```

Si corres el archivo y no truena nada, las dos comprobaciones pasaron.
Sigue agregando `afirma` según avances — cada una que dejas escrita protege
contra que un cambio futuro rompa algo que ya tenías funcionando. Esto es,
literalmente, cómo está construida cada función de la
[biblioteca común](10_biblioteca_comun.md) que vas a usar más adelante:
código + sus propios `afirma` en el mismo archivo, como prueba de que
funciona.

## `traza`: ver una llamada paso a paso

Cuando una función recursiva (capítulo 5) no hace lo que esperabas, es fácil
perderse tratando de imaginar el orden exacto de las llamadas. `traza`
te lo muestra en pantalla, en vez de que lo imagines:

```
función factorial(n)
    si n = 0
        regresa 1
    fin si
    regresa n · factorial(n - 1)
fin función

traza factorial(3)
```

```console
→ factorial(3)
  → factorial(2)
    → factorial(1)
      → factorial(0)
      ← 1
    ← 1
  ← 2
← 6
```

Cada `→` es una llamada que empieza (con sus argumentos exactos), cada `←`
es lo que esa llamada regresó — y la sangría te muestra la profundidad: la
llamada más adentro (`factorial(0)`) es la que de verdad se resuelve
primero (el caso base), y las demás van "regresando" de adentro hacia
afuera. Es la manera más directa de confirmar que entendiste correctamente
cómo se desenvuelve una recursión, en vez de solo confiar en que el
resultado final salió bien.

## Probar cosas sueltas, sin escribir un archivo

Todo lo de arriba asume que la comprobación queda escrita en tu archivo con
`afirma`. Pero a veces solo quieres probar algo rápido — un valor, una
llamada — sin agregarlo permanentemente. Si usas **WinNemi**, la consola de
abajo también funciona como modo interactivo para justo eso: ver el
[**Apéndice: WinNemi**](apendice_winnemi.md#la-consola-como-modo-interactivo).

## Ejercicios

1. Toma la función `suma_hasta` que escribiste en el capítulo 5 y agrégale
   al menos tres `afirma` con casos que puedas calcular a mano (`n = 0`,
   `n = 1`, `n = 5`).
2. A propósito, escribe un `afirma` que sepas que va a fallar (por ejemplo,
   compara contra el valor equivocado) y confirma que el programa se
   detiene ahí — no sigas sin haberlo visto fallar al menos una vez, para
   que reconozcas el mensaje cuando te pase por accidente.
3. Usa `traza` sobre tu propia `suma_hasta(4)` recursiva y confirma que el
   orden de entradas/salidas coincide con lo que esperabas.
4. Escribe `afirma es_par(4)` usando la función del capítulo 5 — ¿qué
   necesita decir la condición para que tenga sentido, dado que `es_par`
   regresa `0`/`1` y no `verdadero`/`falso`?

Siguiente: [**Capítulo 9 — Cadenas de texto**](09_cadenas.md)
