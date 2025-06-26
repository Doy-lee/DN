#if !defined(DN_BASE_STRING_H)
#define DN_BASE_STRING_H

#if !defined(DN_STB_SPRINTF_HEADER_ONLY)
  #define STB_SPRINTF_IMPLEMENTATION
  #define STB_SPRINTF_STATIC
#endif
DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4505) // Unused function warning
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused-function)
#include "../External/stb_sprintf.h"
DN_GCC_WARNING_POP
DN_MSVC_WARNING_POP

#define DN_SPrintF(...) STB_SPRINTF_DECORATE(sprintf)(__VA_ARGS__)
#define DN_SNPrintF(...) STB_SPRINTF_DECORATE(snprintf)(__VA_ARGS__)
#define DN_VSPrintF(...) STB_SPRINTF_DECORATE(vsprintf)(__VA_ARGS__)
#define DN_VSNPrintF(...) STB_SPRINTF_DECORATE(vsnprintf)(__VA_ARGS__)

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
//   dn_base_string.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_Str8 //////////////////////////////////////////////////////////////////////////////////
struct DN_Str8Link
{
  DN_Str8      string; // The string
  DN_Str8Link *next;   // The next string in the linked list
  DN_Str8Link *prev;   // The prev string in the linked list
};

struct DN_Str8BinarySplitResult
{
  DN_Str8 lhs;
  DN_Str8 rhs;
};

struct DN_Str8FindResult
{
  bool     found;                        // True if string was found. If false, the subsequent fields below are not set.
  DN_USize index;                        // Index in the buffer where the found string starts
  DN_Str8  match;                        // Matching string in the buffer that was searched
  DN_Str8  match_to_end_of_buffer;       // Substring containing the found string to the end of the buffer
  DN_Str8  after_match_to_end_of_buffer; // Substring starting after the found string to the end of the buffer
  DN_Str8  start_to_before_match;        // Substring from the start of the buffer up until the found string, not including it
};

enum DN_Str8IsAll
{
  DN_Str8IsAll_Digits,
  DN_Str8IsAll_Hex,
};

enum DN_Str8EqCase
{
  DN_Str8EqCase_Sensitive,
  DN_Str8EqCase_Insensitive,
};

enum DN_Str8FindFlag
{
  DN_Str8FindFlag_Digit      = 1 << 0, // 0-9
  DN_Str8FindFlag_Whitespace = 1 << 1, // '\r', '\t', '\n', ' '
  DN_Str8FindFlag_Alphabet   = 1 << 2, // A-Z, a-z
  DN_Str8FindFlag_Plus       = 1 << 3, // +
  DN_Str8FindFlag_Minus      = 1 << 4, // -
  DN_Str8FindFlag_AlphaNum   = DN_Str8FindFlag_Alphabet | DN_Str8FindFlag_Digit,
};

enum DN_Str8SplitIncludeEmptyStrings
{
  DN_Str8SplitIncludeEmptyStrings_No,
  DN_Str8SplitIncludeEmptyStrings_Yes,
};

struct DN_Str8ToU64Result
{
  bool     success;
  uint64_t value;
};

struct DN_Str8ToI64Result
{
  bool    success;
  int64_t value;
};

struct DN_Str8DotTruncateResult
{
  bool    truncated;
  DN_Str8 str8;
};

// NOTE: DN_FStr8 //////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_FSTR8)
template <DN_USize N>
struct DN_FStr8
{
  char     data[N + 1];
  DN_USize size;

  char *begin() { return data; }
  char *end() { return data + size; }
  char const *begin() const { return data; }
  char const *end() const { return data + size; }
};
#endif // !defined(DN_NO_FSTR8)

struct DN_Str8Builder
{
  DN_Arena    *arena;       // Allocator to use to back the string list
  DN_Str8Link *head;        // First string in the linked list of strings
  DN_Str8Link *tail;        // Last string in the linked list of strings
  DN_USize     string_size; // The size in bytes necessary to construct the current string
  DN_USize     count;       // The number of links in the linked list of strings
};

enum DN_Str8BuilderAdd
{
  DN_Str8BuilderAdd_Append,
  DN_Str8BuilderAdd_Prepend,
};

