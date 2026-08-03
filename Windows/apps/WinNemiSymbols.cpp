// WinNemiSymbols.cpp -- see WinNemiSymbols.h.
#include "WinNemiSymbols.h"
#include "WinNemi.h"

#include <cwchar>

namespace {

constexpr int kCellW = 20;
constexpr int kCellH = 20;

// The symbol-toolbar buttons, in display order. `insert` is what gets typed
// at the caret; `caretOffset` is always 1 -- either right after a single
// glyph, or between the two halves of a delimiter pair (⌊⌋, ⌈⌉), matching
// how a modern editor auto-closes brackets.
const WinNemiSymbol kSymbols[] = {
    { IDM_SYM_FIRST +  0, L"←",  1, L"Asignación ←  (<-)" },
    { IDM_SYM_FIRST +  1, L"≤",  1, L"Menor o igual ≤  (<=)" },
    { IDM_SYM_FIRST +  2, L"≥",  1, L"Mayor o igual ≥  (>=)" },
    { IDM_SYM_FIRST +  3, L"≠",  1, L"Distinto de ≠  (!=)" },
    { IDM_SYM_FIRST +  4, L"∧",  1, L"Y lógico ∧  (y)" },
    { IDM_SYM_FIRST +  5, L"∨",  1, L"O lógico ∨  (o)" },
    { IDM_SYM_FIRST +  6, L"¬",  1, L"Negación ¬  (no)" },
    { IDM_SYM_FIRST +  7, L"√",  1, L"Raíz √" },
    { IDM_SYM_FIRST +  8, L"∈",  1, L"Pertenece ∈  (en)" },
    { IDM_SYM_FIRST +  9, L"⊆",  1, L"Subconjunto ⊆" },
    { IDM_SYM_FIRST + 10, L"⊂",  1, L"Subconjunto propio ⊂" },
    { IDM_SYM_FIRST + 11, L"∅",  1, L"Conjunto vacío ∅" },
    { IDM_SYM_FIRST + 12, L"⌊⌋", 1, L"Piso ⌊ ⌋" },
    { IDM_SYM_FIRST + 13, L"⌈⌉", 1, L"Techo ⌈ ⌉" },
};

constexpr int kSymbolCount = sizeof(kSymbols) / sizeof(kSymbols[0]);

// Picks a font that actually has the math glyphs, instead of trusting
// CreateFontIndirectW blindly -- Windows never fails that call, it just
// silently substitutes a fallback font when the requested face isn't
// installed, so the substitution has to be detected by re-reading the face
// name GDI actually selected.
HFONT CreateSymbolFont(HDC hdc) {
    static const wchar_t *kCandidates[] = { L"Cambria Math", L"Segoe UI Symbol" };

    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;

    for (const wchar_t *name : kCandidates) {
        wcsncpy_s(lf.lfFaceName, LF_FACESIZE, name, _TRUNCATE);

        HFONT hFont = CreateFontIndirectW(&lf);
        if (!hFont) continue;

        HFONT hOld = (HFONT)SelectObject(hdc, hFont);
        wchar_t actualName[LF_FACESIZE] = {};
        GetTextFaceW(hdc, LF_FACESIZE, actualName);
        SelectObject(hdc, hOld);

        if (wcscmp(actualName, name) == 0)
            return hFont;
        DeleteObject(hFont);
    }
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);  // last resort; never delete this one
}

}  // namespace

const WinNemiSymbol *WinNemiSymbolsTable() { return kSymbols; }
int WinNemiSymbolsCount() { return kSymbolCount; }

HIMAGELIST WinNemiSymbolsBuildImageList(HWND hwndForDC) {
    HDC hdcScreen = GetDC(hwndForDC);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    int totalWidth = kCellW * kSymbolCount;
    HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, totalWidth, kCellH);
    HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

    // White background -- becomes the transparency mask color below, so the
    // toolbar's own button face shows through instead of a white square.
    RECT full = { 0, 0, totalWidth, kCellH };
    FillRect(hdcMem, &full, (HBRUSH)GetStockObject(WHITE_BRUSH));

    HFONT hFont = CreateSymbolFont(hdcMem);
    HFONT hFontOld = (HFONT)SelectObject(hdcMem, hFont);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(0, 0, 0));

    for (int i = 0; i < kSymbolCount; ++i) {
        RECT cell = { i * kCellW, 0, (i + 1) * kCellW, kCellH };
        DrawTextW(hdcMem, kSymbols[i].insert, -1, &cell,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdcMem, hFontOld);
    if (hFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hFont);
    SelectObject(hdcMem, hbmpOld);
    DeleteDC(hdcMem);
    ReleaseDC(hwndForDC, hdcScreen);

    HIMAGELIST himl = ImageList_Create(kCellW, kCellH, ILC_COLOR32 | ILC_MASK, kSymbolCount, 0);
    ImageList_AddMasked(himl, hbmp, RGB(255, 255, 255));
    DeleteObject(hbmp);
    return himl;
}
