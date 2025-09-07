#if !defined(DN_BASE_STRING_H)
#define DN_BASE_STRING_H

#include "../dn_base_inc.h"

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

struct DN_Str8Link
{
  DN_Str8      string; // The string
  DN_Str8Link *next;   // The next string in the linked list
  DN_Str8Link *prev;   // The prev string in the linked list
};

struct DN_Str8BSplitResult
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

struct DN_Str8x64
{
  char     data[64];
  DN_USize size;
};

struct DN_Str8x256
{
  char     data[256];
  DN_USize size;
};

DN_API  DN_USize                 DN_CStr8_FSize                  (DN_FMT_ATTRIB char const *fmt, ...);
DN_API  DN_USize                 DN_CStr8_FVSize                 (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API  DN_USize                 DN_CStr8_Size                   (char const *a);
DN_API  DN_USize                 DN_CStr16_Size                  (wchar_t const *a);

#define                          DN_STR16(string)                DN_Str16{(wchar_t *)(string), sizeof(string)/sizeof(string[0]) - 1}
#define                          DN_Str16_HasData(string)        ((string).data && (string).size)

#if defined(__cplusplus)
DN_API  bool                     operator==                      (DN_Str16 const &lhs, DN_Str16 const &rhs);
DN_API  bool                     operator!=                      (DN_Str16 const &lhs, DN_Str16 const &rhs);
#endif

#define                          DN_STR8(string)                 DN_Str8{(char *)(string), (sizeof(string) - 1)}
#define                          DN_STR_FMT(string)              (int)((string).size), (string).data
#define                          DN_Str8_Init(data, size)        DN_Str8{(char *)(data), (size_t)(size)}
#define                          DN_Str8_HasData(string)         ((string).data && (string).size)
DN_API  DN_Str8                  DN_Str8_Alloc                   (DN_Arena *arena, DN_USize size, DN_ZeroMem zero_mem);
DN_API  DN_Str8                  DN_Str8_FromCStr8               (char const *src);
DN_API  DN_Str8                  DN_Str8_FromF                   (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  DN_Str8                  DN_Str8_FromFV                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API  DN_Str8                  DN_Str8_FromFPool               (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  DN_Str8                  DN_Str8_FromStr8                (DN_Arena *arena, DN_Str8 string);
DN_API  bool                     DN_Str8_IsAll                   (DN_Str8 string, DN_Str8IsAll is_all);
DN_API  char *                   DN_Str8_End                     (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_Slice                   (DN_Str8 string, DN_USize offset, DN_USize size);
DN_API  DN_Str8                  DN_Str8_Advance                 (DN_Str8 string, DN_USize amount);
DN_API  DN_Str8                  DN_Str8_NextLine                (DN_Str8 string);
DN_API  DN_Str8BSplitResult      DN_Str8_BSplitArray             (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API  DN_Str8BSplitResult      DN_Str8_BSplit                  (DN_Str8 string, DN_Str8 find);
DN_API  DN_Str8BSplitResult      DN_Str8_BSplitLastArray         (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API  DN_Str8BSplitResult      DN_Str8_BSplitLast              (DN_Str8 string, DN_Str8 find);
DN_API  DN_USize                 DN_Str8_Split                   (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode);
DN_API  DN_Slice<DN_Str8>        DN_Str8_SplitAlloc              (DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);
DN_API  DN_Str8FindResult        DN_Str8_FindStr8Array           (DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case);
DN_API  DN_Str8FindResult        DN_Str8_FindStr8                (DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case);
DN_API  DN_Str8FindResult        DN_Str8_Find                    (DN_Str8 string, uint32_t flags);
DN_API  DN_Str8                  DN_Str8_Segment                 (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API  DN_Str8                  DN_Str8_ReverseSegment          (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API  bool                     DN_Str8_Eq                      (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API  bool                     DN_Str8_EqInsensitive           (DN_Str8 lhs, DN_Str8 rhs);
DN_API  bool                     DN_Str8_StartsWith              (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API  bool                     DN_Str8_StartsWithInsensitive   (DN_Str8 string, DN_Str8 prefix);
DN_API  bool                     DN_Str8_EndsWith                (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API  bool                     DN_Str8_EndsWithInsensitive     (DN_Str8 string, DN_Str8 prefix);
DN_API  bool                     DN_Str8_HasChar                 (DN_Str8 string, char ch);
DN_API  DN_Str8                  DN_Str8_TrimPrefix              (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API  DN_Str8                  DN_Str8_TrimHexPrefix           (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_TrimSuffix              (DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API  DN_Str8                  DN_Str8_TrimAround              (DN_Str8 string, DN_Str8 trim_string);
DN_API  DN_Str8                  DN_Str8_TrimHeadWhitespace      (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_TrimTailWhitespace      (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_TrimWhitespaceAround    (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_TrimByteOrderMark       (DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_FileNameFromPath        (DN_Str8 path);
DN_API  DN_Str8                  DN_Str8_FileNameNoExtension     (DN_Str8 path);
DN_API  DN_Str8                  DN_Str8_FilePathNoExtension     (DN_Str8 path);
DN_API  DN_Str8                  DN_Str8_FileExtension           (DN_Str8 path);
DN_API  DN_Str8                  DN_Str8_FileDirectoryFromPath   (DN_Str8 path);
DN_API  DN_Str8ToU64Result       DN_Str8_ToU64                   (DN_Str8 string, char separator);
DN_API  DN_Str8ToI64Result       DN_Str8_ToI64                   (DN_Str8 string, char separator);
DN_API  DN_Str8                  DN_Str8_AppendF                 (DN_Arena *arena, DN_Str8 string, char const *fmt, ...);
DN_API  DN_Str8                  DN_Str8_AppendFV                (DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args);
DN_API  DN_Str8                  DN_Str8_FillF                   (DN_Arena *arena, DN_USize count, char const *fmt, ...);
DN_API  DN_Str8                  DN_Str8_FillFV                  (DN_Arena *arena, DN_USize count, char const *fmt, va_list args);
DN_API  void                     DN_Str8_Remove                  (DN_Str8 *string, DN_USize offset, DN_USize size);
DN_API  DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddle       (DN_Arena *arena, DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);
DN_API  DN_Str8                  DN_Str8_Lower                   (DN_Arena *arena, DN_Str8 string);
DN_API  DN_Str8                  DN_Str8_Upper                   (DN_Arena *arena, DN_Str8 string);
#if defined(__cplusplus)
DN_API  bool                     operator==                      (DN_Str8 const &lhs, DN_Str8 const &rhs);
DN_API  bool                     operator!=                      (DN_Str8 const &lhs, DN_Str8 const &rhs);
#endif

DN_API DN_Str8                   DN_LStr8_AppendF                       (char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, ...);
#define                          DN_IStr8_AppendF(struct_ptr, fmt, ...) DN_LStr8_AppendF((struct_ptr)->data, &(struct_ptr)->size, DN_ArrayCountU((struct_ptr)->data), fmt, ##__VA_ARGS__)
#define                          DN_Str8_FromIStr8(struct_ptr)          DN_Str8_Init((struct_ptr)->data, (struct_ptr)->size)


DN_API  DN_Str8Builder           DN_Str8Builder_FromArena               (DN_Arena *arena);
DN_API  DN_Str8Builder           DN_Str8Builder_FromStr8PtrRef          (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API  DN_Str8Builder           DN_Str8Builder_FromStr8PtrCopy         (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API  DN_Str8Builder           DN_Str8Builder_FromBuilder             (DN_Arena *arena, DN_Str8Builder const *builder);
DN_API  bool                     DN_Str8Builder_AddArrayRef             (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API  bool                     DN_Str8Builder_AddArrayCopy            (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API  bool                     DN_Str8Builder_AddFV                   (DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                          DN_Str8Builder_AppendArrayRef(builder, strings, size)  DN_Str8Builder_AddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                          DN_Str8Builder_AppendArrayCopy(builder, strings, size) DN_Str8Builder_AddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                          DN_Str8Builder_AppendSliceRef(builder, slice)          DN_Str8Builder_AddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
#define                          DN_Str8Builder_AppendSliceCopy(builder, slice)         DN_Str8Builder_AddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
DN_API  bool                     DN_Str8Builder_AppendRef               (DN_Str8Builder *builder, DN_Str8 string);
DN_API  bool                     DN_Str8Builder_AppendCopy              (DN_Str8Builder *builder, DN_Str8 string);
#define                          DN_Str8Builder_AppendFV(builder, fmt, args)            DN_Str8Builder_AddFV(builder, DN_Str8BuilderAdd_Append, fmt, args)
DN_API  bool                     DN_Str8Builder_AppendF                 (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  bool                     DN_Str8Builder_AppendBytesRef          (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API  bool                     DN_Str8Builder_AppendBytesCopy         (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API  bool                     DN_Str8Builder_AppendBuilderRef        (DN_Str8Builder *dest, DN_Str8Builder const *src);
DN_API  bool                     DN_Str8Builder_AppendBuilderCopy       (DN_Str8Builder *dest, DN_Str8Builder const *src);
#define                          DN_Str8Builder_PrependArrayRef(builder, strings, size)  DN_Str8Builder_AddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                          DN_Str8Builder_PrependArrayCopy(builder, strings, size) DN_Str8Builder_AddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                          DN_Str8Builder_PrependSliceRef(builder, slice)          DN_Str8Builder_AddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
#define                          DN_Str8Builder_PrependSliceCopy(builder, slice)         DN_Str8Builder_AddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
DN_API  bool                     DN_Str8Builder_PrependRef              (DN_Str8Builder *builder, DN_Str8 string);
DN_API  bool                     DN_Str8Builder_PrependCopy             (DN_Str8Builder *builder, DN_Str8 string);
#define                          DN_Str8Builder_PrependFV(builder, fmt, args)            DN_Str8Builder_AddFV(builder, DN_Str8BuilderAdd_Prepend, fmt, args)
DN_API  bool                     DN_Str8Builder_PrependF                (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  bool                     DN_Str8Builder_Erase                   (DN_Str8Builder *builder, DN_Str8 string);
DN_API  DN_Str8                  DN_Str8Builder_Build                   (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API  DN_Str8                  DN_Str8Builder_BuildDelimited          (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena);
DN_API  DN_Slice<DN_Str8>        DN_Str8Builder_BuildSlice              (DN_Str8Builder const *builder, DN_Arena *arena);

DN_API bool                      DN_Char_IsAlphabet                     (char ch);
DN_API bool                      DN_Char_IsDigit                        (char ch);
DN_API bool                      DN_Char_IsAlphaNum                     (char ch);
DN_API bool                      DN_Char_IsWhitespace                   (char ch);
DN_API bool                      DN_Char_IsHex                          (char ch);
DN_API char                      DN_Char_ToHex                          (char ch);
DN_API char                      DN_Char_ToHexUnchecked                 (char ch);
DN_API char                      DN_Char_ToLower                        (char ch);
DN_API char                      DN_Char_ToUpper                        (char ch);

DN_API int                       DN_UTF8_EncodeCodepoint                (uint8_t utf8[4], uint32_t codepoint);
DN_API int                       DN_UTF16_EncodeCodepoint               (uint16_t utf16[2], uint32_t codepoint);
#endif   // !defined(DN_BASE_STRING_H)
