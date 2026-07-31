#ifndef CMDLINECTL_H
#define CMDLINECTL_H

#include <windows.h>
#include <stdarg.h>

// Maximum number of lines to keep in the command line buffer
#define MAX_LINES 1000

// Number of commands remembered per console (circular history)
#define CMDLN_HISTORY_SIZE 50

// NOTE: all char* strings in this API are UTF-8. Internally the
// control works in UTF-16, so any Unicode text is supported.

// Callback function type for processing command lines.
// hCmdLine identifies which control issued the command, so one
// callback can serve several controls.
typedef void (*CMDLN_CALLBACK)(HWND hCmdLine, const char *command, void *userdata);

// Create a command-line control. Several controls may coexist in the
// same application; each keeps its own prompt, input buffer and state.
// 'id' is the child-window control ID (must be unique within the parent).
HWND CreateCmdLine(HWND hParent, int id, int x, int y, int width, int height,
                   CMDLN_CALLBACK callback, void *userdata);

// ----- Output (stdio analogues) -------------------------------------
// The set below mirrors the <stdio.h> console functions one to one, so
// porting a standard command-line program is a mechanical substitution:
//   printf/fprintf(stdout) -> CmdLnPrintf     putc/putchar -> CmdLnPutChar
//   fputs(s, stdout)       -> CmdLnPuts       vfprintf     -> CmdLnVPrintf
//   fgets(buf, n, stdin)   -> CmdLnGets       fflush       -> CmdLnFlush
//   getchar/ungetc         -> CmdLnGetChar/CmdLnUnGetChar
//   feof(stdin)            -> CmdLnEof
// fprintf(stderr, ...) maps to CmdLnPrintf on the same control, which
// is what a real terminal does when both streams are interleaved.

void OutputString(HWND hCmdLine, const char *text);

// printf: formatted output (the result is written as UTF-8 text)
void CmdLnPrintf(HWND hCmdLine, const char *format, ...);

// vfprintf: same, taking a va_list, so client programs can build their
// own variadic wrappers (e.g. an error-reporting function)
void CmdLnVPrintf(HWND hCmdLine, const char *format, va_list args);

// fputs: writes the string as-is, WITHOUT appending a newline
// (note: ISO puts() appends '\n'; this one follows fputs semantics)
void CmdLnPuts(HWND hCmdLine, const char *s);

// putc/putchar: single character
void CmdLnPutChar(HWND hCmdLine, char c);

// fflush: output is already unbuffered, but this forces any pending
// repaint so the text is visible before a blocking read
void CmdLnFlush(HWND hCmdLine);

// Clear the whole console buffer and reset state (BASIC's CLS).
// The command history is preserved, like in a real terminal.
void CmdLnClear(HWND hCmdLine);

// ----- Blocking input (stdio analogues) -----------------------------

// getchar: blocks (pumping messages) until the user presses Enter,
// then yields the line as UTF-8 bytes one call at a time, followed by
// '\n'. Returns -1 on end of input (WM_QUIT or control destroyed).
int CmdLnGetChar(HWND hCmdLine);

// ungetc: pushes back a single character (one level, like ungetc)
int CmdLnUnGetChar(HWND hCmdLine, int c);

// fgets: reads one line (blocking); keeps the trailing '\n' and
// NUL-terminates, reading at most size-1 bytes. Returns buf, or NULL
// on end of input with nothing read -- so the idiomatic stdio check
//   if (CmdLnGets(h, line, sizeof line) == NULL) ...
// works unchanged.
char *CmdLnGets(HWND hCmdLine, char *buf, int size);

// feof: nonzero once end of input has been seen
int CmdLnEof(HWND hCmdLine);

// Inject a line programmatically, exactly as if the user had typed it
// and pressed Enter: the text appears in the console, goes through the
// normal Enter path (callback, CmdLnGets, history) and is echoed.
// Useful for menu items that generate commands (File->Open -> LOAD).
void CmdLnInjectLine(HWND hCmdLine, const char *line);

// ----- Prompt --------------------------------------------------------
// A program that prints its own prompt (like a typical REPL) should
// disable the built-in one with SetCmdLnPrompt(hwnd, "").

void SetCmdLnPrompt(HWND hCmdLine, const char *prompt);
void ShowPrompt(HWND hCmdLine);

#endif
