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
//   dqn_string.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$CSTR] Dqn_CStr8 /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_usize Dqn_CStr8_FSize(DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_usize result = DQN_VSNPRINTF(nullptr, 0, fmt, args);
    va_end(args);
    return result;
}

DQN_API Dqn_usize Dqn_CStr8_FVSize(DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    Dqn_usize result = DQN_VSNPRINTF(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    return result;
}

DQN_API Dqn_usize Dqn_CStr8_Size(char const *src)
{
    Dqn_usize result = 0;
    while (src && src[0] != 0) {
        src++;
        result++;
    }
    return result;
}

DQN_API Dqn_usize Dqn_CStr16_Size(wchar_t const *src)
{
    Dqn_usize result = 0;
    while (src && src[0] != 0) {
        src++;
        result++;
    }

    return result;
}

// NOTE: [$STR6] Dqn_Str16 /////////////////////////////////////////////////////////////////////////
DQN_API bool operator==(Dqn_Str16 const &lhs, Dqn_Str16 const &rhs)
{
    bool result = false;
    if (lhs.size == rhs.size)
        result = DQN_MEMCMP(lhs.data, rhs.data, lhs.size * sizeof(*lhs.data)) == 0;
    return result;
}

DQN_API bool operator!=(Dqn_Str16 const &lhs, Dqn_Str16 const &rhs)
{
    bool result = !(lhs == rhs);
    return result;
}

// NOTE: [$STR8] Dqn_Str8 //////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8 Dqn_Str8_InitCStr8(char const *src)
{
    Dqn_usize size   = Dqn_CStr8_Size(src);
    Dqn_Str8  result = Dqn_Str8_Init(src, size);
    return result;
}

DQN_API bool Dqn_Str8_IsAll(Dqn_Str8 string, Dqn_Str8IsAll is_all)
{
    bool result = Dqn_Str8_HasData(string);
    if (!result)
        return result;

    switch (is_all) {
        case Dqn_Str8IsAll_Digits: {
            for (Dqn_usize index = 0; result && index < string.size; index++)
                result = string.data[index] >= '0' && string.data[index] <= '9';
        } break;

        case Dqn_Str8IsAll_Hex: {
            Dqn_Str8 trimmed = Dqn_Str8_TrimPrefix(string, DQN_STR8("0x"), Dqn_Str8EqCase_Insensitive);
            for (Dqn_usize index = 0; result && index < trimmed.size; index++) {
                char ch = trimmed.data[index];
                result  = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
            }
        } break;
    }

    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Slice(Dqn_Str8 string, Dqn_usize offset, Dqn_usize size)
{
    Dqn_Str8 result = Dqn_Str8_Init(string.data, 0);
    if (!Dqn_Str8_HasData(string))
        return result;

    Dqn_usize capped_offset = DQN_MIN(offset, string.size);
    Dqn_usize max_size      = string.size - capped_offset;
    Dqn_usize capped_size   = DQN_MIN(size, max_size);
    result                  = Dqn_Str8_Init(string.data + capped_offset, capped_size);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Advance(Dqn_Str8 string, Dqn_usize amount)
{
    Dqn_Str8 result = Dqn_Str8_Slice(string, amount, DQN_USIZE_MAX);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_NextLine(Dqn_Str8 string)
{
    Dqn_Str8 result = Dqn_Str8_BinarySplit(string, DQN_STR8("\n")).rhs;
    return result;
}

DQN_API Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitArray(Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size)
{
    Dqn_Str8BinarySplitResult result = {};
    if (!Dqn_Str8_HasData(string) || !find || find_size == 0)
        return result;

    result.lhs = string;
    for (size_t index = 0; !result.rhs.data && index < string.size; index++) {
        for (Dqn_usize find_index = 0; find_index < find_size; find_index++) {
            Dqn_Str8 find_item    = find[find_index];
            Dqn_Str8 string_slice = Dqn_Str8_Slice(string, index, find_item.size);
            if (Dqn_Str8_Eq(string_slice, find_item)) {
                result.lhs.size = index;
                result.rhs.data = string_slice.data + find_item.size;
                result.rhs.size = string.size - (index + find_item.size);
                break;
            }
        }
    }

    return result;
}

DQN_API Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplit(Dqn_Str8 string, Dqn_Str8 find)
{
    Dqn_Str8BinarySplitResult result = Dqn_Str8_BinarySplitArray(string, &find, 1);
    return result;
}

DQN_API Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitLastArray(Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size)
{
    Dqn_Str8BinarySplitResult result = {};
    if (!Dqn_Str8_HasData(string) || !find || find_size == 0)
        return result;

    result.lhs = string;
    for (size_t index = string.size - 1; !result.rhs.data && index < string.size; index--) {
        for (Dqn_usize find_index = 0; find_index < find_size; find_index++) {
            Dqn_Str8 find_item    = find[find_index];
            Dqn_Str8 string_slice = Dqn_Str8_Slice(string, index, find_item.size);
            if (Dqn_Str8_Eq(string_slice, find_item)) {
                result.lhs.size = index;
                result.rhs.data = string_slice.data + find_item.size;
                result.rhs.size = string.size - (index + find_item.size);
                break;
            }
        }
    }

    return result;
}

DQN_API Dqn_Str8BinarySplitResult Dqn_Str8_BinarySplitLast(Dqn_Str8 string, Dqn_Str8 find)
{
    Dqn_Str8BinarySplitResult result = Dqn_Str8_BinarySplitLastArray(string, &find, 1);
    return result;
}

DQN_API Dqn_usize Dqn_Str8_Split(Dqn_Str8 string, Dqn_Str8 delimiter, Dqn_Str8 *splits, Dqn_usize splits_count, Dqn_Str8SplitIncludeEmptyStrings mode)
{
    Dqn_usize result = 0; // The number of splits in the actual string.
    if (!Dqn_Str8_HasData(string) || !Dqn_Str8_HasData(delimiter) || delimiter.size <= 0)
        return result;

    Dqn_Str8BinarySplitResult split = {};
    Dqn_Str8 first                  = string;
    do {
        split = Dqn_Str8_BinarySplit(first, delimiter);
        if (split.lhs.size || mode == Dqn_Str8SplitIncludeEmptyStrings_Yes) {
            if (splits && result < splits_count)
                splits[result] = split.lhs;
            result++;
        }
        first = split.rhs;
    } while (first.size);

    return result;
}

DQN_API Dqn_Slice<Dqn_Str8> Dqn_Str8_SplitAlloc(Dqn_Arena *arena, Dqn_Str8 string, Dqn_Str8 delimiter, Dqn_Str8SplitIncludeEmptyStrings mode)
{
    Dqn_Slice<Dqn_Str8> result          = {};
    Dqn_usize           splits_required = Dqn_Str8_Split(string, delimiter, /*splits*/ nullptr, /*count*/ 0, mode);
    result.data                         = Dqn_Arena_NewArray(arena, Dqn_Str8, splits_required, Dqn_ZeroMem_No);
    if (result.data) {
        result.size = Dqn_Str8_Split(string, delimiter, result.data, splits_required, mode);
        DQN_ASSERT(splits_required == result.size);
    }
    return result;
}

DQN_API Dqn_Str8FindResult Dqn_Str8_FindStr8Array(Dqn_Str8 string, Dqn_Str8 const *find, Dqn_usize find_size, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8FindResult result = {};
    if (!Dqn_Str8_HasData(string) || !find || find_size == 0)
        return result;

    for (Dqn_usize index = 0; !result.found && index < string.size; index++) {
        for (Dqn_usize find_index = 0; find_index < find_size; find_index++) {
            Dqn_Str8 find_item    = find[find_index];
            Dqn_Str8 string_slice = Dqn_Str8_Slice(string, index, find_item.size);
            if (Dqn_Str8_Eq(string_slice, find_item, eq_case)) {
                result.found                  = true;
                result.index                  = index;
                result.start_to_before_match  = Dqn_Str8_Init(string.data, index);
                result.match                  = Dqn_Str8_Init(string.data + index, find_item.size);
                result.match_to_end_of_buffer = Dqn_Str8_Init(result.match.data, string.size - index);
                break;
            }
        }
    }
    return result;
}

DQN_API Dqn_Str8FindResult Dqn_Str8_FindStr8(Dqn_Str8 string, Dqn_Str8 find, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8FindResult result = Dqn_Str8_FindStr8Array(string, &find, 1, eq_case);
    return result;
}

DQN_API Dqn_Str8FindResult Dqn_Str8_Find(Dqn_Str8 string, uint32_t flags)
{
    Dqn_Str8FindResult result = {};
    for (size_t index = 0; !result.found && index < string.size; index++) {
        result.found |= ((flags & Dqn_Str8FindFlag_Digit)      && Dqn_Char_IsDigit(string.data[index]));
        result.found |= ((flags & Dqn_Str8FindFlag_Alphabet)   && Dqn_Char_IsAlphabet(string.data[index]));
        result.found |= ((flags & Dqn_Str8FindFlag_Whitespace) && Dqn_Char_IsWhitespace(string.data[index]));
        result.found |= ((flags & Dqn_Str8FindFlag_Plus)       && string.data[index] == '+');
        result.found |= ((flags & Dqn_Str8FindFlag_Minus)      && string.data[index] == '-');
        if (result.found) {
            result.index                  = index;
            result.match                  = Dqn_Str8_Init(string.data + index, 1);
            result.match_to_end_of_buffer = Dqn_Str8_Init(result.match.data, string.size - index);
        }
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Segment(Dqn_Arena *arena, Dqn_Str8 src, Dqn_usize segment_size, char segment_char)
{
    if (!segment_size || !Dqn_Str8_HasData(src)) {
        Dqn_Str8 result = Dqn_Str8_Copy(arena, src);
        return result;
    }

    Dqn_usize segments = src.size / segment_size;
    if (src.size % segment_size == 0)
        segments--;

    Dqn_usize segment_counter = 0;
    Dqn_Str8  result          = Dqn_Str8_Alloc(arena, src.size + segments, Dqn_ZeroMem_Yes);
    Dqn_usize write_index     = 0;
    DQN_FOR_UINDEX(src_index, src.size) {
        result.data[write_index++] = src.data[src_index];
        if ((src_index + 1) % segment_size == 0 && segment_counter < segments) {
            result.data[write_index++] = segment_char;
            segment_counter++;
        }
        DQN_ASSERTF(write_index <= result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
    }

    DQN_ASSERTF(write_index == result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_ReverseSegment(Dqn_Arena *arena, Dqn_Str8 src, Dqn_usize segment_size, char segment_char)
{
    if (!segment_size || !Dqn_Str8_HasData(src)) {
        Dqn_Str8 result = Dqn_Str8_Copy(arena, src);
        return result;
    }

    Dqn_usize segments = src.size / segment_size;
    if (src.size % segment_size == 0)
        segments--;

    Dqn_usize write_counter   = 0;
    Dqn_usize segment_counter = 0;
    Dqn_Str8  result      = Dqn_Str8_Alloc(arena, src.size + segments, Dqn_ZeroMem_Yes);
    Dqn_usize write_index = result.size - 1;


    DQN_MSVC_WARNING_PUSH
    DQN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
    for (size_t src_index = src.size - 1; src_index < src.size; src_index--) {
    DQN_MSVC_WARNING_POP
        result.data[write_index--] = src.data[src_index];
        if (++write_counter % segment_size == 0 && segment_counter < segments) {
            result.data[write_index--] = segment_char;
            segment_counter++;
        }
    }

    DQN_ASSERT(write_index == SIZE_MAX);
    return result;
}


DQN_API bool Dqn_Str8_Eq(Dqn_Str8 lhs, Dqn_Str8 rhs, Dqn_Str8EqCase eq_case)
{
    if (lhs.size != rhs.size)
        return false;

    if (lhs.size == 0)
        return true;

    if (!lhs.data || !rhs.data)
        return false;

    bool result = true;
    switch (eq_case) {
        case Dqn_Str8EqCase_Sensitive: {
            result = (DQN_MEMCMP(lhs.data, rhs.data, lhs.size) == 0);
        } break;

        case Dqn_Str8EqCase_Insensitive: {
            for (Dqn_usize index = 0; index < lhs.size && result; index++)
                result = (Dqn_Char_ToLower(lhs.data[index]) == Dqn_Char_ToLower(rhs.data[index]));
        } break;
    }
    return result;
}

DQN_API bool Dqn_Str8_EqInsensitive(Dqn_Str8 lhs, Dqn_Str8 rhs)
{
    bool result = Dqn_Str8_Eq(lhs, rhs, Dqn_Str8EqCase_Insensitive);
    return result;
}

DQN_API bool Dqn_Str8_StartsWith(Dqn_Str8 string, Dqn_Str8 prefix, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 substring = {string.data, DQN_MIN(prefix.size, string.size)};
    bool     result    = Dqn_Str8_Eq(substring, prefix, eq_case);
    return result;
}

DQN_API bool Dqn_Str8_StartsWithInsensitive(Dqn_Str8 string, Dqn_Str8 prefix)
{
    bool result = Dqn_Str8_StartsWith(string, prefix, Dqn_Str8EqCase_Insensitive);
    return result;
}

DQN_API bool Dqn_Str8_EndsWith(Dqn_Str8 string, Dqn_Str8 suffix, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 substring = {string.data + string.size - suffix.size, DQN_MIN(string.size, suffix.size)};
    bool     result    = Dqn_Str8_Eq(substring, suffix, eq_case);
    return result;
}

DQN_API bool Dqn_Str8_EndsWithInsensitive(Dqn_Str8 string, Dqn_Str8 suffix)
{
    bool result = Dqn_Str8_EndsWith(string, suffix, Dqn_Str8EqCase_Insensitive);
    return result;
}

DQN_API bool Dqn_Str8_HasChar(Dqn_Str8 string, char ch)
{
    bool result = false;
    for (Dqn_usize index = 0; !result && index < string.size; index++)
        result = string.data[index] == ch;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimPrefix(Dqn_Str8 string, Dqn_Str8 prefix, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 result = string;
    if (Dqn_Str8_StartsWith(string, prefix, eq_case)) {
        result.data += prefix.size;
        result.size -= prefix.size;
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimHexPrefix(Dqn_Str8 string)
{
    Dqn_Str8 result = Dqn_Str8_TrimPrefix(string, DQN_STR8("0x"), Dqn_Str8EqCase_Insensitive);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimSuffix(Dqn_Str8 string, Dqn_Str8 suffix, Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 result = string;
    if (Dqn_Str8_EndsWith(string, suffix, eq_case))
        result.size -= suffix.size;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimAround(Dqn_Str8 string, Dqn_Str8 trim_string)
{
    Dqn_Str8 result = Dqn_Str8_TrimPrefix(string, trim_string);
    result          = Dqn_Str8_TrimSuffix(result, trim_string);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimWhitespaceAround(Dqn_Str8 string)
{
    Dqn_Str8 result = string;
    if (!Dqn_Str8_HasData(string))
        return result;

    char const *start = string.data;
    char const *end   = string.data + string.size;
    while (start < end && Dqn_Char_IsWhitespace(start[0]))
        start++;

    while (end > start && Dqn_Char_IsWhitespace(end[-1]))
        end--;

    result = Dqn_Str8_Init(start, end - start);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_TrimByteOrderMark(Dqn_Str8 string)
{
    Dqn_Str8 result = string;
    if (!Dqn_Str8_HasData(result))
        return result;

    // TODO(dqn): This is little endian
    Dqn_Str8 UTF8_BOM     = DQN_STR8("\xEF\xBB\xBF");
    Dqn_Str8 UTF16_BOM_BE = DQN_STR8("\xEF\xFF");
    Dqn_Str8 UTF16_BOM_LE = DQN_STR8("\xFF\xEF");
    Dqn_Str8 UTF32_BOM_BE = DQN_STR8("\x00\x00\xFE\xFF");
    Dqn_Str8 UTF32_BOM_LE = DQN_STR8("\xFF\xFE\x00\x00");

    result = Dqn_Str8_TrimPrefix(result, UTF8_BOM,     Dqn_Str8EqCase_Sensitive);
    result = Dqn_Str8_TrimPrefix(result, UTF16_BOM_BE, Dqn_Str8EqCase_Sensitive);
    result = Dqn_Str8_TrimPrefix(result, UTF16_BOM_LE, Dqn_Str8EqCase_Sensitive);
    result = Dqn_Str8_TrimPrefix(result, UTF32_BOM_BE, Dqn_Str8EqCase_Sensitive);
    result = Dqn_Str8_TrimPrefix(result, UTF32_BOM_LE, Dqn_Str8EqCase_Sensitive);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_FileNameFromPath(Dqn_Str8 path)
{
    Dqn_Str8 separators[]           = {DQN_STR8("/"), DQN_STR8("\\")};
    Dqn_Str8BinarySplitResult split = Dqn_Str8_BinarySplitLastArray(path, separators, DQN_ARRAY_UCOUNT(separators));
    Dqn_Str8 result                 = Dqn_Str8_HasData(split.rhs) ? split.rhs : split.lhs;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_FileNameNoExtension(Dqn_Str8 path)
{
    Dqn_Str8 file_name = Dqn_Str8_FileNameFromPath(path);
    Dqn_Str8 result    = Dqn_Str8_FilePathNoExtension(file_name);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_FilePathNoExtension(Dqn_Str8 path)
{
    Dqn_Str8BinarySplitResult split = Dqn_Str8_BinarySplitLast(path, DQN_STR8("."));
    Dqn_Str8 result                 = split.lhs;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_FileExtension(Dqn_Str8 path)
{
    Dqn_Str8BinarySplitResult split = Dqn_Str8_BinarySplitLast(path, DQN_STR8("."));
    Dqn_Str8 result                 = split.rhs;
    return result;
}

DQN_API Dqn_Str8ToU64Result Dqn_Str8_ToU64(Dqn_Str8 string, char separator)
{
    // NOTE: Argument check
    Dqn_Str8ToU64Result result = {};
    if (!Dqn_Str8_HasData(string)) {
        result.success = true;
        return result;
    }

    // NOTE: Sanitize input/output
    Dqn_Str8 trim_string = Dqn_Str8_TrimWhitespaceAround(string);
    if (trim_string.size == 0) {
        result.success = true;
        return result;
    }

    // NOTE: Handle prefix '+'
    Dqn_usize start_index = 0;
    if (!Dqn_Char_IsDigit(trim_string.data[0])) {
        if (trim_string.data[0] != '+')
            return result;
        start_index++;
    }

    // NOTE: Convert the string number to the binary number
    for (Dqn_usize index = start_index; index < trim_string.size; index++) {
        char ch = trim_string.data[index];
        if (index) {
            if (separator != 0 && ch == separator)
                continue;
        }

        if (!Dqn_Char_IsDigit(ch))
            return result;

        result.value   = Dqn_Safe_MulU64(result.value, 10);
        uint64_t digit = ch - '0';
        result.value   = Dqn_Safe_AddU64(result.value, digit);
    }

    result.success = true;
    return result;
}

DQN_API Dqn_Str8ToI64Result Dqn_Str8_ToI64(Dqn_Str8 string, char separator)
{
    // NOTE: Argument check
    Dqn_Str8ToI64Result result = {};
    if (!Dqn_Str8_HasData(string)) {
        result.success = true;
        return result;
    }

    // NOTE: Sanitize input/output
    Dqn_Str8 trim_string = Dqn_Str8_TrimWhitespaceAround(string);
    if (trim_string.size == 0) {
        result.success = true;
        return result;
    }

    bool negative         = false;
    Dqn_usize start_index = 0;
    if (!Dqn_Char_IsDigit(trim_string.data[0])) {
        negative = (trim_string.data[start_index] == '-');
        if (!negative && trim_string.data[0] != '+')
            return result;
        start_index++;
    }

    // NOTE: Convert the string number to the binary number
    for (Dqn_usize index = start_index; index < trim_string.size; index++) {
        char ch = trim_string.data[index];
        if (index) {
            if (separator != 0 && ch == separator)
                continue;
        }

        if (!Dqn_Char_IsDigit(ch))
            return result;

        result.value   = Dqn_Safe_MulU64(result.value, 10);
        uint64_t digit = ch - '0';
        result.value   = Dqn_Safe_AddU64(result.value, digit);
    }

    if (negative)
        result.value *= -1;

    result.success = true;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Replace(Dqn_Str8       string,
                                  Dqn_Str8       find,
                                  Dqn_Str8       replace,
                                  Dqn_usize      start_index,
                                  Dqn_Arena     *arena,
                                  Dqn_Str8EqCase eq_case)
{
    Dqn_Str8 result = {};
    if (!Dqn_Str8_HasData(string) || !Dqn_Str8_HasData(find) || find.size > string.size || find.size == 0 || string.size == 0) {
        result = Dqn_Str8_Copy(arena, string);
        return result;
    }

    Dqn_TLSTMem     tmem           = Dqn_TLS_TMem(arena);
    Dqn_Str8Builder string_builder = Dqn_Str8Builder_Init(tmem.arena);
    Dqn_usize       max            = string.size - find.size;
    Dqn_usize       head           = start_index;

    for (Dqn_usize tail = head; tail <= max; tail++) {
        Dqn_Str8 check = Dqn_Str8_Slice(string, tail, find.size);
        if (!Dqn_Str8_Eq(check, find, eq_case))
            continue;

        if (start_index > 0 && string_builder.string_size == 0) {
            // User provided a hint in the string to start searching from, we
            // need to add the string up to the hint. We only do this if there's
            // a replacement action, otherwise we have a special case for no
            // replacements, where the entire string gets copied.
            Dqn_Str8 slice = Dqn_Str8_Init(string.data, head);
            Dqn_Str8Builder_AddRef(&string_builder, slice);
        }

        Dqn_Str8 range = Dqn_Str8_Slice(string, head, (tail - head));
        Dqn_Str8Builder_AddRef(&string_builder, range);
        Dqn_Str8Builder_AddRef(&string_builder, replace);
        head = tail + find.size;
        tail += find.size - 1; // NOTE: -1 since the for loop will post increment us past the end of the find string
    }

    if (string_builder.string_size == 0) {
        // NOTE: No replacement possible, so we just do a full-copy
        result = Dqn_Str8_Copy(arena, string);
    } else {
        Dqn_Str8 remainder = Dqn_Str8_Init(string.data + head, string.size - head);
        Dqn_Str8Builder_AddRef(&string_builder, remainder);
        result = Dqn_Str8Builder_Build(&string_builder, arena);
    }

    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_ReplaceInsensitive(Dqn_Str8 string, Dqn_Str8 find, Dqn_Str8 replace, Dqn_usize start_index, Dqn_Arena *arena)
{
    Dqn_Str8 result = Dqn_Str8_Replace(string, find, replace, start_index, arena, Dqn_Str8EqCase_Insensitive);
    return result;
}

DQN_API void Dqn_Str8_Remove(Dqn_Str8 *string, Dqn_usize offset, Dqn_usize size)
{
    if (!string || !Dqn_Str8_HasData(*string))
        return;

    char *end               = string->data + string->size;
    char *dest              = DQN_MIN(string->data + offset,        end);
    char *src               = DQN_MIN(string->data + offset + size, end);
    Dqn_usize bytes_to_move = end - src;
    DQN_MEMMOVE(dest, src, bytes_to_move);
    string->size -= bytes_to_move;
}

#if defined(__cplusplus)
DQN_API bool operator==(Dqn_Str8 const &lhs, Dqn_Str8 const &rhs)
{
    bool result = Dqn_Str8_Eq(lhs, rhs, Dqn_Str8EqCase_Sensitive);
    return result;
}

DQN_API bool operator!=(Dqn_Str8 const &lhs, Dqn_Str8 const &rhs)
{
    bool result = !(lhs == rhs);
    return result;
}
#endif

DQN_API Dqn_Str8 Dqn_Str8_InitF(Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    Dqn_Str8 result = Dqn_Str8_InitFV(arena, fmt, va);
    va_end(va);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_InitFV(Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Str8 result = {};
    if (!fmt)
        return result;

    Dqn_usize size = Dqn_CStr8_FVSize(fmt, args);
    if (size) {
        result = Dqn_Str8_Alloc(arena, size, Dqn_ZeroMem_No);
        if (Dqn_Str8_HasData(result))
            DQN_VSNPRINTF(result.data, Dqn_Safe_SaturateCastISizeToInt(size + 1 /*null-terminator*/), fmt, args);
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Alloc(Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem)
{
    Dqn_Str8 result = {};
    result.data     = Dqn_Arena_NewArray(arena, char, size + 1, zero_mem);
    if (result.data)
        result.size = size;
    if (zero_mem == Dqn_ZeroMem_No)
        result.data[result.size] = 0;
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_CopyCString(Dqn_Arena *arena, char const *string, Dqn_usize size)
{
    Dqn_Str8 result = {};
    if (!string)
        return result;

    result = Dqn_Str8_Alloc(arena, size, Dqn_ZeroMem_No);
    if (Dqn_Str8_HasData(result)) {
        DQN_MEMCPY(result.data, string, size);
        result.data[size] = 0;
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8_Copy(Dqn_Arena *arena, Dqn_Str8 string)
{
    Dqn_Str8 result = Dqn_Str8_CopyCString(arena, string.data, string.size);
    return result;
}

// NOTE: [$STRB] Dqn_Str8Builder ////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8Builder Dqn_Str8Builder_Init(Dqn_Arena *arena)
{
    Dqn_Str8Builder result = {};
    result.arena           = arena;
    return result;
}

DQN_API Dqn_Str8Builder Dqn_Str8Builder_InitArrayRef(Dqn_Arena      *arena,
                                                     Dqn_Str8 const *strings,
                                                     Dqn_usize       size)
{
    Dqn_Str8Builder result = Dqn_Str8Builder_Init(arena);
    Dqn_Str8Builder_AddArrayRef(&result, strings, size);
    return result;
}

DQN_API Dqn_Str8Builder Dqn_Str8Builder_InitArrayCopy(Dqn_Arena      *arena,
                                                      Dqn_Str8 const *strings,
                                                      Dqn_usize       size)
{
    Dqn_Str8Builder result = Dqn_Str8Builder_Init(arena);
    Dqn_Str8Builder_AddArrayCopy(&result, strings, size);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddArrayRef(Dqn_Str8Builder *builder, Dqn_Str8 const *strings, Dqn_usize size)
{
    if (!builder)
        return false;

    if (!strings || size <= 0)
        return true;

    Dqn_Str8Link *links = Dqn_Arena_NewArray(builder->arena, Dqn_Str8Link, size, Dqn_ZeroMem_No);
    if (!links)
        return false;

    DQN_FOR_UINDEX(index, size) {
        Dqn_Str8      string = strings[index];
        Dqn_Str8Link *link   = links + index;

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

    return true;
}

DQN_API bool Dqn_Str8Builder_AddArrayCopy(Dqn_Str8Builder *builder, Dqn_Str8 const *strings, Dqn_usize size)
{
    if (!builder)
        return false;

    if (!strings || size <= 0)
        return true;

    Dqn_ArenaTempMem tmp_mem = Dqn_Arena_TempMemBegin(builder->arena);
    bool             result  = true;
    Dqn_Str8 *strings_copy   = Dqn_Arena_NewArray(builder->arena, Dqn_Str8, size, Dqn_ZeroMem_No);
    DQN_FOR_UINDEX (index, size) {
        strings_copy[index] = Dqn_Str8_Copy(builder->arena, strings[index]);
        if (strings_copy[index].size != strings[index].size) {
            result = false;
            break;
        }
    }

    if (result)
        result = Dqn_Str8Builder_AddArrayRef(builder, strings_copy, size);

    if (!result)
        Dqn_Arena_TempMemEnd(tmp_mem);

    return result;
}

DQN_API bool Dqn_Str8Builder_AddSliceRef(Dqn_Str8Builder *builder, Dqn_Slice<Dqn_Str8> array)
{
    bool result = Dqn_Str8Builder_AddArrayRef(builder, array.data, array.size);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddSliceCopy(Dqn_Str8Builder *builder, Dqn_Slice<Dqn_Str8> array)
{
    bool result = Dqn_Str8Builder_AddArrayCopy(builder, array.data, array.size);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddRef(Dqn_Str8Builder *builder, Dqn_Str8 string)
{
    bool result = Dqn_Str8Builder_AddArrayRef(builder, &string, 1);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddCopy(Dqn_Str8Builder *builder, Dqn_Str8 string)
{
    bool result = Dqn_Str8Builder_AddArrayCopy(builder, &string, 1);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddFV(Dqn_Str8Builder *builder, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Str8         string   = Dqn_Str8_InitFV(builder->arena, fmt, args);
    Dqn_ArenaTempMem temp_mem = Dqn_Arena_TempMemBegin(builder->arena);
    bool result = Dqn_Str8Builder_AddRef(builder, string);
    if (!result)
        Dqn_Arena_TempMemEnd(temp_mem);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddF(Dqn_Str8Builder *builder, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = Dqn_Str8Builder_AddFV(builder, fmt, args);
    va_end(args);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddBytesRef(Dqn_Str8Builder *builder, void const *ptr, Dqn_usize size)
{
    Dqn_Str8 input  = Dqn_Str8_Init(ptr, size);
    bool     result = Dqn_Str8Builder_AddRef(builder, input);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddBytesCopy(Dqn_Str8Builder *builder, void const *ptr, Dqn_usize size)
{
    Dqn_Str8 input  = Dqn_Str8_Init(ptr, size);
    bool     result = Dqn_Str8Builder_AddCopy(builder, input);
    return result;
}

static bool Dqn_Str8Builder_AddBuilder_(Dqn_Str8Builder *dest, Dqn_Str8Builder const *src, bool copy)
{
    if (!dest)
        return false;
    if (!src)
        return true;

    Dqn_Arena_TempMemBegin(dest->arena);
    Dqn_Str8Link *links = Dqn_Arena_NewArray(dest->arena, Dqn_Str8Link, src->count, Dqn_ZeroMem_No);
    if (!links)
        return false;

    Dqn_Str8Link *first      = nullptr;
    Dqn_Str8Link *last       = nullptr;
    Dqn_usize     link_index = 0;
    bool          result     = true;
    for (Dqn_Str8Link const *it = src->head; it; it = it->next) {
        Dqn_Str8Link *link = links + link_index++;
        link->next         = nullptr;
        link->string       = it->string;

        if (copy) {
            link->string = Dqn_Str8_Copy(dest->arena, it->string);
            if (link->string.size != it->string.size) {
                result = false;
                break;
            }
        }

        if (last) {
            last->next = link;
        } else {
            first = link;
        }
        last = link;
    }

    if (result) {
        if (dest->head)
            dest->tail->next = first;
        else
            dest->head = first;
        dest->tail        = last;
        dest->count       += src->count;
        dest->string_size += src->string_size;
    }
    return true;
}

DQN_API bool Dqn_Str8Builder_AddBuilderRef(Dqn_Str8Builder *dest, Dqn_Str8Builder const *src)
{
    bool result = Dqn_Str8Builder_AddBuilder_(dest, src, false);
    return result;
}

DQN_API bool Dqn_Str8Builder_AddBuilderCopy(Dqn_Str8Builder *dest, Dqn_Str8Builder const *src)
{
    bool result = Dqn_Str8Builder_AddBuilder_(dest, src, true);
    return result;
}

DQN_API bool Dqn_Str8Builder_Erase(Dqn_Str8Builder *builder, Dqn_Str8 string)
{
    for (Dqn_Str8Link **it = &builder->head; *it; it = &((*it)->next)) {
        if ((*it)->string == string) {
            *it = (*it)->next;
            builder->string_size -= string.size;
            builder->count       -= 1;
            return true;
        }
    }
    return false;
}

DQN_API Dqn_Str8Builder Dqn_Str8Builder_Copy(Dqn_Arena *arena, Dqn_Str8Builder const *builder)
{
    Dqn_Str8Builder result = Dqn_Str8Builder_Init(arena);
    Dqn_Str8Builder_AddBuilderCopy(&result, builder);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8Builder_Build(Dqn_Str8Builder const *builder, Dqn_Arena *arena)
{
    Dqn_Str8 result = Dqn_Str8Builder_BuildDelimited(builder, DQN_STR8(""), arena);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8Builder_BuildDelimited(Dqn_Str8Builder const *builder, Dqn_Str8 delimiter, Dqn_Arena *arena)
{
    Dqn_Str8 result = DQN_ZERO_INIT;
    if (!builder || builder->string_size <= 0 || builder->count <= 0)
        return result;

    Dqn_usize size_for_delimiter = Dqn_Str8_HasData(delimiter) ? ((builder->count - 1) * delimiter.size) : 0;
    result.data = Dqn_Arena_NewArray(arena,
                                     char,
                                     builder->string_size + size_for_delimiter + 1 /*null terminator*/,
                                     Dqn_ZeroMem_No);
    if (!result.data)
        return result;

    for (Dqn_Str8Link *link = builder->head; link; link = link->next) {
        DQN_MEMCPY(result.data + result.size, link->string.data, link->string.size);
        result.size += link->string.size;
        if (link->next && Dqn_Str8_HasData(delimiter)) {
            DQN_MEMCPY(result.data + result.size, delimiter.data, delimiter.size);
            result.size += delimiter.size;
        }
    }

    result.data[result.size] = 0;
    DQN_ASSERT(result.size == builder->string_size + size_for_delimiter);
    return result;
}

DQN_API Dqn_Str8 Dqn_Str8Builder_BuildCRT(Dqn_Str8Builder const *builder)
{
    Dqn_Str8 result = DQN_ZERO_INIT;
    if (!builder || builder->string_size <= 0 || builder->count <= 0)
        return result;

    result.data = DQN_CAST(char *)malloc(builder->string_size + 1);
    if (!result.data)
        return result;

    for (Dqn_Str8Link *link = builder->head; link; link = link->next) {
        DQN_MEMCPY(result.data + result.size, link->string.data, link->string.size);
        result.size += link->string.size;
    }

    result.data[result.size] = 0;
    DQN_ASSERT(result.size == builder->string_size);
    return result;
}

DQN_API Dqn_Slice<Dqn_Str8> Dqn_Str8Builder_BuildSlice(Dqn_Str8Builder const *builder, Dqn_Arena *arena)
{
    Dqn_Slice<Dqn_Str8> result = DQN_ZERO_INIT;
    if (!builder || builder->string_size <= 0 || builder->count <= 0)
        return result;

    result = Dqn_Slice_Alloc<Dqn_Str8>(arena, builder->count, Dqn_ZeroMem_No);
    if (!result.data)
        return result;

    Dqn_usize slice_index   = 0;
    for (Dqn_Str8Link *link = builder->head; link; link = link->next)
        result.data[slice_index++] = Dqn_Str8_Copy(arena, link->string);

    DQN_ASSERT(slice_index == builder->count);
    return result;
}

DQN_API void Dqn_Str8Builder_Print(Dqn_Str8Builder const *builder)
{
    for (Dqn_Str8Link *link = builder ? builder->head : nullptr; link; link = link->next)
        Dqn_Print(link->string);
}

DQN_API void Dqn_Str8Builder_PrintLn(Dqn_Str8Builder const *builder)
{
    for (Dqn_Str8Link *link = builder ? builder->head : nullptr; link; link = link->next) {
        if (link->next) {
            Dqn_Print(link->string);
        } else {
            Dqn_Print_Ln(link->string);
        }
    }
}

// NOTE: [$CHAR] Dqn_Char //////////////////////////////////////////////////////////////////////////
DQN_API bool Dqn_Char_IsAlphabet(char ch)
{
    bool result = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    return result;
}

DQN_API bool Dqn_Char_IsDigit(char ch)
{
    bool result = (ch >= '0' && ch <= '9');
    return result;
}

DQN_API bool Dqn_Char_IsAlphaNum(char ch)
{
    bool result = Dqn_Char_IsAlphabet(ch) || Dqn_Char_IsDigit(ch);
    return result;
}

DQN_API bool Dqn_Char_IsWhitespace(char ch)
{
    bool result = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
    return result;
}

DQN_API bool Dqn_Char_IsHex(char ch)
{
    bool result = ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || (ch >= '0' && ch <= '9'));
    return result;
}

DQN_API Dqn_CharHexToU8 Dqn_Char_HexToU8(char ch)
{
    Dqn_CharHexToU8 result = {};
    result.success         = true;
    if (ch >= 'a' && ch <= 'f')
        result.value = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        result.value = ch - 'A' + 10;
    else if (ch >= '0' && ch <= '9')
        result.value = ch - '0';
    else
        result.success = false;
    return result;
}

static char constexpr DQN_HEX_LUT[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
DQN_API char Dqn_Char_ToHex(char ch)
{
    char result = DQN_CAST(char)-1;
    if (ch < 16)
        result = DQN_HEX_LUT[DQN_CAST(uint8_t)ch];
    return result;
}

DQN_API char Dqn_Char_ToHexUnchecked(char ch)
{
    char result = DQN_HEX_LUT[DQN_CAST(uint8_t)ch];
    return result;
}

DQN_API char Dqn_Char_ToLower(char ch)
{
    char result = ch;
    if (result >= 'A' && result <= 'Z')
        result += 'a' - 'A';
    return result;
}

// NOTE: [$UTFX] Dqn_UTF ///////////////////////////////////////////////////////////////////////////
DQN_API int Dqn_UTF8_EncodeCodepoint(uint8_t utf8[4], uint32_t codepoint)
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
        utf8[0] = DQN_CAST(uint8_t) codepoint;
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

DQN_API int Dqn_UTF16_EncodeCodepoint(uint16_t utf16[2], uint32_t codepoint)
{
    // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
    // ----------------------------------------+------------------------------------+------------------+
    // UTF-16 (binary)                         | Code point (binary)                | Range            |
    // ----------------------------------------+------------------------------------+------------------+
    // xxxx'xxxx'xxxx'xxxx                     | xxxx'xxxx'xxxx'xxxx                | U+0000–U+FFFF    |
    // 1101'10xx'xxxx'xxxx 1101'11yy'yyyy'yyyy | xxxx'xxxx'xxyy'yyyy'yyyy + 0x10000 | U+10000–U+10FFFF |
    // ----------------------------------------+------------------------------------+------------------+

    if (codepoint <= 0b1111'1111'1111'1111) {
        utf16[0] = DQN_CAST(uint16_t) codepoint;
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
