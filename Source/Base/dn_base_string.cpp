#define DN_STRING_CPP

#include "../dn_base_inc.h"

// NOTE: DN_CStr8 //////////////////////////////////////////////////////////////////////////////////
DN_API DN_USize DN_CStr8_FSize(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize result = DN_VSNPrintF(nullptr, 0, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_USize DN_CStr8_FVSize(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  va_list args_copy;
  va_copy(args_copy, args);
  DN_USize result = DN_VSNPrintF(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  return result;
}

DN_API DN_USize DN_CStr8_Size(char const *src)
{
  DN_USize result = 0;
  while (src && src[0] != 0) {
    src++;
    result++;
  }
  return result;
}

DN_API DN_USize DN_CStr16_Size(wchar_t const *src)
{
  DN_USize result = 0;
  while (src && src[0] != 0) {
    src++;
    result++;
  }

  return result;
}

// NOTE: DN_Str16 //////////////////////////////////////////////////////////////////////////////////
DN_API bool operator==(DN_Str16 const &lhs, DN_Str16 const &rhs)
{
  bool result = false;
  if (lhs.size == rhs.size)
    result = DN_Memcmp(lhs.data, rhs.data, lhs.size * sizeof(*lhs.data)) == 0;
  return result;
}

DN_API bool operator!=(DN_Str16 const &lhs, DN_Str16 const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_Str8_Alloc(DN_Arena *arena, DN_USize size, DN_ZeroMem zero_mem)
{
  DN_Str8 result = {};
  result.data    = DN_Arena_NewArray(arena, char, size + 1, zero_mem);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8_AllocPool(DN_Pool *pool, DN_USize size)
{
  DN_Str8 result = {};
  result.data    = DN_Pool_NewArray(pool, char, size + 1);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8_FromCStr8(char const *src)
{
  DN_USize size   = DN_CStr8_Size(src);
  DN_Str8  result = DN_Str8_Init(src, size);
  return result;
}

DN_API DN_Str8 DN_Str8_FromF(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  DN_Str8 result = DN_Str8_FromFV(arena, fmt, va);
  va_end(va);
  return result;
}

DN_API DN_Str8 DN_Str8_FromFPool(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize size = DN_CStr8_FVSize(fmt, args);
  va_end(args);

  DN_Str8 result = {};
  if (size)
    result = DN_Str8_AllocPool(pool, size);
  if (result.data)
    DN_VSNPrintF(result.data, DN_SaturateCastISizeToInt(size + 1 /*null-terminator*/), fmt, args);
  return result;
}

DN_API DN_Str8 DN_Str8_FromFV(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = {};
  if (!fmt)
    return result;

  DN_USize size = DN_CStr8_FVSize(fmt, args);
  if (size) {
    result = DN_Str8_Alloc(arena, size, DN_ZeroMem_No);
    if (DN_Str8_HasData(result))
      DN_VSNPrintF(result.data, DN_SaturateCastISizeToInt(size + 1 /*null-terminator*/), fmt, args);
  }
  return result;
}

DN_API bool DN_Str8_IsAll(DN_Str8 string, DN_Str8IsAll is_all)
{
  bool result = DN_Str8_HasData(string);
  if (!result)
    return result;

  switch (is_all) {
    case DN_Str8IsAll_Digits: {
      for (DN_USize index = 0; result && index < string.size; index++)
        result = string.data[index] >= '0' && string.data[index] <= '9';
    } break;

    case DN_Str8IsAll_Hex: {
      DN_Str8 trimmed = DN_Str8_TrimPrefix(string, DN_STR8("0x"), DN_Str8EqCase_Insensitive);
      for (DN_USize index = 0; result && index < trimmed.size; index++) {
        char ch = trimmed.data[index];
        result  = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
      }
    } break;
  }

  return result;
}

DN_API char *DN_Str8_End(DN_Str8 string)
{
  char *result = string.data + string.size;
  return result;
}

DN_API DN_Str8 DN_Str8_Slice(DN_Str8 string, DN_USize offset, DN_USize size)
{
  DN_Str8 result = DN_Str8_Init(string.data, 0);
  if (!DN_Str8_HasData(string))
    return result;

  DN_USize capped_offset = DN_Min(offset, string.size);
  DN_USize max_size      = string.size - capped_offset;
  DN_USize capped_size   = DN_Min(size, max_size);
  result                 = DN_Str8_Init(string.data + capped_offset, capped_size);
  return result;
}

DN_API DN_Str8 DN_Str8_Advance(DN_Str8 string, DN_USize amount)
{
  DN_Str8 result = DN_Str8_Slice(string, amount, DN_USIZE_MAX);
  return result;
}

DN_API DN_Str8 DN_Str8_NextLine(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_BSplit(string, DN_STR8("\n")).rhs;
  return result;
}

DN_API DN_Str8BSplitResult DN_Str8_BSplitArray(DN_Str8 string, DN_Str8 const *find, DN_USize find_size)
{
  DN_Str8BSplitResult result = {};
  if (!DN_Str8_HasData(string) || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (size_t index = 0; !result.rhs.data && index < string.size; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8_Slice(string, index, find_item.size);
      if (DN_Str8_Eq(string_slice, find_item)) {
        result.lhs.size = index;
        result.rhs.data = string_slice.data + find_item.size;
        result.rhs.size = string.size - (index + find_item.size);
        break;
      }
    }
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8_BSplit(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result = DN_Str8_BSplitArray(string, &find, 1);
  return result;
}

DN_API DN_Str8BSplitResult DN_Str8_BSplitLastArray(DN_Str8 string, DN_Str8 const *find, DN_USize find_size)
{
  DN_Str8BSplitResult result = {};
  if (!DN_Str8_HasData(string) || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (size_t index = string.size - 1; !result.rhs.data && index < string.size; index--) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8_Slice(string, index, find_item.size);
      if (DN_Str8_Eq(string_slice, find_item)) {
        result.lhs.size = index;
        result.rhs.data = string_slice.data + find_item.size;
        result.rhs.size = string.size - (index + find_item.size);
        break;
      }
    }
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8_BSplitLast(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result = DN_Str8_BSplitLastArray(string, &find, 1);
  return result;
}

DN_API DN_USize DN_Str8_Split(DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_USize result = 0; // The number of splits in the actual string.
  if (!DN_Str8_HasData(string) || !DN_Str8_HasData(delimiter) || delimiter.size <= 0)
    return result;

  DN_Str8BSplitResult split = {};
  DN_Str8                  first = string;
  do {
    split = DN_Str8_BSplit(first, delimiter);
    if (split.lhs.size || mode == DN_Str8SplitIncludeEmptyStrings_Yes) {
      if (splits && result < splits_count)
        splits[result] = split.lhs;
      result++;
    }
    first = split.rhs;
  } while (first.size);

  return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8_SplitAlloc(DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Slice<DN_Str8> result          = {};
  DN_USize          splits_required = DN_Str8_Split(string, delimiter, /*splits*/ nullptr, /*count*/ 0, mode);
  result.data                       = DN_Arena_NewArray(arena, DN_Str8, splits_required, DN_ZeroMem_No);
  if (result.data) {
    result.size = DN_Str8_Split(string, delimiter, result.data, splits_required, mode);
    DN_Assert(splits_required == result.size);
  }
  return result;
}

DN_API DN_Str8FindResult DN_Str8_FindStr8Array(DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case)
{
  DN_Str8FindResult result = {};
  for (DN_USize index = 0; !result.found && index < string.size; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8_Slice(string, index, find_item.size);
      if (DN_Str8_Eq(string_slice, find_item, eq_case)) {
        result.found                        = true;
        result.index                        = index;
        result.start_to_before_match        = DN_Str8_Init(string.data, index);
        result.match                        = DN_Str8_Init(string.data + index, find_item.size);
        result.match_to_end_of_buffer       = DN_Str8_Init(result.match.data, string.size - index);
        result.after_match_to_end_of_buffer = DN_Str8_Advance(result.match_to_end_of_buffer, find_item.size);
        break;
      }
    }
  }
  return result;
}

DN_API DN_Str8FindResult DN_Str8_FindStr8(DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case)
{
  DN_Str8FindResult result = DN_Str8_FindStr8Array(string, &find, 1, eq_case);
  return result;
}

DN_API DN_Str8FindResult DN_Str8_Find(DN_Str8 string, uint32_t flags)
{
  DN_Str8FindResult result = {};
  for (size_t index = 0; !result.found && index < string.size; index++) {
    result.found |= ((flags & DN_Str8FindFlag_Digit) && DN_Char_IsDigit(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Alphabet) && DN_Char_IsAlphabet(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Whitespace) && DN_Char_IsWhitespace(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Plus) && string.data[index] == '+');
    result.found |= ((flags & DN_Str8FindFlag_Minus) && string.data[index] == '-');
    if (result.found) {
      result.index                        = index;
      result.match                        = DN_Str8_Init(string.data + index, 1);
      result.match_to_end_of_buffer       = DN_Str8_Init(result.match.data, string.size - index);
      result.after_match_to_end_of_buffer = DN_Str8_Advance(result.match_to_end_of_buffer, 1);
    }
  }
  return result;
}

DN_API DN_Str8 DN_Str8_Segment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || !DN_Str8_HasData(src)) {
    DN_Str8 result = DN_Str8_FromStr8(arena, src);
    return result;
  }

  DN_USize segments = src.size / segment_size;
  if (src.size % segment_size == 0)
    segments--;

  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8_Alloc(arena, src.size + segments, DN_ZeroMem_Yes);
  DN_USize write_index     = 0;
  for (DN_ForIndexU(src_index, src.size)) {
    result.data[write_index++] = src.data[src_index];
    if ((src_index + 1) % segment_size == 0 && segment_counter < segments) {
      result.data[write_index++] = segment_char;
      segment_counter++;
    }
    DN_AssertF(write_index <= result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
  }

  DN_AssertF(write_index == result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
  return result;
}

DN_API DN_Str8 DN_Str8_ReverseSegment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || !DN_Str8_HasData(src)) {
    DN_Str8 result = DN_Str8_FromStr8(arena, src);
    return result;
  }

  DN_USize segments = src.size / segment_size;
  if (src.size % segment_size == 0)
    segments--;

  DN_USize write_counter   = 0;
  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8_Alloc(arena, src.size + segments, DN_ZeroMem_Yes);
  DN_USize write_index     = result.size - 1;

  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
  for (size_t src_index = src.size - 1; src_index < src.size; src_index--) {
    DN_MSVC_WARNING_POP
    result.data[write_index--] = src.data[src_index];
    if (++write_counter % segment_size == 0 && segment_counter < segments) {
      result.data[write_index--] = segment_char;
      segment_counter++;
    }
  }

  DN_Assert(write_index == SIZE_MAX);
  return result;
}

DN_API bool DN_Str8_Eq(DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case)
{
  if (lhs.size != rhs.size)
    return false;

  if (lhs.size == 0)
    return true;

  if (!lhs.data || !rhs.data)
    return false;

  bool result = true;
  switch (eq_case) {
    case DN_Str8EqCase_Sensitive: {
      result = (DN_Memcmp(lhs.data, rhs.data, lhs.size) == 0);
    } break;

    case DN_Str8EqCase_Insensitive: {
      for (DN_USize index = 0; index < lhs.size && result; index++)
        result = (DN_Char_ToLower(lhs.data[index]) == DN_Char_ToLower(rhs.data[index]));
    } break;
  }
  return result;
}

DN_API bool DN_Str8_EqInsensitive(DN_Str8 lhs, DN_Str8 rhs)
{
  bool result = DN_Str8_Eq(lhs, rhs, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8_StartsWith(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data, DN_Min(prefix.size, string.size)};
  bool    result    = DN_Str8_Eq(substring, prefix, eq_case);
  return result;
}

DN_API bool DN_Str8_StartsWithInsensitive(DN_Str8 string, DN_Str8 prefix)
{
  bool result = DN_Str8_StartsWith(string, prefix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8_EndsWith(DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data + string.size - suffix.size, DN_Min(string.size, suffix.size)};
  bool    result    = DN_Str8_Eq(substring, suffix, eq_case);
  return result;
}

DN_API bool DN_Str8_EndsWithInsensitive(DN_Str8 string, DN_Str8 suffix)
{
  bool result = DN_Str8_EndsWith(string, suffix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8_HasChar(DN_Str8 string, char ch)
{
  bool result = false;
  for (DN_USize index = 0; !result && index < string.size; index++)
    result = string.data[index] == ch;
  return result;
}

DN_API DN_Str8 DN_Str8_TrimPrefix(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 result = string;
  if (DN_Str8_StartsWith(string, prefix, eq_case)) {
    result.data += prefix.size;
    result.size -= prefix.size;
  }
  return result;
}

DN_API DN_Str8 DN_Str8_TrimHexPrefix(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_TrimPrefix(string, DN_STR8("0x"), DN_Str8EqCase_Insensitive);
  return result;
}

DN_API DN_Str8 DN_Str8_TrimSuffix(DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case)
{
  DN_Str8 result = string;
  if (DN_Str8_EndsWith(string, suffix, eq_case))
    result.size -= suffix.size;
  return result;
}

DN_API DN_Str8 DN_Str8_TrimAround(DN_Str8 string, DN_Str8 trim_string)
{
  DN_Str8 result = DN_Str8_TrimPrefix(string, trim_string);
  result         = DN_Str8_TrimSuffix(result, trim_string);
  return result;
}

DN_API DN_Str8 DN_Str8_TrimHeadWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (!DN_Str8_HasData(string))
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.size;
  while (start < end && DN_Char_IsWhitespace(start[0]))
    start++;

  result = DN_Str8_Init(start, end - start);
  return result;
}

DN_API DN_Str8 DN_Str8_TrimTailWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (!DN_Str8_HasData(string))
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.size;
  while (end > start && DN_Char_IsWhitespace(end[-1]))
    end--;

  result = DN_Str8_Init(start, end - start);
  return result;
}

DN_API DN_Str8 DN_Str8_TrimWhitespaceAround(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_TrimHeadWhitespace(string);
  result         = DN_Str8_TrimTailWhitespace(result);
  return result;
}

DN_API DN_Str8 DN_Str8_TrimByteOrderMark(DN_Str8 string)
{
  DN_Str8 result = string;
  if (!DN_Str8_HasData(result))
    return result;

  // TODO(dn): This is little endian
  DN_Str8 UTF8_BOM     = DN_STR8("\xEF\xBB\xBF");
  DN_Str8 UTF16_BOM_BE = DN_STR8("\xEF\xFF");
  DN_Str8 UTF16_BOM_LE = DN_STR8("\xFF\xEF");
  DN_Str8 UTF32_BOM_BE = DN_STR8("\x00\x00\xFE\xFF");
  DN_Str8 UTF32_BOM_LE = DN_STR8("\xFF\xFE\x00\x00");

  result = DN_Str8_TrimPrefix(result, UTF8_BOM, DN_Str8EqCase_Sensitive);
  result = DN_Str8_TrimPrefix(result, UTF16_BOM_BE, DN_Str8EqCase_Sensitive);
  result = DN_Str8_TrimPrefix(result, UTF16_BOM_LE, DN_Str8EqCase_Sensitive);
  result = DN_Str8_TrimPrefix(result, UTF32_BOM_BE, DN_Str8EqCase_Sensitive);
  result = DN_Str8_TrimPrefix(result, UTF32_BOM_LE, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8_FileNameFromPath(DN_Str8 path)
{
  DN_Str8                  separators[] = {DN_STR8("/"), DN_STR8("\\")};
  DN_Str8BSplitResult split        = DN_Str8_BSplitLastArray(path, separators, DN_ArrayCountU(separators));
  DN_Str8                  result       = DN_Str8_HasData(split.rhs) ? split.rhs : split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8_FileNameNoExtension(DN_Str8 path)
{
  DN_Str8 file_name = DN_Str8_FileNameFromPath(path);
  DN_Str8 result    = DN_Str8_FilePathNoExtension(file_name);
  return result;
}

DN_API DN_Str8 DN_Str8_FilePathNoExtension(DN_Str8 path)
{
  DN_Str8BSplitResult split  = DN_Str8_BSplitLast(path, DN_STR8("."));
  DN_Str8                  result = split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8_FileExtension(DN_Str8 path)
{
  DN_Str8BSplitResult split  = DN_Str8_BSplitLast(path, DN_STR8("."));
  DN_Str8                  result = split.rhs;
  return result;
}

DN_API DN_Str8 DN_Str8_FileDirectoryFromPath(DN_Str8 path)
{
  DN_Str8                  separators[] = {DN_STR8("/"), DN_STR8("\\")};
  DN_Str8BSplitResult split        = DN_Str8_BSplitLastArray(path, separators, DN_ArrayCountU(separators));
  DN_Str8                  result       = split.lhs;
  return result;
}

DN_API DN_Str8ToU64Result DN_Str8_ToU64(DN_Str8 string, char separator)
{
  // NOTE: Argument check
  DN_Str8ToU64Result result = {};
  if (!DN_Str8_HasData(string)) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8_TrimWhitespaceAround(string);
  if (trim_string.size == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Handle prefix '+'
  DN_USize start_index = 0;
  if (!DN_Char_IsDigit(trim_string.data[0])) {
    if (trim_string.data[0] != '+')
      return result;
    start_index++;
  }

  // NOTE: Convert the string number to the binary number
  for (DN_USize index = start_index; index < trim_string.size; index++) {
    char ch = trim_string.data[index];
    if (index) {
      if (separator != 0 && ch == separator)
        continue;
    }

    if (!DN_Char_IsDigit(ch))
      return result;

    result.value   = DN_SafeMulU64(result.value, 10);
    uint64_t digit = ch - '0';
    result.value   = DN_SafeAddU64(result.value, digit);
  }

  result.success = true;
  return result;
}

DN_API DN_Str8ToI64Result DN_Str8_ToI64(DN_Str8 string, char separator)
{
  // NOTE: Argument check
  DN_Str8ToI64Result result = {};
  if (!DN_Str8_HasData(string)) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8_TrimWhitespaceAround(string);
  if (trim_string.size == 0) {
    result.success = true;
    return result;
  }

  bool     negative    = false;
  DN_USize start_index = 0;
  if (!DN_Char_IsDigit(trim_string.data[0])) {
    negative = (trim_string.data[start_index] == '-');
    if (!negative && trim_string.data[0] != '+')
      return result;
    start_index++;
  }

  // NOTE: Convert the string number to the binary number
  for (DN_USize index = start_index; index < trim_string.size; index++) {
    char ch = trim_string.data[index];
    if (index) {
      if (separator != 0 && ch == separator)
        continue;
    }

    if (!DN_Char_IsDigit(ch))
      return result;

    result.value   = DN_SafeMulU64(result.value, 10);
    uint64_t digit = ch - '0';
    result.value   = DN_SafeAddU64(result.value, digit);
  }

  if (negative)
    result.value *= -1;

  result.success = true;
  return result;
}

DN_API DN_Str8 DN_Str8_AppendF(DN_Arena *arena, DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_AppendFV(arena, string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_AppendFV(DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args)
{
  // TODO: Calculate size and write into one buffer instead of 2 appends
  DN_Str8 append = DN_Str8_FromFV(arena, fmt, args);
  DN_Str8 result = DN_Str8_Alloc(arena, string.size + append.size, DN_ZeroMem_No);
  DN_Memcpy(result.data, string.data, string.size);
  DN_Memcpy(result.data + string.size, append.data, append.size);
  return result;
}

DN_API DN_Str8 DN_Str8_FillF(DN_Arena *arena, DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_FillFV(arena, count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_FillFV(DN_Arena *arena, DN_USize count, char const *fmt, va_list args)
{
  DN_Str8 fill = DN_Str8_FromFV(arena, fmt, args);
  DN_Str8 result = DN_Str8_Alloc(arena, count * fill.size, DN_ZeroMem_No);
  for (DN_USize index = 0; index < count; index++) {
    void *dest = result.data + (index * fill.size);
    DN_Memcpy(dest, fill.data, fill.size);
  }
  return result;
}

DN_API void DN_Str8_Remove(DN_Str8 *string, DN_USize offset, DN_USize size)
{
  if (!string || !DN_Str8_HasData(*string))
    return;

  char    *end           = string->data + string->size;
  char    *dest          = DN_Min(string->data + offset, end);
  char    *src           = DN_Min(string->data + offset + size, end);
  DN_USize bytes_to_move = end - src;
  DN_Memmove(dest, src, bytes_to_move);
  string->size -= bytes_to_move;
}

DN_API DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddle(DN_Arena *arena, DN_Str8 str8, uint32_t side_size, DN_Str8 truncator)
{
  DN_Str8DotTruncateResult result = {};
  if (str8.size <= (side_size * 2)) {
    result.str8 = DN_Str8_FromStr8(arena, str8);
    return result;
  }

  DN_Str8 head     = DN_Str8_Slice(str8, 0, side_size);
  DN_Str8 tail     = DN_Str8_Slice(str8, str8.size - side_size, side_size);
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(3) when a string is required in call to 'DN_Str8_FromF' Actual type: 'struct DN_Str8'
  result.str8      = DN_Str8_FromF(arena, "%S%S%S", head, truncator, tail);
  DN_MSVC_WARNING_POP
  result.truncated = true;
  return result;
}

DN_API DN_Str8 DN_Str8_Lower(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = DN_Str8_FromStr8(arena, string);
  for (DN_ForIndexU(index, result.size))
    result.data[index] = DN_Char_ToLower(result.data[index]);
  return result;
}

DN_API DN_Str8 DN_Str8_Upper(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = DN_Str8_FromStr8(arena, string);
  for (DN_ForIndexU(index, result.size))
    result.data[index] = DN_Char_ToUpper(result.data[index]);
  return result;
}

#if defined(__cplusplus)
DN_API bool operator==(DN_Str8 const &lhs, DN_Str8 const &rhs)
{
  bool result = DN_Str8_Eq(lhs, rhs, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API bool operator!=(DN_Str8 const &lhs, DN_Str8 const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}
#endif

DN_API DN_Str8 DN_Str8_FromStr8(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Alloc(arena, string.size, DN_ZeroMem_No);
  if (DN_Str8_HasData(result)) {
    DN_Memcpy(result.data, string.data, string.size);
    result.data[string.size] = 0;
  }
  return result;
}

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////
DN_API DN_Str8Builder DN_Str8Builder_FromArena(DN_Arena *arena)
{
  DN_Str8Builder result = {};
  result.arena          = arena;
  return result;
}

DN_API DN_Str8Builder DN_Str8Builder_FromStr8PtrRef(DN_Arena *arena, DN_Str8 const *strings, DN_USize size)
{
  DN_Str8Builder result = DN_Str8Builder_FromArena(arena);
  DN_Str8Builder_AppendArrayRef(&result, strings, size);
  return result;
}

DN_API DN_Str8Builder DN_Str8Builder_FromStr8PtrCopy(DN_Arena *arena, DN_Str8 const *strings, DN_USize size)
{
  DN_Str8Builder result = DN_Str8Builder_FromArena(arena);
  DN_Str8Builder_AppendArrayCopy(&result, strings, size);
  return result;
}

DN_API DN_Str8Builder DN_Str8Builder_FromBuilder(DN_Arena *arena, DN_Str8Builder const *builder)
{
  DN_Str8Builder result = DN_Str8Builder_FromArena(arena);
  DN_Str8Builder_AppendBuilderCopy(&result, builder);
  return result;
}


DN_API bool DN_Str8Builder_AddArrayRef(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || size <= 0)
    return true;

  DN_Str8Link *links = DN_Arena_NewArray(builder->arena, DN_Str8Link, size, DN_ZeroMem_No);
  if (!links)
    return false;

  if (add == DN_Str8BuilderAdd_Append) {
    for (DN_ForIndexU(index, size)) {
      DN_Str8      string = strings[index];
      DN_Str8Link *link   = links + index;

      link->string = string;
      link->next   = NULL;

      if (builder->head)
        builder->tail->next = link;
      else
        builder->head = link;

      builder->tail = link;
      builder->count++;
      builder->string_size += string.size;
    }
  } else {
    DN_Assert(add == DN_Str8BuilderAdd_Prepend);
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
    for (DN_USize index = size - 1; index < size; index--) {
      DN_MSVC_WARNING_POP
      DN_Str8      string = strings[index];
      DN_Str8Link *link   = links + index;
      link->string        = string;
      link->next          = builder->head;
      builder->head       = link;
      if (!builder->tail)
        builder->tail = link;
      builder->count++;
      builder->string_size += string.size;
    }
  }
  return true;
}

DN_API bool DN_Str8Builder_AddArrayCopy(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || size <= 0)
    return true;

  DN_ArenaTempMem tmp_mem      = DN_Arena_TempMemBegin(builder->arena);
  bool            result       = true;
  DN_Str8        *strings_copy = DN_Arena_NewArray(builder->arena, DN_Str8, size, DN_ZeroMem_No);
  for (DN_ForIndexU(index, size)) {
    strings_copy[index] = DN_Str8_FromStr8(builder->arena, strings[index]);
    if (strings_copy[index].size != strings[index].size) {
      result = false;
      break;
    }
  }

  if (result)
    result = DN_Str8Builder_AddArrayRef(builder, strings_copy, size, add);

  if (!result)
    DN_Arena_TempMemEnd(tmp_mem);

  return result;
}

DN_API bool DN_Str8Builder_AddFV(DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8         string   = DN_Str8_FromFV(builder->arena, fmt, args);
  DN_ArenaTempMem temp_mem = DN_Arena_TempMemBegin(builder->arena);
  bool            result   = DN_Str8Builder_AddArrayRef(builder, &string, 1, add);
  if (!result)
    DN_Arena_TempMemEnd(temp_mem);
  return result;
}

DN_API bool DN_Str8Builder_AppendRef(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8Builder_AddArrayRef(builder, &string, 1, DN_Str8BuilderAdd_Append);
  return result;
}

DN_API bool DN_Str8Builder_AppendCopy(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8Builder_AddArrayCopy(builder, &string, 1, DN_Str8BuilderAdd_Append);
  return result;
}

DN_API bool DN_Str8Builder_AppendF(DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_Str8Builder_AppendFV(builder, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_Str8Builder_AppendBytesRef(DN_Str8Builder *builder, void const *ptr, DN_USize size)
{
  DN_Str8 input  = DN_Str8_Init(ptr, size);
  bool    result = DN_Str8Builder_AppendRef(builder, input);
  return result;
}

DN_API bool DN_Str8Builder_AppendBytesCopy(DN_Str8Builder *builder, void const *ptr, DN_USize size)
{
  DN_Str8 input  = DN_Str8_Init(ptr, size);
  bool    result = DN_Str8Builder_AppendCopy(builder, input);
  return result;
}

static bool DN_Str8Builder_AppendBuilder_(DN_Str8Builder *dest, DN_Str8Builder const *src, bool copy)
{
  if (!dest)
    return false;
  if (!src)
    return true;

  DN_Arena_TempMemBegin(dest->arena);
  DN_Str8Link *links = DN_Arena_NewArray(dest->arena, DN_Str8Link, src->count, DN_ZeroMem_No);
  if (!links)
    return false;

  DN_Str8Link *first      = nullptr;
  DN_Str8Link *last       = nullptr;
  DN_USize     link_index = 0;
  bool         result     = true;
  for (DN_Str8Link const *it = src->head; it; it = it->next) {
    DN_Str8Link *link = links + link_index++;
    link->next        = nullptr;
    link->string      = it->string;

    if (copy) {
      link->string = DN_Str8_FromStr8(dest->arena, it->string);
      if (link->string.size != it->string.size) {
        result = false;
        break;
      }
    }

    if (last)
      last->next = link;
    else
      first = link;
    last = link;
  }

  if (result) {
    if (dest->head)
      dest->tail->next = first;
    else
      dest->head = first;
    dest->tail = last;
    dest->count += src->count;
    dest->string_size += src->string_size;
  }
  return true;
}

DN_API bool DN_Str8Builder_AppendBuilderRef(DN_Str8Builder *dest, DN_Str8Builder const *src)
{
  bool result = DN_Str8Builder_AppendBuilder_(dest, src, false);
  return result;
}

DN_API bool DN_Str8Builder_AppendBuilderCopy(DN_Str8Builder *dest, DN_Str8Builder const *src)
{
  bool result = DN_Str8Builder_AppendBuilder_(dest, src, true);
  return result;
}

DN_API bool DN_Str8Builder_PrependRef(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8Builder_AddArrayRef(builder, &string, 1, DN_Str8BuilderAdd_Prepend);
  return result;
}

DN_API bool DN_Str8Builder_PrependCopy(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8Builder_AddArrayCopy(builder, &string, 1, DN_Str8BuilderAdd_Prepend);
  return result;
}

DN_API bool DN_Str8Builder_PrependF(DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_Str8Builder_PrependFV(builder, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_Str8Builder_Erase(DN_Str8Builder *builder, DN_Str8 string)
{
  for (DN_Str8Link **it = &builder->head; *it; it = &((*it)->next)) {
    if ((*it)->string == string) {
      *it = (*it)->next;
      builder->string_size -= string.size;
      builder->count -= 1;
      return true;
    }
  }
  return false;
}

DN_API DN_Str8 DN_Str8Builder_Build(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8Builder_BuildDelimited(builder, DN_STR8(""), arena);
  return result;
}

DN_API DN_Str8 DN_Str8Builder_BuildDelimited(DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  DN_USize size_for_delimiter = DN_Str8_HasData(delimiter) ? ((builder->count - 1) * delimiter.size) : 0;
  result.data                 = DN_Arena_NewArray(arena,
                                  char,
                                  builder->string_size + size_for_delimiter + 1 /*null terminator*/,
                                  DN_ZeroMem_No);
  if (!result.data)
    return result;

  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + result.size, link->string.data, link->string.size);
    result.size += link->string.size;
    if (link->next && DN_Str8_HasData(delimiter)) {
      DN_Memcpy(result.data + result.size, delimiter.data, delimiter.size);
      result.size += delimiter.size;
    }
  }

  result.data[result.size] = 0;
  DN_Assert(result.size == builder->string_size + size_for_delimiter);
  return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8Builder_BuildSlice(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Slice<DN_Str8> result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result = DN_Slice_Alloc<DN_Str8>(arena, builder->count, DN_ZeroMem_No);
  if (!result.data)
    return result;

  DN_USize slice_index = 0;
  for (DN_Str8Link *link = builder->head; link; link = link->next)
    result.data[slice_index++] = DN_Str8_FromStr8(arena, link->string);

  DN_Assert(slice_index == builder->count);
  return result;
}

DN_API DN_Str8 DN_LStr8_AppendFV(char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, va_list args)
{
  *buf_size      += DN_VSNPrintF(buf + *buf_size, DN_CAST(int)(buf_max - *buf_size), fmt, args);
  DN_Str8 result  = DN_Str8_Init(buf, *buf_size);
  return result;
}

DN_API DN_Str8 DN_LStr8_AppendF(char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_LStr8_AppendFV(buf, buf_size, buf_max, fmt, args);
  va_end(args);
  return result;
}

// NOTE: DN_Char ///////////////////////////////////////////////////////////////////////////////////
DN_API bool DN_Char_IsAlphabet(char ch)
{
  bool result = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
  return result;
}

DN_API bool DN_Char_IsDigit(char ch)
{
  bool result = (ch >= '0' && ch <= '9');
  return result;
}

DN_API bool DN_Char_IsAlphaNum(char ch)
{
  bool result = DN_Char_IsAlphabet(ch) || DN_Char_IsDigit(ch);
  return result;
}

DN_API bool DN_Char_IsWhitespace(char ch)
{
  bool result = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
  return result;
}

DN_API bool DN_Char_IsHex(char ch)
{
  bool result = ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || (ch >= '0' && ch <= '9'));
  return result;
}

DN_API char DN_Char_ToLower(char ch)
{
  char result = ch;
  if (result >= 'A' && result <= 'Z')
    result += 'a' - 'A';
  return result;
}

DN_API char DN_Char_ToUpper(char ch)
{
  char result = ch;
  if (result >= 'a' && result <= 'z')
    result -= 'a' - 'A';
  return result;
}

// NOTE: DN_UTF ////////////////////////////////////////////////////////////////////////////////////
DN_API int DN_UTF8_EncodeCodepoint(uint8_t utf8[4], uint32_t codepoint)
{
  // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
  // ----------------------------------------+----------------------------+--------------------+
  // UTF-8 (binary)                          | Code point (binary)        | Range              |
  // ----------------------------------------+----------------------------+--------------------+
  // 0xxx'xxxx                               |                   xxx'xxxx | U+0000  - U+007F   |
  // 110x'xxxx 10yy'yyyy                     |              xxx'xxyy'yyyy | U+0080  - U+07FF   |
  // 1110'xxxx 10yy'yyyy 10zz'zzzz           |        xxxx'yyyy'yyzz'zzzz | U+0800  - U+FFFF   |
  // 1111'0xxx 10yy'yyyy 10zz'zzzz 10ww'wwww | x'xxyy'yyyy'zzzz'zzww'wwww | U+10000 - U+10FFFF |
  // ----------------------------------------+----------------------------+--------------------+

  if (codepoint <= 0b0111'1111) {
    utf8[0] = DN_CAST(uint8_t) codepoint;
    return 1;
  }

  if (codepoint <= 0b0111'1111'1111) {
    utf8[0] = (0b1100'0000 | ((codepoint >> 6) & 0b01'1111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111)); // y
    return 2;
  }

  if (codepoint <= 0b1111'1111'1111'1111) {
    utf8[0] = (0b1110'0000 | ((codepoint >> 12) & 0b00'1111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 6) & 0b11'1111));  // y
    utf8[2] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111));  // z
    return 3;
  }

  if (codepoint <= 0b1'1111'1111'1111'1111'1111) {
    utf8[0] = (0b1111'0000 | ((codepoint >> 18) & 0b00'0111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 12) & 0b11'1111)); // y
    utf8[2] = (0b1000'0000 | ((codepoint >> 6) & 0b11'1111));  // z
    utf8[3] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111));  // w
    return 4;
  }

  return 0;
}

DN_API int DN_UTF16_EncodeCodepoint(uint16_t utf16[2], uint32_t codepoint)
{
  // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
  // ----------------------------------------+------------------------------------+------------------+
  // UTF-16 (binary)                         | Code point (binary)                | Range            |
  // ----------------------------------------+------------------------------------+------------------+
  // xxxx'xxxx'xxxx'xxxx                     | xxxx'xxxx'xxxx'xxxx                | U+0000???U+FFFF    |
  // 1101'10xx'xxxx'xxxx 1101'11yy'yyyy'yyyy | xxxx'xxxx'xxyy'yyyy'yyyy + 0x10000 | U+10000???U+10FFFF |
  // ----------------------------------------+------------------------------------+------------------+

  if (codepoint <= 0b1111'1111'1111'1111) {
    utf16[0] = DN_CAST(uint16_t) codepoint;
    return 1;
  }

  if (codepoint <= 0b1111'1111'1111'1111'1111) {
    uint32_t surrogate_codepoint = codepoint + 0x10000;
    utf16[0]                     = 0b1101'1000'0000'0000 | ((surrogate_codepoint >> 10) & 0b11'1111'1111); // x
    utf16[1]                     = 0b1101'1100'0000'0000 | ((surrogate_codepoint >> 0) & 0b11'1111'1111);  // y
    return 2;
  }

  return 0;
}
