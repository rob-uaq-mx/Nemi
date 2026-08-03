// WinNemiState.h -- persists the main window's placement (maximized or not,
// and its size/position) across sessions, in a small .ini next to the .exe
// (GetPrivateProfileString/WritePrivateProfileString) -- no registry, so
// there is nothing for the installer's uninstaller to clean up.
#ifndef WINNEMISTATE_H
#define WINNEMISTATE_H

#include <windows.h>

#include <string>
#include <vector>

// Reads WinNemi.ini (next to the running .exe) into `*placement`. Returns
// FALSE if no saved state exists yet (first run, or the .ini was deleted)
// -- callers should fall back to their own defaults in that case, not treat
// it as an error.
BOOL WinNemiStateLoad(WINDOWPLACEMENT *placement);

// Writes the window's current placement (via GetWindowPlacement) to
// WinNemi.ini. Call this while `hwnd` is still valid, e.g. at the top of
// the WM_DESTROY handler.
void WinNemiStateSave(HWND hwnd);

// Extra directories to search for 'incluye' targets not found next to the
// file that includes them (WinNemi.ini, section [Incluye], key Rutas=,
// ';'-separated) -- empty if none are configured yet. Edited via the
// "Rutas de inclusión..." dialog (WinNemiIncludeDlg.cpp).
std::vector<std::wstring> WinNemiStateLoadIncludeDirs();

// Overwrites the [Incluye] Rutas= list in WinNemi.ini with `dirs`.
void WinNemiStateSaveIncludeDirs(const std::vector<std::wstring> &dirs);

// The `bibcom` folder next to the running .exe, if it exists on disk (the
// installed layout, or a portable copy with bibcom/ alongside WinNemi.exe)
// -- empty if there is none, so callers can simply skip it.
std::wstring WinNemiStateImplicitBibcomDir();

// The running .exe's own directory (trailing backslash) -- e.g. for
// locating manual/ next to it, same as bibcom/ (WinNemiStateImplicitBibcomDir).
std::wstring WinNemiStateExeDir();

#endif  // WINNEMISTATE_H
