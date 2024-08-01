#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\ $$$$$$$$\ $$$$$$$\  $$$$$$\ $$\   $$\  $$$$$$\
//   $$  __$$\\__$$  __|$$  __$$\ \_$$  _|$$$\  $$ |$$  __$$\
//   $$ /  \__|  $$ |   $$ |  $$ |  $$ |  $$$$\ $$ |$$ /  \__|
//   \$$$$$$\    $$ |   $$$$$$$  |  $$ |  $$ $$\$$ |$$ |$$$$\
//    \____$$\   $$ |   $$  __$$<   $$ |  $$ \$$$$ |$$ |\_$$ |
//   $$\   $$ |  $$ |   $$ |  $$ |  $$ |  $$ |\$$$ |$$ |  $$ |
//   \$$$$$$  |  $$ |   $$ |  $$ |$$$$$$\ $$ | \$$ |\$$$$$$  |
//    \______/   \__|   \__|  \__|\______|\__|  \__| \______/
//
//   dqn_string.h -- UTF8/16 string manipulation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$CSTR] Dqn_CStr8       -- C-string helpers
// [$STR8] Dqn_Str8        -- Pointer and length strings
// [$STRB] Dqn_Str8Builder -- Construct strings dynamically
// [$FSTR] Dqn_FStr8       -- Fixed-size strings
// [$CHAR] Dqn_Char        -- Character ascii/digit.. helpers
// [$UTFX] Dqn_UTF         -- Unicode helpers
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$STR8] Dqn_Str8 //////////////////////////////////////////////////////////////////////////
struct Dqn_Str8Link
{
    Dqn_Str8      string; // The string
    Dqn_Str8Link *next;   // The next string in the linked list
};

struct Dqn_Str16 // A pointer and length style string that holds slices to UTF16 bytes.
{
    wchar_t   *data; // The UTF16 bytes of the string
    Dqn_usize  size; // The number of characters in the string

    #if defined(__cplusplus)
    wchar_t const *begin() const { return data; }        // Const begin iterator for range-for loops
    wchar_t const *end  () const { return data + size; } // Const end iterator for range-for loops
    wchar_t       *begin()       { return data; }        // Begin iterator for range-for loops
    wchar_t       *end  ()       { return data + size; } // End iterator for range-for loops
    #endif
};

struct Dqn_Str8BinarySplitResult
{
    Dqn_Str8 lhs;
    Dqn_Str8 rhs;
};

struct Dqn_Str8FindResult
{
    bool      found;                  // True if string was found. If false, the subsequent fields below are not set.
    Dqn_usize index;                  // The index in the buffer where the found string starts
    Dqn_Str8  match;                  // The matching string in the buffer that was searched
    Dqn_Str8  match_to_end_of_buffer; // The substring containing the found string to the end of the buffer
    Dqn_Str8  start_to_before_match;  // The substring from the start of the buffer up until the found string, not including it
};

enum Dqn_Str8IsAll
{
    Dqn_Str8IsAll_Digits,
    Dqn_Str8IsAll_Hex,
};

enum Dqn_Str8EqCase
{
    Dqn_Str8EqCase_Sensitive,
    Dqn_Str8EqCase_Insensitive,
};

enum Dqn_Str8FindFlag
{
    Dqn_Str8FindFlag_Digit      = 1 << 0, // 0-9
    Dqn_Str8FindFlag_Whitespace = 1 << 1, // '\r', '\t', '\n', ' '
    Dqn_Str8FindFlag_Alphabet   = 1 << 2, // A-Z, a-z
    Dqn_Str8FindFlag_Plus       = 1 << 3, // +
    Dqn_Str8FindFlag_Minus      = 1 << 4, // -
    Dqn_Str8FindFlag_AlphaNum   = Dqn_Str8FindFlag_Alphabet | Dqn_Str8FindFlag_Digit,
};

