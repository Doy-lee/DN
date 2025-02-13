#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\ $$$$$$$$\ $$\       $$$$$$$\  $$$$$$$$\ $$$$$$$\   $$$$$$\
//   $$ |  $$ |$$  _____|$$ |      $$  __$$\ $$  _____|$$  __$$\ $$  __$$\
//   $$ |  $$ |$$ |      $$ |      $$ |  $$ |$$ |      $$ |  $$ |$$ /  \__|
//   $$$$$$$$ |$$$$$\    $$ |      $$$$$$$  |$$$$$\    $$$$$$$  |\$$$$$$\
//   $$  __$$ |$$  __|   $$ |      $$  ____/ $$  __|   $$  __$$<  \____$$\
//   $$ |  $$ |$$ |      $$ |      $$ |      $$ |      $$ |  $$ |$$\   $$ |
//   $$ |  $$ |$$$$$$$$\ $$$$$$$$\ $$ |      $$$$$$$$\ $$ |  $$ |\$$$$$$  |
//   \__|  \__|\________|\________|\__|      \________|\__|  \__| \______/
//
//   dqn_helpers.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$PCGX] DN_PCG32 /////////////////////////////////////////////////////////////////////////
#define DN_PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define DN_PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL

DN_API DN_PCG32 DN_PCG32_Init(uint64_t seed)
{
    DN_PCG32 result = {};
    DN_PCG32_Next(&result);
    result.state += seed;
    DN_PCG32_Next(&result);
    return result;
}

DN_API uint32_t DN_PCG32_Next(DN_PCG32 *rng)
{
    uint64_t state = rng->state;
    rng->state     = state * DN_PCG_DEFAULT_MULTIPLIER_64 + DN_PCG_DEFAULT_INCREMENT_64;

    // XSH-RR
    uint32_t value = (uint32_t)((state ^ (state >> 18)) >> 27);
    int      rot   = state >> 59;
    return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

DN_API uint64_t DN_PCG32_Next64(DN_PCG32 *rng)
{
    uint64_t value = DN_PCG32_Next(rng);
    value <<= 32;
    value |= DN_PCG32_Next(rng);
    return value;
}

DN_API uint32_t DN_PCG32_Range(DN_PCG32 *rng, uint32_t low, uint32_t high)
{
    uint32_t bound     = high - low;
    uint32_t threshold = -(int32_t)bound % bound;

    for (;;) {
        uint32_t r = DN_PCG32_Next(rng);
        if (r >= threshold)
            return low + (r % bound);
    }
}

DN_API float DN_PCG32_NextF32(DN_PCG32 *rng)
{
    uint32_t x = DN_PCG32_Next(rng);
    return (float)(int32_t)(x >> 8) * 0x1.0p-24f;
}

DN_API double DN_PCG32_NextF64(DN_PCG32 *rng)
{
    uint64_t x = DN_PCG32_Next64(rng);
    return (double)(int64_t)(x >> 11) * 0x1.0p-53;
}

DN_API void DN_PCG32_Advance(DN_PCG32 *rng, uint64_t delta)
{
    uint64_t cur_mult = DN_PCG_DEFAULT_MULTIPLIER_64;
    uint64_t cur_plus = DN_PCG_DEFAULT_INCREMENT_64;

    uint64_t acc_mult = 1;
    uint64_t acc_plus = 0;

    while (delta != 0) {
        if (delta & 1) {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta >>= 1;
    }

    rng->state = acc_mult * rng->state + acc_plus;
}

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: [$JSON] DN_JSONBuilder ///////////////////////////////////////////////////////////////////
DN_API DN_JSONBuilder DN_JSONBuilder_Init(DN_Arena *arena, int spaces_per_indent)
{
    DN_JSONBuilder result      = {};
    result.spaces_per_indent    = spaces_per_indent;
    result.string_builder.arena = arena;
    return result;
}

DN_API DN_Str8 DN_JSONBuilder_Build(DN_JSONBuilder const *builder, DN_Arena *arena)
{
    DN_Str8 result = DN_Str8Builder_Build(&builder->string_builder, arena);
    return result;
}

DN_API void DN_JSONBuilder_KeyValue(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
    if (key.size == 0 && value.size == 0)
        return;

    DN_JSONBuilderItem item = DN_JSONBuilderItem_KeyValue;
    if (value.size >= 1) {
        if (value.data[0] == '{' || value.data[0] == '[')
            item = DN_JSONBuilderItem_OpenContainer;
        else if (value.data[0] == '}' || value.data[0] == ']')
            item = DN_JSONBuilderItem_CloseContainer;
    }

    bool adding_to_container_with_items =
        item != DN_JSONBuilderItem_CloseContainer && (builder->last_item == DN_JSONBuilderItem_KeyValue ||
                                                       builder->last_item == DN_JSONBuilderItem_CloseContainer);

    uint8_t prefix_size = 0;
    char    prefix[2]   = {0};
    if (adding_to_container_with_items)
        prefix[prefix_size++] = ',';

    if (builder->last_item != DN_JSONBuilderItem_Empty)
        prefix[prefix_size++] = '\n';

    if (item == DN_JSONBuilderItem_CloseContainer)
        builder->indent_level--;

    int spaces_per_indent = builder->spaces_per_indent ? builder->spaces_per_indent : 2;
    int spaces            = builder->indent_level * spaces_per_indent;

    if (key.size) {
        DN_Str8Builder_AppendF(&builder->string_builder,
                             "%.*s%*c\"%.*s\": %.*s",
                             prefix_size,
                             prefix,
                             spaces,
                             ' ',
                             DN_STR_FMT(key),
                             DN_STR_FMT(value));
    } else {
        if (spaces == 0)
            DN_Str8Builder_AppendF(&builder->string_builder, "%.*s%.*s", prefix_size, prefix, DN_STR_FMT(value));
        else
            DN_Str8Builder_AppendF(&builder->string_builder, "%.*s%*c%.*s", prefix_size, prefix, spaces, ' ', DN_STR_FMT(value));
    }

    if (item == DN_JSONBuilderItem_OpenContainer)
        builder->indent_level++;

    builder->last_item = item;
}

DN_API void DN_JSONBuilder_KeyValueFV(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, va_list args)
{
    DN_TLSTMem tmem  = DN_TLS_TMem(builder->string_builder.arena);
    DN_Str8    value = DN_Str8_InitFV(tmem.arena, value_fmt, args);
    DN_JSONBuilder_KeyValue(builder, key, value);
}

DN_API void DN_JSONBuilder_KeyValueF(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, ...)
{
    va_list args;
    va_start(args, value_fmt);
    DN_JSONBuilder_KeyValueFV(builder, key, value_fmt, args);
    va_end(args);
}

DN_API void DN_JSONBuilder_ObjectBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
    DN_JSONBuilder_KeyValue(builder, name, DN_STR8("{"));
}

DN_API void DN_JSONBuilder_ObjectEnd(DN_JSONBuilder *builder)
{
    DN_JSONBuilder_KeyValue(builder, DN_STR8(""), DN_STR8("}"));
}

DN_API void DN_JSONBuilder_ArrayBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
    DN_JSONBuilder_KeyValue(builder, name, DN_STR8("["));
}

DN_API void DN_JSONBuilder_ArrayEnd(DN_JSONBuilder *builder)
{
    DN_JSONBuilder_KeyValue(builder, DN_STR8(""), DN_STR8("]"));
}

DN_API void DN_JSONBuilder_Str8Named(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
    DN_JSONBuilder_KeyValueF(builder, key, "\"%.*s\"", value.size, value.data);
}

DN_API void DN_JSONBuilder_LiteralNamed(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
    DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value.size, value.data);
}

