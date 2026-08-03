@echo off
setlocal enabledelayedexpansion

rem make_installer.bat -- rebuilds WinNemi in Release (static MSVC runtime,
rem see ../CMakeLists.txt's CMAKE_MSVC_RUNTIME_LIBRARY) and packages it with
rem Inno Setup. Run from anywhere -- paths are relative to this script's own
rem location, not the current directory.
rem
rem Prerequisite: Inno Setup 6 (https://jrsoftware.org/isinfo.php, or
rem   winget install --id JRSoftware.InnoSetup
rem ). See README.md in this folder for details.

set "SCRIPT_DIR=%~dp0"
set "WINDOWS_DIR=%SCRIPT_DIR%.."

echo === 1/4: Configurando CMake ===
cmake -S "%WINDOWS_DIR%" -B "%WINDOWS_DIR%\build"
if errorlevel 1 goto :error

echo === 2/4: Compilando en Release ===
cmake --build "%WINDOWS_DIR%\build" --config Release
if errorlevel 1 goto :error

echo === 3/4: Ejecutando pruebas ===
ctest --test-dir "%WINDOWS_DIR%\build" -C Release --output-on-failure
if errorlevel 1 goto :error

echo === 4/4: Empaquetando con Inno Setup ===
set "ISCC="
where iscc >nul 2>nul
if not errorlevel 1 (
    set "ISCC=iscc"
) else if exist "%LocalAppData%\Programs\Inno Setup 6\ISCC.exe" (
    set "ISCC=%LocalAppData%\Programs\Inno Setup 6\ISCC.exe"
) else if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
    set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
) else if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
    set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
)

if "%ISCC%"=="" (
    echo No se encontro ISCC.exe ^(Inno Setup 6^). Instalalo con:
    echo   winget install --id JRSoftware.InnoSetup
    goto :error
)

"%ISCC%" "%SCRIPT_DIR%WinNemi.iss"
if errorlevel 1 goto :error

echo.
echo Listo. Instalador en: %SCRIPT_DIR%output\WinNemi-Setup.exe
exit /b 0

:error
echo.
echo FALLO make_installer.bat -- revisa el mensaje de arriba.
exit /b 1
