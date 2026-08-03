# Instalador de WinNemi

Empaqueta `WinNemi.exe` (compilado en Release, con **runtime de MSVC
estático** — ver `CMAKE_MSVC_RUNTIME_LIBRARY` en `../CMakeLists.txt`, sección
"static MSVC runtime") junto con el corpus de `../../examples/*.nemi`, la
biblioteca común `../../bibcom/*.nemi` (v0.2, `Nemi.md` §19/§22) y
`../../Nemi.md`, en un instalador `.exe` autocontenido: la máquina destino no
necesita tener el Visual C++ Redistributable instalado.

**Sobre usar `bibcom/` desde un script propio:** `incluye "ruta.nemi"`
resuelve rutas relativas al directorio del archivo que hace el `incluye`, no
a la carpeta de instalación — así que un archivo `.nemi` guardado en otra
carpeta (p. ej. `Documentos\`) no encuentra `bibcom/` automáticamente por esa
vía. Dentro de **`WinNemi.exe`** esto ya no es un problema: la app detecta
sola la carpeta `bibcom\` junto a su propio `.exe` (instalada o no), sin
configurar nada. Además, el menú **Ejecutar → Rutas de inclusión...** abre un
diálogo donde se pueden agregar (o quitar) carpetas propias — útil para que
un estudiante tenga su propia biblioteca de funciones reutilizables, no solo
`bibcom/`; el instalador ya deja `bibcom\` como primera entrada de ejemplo.

Desde el **CLI** (`nemi`, no `WinNemi.exe`) la detección automática no
aplica — ahí se usa `-I` explícitamente:
```console
nemi mi_archivo.nemi -I "%LocalAppData%\Programs\WinNemi\bibcom"
```
Alternativas (CLI o `WinNemi.exe` por igual): copiar la carpeta `bibcom\`
(queda en la carpeta de instalación, junto a `WinNemi.exe`) a donde el
estudiante vaya a guardar sus propios archivos, o usar una ruta relativa que
apunte de vuelta a la instalación, p. ej.
`incluye "../../../AppData/Local/Programs/WinNemi/bibcom/teoria_numeros.nemi"`
(ajustando el número de `../` a la ubicación real).

## Requisito: Inno Setup 6

```console
winget install --id JRSoftware.InnoSetup
```

(o descárgalo de <https://jrsoftware.org/isinfo.php>). No hace falta que
`ISCC.exe` esté en el `PATH` — `make_installer.bat` lo busca solo en las
ubicaciones típicas de instalación.

## Construir el instalador

Un solo comando, desde donde sea (las rutas del script son relativas a su
propia ubicación, no al directorio actual):

```console
Windows\installer\make_installer.bat
```

Hace, en orden: (1) reconfigura CMake, (2) compila en **Release**, (3) corre
`ctest` — si algo falla, se detiene ahí y no genera un instalador con una
build rota, (4) compila `WinNemi.iss` con Inno Setup. El resultado queda en:

```
Windows\installer\output\WinNemi-Setup.exe
```

(`output/` está en `.gitignore` — es un artefacto de build, como `build/`,
no se versiona).

## Qué hace el instalador (`WinNemi.iss`)

- **Sin permisos de administrador**: instala en `%LocalAppData%\Programs\WinNemi`
  (por usuario), no en `Archivos de programa` — funciona en máquinas de
  laboratorio/salón de clase sin privilegios elevados.
- Asistente en **español**.
- Crea accesos directos en el menú Inicio (y, opcionalmente, en el
  escritorio).
- Incluye desinstalador.
- **No está firmado digitalmente** (una firma de código cuesta dinero anual
  y no es necesaria para un proyecto de curso) — Windows SmartScreen
  probablemente muestre "editor desconocido" la primera vez que alguien lo
  ejecute. Se resuelve con "Más información" → "Ejecutar de todas formas";
  es normal para software sin firmar, no indica ningún problema real.

## Probar el instalador sin dejar rastro

Instalación y desinstalación silenciosas (útil para verificar sin abrir el
asistente gráfico):

```console
Windows\installer\output\WinNemi-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
"%LocalAppData%\Programs\WinNemi\unins000.exe" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
```

## Actualizar la versión

`AppVersion` está fijo en `WinNemi.iss` (`#define MyAppVersion "0.1.0"`);
súbelo a mano cuando corresponda — el instalador no lo deriva de ningún otro
archivo del repo.