DN_API void DN_JSONBuilder_U64Named(DN_JSONBuilder *builder, DN_Str8 key, uint64_t value)
{
    DN_JSONBuilder_KeyValueF(builder, key, "%I64u", value);
}

DN_API void DN_JSONBuilder_I64Named(DN_JSONBuilder *builder, DN_Str8 key, int64_t value)
{
    DN_JSONBuilder_KeyValueF(builder, key, "%I64d", value);
}

DN_API void DN_JSONBuilder_F64Named(DN_JSONBuilder *builder, DN_Str8 key, double value, int decimal_places)
{
    if (!builder)
        return;

    if (decimal_places >= 16)
        decimal_places = 16;

    // NOTE: Generate the format string for the float, depending on how many
    // decimals places it wants.
    char float_fmt[16];
    if (decimal_places > 0) {
        // NOTE: Emit the format string "%.<decimal_places>f" i.e. %.1f
        DN_SNPRINTF(float_fmt, sizeof(float_fmt), "%%.%df", decimal_places);
    } else {
        // NOTE: Emit the format string "%f"
        DN_SNPRINTF(float_fmt, sizeof(float_fmt), "%%f");
    }
    DN_JSONBuilder_KeyValueF(builder, key, float_fmt, value);
}

DN_API void DN_JSONBuilder_BoolNamed(DN_JSONBuilder *builder, DN_Str8 key, bool value)
{
    DN_Str8 value_string = value ? DN_STR8("true") : DN_STR8("false");
    DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value_string.size, value_string.data);
}
#endif // !defined(DN_NO_JSON_BUILDER)

// NOTE: [$BITS] DN_Bit ///////////////////////////////////////////////////////////////////////////
DN_API void DN_Bit_UnsetInplace(DN_USize *flags, DN_USize bitfield)
{
    *flags = (*flags & ~bitfield);
}

DN_API void DN_Bit_SetInplace(DN_USize *flags, DN_USize bitfield)
{
    *flags = (*flags | bitfield);
}

DN_API bool DN_Bit_IsSet(DN_USize bits, DN_USize bits_to_set)
{
    auto result = DN_CAST(bool)((bits & bits_to_set) == bits_to_set);
    return result;
}

DN_API bool DN_Bit_IsNotSet(DN_USize bits, DN_USize bits_to_check)
{
    auto result = !DN_Bit_IsSet(bits, bits_to_check);
    return result;
}

// NOTE: [$SAFE] DN_Safe //////////////////////////////////////////////////////////////////////////
DN_API int64_t DN_Safe_AddI64(int64_t a, int64_t b)
{
    int64_t result = DN_CHECKF(a <= INT64_MAX - b, "a=%zd, b=%zd", a, b) ? (a + b) : INT64_MAX;
    return result;
}

DN_API int64_t DN_Safe_MulI64(int64_t a, int64_t b)
{
    int64_t result = DN_CHECKF(a <= INT64_MAX / b, "a=%zd, b=%zd", a, b) ? (a * b) : INT64_MAX;
    return result;
}

DN_API uint64_t DN_Safe_AddU64(uint64_t a, uint64_t b)
{
    uint64_t result = DN_CHECKF(a <= UINT64_MAX - b, "a=%zu, b=%zu", a, b) ? (a + b) : UINT64_MAX;
    return result;
}

DN_API uint64_t DN_Safe_SubU64(uint64_t a, uint64_t b)
{
    uint64_t result = DN_CHECKF(a >= b, "a=%zu, b=%zu", a, b) ? (a - b) : 0;
    return result;
}

DN_API uint64_t DN_Safe_MulU64(uint64_t a, uint64_t b)
{
    uint64_t result = DN_CHECKF(a <= UINT64_MAX / b, "a=%zu, b=%zu", a, b) ? (a * b) : UINT64_MAX;
    return result;
}

DN_API uint32_t DN_Safe_SubU32(uint32_t a, uint32_t b)
{
    uint32_t result = DN_CHECKF(a >= b, "a=%u, b=%u", a, b) ? (a - b) : 0;
    return result;
}

// NOTE: DN_Safe_SaturateCastUSizeToI* ////////////////////////////////////////////////////////////
// INT*_MAX literals will be promoted to the type of uintmax_t as uintmax_t is
// the highest possible rank (unsigned > signed).
DN_API int DN_Safe_SaturateCastUSizeToInt(DN_USize val)
{
    int result = DN_CHECK(DN_CAST(uintmax_t) val <= INT_MAX) ? DN_CAST(int) val : INT_MAX;
    return result;
}

DN_API int8_t DN_Safe_SaturateCastUSizeToI8(DN_USize val)
{
    int8_t result = DN_CHECK(DN_CAST(uintmax_t) val <= INT8_MAX) ? DN_CAST(int8_t) val : INT8_MAX;
    return result;
}

DN_API int16_t DN_Safe_SaturateCastUSizeToI16(DN_USize val)
{
    int16_t result = DN_CHECK(DN_CAST(uintmax_t) val <= INT16_MAX) ? DN_CAST(int16_t) val : INT16_MAX;
    return result;
}

DN_API int32_t DN_Safe_SaturateCastUSizeToI32(DN_USize val)
{
    int32_t result = DN_CHECK(DN_CAST(uintmax_t) val <= INT32_MAX) ? DN_CAST(int32_t) val : INT32_MAX;
    return result;
}

DN_API int64_t DN_Safe_SaturateCastUSizeToI64(DN_USize val)
{
    int64_t result = DN_CHECK(DN_CAST(uintmax_t) val <= INT64_MAX) ? DN_CAST(int64_t) val : INT64_MAX;
    return result;
}

// NOTE: DN_Safe_SaturateCastUSizeToU* ////////////////////////////////////////////////////////////
// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API uint8_t DN_Safe_SaturateCastUSizeToU8(DN_USize val)
{
    uint8_t result = DN_CHECK(val <= UINT8_MAX) ? DN_CAST(uint8_t) val : UINT8_MAX;
    return result;
}

DN_API uint16_t DN_Safe_SaturateCastUSizeToU16(DN_USize val)
{
    uint16_t result = DN_CHECK(val <= UINT16_MAX) ? DN_CAST(uint16_t) val : UINT16_MAX;
    return result;
}

DN_API uint32_t DN_Safe_SaturateCastUSizeToU32(DN_USize val)
{
    uint32_t result = DN_CHECK(val <= UINT32_MAX) ? DN_CAST(uint32_t) val : UINT32_MAX;
    return result;
}

DN_API uint64_t DN_Safe_SaturateCastUSizeToU64(DN_USize val)
{
    uint64_t result = DN_CHECK(DN_CAST(uint64_t) val <= UINT64_MAX) ? DN_CAST(uint64_t) val : UINT64_MAX;
    return result;
}

// NOTE: DN_Safe_SaturateCastU64To* ///////////////////////////////////////////////////////////////
DN_API int DN_Safe_SaturateCastU64ToInt(uint64_t val)
{
    int result = DN_CHECK(val <= INT_MAX) ? DN_CAST(int)val : INT_MAX;
    return result;
}

DN_API int8_t DN_Safe_SaturateCastU64ToI8(uint64_t val)
{
    int8_t result = DN_CHECK(val <= INT8_MAX) ? DN_CAST(int8_t)val : INT8_MAX;
    return result;
}

DN_API int16_t DN_Safe_SaturateCastU64ToI16(uint64_t val)
{
    int16_t result = DN_CHECK(val <= INT16_MAX) ? DN_CAST(int16_t)val : INT16_MAX;
    return result;
}

DN_API int32_t DN_Safe_SaturateCastU64ToI32(uint64_t val)
{
    int32_t result = DN_CHECK(val <= INT32_MAX) ? DN_CAST(int32_t)val : INT32_MAX;
    return result;
}

