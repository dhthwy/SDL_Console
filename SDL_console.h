#ifndef SDL_CONSOLE
#define SDL_CONSOLE

#include <cstddef>
#include <string>

struct Console_con;
typedef struct Console_con Console_con;

typedef struct _console_color {
    int r, g, b, a;
} Console_Color;

extern "C" {

typedef void* (*Console_SymResolverProc)(const char*);
void Console_Init(Console_SymResolverProc);

Console_con*
Console_Create(const char* title,
    const char* prompt,
    const int font_size);

void Console_Shutdown(Console_con* con);

/*
 * Clean up the console.
 */
bool Console_Destroy(Console_con* con);

/*
 * Set the background color of the console.
 */
void Console_SetBackgroundColor(Console_con* con, Console_Color);

/*
 * Set the font color.
 */
void Console_SetFontColor(Console_con* con, Console_Color);

int Console_GetColumns(Console_con* con);

int Console_GetRows(Console_con* con);

void Console_Clear(Console_con* con);

void Console_SetPrompt(Console_con* con, const char* prompt);

int Console_MainLoop(Console_con* con);

/*
 * Get the last error.
 */
const char*
Console_GetError(void);

void Console_AddLine(Console_con* con, const char* s);

int Console_GetLine(Console_con* con, std::string& buf);

bool Console_HasFocus(Console_con* con);

void Console_SetScrollback(Console_con* con, const int lines);
}

#endif
