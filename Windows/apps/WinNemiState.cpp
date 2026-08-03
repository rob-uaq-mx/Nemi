// WinNemiState.cpp -- see WinNemiState.h.
#include "WinNemiState.h"

#include <string>

namespace {

// The running .exe's own directory -- works the same whether WinNemi.exe is
// the dev build (Windows/build/Debug/) or the installed copy
// (%LocalAppData%\Programs\WinNemi\), with no path hardcoded either way.
std::wstring ExeDir() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring s(path);
    size_t slash = s.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        s.resize(slash + 1);
    return s;
}

// WinNemi.ini lives next to the running .exe.
std::wstring IniPath() {
    return ExeDir() + L"WinNemi.ini";
}

const wchar_t kSection[] = L"Ventana";
const wchar_t kIncludeSection[] = L"Incluye";

}  // namespace

BOOL WinNemiStateLoad(WINDOWPLACEMENT *placement) {
    std::wstring ini = IniPath();

    // GetPrivateProfileIntW returns the supplied default (here, an
    // out-of-range sentinel) when the key or the file itself doesn't exist
    // -- that is how "no saved state yet" is told apart from "loaded fine".
    int maximized = GetPrivateProfileIntW(kSection, L"Maximizado", -1, ini.c_str());
    if (maximized < 0)
        return FALSE;

    RECT r;
    r.left   = GetPrivateProfileIntW(kSection, L"Izq", 0, ini.c_str());
    r.top    = GetPrivateProfileIntW(kSection, L"Arr", 0, ini.c_str());
    r.right  = GetPrivateProfileIntW(kSection, L"Der", 0, ini.c_str());
    r.bottom = GetPrivateProfileIntW(kSection, L"Ab", 0, ini.c_str());
    if (r.right <= r.left || r.bottom <= r.top)
        return FALSE;  // corrupted/hand-edited .ini -- ignore it, don't crash

    ZeroMemory(placement, sizeof(*placement));
    placement->length = sizeof(*placement);
    placement->showCmd = maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
    placement->rcNormalPosition = r;
    return TRUE;
}

void WinNemiStateSave(HWND hwnd) {
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    if (!GetWindowPlacement(hwnd, &wp))
        return;

    std::wstring ini = IniPath();
    BOOL maximized = (wp.showCmd == SW_SHOWMAXIMIZED);
    const RECT &r = wp.rcNormalPosition;

    WritePrivateProfileStringW(kSection, L"Maximizado", maximized ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(kSection, L"Izq", std::to_wstring(r.left).c_str(), ini.c_str());
    WritePrivateProfileStringW(kSection, L"Arr", std::to_wstring(r.top).c_str(), ini.c_str());
    WritePrivateProfileStringW(kSection, L"Der", std::to_wstring(r.right).c_str(), ini.c_str());
    WritePrivateProfileStringW(kSection, L"Ab", std::to_wstring(r.bottom).c_str(), ini.c_str());
}

std::vector<std::wstring> WinNemiStateLoadIncludeDirs() {
    std::wstring ini = IniPath();

    // 4096 wide chars is generous for a handful of directories -- if a
    // student somehow needs more, GetPrivateProfileStringW just truncates,
    // it won't crash.
    std::wstring buf(4096, L'\0');
    DWORD len = GetPrivateProfileStringW(kIncludeSection, L"Rutas", L"",
                                          buf.data(), (DWORD)buf.size(), ini.c_str());
    buf.resize(len);

    std::vector<std::wstring> dirs;
    size_t start = 0;
    while (start <= buf.size()) {
        size_t sep = buf.find(L';', start);
        std::wstring part = buf.substr(start, sep == std::wstring::npos
                                                   ? std::wstring::npos
                                                   : sep - start);
        if (!part.empty())
            dirs.push_back(part);
        if (sep == std::wstring::npos)
            break;
        start = sep + 1;
    }
    return dirs;
}

void WinNemiStateSaveIncludeDirs(const std::vector<std::wstring> &dirs) {
    std::wstring ini = IniPath();
    std::wstring joined;
    for (size_t i = 0; i < dirs.size(); ++i) {
        if (i)
            joined += L';';
        joined += dirs[i];
    }
    WritePrivateProfileStringW(kIncludeSection, L"Rutas", joined.c_str(), ini.c_str());
}

std::wstring WinNemiStateImplicitBibcomDir() {
    std::wstring dir = ExeDir() + L"bibcom";
    DWORD attrs = GetFileAttributesW(dir.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return L"";
    return dir;
}

std::wstring WinNemiStateExeDir() {
    return ExeDir();
}
