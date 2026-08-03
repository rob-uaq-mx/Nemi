# devtools/ — herramientas de autoría para WinNemi

## `ascii_to_bmp.py`

Convierte una rejilla de texto (dígitos hex `0`-`F`, uno por píxel) en un
`.bmp` de 4 bits por píxel (16 colores) — el formato clásico de bitmap de
toolbar de Win32. Paleta fija VGA/EGA de 16 colores (documentada en el
propio script).

```console
$ python ascii_to_bmp.py art/toolbar_std.art.txt ../apps/toolbar_std.bmp
```

**Formato de entrada:** exactamente 16 líneas (16 px de alto), cada una de
longitud `16·n` (n íconos de 16×16 uno junto al otro, sin separador). No se
admiten comentarios dentro del archivo — el orden de los íconos se
documenta aparte, abajo.

## `art/toolbar_std.art.txt`

Los 9 íconos de la barra estándar de `WinNemi`, en este orden (columnas
0-15, 16-31, 32-47, …):

| # | Ícono | Comando |
|---|---|---|
| 1 | Nuevo (página con esquina doblada) | `IDM_NEW` |
| 2 | Abrir (carpeta amarilla) | `IDM_OPEN` |
| 3 | Guardar (disco flexible) | `IDM_SAVE` |
| 4 | Cortar (tijeras) | `IDM_CUT` |
| 5 | Copiar (dos páginas superpuestas) | `IDM_COPY` |
| 6 | Pegar (portapapeles) | `IDM_PASTE` |
| 7 | Buscar (lupa) | `IDM_FIND` |
| 8 | Fuente (letra "A" azul) | `IDM_FONT` |
| 9 | Ejecutar (triángulo verde) | `IDM_RUN` |

Si cambias el arte, vuelve a generar `Windows/apps/toolbar_std.bmp` con el
comando de arriba y recompila — el `.bmp` se versiona en el repo (igual que
`WinNemi.ico`), no se regenera en cada build.

## `manual_to_html.py`

Convierte el conjunto cerrado de documentos con enlaces cruzados —
`manual/*.md` (el tutorial), `Nemi.md` (la spec) y `bibcom/README.md` (el
catálogo de la biblioteca común) — a HTML autocontenido, en `docs/` (raíz
del repo). Ese `docs/` sirve dos propósitos: el menú **Ayuda** de
`WinNemi.exe` (`IDM_HELP`, `WinNemi.cpp`, empaquetado por
`Windows/installer/WinNemi.iss`) y el sitio de **GitHub Pages** del
proyecto (`docs/index.html` es la portada). Usa `pandoc` (probado con
3.8.2.1; debe estar en el `PATH`) — sin argumentos, opera sobre rutas fijas:

```console
$ python manual_to_html.py
```

Los tres archivos solo se enlazan entre sí (nunca a nada fuera del
conjunto, verificado con grep) — así que **cualquier** enlace `.md` que
aparezca en ellos (con o sin `../`) se reescribe a `.html`, y la salida
reproduce la misma estructura relativa del repo para que esos enlaces
sigan resolviendo:

```
docs/manual/*.html       (desde manual/*.md)
docs/Nemi.html            (desde Nemi.md)
docs/bibcom/README.html   (desde bibcom/README.md)
```

CSS de `manual.css` incrustado (`--embed-resources`), cada `.html` queda
autocontenido.

`docs/*.html` se versiona en el repo, igual que `toolbar_std.bmp` — vuelve
a correr el comando de arriba a mano cuando cambie alguno de los tres
archivos fuente; `make_installer.bat` no lo regenera (así no depende de
tener `pandoc` instalado en cada máquina donde se compile). `docs/index.html`
y `docs/.nojekyll` son archivos aparte, escritos a mano — este script no
los toca.

## `manual.css`

Hoja de estilos mínima (ancho de línea cómodo, tipografía de sistema,
bloques de código y tablas legibles) usada por `manual_to_html.py`.