// NOTE: DN_CStr8 //////////////////////////////////////////////////////////////////////////////////
template <DN_USize N> constexpr    DN_USize                 DN_CStr8_ArrayUCount           (char const (&literal)[N]) { (void)literal; return N - 1; }
template <DN_USize N> constexpr    DN_USize                 DN_CStr8_ArrayICount           (char const (&literal)[N]) { (void)literal; return N - 1; }
DN_API                             DN_USize                 DN_CStr8_FSize                 (DN_FMT_ATTRIB char const *fmt, ...);
DN_API                             DN_USize                 DN_CStr8_FVSize                (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API                             DN_USize                 DN_CStr8_Size                  (char const *a);
DN_API                             DN_USize                 DN_CStr16_Size                 (wchar_t const *a);

// NOTE: DN_Str16 //////////////////////////////////////////////////////////////////////////////////
#define                                                     DN_STR16(string)               DN_Str16{(wchar_t *)(string), sizeof(string)/sizeof(string[0]) - 1}
#define                                                     DN_Str16_HasData(string)       ((string).data && (string).size)

#if defined(__cplusplus)
DN_API                             bool                     operator==                      (DN_Str16 const &lhs, DN_Str16 const &rhs);
DN_API                             bool                     operator!=                      (DN_Str16 const &lhs, DN_Str16 const &rhs);
#endif

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////
#define                                                     DN_STR8(string)                 DN_Str8{(char *)(string), (sizeof(string) - 1)}
#define                                                     DN_STR_FMT(string)              (int)((string).size), (string).data
#define                                                     DN_Str8_Init(data, size)        DN_Str8{(char *)(data), (size_t)(size)}

DN_API                             DN_Str8                  DN_Str8_InitCStr8               (char const *src);
#define                                                     DN_Str8_HasData(string)         ((string).data && (string).size)
DN_API                             bool                     DN_Str8_IsAll                   (DN_Str8 string, DN_Str8IsAll is_all);

DN_API                             DN_Str8                  DN_Str8_InitF                   (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API                             DN_Str8                  DN_Str8_InitFV                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API                             DN_Str8                  DN_Str8_Alloc                   (DN_Arena *arena, DN_USize size, DN_ZeroMem zero_mem);
DN_API                             DN_Str8                  DN_Str8_Copy                    (DN_Arena *arena, DN_Str8 string);

DN_API                             char *                   DN_Str8_End                     (DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_Slice                   (DN_Str8 string, DN_USize offset, DN_USize size);
DN_API                             DN_Str8                  DN_Str8_Advance                 (DN_Str8 string, DN_USize amount);
DN_API                             DN_Str8                  DN_Str8_NextLine                (DN_Str8 string);
DN_API                             DN_Str8BinarySplitResult DN_Str8_BinarySplitArray        (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API                             DN_Str8BinarySplitResult DN_Str8_BinarySplit             (DN_Str8 string, DN_Str8 find);
DN_API                             DN_Str8BinarySplitResult DN_Str8_BinarySplitLastArray    (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API                             DN_Str8BinarySplitResult DN_Str8_BinarySplitLast         (DN_Str8 string, DN_Str8 find);
DN_API                             DN_USize                 DN_Str8_Split                   (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode);
DN_API                             DN_Slice<DN_Str8>        DN_Str8_SplitAlloc              (DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);

DN_API                             DN_Str8FindResult        DN_Str8_FindStr8Array           (DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case);
DN_API                             DN_Str8FindResult        DN_Str8_FindStr8                (DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case);
DN_API                             DN_Str8FindResult        DN_Str8_Find                    (DN_Str8 string, uint32_t flags);
DN_API                             DN_Str8                  DN_Str8_Segment                 (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API                             DN_Str8                  DN_Str8_ReverseSegment          (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);

DN_API                             bool                     DN_Str8_Eq                      (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API                             bool                     DN_Str8_EqInsensitive           (DN_Str8 lhs, DN_Str8 rhs);
DN_API                             bool                     DN_Str8_StartsWith              (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API                             bool                     DN_Str8_StartsWithInsensitive   (DN_Str8 string, DN_Str8 prefix);
DN_API                             bool                     DN_Str8_EndsWith                (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API                             bool                     DN_Str8_EndsWithInsensitive     (DN_Str8 string, DN_Str8 prefix);
DN_API                             bool                     DN_Str8_HasChar                 (DN_Str8 string, char ch);

DN_API                             DN_Str8                  DN_Str8_TrimPrefix              (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API                             DN_Str8                  DN_Str8_TrimHexPrefix           (DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_TrimSuffix              (DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API                             DN_Str8                  DN_Str8_TrimAround              (DN_Str8 string, DN_Str8 trim_string);
DN_API                             DN_Str8                  DN_Str8_TrimHeadWhitespace      (DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_TrimTailWhitespace      (DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_TrimWhitespaceAround    (DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_TrimByteOrderMark       (DN_Str8 string);

DN_API                             DN_Str8                  DN_Str8_FileNameFromPath        (DN_Str8 path);
DN_API                             DN_Str8                  DN_Str8_FileNameNoExtension     (DN_Str8 path);
DN_API                             DN_Str8                  DN_Str8_FilePathNoExtension     (DN_Str8 path);
DN_API                             DN_Str8                  DN_Str8_FileExtension           (DN_Str8 path);
DN_API                             DN_Str8                  DN_Str8_FileDirectoryFromPath   (DN_Str8 path);

DN_API                             DN_Str8ToU64Result       DN_Str8_ToU64                   (DN_Str8 string, char separator);
DN_API                             DN_Str8ToI64Result       DN_Str8_ToI64                   (DN_Str8 string, char separator);

DN_API                             DN_Str8                  DN_Str8_AppendF                 (DN_Arena *arena, DN_Str8 string, char const *fmt, ...);
DN_API                             DN_Str8                  DN_Str8_AppendFV                (DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args);

DN_API                             DN_Str8                  DN_Str8_FillF                   (DN_Arena *arena, DN_USize count, char const *fmt, ...);
DN_API                             DN_Str8                  DN_Str8_FillFV                  (DN_Arena *arena, DN_USize count, char const *fmt, va_list args);

DN_API                             void                     DN_Str8_Remove                  (DN_Str8 *string, DN_USize offset, DN_USize size);

DN_API                             DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddle       (DN_Arena *arena, DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);

DN_API                             DN_Str8                  DN_Str8_Lower                   (DN_Arena *arena, DN_Str8 string);
DN_API                             DN_Str8                  DN_Str8_Upper                   (DN_Arena *arena, DN_Str8 string);

#if defined(__cplusplus)
DN_API                             bool                     operator==                      (DN_Str8 const &lhs, DN_Str8 const &rhs);
DN_API                             bool                     operator!=                      (DN_Str8 const &lhs, DN_Str8 const &rhs);
#endif

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////
DN_API                             DN_Str8Builder           DN_Str8Builder_Init                    (DN_Arena *arena);
DN_API                             DN_Str8Builder           DN_Str8Builder_InitArrayRef            (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API                             DN_Str8Builder           DN_Str8Builder_InitArrayCopy           (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
template <DN_USize N>              DN_Str8Builder           DN_Str8Builder_InitCArrayRef           (DN_Arena *arena, DN_Str8 const (&array)[N]);
template <DN_USize N>              DN_Str8Builder           DN_Str8Builder_InitCArrayCopy          (DN_Arena *arena, DN_Str8 const (&array)[N]);

DN_API                             bool                     DN_Str8Builder_AddArrayRef             (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API                             bool                     DN_Str8Builder_AddArrayCopy            (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API                             bool                     DN_Str8Builder_AddFV                   (DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args);

#define                                                     DN_Str8Builder_AppendArrayRef(builder, strings, size)  DN_Str8Builder_AddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                                                     DN_Str8Builder_AppendArrayCopy(builder, strings, size) DN_Str8Builder_AddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                                                     DN_Str8Builder_AppendSliceRef(builder, slice)          DN_Str8Builder_AddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
#define                                                     DN_Str8Builder_AppendSliceCopy(builder, slice)         DN_Str8Builder_AddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
DN_API                             bool                     DN_Str8Builder_AppendRef               (DN_Str8Builder *builder, DN_Str8 string);
DN_API                             bool                     DN_Str8Builder_AppendCopy              (DN_Str8Builder *builder, DN_Str8 string);
#define                                                     DN_Str8Builder_AppendFV(builder, fmt, args)            DN_Str8Builder_AddFV(builder, DN_Str8BuilderAdd_Append, fmt, args)
DN_API                             bool                     DN_Str8Builder_AppendF                 (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API                             bool                     DN_Str8Builder_AppendBytesRef          (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API                             bool                     DN_Str8Builder_AppendBytesCopy         (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API                             bool                     DN_Str8Builder_AppendBuilderRef        (DN_Str8Builder *dest, DN_Str8Builder const *src);
DN_API                             bool                     DN_Str8Builder_AppendBuilderCopy       (DN_Str8Builder *dest, DN_Str8Builder const *src);

#define                                                     DN_Str8Builder_PrependArrayRef(builder, strings, size)  DN_Str8Builder_AddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                                                     DN_Str8Builder_PrependArrayCopy(builder, strings, size) DN_Str8Builder_AddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                                                     DN_Str8Builder_PrependSliceRef(builder, slice)          DN_Str8Builder_AddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
#define                                                     DN_Str8Builder_PrependSliceCopy(builder, slice)         DN_Str8Builder_AddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
DN_API                             bool                     DN_Str8Builder_PrependRef              (DN_Str8Builder *builder, DN_Str8 string);
DN_API                             bool                     DN_Str8Builder_PrependCopy             (DN_Str8Builder *builder, DN_Str8 string);
#define                                                     DN_Str8Builder_PrependFV(builder, fmt, args)            DN_Str8Builder_AddFV(builder, DN_Str8BuilderAdd_Prepend, fmt, args)
DN_API                             bool                     DN_Str8Builder_PrependF                (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);

DN_API                             bool                     DN_Str8Builder_Erase                   (DN_Str8Builder *builder, DN_Str8 string);
DN_API                             DN_Str8Builder           DN_Str8Builder_Copy                    (DN_Arena *arena, DN_Str8Builder const *builder);
DN_API                             DN_Str8                  DN_Str8Builder_Build                   (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API                             DN_Str8                  DN_Str8Builder_BuildDelimited          (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena);
DN_API                             DN_Slice<DN_Str8>        DN_Str8Builder_BuildSlice              (DN_Str8Builder const *builder, DN_Arena *arena);

// NOTE: DN_FStr8 //////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_FSTR8)
template <DN_USize N>              DN_FStr8<N>              DN_FStr8_InitF                 (DN_FMT_ATTRIB char const *fmt, ...);
template <DN_USize N>              DN_FStr8<N>              DN_FStr8_InitFV                (char const *fmt, va_list args);
template <DN_USize N>              DN_USize                 DN_FStr8_Max                   (DN_FStr8<N> const *string);
template <DN_USize N>              void                     DN_FStr8_Clear                 (DN_FStr8<N> *string);
template <DN_USize N>              bool                     DN_FStr8_AddFV                 (DN_FStr8<N> *string, DN_FMT_ATTRIB char const *fmt, va_list va);
template <DN_USize N>              bool                     DN_FStr8_AddF                  (DN_FStr8<N> *string, DN_FMT_ATTRIB char const *fmt, ...);
template <DN_USize N>              bool                     DN_FStr8_AddCStr8              (DN_FStr8<N> *string, char const *value, DN_USize size);
template <DN_USize N>              bool                     DN_FStr8_Add                   (DN_FStr8<N> *string, DN_Str8 value);
template <DN_USize N>              DN_Str8                  DN_FStr8_ToStr8                (DN_FStr8<N> const *string);
template <DN_USize N>              bool                     DN_FStr8_Eq                    (DN_FStr8<N> const *lhs, DN_FStr8<N> const *rhs, DN_Str8EqCase eq_case);
template <DN_USize N>              bool                     DN_FStr8_EqStr8                (DN_FStr8<N> const *lhs, DN_Str8 rhs, DN_Str8EqCase eq_case);
template <DN_USize N>              bool                     DN_FStr8_EqInsensitive         (DN_FStr8<N> const *lhs, DN_FStr8<N> const *rhs);
template <DN_USize N>              bool                     DN_FStr8_EqStr8Insensitive     (DN_FStr8<N> const *lhs, DN_Str8 rhs);
template <DN_USize A, DN_USize B>  bool                     DN_FStr8_EqFStr8               (DN_FStr8<A> const *lhs, DN_FStr8<B> const *rhs, DN_Str8EqCase eq_case);
template <DN_USize A, DN_USize B>  bool                     DN_FStr8_EqFStr8Insensitive    (DN_FStr8<A> const *lhs, DN_FStr8<B> const *rhs);
template <DN_USize N>              bool                     operator==                     (DN_FStr8<N> const &lhs, DN_FStr8<N> const &rhs);
template <DN_USize N>              bool                     operator!=                     (DN_FStr8<N> const &lhs, DN_FStr8<N> const &rhs);
template <DN_USize N>              bool                     operator==                     (DN_FStr8<N> const &lhs, DN_Str8 const &rhs);
template <DN_USize N>              bool                     operator!=                     (DN_FStr8<N> const &lhs, DN_Str8 const &rhs);
#endif // !defined(DN_NO_FSTR8)

// NOTE: DN_Char ///////////////////////////////////////////////////////////////////////////////////
struct DN_CharHexToU8
{
    bool    success;
    uint8_t value;
};

DN_API                             bool                     DN_Char_IsAlphabet            (char ch);
DN_API                             bool                     DN_Char_IsDigit               (char ch);
DN_API                             bool                     DN_Char_IsAlphaNum            (char ch);
DN_API                             bool                     DN_Char_IsWhitespace          (char ch);
DN_API                             bool                     DN_Char_IsHex                 (char ch);
DN_API                             DN_CharHexToU8           DN_Char_HexToU8               (char ch);
DN_API                             char                     DN_Char_ToHex                 (char ch);
DN_API                             char                     DN_Char_ToHexUnchecked        (char ch);
DN_API                             char                     DN_Char_ToLower               (char ch);
DN_API                             char                     DN_Char_ToUpper               (char ch);

// NOTE: DN_UTF ////////////////////////////////////////////////////////////////////////////////////
DN_API                             int                      DN_UTF8_EncodeCodepoint       (uint8_t utf8[4], uint32_t codepoint);
DN_API                             int                      DN_UTF16_EncodeCodepoint      (uint16_t utf16[2], uint32_t codepoint);

// NOTE: DN_Str8Builder ///////////////////////////////////////////////////////////////////////////
template <DN_USize N>
DN_Str8Builder DN_Str8Builder_InitCArrayRef(DN_Arena *arena, DN_Str8 const (&array)[N])
{
  DN_Str8Builder result = DN_Str8Builder_InitArrayRef(arena, array, N);
  return result;
}

template <DN_USize N>
DN_Str8Builder DN_Str8Builder_InitCArrayCopy(DN_Arena *arena, DN_Str8 const (&array)[N])
{
  DN_Str8Builder result = DN_Str8Builder_InitArrayCopy(arena, array, N);
  return result;
}

template <DN_USize N>
bool DN_Str8Builder_AddCArrayRef(DN_Str8Builder *builder, DN_Str8 const (&array)[N], DN_Str8BuilderAdd add)
{
  bool result = DN_Str8Builder_AddArrayRef(builder, array, N, add);
  return result;
}

template <DN_USize N>
bool DN_Str8Builder_AddCArrayCopy(DN_Str8Builder *builder, DN_Str8 const (&array)[N], DN_Str8BuilderAdd add)
{
  bool result = DN_Str8Builder_AddArrayCopy(builder, array, N, add);
  return result;
}

#if !defined(DN_NO_FSTR8)
// NOTE: DN_FStr8 //////////////////////////////////////////////////////////////////////////////////
template <DN_USize N>
DN_FStr8<N> DN_FStr8_InitF(DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_FStr8<N> result = {};
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    DN_FStr8_AddFV(&result, fmt, args);
    va_end(args);
  }
  return result;
}

template <DN_USize N>
DN_FStr8<N> DN_FStr8_InitFV(char const *fmt, va_list args)
{
  DN_FStr8<N> result = {};
  DN_FStr8_AddFV(&result, fmt, args);
  return result;
}

template <DN_USize N>
DN_USize DN_FStr8_Max(DN_FStr8<N> const *)
{
  DN_USize result = N;
  return result;
}

template <DN_USize N>
void DN_FStr8_Clear(DN_FStr8<N> *string)
{
  *string = {};
}

template <DN_USize N>
bool DN_FStr8_AddFV(DN_FStr8<N> *string, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  bool result = false;
  if (!string || !fmt)
    return result;

  DN_USize require = DN_CStr8_FVSize(fmt, args) + 1 /*null_terminate*/;
  DN_USize space   = (N + 1) - string->size;
  result           = require <= space;
  string->size += DN_VSNPrintF(string->data + string->size, DN_CAST(int) space, fmt, args);

  // NOTE: snprintf returns the required size of the format string
  // irrespective of if there's space or not.
  string->size = DN_Min(string->size, N);
  return result;
}

template <DN_USize N>
bool DN_FStr8_AddF(DN_FStr8<N> *string, DN_FMT_ATTRIB char const *fmt, ...)
{
  bool result = false;
  if (!string || !fmt)
    return result;
  va_list args;
  va_start(args, fmt);
  result = DN_FStr8_AddFV(string, fmt, args);
  va_end(args);
  return result;
}

template <DN_USize N>
bool DN_FStr8_AddCStr8(DN_FStr8<N> *string, char const *src, DN_USize size)
{
  DN_Assert(string->size <= N);
  bool result = false;
  if (!string || !src || size == 0 || string->size >= N)
    return result;

  DN_USize space = N - string->size;
  result         = size <= space;
  DN_Memcpy(string->data + string->size, src, DN_Min(space, size));
  string->size               = DN_Min(string->size + size, N);
  string->data[string->size] = 0;
  return result;
}

template <DN_USize N>
bool DN_FStr8_Add(DN_FStr8<N> *string, DN_Str8 src)
{
  bool result = DN_FStr8_AddCStr8(string, src.data, src.size);
  return result;
}

template <DN_USize N>
DN_Str8 DN_FStr8_ToStr8(DN_FStr8<N> const *string)
{
  DN_Str8 result = {};
  if (!string || string->size <= 0)
    return result;

  result.data = DN_CAST(char *) string->data;
  result.size = string->size;
  return result;
}

template <DN_USize N>
bool DN_FStr8_Eq(DN_FStr8<N> const *lhs, DN_FStr8<N> const *rhs, DN_Str8EqCase eq_case)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  DN_Str8 rhs_s8 = DN_FStr8_ToStr8(rhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs_s8, eq_case);
  return result;
}

template <DN_USize N>
bool DN_FStr8_EqStr8(DN_FStr8<N> const *lhs, DN_Str8 rhs, DN_Str8EqCase eq_case)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs, eq_case);
  return result;
}

template <DN_USize N>
bool DN_FStr8_EqInsensitive(DN_FStr8<N> const *lhs, DN_FStr8<N> const *rhs)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  DN_Str8 rhs_s8 = DN_FStr8_ToStr8(rhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs_s8, DN_Str8EqCase_Insensitive);
  return result;
}

template <DN_USize N>
bool DN_FStr8_EqStr8Insensitive(DN_FStr8<N> const *lhs, DN_Str8 rhs)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs, DN_Str8EqCase_Insensitive);
  return result;
}

template <DN_USize A, DN_USize B>
bool DN_FStr8_EqFStr8(DN_FStr8<A> const *lhs, DN_FStr8<B> const *rhs, DN_Str8EqCase eq_case)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  DN_Str8 rhs_s8 = DN_FStr8_ToStr8(rhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs_s8, eq_case);
  return result;
}

template <DN_USize A, DN_USize B>
bool DN_FStr8_EqFStr8Insensitive(DN_FStr8<A> const *lhs, DN_FStr8<B> const *rhs)
{
  DN_Str8 lhs_s8 = DN_FStr8_ToStr8(lhs);
  DN_Str8 rhs_s8 = DN_FStr8_ToStr8(rhs);
  bool    result = DN_Str8_Eq(lhs_s8, rhs_s8, DN_Str8EqCase_Insensitive);
  return result;
}

template <DN_USize N>
bool operator==(DN_FStr8<N> const &lhs, DN_FStr8<N> const &rhs)
{
  bool result = DN_FStr8_Eq(&lhs, &rhs, DN_Str8EqCase_Sensitive);
  return result;
}

template <DN_USize N>
bool operator!=(DN_FStr8<N> const &lhs, DN_FStr8<N> const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

template <DN_USize N>
bool operator==(DN_FStr8<N> const &lhs, DN_Str8 const &rhs)
{
  bool result = DN_Str8_Eq(DN_FStr8_ToStr8(&lhs), rhs, DN_Str8EqCase_Insensitive);
  return result;
}

template <DN_USize N>
bool operator!=(DN_FStr8<N> const &lhs, DN_Str8 const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}
#endif // !defined(DN_NO_FSTR8)
#endif   // !defined(DN_BASE_STRING_H)
