/*
 * winmain.c - demo: two independent command-line consoles.
 *
 * Uses explicit ...W calls so it compiles identically with gcc
 * (no UNICODE) and MSVC (/DUNICODE). Strings passed to the
 * CmdLineCtl API are char* UTF-8; save this file as UTF-8.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "CmdLineCtl.h"

const WCHAR g_szClassName[] = L"myWindowClass";

#define ID_CMDLN_TOP    101
#define ID_CMDLN_BOTTOM 102

// One callback serves both controls: hCmdLine tells them apart,
// and userdata carries a per-console name.
void OnCommandLine(HWND hCmdLine, const char *command, void *userdata)
{
    const char *name = (const char *)userdata;

    if (strlen(command) > 0)
        CmdLnPrintf(hCmdLine, "[%s] You typed: %s\n", name, command);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hCmdlnTop, hCmdlnBottom;

    switch (msg)
    {
    case WM_CREATE:
        hCmdlnTop = CreateCmdLine(hwnd, ID_CMDLN_TOP, 0, 0, 0, 0,
                                  OnCommandLine, "console A");
        hCmdlnBottom = CreateCmdLine(hwnd, ID_CMDLN_BOTTOM, 0, 0, 0, 0,
                                     OnCommandLine, "console B");

        if (!hCmdlnTop || !hCmdlnBottom)
        {
            MessageBoxW(hwnd, L"Failed to create command line control.",
                        L"Error", MB_ICONERROR);
            return -1;
        }

        // Each console has its own prompt and history
        SetCmdLnPrompt(hCmdlnTop, "A> ");
        SetCmdLnPrompt(hCmdlnBottom, "B> ");

        CmdLnPrintf(hCmdlnTop, "Console A - Command Line Interface v3.0\n");
        CmdLnPrintf(hCmdlnTop, "Unicode test: \xCF\x80 \xE2\x89\x88 %.5f, "
                               "\xE2\x88\x9A""2 \xE2\x89\x88 %.5f\n\n",
                    3.14159265, 1.41421356);
        CmdLnPrintf(hCmdlnBottom, "Console B - Command Line Interface v3.0\n\n");

        ShowPrompt(hCmdlnTop);
        ShowPrompt(hCmdlnBottom);
        break;

    case WM_SIZE:
        if (hCmdlnTop && hCmdlnBottom)
        {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            int half = h / 2;

            // Split the client area horizontally between the two consoles
            MoveWindow(hCmdlnTop, 0, 0, w, half, TRUE);
            MoveWindow(hCmdlnBottom, 0, half, w, h - half, TRUE);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc;
    HWND hwnd;
    MSG Msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        L"Two command lines, one window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 560, 400,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error!",
                    MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageW(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }
    return (int)Msg.wParam;
}
