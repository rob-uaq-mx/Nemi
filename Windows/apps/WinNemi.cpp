// WinNemi.cpp -- editor + consola para archivos Nemi.
//
// Modernizado desde popedit/POPPAD.C: llamadas ...W explícitas (sin macros
// TCHAR), mensajes en español, y sin las variables `static` de función que
// usaba el original -- el estado por ventana se guarda en un AppState*
// asociado vía GWLP_USERDATA en WM_NCCREATE (inspirado en el truco de
// FrameWindow::procedure de windows-gui/win32/Win32/OOPWin, sin adoptar su
// jerarquía completa de clases).
//
// Añade, sobre PopPad: un panel de consola (CmdLineCtl) bajo el editor, y un
// menú "Ejecutar" que carga el texto del editor en el intérprete de Nemi.
#include "WinNemi.h"
#include "CmdLineCtl.h"
#include "WinNemiSymbols.h"
#include "WinNemiState.h"
#include "WinNemiIncludeDlg.h"

#include <nemi/nemi.hpp>

#include <commctrl.h>
#include <shellapi.h>

#include <iostream>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

// Activa Common Controls v6 (visual moderno del control Toolbar, en vez del
// estilo plano de Windows 95) sin necesitar un archivo .manifest aparte.
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define EDITID          1
#define ID_CMDLN        2
#define IDC_TOOLBAR_STD 3
#define IDC_TOOLBAR_SYM 4

// ----- Estado por ventana (vía GWLP_USERDATA) -------------------------------
struct AppState {
    HINSTANCE hInst = nullptr;
    HWND hwndEdit = nullptr;
    HWND hCmdLine = nullptr;
    HWND hToolbarStd = nullptr;
    HWND hToolbarSym = nullptr;
    int toolbarStdHeight = 0;
    int toolbarSymHeight = 0;
    HIMAGELIST himlStd = nullptr;
    HIMAGELIST himlSym = nullptr;
    HWND hDlgModeless = nullptr;
    UINT iMsgFindReplace = 0;
    bool dirty = false;
    int searchOffset = 0;
    std::wstring filePath;   // vacío = sin título
    std::wstring titleName;
    std::unique_ptr<nemi::Interpreter> interp;  // null hasta el primer "Ejecutar" exitoso
    nemi::Environment session;  // variables de la consola, persisten entre líneas (se
                                 // reinicia junto con `interp` en cada F5)
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

// ----- imprime(...) -> consola --------------------------------------------
// prim_print (src/interpreter.cpp) escribe a std::cout; una app WIN32 no
// tiene consola adjunta, así que sin esta redirección esa salida se perdería
// en silencio. Se instala solo durante la ejecución (RAII), sin tocar la
// librería `nemi` compartida con nemi-cli/corpus_test.
class ConsoleStreambuf : public std::streambuf {
public:
    explicit ConsoleStreambuf(HWND hCmdLine) : hCmdLine_(hCmdLine) {}

protected:
    int overflow(int c) override {
        if (c == EOF)
            return c;
        line_.push_back(static_cast<char>(c));
        if (c == '\n') {
            CmdLnPuts(hCmdLine_, line_.c_str());
            line_.clear();
        }
        return c;
    }

