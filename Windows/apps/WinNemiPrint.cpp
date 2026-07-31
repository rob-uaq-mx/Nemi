// WinNemiPrint.cpp -- printing stub for the WinNemi editor.
//
// Modernized signature only (popedit/POPPRNT0.C): the original book's
// dummy printing module, which always reports failure. A real
// implementation is out of scope here, same as in the reference.
#include "WinNemi.h"

BOOL WinNemiPrintFile(HINSTANCE hInst, HWND hwnd, HWND hwndEdit, const std::wstring &titleName) {
    (void)hInst;
    (void)hwnd;
    (void)hwndEdit;
    (void)titleName;
    return FALSE;
}
