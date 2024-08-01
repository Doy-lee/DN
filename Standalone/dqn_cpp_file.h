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
//   dqn_cpp_file.h -- Functions to emit C++ formatted code
//
////////////////////////////////////////////////////////////////////////////////
*/
#if !defined(DQN_CPP_FILE_H)
#define DQN_CPP_FILE_H

#include <stdio.h>  /// printf, fputc
#include <stdarg.h> /// va_list...
#include <assert.h> /// assert

typedef struct Dqn_CppFile { ///< Maintains state for printing C++ style formatted files
    FILE          *file;             ///< (Write) File to print to
    int            indent;           ///< Current indent level of the printer
    int            space_per_indent; ///< (Write) Number of spaces per indent
    unsigned char  if_chain[256];    ///
    unsigned char  if_chain_size;    ///
} Dqn_CppFile;

#define Dqn_CppSpacePerIndent(cpp) ((cpp) && (cpp)->space_per_indent) ? ((cpp)->space_per_indent) : 4

/// Print the format string indented and terminate the string with a new-line.
void Dqn_CppLineV(Dqn_CppFile *cpp, char const *fmt, va_list args);
void Dqn_CppLine(Dqn_CppFile *cpp, char const *fmt, ...);

/// Print the format string indented
void Dqn_CppPrintV(Dqn_CppFile *cpp, char const *fmt, va_list args);
void Dqn_CppPrint(Dqn_CppFile *cpp, char const *fmt, ...);

/// Print the format string
#define Dqn_CppAppend(cpp, fmt, ...) fprintf((cpp)->file, fmt, ##__VA_ARGS__)
#define Dqn_CppAppendV(cpp, fmt, args) vfprintf((cpp)->file, fmt, args)

/// End the current line, useful after CppPrint and CppAppend
#define Dqn_CppNewLine(cpp) fputc('\n', (cpp)->file)

/// Manually modify the indent level
#define Dqn_CppIndent(cpp) (cpp)->indent++
#define Dqn_CppUnindent(cpp) (cpp)->indent--; assert((cpp)->indent >= 0)

/// Block scope functions that execute a function on entry and exit of the 
/// scope by exploiting the comma operator and a for loop.
///
/// @code
/// Dqn_CppEnumBlock(cpp, "abc") {
///     printf("Hello world!");
/// }
///
/// // Is equivalent to
///
/// Dqn_CppBeginBlock(cpp, "abc");
/// printf("Hello world!");
/// Dqn_CppEndEnumBlock(cpp);
/// @endcode

#define Dqn_CppEnumBlock(cpp, fmt, ...)                          \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginEnumBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndEnumBlock(cpp), false))

#define Dqn_CppForBlock(cpp, fmt, ...)                           \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginForBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndForBlock(cpp), false))

#define Dqn_CppWhileBlock(cpp, fmt, ...)                           \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginWhileBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndWhileBlock(cpp), false))

#define Dqn_CppIfOrElseIfBlock(cpp, fmt, ...)             \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginIfOrElseIfBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndIfOrElseIfBlock(cpp), false))

#define Dqn_CppElseBlock(cpp)                             \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginElseBlock(cpp), true);          \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndElseBlock(cpp), false))

#define Dqn_CppFuncBlock(cpp, fmt, ...)                          \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginFuncBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndFuncBlock(cpp), false))

#define Dqn_CppStructBlock(cpp, fmt, ...)                        \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginStructBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndStructBlock(cpp), false))

#define Dqn_CppSwitchBlock(cpp, fmt, ...)                        \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginSwitchBlock(cpp, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndSwitchBlock(cpp), false))