    int sync() override {
        if (!line_.empty()) {
            CmdLnPuts(hCmdLine_, line_.c_str());
            line_.clear();
        }
        return 0;
    }

private:
    HWND hCmdLine_;
    std::string line_;
};

class RedirectCout {
public:
    explicit RedirectCout(HWND hCmdLine) : buf_(hCmdLine), old_(std::cout.rdbuf(&buf_)) {}
    ~RedirectCout() { std::cout.rdbuf(old_); }
    RedirectCout(const RedirectCout &) = delete;
    RedirectCout &operator=(const RedirectCout &) = delete;

private:
    ConsoleStreambuf buf_;
    std::streambuf *old_;
};

// ----- Mensajes (español) ---------------------------------------------------
static void DoCaption(HWND hwnd, AppState *state) {
    std::wstring name = state->titleName.empty() ? L"(sin título)" : state->titleName;
    SetWindowTextW(hwnd, (L"WinNemi - " + name).c_str());
}

static void OkMessage(HWND hwnd, const wchar_t *format, const std::wstring &titleName) {
    std::wstring name = titleName.empty() ? L"(sin título)" : titleName;
    wchar_t buffer[64 + MAX_PATH];
    wsprintfW(buffer, format, name.c_str());
    MessageBoxW(hwnd, buffer, L"WinNemi", MB_OK | MB_ICONEXCLAMATION);
}

static int AskAboutSave(HWND hwnd, AppState *state) {
    std::wstring name = state->titleName.empty() ? L"(sin título)" : state->titleName;
    wchar_t buffer[64 + MAX_PATH];
    wsprintfW(buffer, L"¿Guardar los cambios en %s?", name.c_str());

    int result = MessageBoxW(hwnd, buffer, L"WinNemi", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (result == IDYES && !SendMessageW(hwnd, WM_COMMAND, IDM_SAVE, 0))
        result = IDCANCEL;
    return result;
}

// ----- Ejecutar / consola ---------------------------------------------------
static void RunCurrentBuffer(AppState *state) {
    int len = GetWindowTextLengthW(state->hwndEdit);
    std::wstring wide(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(state->hwndEdit, wide.data(), len + 1);
    wide.resize(static_cast<size_t>(len));

    std::string source = WideToUtf8(wide);
    // Passing the saved path (if any) lets a relative `incluye "ruta.nemi"`
    // resolve against the file's own directory, not the process's CWD; an
    // unsaved buffer falls back to CWD (nemi::load's default).
    std::string source_path = state->filePath.empty() ? "" : WideToUtf8(state->filePath);

    // If not found next to the file's own directory, 'incluye' also tries:
    // the student's own configured folders (Ejecutar > Rutas de
    // inclusión...), then bibcom/ next to WinNemi.exe if it's there -- same
    // order the -I CLI flag uses (yours wins, bibcom/ is the last resort).
    std::vector<std::string> search_paths;
    for (const auto &dir : WinNemiStateLoadIncludeDirs())
        search_paths.push_back(WideToUtf8(dir));
    std::wstring bibcom = WinNemiStateImplicitBibcomDir();
    if (!bibcom.empty())
        search_paths.push_back(WideToUtf8(bibcom));

    CmdLnPrintf(state->hCmdLine, "\r\n--- Ejecutando ---\r\n");

    try {
        auto interp = std::make_unique<nemi::Interpreter>(
            nemi::load(source, source_path, search_paths));
        {
            RedirectCout guard(state->hCmdLine);
            interp->run_program();
        }
        state->interp = std::move(interp);
        state->session = nemi::Environment();  // cada F5 parte de una sesión limpia
        CmdLnPrintf(state->hCmdLine, "Listo.\r\n");
    } catch (const std::exception &e) {
        CmdLnPrintf(state->hCmdLine, "nemi: %s\r\n", e.what());
    }

    ShowPrompt(state->hCmdLine);
}

static void OnConsoleLine(HWND hCmdLine, const char *command, void *userdata) {
    auto *state = static_cast<AppState *>(userdata);

    if (!state->interp) {
        CmdLnPrintf(hCmdLine,
                    "No hay ningún archivo ejecutado. "
                    "Use Ejecutar > Ejecutar archivo (F5).\r\n");
        return;
    }

    try {
        RedirectCout guard(hCmdLine);
        nemi::Value result = state->interp->run_line(command, state->session);
        if (!std::holds_alternative<std::monostate>(result))
            CmdLnPrintf(hCmdLine, "%s\r\n", nemi::format_value(result).c_str());
    } catch (const std::exception &e) {
        CmdLnPrintf(hCmdLine, "nemi: %s\r\n", e.what());
    }
}

// ----- Barras de herramientas ------------------------------------------------
static TBBUTTON MakeToolbarButton(int bitmapIndex, int commandId, const wchar_t *tooltip) {
    TBBUTTON b = {};
    b.iBitmap = bitmapIndex;
    b.idCommand = commandId;
    b.fsState = TBSTATE_ENABLED;
    b.fsStyle = BTNS_BUTTON;
    b.iString = reinterpret_cast<INT_PTR>(tooltip);
    return b;
}

static TBBUTTON MakeToolbarSeparator() {
    TBBUTTON b = {};
    b.fsStyle = BTNS_SEP;
    return b;
}

// Crea la barra estándar (bitmap de devtools/, ver WinNemi.rc) con los 9
// botones que ya tienen un IDM_* de menú -- así que no hace falta ninguna
// lógica nueva en WM_COMMAND, el botón manda el mismo mensaje que el menú.
static HWND CreateStandardToolbar(HWND hwndParent, HINSTANCE hInst, HIMAGELIST *outHiml) {
    HWND hToolbar = CreateWindowExW(
        0, L"ToolbarWindow32", NULL,  // TOOLBARCLASSNAME is TEXT()-based (narrow unless
                                      // UNICODE is defined); this file uses explicit ...W
                                      // calls throughout instead, so spell it out directly
        // Ni CCS_NORESIZE (apagaría el auto-tamaño que TB_AUTOSIZE necesita
        // más abajo para calcular el alto natural, que se cachea en AppState
        // -- toolbarStdHeight/toolbarSymHeight) ni, sobre todo, sin
        // CCS_NOPARENTALIGN: sin ese estilo un toolbar se "acopla" solo a la
        // parte de arriba del cliente del padre en cada resize (su
        // comportamiento por defecto), ignorando el Y que le pasemos a
        // MoveWindow -- con dos barras, ambas terminan en Y=0, superpuestas.
        // Con CCS_NOPARENTALIGN el control respeta la posición/tamaño que le
        // demos a mano en WM_SIZE, y TB_AUTOSIZE sigue funcionando igual.
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
        0, 0, 0, 0, hwndParent, reinterpret_cast<HMENU>(IDC_TOOLBAR_STD), hInst, NULL);

    HBITMAP hbmp = static_cast<HBITMAP>(
        LoadImageW(hInst, L"WinNemiToolbarStd", IMAGE_BITMAP, 0, 0, 0));
    HIMAGELIST himl = ImageList_Create(16, 16, ILC_COLOR4 | ILC_MASK, 9, 0);
    if (hbmp) {
        ImageList_AddMasked(himl, hbmp, RGB(255, 255, 255));
        DeleteObject(hbmp);
    }
    SendMessageW(hToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON buttons[] = {
        MakeToolbarButton(0, IDM_NEW,   L"Nuevo (Ctrl+N)"),
        MakeToolbarButton(1, IDM_OPEN,  L"Abrir... (Ctrl+O)"),
        MakeToolbarButton(2, IDM_SAVE,  L"Guardar (Ctrl+S)"),
        MakeToolbarSeparator(),
        MakeToolbarButton(3, IDM_CUT,   L"Cortar (Ctrl+X)"),
        MakeToolbarButton(4, IDM_COPY,  L"Copiar (Ctrl+C)"),
        MakeToolbarButton(5, IDM_PASTE, L"Pegar (Ctrl+V)"),
        MakeToolbarSeparator(),
        MakeToolbarButton(6, IDM_FIND,  L"Buscar... (Ctrl+F)"),
        MakeToolbarSeparator(),
        MakeToolbarButton(7, IDM_FONT,  L"Fuente..."),
        MakeToolbarSeparator(),
        MakeToolbarButton(8, IDM_RUN,   L"Ejecutar archivo (F5)"),
    };
    SendMessageW(hToolbar, TB_ADDBUTTONSW, sizeof(buttons) / sizeof(buttons[0]),
                 reinterpret_cast<LPARAM>(buttons));
    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);

    *outHiml = himl;
    return hToolbar;
}

// Crea la barra de símbolos: un botón por cada entrada de WinNemiSymbols.h,
// con la imagen renderizada en tiempo de ejecución (no un bitmap de
// devtools/ -- ver WinNemiSymbols.cpp).
static HWND CreateSymbolToolbar(HWND hwndParent, HINSTANCE hInst, HIMAGELIST *outHiml) {
    HWND hToolbar = CreateWindowExW(
        0, L"ToolbarWindow32", NULL,  // TOOLBARCLASSNAME is TEXT()-based (narrow unless
                                      // UNICODE is defined); this file uses explicit ...W
                                      // calls throughout instead, so spell it out directly
        // Ver el comentario en CreateStandardToolbar sobre por qué ni
        // CCS_NORESIZE ni (sobre todo) CCS_NOPARENTALIGN pueden faltar aquí.
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
        0, 0, 0, 0, hwndParent, reinterpret_cast<HMENU>(IDC_TOOLBAR_SYM), hInst, NULL);

    HIMAGELIST himl = WinNemiSymbolsBuildImageList(hwndParent);
    SendMessageW(hToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    int count = WinNemiSymbolsCount();
    const WinNemiSymbol *symbols = WinNemiSymbolsTable();
    std::vector<TBBUTTON> buttons(count);
    for (int i = 0; i < count; ++i)
        buttons[i] = MakeToolbarButton(i, symbols[i].id, symbols[i].tooltip);
    SendMessageW(hToolbar, TB_ADDBUTTONSW, count, reinterpret_cast<LPARAM>(buttons.data()));
    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);

    *outHiml = himl;
    return hToolbar;
}

// Inserta el texto de un botón de la barra de símbolos en el cursor del
// editor, dejando el cursor en `caretOffset` (mide en unidades UTF-16
// dentro de `insert` -- justo después de un glifo suelto, o entre las dos
// mitades de un par delimitador como ⌊⌋).
static void InsertSymbol(HWND hwndEdit, const WinNemiSymbol &sym) {
    DWORD selStart = 0, selEnd = 0;
    SendMessageW(hwndEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart),
                reinterpret_cast<LPARAM>(&selEnd));
    SendMessageW(hwndEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(sym.insert));
    DWORD caret = selStart + static_cast<DWORD>(sym.caretOffset);
    SendMessageW(hwndEdit, EM_SETSEL, caret, caret);
}

// ----- WinMain ---------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance, L"WinNemi");
    wc.hIconSm       = LoadIconW(hInstance, L"WinNemi");
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);  // IDC_ARROW is an
                                          // A/W-agnostic MAKEINTRESOURCE value
                                          // (same cast used in cmdln/src/winmain.c)
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = L"WinNemi";
    wc.lpszClassName = L"WinNemi";

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"No se pudo registrar la clase de ventana.", L"WinNemi", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0, L"WinNemi", L"WinNemi", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"No se pudo crear la ventana.", L"WinNemi", MB_ICONERROR);
        return 0;
    }

    // Recuerda tamaño/posición/maximizado de la sesión anterior (WinNemi.ini
    // junto al .exe -- ver WinNemiState.h). Solo se usa cuando nCmdShow es el
    // caso normal/por-defecto (doble clic en el .exe, o un acceso directo sin
    // "modo de ventana" forzado); si quien lanzó el proceso pidió
    // explícitamente minimizado/maximizado, eso tiene prioridad sobre lo
    // guardado -- mismo criterio que usan la mayoría de las apps bien
    // portadas.
    WINDOWPLACEMENT savedPlacement;
    if (nCmdShow == SW_SHOWNORMAL && WinNemiStateLoad(&savedPlacement)) {
        SetWindowPlacement(hwnd, &savedPlacement);
    } else {
        ShowWindow(hwnd, nCmdShow);
    }
    UpdateWindow(hwnd);

    HACCEL hAccel = LoadAcceleratorsW(hInstance, L"WinNemi");
    auto *state = reinterpret_cast<AppState *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!state->hDlgModeless || !IsDialogMessageW(state->hDlgModeless, &msg)) {
            if (!TranslateAcceleratorW(hwnd, hAccel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }
    return static_cast<int>(msg.wParam);
}

// ----- WndProc ---------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(new AppState()));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto *state = reinterpret_cast<AppState *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_CREATE: {
        state->hInst = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;

        state->hToolbarStd = CreateStandardToolbar(hwnd, state->hInst, &state->himlStd);
        state->hToolbarSym = CreateSymbolToolbar(hwnd, state->hInst, &state->himlSym);
        if (!state->hToolbarStd || !state->hToolbarSym) {
            MessageBoxW(hwnd, L"No se pudieron crear las barras de herramientas.", L"WinNemi",
                        MB_ICONERROR);
            return -1;
        }
        RECT rcStd, rcSym;
        GetWindowRect(state->hToolbarStd, &rcStd);
        GetWindowRect(state->hToolbarSym, &rcSym);
        state->toolbarStdHeight = rcStd.bottom - rcStd.top;
        state->toolbarSymHeight = rcSym.bottom - rcSym.top;

        state->hwndEdit = CreateWindowExW(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_BORDER |
                ES_LEFT | ES_MULTILINE | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(EDITID), state->hInst, NULL);

        state->hCmdLine = CreateCmdLine(hwnd, ID_CMDLN, 0, 0, 0, 0, OnConsoleLine, state);

        if (!state->hwndEdit || !state->hCmdLine) {
            MessageBoxW(hwnd, L"No se pudo crear el editor o la consola.", L"WinNemi", MB_ICONERROR);
            return -1;
        }

        SendMessageW(state->hwndEdit, EM_SETLIMITTEXT, 0, 0);  // sin el límite ~32K por defecto

        SetCmdLnPrompt(state->hCmdLine, "nemi> ");
        CmdLnPrintf(state->hCmdLine, "WinNemi -- editor y consola de Nemi\r\n");
        CmdLnPrintf(state->hCmdLine,
                    "Use Ejecutar > Ejecutar archivo (F5) para cargar el archivo actual.\r\n\r\n");
        ShowPrompt(state->hCmdLine);

        WinNemiFileInitialize(hwnd);
        WinNemiFontInitialize(state->hwndEdit);

        // Nombre documentado y estable de FINDMSGSTRING (commdlg.h), en ancho
        // directo -- el macro en sí expande a un literal *narrow*.
        state->iMsgFindReplace = RegisterWindowMessageW(L"commdlg_FindReplace");

        DoCaption(hwnd, state);
        return 0;
    }

    case WM_SETFOCUS:
        SetFocus(state->hwndEdit);
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);

        int y = 0;
        MoveWindow(state->hToolbarStd, 0, y, w, state->toolbarStdHeight, TRUE);
        y += state->toolbarStdHeight;
        MoveWindow(state->hToolbarSym, 0, y, w, state->toolbarSymHeight, TRUE);
        y += state->toolbarSymHeight;

        int remaining = h - y;
        int editHeight = (remaining * 65) / 100;  // ~65% editor, ~35% consola (del espacio restante)

        MoveWindow(state->hwndEdit, 0, y, w, editHeight, TRUE);
        MoveWindow(state->hCmdLine, 0, y + editHeight, w, remaining - editHeight, TRUE);
        return 0;
    }

    case WM_INITMENUPOPUP:
        switch (lParam) {
        case 1:  // menú Editar
            EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_UNDO,
                          SendMessageW(state->hwndEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_PASTE,
                          IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED);
            {
                DWORD selStart, selEnd;
                SendMessageW(state->hwndEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart),
                            reinterpret_cast<LPARAM>(&selEnd));
                UINT enable = (selStart != selEnd) ? MF_ENABLED : MF_GRAYED;
                EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_CUT, enable);
                EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_COPY, enable);
                EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_CLEAR, enable);
            }
            break;

        case 2: {  // menú Buscar
            UINT enable = (state->hDlgModeless == NULL) ? MF_ENABLED : MF_GRAYED;
            EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_FIND, enable);
            EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_NEXT, enable);
            EnableMenuItem(reinterpret_cast<HMENU>(wParam), IDM_REPLACE, enable);
            break;
        }
        }
        return 0;

    case WM_COMMAND:
        if (lParam && LOWORD(wParam) == EDITID) {
            switch (HIWORD(wParam)) {
            case EN_UPDATE:
                state->dirty = true;
                return 0;
            case EN_ERRSPACE:
            case EN_MAXTEXT:
                MessageBoxW(hwnd, L"El editor se quedó sin espacio.", L"WinNemi", MB_OK | MB_ICONSTOP);
                return 0;
            }
            break;
        }

        // Botones de la barra de símbolos (WinNemiSymbols.h): un rango
        // contiguo de IDs, así que un solo chequeo de límites basta en vez
        // de un `case` por símbolo.
        if (LOWORD(wParam) >= IDM_SYM_FIRST &&
            LOWORD(wParam) < IDM_SYM_FIRST + WinNemiSymbolsCount()) {
            const WinNemiSymbol *symbols = WinNemiSymbolsTable();
            InsertSymbol(state->hwndEdit, symbols[LOWORD(wParam) - IDM_SYM_FIRST]);
            SetFocus(state->hwndEdit);
            return 0;
        }

        switch (LOWORD(wParam)) {
        case IDM_NEW:
            if (state->dirty && IDCANCEL == AskAboutSave(hwnd, state))
                return 0;
            SetWindowTextW(state->hwndEdit, L"");
            state->filePath.clear();
            state->titleName.clear();
            DoCaption(hwnd, state);
            state->dirty = false;
            return 0;

        case IDM_OPEN:
            if (state->dirty && IDCANCEL == AskAboutSave(hwnd, state))
                return 0;
            if (WinNemiFileOpenDlg(hwnd, state->filePath, state->titleName)) {
                if (!WinNemiFileRead(state->hwndEdit, state->filePath)) {
                    OkMessage(hwnd, L"No se pudo leer el archivo %s", state->titleName);
                    state->filePath.clear();
                    state->titleName.clear();
                }
            }
            DoCaption(hwnd, state);
            state->dirty = false;
            return 0;

        case IDM_SAVE:
            if (!state->filePath.empty()) {
                if (WinNemiFileWrite(state->hwndEdit, state->filePath)) {
                    state->dirty = false;
                    return 1;
                }
                OkMessage(hwnd, L"No se pudo guardar el archivo %s", state->titleName);
                return 0;
            }
            [[fallthrough]];
        case IDM_SAVEAS:
            if (WinNemiFileSaveDlg(hwnd, state->filePath, state->titleName)) {
                DoCaption(hwnd, state);
                if (WinNemiFileWrite(state->hwndEdit, state->filePath)) {
                    state->dirty = false;
                    return 1;
                }
                OkMessage(hwnd, L"No se pudo guardar el archivo %s", state->titleName);
            }
            return 0;

        case IDM_PRINT:
            if (!WinNemiPrintFile(state->hInst, hwnd, state->hwndEdit, state->titleName))
                OkMessage(hwnd, L"No se pudo imprimir el archivo %s", state->titleName);
            return 0;

        case IDM_EXIT:
            SendMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case IDM_UNDO:
            SendMessageW(state->hwndEdit, WM_UNDO, 0, 0);
            return 0;
        case IDM_CUT:
            SendMessageW(state->hwndEdit, WM_CUT, 0, 0);
            return 0;
        case IDM_COPY:
            SendMessageW(state->hwndEdit, WM_COPY, 0, 0);
            return 0;
        case IDM_PASTE:
            SendMessageW(state->hwndEdit, WM_PASTE, 0, 0);
            return 0;
        case IDM_CLEAR:
            SendMessageW(state->hwndEdit, WM_CLEAR, 0, 0);
            return 0;
        case IDM_SELALL:
            SendMessageW(state->hwndEdit, EM_SETSEL, 0, -1);
            return 0;

        case IDM_FIND:
            SendMessageW(state->hwndEdit, EM_GETSEL, 0, reinterpret_cast<LPARAM>(&state->searchOffset));
            state->hDlgModeless = WinNemiFindFindDlg(hwnd);
            return 0;

        case IDM_NEXT:
            SendMessageW(state->hwndEdit, EM_GETSEL, 0, reinterpret_cast<LPARAM>(&state->searchOffset));
            if (WinNemiFindValidFind())
                WinNemiFindNextText(state->hwndEdit, &state->searchOffset);
            else
                state->hDlgModeless = WinNemiFindFindDlg(hwnd);
            return 0;

        case IDM_REPLACE:
            SendMessageW(state->hwndEdit, EM_GETSEL, 0, reinterpret_cast<LPARAM>(&state->searchOffset));
            state->hDlgModeless = WinNemiFindReplaceDlg(hwnd);
            return 0;

        case IDM_FONT:
            if (WinNemiFontChooseFont(hwnd))
                WinNemiFontSetFont(state->hwndEdit);
            return 0;

        case IDM_RUN:
            RunCurrentBuffer(state);
            return 0;

        case IDM_INCLUDE_PATHS: {
            std::vector<std::wstring> dirs = WinNemiStateLoadIncludeDirs();
            if (WinNemiIncludeDlgShow(state->hInst, hwnd, dirs))
                WinNemiStateSaveIncludeDirs(dirs);
            return 0;
        }

        case IDM_HELP: {
            std::wstring helpPath = WinNemiStateExeDir() + L"manual\\00_indice.html";
            if (GetFileAttributesW(helpPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                OkMessage(hwnd, L"No se encontró la carpeta manual\\ junto al programa.", L"");
            else
                ShellExecuteW(NULL, L"open", helpPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            return 0;
        }

        case IDM_ABOUT:
            DialogBoxW(state->hInst, L"AboutBox", hwnd, AboutDlgProc);
            return 0;
        }
        break;

    case WM_CLOSE:
        if (!state->dirty || IDCANCEL != AskAboutSave(hwnd, state))
            DestroyWindow(hwnd);
        return 0;

    case WM_QUERYENDSESSION:
        return (!state->dirty || IDCANCEL != AskAboutSave(hwnd, state)) ? 1 : 0;

    case WM_DESTROY:
        WinNemiStateSave(hwnd);  // primero -- necesita hwnd todavía válido
        if (state->himlStd) ImageList_Destroy(state->himlStd);
        if (state->himlSym) ImageList_Destroy(state->himlSym);
        WinNemiFontDeinitialize();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        delete state;
        PostQuitMessage(0);
        return 0;

    default:
        if (msg == state->iMsgFindReplace) {
            auto *pfr = reinterpret_cast<LPFINDREPLACEW>(lParam);

            if (pfr->Flags & FR_DIALOGTERM)
                state->hDlgModeless = NULL;

            if (pfr->Flags & FR_FINDNEXT) {
                if (!WinNemiFindFindText(state->hwndEdit, &state->searchOffset, pfr))
                    OkMessage(hwnd, L"Texto no encontrado.", L"");
            }
            if (pfr->Flags & (FR_REPLACE | FR_REPLACEALL)) {
                if (!WinNemiFindReplaceText(state->hwndEdit, &state->searchOffset, pfr))
                    OkMessage(hwnd, L"Texto no encontrado.", L"");
            }
            if (pfr->Flags & FR_REPLACEALL) {
                while (WinNemiFindReplaceText(state->hwndEdit, &state->searchOffset, pfr))
                    ;
            }
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
