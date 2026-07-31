/*
 * CmdLineCtl.c - primitive command line inside a Win32 EDIT control,
 * implemented with classic subclassing (SetWindowLongPtrW).
 *
 * Public API is char* interpreted as UTF-8. Internally the control
 * talks to the EDIT window in UTF-16 through explicit ...W calls, so
 * it behaves identically whether or not UNICODE/_UNICODE are defined
 * at compile time, and it accepts any Unicode text.
 *
 * All state is kept per instance in a CmdLnState struct attached to
 * the window with SetPropW, so an application may create any number
 * of command-line controls.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "CmdLineCtl.h"

// Window property holding the per-instance state pointer
#define CMDLN_PROP_STATE L"CmdLnState"

// Per-instance state, owned by the control (freed on WM_NCDESTROY)
typedef struct CmdLnState
{
    WNDPROC origProc;       // Original EDIT window procedure
    CMDLN_CALLBACK callback;
    void *userdata;
    HFONT hFont;
    char inputBuffer[1024]; // UTF-8
    int inputIndex;
    int inputReady;
    int ungetChar;          // Pushed back character (-1 means none)
    int eofFlag;            // Set once end of input has been seen
    char promptString[256]; // UTF-8
    DWORD inputStart;       // Where the current user input begins, in
                            // UTF-16 units; everything before it is
                            // read-only history

    // Command history (circular buffer, UTF-8 entries)
    char *history[CMDLN_HISTORY_SIZE];
    int histCount;          // Valid entries (<= CMDLN_HISTORY_SIZE)
    int histHead;           // Next slot to write
    int histPos;            // Steps back from newest; -1 = not browsing
    char *savedLine;        // Line being typed before browsing started
} CmdLnState;

// strdup is POSIX, not C99, so provide our own
static char *StrDup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy)
        memcpy(copy, s, len);
    return copy;
}

// UTF-8 -> UTF-16 (malloc'ed, caller frees; NULL on failure)
static WCHAR *Utf8ToWide(const char *s)
{
    if (!s)
        return NULL;

    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (len <= 0)
        return NULL;

    WCHAR *w = malloc((size_t)len * sizeof(WCHAR));
    if (w)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, w, len);
    return w;
}

// UTF-16 -> UTF-8 (malloc'ed, caller frees; NULL on failure)
static char *WideToUtf8(const WCHAR *w)
{
    if (!w)
        return NULL;

    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return NULL;

    char *s = malloc((size_t)len);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s, len, NULL, NULL);
    return s;
}

// Retrieve the per-instance state of a command-line control
static CmdLnState *GetState(HWND hwnd)
{
    return (CmdLnState *)GetPropW(hwnd, CMDLN_PROP_STATE);
}

// Convert lone Unix newlines (\n) to Windows newlines (\r\n).
// Byte-oriented but UTF-8-safe: multi-byte sequences never contain
// bytes below 0x80, so '\n' and '\r' cannot be false positives.
static char *ConvertNewlines(const char *input)
{
    if (!input)
        return NULL;

    // Count lone \n characters (not already preceded by \r)
    int lfCount = 0;
    char prev = '\0';
    for (const char *p = input; *p; p++)
    {
        if (*p == '\n' && prev != '\r')
            lfCount++;
        prev = *p;
    }

    size_t inputLen = strlen(input);
    char *result = malloc(inputLen + lfCount + 1); // +lfCount for extra \r chars
    if (!result)
        return NULL;

    const char *src = input;
    char *dst = result;
    prev = '\0';
    while (*src)
    {
        if (*src == '\n' && prev != '\r')
            *dst++ = '\r';
        *dst++ = *src;
        prev = *src;
        src++;
    }
    *dst = '\0';

    return result;
}

// Get current selection of the edit control
static void GetSel(HWND hwnd, DWORD *start, DWORD *end)
{
    SendMessageW(hwnd, EM_GETSEL, (WPARAM)start, (LPARAM)end);
}

// Move caret to the end of the text
static void CaretToEnd(HWND hwnd)
{
    int textLen = GetWindowTextLengthW(hwnd);
    SendMessageW(hwnd, EM_SETSEL, textLen, textLen);
}

// Remove lines from the top of the edit control.
// Returns the number of UTF-16 units removed.
static int RemoveLinesFromTop(HWND hwnd, int linesToRemove)
{
    if (linesToRemove <= 0)
        return 0;

    // Get the character index of the start of the first line to keep
    int charIndex = (int)SendMessageW(hwnd, EM_LINEINDEX, linesToRemove, 0);

    if (charIndex > 0)
    {
        SendMessageW(hwnd, EM_SETSEL, 0, charIndex);
        SendMessageW(hwnd, EM_REPLACESEL, FALSE, (LPARAM)L"");
        return charIndex;
    }
    return 0;
}

// Trim buffer to MAX_LINES if exceeded
static void TrimBuffer(HWND hwnd, CmdLnState *state)
{
    int lineCount = (int)SendMessageW(hwnd, EM_GETLINECOUNT, 0, 0);
    if (lineCount > MAX_LINES)
    {
        int removed = RemoveLinesFromTop(hwnd, lineCount - MAX_LINES);

        // Keep the input anchor consistent with the shifted text
        if ((DWORD)removed >= state->inputStart)
            state->inputStart = 0;
        else
            state->inputStart -= removed;

        // RemoveLinesFromTop left the caret at position 0; put it back
        // at the end so typing continues on the input line.
        CaretToEnd(hwnd);
    }
}

// Extract current command as UTF-8: everything from the input anchor
// to the end of the text
static char *ExtractCurrentCommand(HWND hwnd, CmdLnState *state)
{
    int textLen = GetWindowTextLengthW(hwnd);
    if (textLen <= (int)state->inputStart)
        return NULL;

    WCHAR *buffer = malloc(((size_t)textLen + 1) * sizeof(WCHAR));
    if (!buffer)
        return NULL;

    GetWindowTextW(hwnd, buffer, textLen + 1);

    char *command = WideToUtf8(buffer + state->inputStart);

    free(buffer);
    return command;
}

// Entry 'stepsBack' steps behind the newest one, or NULL if out of range
static const char *HistoryAt(CmdLnState *state, int stepsBack)
{
    if (stepsBack < 0 || stepsBack >= state->histCount)
        return NULL;

    int idx = ((state->histHead - 1 - stepsBack) % CMDLN_HISTORY_SIZE
               + CMDLN_HISTORY_SIZE) % CMDLN_HISTORY_SIZE;
    return state->history[idx];
}

// Append a command to the history (skips empty and consecutive duplicates)
static void HistoryAdd(CmdLnState *state, const char *cmd)
{
    if (!cmd || !cmd[0])
        return;

    // Skip if identical to the newest entry
    const char *newest = HistoryAt(state, 0);
    if (newest && strcmp(newest, cmd) == 0)
        return;

    char *copy = StrDup(cmd);
    if (!copy)
        return;

    free(state->history[state->histHead]); // May be NULL (unused slot)
    state->history[state->histHead] = copy;
    state->histHead = (state->histHead + 1) % CMDLN_HISTORY_SIZE;
    if (state->histCount < CMDLN_HISTORY_SIZE)
        state->histCount++;
}

// Replace the editable region (from the input anchor to the end)
// with the given UTF-8 text
static void ReplaceCurrentInput(HWND hwnd, CmdLnState *state, const char *text)
{
    WCHAR *wide = Utf8ToWide(text);
    if (!wide)
        return;

    int textLen = GetWindowTextLengthW(hwnd);
    SendMessageW(hwnd, EM_SETSEL, state->inputStart, textLen);
    SendMessageW(hwnd, EM_REPLACESEL, FALSE, (LPARAM)wide);
    SendMessageW(hwnd, EM_SCROLLCARET, 0, 0);

    free(wide);
}

// Browse the history: direction +1 = older (Up), -1 = newer (Down)
static void HistoryBrowse(HWND hwnd, CmdLnState *state, int direction)
{
    if (direction > 0)
    {
        const char *entry = HistoryAt(state, state->histPos + 1);
        if (!entry)
            return; // Already at the oldest entry (or history empty)

        if (state->histPos == -1)
        {
            // Leaving the line being typed: remember it
            free(state->savedLine);
            state->savedLine = ExtractCurrentCommand(hwnd, state);
        }
        state->histPos++;
        ReplaceCurrentInput(hwnd, state, entry);
    }
    else
    {
        if (state->histPos == -1)
            return; // Not browsing

        if (state->histPos == 0)
        {
            // Back past the newest entry: restore the typed line
            state->histPos = -1;
            ReplaceCurrentInput(hwnd, state,
                                state->savedLine ? state->savedLine : "");
            free(state->savedLine);
            state->savedLine = NULL;
        }
        else
        {
            state->histPos--;
            ReplaceCurrentInput(hwnd, state, HistoryAt(state, state->histPos));
        }
    }
}

static LRESULT CALLBACK CmdlnProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CmdLnState *state = GetState(hwnd);
    DWORD selStart, selEnd;

    if (!state)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_CHAR:
        if (wParam == VK_BACK)
        {
            GetSel(hwnd, &selStart, &selEnd);

            // Block if the deletion would touch protected text
            if (selStart < state->inputStart ||
                (selStart == selEnd && selStart <= state->inputStart))
                return 0;
        }
        else if (wParam == VK_RETURN)
        {
            // The command is everything after the input anchor,
            // regardless of where the caret happens to be.
            char *command = ExtractCurrentCommand(hwnd, state);

            // Store input for CmdLnGetChar (empty line -> empty buffer,
            // so CmdLnGetChar yields just '\n')
            if (command)
            {
                strncpy(state->inputBuffer, command, sizeof(state->inputBuffer) - 1);
                state->inputBuffer[sizeof(state->inputBuffer) - 1] = '\0';
            }
            else
            {
                state->inputBuffer[0] = '\0';
            }
            state->inputIndex = 0;
            state->inputReady = 1;

            // Record in history and reset browsing state
            if (command)
                HistoryAdd(state, command);
            state->histPos = -1;
            free(state->savedLine);
            state->savedLine = NULL;

            // Make sure the newline is appended at the end
            CaretToEnd(hwnd);

            // Let the edit control insert the newline first
            LRESULT result = CallWindowProcW(state->origProc, hwnd, msg, wParam, lParam);

            // Now call callback after cursor has moved to new line
            if (command && strlen(command) > 0 && state->callback)
                state->callback(hwnd, command, state->userdata);

            if (command)
                free(command);

            // Show prompt for next command (this also updates inputStart)
            ShowPrompt(hwnd);

            TrimBuffer(hwnd, state);
            SendMessageW(hwnd, EM_SCROLLCARET, 0, 0);

            return result;
        }
        else if (wParam >= 32 || wParam == '\t')
        {
            // Printable character typed while the caret (or part of the
            // selection) is in the protected region: jump to the end
            // instead of modifying history.
            GetSel(hwnd, &selStart, &selEnd);
            if (selStart < state->inputStart)
                CaretToEnd(hwnd);
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_DELETE)
        {
            GetSel(hwnd, &selStart, &selEnd);

            // Block if the deletion would touch protected text
            if (selStart < state->inputStart)
                return 0;
        }
        else if (wParam == VK_UP)
        {
            HistoryBrowse(hwnd, state, +1);
            return 0; // Don't let the edit control move the caret
        }
        else if (wParam == VK_DOWN)
        {
            HistoryBrowse(hwnd, state, -1);
            return 0;
        }
        break;

    case WM_PASTE:
        // Redirect pastes that would land in protected text
        GetSel(hwnd, &selStart, &selEnd);
        if (selStart < state->inputStart)
            CaretToEnd(hwnd);
        break;

    case WM_CUT:
        GetSel(hwnd, &selStart, &selEnd);
        if (selStart < state->inputStart)
        {
            // Degrade to a copy so history is not modified
            SendMessageW(hwnd, WM_COPY, 0, 0);
            return 0;
        }
        break;

    case WM_CLEAR:
        GetSel(hwnd, &selStart, &selEnd);
        if (selStart < state->inputStart)
            return 0;
        break;

    case WM_UNDO:
    case EM_UNDO:
        // Undo could resurrect or delete protected text
        return 0;

    case WM_NCDESTROY:
    {
        // Clean up: restore original wndproc, free font and state
        WNDPROC origProc = state->origProc;
        LRESULT result = CallWindowProcW(origProc, hwnd, msg, wParam, lParam);

        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)origProc);
        RemovePropW(hwnd, CMDLN_PROP_STATE);

        if (state->hFont)
            DeleteObject(state->hFont);
        for (int i = 0; i < CMDLN_HISTORY_SIZE; i++)
            free(state->history[i]);
        free(state->savedLine);
        free(state);

        return result;
    }
    }
    return CallWindowProcW(state->origProc, hwnd, msg, wParam, lParam);
}

HWND CreateCmdLine(HWND hParent, int id, int x, int y, int width, int height,
                   CMDLN_CALLBACK callback, void *userdata)
{
    // Allocate and initialize per-instance state
    CmdLnState *state = calloc(1, sizeof(CmdLnState));
    if (!state)
        return NULL;

    state->callback = callback;
    state->userdata = userdata;
    state->ungetChar = -1;
    state->histPos = -1;    // Not browsing (calloc zeroed the rest)
    strcpy(state->promptString, "> ");

    // Create the edit control (explicitly Unicode) with appropriate
    // styles for command line
    HWND hCmdLine = CreateWindowExW(
        0,       // Extended styles
        L"EDIT", // Standard EDIT class
        L"",     // Initial text
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
        x, y, width, height,    // Position and size
        hParent,                // Parent window
        (HMENU)(INT_PTR)id,     // Control ID (unique per parent)
        GetModuleHandleW(NULL), // Instance handle
        NULL                    // Creation parameters
    );

    if (!hCmdLine)
    {
        free(state);
        return NULL;
    }

    // Raise the default text limit (~30000 chars) to the maximum,
    // otherwise EM_REPLACESEL starts failing silently long before
    // MAX_LINES kicks in.
    SendMessageW(hCmdLine, EM_SETLIMITTEXT, 0, 0);

    // Set Lucida Console font for command line appearance
    state->hFont = CreateFontW(
        -14,                     // Height (negative for character height)
        0,                       // Width (0 = default ratio)
        0,                       // Escapement
        0,                       // Orientation
        FW_NORMAL,               // Weight
        FALSE,                   // Italic
        FALSE,                   // Underline
        FALSE,                   // StrikeOut
        DEFAULT_CHARSET,         // CharSet
        OUT_DEFAULT_PRECIS,      // OutputPrecision
        CLIP_DEFAULT_PRECIS,     // ClipPrecision
        DEFAULT_QUALITY,         // Quality
        FIXED_PITCH | FF_MODERN, // PitchAndFamily (monospace)
        L"Lucida Console"        // Font name
    );

    if (state->hFont)
        SendMessageW(hCmdLine, WM_SETFONT, (WPARAM)state->hFont, TRUE);

    // Attach the state, then subclass
    if (!SetPropW(hCmdLine, CMDLN_PROP_STATE, (HANDLE)state))
    {
        if (state->hFont)
            DeleteObject(state->hFont);
        free(state);
        DestroyWindow(hCmdLine);
        return NULL;
    }

    state->origProc = (WNDPROC)SetWindowLongPtrW(hCmdLine, GWLP_WNDPROC, (LONG_PTR)CmdlnProc);
    if (!state->origProc)
    {
        RemovePropW(hCmdLine, CMDLN_PROP_STATE);
        if (state->hFont)
            DeleteObject(state->hFont);
        free(state);
        DestroyWindow(hCmdLine);
        return NULL;
    }

    return hCmdLine;
}

void OutputString(HWND hCmdLine, const char *text)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state || !text)
        return;

    // Convert Unix newlines, then UTF-8 -> UTF-16 for the control
    char *convertedText = ConvertNewlines(text);
    if (!convertedText)
        return;

    WCHAR *wideText = Utf8ToWide(convertedText);
    free(convertedText);
    if (!wideText)
        return;

    // Append at the end of the text
    CaretToEnd(hCmdLine);
    SendMessageW(hCmdLine, EM_REPLACESEL, FALSE, (LPARAM)wideText);

    // Output pushes the input anchor forward
    state->inputStart = GetWindowTextLengthW(hCmdLine);

    TrimBuffer(hCmdLine, state);

    // Auto-scroll to bottom to show new text
    SendMessageW(hCmdLine, EM_SCROLLCARET, 0, 0);

    free(wideText);
}

void CmdLnVPrintf(HWND hCmdLine, const char *format, va_list args)
{
    if (!hCmdLine || !format)
        return;

    // Measure on a copy: the caller's va_list must stay usable
    va_list measure;
    va_copy(measure, args);
    int needed = vsnprintf(NULL, 0, format, measure);
    va_end(measure);

    if (needed <= 0)
        return;

    char *buffer = malloc(needed + 1);
    if (!buffer)
        return;

    vsnprintf(buffer, needed + 1, format, args);

    OutputString(hCmdLine, buffer);

    free(buffer);
}

void CmdLnPrintf(HWND hCmdLine, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    CmdLnVPrintf(hCmdLine, format, args);
    va_end(args);
}

void CmdLnPuts(HWND hCmdLine, const char *s)
{
    // fputs semantics: no newline appended
    OutputString(hCmdLine, s);
}

void CmdLnFlush(HWND hCmdLine)
{
    // Output is unbuffered; just force any pending repaint so the
    // text is visible before a blocking read.
    if (hCmdLine)
        UpdateWindow(hCmdLine);
}

void CmdLnClear(HWND hCmdLine)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return;

    SetWindowTextW(hCmdLine, L"");
    state->inputStart = 0;

    // Discard any pending unread input; keep the command history
    state->inputBuffer[0] = '\0';
    state->inputIndex = 0;
    state->inputReady = 0;
    state->ungetChar = -1;
}

void CmdLnPutChar(HWND hCmdLine, char c)
{
    char str[2] = {c, '\0'};
    OutputString(hCmdLine, str);
}

int CmdLnGetChar(HWND hCmdLine)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return -1;

    if (state->eofFlag)
        return -1;

    // Check if there's a pushed back character
    if (state->ungetChar != -1)
    {
        int c = state->ungetChar;
        state->ungetChar = -1;
        return c;
    }

    // Wait for input to be ready (nested message loop)
    MSG msg;
    while (!state->inputReady)
    {
        BOOL ret = GetMessageW(&msg, NULL, 0, 0);
        if (ret <= 0)
        {
            // WM_QUIT (0) or error (-1): re-post so the main loop
            // also sees it, and report end of input.
            if (ret == 0)
                PostQuitMessage((int)msg.wParam);
            state->eofFlag = 1;
            return -1;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        // The control may have been destroyed while dispatching
        state = GetState(hCmdLine);
        if (!state)
            return -1;
    }

    // Return UTF-8 bytes from the buffer one by one (a non-ASCII
    // character therefore spans several calls)
    if (state->inputIndex < (int)strlen(state->inputBuffer))
        return (unsigned char)state->inputBuffer[state->inputIndex++];

    // End of input - return newline and reset
    state->inputReady = 0;
    state->inputIndex = 0;
    return '\n';
}

char *CmdLnGets(HWND hCmdLine, char *buf, int size)
{
    if (!buf || size <= 0)
        return NULL;

    int i = 0;
    while (i < size - 1)
    {
        int c = CmdLnGetChar(hCmdLine);
        if (c == -1)
            break;              // End of input

        buf[i++] = (char)c;
        if (c == '\n')
            break;              // fgets keeps the newline and stops
    }

    // fgets contract: NULL when end of input arrives with nothing read
    if (i == 0)
        return NULL;

    buf[i] = '\0';
    return buf;
}

int CmdLnEof(HWND hCmdLine)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return 1;               // Destroyed control counts as EOF

    return state->eofFlag;
}

int CmdLnUnGetChar(HWND hCmdLine, int c)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return -1;

    // Can only push back one character (like standard ungetc)
    if (state->ungetChar != -1)
        return -1; // Already have a pushed back character

    state->ungetChar = c;
    return c;
}

void CmdLnInjectLine(HWND hCmdLine, const char *line)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state || !line)
        return;

    // Put the text in the editable region (replacing any partial input),
    // then simulate Enter so it goes through the normal path: history,
    // callback, CmdLnGets buffer, echo.
    ReplaceCurrentInput(hCmdLine, state, line);
    SendMessageW(hCmdLine, WM_CHAR, VK_RETURN, 0);
}

void SetCmdLnPrompt(HWND hCmdLine, const char *prompt)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return;

    if (prompt && strlen(prompt) < sizeof(state->promptString))
        strcpy(state->promptString, prompt);
}

void ShowPrompt(HWND hCmdLine)
{
    CmdLnState *state = GetState(hCmdLine);
    if (!state)
        return;

    // OutputString also updates inputStart to the position
    // right after the prompt.
    OutputString(hCmdLine, state->promptString);
}