#define Dqn_CppBlock(cpp, ending, fmt, ...)                      \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) =            \
             (Dqn_CppBeginBlock(cpp, false /*append*/, fmt, ##__VA_ARGS__), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                  \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppEndBlock(cpp, ending), false))

#define Dqn_CppIfChain(cpp)                                                             \
    for (bool DQN_CPP_TOKEN_PASTE_(once_, __LINE__) = (Dqn_CppBeginIfChain(cpp), true); \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__);                                         \
         DQN_CPP_TOKEN_PASTE_(once_, __LINE__)      = (Dqn_CppEndIfChain(cpp), false))

/// Print the format string followed by a "{" and enter a new line whilst
/// increasing the indent level after the brace.
void Dqn_CppBeginBlock (Dqn_CppFile *cpp, bool append, char const *fmt, ...);
void Dqn_CppBeginBlockV(Dqn_CppFile *cpp, bool append, char const *fmt, va_list args);
void Dqn_CppEndBlock   (Dqn_CppFile *cpp, char const *ending);

/// Begin/End a block, specifically for the following language constructs.
#define Dqn_CppBeginEnumBlock(cpp, fmt, ...)   Dqn_CppBeginBlock(cpp, false /*append*/, "enum " fmt, ##__VA_ARGS__)
#define Dqn_CppEndEnumBlock(cpp)               Dqn_CppEndBlock(cpp, ";\n")

#define Dqn_CppBeginWhileBlock(cpp, fmt, ...)  Dqn_CppBeginBlock(cpp, false /*append*/, "while (" fmt ")", ##__VA_ARGS__)
#define Dqn_CppEndWhileBlock(cpp)              Dqn_CppEndBlock(cpp, "\n")

#define Dqn_CppBeginForBlock(cpp, fmt, ...)    Dqn_CppBeginBlock(cpp, false /*append*/, "for (" fmt ")", ##__VA_ARGS__)
#define Dqn_CppEndForBlock(cpp)                Dqn_CppEndBlock(cpp, "\n")

#define Dqn_CppBeginFuncBlock(cpp, fmt, ...)   Dqn_CppBeginBlock(cpp, false /*append*/, fmt, ##__VA_ARGS__)
#define Dqn_CppEndFuncBlock(cpp)               Dqn_CppEndBlock(cpp, "\n")

#define Dqn_CppBeginStructBlock(cpp, fmt, ...) Dqn_CppBeginBlock(cpp, false /*append*/, "struct " fmt, ##__VA_ARGS__)
#define Dqn_CppEndStructBlock(cpp)             Dqn_CppEndBlock(cpp, ";\n")

#define Dqn_CppBeginSwitchBlock(cpp, fmt, ...) Dqn_CppBeginBlock(cpp, false /*append*/, "switch (" fmt ")", ##__VA_ARGS__)
#define Dqn_CppEndSwitchBlock(cpp)             Dqn_CppEndBlock(cpp, "\n")

void    Dqn_CppBeginIfOrElseIfBlock            (Dqn_CppFile *cpp, char const *fmt, ...);
#define Dqn_CppEndIfOrElseIfBlock(cpp)         Dqn_CppEndBlock(cpp, "")

void    Dqn_CppBeginElseBlock                  (Dqn_CppFile *cpp);
void    Dqn_CppEndElseBlock                    (Dqn_CppFile *cpp);

#define DQN_CPP_TOKEN_PASTE2_(x, y) x ## y
#define DQN_CPP_TOKEN_PASTE_(x, y) DQN_CPP_TOKEN_PASTE2_(x, y)
#endif // DQN_CPP_FILE_H

#if defined(DQN_CPP_FILE_IMPLEMENTATION)
void Dqn_CppLineV(Dqn_CppFile *cpp, char const *fmt, va_list args)
{
    Dqn_CppPrintV(cpp, fmt, args);
    Dqn_CppNewLine(cpp);
}

void Dqn_CppLine(Dqn_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_CppLineV(cpp, fmt, args);
    va_end(args);
}

void Dqn_CppPrintV(Dqn_CppFile *cpp, char const *fmt, va_list args)
{
    int space_per_indent = Dqn_CppSpacePerIndent(cpp);
    int spaces           = fmt ? (cpp->indent * space_per_indent) : 0;
    fprintf(cpp->file, "%*s", spaces, "");
    vfprintf(cpp->file, fmt, args);
}

void Dqn_CppPrint(Dqn_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_CppPrintV(cpp, fmt, args);
    va_end(args);
}

void Dqn_CppBeginBlock(Dqn_CppFile *cpp, bool append, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_CppBeginBlockV(cpp, append, fmt, args);
    va_end(args);
}

void Dqn_CppBeginBlockV(Dqn_CppFile *cpp, bool append, char const *fmt, va_list args)
{
    if (append)
        Dqn_CppAppendV(cpp, fmt, args);
    else
        Dqn_CppPrintV(cpp, fmt, args);

    bool empty_fmt = fmt == nullptr || strlen(fmt) == 0;
    Dqn_CppAppend(cpp, "%s{\n", empty_fmt ? "" : " ");
    Dqn_CppIndent(cpp);
}

void Dqn_CppEndBlock(Dqn_CppFile *cpp, char const *ending)
{
    Dqn_CppUnindent(cpp);
    Dqn_CppPrint(cpp, "}%s", ending);
}

void Dqn_CppBeginIfOrElseIfBlock(Dqn_CppFile *cpp, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] == 0)
        Dqn_CppPrint(cpp, "if");
    else
        Dqn_CppAppend(cpp, " else if");

    Dqn_CppAppend(cpp, " (");
    Dqn_CppAppendV(cpp, fmt, args);
    Dqn_CppAppend(cpp, ") {\n");
    Dqn_CppIndent(cpp);
    va_end(args);
    cpp->if_chain[cpp->if_chain_size - 1]++;
}

void Dqn_CppBeginElseBlock(Dqn_CppFile *cpp)
{
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1)
        Dqn_CppBeginBlock(cpp, true /*append*/, " else");
}

void Dqn_CppEndElseBlock(Dqn_CppFile *cpp)
{
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1)
        Dqn_CppEndBlock(cpp, "");
}

void Dqn_CppBeginIfChain(Dqn_CppFile *cpp)
{
    assert(cpp->if_chain_size < sizeof(cpp->if_chain)/sizeof(cpp->if_chain[0]));
    cpp->if_chain_size++;
}

void Dqn_CppEndIfChain(Dqn_CppFile *cpp)
{
    assert(cpp->if_chain_size);
    if (cpp->if_chain[cpp->if_chain_size - 1] >= 1) {
        Dqn_CppNewLine(cpp);
    }
    cpp->if_chain[cpp->if_chain_size - 1] = 0;
    cpp->if_chain_size--;
}

#endif // DQN_CPP_FILE_IMPLEMENTATION
