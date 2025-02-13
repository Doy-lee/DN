/*
////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$$$$$$\  $$$$$$$\        $$$$$$$$\ $$$$$$\ $$\       $$$$$$$$\
//   $$  __$$\ $$  __$$\ $$  __$$\       $$  _____|\_$$  _|$$ |      $$  _____|
//   $$ /  \__|$$ |  $$ |$$ |  $$ |      $$ |        $$ |  $$ |      $$ |
//   $$ |      $$$$$$$  |$$$$$$$  |      $$$$$\      $$ |  $$ |      $$$$$\
//   $$ |      $$  ____/ $$  ____/       $$  __|     $$ |  $$ |      $$  __|
//   $$ |  $$\ $$ |      $$ |            $$ |        $$ |  $$ |      $$ |
//   \$$$$$$  |$$ |      $$ |            $$ |      $$$$$$\ $$$$$$$$\ $$$$$$$$\
//    \______/ \__|      \__|            \__|      \______|\________|\________|
//
//   dn_cpp_file.h -- Functions to emit C++ formatted code
//
////////////////////////////////////////////////////////////////////////////////
*/
#if !defined(DN_CPP_FILE_H)
#define DN_CPP_FILE_H

#include <stdio.h>  /// printf, fputc
#include <stdarg.h> /// va_list...
#include <assert.h> /// assert

typedef struct DN_CppFile { ///< Maintains state for printing C++ style formatted files
    FILE          *file;             ///< (Write) File to print to
    int            indent;           ///< Current indent level of the printer
    int            space_per_indent; ///< (Write) Number of spaces per indent
    unsigned char  if_chain[256];    ///
    unsigned char  if_chain_size;    ///
} DN_CppFile;

#define DN_CppSpacePerIndent(cpp) ((cpp) && (cpp)->space_per_indent) ? ((cpp)->space_per_indent) : 4

/// Print the format string indented and terminate the string with a new-line.
void DN_CppLineV(DN_CppFile *cpp, char const *fmt, va_list args);
void DN_CppLine(DN_CppFile *cpp, char const *fmt, ...);

/// Print the format string indented
void DN_CppPrintV(DN_CppFile *cpp, char const *fmt, va_list args);
void DN_CppPrint(DN_CppFile *cpp, char const *fmt, ...);

/// Print the format string
#define DN_CppAppend(cpp, fmt, ...) fprintf((cpp)->file, fmt, ##__VA_ARGS__)
#define DN_CppAppendV(cpp, fmt, args) vfprintf((cpp)->file, fmt, args)

/// End the current line, useful after CppPrint and CppAppend
#define DN_CppNewLine(cpp) fputc('\n', (cpp)->file)

/// Manually modify the indent level
#define DN_CppIndent(cpp) (cpp)->indent++
#define DN_CppUnindent(cpp) (cpp)->indent--; assert((cpp)->indent >= 0)

/// Block scope functions that execute a function on entry and exit of the 
/// scope by exploiting the comma operator and a for loop.
///
/// @code
/// DN_CppEnumBlock(cpp, "abc") {
///     printf("Hello world!");
/// }
///
/// // Is equivalent to
///
/// DN_CppBeginBlock(cpp, "abc");
/// printf("Hello world!");
/// DN_CppEndEnumBlock(cpp);
/// @endcode

#define DN_CppEnumBlock(cpp, fmt, ...)                          \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginEnumBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndEnumBlock(cpp), false))

#define DN_CppForBlock(cpp, fmt, ...)                           \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginForBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndForBlock(cpp), false))

#define DN_CppWhileBlock(cpp, fmt, ...)                           \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginWhileBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndWhileBlock(cpp), false))

#define DN_CppIfOrElseIfBlock(cpp, fmt, ...)             \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginIfOrElseIfBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndIfOrElseIfBlock(cpp), false))

#define DN_CppElseBlock(cpp)                             \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginElseBlock(cpp), true);          \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndElseBlock(cpp), false))

#define DN_CppFuncBlock(cpp, fmt, ...)                          \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginFuncBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndFuncBlock(cpp), false))

#define DN_CppStructBlock(cpp, fmt, ...)                        \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginStructBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndStructBlock(cpp), false))

#define DN_CppSwitchBlock(cpp, fmt, ...)                        \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginSwitchBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndSwitchBlock(cpp), false))

#define DN_CppBlock(cpp, ending, fmt, ...)                      \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (DN_CppBeginBlock(cpp, false /*append*/, fmt, ##__VA_ARGS__), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppEndBlock(cpp, ending), false))

#define DN_CppIfChain(cpp)                                                             \
    for (bool DN_CPP_TOKEN_PASTE_(once_, __LINE__) = (DN_CppBeginIfChain(cpp), true); \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__);                                         \
         DN_CPP_TOKEN_PASTE_(once_, __LINE__)      = (DN_CppEndIfChain(cpp), false))

/// Print the format string followed by a "{" and enter a new line whilst
/// increasing the indent level after the brace.
void DN_CppBeginBlock (DN_CppFile *cpp, bool append, char const *fmt, ...);
void DN_CppBeginBlockV(DN_CppFile *cpp, bool append, char const *fmt, va_list args);
void DN_CppEndBlock   (DN_CppFile *cpp, char const *ending);

