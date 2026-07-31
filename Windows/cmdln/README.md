# CmdLineCtl — a command-line control for Win32

A primitive interactive command line hosted inside a standard Win32 `EDIT`
control, implemented in C through window subclassing. Multiple independent
consoles can coexist in the same application, each with its own prompt,
input state, and command history.

The included demo (`winmain.c`) creates one window split into two
independent consoles that echo whatever is typed.

## Files

```
include/CmdLineCtl.h    Public API
src/CmdLineCtl.c        The control implementation
src/winmain.c           Demo application (two consoles in one window)
makefile                GNU make / MinGW (gcc)
makefile.vc             NMAKE / Visual Studio (cl)
```

## Building

### MinGW (gcc)

```
make
```

### Visual Studio (cl)

From a *Developer Command Prompt for VS*:

```
nmake /f makefile.vc
```

Both produce `bin\wincmdln.exe`.

**Note on line endings:** NMAKE expects CRLF line endings in makefiles.
If NMAKE silently ignores rules or does nothing, convert the file first:

```
powershell -Command "(Get-Content makefile.vc) | Set-Content makefile.vc"
```

## Public API

All functions take the `HWND` of the control, so any number of controls
can be used side by side.

```c
// Creation. 'id' is the child-window control ID (unique per parent).
HWND CreateCmdLine(HWND hParent, int id, int x, int y, int width, int height,
                   CMDLN_CALLBACK callback, void *userdata);

// Output
void OutputString(HWND hCmdLine, const char *text);
void CmdLnPrintf(HWND hCmdLine, const char *format, ...);   // printf
void CmdLnVPrintf(HWND hCmdLine, const char *format, va_list args); // vfprintf
void CmdLnPuts(HWND hCmdLine, const char *s);               // fputs (no '\n' added)
void CmdLnPutChar(HWND hCmdLine, char c);                   // putchar
void CmdLnFlush(HWND hCmdLine);                             // fflush
void CmdLnClear(HWND hCmdLine);                             // clear screen (CLS)

// Blocking input (stdio style)
int   CmdLnGetChar(HWND hCmdLine);                          // getchar
int   CmdLnUnGetChar(HWND hCmdLine, int c);                 // ungetc
char *CmdLnGets(HWND hCmdLine, char *buf, int size);        // fgets
int   CmdLnEof(HWND hCmdLine);                              // feof

// Prompt
void SetCmdLnPrompt(HWND hCmdLine, const char *prompt);
void ShowPrompt(HWND hCmdLine);
```

The set mirrors the `<stdio.h>` console functions one to one, so porting
a standard command-line program (a REPL, an interpreter, a calculator)
is a mechanical substitution: `printf` → `CmdLnPrintf`, `fgets(buf, n,
stdin)` → `CmdLnGets`, `putc(c, stdout)` → `CmdLnPutChar`, `fflush` →
`CmdLnFlush`, and `fprintf(stderr, ...)` → `CmdLnPrintf` on the same
control (a terminal interleaves both streams anyway). `CmdLnGets`
follows the exact `fgets` contract — keeps the trailing `'\n'`, reads at
most `size-1` bytes, returns `NULL` on end of input — so idiomatic EOF
checks work unchanged. A program that prints its own prompt should
disable the built-in one with `SetCmdLnPrompt(hwnd, "")`.

Commands are delivered through a callback:

```c
typedef void (*CMDLN_CALLBACK)(HWND hCmdLine, const char *command,
                               void *userdata);
```

`hCmdLine` identifies which console issued the command, so one callback
can serve several controls; `userdata` is whatever pointer was passed to
`CreateCmdLine`.

Alternatively, input can be consumed character by character with
`CmdLnGetChar`, which blocks inside a nested message loop until the user
presses Enter, then yields the line one byte at a time followed by `'\n'`.
It returns `-1` on `WM_QUIT` (the quit message is re-posted so the main
loop also sees it).

## Unicode design

**All `char*` strings in the public API are UTF-8.** Internally the
control works exclusively in UTF-16.

