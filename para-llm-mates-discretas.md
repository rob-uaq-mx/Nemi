# Para el LLM que está escribiendo las notas de Matemáticas Discretas

Hola. Soy el asistente que, junto con el usuario, construyó **Nemi**: un
pequeño lenguaje/intérprete pensado justo para el pseudocódigo que aparece en
notas como las tuyas. Te dejo esto por si te sirve mientras redactas.

## Qué es y por qué te puede servir

Nemi nació exactamente de este problema: un pseudocódigo en español, con
notación matemática (`←`, `≤`, `∧`, `⌊⌋`, `√`, índices base 1...), que hasta
ahora solo existía como texto en las notas y no se podía **ejecutar de
verdad** para comprobar que un algoritmo hace lo que el enunciado dice. Ahora
sí se puede: hay tres intérpretes completos y equivalentes (Python, y dos en
C++17 — uno en línea de comandos y otro con editor+consola para Windows),
verificados contra el corpus de `examples/` (`factorial`, `fib`, `mcd`,
`es_primo`, `exp_mod` para RSA, Warshall, etc.) **y** contra una biblioteca
común de funciones ya escritas y probadas — ver la siguiente sección,
probablemente lo más útil de esta nota para ti.

## La biblioteca común (`bibcom/`) — probablemente te ahorra trabajo

Desde la versión 0.2 del lenguaje, el repo trae seis módulos `.nemi` reales
(no solo ejemplos sueltos) con funciones que muy probablemente ya cubren lo
que estás describiendo en tus notas — están en `bibcom/` en la raíz:

| Archivo | Unidad del curso | Funciones |
|---|---|---|
| `teoria_numeros.nemi` | 2 | `mcd`, `mcm`, `primo`, `exp_mod`, `euclides_extendido`, `mod_inv` |
| `conteo.nemi` | 3 | `factorial`, `P`, `C`, `pascal`, `multinomial`, `genera_combinaciones`, `genera_permutaciones` |
| `conjuntos.nemi` | 1 | `complemento`, `subconjunto_propio`, `potencia`, `producto_cartesiano` |
| `relaciones.nemi` | 1 | `es_refleja`, `es_simetrica`, `es_transitiva`, `es_antisimetrica`, `composicion`, `cerradura_transitiva` |
| `booleana.nemi` | 4 | `nand`, `nor`, `xor`, `xnor`, `minterminos`, `equivalentes`, `evalua` |
| `cadenas.nemi` | 1–2 | `invierte`, `prefijo`, `sufijo`, `a_binario`, `desde_base` |

Si estás escribiendo un ejemplo que use, digamos, el algoritmo de Euclides
extendido o generar todas las combinaciones de un conjunto, **no lo
reescribas** — cita o usa directamente la función de `bibcom/`, ya está
verificada línea por línea contra la spec (cada módulo trae sus propios
`afirma` de aceptación, ver la siguiente sección). `incluye "bibcom.nemi"`
(desde donde `bibcom/` sea alcanzable por ruta relativa) trae los seis de
una vez; o `incluye "teoria_numeros.nemi"` si solo necesitas uno.

También desde v0.2, el lenguaje tiene el tipo `Conjunto` de verdad
(`{1, 2, 3}`, `∈`/`⊆`/`⊂` como operadores de verdad, `unión(A, B)`/
`intersección(A, B)`/`diferencia(A, B)` como funciones —
**ojo: `∪`/`∩`/`∖` son solo la notación matemática que usa la spec para
describirlas en prosa, no sintaxis Nemi válida; en código siempre son
llamadas a función**, `para cada x en A repite ... fin para`) — útil si tus
notas de teoría de conjuntos/relaciones quieren pseudocódigo ejecutable en
vez de solo notación matemática estática.

### `afirma`: para el flujo de "verificar antes de publicar" de la siguiente sección

La instrucción `afirma expr [, "mensaje"]` (dentro de un `.nemi`) aborta con
el archivo/línea/expresión exactos si `expr` no se cumple. En vez de calcular
un caso con `--llama` y copiar el número a mano a las notas, puedes escribir
el resultado que vas a publicar directamente como `afirma mcd(504, 396) = 36`
en el propio archivo — si algún día cambias el algoritmo y el número deja de
cuadrar, te enteras corriendo el archivo, no por inspección visual. Así están
escritos los seis módulos de `bibcom/` (revisa cualquiera de ellos como
ejemplo del patrón).

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

# bibcom/ funciona igual, tanto para probar como para reusar (ver arriba)
$ ./cpp/build/nemi bibcom/bibcom.nemi --llama "C(52, 5)"
2598960
```

`--llama "expr"` evalúa una expresión contra las definiciones del archivo e
imprime el resultado (admite literales de arreglo `[...]` y de cadena).
`--lexemas` vuelca los tokens y `--asa` el árbol sintáctico — útiles si
escribiste algo y el parser se queja, para ver exactamente dónde se rompió la
gramática. `--ayuda` lista todas las opciones.

Si prefieres dejar los `afirma` escritos en el propio archivo (ver la sección
de arriba) en vez de usar `--llama` caso por caso, corre el archivo sin más
flags — si algún `afirma` es falso, el intérprete aborta con el archivo,
línea y expresión exactos; si no imprime nada y termina con código 0, todos
se cumplieron.

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
