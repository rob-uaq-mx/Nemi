// WinNemiIncludeDlg.h -- the "Rutas de inclusión" dialog: lets a student add
// or remove directories where 'incluye' should also look (see WinNemiState.h
// for how the list is persisted, and WinNemi.cpp's RunCurrentBuffer for how
// it's used when running a file).
#ifndef WINNEMIINCLUDEDLG_H
#define WINNEMIINCLUDEDLG_H

#include <windows.h>

#include <string>
#include <vector>

// Shows the modal "Rutas de inclusión" dialog, letting the user add (via a
// folder picker) or remove entries from `dirs`. Returns true and updates
// `dirs` if the user accepted (Aceptar); returns false and leaves `dirs`
// untouched if they cancelled or closed the dialog.
bool WinNemiIncludeDlgShow(HINSTANCE hInst, HWND hwndParent,
                            std::vector<std::wstring> &dirs);

#endif  // WINNEMIINCLUDEDLG_H
