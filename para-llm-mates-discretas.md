# Para el LLM que está escribiendo las notas de Matemáticas Discretas

Hola. Soy el asistente que, junto con el usuario, construyó **Nemi**: un
pequeño lenguaje/intérprete pensado justo para el pseudocódigo que aparece en
notas como las tuyas. Te dejo esto por si te sirve mientras redactas.

## Qué es y por qué te puede servir

Nemi nació exactamente de este problema: un pseudocódigo en español, con
notación matemática (`←`, `≤`, `∧`, `⌊⌋`, `√`, índices base 1...), que hasta
ahora solo existía como texto en las notas y no se podía **ejecutar de
verdad** para comprobar que un algoritmo hace lo que el enunciado dice. Ahora
sí se puede: hay dos intérpretes completos y equivalentes (Python y C++17),
verificados contra un corpus de 16 ejemplos con salidas conocidas
(`factorial`, `fib`, `mcd`, `es_primo`, `exp_mod` para RSA, Warshall, etc.).

La especificación completa —gramática, tipos, precedencia de operadores,
semántica exacta— está en `Nemi.md` en la raíz de este repo. Si vas a citar o
adaptar pseudocódigo en tus notas y quieres que use la misma sintaxis que el
intérprete acepta, esa es la referencia autoritativa (no la adivines por los
ejemplos sueltos: hay detalles como que los bloques cierran con `fin si` /
`fin función` / `fin para`, no con un `fin` genérico).

## Sugerencia de uso con estudiantes

Para clase, lo más directo es **WinNemi** (`Windows/build/Debug/WinNemi.exe`
tras compilar, o pídele al usuario que te indique la ruta actual): es un
editor de texto tipo Bloc de notas (basado en el PopPad del libro de Petzold,
modernizado a Unicode) con una consola interactiva integrada debajo. El flujo
para un estudiante es:

1. Escribe o pega un algoritmo `.nemi` en el editor (arriba).
2. Presiona **F5** ("Ejecutar archivo") — corre el script y cualquier
   `imprime(...)` aparece en la consola de abajo.
3. Ya con las funciones cargadas, puede **experimentar en la consola**:
   llamar `factorial(5)` y ver `120`, o incluso encadenar pasos con variables
   persistentes: `r ← factorial(5)` (silencioso, es una asignación) y luego
   `r` en otra línea para ver el valor. Esto es nuevo — antes cada línea de
   consola era independiente; ahora hay una sesión que recuerda variables
   entre líneas, así que sirve para que el estudiante "juegue" con resultados
   intermedios sin tener que modificar el archivo.

Un detalle de sintaxis que confunde al principio: `=` es **comparación**, no
asignación — la asignación es `←` (o su alias de teclado `<-`). Vale la pena
advertirlo antes de que alguien lo escriba mal en la consola.

## Para tus propias pruebas (verificar pseudocódigo antes de publicarlo)

Si quieres comprobar que un algoritmo que estás describiendo en las notas da
el resultado que afirmas, no hace falta la GUI — el CLI es más rápido para
esto:

```console
# Python (referencia, vive en python/, corre desde ahí)
$ cd python
$ python -m nemi ../examples/factorial.nemi --llama "factorial(5)"
120

# C++ (mismo comportamiento, más rápido de arrancar)
$ ./cpp/build/nemi examples/factorial.nemi --llama "factorial(100)"
```

`--llama "expr"` evalúa una expresión contra las definiciones del archivo e
imprime el resultado (admite literales de arreglo `[...]` y de cadena).
`--lexemas` vuelca los tokens y `--asa` el árbol sintáctico — útiles si
escribiste algo y el parser se queja, para ver exactamente dónde se rompió la
gramática. `--ayuda` lista todas las opciones.

Un flujo práctico para ti: si estás redactando un ejemplo nuevo (pongamos,
una variante de Warshall o un algoritmo de la unidad que estés escribiendo),
puedes escribirlo como `.nemi`, correrlo con `--llama` sobre el caso de
prueba que vas a poner en las notas, y confirmar el número antes de
imprimirlo en el texto — exactamente el mismo método que usamos para construir
el corpus de aceptación en `examples/` (cada archivo ahí tiene su salida
verificada documentada en la tabla del `README.md` de la raíz).

Si te da algún error raro de sintaxis, antes de asumir que es un bug del
intérprete, contrasta contra `Nemi.md` — casi siempre es un detalle de
gramática (cierres de bloque explícitos, `entonces` opcional pero `fin si`
obligatorio, etc.), no un límite real del lenguaje.

Suerte con las notas — el lenguaje está para servirles a ustedes, así que si
algo no alcanza (una construcción que necesites y Nemi no soporte), díselo al
usuario; probablemente se pueda extender.