/// Begin/End a block, specifically for the following language constructs.
#define DN_CppBeginEnumBlock(cpp, fmt, ...)   DN_CppBeginBlock(cpp, false /*append*/, "enum " fmt, ##__VA_ARGS__)
#define DN_CppEndEnumBlock(cpp)               DN_CppEndBlock(cpp, ";\n")

#define DN_CppBeginWhileBlock(cpp, fmt, ...)  DN_CppBeginBlock(cpp, false /*append*/, "while (" fmt ")", ##__VA_ARGS__)
#define DN_CppEndWhileBlock(cpp)              DN_CppEndBlock(cpp, "\n")

#define DN_CppBeginForBlock(cpp, fmt, ...)    DN_CppBeginBlock(cpp, false /*append*/, "for (" fmt ")", ##__VA_ARGS__)
#define DN_CppEndForBlock(cpp)                DN_CppEndBlock(cpp, "\n")

#define DN_CppBeginFuncBlock(cpp, fmt, ...)   DN_CppBeginBlock(cpp, false /*append*/, fmt, ##__VA_ARGS__)
#define DN_CppEndFuncBlock(cpp)               DN_CppEndBlock(cpp, "\n")

#define DN_CppBeginStructBlock(cpp, fmt, ...) DN_CppBeginBlock(cpp, false /*append*/, "struct " fmt, ##__VA_ARGS__)
#define DN_CppEndStructBlock(cpp)             DN_CppEndBlock(cpp, ";\n")

#define DN_CppBeginSwitchBlock(cpp, fmt, ...) DN_CppBeginBlock(cpp, false /*append*/, "switch (" fmt ")", ##__VA_ARGS__)
#define DN_CppEndSwitchBlock(cpp)             DN_CppEndBlock(cpp, "\n")

void    DN_CppBeginIfOrElseIfBlock            (DN_CppFile *cpp, char const *fmt, ...);
#define DN_CppEndIfOrElseIfBlock(cpp)         DN_CppEndBlock(cpp, "")

void    DN_CppBeginElseBlock                  (DN_CppFile *cpp);
void    DN_CppEndElseBlock                    (DN_CppFile *cpp);

#define DN_CPP_TOKEN_PASTE2_(x, y) x ## y
#define DN_CPP_TOKEN_PASTE_(x, y) DN_CPP_TOKEN_PASTE2_(x, y)
#endif // DN_CPP_FILE_H

#if defined(DN_CPP_FILE_IMPLEMENTATION)
void DN_CppLineV(DN_CppFile *cpp, char const *fmt, va_list args)
{
    DN_CppPrintV(cpp, fmt, args);
    DN_CppNewLine(cpp);
}

void DN_CppLine(DN_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_CppLineV(cpp, fmt, args);
    va_end(args);
}

void DN_CppPrintV(DN_CppFile *cpp, char const *fmt, va_list args)
{
    int space_per_indent = DN_CppSpacePerIndent(cpp);
    int spaces           = fmt ? (cpp->indent * space_per_indent) : 0;
    fprintf(cpp->file, "%*s", spaces, "");
    vfprintf(cpp->file, fmt, args);
}

void DN_CppPrint(DN_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_CppPrintV(cpp, fmt, args);
    va_end(args);
}

void DN_CppBeginBlock(DN_CppFile *cpp, bool append, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_CppBeginBlockV(cpp, append, fmt, args);
    va_end(args);
}

void DN_CppBeginBlockV(DN_CppFile *cpp, bool append, char const *fmt, va_list args)
{
    if (append)
        DN_CppAppendV(cpp, fmt, args);
    else
        DN_CppPrintV(cpp, fmt, args);

    bool empty_fmt = fmt == nullptr || strlen(fmt) == 0;
    DN_CppAppend(cpp, "%s{\n", empty_fmt ? "" : " ");
    DN_CppIndent(cpp);
}

void DN_CppEndBlock(DN_CppFile *cpp, char const *ending)
{
    DN_CppUnindent(cpp);
    DN_CppPrint(cpp, "}%s", ending);
}

void DN_CppBeginIfOrElseIfBlock(DN_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] == 0)
        DN_CppPrint(cpp, "if");
    else
        DN_CppAppend(cpp, " else if");

    DN_CppAppend(cpp, " (");
    DN_CppAppendV(cpp, fmt, args);
    DN_CppAppend(cpp, ") {\n");
    DN_CppIndent(cpp);
    va_end(args);
    cpp->if_chain[cpp->if_chain_size - 1]++;
}

void DN_CppBeginElseBlock(DN_CppFile *cpp)
{
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1)
        DN_CppBeginBlock(cpp, true /*append*/, " else");
}

void DN_CppEndElseBlock(DN_CppFile *cpp)
{
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1)
        DN_CppEndBlock(cpp, "");
}

void DN_CppBeginIfChain(DN_CppFile *cpp)
{
    assert(cpp->if_chain_size < sizeof(cpp->if_chain)/sizeof(cpp->if_chain[0]));
    cpp->if_chain_size++;
}

void DN_CppEndIfChain(DN_CppFile *cpp)
{
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1) {
        DN_CppNewLine(cpp);
    }
    cpp->if_chain[cpp->if_chain_size - 1] = 0;
    cpp->if_chain_size--;
}

#endif // DN_CPP_FILE_IMPLEMENTATION
