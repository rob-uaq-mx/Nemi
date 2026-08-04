# Manual de Nemi — de cero a programar (y de paso, matemáticas discretas)

Este manual asume que **nunca has programado antes**. Tampoco asume que ya
sabes matemáticas discretas — de hecho, varios capítulos usan Nemi para
*entender* las ideas de matemáticas discretas ejecutándolas, no solo
leyéndolas. Si ya sabes programar en otro lenguaje, puedes ir directo a
[`referencia_rapida.md`](referencia_rapida.md) y volver aquí solo cuando algo
no te quede claro.

## Qué necesitas

Un intérprete de Nemi funcionando. Dos opciones:

- **WinNemi** (Windows, más fácil para empezar): un editor con una consola
  integrada abajo. Pídele a quien te compartió este repo el instalador
  (`Windows/installer/`), o compílalo tú (`Windows/README.md`).
- **La línea de comandos** (cualquier sistema, Python instalado):
  ```console
  $ cd python
  $ python -m nemi ../examples/factorial.nemi --llama "factorial(5)"
  120
  ```
  Todos los ejemplos de este manual usan este formato. Si ves rarezas con
  acentos o símbolos en Windows, corre `set PYTHONUTF8=1` primero.

No necesitas instalar nada más — ni compilador aparte, ni librerías. Nemi
trae todo lo que usarás en este manual.

## Cómo está organizado

Cada capítulo depende **solo** de los anteriores — no hay saltos. Si te
saltas uno, probablemente algo del siguiente no tenga sentido. Cada uno
termina con 2-4 ejercicios; resuélvelos antes de seguir, son la única forma
de que el conocimiento se quede.

| # | Capítulo | Qué aprendes |
|---|---|---|
| 1 | [¿Qué es un programa?](01_que_es_un_programa.md) | Qué es ejecutar un algoritmo, tu primer programa Nemi, `imprime` |
| 2 | [Variables y tipos](02_variables_y_tipos.md) | `←`, enteros/reales/cadenas/bits, aritmética, `mod` |
| 3 | [Decisiones](03_decisiones.md) | `si`/`alt`, comparar valores, `y`/`o`/`no` |
| 4 | [Repetición](04_repeticion.md) | `para`/`mientras`, acumular resultados |
| 5 | [Funciones](05_funciones.md) | `función`/`procedimiento`, parámetros, `regresa` |
| 6 | [Arreglos](06_arreglos.md) | Listas de valores, `[i]`, por qué empiezan en 1 |
| 7 | [Conjuntos](07_conjuntos.md) | El tipo `Conjunto`, `∈`/`⊆`, `para cada` |
| 8 | [Verificar tu trabajo](08_verificacion.md) | `afirma` para autocalificarte, `traza` para ver la recursión |
| 9 | [Cadenas de texto](09_cadenas.md) | Operar caracteres y texto |
| 10 | [La biblioteca común](10_biblioteca_comun.md) | `incluye`, usar `bibcom/` en vez de reinventar |

Y para cuando ya sepas todo esto y solo necesites recordar la sintaxis:
[**Referencia rápida**](referencia_rapida.md) — todo en tablas, sin
explicaciones. Si usas **WinNemi**, hay además un
[**Apéndice: WinNemi**](apendice_winnemi.md) con lo que agrega el editor
(la consola como modo interactivo, barra de símbolos, rutas de inclusión)
— no hace falta para aprender el lenguaje, solo para sacarle provecho a la
herramienta.

## Una nota sobre cómo aprender con Nemi

La ventaja de que Nemi sea **ejecutable** (a diferencia del pseudocódigo de
un libro, que solo se lee) es que puedes **comprobar** cada idea en el
momento en que la lees, no confiar en que la entendiste. Te vamos a insistir
en esto todo el manual: cuando un capítulo diga "prueba esto", ábrelo de
verdad en tu intérprete y córrelo. Si el resultado no es el que esperabas,
ahí está lo interesante — un malentendido real, no uno que se te va a olvidar
en el examen.

La especificación completa del lenguaje (para cuando este manual no cubra
algo específico que necesites) está en [`../Nemi.md`](../Nemi.md) — es
formal y densa, pero es la referencia autoritativa.

Empieza aquí: [**Capítulo 1 — ¿Qué es un programa?**](01_que_es_un_programa.md)
