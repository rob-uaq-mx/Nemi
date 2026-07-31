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

#include <nemi/nemi.hpp>

#include <iostream>
#include <memory>
#include <streambuf>
#include <string>

#define EDITID    1
#define ID_CMDLN  2

// ----- Estado por ventana (vía GWLP_USERDATA) -------------------------------
struct AppState {
    HINSTANCE hInst = nullptr;
    HWND hwndEdit = nullptr;
    HWND hCmdLine = nullptr;
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

    CmdLnPrintf(state->hCmdLine, "\r\n--- Ejecutando ---\r\n");

    try {
        auto interp = std::make_unique<nemi::Interpreter>(nemi::load(source, source_path));
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

// ----- WinMain ---------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

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

    ShowWindow(hwnd, nCmdShow);
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
        int editHeight = (h * 65) / 100;  // ~65% editor, ~35% consola

        MoveWindow(state->hwndEdit, 0, 0, w, editHeight, TRUE);
        MoveWindow(state->hCmdLine, 0, editHeight, w, h - editHeight, TRUE);
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

        case IDM_HELP:
            OkMessage(hwnd, L"Ayuda no disponible todavía.", L"");
            return 0;

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
