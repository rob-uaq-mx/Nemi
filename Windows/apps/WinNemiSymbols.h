// WinNemiSymbols.h -- the "barra de símbolos" (symbol toolbar): a table of
// hard-to-type Nemi glyphs, each inserted into the editor at the caret when
// its toolbar button is clicked, plus the runtime-rendered HIMAGELIST that
// draws each button. The glyphs are rendered with GDI (TextOutW, a
// math-capable font) rather than hand-drawn pixel art like the standard
// toolbar's bitmap (see devtools/README.md) -- a dozen-plus precise Unicode
// math glyphs are much easier to get right by reusing the font's own
// hinting than by placing pixels by hand, and it guarantees the button
// looks exactly like the glyph the editor itself would show.
#ifndef WINNEMISYMBOLS_H
#define WINNEMISYMBOLS_H

#include <windows.h>
#include <commctrl.h>

// One button of the symbol toolbar.
struct WinNemiSymbol {
    int id;                 // IDM_SYM_FIRST + index (see WinNemi.h)
    const wchar_t *insert;  // text inserted at the caret (1 or 2 characters)
    int caretOffset;        // caret position within `insert` after inserting
                             // it (UTF-16 code units) -- right after a single
                             // glyph, or between the two halves of a
                             // delimiter pair like ⌊⌋
    const wchar_t *tooltip;
};

// The fixed table of symbol-toolbar buttons, in display order, and its size.
const WinNemiSymbol *WinNemiSymbolsTable();
int WinNemiSymbolsCount();

// Renders every symbol in the table into a fresh HIMAGELIST sized for a
// toolbar (one cell per symbol, white background treated as transparent),
// using a font capable of the Unicode math glyphs -- tries "Cambria Math"
// first, falls back to "Segoe UI Symbol", then to whatever
// GetStockObject(DEFAULT_GUI_FONT) provides. `hwndForDC` is any window used
// only to get a compatible screen DC (the returned bitmap itself does not
// depend on it afterwards). Caller owns the returned HIMAGELIST
// (ImageList_Destroy).
HIMAGELIST WinNemiSymbolsBuildImageList(HWND hwndForDC);

#endif  // WINNEMISYMBOLS_H
