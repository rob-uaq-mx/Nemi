// WinNemiIncludeDlg.cpp -- see WinNemiIncludeDlg.h.
#include "WinNemiIncludeDlg.h"

#include <shlobj.h>

#include "WinNemi.h"

namespace {

// Picks a folder with the classic shell browser (no COM initialization
// drama needed for this one, unlike IFileDialog) -- returns true and fills
// `picked` if the user chose a folder, false if they cancelled.
bool BrowseForFolder(HWND hwndOwner, std::wstring &picked) {
    BROWSEINFOW bi = {};
    bi.hwndOwner = hwndOwner;
    bi.lpszTitle = L"Selecciona una carpeta";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl)
        return false;

    wchar_t path[MAX_PATH] = {};
    bool ok = SHGetPathFromIDListW(pidl, path);
    CoTaskMemFree(pidl);
    if (!ok)
        return false;

    picked = path;
    return true;
}

struct IncludeDlgData {
    std::vector<std::wstring> *dirs = nullptr;
    bool accepted = false;
};

INT_PTR CALLBACK IncludeDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, static_cast<LONG_PTR>(lParam));
        auto *data = reinterpret_cast<IncludeDlgData *>(lParam);
        HWND hList = GetDlgItem(hDlg, IDD_INCLUDES_LIST);
        for (const auto &dir : *data->dirs)
            SendMessageW(hList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(dir.c_str()));
        return TRUE;
    }
    case WM_COMMAND: {
        auto *data = reinterpret_cast<IncludeDlgData *>(
            GetWindowLongPtrW(hDlg, DWLP_USER));
        switch (LOWORD(wParam)) {
        case IDD_INCLUDES_ADD: {
            std::wstring picked;
            if (BrowseForFolder(hDlg, picked))
                SendMessageW(GetDlgItem(hDlg, IDD_INCLUDES_LIST), LB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(picked.c_str()));
            return TRUE;
        }
        case IDD_INCLUDES_REMOVE: {
            HWND hList = GetDlgItem(hDlg, IDD_INCLUDES_LIST);
            int sel = static_cast<int>(SendMessageW(hList, LB_GETCURSEL, 0, 0));
            if (sel != LB_ERR)
                SendMessageW(hList, LB_DELETESTRING, sel, 0);
            return TRUE;
        }
        case IDOK: {
            HWND hList = GetDlgItem(hDlg, IDD_INCLUDES_LIST);
            int n = static_cast<int>(SendMessageW(hList, LB_GETCOUNT, 0, 0));
            data->dirs->clear();
            for (int i = 0; i < n; ++i) {
                int len = static_cast<int>(SendMessageW(hList, LB_GETTEXTLEN, i, 0));
                std::wstring s(len, L'\0');
                SendMessageW(hList, LB_GETTEXT, i, reinterpret_cast<LPARAM>(s.data()));
                data->dirs->push_back(std::move(s));
            }
            data->accepted = true;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

}  // namespace

bool WinNemiIncludeDlgShow(HINSTANCE hInst, HWND hwndParent,
                            std::vector<std::wstring> &dirs) {
    IncludeDlgData data;
    data.dirs = &dirs;
    DialogBoxParamW(hInst, L"IncludesDlg", hwndParent, IncludeDlgProc,
                     reinterpret_cast<LPARAM>(&data));
    return data.accepted;
}
