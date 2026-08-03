# 10. La biblioteca común

## `incluye`: traer código de otro archivo

Ya escribiste `mcd` (capítulo 8), `factorial` (capítulo 5), y varias
funciones más — todas del tipo que **cualquier** curso de matemáticas
discretas repite constantemente. La instrucción `incluye "ruta.nemi"` te
deja usar funciones que ya viven en otro archivo, sin copiarlas:

```
incluye "bibcom/teoria_numeros.nemi"

imprime(mcd(504, 396))
imprime(primo(97))
```

```console
36
1
```

`incluye` pega el contenido del archivo indicado justo en ese punto —
como si hubieras copiado y pegado `teoria_numeros.nemi` completo ahí, pero
sin tener que hacerlo tú, y sin arriesgarte a copiar mal una línea. La ruta
es relativa a **dónde está tu propio archivo**: el ejemplo de arriba
funciona si tu archivo vive junto a la carpeta `bibcom/` (como en este
repositorio, donde tu archivo estaría en la raíz o en `examples/`, junto a
`bibcom/`).

Si tu archivo vive en **otra** carpeta (por ejemplo, guardaste tus tareas en
`Documentos\`, lejos de este repositorio) y no quieres copiar `bibcom/`
ahí, la opción `-I` le dice al intérprete dónde más buscar:

```console
nemi mi_tarea.nemi -I ruta/a/bibcom
```

`-I` se prueba **después** de la carpeta de tu propio archivo, así que nunca
cambia nada de lo que ya funciona — solo agrega un lugar más donde buscar.

## `bibcom/`: seis módulos ya escritos y comprobados

En vez de reescribir `mcd`, `factorial`, `C(n, r)` (combinaciones), o las
propiedades de una relación (reflexiva, simétrica, transitiva) cada vez que
las necesitas, `bibcom/` ya las trae — y cada una viene con sus propios
`afirma` (capítulo 8) como prueba de que están bien:

| Archivo | Unidad | Trae |
|---|---|---|
| `teoria_numeros.nemi` | 2 | `mcd`, `mcm`, `primo`, `exp_mod`, `euclides_extendido`, `mod_inv` |
| `conteo.nemi` | 3 | `factorial`, `P`, `C`, `pascal`, `multinomial`, generadores de combinaciones/permutaciones |
| `conjuntos.nemi` | 1 | `complemento`, `subconjunto_propio`, `potencia`, `producto_cartesiano` |
| `relaciones.nemi` | 1 | `es_refleja`, `es_simetrica`, `es_transitiva`, `es_antisimetrica`, `composicion`, `cerradura_transitiva` |
| `booleana.nemi` | 4 | `nand`, `nor`, `xor`, `xnor`, `minterminos`, `equivalentes`, `evalua` |
| `cadenas.nemi` | 1–2 | `invierte`, `prefijo`, `sufijo`, `a_binario`, `desde_base` |

Tabla completa con firma, descripción y ejemplo de cada función:
[**`bibcom/README.md`**](../bibcom/README.md).

Si necesitas varios módulos a la vez, `bibcom.nemi` los trae todos con un
solo `incluye`:

```
incluye "bibcom/bibcom.nemi"

imprime(C(52, 5))
imprime(xor(1, 0))
imprime(invierte("abc"))
```

```console
2598960
1
cba
```

## ¿Cuándo reescribir y cuándo usar `bibcom/`?

Para **aprender** cómo funciona un algoritmo (por ejemplo, entender el
algoritmo de Euclides paso a paso), escríbelo tú mismo, con `traza`
(capítulo 8) si hace falta — ese es el punto de los capítulos 1 al 9. Para
**usarlo** dentro de algo más grande (necesitas `mcd` como un paso de otra
función, o quieres confirmar el resultado de tu tarea), usa `bibcom/` — ya
está escrito, ya está comprobado, y reescribirlo solo introduce la
posibilidad de un error nuevo en algo que ya existe correctamente.

Los propios archivos de `bibcom/` son código corto y legible — ábrelos
directamente si quieres ver cómo está resuelto un algoritmo en particular
(son el mismo tipo de código que ya sabes leer después de este manual, no
cajas negras).

## Ejercicios

1. Escribe un archivo `.nemi` que incluya `teoria_numeros.nemi` y calcule
   `mod_inv(3, 7)` (el inverso de 3 módulo 7) — confirma el resultado con
   `afirma` (capítulo 8) contra el valor de la tabla en
   [`bibcom/README.md`](../bibcom/README.md).
2. Usa `conteo.nemi` para calcular `C(10, 3)` (combinaciones de 3 elementos
   de un total de 10) y confirma a mano que el resultado tiene sentido
   (¿es mayor o menor que `C(10, 5)`? ¿por qué?).
3. Compara tu propia `función es_palindromo` del capítulo 9 contra
   `invierte` de `cadenas.nemi`: escribe una versión de `es_palindromo` que
   use `invierte(s)` y compare con `s` directamente, en vez de recorrer
   carácter por carácter. ¿Cuál versión entiendes más rápido al releerla?
4. Abre `bibcom/relaciones.nemi` tú mismo y busca `cerradura_transitiva` —
   ¿reconoces el algoritmo de Warshall que se menciona en el comentario?

Siguiente: [**Referencia rápida**](referencia_rapida.md) — o, si ya
terminaste los diez capítulos, vuelve cuando quieras: la
[especificación completa](../Nemi.md) tiene todavía más detalle del que
cubrimos aquí.
