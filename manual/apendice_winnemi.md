# Apéndice: WinNemi

Este apéndice es solo para quien usa **WinNemi** (el editor de Windows). El
resto del manual no depende de nada de esto — cualquier ejemplo funciona
igual con `python -m nemi` o con WinNemi. Aquí está lo que WinNemi agrega
*además* del lenguaje: su editor, su consola interactiva, y sus atajos.

## El editor y "Ejecutar"

WinNemi es un editor de texto (como el Bloc de notas) con una consola
integrada abajo. Escribes tu programa arriba y presionas **Ejecutar →
Ejecutar archivo** (**F5**): esto carga todas las `función`/`procedimiento`
que hayas escrito y corre el cuerpo del guion (las instrucciones sueltas,
fuera de cualquier función), exactamente como `python -m nemi tu_archivo.nemi`.
El resultado aparece en la consola de abajo.

## La consola como modo interactivo

La consola no es solo para ver la salida de tu `imprime` — también puedes
**escribirle** directamente, en el prompt `nemi>`. Ahí puedes probar una
función suelta, revisar el valor de una variable, o experimentar con una
expresión sin tener que agregarla a tu archivo:

```
nemi> imprime(mcd(504, 396))
36
nemi> x ← 3 en {1, 2, 3, 4}
nemi> imprime(x)
verdadero
```

Dos cosas a tener presentes:

- **La consola necesita que hayas ejecutado el archivo al menos una vez**
  (F5) antes de aceptar líneas — es lo que le da las funciones que puedes
  llamar. Si escribes algo antes, te lo recuerda: "No hay ningún archivo
  ejecutado. Use Ejecutar > Ejecutar archivo (F5)."
- Las variables que defines en la consola (como `x` arriba) **persisten
  entre líneas** — es tu "sesión" de prueba, separada del archivo. Volver a
  presionar F5 recarga el archivo **y** borra esa sesión, empezando de cero.

Si solo quieres limpiar las variables sueltas que acumulaste probando cosas
— sin perder las funciones que ya cargaste ni tener que releer el
archivo — usa **Ejecutar → Reiniciar sesión** (**Ctrl+Mayús+F5**) en vez de
F5:

```
nemi> x ← 99
nemi> imprime(mcd(504, 396))
36
```
*(Ejecutar → Reiniciar sesión)*
```
Sesión reiniciada.
nemi> imprime(x)
nemi: variable no definida: x
nemi> imprime(mcd(504, 396))
36
```

`x` desapareció, pero `mcd` (que viene del archivo, no de la consola) sigue
funcionando — "Reiniciar sesión" es más ligero que F5 precisamente por eso.

## Barra de símbolos

Escribir `∈`, `≤`, `⌊ ⌋` y demás símbolos de Nemi con el teclado puede ser
incómodo. La barra de símbolos (el segundo renglón de íconos, arriba del
editor) inserta cualquiera de ellos con un clic, en la posición donde esté
el cursor — incluidos los pares como `⌊⌋`/`⌈⌉`, que se insertan completos
con el cursor ya en medio, listo para escribir lo que va adentro.

## Rutas de inclusión

`incluye "archivo.nemi"` (capítulo 10) busca el archivo junto al tuyo por
defecto. WinNemi además:

- Encuentra solo la carpeta `bibcom/` si está junto al propio `WinNemi.exe`
  (así viene si lo instalaste) — no necesitas hacer nada para usar la
  [biblioteca común](10_biblioteca_comun.md).
- Te deja agregar tus propias carpetas desde **Ejecutar → Rutas de
  inclusión...** — útil si vas armando tu propia colección de funciones
  reutilizables en otra carpeta.

## Ayuda

El menú **Ayuda** abre este mismo manual en tu navegador.

Siguiente: [**Referencia rápida**](referencia_rapida.md) — o vuelve al
[**índice**](00_indice.md).