Following Microsoft's current recommendation, the implementation calls
the wide-character API functions **explicitly** (`CreateWindowExW`,
`GetWindowTextW`, `SendMessageW`, `SetPropW`, ...) instead of relying on
the generic-name macros (`CreateWindowEx`, ...) that expand to the A or W
variant depending on whether `UNICODE` is defined. The TCHAR/generic-name
mechanism was designed in the 1990s to build the same source for the
ANSI-only Windows 9x line and for Windows NT; that portability problem no
longer exists, and Microsoft now advises calling the W functions directly
and treating TCHAR as legacy.

Consequences of this choice:

- The code compiles and behaves **identically** with or without
  `UNICODE`/`_UNICODE` defined, under both gcc and MSVC.
- Any Unicode text works end to end: Greek, Cyrillic, math symbols
  (`π ≈ 3.14159`, `√2 ≈ 1.41421`), etc.
- Client programs keep using plain `char*` and ordinary string literals;
  since ASCII is a subset of UTF-8, existing code needs no changes. To
  embed non-ASCII text in literals, save the source file as UTF-8
  (`makefile.vc` passes `/utf-8` to cl; gcc assumes UTF-8 by default).

Conversion happens only at the boundary, via `MultiByteToWideChar` /
`WideCharToMultiByte` (`CP_UTF8`). Internal positions (the input anchor,
selection indexes) are counted in UTF-16 code units, consistently
measured with the W functions.

One consequence for `CmdLnGetChar`: input is delivered as UTF-8 bytes,
so a non-ASCII character spans several calls — the same behavior as
`getchar()` on a UTF-8 Linux terminal.

## Implementation notes

### Subclassing

The control subclasses the `EDIT` window procedure the classic way, with
`SetWindowLongPtrW(GWLP_WNDPROC, ...)` and `CallWindowProcW` to chain to
the original procedure.

The comctl32 helpers (`SetWindowSubclass`/`DefSubclassProc`) are
deliberately **not** used: without a side-by-side manifest an application
loads comctl32.dll version 5.82, which exports those functions by ordinal
only, while MinGW's import library references them by name — producing
"entry point not found" at run time.

### Per-instance state

All state lives in a heap-allocated `CmdLnState` struct attached to the
window with `SetPropW`. Nothing is global, which is what allows several
controls per process. On `WM_NCDESTROY` the control restores the original
window procedure, removes the property, and frees the font, the history,
and the state itself.

### Protected input region

`inputStart` (the *input anchor*) marks where the current user input
begins — right after the prompt. Everything before it is read-only
history:

- Typing with the caret inside the history jumps the caret to the end
  instead of modifying it.
- `Backspace`/`Delete` are blocked when they would touch protected text.
- Pasting into the history is redirected to the end; Cut degrades to
  Copy; undo is disabled (it could resurrect or delete protected text).
- Navigation and selection remain free, so the user can scroll, select,
  and copy previous output.

The anchor is updated every time output is appended, and adjusted when
old lines are trimmed.

### Command history

Each console keeps a circular buffer of `CMDLN_HISTORY_SIZE` (50)
commands. `Up`/`Down` browse older/newer entries; going past the newest
entry restores the line that was being typed. Empty lines and consecutive
duplicates are not recorded (like bash's `ignoredups`).

### Buffer trimming

The text buffer is capped at `MAX_LINES` (1000) lines; older lines are
removed from the top. The edit control's default text limit (~30,000
characters) is raised to the maximum at creation time, since otherwise
`EM_REPLACESEL` starts failing silently long before the line cap is
reached. Note that lines are counted as *displayed* (wrapped) lines, so a
long output line may consume several units of the cap.

## Limitations

- Input lines are capped at 1023 UTF-8 bytes for `CmdLnGetChar`
  (`inputBuffer` size); the callback receives the full line regardless.
- `CmdLnUnGetChar` can push back a single character, like `ungetc`.
- Output appended while the user is typing moves the anchor past the
  partial input, effectively freezing it into the history.
- One `CmdLnGetChar` nested message loop per thread at a time is the
  intended usage pattern.