enum Dqn_Str8SplitIncludeEmptyStrings
{
    Dqn_Str8SplitIncludeEmptyStrings_No,
    Dqn_Str8SplitIncludeEmptyStrings_Yes,
};

struct Dqn_Str8ToU64Result
{
    bool     success;
    uint64_t value;
};

struct Dqn_Str8ToI64Result
{
    bool    success;
    int64_t value;
};

// NOTE: [$FSTR] Dqn_FStr8 /////////////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_FSTR8)
template <Dqn_usize N> struct Dqn_FStr8
{
    char      data[N+1];
    Dqn_usize size;

    char       *begin()       { return data; }
    char       *end  ()       { return data + size; }
    char const *begin() const { return data; }
    char const *end  () const { return data + size; }
};
#endif // !defined(DQN_NO_FSTR8)

struct Dqn_Str8Builder
{
    Dqn_Arena    *arena;       // Allocator to use to back the string list
    Dqn_Str8Link *head;        // First string in the linked list of strings
    Dqn_Str8Link *tail;        // Last string in the linked list of strings
    Dqn_usize     string_size; // The size in bytes necessary to construct the current string
    Dqn_usize     count;       // The number of links in the linked list of strings
};

// NOTE: [$CSTR] Dqn_CStr8 /////////////////////////////////////////////////////////////////////////
template <Dqn_usize N> constexpr    Dqn_usize                 Dqn_CStr8_ArrayUCount           (char const (&literal)[N]) { (void)literal; return N - 1; }
template <Dqn_usize N> constexpr    Dqn_usize                 Dqn_CStr8_ArrayICount           (char const (&literal)[N]) { (void)literal; return N - 1; }
DQN_API                             Dqn_usize                 Dqn_CStr8_FSize                 (DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API                             Dqn_usize                 Dqn_CStr8_FVSize                (DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API                             Dqn_usize                 Dqn_CStr8_Size                  (char const *a);
DQN_API                             Dqn_usize                 Dqn_CStr16_Size                 (wchar_t const *a);

// NOTE: [$STR6] Dqn_Str16 /////////////////////////////////////////////////////////////////////////
#define                                                       DQN_STR16(string)               Dqn_Str16{(wchar_t *)(string), sizeof(string)/sizeof(string[0]) - 1}
#define                                                       Dqn_Str16_HasData(string)       ((string).data && (string).size)

#if defined(__cplusplus)
DQN_API                             bool                      operator==                      (Dqn_Str16 const &lhs, Dqn_Str16 const &rhs);
DQN_API                             bool                      operator!=                      (Dqn_Str16 const &lhs, Dqn_Str16 const &rhs);
#endif

// NOTE: [$STR8] Dqn_Str8 //////////////////////////////////////////////////////////////////////////
#define                                                       DQN_STR8(string)                 Dqn_Str8{(char *)(string), (sizeof(string) - 1)}
#define                                                       DQN_STR_FMT(string)              (int)((string).size), (string).data
#define                                                       Dqn_Str8_Init(data, size)        Dqn_Str8{(char *)(data), (size_t)(size)}

DQN_API                             Dqn_Str8                  Dqn_Str8_InitCStr8               (char const *src);
#define                                                       Dqn_Str8_HasData(string)         ((string).data && (string).size)
DQN_API                             bool                      Dqn_Str8_IsAll                   (Dqn_Str8 string, Dqn_Str8IsAll is_all);

DQN_API                             Dqn_Str8                  Dqn_Str8_InitF                   (Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, ...);
#define                                                       Dqn_Str8_InitF_TLS(...)          Dqn_Str8_InitF(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8_InitFV                  (Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, va_list args);
#define                                                       Dqn_Str8_InitFV_TLS(...)         Dqn_Str8_InitFV(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8_Alloc                   (Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem);
#define                                                       Dqn_Str8_Alloc_TLS(...)          Dqn_Str8_Alloc(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8_CopyCString             (Dqn_Arena *arena, char const *string, Dqn_usize size);
#define                                                       Dqn_Str8_CopyCString_TLS(...)    Dqn_Str8_CopyCString(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8_Copy                    (Dqn_Arena *arena, Dqn_Str8 string);
#define                                                       Dqn_Str8_Copy_TLS(...)            Dqn_Str8_Copy(Dqn_TLS_TopArena(), ##__VA_ARGS__)

DQN_API                             Dqn_Str8                  Dqn_Str8_Slice                   (Dqn_Str8 string, Dqn_usize offset, Dqn_usize size);
DQN_API                             Dqn_Str8                  Dqn_Str8_Advance                 (Dqn_Str8 string, Dqn_usize amount);
DQN_API                             Dqn_Str8                  Dqn_Str8_NextLine                (Dqn_Str8 string);
DQN_API                             Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitArray        (Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size);
DQN_API                             Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplit             (Dqn_Str8 string, Dqn_Str8 find);
DQN_API                             Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitLastArray    (Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size);
DQN_API                             Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitLast         (Dqn_Str8 string, Dqn_Str8 find);
DQN_API                             Dqn_usize                 Dqn_Str8_Split                   (Dqn_Str8 string, Dqn_Str8 delimiter, Dqn_Str8 *splits, Dqn_usize splits_count, Dqn_Str8SplitIncludeEmptyStrings mode);
DQN_API                             Dqn_Slice<Dqn_Str8>       Dqn_Str8_SplitAlloc              (Dqn_Arena *arena, Dqn_Str8 string, Dqn_Str8 delimiter, Dqn_Str8SplitIncludeEmptyStrings mode);
#define                                                       Dqn_Str8_SplitAlloc_TLS(...)     Dqn_Str8_SplitAlloc(Dqn_TLS_TopArena(), ##__VA_ARGS__)

DQN_API                             Dqn_Str8FindResult        Dqn_Str8_FindStr8Array           (Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size, Dqn_Str8EqCase eq_case);
DQN_API                             Dqn_Str8FindResult        Dqn_Str8_FindStr8                (Dqn_Str8 string, Dqn_Str8 find, Dqn_Str8EqCase eq_case);
DQN_API                             Dqn_Str8FindResult        Dqn_Str8_Find                    (Dqn_Str8 string, uint32_t flags);
DQN_API                             Dqn_Str8                  Dqn_Str8_Segment                 (Dqn_Arena *arena, Dqn_Str8 src, Dqn_usize segment_size, char segment_char);
#define                                                       Dqn_Str8_Segment_TLS(...)        Dqn_Str8_Segment(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8_ReverseSegment          (Dqn_Arena *arena, Dqn_Str8 src, Dqn_usize segment_size, char segment_char);
#define                                                       Dqn_Str8_ReverseSegment_TLS(...) Dqn_Str8_ReverseSegment(Dqn_TLS_TopArena(), ##__VA_ARGS__)

DQN_API                             bool                      Dqn_Str8_Eq                      (Dqn_Str8 lhs, Dqn_Str8 rhs, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             bool                      Dqn_Str8_EqInsensitive           (Dqn_Str8 lhs, Dqn_Str8 rhs);
DQN_API                             bool                      Dqn_Str8_StartsWith              (Dqn_Str8 string, Dqn_Str8 prefix, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             bool                      Dqn_Str8_StartsWithInsensitive   (Dqn_Str8 string, Dqn_Str8 prefix);
DQN_API                             bool                      Dqn_Str8_EndsWith                (Dqn_Str8 string, Dqn_Str8 prefix, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             bool                      Dqn_Str8_EndsWithInsensitive     (Dqn_Str8 string, Dqn_Str8 prefix);
DQN_API                             bool                      Dqn_Str8_HasChar                 (Dqn_Str8 string, char ch);

DQN_API                             Dqn_Str8                  Dqn_Str8_TrimPrefix              (Dqn_Str8 string, Dqn_Str8 prefix, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             Dqn_Str8                  Dqn_Str8_TrimHexPrefix           (Dqn_Str8 string);
DQN_API                             Dqn_Str8                  Dqn_Str8_TrimSuffix              (Dqn_Str8 string, Dqn_Str8 suffix, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             Dqn_Str8                  Dqn_Str8_TrimAround              (Dqn_Str8 string, Dqn_Str8 trim_string);
DQN_API                             Dqn_Str8                  Dqn_Str8_TrimWhitespaceAround    (Dqn_Str8 string);
DQN_API                             Dqn_Str8                  Dqn_Str8_TrimByteOrderMark       (Dqn_Str8 string);

DQN_API                             Dqn_Str8                  Dqn_Str8_FileNameFromPath        (Dqn_Str8 path);
DQN_API                             Dqn_Str8                  Dqn_Str8_FileNameNoExtension     (Dqn_Str8 path);
DQN_API                             Dqn_Str8                  Dqn_Str8_FilePathNoExtension     (Dqn_Str8 path);
DQN_API                             Dqn_Str8                  Dqn_Str8_FileExtension           (Dqn_Str8 path);

DQN_API                             Dqn_Str8ToU64Result       Dqn_Str8_ToU64                   (Dqn_Str8 string, char separator);
DQN_API                             Dqn_Str8ToI64Result       Dqn_Str8_ToI64                   (Dqn_Str8 string, char separator);

DQN_API                             Dqn_Str8                  Dqn_Str8_Replace                 (Dqn_Str8 string, Dqn_Str8 find, Dqn_Str8 replace, Dqn_usize start_index, Dqn_Arena *arena, Dqn_Str8EqCase eq_case = Dqn_Str8EqCase_Sensitive);
DQN_API                             Dqn_Str8                  Dqn_Str8_ReplaceInsensitive      (Dqn_Str8 string, Dqn_Str8 find, Dqn_Str8 replace, Dqn_usize start_index, Dqn_Arena *arena);
DQN_API                             void                      Dqn_Str8_Remove                  (Dqn_Str8 *string, Dqn_usize offset, Dqn_usize size);

#if defined(__cplusplus)
DQN_API                             bool                      operator==                       (Dqn_Str8 const &lhs, Dqn_Str8 const &rhs);
DQN_API                             bool                      operator!=                       (Dqn_Str8 const &lhs, Dqn_Str8 const &rhs);
#endif

// NOTE: [$STRB] Dqn_Str8Builder ///////////////////////////////////////////////////////////////////
DQN_API                             Dqn_Str8Builder           Dqn_Str8Builder_Init                    (Dqn_Arena *arena);
#define                                                       Dqn_Str8Builder_Init_TLS()              Dqn_Str8Builder_Init(Dqn_TLS_TopArena())
DQN_API                             Dqn_Str8Builder           Dqn_Str8Builder_InitArrayRef            (Dqn_Arena *arena, Dqn_Str8 const *strings, Dqn_usize size);
#define                                                       Dqn_Str8Builder_InitArrayRef_TLS(...)   Dqn_Str8Builder_InitArrayRef(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8Builder           Dqn_Str8Builder_InitArrayCopy           (Dqn_Arena *arena, Dqn_Str8 const *strings, Dqn_usize size);
#define                                                       Dqn_Str8Builder_InitArrayCopy_TLS(...)  Dqn_Str8Builder_InitArrayCopy(Dqn_TLS_TopArena(), ##__VA_ARGS__)
template <Dqn_usize N>              Dqn_Str8Builder           Dqn_Str8Builder_InitCArrayRef           (Dqn_Arena *arena, Dqn_Str8 const (&array)[N]);
#define                                                       Dqn_Str8Builder_InitCArrayRef_TLS(...)  Dqn_Str8Builder_InitCArrayRef(Dqn_TLS_TopArena(), ##__VA_ARGS__)
template <Dqn_usize N>              Dqn_Str8Builder           Dqn_Str8Builder_InitCArrayCopy          (Dqn_Arena *arena, Dqn_Str8 const (&array)[N]);
#define                                                       Dqn_Str8Builder_InitCArrayCopy_TLS(...) Dqn_Str8Builder_InitCArrayCopy(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             bool                      Dqn_Str8Builder_AddArrayRef             (Dqn_Str8Builder *builder, Dqn_Str8 const *strings, Dqn_usize size);
DQN_API                             bool                      Dqn_Str8Builder_AddArrayCopy            (Dqn_Str8Builder *builder, Dqn_Str8 const *strings, Dqn_usize size);
template <Dqn_usize N>              bool                      Dqn_Str8Builder_AddCArrayRef            (Dqn_Str8Builder *builder, Dqn_Str8 const (&array)[N]);
template <Dqn_usize N>              bool                      Dqn_Str8Builder_AddCArrayCopy           (Dqn_Str8Builder *builder, Dqn_Str8 const (&array)[N]);
DQN_API                             bool                      Dqn_Str8Builder_AddSliceRef             (Dqn_Str8Builder *builder, Dqn_Slice<Dqn_Str8> string);
DQN_API                             bool                      Dqn_Str8Builder_AddSliceCopy            (Dqn_Str8Builder *builder, Dqn_Slice<Dqn_Str8> string);
DQN_API                             bool                      Dqn_Str8Builder_AddRef                  (Dqn_Str8Builder *builder, Dqn_Str8 string);
DQN_API                             bool                      Dqn_Str8Builder_AddCopy                 (Dqn_Str8Builder *builder, Dqn_Str8 string);
DQN_API                             bool                      Dqn_Str8Builder_AddFV                   (Dqn_Str8Builder *builder, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API                             bool                      Dqn_Str8Builder_AddF                    (Dqn_Str8Builder *builder, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API                             bool                      Dqn_Str8Builder_AddBytesRef             (Dqn_Str8Builder *builder, void const *ptr, Dqn_usize size);
DQN_API                             bool                      Dqn_Str8Builder_AddBytesCopy            (Dqn_Str8Builder *builder, void const *ptr, Dqn_usize size);
DQN_API                             bool                      Dqn_Str8Builder_AddBuilderRef           (Dqn_Str8Builder *dest, Dqn_Str8Builder const *src);
DQN_API                             bool                      Dqn_Str8Builder_AddBuilderCopy          (Dqn_Str8Builder *dest, Dqn_Str8Builder const *src);
DQN_API                             bool                      Dqn_Str8Builder_Erase                   (Dqn_Str8Builder *builder, Dqn_Str8 string);
DQN_API                             Dqn_Str8Builder           Dqn_Str8Builder_Copy                    (Dqn_Arena *arena, Dqn_Str8Builder const *builder);
#define                                                       Dqn_Str8Builder_Copy_TLS(...)           Dqn_Str8Builder_Copy(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API                             Dqn_Str8                  Dqn_Str8Builder_Build                   (Dqn_Str8Builder const *builder, Dqn_Arena *arena);
#define                                                       Dqn_Str8Builder_Build_TLS(...)          Dqn_Str8Builder_Build(__VA_ARGS__, Dqn_TLS_TopArena())
DQN_API                             Dqn_Str8                  Dqn_Str8Builder_BuildDelimited          (Dqn_Str8Builder const *builder, Dqn_Str8 delimiter, Dqn_Arena *arena);
#define                                                       Dqn_Str8Builder_BuildDelimited_TLS(...) Dqn_Str8Builder_BuildDelimited(__VA_ARGS__, Dqn_TLS_TopArena())
DQN_API                             Dqn_Str8                  Dqn_Str8Builder_BuildCRT                (Dqn_Str8Builder const *builder);
DQN_API                             Dqn_Slice<Dqn_Str8>       Dqn_Str8Builder_BuildSlice              (Dqn_Str8Builder const *builder, Dqn_Arena *arena);
#define                                                       Dqn_Str8Builder_BuildSlice_TLS(...)     Dqn_Str8Builder_BuildSlice(__VA_ARGS__, Dqn_TLS_TopArena())
DQN_API                             void                      Dqn_Str8Builder_Print                   (Dqn_Str8Builder const *builder);
DQN_API                             void                      Dqn_Str8Builder_PrintLn                 (Dqn_Str8Builder const *builder);

// NOTE: [$FSTR] Dqn_FStr8 //////////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_FSTR8)
template <Dqn_usize N>              Dqn_FStr8<N>              Dqn_FStr8_InitF                 (DQN_FMT_ATTRIB char const *fmt, ...);
template <Dqn_usize N>              Dqn_usize                 Dqn_FStr8_Max                   (Dqn_FStr8<N> const *string);
template <Dqn_usize N>              void                      Dqn_FStr8_Clear                 (Dqn_FStr8<N> *string);
template <Dqn_usize N>              bool                      Dqn_FStr8_AddFV                 (Dqn_FStr8<N> *string, DQN_FMT_ATTRIB char const *fmt, va_list va);
template <Dqn_usize N>              bool                      Dqn_FStr8_AddF                  (Dqn_FStr8<N> *string, DQN_FMT_ATTRIB char const *fmt, ...);
template <Dqn_usize N>              bool                      Dqn_FStr8_AddCStr8              (Dqn_FStr8<N> *string, char const *value, Dqn_usize size);
template <Dqn_usize N>              bool                      Dqn_FStr8_Add                   (Dqn_FStr8<N> *string, Dqn_Str8 value);
template <Dqn_usize N>              Dqn_Str8                  Dqn_FStr8_ToStr8                (Dqn_FStr8<N> const *string);
template <Dqn_usize N>              bool                      Dqn_FStr8_Eq                    (Dqn_FStr8<N> const *lhs, Dqn_FStr8<N> const *rhs, Dqn_Str8EqCase eq_case);
template <Dqn_usize N>              bool                      Dqn_FStr8_EqStr8                (Dqn_FStr8<N> const *lhs, Dqn_Str8 rhs, Dqn_Str8EqCase eq_case);
template <Dqn_usize N>              bool                      Dqn_FStr8_EqInsensitive         (Dqn_FStr8<N> const *lhs, Dqn_FStr8<N> const *rhs);
template <Dqn_usize N>              bool                      Dqn_FStr8_EqStr8Insensitive     (Dqn_FStr8<N> const *lhs, Dqn_Str8 rhs);
template <Dqn_usize A, Dqn_usize B> bool                      Dqn_FStr8_EqFStr8               (Dqn_FStr8<A> const *lhs, Dqn_FStr8<B> const *rhs, Dqn_Str8EqCase eq_case);
template <Dqn_usize A, Dqn_usize B> bool                      Dqn_FStr8_EqFStr8Insensitive    (Dqn_FStr8<A> const *lhs, Dqn_FStr8<B> const *rhs);
template <Dqn_usize N>              bool                      operator==                      (Dqn_FStr8<N> const &lhs, Dqn_FStr8<N> const &rhs);
template <Dqn_usize N>              bool                      operator!=                      (Dqn_FStr8<N> const &lhs, Dqn_FStr8<N> const &rhs);
#endif // !defined(DQN_NO_FSTR8)

// NOTE: [$CHAR] Dqn_Char //////////////////////////////////////////////////////////////////////////
struct Dqn_CharHexToU8
{
    bool    success;
    uint8_t value;
};

DQN_API                             bool                      Dqn_Char_IsAlphabet            (char ch);
DQN_API                             bool                      Dqn_Char_IsDigit               (char ch);
DQN_API                             bool                      Dqn_Char_IsAlphaNum            (char ch);
DQN_API                             bool                      Dqn_Char_IsWhitespace          (char ch);
DQN_API                             bool                      Dqn_Char_IsHex                 (char ch);
DQN_API                             Dqn_CharHexToU8           Dqn_Char_HexToU8               (char ch);
DQN_API                             char                      Dqn_Char_ToHex                 (char ch);
DQN_API                             char                      Dqn_Char_ToHexUnchecked        (char ch);
DQN_API                             char                      Dqn_Char_ToLower               (char ch);

// NOTE: [$UTFX] Dqn_UTF ///////////////////////////////////////////////////////////////////////////
DQN_API                             int                       Dqn_UTF8_EncodeCodepoint       (uint8_t utf8[4], uint32_t codepoint);
DQN_API                             int                       Dqn_UTF16_EncodeCodepoint      (uint16_t utf16[2], uint32_t codepoint);

// NOTE: [$STRB] Dqn_Str8Builder ///////////////////////////////////////////////////////////////////
template <Dqn_usize N> Dqn_Str8Builder Dqn_Str8Builder_InitCArrayRef(Dqn_Arena *arena, Dqn_Str8 const (&array)[N])
{
    Dqn_Str8Builder result = Dqn_Str8Builder_InitArrayRef(arena, array, N);
    return result;
}

template <Dqn_usize N> Dqn_Str8Builder Dqn_Str8Builder_InitCArrayCopy(Dqn_Arena *arena, Dqn_Str8 const (&array)[N])
{
    Dqn_Str8Builder result = Dqn_Str8Builder_InitArrayCopy(arena, array, N);
    return result;
}

template <Dqn_usize N> bool Dqn_Str8Builder_AddCArrayRef(Dqn_Str8Builder *builder, Dqn_Str8 const (&array)[N])
{
    bool result = Dqn_Str8Builder_AddArrayRef(builder, array, N);
    return result;
}

template <Dqn_usize N> bool Dqn_Str8Builder_AddCArrayCopy(Dqn_Str8Builder *builder, Dqn_Str8 const (&array)[N])
{
    bool result = Dqn_Str8Builder_AddArrayCopy(builder, array, N);
    return result;
}

#if !defined(DQN_NO_FSTR8)
// NOTE: [$FSTR] Dqn_FStr8 /////////////////////////////////////////////////////////////////////////
template <Dqn_usize N> Dqn_FStr8<N> Dqn_FStr8_InitF(DQN_FMT_ATTRIB char const *fmt, ...)
{
    Dqn_FStr8<N> result = {};
    if (fmt) {
        va_list args;
        va_start(args, fmt);
        Dqn_FStr8_AddFV(&result, fmt, args);
        va_end(args);
    }
    return result;
}

template <Dqn_usize N> Dqn_usize Dqn_FStr8_Max(Dqn_FStr8<N> const *)
{
    Dqn_usize result = N;
    return result;
}

template <Dqn_usize N> void Dqn_FStr8_Clear(Dqn_FStr8<N> *string)
{
    *string = {};
}

template <Dqn_usize N> bool Dqn_FStr8_AddFV(Dqn_FStr8<N> *string, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    bool result = false;
    if (!string || !fmt)
        return result;

    Dqn_usize require = Dqn_CStr8_FVSize(fmt, args) + 1 /*null_terminate*/;
    Dqn_usize space   = (N + 1) - string->size;
    result            = require <= space;
    string->size     += DQN_VSNPRINTF(string->data + string->size, DQN_CAST(int)space, fmt, args);

    // NOTE: snprintf returns the required size of the format string
    // irrespective of if there's space or not.
    string->size = DQN_MIN(string->size, N);
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_AddF(Dqn_FStr8<N> *string, DQN_FMT_ATTRIB char const *fmt, ...)
{
    bool result = false;
    if (!string || !fmt)
        return result;
    va_list args;
    va_start(args, fmt);
    result = Dqn_FStr8_AddFV(string, fmt, args);
    va_end(args);
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_AddCStr8(Dqn_FStr8<N> *string, char const *src, Dqn_usize size)
{
    DQN_ASSERT(string->size <= N);
    bool result = false;
    if (!string || !src || size == 0 || string->size >= N)
        return result;

    Dqn_usize space = N - string->size;
    result          = size <= space;
    DQN_MEMCPY(string->data + string->size, src, DQN_MIN(space, size));
    string->size = DQN_MIN(string->size + size, N);
    string->data[string->size] = 0;
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_Add(Dqn_FStr8<N> *string, Dqn_Str8 src)
{
    bool result = Dqn_FStr8_AddCStr8(string, src.data, src.size);
    return result;
}

template <Dqn_usize N> Dqn_Str8 Dqn_FStr8_ToStr8(Dqn_FStr8<N> const *string)
{
    Dqn_Str8 result = {};
    if (!string || string->size <= 0)
        return result;

    result.data = DQN_CAST(char *)string->data;
    result.size = string->size;
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_Eq(Dqn_FStr8<N> const *lhs, Dqn_FStr8<N> const *rhs, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    Dqn_Str8 rhs_s8 = Dqn_FStr8_ToStr8(rhs);
    bool result = Dqn_Str8_Eq(lhs_s8, rhs_s8, eq_case);
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_EqStr8(Dqn_FStr8<N> const *lhs, Dqn_Str8 rhs, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    bool result = Dqn_Str8_Eq(lhs_s8, rhs, eq_case);
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_EqInsensitive(Dqn_FStr8<N> const *lhs, Dqn_FStr8<N> const *rhs)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    Dqn_Str8 rhs_s8 = Dqn_FStr8_ToStr8(rhs);
    bool result     = Dqn_Str8_Eq(lhs_s8, rhs_s8, Dqn_Str8EqCase_Insensitive);
    return result;
}

template <Dqn_usize N> bool Dqn_FStr8_EqStr8Insensitive(Dqn_FStr8<N> const *lhs, Dqn_Str8 rhs)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    bool result     = Dqn_Str8_Eq(lhs_s8, rhs, Dqn_Str8EqCase_Insensitive);
    return result;
}

template <Dqn_usize A, Dqn_usize B> bool Dqn_FStr8_EqFStr8(Dqn_FStr8<A> const *lhs, Dqn_FStr8<B> const *rhs, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    Dqn_Str8 rhs_s8 = Dqn_FStr8_ToStr8(rhs);
    bool result     = Dqn_Str8_Eq(lhs_s8, rhs_s8, eq_case);
    return result;
}

template <Dqn_usize A, Dqn_usize B> bool Dqn_FStr8_EqFStr8Insensitive(Dqn_FStr8<A> const *lhs, Dqn_FStr8<B> const *rhs)
{
    Dqn_Str8 lhs_s8 = Dqn_FStr8_ToStr8(lhs);
    Dqn_Str8 rhs_s8 = Dqn_FStr8_ToStr8(rhs);
    bool result = Dqn_Str8_Eq(lhs_s8, rhs_s8, Dqn_Str8EqCase_Insensitive);
    return result;
}

template <Dqn_usize N> bool operator==(Dqn_FStr8<N> const &lhs, Dqn_FStr8<N> const &rhs)
{
    bool result = Dqn_FStr8_Eq(&lhs, &rhs, Dqn_Str8EqCase_Sensitive);
    return result;
}

template <Dqn_usize N> bool operator!=(Dqn_FStr8<N> const &lhs, Dqn_FStr8<N> const &rhs)
{
    bool result = !(lhs == rhs);
    return result;
}
#endif // !defined(DQN_NO_FSTR8)