DN_API int64_t DN_Safe_SaturateCastU64ToI64(uint64_t val)
{
    int64_t result = DN_CHECK(val <= INT64_MAX) ? DN_CAST(int64_t)val : INT64_MAX;
    return result;
}

// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API unsigned int DN_Safe_SaturateCastU64ToUInt(uint64_t val)
{
    unsigned int result = DN_CHECK(val <= UINT8_MAX) ? DN_CAST(unsigned int) val : UINT_MAX;
    return result;
}

DN_API uint8_t DN_Safe_SaturateCastU64ToU8(uint64_t val)
{
    uint8_t result = DN_CHECK(val <= UINT8_MAX) ? DN_CAST(uint8_t) val : UINT8_MAX;
    return result;
}

DN_API uint16_t DN_Safe_SaturateCastU64ToU16(uint64_t val)
{
    uint16_t result = DN_CHECK(val <= UINT16_MAX) ? DN_CAST(uint16_t) val : UINT16_MAX;
    return result;
}

DN_API uint32_t DN_Safe_SaturateCastU64ToU32(uint64_t val)
{
    uint32_t result = DN_CHECK(val <= UINT32_MAX) ? DN_CAST(uint32_t) val : UINT32_MAX;
    return result;
}

// NOTE: DN_Safe_SaturateCastISizeToI* ////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API int DN_Safe_SaturateCastISizeToInt(DN_ISize val)
{
    DN_ASSERT(val >= INT_MIN && val <= INT_MAX);
    int result = DN_CAST(int) DN_CLAMP(val, INT_MIN, INT_MAX);
    return result;
}

DN_API int8_t DN_Safe_SaturateCastISizeToI8(DN_ISize val)
{
    DN_ASSERT(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DN_CAST(int8_t) DN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DN_API int16_t DN_Safe_SaturateCastISizeToI16(DN_ISize val)
{
    DN_ASSERT(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DN_CAST(int16_t) DN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DN_API int32_t DN_Safe_SaturateCastISizeToI32(DN_ISize val)
{
    DN_ASSERT(val >= INT32_MIN && val <= INT32_MAX);
    int32_t result = DN_CAST(int32_t) DN_CLAMP(val, INT32_MIN, INT32_MAX);
    return result;
}

DN_API int64_t DN_Safe_SaturateCastISizeToI64(DN_ISize val)
{
    DN_ASSERT(DN_CAST(int64_t) val >= INT64_MIN && DN_CAST(int64_t) val <= INT64_MAX);
    int64_t result = DN_CAST(int64_t) DN_CLAMP(DN_CAST(int64_t) val, INT64_MIN, INT64_MAX);
    return result;
}

// NOTE: DN_Safe_SaturateCastISizeToU* ////////////////////////////////////////////////////////////
// If the value is a negative integer, we clamp to 0. Otherwise, we know that
// the value is >=0, we can upcast safely to bounds check against the maximum
// allowed value.
DN_API unsigned int DN_Safe_SaturateCastISizeToUInt(DN_ISize val)
{
    unsigned int result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT_MAX))
            result = DN_CAST(unsigned int) val;
        else
            result = UINT_MAX;
    }
    return result;
}

DN_API uint8_t DN_Safe_SaturateCastISizeToU8(DN_ISize val)
{
    uint8_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DN_API uint16_t DN_Safe_SaturateCastISizeToU16(DN_ISize val)
{
    uint16_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DN_API uint32_t DN_Safe_SaturateCastISizeToU32(DN_ISize val)
{
    uint32_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT32_MAX))
            result = DN_CAST(uint32_t) val;
        else
            result = UINT32_MAX;
    }
    return result;
}

DN_API uint64_t DN_Safe_SaturateCastISizeToU64(DN_ISize val)
{
    uint64_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT64_MAX))
            result = DN_CAST(uint64_t) val;
        else
            result = UINT64_MAX;
    }
    return result;
}

// NOTE: DN_Safe_SaturateCastI64To* ///////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API DN_ISize DN_Safe_SaturateCastI64ToISize(int64_t val)
{
    DN_CHECK(val >= DN_ISIZE_MIN && val <= DN_ISIZE_MAX);
    DN_ISize result = DN_CAST(int64_t) DN_CLAMP(val, DN_ISIZE_MIN, DN_ISIZE_MAX);
    return result;
}

DN_API int8_t DN_Safe_SaturateCastI64ToI8(int64_t val)
{
    DN_CHECK(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DN_CAST(int8_t) DN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DN_API int16_t DN_Safe_SaturateCastI64ToI16(int64_t val)
{
    DN_CHECK(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DN_CAST(int16_t) DN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DN_API int32_t DN_Safe_SaturateCastI64ToI32(int64_t val)
{
    DN_CHECK(val >= INT32_MIN && val <= INT32_MAX);
    int32_t result = DN_CAST(int32_t) DN_CLAMP(val, INT32_MIN, INT32_MAX);
    return result;
}

DN_API unsigned int DN_Safe_SaturateCastI64ToUInt(int64_t val)
{
    unsigned int result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT_MAX))
            result = DN_CAST(unsigned int) val;
        else
            result = UINT_MAX;
    }
    return result;
}

DN_API DN_ISize DN_Safe_SaturateCastI64ToUSize(int64_t val)
{
    DN_USize result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= DN_USIZE_MAX))
            result = DN_CAST(DN_USize) val;
        else
            result = DN_USIZE_MAX;
    }
    return result;
}

DN_API uint8_t DN_Safe_SaturateCastI64ToU8(int64_t val)
{
    uint8_t result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DN_API uint16_t DN_Safe_SaturateCastI64ToU16(int64_t val)
{
    uint16_t result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DN_API uint32_t DN_Safe_SaturateCastI64ToU32(int64_t val)
{
    uint32_t result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT32_MAX))
            result = DN_CAST(uint32_t) val;
        else
            result = UINT32_MAX;
    }
    return result;
}

DN_API uint64_t DN_Safe_SaturateCastI64ToU64(int64_t val)
{
    uint64_t result = 0;
    if (DN_CHECK(val >= DN_CAST(int64_t) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT64_MAX))
            result = DN_CAST(uint64_t) val;
        else
            result = UINT64_MAX;
    }
    return result;
}

