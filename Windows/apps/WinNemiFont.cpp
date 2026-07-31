// WinNemiFont.cpp -- font selection for the WinNemi editor.
//
// Modernized from popedit/POPFONT.C: LOGFONT -> LOGFONTW, CHOOSEFONT ->
// CHOOSEFONTW, ChooseFont -> ChooseFontW, CreateFontIndirect ->
// CreateFontIndirectW.
#include "WinNemi.h"

static LOGFONTW logfont;
static HFONT hFont;

BOOL WinNemiFontChooseFont(HWND hwnd) {
    CHOOSEFONTW cf;

    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(CHOOSEFONTW);
    cf.hwndOwner   = hwnd;
    cf.lpLogFont   = &logfont;
    cf.Flags       = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;

    return ChooseFontW(&cf);
}

void WinNemiFontInitialize(HWND hwndEdit) {
    GetObjectW(GetStockObject(SYSTEM_FONT), sizeof(LOGFONTW), &logfont);
    hFont = CreateFontIndirectW(&logfont);
    SendMessageW(hwndEdit, WM_SETFONT, (WPARAM)hFont, 0);
}

void WinNemiFontSetFont(HWND hwndEdit) {
    HFONT hFontNew = CreateFontIndirectW(&logfont);
    SendMessageW(hwndEdit, WM_SETFONT, (WPARAM)hFontNew, 0);
    DeleteObject(hFont);
    hFont = hFontNew;

    RECT rect;
    GetClientRect(hwndEdit, &rect);
    InvalidateRect(hwndEdit, &rect, TRUE);
}

void WinNemiFontDeinitialize(void) {
    DeleteObject(hFont);
}
