#pragma once
#include "dqn.h"
#include <immintrin.h>

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//     /$$$$$$  /$$    /$$ /$$   /$$        /$$$$$$$   /$$    /$$$$$$  /$$$$$$$$
//    /$$__  $$| $$   | $$| $$  / $$       | $$____/ /$$$$   /$$__  $$| $$_____/
//   | $$  \ $$| $$   | $$|  $$/ $$/       | $$     |_  $$  |__/  \ $$| $$
//   | $$$$$$$$|  $$ / $$/ \  $$$$/ /$$$$$$| $$$$$$$  | $$    /$$$$$$/| $$$$$
//   | $$__  $$ \  $$ $$/   >$$  $$|______/|_____  $$ | $$   /$$____/ | $$__/
//   | $$  | $$  \  $$$/   /$$/\  $$        /$$  \ $$ | $$  | $$      | $$
//   | $$  | $$   \  $/   | $$  \ $$       |  $$$$$$//$$$$$$| $$$$$$$$| $$
//   |__/  |__/    \_/    |__/  |__/        \______/|______/|________/|__/
//
//   dqn_avx512f.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_API DN_Str8FindResult DN_Str8_FindStr8AVX512F(DN_Str8 string, DN_Str8 find)
{
    // NOTE: Algorithm as described in http://0x80.pl/articles/simd-strfind.html
    DN_Str8FindResult result = {};
    if (!DN_Str8_HasData(string) || !DN_Str8_HasData(find) || find.size > string.size)
        return result;

    __m512i const   find_first_ch   = _mm512_set1_epi8(find.data[0]);
    __m512i const   find_last_ch    = _mm512_set1_epi8(find.data[find.size - 1]);

    DN_USize const search_size     = string.size - find.size;
    DN_USize       simd_iterations = search_size / sizeof(__m512i);
    char const    *ptr             = string.data;

    while (simd_iterations--) {
        __m512i find_first_ch_block  = _mm512_loadu_si512(ptr);
        __m512i find_last_ch_block   = _mm512_loadu_si512(ptr + find.size - 1);

        // NOTE: AVX512F does not have a cmpeq so we use XOR to place a 0 bit
        // where matches are found.
        __m512i first_ch_matches     = _mm512_xor_si512(find_first_ch_block, find_first_ch);

        // NOTE: We can combine the 2nd XOR and merge the 2 XOR results into one
        // operation using the ternarylogic intrinsic.
        //
        // A = first_ch_matches (find_first_ch_block ^ find_first_ch)
        // B = find_last_ch_block
        // C = find_last_ch
        //
        // ternarylogic op => A | (B ^ C) => 0b1111'0110 => 0xf6
        //
        // / A / B / C / B ^ C / A | (B ^ C) /
        // | 0 | 0 | 0 | 0     | 0           |
        // | 0 | 0 | 1 | 1     | 1           |
        // | 0 | 1 | 0 | 1     | 1           |
        // | 0 | 1 | 1 | 0     | 0           |
        // | 1 | 0 | 0 | 0     | 1           |
        // | 1 | 0 | 1 | 1     | 1           |
        // | 1 | 1 | 0 | 1     | 1           |
        // | 1 | 1 | 1 | 0     | 1           |

        __m512i   ch_matches     = _mm512_ternarylogic_epi32(first_ch_matches, find_last_ch_block, find_last_ch, 0xf6);

        // NOTE: Matches were XOR-ed and are hence indicated as zero so we mask
        // out which 32 bit elements in the vector had zero bytes. This uses a
        // bit twiddling trick
        // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
        __mmask16 zero_byte_mask = {};
        {
            const __m512i v01  = _mm512_set1_epi32(0x01010101u);
            const __m512i v80  = _mm512_set1_epi32(0x80808080u);
            const __m512i v1   = _mm512_sub_epi32(ch_matches, v01);
            const __m512i tmp1 = _mm512_ternarylogic_epi32(v1, ch_matches, v80, 0x20);
            zero_byte_mask     = _mm512_test_epi32_mask(tmp1, tmp1);
        }

        while (zero_byte_mask) {
            uint64_t const lsb_zero_pos = _tzcnt_u64(zero_byte_mask);
            char const    *base_ptr     = ptr + (4 * lsb_zero_pos);

            if (DN_MEMCMP(base_ptr + 0, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data;
            } else if (DN_MEMCMP(base_ptr + 1, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 1;
            } else if (DN_MEMCMP(base_ptr + 2, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 2;
            } else if (DN_MEMCMP(base_ptr + 3, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 3;
            }

            if (result.found) {
                result.start_to_before_match        = DN_Str8_Init(string.data, result.index);
                result.match                        = DN_Str8_Init(string.data + result.index, find.size);
                result.match_to_end_of_buffer       = DN_Str8_Init(result.match.data, string.size - result.index);
                result.after_match_to_end_of_buffer = DN_Str8_Advance(result.match_to_end_of_buffer, find.size);
                return result;
            }

            zero_byte_mask = DN_Bit_ClearNextLSB(zero_byte_mask);
        }

        ptr += sizeof(__m512i);
    }

    for (DN_USize index = ptr - string.data; index < string.size; index++) {
        DN_Str8 string_slice = DN_Str8_Slice(string, index, find.size);
        if (DN_Str8_Eq(string_slice, find)) {
            result.found                        = true;
            result.index                        = index;
            result.start_to_before_match        = DN_Str8_Init(string.data, index);
            result.match                        = DN_Str8_Init(string.data + index, find.size);
            result.match_to_end_of_buffer       = DN_Str8_Init(result.match.data, string.size - index);
            result.after_match_to_end_of_buffer = DN_Str8_Advance(result.match_to_end_of_buffer, find.size);
            return result;
        }
    }

    return result;
}

DN_API DN_Str8FindResult DN_Str8_FindLastStr8AVX512F(DN_Str8 string, DN_Str8 find)
{
    // NOTE: Algorithm as described in http://0x80.pl/articles/simd-strfind.html
    DN_Str8FindResult result = {};
    if (!DN_Str8_HasData(string) || !DN_Str8_HasData(find) || find.size > string.size)
        return result;

    __m512i const   find_first_ch   = _mm512_set1_epi8(find.data[0]);
    __m512i const   find_last_ch    = _mm512_set1_epi8(find.data[find.size - 1]);

    DN_USize const search_size     = string.size - find.size;
    DN_USize       simd_iterations = search_size / sizeof(__m512i);
    char const     *ptr             = string.data + search_size + 1;

    while (simd_iterations--) {
        ptr                         -= sizeof(__m512i);
        __m512i find_first_ch_block  = _mm512_loadu_si512(ptr);
        __m512i find_last_ch_block   = _mm512_loadu_si512(ptr + find.size - 1);

        // NOTE: AVX512F does not have a cmpeq so we use XOR to place a 0 bit
        // where matches are found.
        __m512i first_ch_matches     = _mm512_xor_si512(find_first_ch_block, find_first_ch);

        // NOTE: We can combine the 2nd XOR and merge the 2 XOR results into one
        // operation using the ternarylogic intrinsic.
        //
        // A = first_ch_matches (find_first_ch_block ^ find_first_ch)
        // B = find_last_ch_block
        // C = find_last_ch
        //
        // ternarylogic op => A | (B ^ C) => 0b1111'0110 => 0xf6
        //
        // / A / B / C / B ^ C / A | (B ^ C) /
        // | 0 | 0 | 0 | 0     | 0           |
        // | 0 | 0 | 1 | 1     | 1           |
        // | 0 | 1 | 0 | 1     | 1           |
        // | 0 | 1 | 1 | 0     | 0           |
        // | 1 | 0 | 0 | 0     | 1           |
        // | 1 | 0 | 1 | 1     | 1           |
        // | 1 | 1 | 0 | 1     | 1           |
        // | 1 | 1 | 1 | 0     | 1           |

        __m512i   ch_matches     = _mm512_ternarylogic_epi32(first_ch_matches, find_last_ch_block, find_last_ch, 0xf6);

        // NOTE: Matches were XOR-ed and are hence indicated as zero so we mask
        // out which 32 bit elements in the vector had zero bytes. This uses a
        // bit twiddling trick
        // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
        __mmask16 zero_byte_mask = {};
        {
            const __m512i v01  = _mm512_set1_epi32(0x01010101u);
            const __m512i v80  = _mm512_set1_epi32(0x80808080u);
            const __m512i v1   = _mm512_sub_epi32(ch_matches, v01);
            const __m512i tmp1 = _mm512_ternarylogic_epi32(v1, ch_matches, v80, 0x20);
            zero_byte_mask     = _mm512_test_epi32_mask(tmp1, tmp1);
        }

        while (zero_byte_mask) {
            uint64_t const lsb_zero_pos = _tzcnt_u64(zero_byte_mask);
            char const    *base_ptr     = ptr + (4 * lsb_zero_pos);

            if (DN_MEMCMP(base_ptr + 0, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data;
            } else if (DN_MEMCMP(base_ptr + 1, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 1;
            } else if (DN_MEMCMP(base_ptr + 2, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 2;
            } else if (DN_MEMCMP(base_ptr + 3, find.data, find.size) == 0) {
                result.found = true;
                result.index = base_ptr - string.data + 3;
            }

            if (result.found) {
                result.start_to_before_match  = DN_Str8_Init(string.data, result.index);
                result.match                  = DN_Str8_Init(string.data + result.index, find.size);
                result.match_to_end_of_buffer = DN_Str8_Init(result.match.data, string.size - result.index);
                return result;
            }

            zero_byte_mask = DN_Bit_ClearNextLSB(zero_byte_mask);
        }
    }

    for (DN_USize index = ptr - string.data - 1; index < string.size; index--) {
        DN_Str8 string_slice = DN_Str8_Slice(string, index, find.size);
        if (DN_Str8_Eq(string_slice, find)) {
            result.found                  = true;
            result.index                  = index;
            result.start_to_before_match  = DN_Str8_Init(string.data, index);
            result.match                  = DN_Str8_Init(string.data + index, find.size);
            result.match_to_end_of_buffer = DN_Str8_Init(result.match.data, string.size - index);
            return result;
        }
    }

    return result;
}

DN_API DN_Str8BinarySplitResult DN_Str8_BinarySplitAVX512F(DN_Str8 string, DN_Str8 find)
{
    DN_Str8BinarySplitResult result      = {};
    DN_Str8FindResult        find_result = DN_Str8_FindStr8AVX512F(string, find);
    if (find_result.found) {
        result.lhs.data = string.data;
        result.lhs.size = find_result.index;
        result.rhs      = DN_Str8_Advance(find_result.match_to_end_of_buffer, find.size);
    } else {
        result.lhs = string;
    }

    return result;
}

DN_API DN_Str8BinarySplitResult DN_Str8_BinarySplitLastAVX512F(DN_Str8 string, DN_Str8 find)
{
    DN_Str8BinarySplitResult result      = {};
    DN_Str8FindResult        find_result = DN_Str8_FindLastStr8AVX512F(string, find);
    if (find_result.found) {
        result.lhs.data = string.data;
        result.lhs.size = find_result.index;
        result.rhs      = DN_Str8_Advance(find_result.match_to_end_of_buffer, find.size);
    } else {
        result.lhs = string;
    }

    return result;
}

DN_API DN_USize DN_Str8_SplitAVX512F(DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode)
{
    DN_USize result = 0; // The number of splits in the actual string.
    if (!DN_Str8_HasData(string) || !DN_Str8_HasData(delimiter) || delimiter.size <= 0)
        return result;

    DN_Str8BinarySplitResult split = {};
    DN_Str8 first                  = string;
    do {
        split = DN_Str8_BinarySplitAVX512F(first, delimiter);
        if (split.lhs.size || mode == DN_Str8SplitIncludeEmptyStrings_Yes) {
            if (splits && result < splits_count)
                splits[result] = split.lhs;
            result++;
        }
        first = split.rhs;
    } while (first.size);

    return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8_SplitAllocAVX512F(DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
    DN_Slice<DN_Str8> result          = {};
    DN_USize           splits_required = DN_Str8_SplitAVX512F(string, delimiter, /*splits*/ nullptr, /*count*/ 0, mode);
    result.data                         = DN_Arena_NewArray(arena, DN_Str8, splits_required, DN_ZeroMem_No);
    if (result.data) {
        result.size = DN_Str8_SplitAVX512F(string, delimiter, result.data, splits_required, mode);
        DN_ASSERT(splits_required == result.size);
    }
    return result;
}