// NOTE: DN_Safe_SaturateCastIntTo* ///////////////////////////////////////////////////////////////
DN_API int8_t DN_Safe_SaturateCastIntToI8(int val)
{
    DN_CHECK(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DN_CAST(int8_t) DN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DN_API int16_t DN_Safe_SaturateCastIntToI16(int val)
{
    DN_CHECK(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DN_CAST(int16_t) DN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DN_API uint8_t DN_Safe_SaturateCastIntToU8(int val)
{
    uint8_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DN_API uint16_t DN_Safe_SaturateCastIntToU16(int val)
{
    uint16_t result = 0;
    if (DN_CHECK(val >= DN_CAST(DN_ISize) 0)) {
        if (DN_CHECK(DN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DN_API uint32_t DN_Safe_SaturateCastIntToU32(int val)
{
    static_assert(sizeof(val) <= sizeof(uint32_t), "Sanity check to allow simplifying of casting");
    uint32_t result = 0;
    if (DN_CHECK(val >= 0))
        result = DN_CAST(uint32_t) val;
    return result;
}

DN_API uint64_t DN_Safe_SaturateCastIntToU64(int val)
{
    static_assert(sizeof(val) <= sizeof(uint64_t), "Sanity check to allow simplifying of casting");
    uint64_t result = 0;
    if (DN_CHECK(val >= 0))
        result = DN_CAST(uint64_t) val;
    return result;
}

// NOTE: [$MISC] Misc //////////////////////////////////////////////////////////////////////////////
DN_API int DN_FmtBuffer3DotTruncate(char *buffer, int size, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size_required = DN_VSNPRINTF(buffer, size, fmt, args);
    int result        = DN_MAX(DN_MIN(size_required, size - 1), 0);
    if (result == size - 1) {
        buffer[size - 2] = '.';
        buffer[size - 3] = '.';
    }
    va_end(args);
    return result;
}

DN_API DN_U64Str8 DN_U64ToStr8(uint64_t val, char separator)
{
    DN_U64Str8 result = {};
    if (val == 0) {
        result.data[result.size++] = '0';
    } else {
        // NOTE: The number is written in reverse because we form the string by
        // dividing by 10, so we write it in, then reverse it out after all is
        // done.
        DN_U64Str8 temp = {};
        for (DN_USize digit_count = 0; val > 0; digit_count++) {
            if (separator && (digit_count != 0) && (digit_count % 3 == 0))
                temp.data[temp.size++] = separator;

            auto digit             = DN_CAST(char)(val % 10);
            temp.data[temp.size++] = '0' + digit;
            val /= 10;
        }

        // NOTE: Reverse the string
        DN_MSVC_WARNING_PUSH
        DN_MSVC_WARNING_DISABLE(6293) // Ill-defined for-loop
        DN_MSVC_WARNING_DISABLE(
            6385) // Reading invalid data from 'temp.data' NOTE(doyle): Unsigned overflow is valid for loop termination
        for (DN_USize temp_index = temp.size - 1; temp_index < temp.size; temp_index--) {
            char ch                    = temp.data[temp_index];
            result.data[result.size++] = ch;
        }
        DN_MSVC_WARNING_POP
    }

    return result;
}

DN_API DN_U64ByteSize DN_U64ToByteSize(uint64_t bytes, DN_U64ByteSizeType desired_type)
{
    DN_U64ByteSize result = {};
    result.bytes           = DN_CAST(DN_F64)bytes;
    if (!DN_CHECK(desired_type != DN_U64ByteSizeType_Count)) {
        result.suffix = DN_U64ByteSizeTypeString(result.type);
        return result;
    }

    if (desired_type == DN_U64ByteSizeType_Auto) {
        for (; result.type < DN_U64ByteSizeType_Count && result.bytes >= 1024.0; result.type = DN_CAST(DN_U64ByteSizeType)(DN_CAST(DN_USize)result.type + 1))
            result.bytes /= 1024.0;
    } else {
        for (; result.type < desired_type; result.type = DN_CAST(DN_U64ByteSizeType)(DN_CAST(DN_USize)result.type + 1))
            result.bytes /= 1024.0;
    }

    result.suffix = DN_U64ByteSizeTypeString(result.type);
    return result;
}

DN_API DN_Str8 DN_U64ToByteSizeStr8(DN_Arena *arena, uint64_t bytes, DN_U64ByteSizeType desired_type)
{
    DN_U64ByteSize byte_size = DN_U64ToByteSize(bytes, desired_type);
    DN_Str8        result    = DN_Str8_InitF(arena, "%.2f%.*s", byte_size.bytes, DN_STR_FMT(byte_size.suffix));
    return result;
}

DN_API DN_Str8 DN_U64ByteSizeTypeString(DN_U64ByteSizeType type)
{
    DN_Str8 result = DN_STR8("");
    switch (type) {
        case DN_U64ByteSizeType_B:     result = DN_STR8("B");   break;
        case DN_U64ByteSizeType_KiB:   result = DN_STR8("KiB"); break;
        case DN_U64ByteSizeType_MiB:   result = DN_STR8("MiB"); break;
        case DN_U64ByteSizeType_GiB:   result = DN_STR8("GiB"); break;
        case DN_U64ByteSizeType_TiB:   result = DN_STR8("TiB"); break;
        case DN_U64ByteSizeType_Count: result = DN_STR8("");    break;
        case DN_U64ByteSizeType_Auto:  result = DN_STR8("");    break;
    }
    return result;
}

DN_API DN_Str8 DN_U64ToAge(DN_Arena *arena, uint64_t age_s, DN_U64AgeUnit unit)
{
    DN_Str8 result = {};
    if (!arena)
        return result;

    DN_TLSTMem     tmem      = DN_TLS_TMem(arena);
    DN_Str8Builder builder   = DN_Str8Builder_Init(tmem.arena);
    uint64_t        remainder = age_s;

    if (unit & DN_U64AgeUnit_Year) {
        DN_USize value = remainder / DN_YEARS_TO_S(1);
        remainder -= DN_YEARS_TO_S(value);
        if (value)
            DN_Str8Builder_AppendF(&builder, "%s%I64uyr", builder.string_size ? " " : "", value);
    }

    if (unit & DN_U64AgeUnit_Week) {
        DN_USize value = remainder / DN_WEEKS_TO_S(1);
        remainder -= DN_WEEKS_TO_S(value);
        if (value)
            DN_Str8Builder_AppendF(&builder, "%s%I64uw", builder.string_size ? " " : "", value);
    }

    if (unit & DN_U64AgeUnit_Day) {
        DN_USize value = remainder / DN_DAYS_TO_S(1);
        remainder -= DN_DAYS_TO_S(value);
        if (value)
            DN_Str8Builder_AppendF(&builder, "%s%I64ud", builder.string_size ? " " : "", value);
    }

    if (unit & DN_U64AgeUnit_Hr) {
        DN_USize value = remainder / DN_HOURS_TO_S(1);
        remainder -= DN_HOURS_TO_S(value);
        if (value)
            DN_Str8Builder_AppendF(&builder, "%s%I64uh", builder.string_size ? " " : "", value);
    }

    if (unit & DN_U64AgeUnit_Min) {
        DN_USize value = remainder / DN_MINS_TO_S(1);
        remainder -= DN_MINS_TO_S(value);
        if (value)
            DN_Str8Builder_AppendF(&builder, "%s%I64um", builder.string_size ? " " : "", value);
    }

    if (unit & DN_U64AgeUnit_Sec) {
        DN_USize value = remainder;
        DN_Str8Builder_AppendF(&builder, "%s%I64us", builder.string_size ? " " : "", value);
    }

    result = DN_Str8Builder_Build(&builder, arena);
    return result;
}

DN_API DN_Str8 DN_F64ToAge(DN_Arena *arena, DN_F64 age_s, DN_U64AgeUnit unit)
{
    DN_Str8 result = {};
    if (!arena)
        return result;

    DN_TLSTMem     tmem      = DN_TLS_TMem(arena);
    DN_Str8Builder builder   = DN_Str8Builder_Init(tmem.arena);
    DN_F64         remainder = age_s;

    if (unit & DN_U64AgeUnit_Year) {
        DN_F64 value = remainder / DN_CAST(DN_F64)DN_YEARS_TO_S(1);
        if (value >= 1.0) {
            remainder -= DN_YEARS_TO_S(value);
            DN_Str8Builder_AppendF(&builder, "%s%.1fyr", builder.string_size ? " " : "", value);
        }
    }

    if (unit & DN_U64AgeUnit_Week) {
        DN_F64 value = remainder / DN_CAST(DN_F64)DN_WEEKS_TO_S(1);
        if (value >= 1.0) {
            remainder -= DN_WEEKS_TO_S(value);
            DN_Str8Builder_AppendF(&builder, "%s%.1fw", builder.string_size ? " " : "", value);
        }
    }

    if (unit & DN_U64AgeUnit_Day) {
        DN_F64 value = remainder / DN_CAST(DN_F64)DN_DAYS_TO_S(1);
        if (value >= 1.0) {
            remainder -= DN_WEEKS_TO_S(value);
            DN_Str8Builder_AppendF(&builder, "%s%.1fd", builder.string_size ? " " : "", value);
        }
    }

    if (unit & DN_U64AgeUnit_Hr) {
        DN_F64 value = remainder / DN_CAST(DN_F64)DN_HOURS_TO_S(1);
        if (value >= 1.0) {
            remainder -= DN_HOURS_TO_S(value);
            DN_Str8Builder_AppendF(&builder, "%s%.1fh", builder.string_size ? " " : "", value);
        }
    }

    if (unit & DN_U64AgeUnit_Min) {
        DN_F64 value = remainder / DN_CAST(DN_F64)DN_MINS_TO_S(1);
        if (value >= 1.0) {
            remainder -= DN_MINS_TO_S(value);
            DN_Str8Builder_AppendF(&builder, "%s%.1fm", builder.string_size ? " " : "", value);
        }
    }

    if (unit & DN_U64AgeUnit_Sec) {
        DN_F64 value = remainder;
        DN_Str8Builder_AppendF(&builder, "%s%.1fs", builder.string_size ? " " : "", value);
    }

    result = DN_Str8Builder_Build(&builder, arena);
    return result;
}

DN_API uint64_t DN_HexToU64(DN_Str8 hex)
{
    DN_Str8  real_hex     = DN_Str8_TrimPrefix(DN_Str8_TrimPrefix(hex, DN_STR8("0x")), DN_STR8("0X"));
    DN_USize max_hex_size = sizeof(uint64_t) * 2 /*hex chars per byte*/;
    DN_ASSERT(real_hex.size <= max_hex_size);

    DN_USize size   = DN_MIN(max_hex_size, real_hex.size);
    uint64_t  result = 0;
    for (DN_USize index = 0; index < size; index++) {
        char            ch  = real_hex.data[index];
        DN_CharHexToU8 val = DN_Char_HexToU8(ch);
        if (!val.success)
            break;
        result = (result << 4) | val.value;
    }
    return result;
}

DN_API DN_Str8 DN_U64ToHex(DN_Arena *arena, uint64_t number, uint32_t flags)
{
    DN_Str8 prefix = {};
    if ((flags & DN_HexU64Str8Flags_0xPrefix))
        prefix = DN_STR8("0x");

    char const *fmt           = (flags & DN_HexU64Str8Flags_UppercaseHex) ? "%I64X" : "%I64x";
    DN_USize   required_size = DN_CStr8_FSize(fmt, number) + prefix.size;
    DN_Str8    result        = DN_Str8_Alloc(arena, required_size, DN_ZeroMem_No);

    if (DN_Str8_HasData(result)) {
        DN_MEMCPY(result.data, prefix.data, prefix.size);
        int space = DN_CAST(int) DN_MAX((result.size - prefix.size) + 1, 0); /*null-terminator*/
        DN_SNPRINTF(result.data + prefix.size, space, fmt, number);
    }
    return result;
}

DN_API DN_U64HexStr8 DN_U64ToHexStr8(uint64_t number, DN_U64HexStr8Flags flags)
{
    DN_Str8 prefix = {};
    if (flags & DN_HexU64Str8Flags_0xPrefix)
        prefix = DN_STR8("0x");

    DN_U64HexStr8 result = {};
    DN_MEMCPY(result.data, prefix.data, prefix.size);
    result.size += DN_CAST(int8_t) prefix.size;

    char const *fmt = (flags & DN_HexU64Str8Flags_UppercaseHex) ? "%I64X" : "%I64x";
    int size        = DN_SNPRINTF(result.data + result.size, DN_ARRAY_UCOUNT(result.data) - result.size, fmt, number);
    result.size += DN_CAST(uint8_t) size;
    DN_ASSERT(result.size < DN_ARRAY_UCOUNT(result.data));

    // NOTE: snprintf returns the required size of the format string
    // irrespective of if there's space or not, but, always null terminates so
    // the last byte is wasted.
    result.size = DN_MIN(result.size, DN_ARRAY_UCOUNT(result.data) - 1);
    return result;
}

DN_API bool DN_BytesToHexPtr(void const *src, DN_USize src_size, char *dest, DN_USize dest_size)
{
    if (!src || !dest)
        return false;

    if (!DN_CHECK(dest_size >= src_size * 2))
        return false;

    char const          *HEX    = "0123456789abcdef";
    unsigned char const *src_u8 = DN_CAST(unsigned char const *) src;
    for (DN_USize src_index = 0, dest_index = 0; src_index < src_size; src_index++) {
        char byte          = src_u8[src_index];
        char hex01         = (byte >> 4) & 0b1111;
        char hex02         = (byte >> 0) & 0b1111;
        dest[dest_index++] = HEX[(int)hex01];
        dest[dest_index++] = HEX[(int)hex02];
    }

    return true;
}

DN_API DN_Str8 DN_BytesToHex(DN_Arena *arena, void const *src, DN_USize size)
{
    DN_Str8 result = {};
    if (!src || size <= 0)
        return result;

    result                   = DN_Str8_Alloc(arena, size * 2, DN_ZeroMem_No);
    result.data[result.size] = 0;
    bool converted           = DN_BytesToHexPtr(src, size, result.data, result.size);
    DN_ASSERT(converted);
    return result;
}

DN_API DN_USize DN_HexToBytesPtrUnchecked(DN_Str8 hex, void *dest, DN_USize dest_size)
{
    DN_USize      result  = 0;
    unsigned char *dest_u8 = DN_CAST(unsigned char *) dest;

    for (DN_USize hex_index = 0; hex_index < hex.size; hex_index += 2, result += 1) {
        char hex01      = hex.data[hex_index];
        char hex02      = (hex_index + 1 < hex.size) ? hex.data[hex_index + 1] : 0;
        char bit4_01    = DN_Char_HexToU8(hex01).value;
        char bit4_02    = DN_Char_HexToU8(hex02).value;
        char byte       = (bit4_01 << 4) | (bit4_02 << 0);
        dest_u8[result] = byte;
    }

    DN_ASSERT(result <= dest_size);
    return result;
}

DN_API DN_USize DN_HexToBytesPtr(DN_Str8 hex, void *dest, DN_USize dest_size)
{
    hex = DN_Str8_TrimPrefix(hex, DN_STR8("0x"));
    hex = DN_Str8_TrimPrefix(hex, DN_STR8("0X"));

    DN_USize result = 0;
    if (!DN_Str8_HasData(hex))
        return result;

    // NOTE: Trimmed hex can be "0xf" -> "f" or "0xAB" -> "AB"
    // Either way, the size can be odd or even, hence we round up to the nearest
    // multiple of two to ensure that we calculate the min buffer size orrectly.
    DN_USize hex_size_rounded_up = hex.size + (hex.size % 2);
    DN_USize min_buffer_size     = hex_size_rounded_up / 2;
    if (hex.size <= 0 || !DN_CHECK(dest_size >= min_buffer_size)) {
        return result;
    }

    result = DN_HexToBytesPtrUnchecked(hex, dest, dest_size);
    return result;
}

DN_API DN_Str8 DN_HexToBytesUnchecked(DN_Arena *arena, DN_Str8 hex)
{
    DN_USize hex_size_rounded_up = hex.size + (hex.size % 2);
    DN_Str8  result              = DN_Str8_Alloc(arena, (hex_size_rounded_up / 2), DN_ZeroMem_No);
    if (result.data) {
        DN_USize bytes_written = DN_HexToBytesPtr(hex, result.data, result.size);
        DN_ASSERT(bytes_written == result.size);
    }
    return result;
}

DN_API DN_Str8 DN_HexToBytes(DN_Arena *arena, DN_Str8 hex)
{
    hex = DN_Str8_TrimPrefix(hex, DN_STR8("0x"));
    hex = DN_Str8_TrimPrefix(hex, DN_STR8("0X"));

    DN_Str8 result = {};
    if (!DN_Str8_HasData(hex))
        return result;

    if (!DN_CHECK(DN_Str8_IsAll(hex, DN_Str8IsAll_Hex)))
        return result;

    result = DN_HexToBytesUnchecked(arena, hex);
    return result;
}


// NOTE: [$CORE] DN_Core //////////////////////////////////////////////////////////////////////////
DN_Core *g_dn_core;

DN_API void DN_Core_Init(DN_Core *core, DN_CoreOnInit on_init)
{
    // NOTE: Init check ///////////////////////////////////////////////////////////////////////////
    if (core->init)
        return;

    #define DN_CPU_FEAT_XENTRY(label) g_dn_cpu_feature_decl[DN_CPUFeature_##label] = {DN_CPUFeature_##label, DN_STR8(#label)};
    DN_CPU_FEAT_XMACRO
    #undef DN_CPU_FEAT_XENTRY

    // NOTE: Setup OS //////////////////////////////////////////////////////////////////////////////
    {
    #if defined(DN_OS_WIN32)
        SYSTEM_INFO system_info = {};
        GetSystemInfo(&system_info);
        core->os_page_size         = system_info.dwPageSize;
        core->os_alloc_granularity = system_info.dwAllocationGranularity;
        QueryPerformanceFrequency(&core->win32_qpc_frequency);

        HMODULE module = LoadLibraryA("kernel32.dll");
        g_dn_win_set_thread_description = DN_CAST(DN_WinSetThreadDescriptionFunc *) GetProcAddress(module, "SetThreadDescription");
        FreeLibrary(module);
    #else
        // TODO(doyle): Get the proper page size from the OS.
        core->os_page_size         = DN_KILOBYTES(4);
        core->os_alloc_granularity = DN_KILOBYTES(64);
    #endif
    }

    core->init_mutex = DN_OS_MutexInit();
    DN_OS_MutexLock(&core->init_mutex);
    DN_DEFER {
        DN_OS_MutexUnlock(&core->init_mutex);
    };

    DN_Core_SetPointer(core);
    core->init       = true;
    core->cpu_report = DN_CPU_Report();

    // NOTE Initialise fields //////////////////////////////////////////////////////////////////////
    #if !defined(DN_NO_PROFILER)
    core->profiler = &core->profiler_default_instance;
    #endif

    // NOTE: BEGIN IMPORTANT ORDER OF STATEMENTS ///////////////////////////////////////////////////
    #if defined(DN_LEAK_TRACKING)
    // NOTE: Setup the allocation table with allocation tracking turned off on
    // the arena we're using to initialise the table.
    core->alloc_table_arena = DN_Arena_InitSize(DN_MEGABYTES(1), DN_KILOBYTES(512), DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak);
    core->alloc_table       = DN_DSMap_Init<DN_DebugAlloc>(&core->alloc_table_arena, 4096, DN_DSMapFlags_Nil);
    #endif

    core->arena = DN_Arena_InitSize(DN_KILOBYTES(64), DN_KILOBYTES(4), DN_ArenaFlags_AllocCanLeak);
    core->pool  = DN_Pool_Init(&core->arena, /*align*/ 0);
    DN_ArenaCatalog_Init(&core->arena_catalog, &core->pool);
    DN_ArenaCatalog_AddF(&core->arena_catalog, &core->arena, "DN Core");

    #if defined(DN_LEAK_TRACKING)
    DN_ArenaCatalog_AddF(&core->arena_catalog, &core->alloc_table_arena, "DN Allocation Table");
    #endif

    // NOTE: Initialise tmem arenas which allocate memory and will be
    // recorded to the now initialised allocation table. The initialisation
    // of tmem memory may request tmem memory itself in leak tracing mode.
    // This is supported as the tmem arenas defer allocation tracking until
    // initialisation is done.
    DN_TLS_Init(&core->tls);
    DN_OS_ThreadSetTLS(&core->tls);
    DN_TLSTMem tmem = DN_TLS_TMem(nullptr);
    // NOTE: END IMPORTANT ORDER OF STATEMENTS /////////////////////////////////////////////////////

    core->exe_dir = DN_OS_EXEDir(&core->arena);

    // NOTE: Print out init features ///////////////////////////////////////////////////////////////
    DN_Str8Builder builder = DN_Str8Builder_Init(tmem.arena);
    if (on_init & DN_CoreOnInit_LogLibFeatures) {
        DN_Str8Builder_AppendRef(&builder, DN_STR8("DN Library initialised:\n"));

        DN_F64 page_size_kib         = core->os_page_size / 1024.0;
        DN_F64 alloc_granularity_kib = core->os_alloc_granularity / 1024.0;
        DN_Str8Builder_AppendF(
            &builder, "  OS Page Size/Alloc Granularity: %.1f/%.1fKiB\n", page_size_kib, alloc_granularity_kib);

        #if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
        if (DN_ASAN_POISON) {
            DN_Str8Builder_AppendF(
                &builder, "  ASAN manual poisoning%s\n", DN_ASAN_VET_POISON ? " (+vet sanity checks)" : "");
            DN_Str8Builder_AppendF(&builder, "  ASAN poison guard size: %u\n", DN_ASAN_POISON_GUARD_SIZE);
        }
        #endif

        #if defined(DN_LEAK_TRACKING)
        DN_Str8Builder_AppendRef(&builder, DN_STR8("  Allocation leak tracing\n"));
        #endif

        #if !defined(DN_NO_PROFILER)
        DN_Str8Builder_AppendRef(&builder, DN_STR8("  TSC profiler available\n"));
        #endif

        #if defined(DN_USE_STD_PRINTF)
        DN_Str8Builder_AppendRef(&builder, DN_STR8("  Using stdio's printf functions\n"));
        #else
        DN_Str8Builder_AppendRef(&builder, DN_STR8("  Using stb_sprintf functions\n"));
        #endif

        // TODO(doyle): Add stacktrace feature log
    }

    if (on_init & DN_CoreOnInit_LogCPUFeatures) {
        DN_CPUReport const *report = &core->cpu_report;
        DN_Str8 brand              = DN_Str8_TrimWhitespaceAround(DN_Str8_Init(report->brand, sizeof(report->brand) - 1));
        DN_Str8Builder_AppendF(&builder,
                                "  CPU '%.*s' from '%s' detected:\n",
                                DN_STR_FMT(brand),
                                report->vendor);

        DN_USize longest_feature_name = 0;
        DN_FOR_UINDEX(feature_index, DN_CPUFeature_Count) {
            DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
            longest_feature_name = DN_MAX(longest_feature_name, feature_decl.label.size);
        }

        DN_FOR_UINDEX(feature_index, DN_CPUFeature_Count) {
            DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
            bool               has_feature  = DN_CPU_HasFeature(report, feature_decl.value);
            DN_Str8Builder_AppendF(&builder,
                                    "    %.*s:%*s%s\n",
                                    DN_STR_FMT(feature_decl.label),
                                    DN_CAST(int)(longest_feature_name - feature_decl.label.size),
                                    "",
                                    has_feature ? "available" : "not available");
        }
    }

    DN_Str8 info_log = DN_Str8Builder_Build(&builder, tmem.arena);
    if (DN_Str8_HasData(info_log))
        DN_Log_DebugF("%.*s", DN_STR_FMT(info_log));
}

DN_API void DN_Core_BeginFrame()
{
    DN_Atomic_SetValue64(&g_dn_core->mem_allocs_frame, 0);
}

DN_API void DN_Core_SetPointer(DN_Core *library)
{
    if (library) {
        g_dn_core = library;
        DN_OS_ThreadSetTLS(&library->tls);
    }
}

#if !defined(DN_NO_PROFILER)
DN_API void DN_Core_SetProfiler(DN_Profiler *profiler)
{
    if (profiler)
        g_dn_core->profiler = profiler;
}
#endif

DN_API void DN_Core_SetLogCallback(DN_LogProc *proc, void *user_data)
{
    g_dn_core->log_callback  = proc;
    g_dn_core->log_user_data = user_data;
}

DN_API void DN_Core_DumpThreadContextArenaStat(DN_Str8 file_path)
{
#if defined(DN_DEBUG_THREAD_CONTEXT)
    // NOTE: Open a file to write the arena stats to
    FILE *file = nullptr;
    fopen_s(&file, file_path.data, "a+b");
    if (file) {
        DN_Log_ErrorF("Failed to dump thread context arenas [file=%.*s]", DN_STR_FMT(file_path));
        return;
    }

    // NOTE: Copy the stats from library book-keeping
    // NOTE: Extremely short critical section, copy the stats then do our
    // work on it.
    DN_ArenaStat stats[DN_CArray_CountI(g_dn_core->thread_context_arena_stats)];
    int           stats_size = 0;

    DN_TicketMutex_Begin(&g_dn_core->thread_context_mutex);
    stats_size = g_dn_core->thread_context_arena_stats_count;
    DN_MEMCPY(stats, g_dn_core->thread_context_arena_stats, sizeof(stats[0]) * stats_size);
    DN_TicketMutex_End(&g_dn_core->thread_context_mutex);

    // NOTE: Print the cumulative stat
    DN_DateHMSTimeStr now = DN_Date_HMSLocalTimeStrNow();
    fprintf(file,
            "Time=%.*s %.*s | Thread Context Arenas | Count=%d\n",
            now.date_size,
            now.date,
            now.hms_size,
            now.hms,
            g_dn_core->thread_context_arena_stats_count);

    // NOTE: Write the cumulative thread arena data
    {
        DN_ArenaStat stat = {};
        for (DN_USize index = 0; index < stats_size; index++) {
            DN_ArenaStat const *current = stats + index;
            stat.capacity += current->capacity;
            stat.used += current->used;
            stat.wasted += current->wasted;
            stat.blocks += current->blocks;

            stat.capacity_hwm = DN_MAX(stat.capacity_hwm, current->capacity_hwm);
            stat.used_hwm     = DN_MAX(stat.used_hwm, current->used_hwm);
            stat.wasted_hwm   = DN_MAX(stat.wasted_hwm, current->wasted_hwm);
            stat.blocks_hwm   = DN_MAX(stat.blocks_hwm, current->blocks_hwm);
        }

        DN_ArenaStatStr stats_string = DN_Arena_StatStr(&stat);
        fprintf(file, "  [ALL] CURR %.*s\n", stats_string.size, stats_string.data);
    }

    // NOTE: Print individual thread arena data
    for (DN_USize index = 0; index < stats_size; index++) {
        DN_ArenaStat const *current        = stats + index;
        DN_ArenaStatStr     current_string = DN_Arena_StatStr(current);
        fprintf(file, "  [%03d] CURR %.*s\n", DN_CAST(int) index, current_string.size, current_string.data);
    }

    fclose(file);
    DN_Log_InfoF("Dumped thread context arenas [file=%.*s]", DN_STR_FMT(file_path));
#else
    (void)file_path;
#endif // #if defined(DN_DEBUG_THREAD_CONTEXT)
}

DN_API DN_Arena *DN_Core_AllocArenaF(DN_USize reserve, DN_USize commit, uint8_t arena_flags, char const *fmt, ...)
{
    DN_ASSERT(g_dn_core->init);
    va_list args;
    va_start(args, fmt);
    DN_ArenaCatalog *catalog = &g_dn_core->arena_catalog;
    DN_Arena        *result  = DN_ArenaCatalog_AllocFV(catalog, reserve, commit, arena_flags, fmt, args);
    va_end(args);
    return result;
}

DN_API bool DN_Core_EraseArena(DN_Arena *arena, DN_ArenaCatalogFreeArena free_arena)
{
    DN_ArenaCatalog *catalog = &g_dn_core->arena_catalog;
    bool result = DN_ArenaCatalog_Erase(catalog, arena, free_arena);
    return result;
}

#if !defined(DN_NO_PROFILER)
// NOTE: [$PROF] DN_Profiler //////////////////////////////////////////////////////////////////////
DN_API DN_ProfilerZoneScope::DN_ProfilerZoneScope(DN_Str8 name, uint16_t anchor_index)
{
    zone = DN_Profiler_BeginZoneAtIndex(name, anchor_index);
}

DN_API DN_ProfilerZoneScope::~DN_ProfilerZoneScope()
{
    DN_Profiler_EndZone(zone);
}

DN_API DN_ProfilerAnchor *DN_Profiler_ReadBuffer()
{
    uint8_t mask               = DN_ARRAY_UCOUNT(g_dn_core->profiler->anchors) - 1;
    DN_ProfilerAnchor *result = g_dn_core->profiler->anchors[(g_dn_core->profiler->active_anchor_buffer - 1) & mask];
    return result;
}

DN_API DN_ProfilerAnchor *DN_Profiler_WriteBuffer()
{
    uint8_t mask               = DN_ARRAY_UCOUNT(g_dn_core->profiler->anchors) - 1;
    DN_ProfilerAnchor *result = g_dn_core->profiler->anchors[(g_dn_core->profiler->active_anchor_buffer + 0) & mask];
    return result;
}

DN_API DN_ProfilerZone DN_Profiler_BeginZoneAtIndex(DN_Str8 name, uint16_t anchor_index)
{
    DN_ProfilerAnchor *anchor           = DN_Profiler_WriteBuffer() + anchor_index;
    // TODO: We need per-thread-local-storage profiler so that we can use these apis
    // across threads. For now, we let them overwrite each other but this is not tenable.
#if 0
    if (DN_Str8_HasData(anchor->name) && anchor->name != name)
        DN_ASSERTF(name == anchor->name, "Potentially overwriting a zone by accident? Anchor is '%.*s', name is '%.*s'", DN_STR_FMT(anchor->name), DN_STR_FMT(name));
#endif
    anchor->name                      = name;
    DN_ProfilerZone result           = {};
    result.begin_tsc                  = DN_CPU_TSC();
    result.anchor_index               = anchor_index;
    result.parent_zone                = g_dn_core->profiler->parent_zone;
    result.elapsed_tsc_at_zone_start  = anchor->tsc_inclusive;
    g_dn_core->profiler->parent_zone = anchor_index;
    return result;
}

DN_API void DN_Profiler_EndZone(DN_ProfilerZone zone)
{
    uint64_t            elapsed_tsc   = DN_CPU_TSC() - zone.begin_tsc;
    DN_ProfilerAnchor *anchor_buffer = DN_Profiler_WriteBuffer();
    DN_ProfilerAnchor *anchor        = anchor_buffer + zone.anchor_index;

    anchor->hit_count++;
    anchor->tsc_inclusive = zone.elapsed_tsc_at_zone_start + elapsed_tsc;
    anchor->tsc_exclusive += elapsed_tsc;

    DN_ProfilerAnchor *parent_anchor = anchor_buffer + zone.parent_zone;
    parent_anchor->tsc_exclusive -= elapsed_tsc;
    g_dn_core->profiler->parent_zone = zone.parent_zone;
}

DN_API void DN_Profiler_SwapAnchorBuffer()
{
    g_dn_core->profiler->active_anchor_buffer++;
    g_dn_core->profiler->parent_zone = 0;
    DN_ProfilerAnchor *anchors = DN_Profiler_WriteBuffer();
    DN_MEMSET(anchors,
               0,
               DN_ARRAY_UCOUNT(g_dn_core->profiler->anchors[0]) * sizeof(g_dn_core->profiler->anchors[0][0]));
}

DN_API void DN_Profiler_Dump(uint64_t tsc_per_second)
{
    DN_ProfilerAnchor *anchors = DN_Profiler_ReadBuffer();
    for (size_t anchor_index = 1; anchor_index < DN_PROFILER_ANCHOR_BUFFER_SIZE; anchor_index++) {
        DN_ProfilerAnchor const *anchor = anchors + anchor_index;
        if (!anchor->hit_count)
            continue;

        uint64_t tsc_exclusive              = anchor->tsc_exclusive;
        uint64_t tsc_inclusive              = anchor->tsc_inclusive;
        DN_F64  tsc_exclusive_milliseconds = tsc_exclusive * 1000 / DN_CAST(DN_F64) tsc_per_second;
        if (tsc_exclusive == tsc_inclusive) {
            DN_Print_LnF("%.*s[%u]: %.1fms", DN_STR_FMT(anchor->name), anchor->hit_count, tsc_exclusive_milliseconds);
        } else {
            DN_F64 tsc_inclusive_milliseconds = tsc_inclusive * 1000 / DN_CAST(DN_F64) tsc_per_second;
            DN_Print_LnF("%.*s[%u]: %.1f/%.1fms",
                          DN_STR_FMT(anchor->name),
                          anchor->hit_count,
                          tsc_exclusive_milliseconds,
                          tsc_inclusive_milliseconds);
        }
    }
}
#endif // !defined(DN_NO_PROFILER)

// NOTE: [$JOBQ] DN_JobQueue ///////////////////////////////////////////////////////////////////////
DN_API DN_JobQueueSPMC DN_OS_JobQueueSPMCInit()
{
    DN_JobQueueSPMC result               = {};
    result.thread_wait_for_job_semaphore  = DN_OS_SemaphoreInit(0 /*initial_count*/);
    result.wait_for_completion_semaphore  = DN_OS_SemaphoreInit(0 /*initial_count*/);
    result.complete_queue_write_semaphore = DN_OS_SemaphoreInit(DN_ARRAY_UCOUNT(result.complete_queue));
    result.mutex                          = DN_OS_MutexInit();
    return result;
}

DN_API bool DN_OS_JobQueueSPMCCanAdd(DN_JobQueueSPMC const *queue, uint32_t count)
{
    uint32_t read_index  = queue->read_index;
    uint32_t write_index = queue->write_index;
    uint32_t size        = write_index - read_index;
    bool     result      = (size + count) <= DN_ARRAY_UCOUNT(queue->jobs);
    return result;
}

DN_API bool DN_OS_JobQueueSPMCAddArray(DN_JobQueueSPMC *queue, DN_Job *jobs, uint32_t count)
{
    if (!queue)
        return false;

    uint32_t const pot_mask    = DN_ARRAY_UCOUNT(queue->jobs) - 1;
    uint32_t       read_index  = queue->read_index;
    uint32_t       write_index = queue->write_index;
    uint32_t       size        = write_index - read_index;

    if ((size + count) > DN_ARRAY_UCOUNT(queue->jobs))
        return false;

    for (size_t offset = 0; offset < count; offset++) {
        uint32_t wrapped_write_index     = (write_index + offset) & pot_mask;
        queue->jobs[wrapped_write_index] = jobs[offset];
    }

    DN_OS_MutexLock(&queue->mutex);
    queue->write_index += count;
    DN_OS_SemaphoreIncrement(&queue->thread_wait_for_job_semaphore, count);
    DN_OS_MutexUnlock(&queue->mutex);
    return true;
}

DN_API bool DN_OS_JobQueueSPMCAdd(DN_JobQueueSPMC *queue, DN_Job job)
{
    bool result = DN_OS_JobQueueSPMCAddArray(queue, &job, 1);
    return result;
}

DN_API int32_t DN_OS_JobQueueSPMCThread(DN_OSThread *thread)
{
    DN_JobQueueSPMC *queue    = DN_CAST(DN_JobQueueSPMC *) thread->user_context;
    uint32_t const    pot_mask = DN_ARRAY_UCOUNT(queue->jobs) - 1;
    static_assert(DN_ARRAY_UCOUNT(queue->jobs) == DN_ARRAY_UCOUNT(queue->complete_queue), "PoT mask is used to mask access to both arrays");

    for (;;) {
        DN_OS_SemaphoreWait(&queue->thread_wait_for_job_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
        if (queue->quit)
            break;

        DN_ASSERT(queue->read_index != queue->write_index);

        DN_OS_MutexLock(&queue->mutex);
        uint32_t wrapped_read_index = queue->read_index & pot_mask;
        DN_Job  job                = queue->jobs[wrapped_read_index];
        queue->read_index += 1;
        DN_OS_MutexUnlock(&queue->mutex);

        job.elapsed_tsc -= DN_CPU_TSC();
        job.func(thread, job.user_context);
        job.elapsed_tsc += DN_CPU_TSC();

        if (job.add_to_completion_queue) {
            DN_OS_SemaphoreWait(&queue->complete_queue_write_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
            DN_OS_MutexLock(&queue->mutex);
            queue->complete_queue[(queue->complete_write_index++ & pot_mask)] = job;
            DN_OS_MutexUnlock(&queue->mutex);
            DN_OS_SemaphoreIncrement(&queue->complete_queue_write_semaphore, 1);
        }

        // NOTE: Update finish counter
        DN_OS_MutexLock(&queue->mutex);
        queue->finish_index += 1;

        // NOTE: If all jobs are finished and we have another thread who is
        // blocked via `WaitForCompletion` for this job queue, we will go
        // release the semaphore to wake them all up.
        bool all_jobs_finished = queue->finish_index == queue->write_index;
        if (all_jobs_finished && queue->threads_waiting_for_completion) {
            DN_OS_SemaphoreIncrement(&queue->wait_for_completion_semaphore,
                                      queue->threads_waiting_for_completion);
            queue->threads_waiting_for_completion = 0;
        }
        DN_OS_MutexUnlock(&queue->mutex);
    }

    return queue->quit_exit_code;
}

DN_API void DN_OS_JobQueueSPMCWaitForCompletion(DN_JobQueueSPMC *queue)
{
    DN_OS_MutexLock(&queue->mutex);
    if (queue->finish_index == queue->write_index) {
        DN_OS_MutexUnlock(&queue->mutex);
        return;
    }
    queue->threads_waiting_for_completion++;
    DN_OS_MutexUnlock(&queue->mutex);

    DN_OS_SemaphoreWait(&queue->wait_for_completion_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DN_API DN_USize DN_OS_JobQueueSPMCGetFinishedJobs(DN_JobQueueSPMC *queue, DN_Job *jobs, DN_USize jobs_size)
{
    DN_USize result = 0;
    if (!queue || !jobs || jobs_size <= 0)
        return result;

    uint32_t const pot_mask = DN_ARRAY_UCOUNT(queue->jobs) - 1;
    DN_OS_MutexLock(&queue->mutex);
    while (queue->complete_read_index < queue->complete_write_index && result < jobs_size) {
        jobs[result++] = queue->complete_queue[(queue->complete_read_index++ & pot_mask)];
    }
    DN_OS_MutexUnlock(&queue->mutex);

    return result;
}
