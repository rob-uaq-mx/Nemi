// WinNemiFind.cpp -- search and replace for the WinNemi editor.
//
// Modernized from popedit/POPFIND.C: FINDREPLACE -> FINDREPLACEW,
// FindText/ReplaceText -> the W variants, strstr -> wcsstr over the
// control's native UTF-16 text. Also fixes a latent bug in the original:
// it computed the found position from a pointer *after* the buffer holding
// it had already been freed (undefined behaviour, harmless in practice on
// every real libc but still UB) -- here the buffer (a std::wstring) stays
// alive for the whole function.
#include "WinNemi.h"

#include <cwchar>

#define MAX_STRING_LEN 256

static WCHAR szFindText[MAX_STRING_LEN];
static WCHAR szReplText[MAX_STRING_LEN];

HWND WinNemiFindFindDlg(HWND hwnd) {
    static FINDREPLACEW fr;  // must be static: backs a modeless dialog

    ZeroMemory(&fr, sizeof(fr));
    fr.lStructSize   = sizeof(FINDREPLACEW);
    fr.hwndOwner     = hwnd;
    fr.Flags         = FR_HIDEUPDOWN | FR_HIDEMATCHCASE | FR_HIDEWHOLEWORD;
    fr.lpstrFindWhat = szFindText;
    fr.wFindWhatLen  = MAX_STRING_LEN;

    return FindTextW(&fr);
}

HWND WinNemiFindReplaceDlg(HWND hwnd) {
    static FINDREPLACEW fr;  // must be static: backs a modeless dialog

    ZeroMemory(&fr, sizeof(fr));
    fr.lStructSize      = sizeof(FINDREPLACEW);
    fr.hwndOwner        = hwnd;
    fr.Flags            = FR_HIDEUPDOWN | FR_HIDEMATCHCASE | FR_HIDEWHOLEWORD;
    fr.lpstrFindWhat    = szFindText;
    fr.lpstrReplaceWith = szReplText;
    fr.wFindWhatLen     = MAX_STRING_LEN;
    fr.wReplaceWithLen  = MAX_STRING_LEN;

    return ReplaceTextW(&fr);
}

BOOL WinNemiFindFindText(HWND hwndEdit, int *searchOffset, LPFINDREPLACEW pfr) {
    int len = GetWindowTextLengthW(hwndEdit);
    std::wstring doc(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(hwndEdit, doc.data(), len + 1);
    doc.resize(static_cast<size_t>(len));

    const WCHAR *pos = wcsstr(doc.c_str() + *searchOffset, pfr->lpstrFindWhat);
    if (!pos)
        return FALSE;

    int foundPos = static_cast<int>(pos - doc.c_str());
    *searchOffset = foundPos + static_cast<int>(wcslen(pfr->lpstrFindWhat));

    SendMessageW(hwndEdit, EM_SETSEL, foundPos, *searchOffset);
    SendMessageW(hwndEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

BOOL WinNemiFindNextText(HWND hwndEdit, int *searchOffset) {
    FINDREPLACEW fr = {};
    fr.lpstrFindWhat = szFindText;

    return WinNemiFindFindText(hwndEdit, searchOffset, &fr);
}

BOOL WinNemiFindReplaceText(HWND hwndEdit, int *searchOffset, LPFINDREPLACEW pfr) {
    if (!WinNemiFindFindText(hwndEdit, searchOffset, pfr))
        return FALSE;

    SendMessageW(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)pfr->lpstrReplaceWith);
    return TRUE;
}

BOOL WinNemiFindValidFind(void) {
    return szFindText[0] != L'\0';
}
