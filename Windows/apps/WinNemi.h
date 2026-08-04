// WinNemi.h -- shared declarations for the WinNemi editor.
//
// Modernized replacement for popedit/POPPAD.H: same menu-command IDs (plus
// one new one, IDM_RUN), and the cross-file function declarations updated
// from ANSI PSTR buffers to explicit wide-character (...W) signatures with
// std::wstring, matching the UTF-16-internal / UTF-8-at-the-boundary
// convention already established by CmdLineCtl.c.
#ifndef WINNEMI_H
#define WINNEMI_H

#include <windows.h>
#include <commdlg.h>

#include <string>

// ----- Menu command IDs (unchanged from POPPAD.H) --------------------------
#define IDM_NEW          10
#define IDM_OPEN         11
#define IDM_SAVE         12
#define IDM_SAVEAS       13
#define IDM_PRINT        14
#define IDM_EXIT         15

#define IDM_UNDO         20
#define IDM_CUT          21
#define IDM_COPY         22
#define IDM_PASTE        23
#define IDM_CLEAR        24
#define IDM_SELALL       25

#define IDM_FIND         30
#define IDM_NEXT         31
#define IDM_REPLACE      32

#define IDM_FONT         40

#define IDM_HELP         50
#define IDM_ABOUT        51

// New: the "Ejecutar" menu, added for WinNemi.
#define IDM_RUN          60

// New: opens the "Rutas de inclusión" dialog (WinNemiIncludeDlg.cpp).
#define IDM_INCLUDE_PATHS 65

// New: clears the console's session variables (state->session) without
// touching state->interp -- lighter than re-running the whole buffer (F5).
#define IDM_RESET_SESSION 66

// New: one command ID per button of the symbol toolbar (WinNemiSymbols.cpp),
// sequential from IDM_SYM_FIRST -- WndProc's WM_COMMAND handler recognizes
// the whole range with a single bounds check instead of one case per symbol
// (see WinNemiSymbolsTable/WinNemiSymbolsCount in WinNemiSymbols.h).
#define IDM_SYM_FIRST    70

#define IDD_FNAME        10

// New: control IDs for the "Rutas de inclusión" dialog (WinNemi.rc's
// IncludesDlg + WinNemiIncludeDlg.cpp).
#define IDD_INCLUDES_LIST    100
#define IDD_INCLUDES_ADD     101
#define IDD_INCLUDES_REMOVE  102

// ----- UTF-8 <-> UTF-16 conversion (the boundary; Win32 speaks UTF-16, the
// Nemi interpreter and .nemi files speak UTF-8) ------------------------------
// Implemented once here (inline) and shared by every WinNemi*.cpp file that
// needs it, instead of duplicating the MultiByteToWideChar/WideCharToMultiByte
// boilerplate in each one (CmdLineCtl.c has its own private copy of the same
// pattern; its versions are `static`, not exported, so they are not reused
// here -- these are WinNemi's own, kept deliberately tiny).
inline std::wstring Utf8ToWide(const std::string &s) {
    if (s.empty())
        return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), len);
    return w;
}

inline std::string WideToUtf8(const std::wstring &w) {
    if (w.empty())
        return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), len, NULL, NULL);
    return s;
}

// ----- Functions in WinNemiFile.cpp -----------------------------------------
void WinNemiFileInitialize(HWND hwnd);
BOOL WinNemiFileOpenDlg(HWND hwnd, std::wstring &filePath, std::wstring &titleName);
BOOL WinNemiFileSaveDlg(HWND hwnd, std::wstring &filePath, std::wstring &titleName);
BOOL WinNemiFileRead(HWND hwndEdit, const std::wstring &filePath);
BOOL WinNemiFileWrite(HWND hwndEdit, const std::wstring &filePath);

// ----- Functions in WinNemiFind.cpp -----------------------------------------
HWND WinNemiFindFindDlg(HWND hwnd);
HWND WinNemiFindReplaceDlg(HWND hwnd);
BOOL WinNemiFindFindText(HWND hwndEdit, int *searchOffset, LPFINDREPLACEW pfr);
BOOL WinNemiFindReplaceText(HWND hwndEdit, int *searchOffset, LPFINDREPLACEW pfr);
BOOL WinNemiFindNextText(HWND hwndEdit, int *searchOffset);
BOOL WinNemiFindValidFind(void);

// ----- Functions in WinNemiFont.cpp -----------------------------------------
void WinNemiFontInitialize(HWND hwndEdit);
BOOL WinNemiFontChooseFont(HWND hwnd);
void WinNemiFontSetFont(HWND hwndEdit);
void WinNemiFontDeinitialize(void);

// ----- Functions in WinNemiPrint.cpp -----------------------------------------
BOOL WinNemiPrintFile(HINSTANCE hInst, HWND hwnd, HWND hwndEdit, const std::wstring &titleName);

// (WinNemiState.cpp and WinNemiIncludeDlg.cpp declare their own functions in
// WinNemiState.h / WinNemiIncludeDlg.h -- WinNemi.cpp includes those headers
// directly, same as WinNemiSymbols.h.)

#endif  // WINNEMI_H
