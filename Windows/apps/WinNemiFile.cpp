// WinNemiFile.cpp -- file dialogs and file I/O for the WinNemi editor.
//
// Modernized from popedit/POPFILE.C: OPENFILENAME -> OPENFILENAMEW,
// GetOpenFileName/GetSaveFileName -> the W variants, and a *.nemi filter.
// Disk files are treated as plain UTF-8 bytes (matching every .nemi file in
// examples/ -- no BOM), converted to/from UTF-16 only at the EDIT control
// boundary via Utf8ToWide/WideToUtf8 (WinNemi.h).
#include "WinNemi.h"

#include <cstdio>

static OPENFILENAMEW ofn;

// A Win32 EDIT control needs "\r\n" to recognize a line break -- a lone '\n'
// does not start a new line, so a plain-LF .nemi file (every file under
// examples/ is LF-only) would render as one giant run-together line without
// this. Same fix CmdLineCtl.c already applies to its own console output
// (ConvertNewlines there), just not shared since that one is `static`.
static std::string LfToCrlf(const std::string &input) {
    std::string out;
    out.reserve(input.size());
    char prev = '\0';
    for (char c : input) {
        if (c == '\n' && prev != '\r')
            out.push_back('\r');
        out.push_back(c);
        prev = c;
    }
    return out;
}

// The inverse, applied on save: drop the '\r' of every "\r\n" pair, so files
// written from the editor keep the LF-only convention used by the rest of
// the Nemi corpus instead of picking up CRLF from the EDIT control.
static std::string CrlfToLf(const std::string &input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r' && i + 1 < input.size() && input[i + 1] == '\n')
            continue;
        out.push_back(input[i]);
    }
    return out;
}

void WinNemiFileInitialize(HWND hwnd) {
    static WCHAR szFilter[] =
        L"Archivos Nemi (*.nemi)\0*.nemi\0"
        L"Todos los archivos (*.*)\0*.*\0\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize   = sizeof(OPENFILENAMEW);
    ofn.hwndOwner     = hwnd;
    ofn.lpstrFilter   = szFilter;
    ofn.nMaxFile      = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrDefExt   = L"nemi";
}

BOOL WinNemiFileOpenDlg(HWND hwnd, std::wstring &filePath, std::wstring &titleName) {
    WCHAR file[MAX_PATH] = L"";
    WCHAR title[MAX_PATH] = L"";

    ofn.hwndOwner      = hwnd;
    ofn.lpstrFile      = file;
    ofn.lpstrFileTitle = title;
    ofn.Flags          = OFN_HIDEREADONLY | OFN_CREATEPROMPT;

    if (!GetOpenFileNameW(&ofn))
        return FALSE;

    filePath  = file;
    titleName = title;
    return TRUE;
}

BOOL WinNemiFileSaveDlg(HWND hwnd, std::wstring &filePath, std::wstring &titleName) {
    WCHAR file[MAX_PATH] = L"";
    WCHAR title[MAX_PATH] = L"";

    // Pre-fill with the current name so "Save As" starts from it.
    if (!filePath.empty()) {
        size_t n = filePath.copy(file, MAX_PATH - 1);
        file[n] = L'\0';
    }

    ofn.hwndOwner      = hwnd;
    ofn.lpstrFile      = file;
    ofn.lpstrFileTitle = title;
    ofn.Flags          = OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn))
        return FALSE;

    filePath  = file;
    titleName = title;
    return TRUE;
}

BOOL WinNemiFileRead(HWND hwndEdit, const std::wstring &filePath) {
    FILE *file = _wfopen(filePath.c_str(), L"rb");
    if (!file)
        return FALSE;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::string buffer(static_cast<size_t>(length), '\0');
    size_t got = fread(buffer.data(), 1, buffer.size(), file);
    fclose(file);

    if (got != buffer.size())
        return FALSE;

    SetWindowTextW(hwndEdit, Utf8ToWide(LfToCrlf(buffer)).c_str());
    return TRUE;
}

BOOL WinNemiFileWrite(HWND hwndEdit, const std::wstring &filePath) {
    int len = GetWindowTextLengthW(hwndEdit);
    std::wstring wide(static_cast<size_t>(len) + 1, L'\0');  // +1: GetWindowTextW's NUL slot
    GetWindowTextW(hwndEdit, wide.data(), len + 1);
    wide.resize(static_cast<size_t>(len));  // drop the trailing NUL from the logical content

    std::string utf8 = CrlfToLf(WideToUtf8(wide));

    FILE *file = _wfopen(filePath.c_str(), L"wb");
    if (!file)
        return FALSE;

    size_t written = fwrite(utf8.data(), 1, utf8.size(), file);
    fclose(file);

    return written == utf8.size();
}
