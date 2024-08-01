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

// NOTE: [$PCGX] Dqn_PCG32 /////////////////////////////////////////////////////////////////////////
#define DQN_PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define DQN_PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL

DQN_API Dqn_PCG32 Dqn_PCG32_Init(uint64_t seed)
{
    Dqn_PCG32 result = {};
    Dqn_PCG32_Next(&result);
    result.state += seed;
    Dqn_PCG32_Next(&result);
    return result;
}

DQN_API uint32_t Dqn_PCG32_Next(Dqn_PCG32 *rng)
{
    uint64_t state = rng->state;
    rng->state     = state * DQN_PCG_DEFAULT_MULTIPLIER_64 + DQN_PCG_DEFAULT_INCREMENT_64;

    // XSH-RR
    uint32_t value = (uint32_t)((state ^ (state >> 18)) >> 27);
    int      rot   = state >> 59;
    return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

DQN_API uint64_t Dqn_PCG32_Next64(Dqn_PCG32 *rng)
{
    uint64_t value = Dqn_PCG32_Next(rng);
    value <<= 32;
    value |= Dqn_PCG32_Next(rng);
    return value;
}

DQN_API uint32_t Dqn_PCG32_Range(Dqn_PCG32 *rng, uint32_t low, uint32_t high)
{
    uint32_t bound     = high - low;
    uint32_t threshold = -(int32_t)bound % bound;

    for (;;) {
        uint32_t r = Dqn_PCG32_Next(rng);
        if (r >= threshold)
            return low + (r % bound);
    }
}

DQN_API float Dqn_PCG32_NextF32(Dqn_PCG32 *rng)
{
    uint32_t x = Dqn_PCG32_Next(rng);
    return (float)(int32_t)(x >> 8) * 0x1.0p-24f;
}

DQN_API double Dqn_PCG32_NextF64(Dqn_PCG32 *rng)
{
    uint64_t x = Dqn_PCG32_Next64(rng);
    return (double)(int64_t)(x >> 11) * 0x1.0p-53;
}

DQN_API void Dqn_PCG32_Advance(Dqn_PCG32 *rng, uint64_t delta)
{
    uint64_t cur_mult = DQN_PCG_DEFAULT_MULTIPLIER_64;
    uint64_t cur_plus = DQN_PCG_DEFAULT_INCREMENT_64;

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

#if !defined(DQN_NO_JSON_BUILDER)
// NOTE: [$JSON] Dqn_JSONBuilder ///////////////////////////////////////////////////////////////////
DQN_API Dqn_JSONBuilder Dqn_JSONBuilder_Init(Dqn_Arena *arena, int spaces_per_indent)
{
    Dqn_JSONBuilder result      = {};
    result.spaces_per_indent    = spaces_per_indent;
    result.string_builder.arena = arena;
    return result;
}

DQN_API Dqn_Str8 Dqn_JSONBuilder_Build(Dqn_JSONBuilder const *builder, Dqn_Arena *arena)
{
    Dqn_Str8 result = Dqn_Str8Builder_Build(&builder->string_builder, arena);
    return result;
}

DQN_API void Dqn_JSONBuilder_KeyValue(Dqn_JSONBuilder *builder, Dqn_Str8 key, Dqn_Str8 value)
{
    if (key.size == 0 && value.size == 0)
        return;

    Dqn_JSONBuilderItem item = Dqn_JSONBuilderItem_KeyValue;
    if (value.size == 1) {
        if (value.data[0] == '{' || value.data[0] == '[')
            item = Dqn_JSONBuilderItem_OpenContainer;
        else if (value.data[0] == '}' || value.data[0] == ']')
            item = Dqn_JSONBuilderItem_CloseContainer;
    }

    bool adding_to_container_with_items =
        item != Dqn_JSONBuilderItem_CloseContainer && (builder->last_item == Dqn_JSONBuilderItem_KeyValue ||
                                                       builder->last_item == Dqn_JSONBuilderItem_CloseContainer);

    uint8_t prefix_size = 0;
    char    prefix[2]   = {0};
    if (adding_to_container_with_items)
        prefix[prefix_size++] = ',';

    if (builder->last_item != Dqn_JSONBuilderItem_Empty)
        prefix[prefix_size++] = '\n';

    if (item == Dqn_JSONBuilderItem_CloseContainer)
        builder->indent_level--;

    int spaces_per_indent = builder->spaces_per_indent ? builder->spaces_per_indent : 2;
    int spaces            = builder->indent_level * spaces_per_indent;

    if (key.size) {
        Dqn_Str8Builder_AddF(&builder->string_builder,
                             "%.*s%*c\"%.*s\": %.*s",
                             prefix_size,
                             prefix,
                             spaces,
                             ' ',
                             DQN_STR_FMT(key),
                             DQN_STR_FMT(value));
    } else {
        Dqn_Str8Builder_AddF(&builder->string_builder, "%.*s%*c%.*s", prefix_size, prefix, spaces, ' ', DQN_STR_FMT(value));
    }

    if (item == Dqn_JSONBuilderItem_OpenContainer)
        builder->indent_level++;

    builder->last_item = item;
}

DQN_API void Dqn_JSONBuilder_KeyValueFV(Dqn_JSONBuilder *builder, Dqn_Str8 key, char const *value_fmt, va_list args)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(builder->string_builder.arena);
    Dqn_Str8    value   = Dqn_Str8_InitFV(tmem.arena, value_fmt, args);
    Dqn_JSONBuilder_KeyValue(builder, key, value);
}

DQN_API void Dqn_JSONBuilder_KeyValueF(Dqn_JSONBuilder *builder, Dqn_Str8 key, char const *value_fmt, ...)
{
    va_list args;
    va_start(args, value_fmt);
    Dqn_JSONBuilder_KeyValueFV(builder, key, value_fmt, args);
    va_end(args);
}

DQN_API void Dqn_JSONBuilder_ObjectBeginNamed(Dqn_JSONBuilder *builder, Dqn_Str8 name)
{
    Dqn_JSONBuilder_KeyValue(builder, name, DQN_STR8("{"));
}

DQN_API void Dqn_JSONBuilder_ObjectEnd(Dqn_JSONBuilder *builder)
{
    Dqn_JSONBuilder_KeyValue(builder, DQN_STR8(""), DQN_STR8("}"));
}

DQN_API void Dqn_JSONBuilder_ArrayBeginNamed(Dqn_JSONBuilder *builder, Dqn_Str8 name)
{
    Dqn_JSONBuilder_KeyValue(builder, name, DQN_STR8("["));
}

DQN_API void Dqn_JSONBuilder_ArrayEnd(Dqn_JSONBuilder *builder)
{
    Dqn_JSONBuilder_KeyValue(builder, DQN_STR8(""), DQN_STR8("]"));
}

DQN_API void Dqn_JSONBuilder_StrNamed(Dqn_JSONBuilder *builder, Dqn_Str8 key, Dqn_Str8 value)
{
    Dqn_JSONBuilder_KeyValueF(builder, key, "\"%.*s\"", value.size, value.data);
}

DQN_API void Dqn_JSONBuilder_LiteralNamed(Dqn_JSONBuilder *builder, Dqn_Str8 key, Dqn_Str8 value)
{
    Dqn_JSONBuilder_KeyValueF(builder, key, "%.*s", value.size, value.data);
}

DQN_API void Dqn_JSONBuilder_U64Named(Dqn_JSONBuilder *builder, Dqn_Str8 key, uint64_t value)
{
    Dqn_JSONBuilder_KeyValueF(builder, key, "%I64u", value);
}

DQN_API void Dqn_JSONBuilder_I64Named(Dqn_JSONBuilder *builder, Dqn_Str8 key, int64_t value)
{
    Dqn_JSONBuilder_KeyValueF(builder, key, "%I64d", value);
}

DQN_API void Dqn_JSONBuilder_F64Named(Dqn_JSONBuilder *builder, Dqn_Str8 key, double value, int decimal_places)
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
        DQN_SNPRINTF(float_fmt, sizeof(float_fmt), "%%.%df", decimal_places);
    } else {
        // NOTE: Emit the format string "%f"
        DQN_SNPRINTF(float_fmt, sizeof(float_fmt), "%%f");
    }

    char fmt[32];
    if (key.size)
        DQN_SNPRINTF(fmt, sizeof(fmt), "\"%%.*s\": %s", float_fmt);
    else
        DQN_SNPRINTF(fmt, sizeof(fmt), "%s", float_fmt);

    Dqn_JSONBuilder_KeyValueF(builder, key, fmt, value);
}

DQN_API void Dqn_JSONBuilder_BoolNamed(Dqn_JSONBuilder *builder, Dqn_Str8 key, bool value)
{
    Dqn_Str8 value_string = value ? DQN_STR8("true") : DQN_STR8("false");
    Dqn_JSONBuilder_KeyValueF(builder, key, "%.*s", value_string.size, value_string.data);
}
#endif // !defined(DQN_NO_JSON_BUILDER)

// NOTE: [$BITS] Dqn_Bit ///////////////////////////////////////////////////////////////////////////
DQN_API void Dqn_Bit_UnsetInplace(Dqn_usize *flags, Dqn_usize bitfield)
{
    *flags = (*flags & ~bitfield);
}

DQN_API void Dqn_Bit_SetInplace(Dqn_usize *flags, Dqn_usize bitfield)
{
    *flags = (*flags | bitfield);
}

DQN_API bool Dqn_Bit_IsSet(Dqn_usize bits, Dqn_usize bits_to_set)
{
    auto result = DQN_CAST(bool)((bits & bits_to_set) == bits_to_set);
    return result;
}

DQN_API bool Dqn_Bit_IsNotSet(Dqn_usize bits, Dqn_usize bits_to_check)
{
    auto result = !Dqn_Bit_IsSet(bits, bits_to_check);
    return result;
}

// NOTE: [$SAFE] Dqn_Safe //////////////////////////////////////////////////////////////////////////
DQN_API int64_t Dqn_Safe_AddI64(int64_t a, int64_t b)
{
    int64_t result = DQN_CHECKF(a <= INT64_MAX - b, "a=%zd, b=%zd", a, b) ? (a + b) : INT64_MAX;
    return result;
}

DQN_API int64_t Dqn_Safe_MulI64(int64_t a, int64_t b)
{
    int64_t result = DQN_CHECKF(a <= INT64_MAX / b, "a=%zd, b=%zd", a, b) ? (a * b) : INT64_MAX;
    return result;
}

DQN_API uint64_t Dqn_Safe_AddU64(uint64_t a, uint64_t b)
{
    uint64_t result = DQN_CHECKF(a <= UINT64_MAX - b, "a=%zu, b=%zu", a, b) ? (a + b) : UINT64_MAX;
    return result;
}

DQN_API uint64_t Dqn_Safe_SubU64(uint64_t a, uint64_t b)
{
    uint64_t result = DQN_CHECKF(a >= b, "a=%zu, b=%zu", a, b) ? (a - b) : 0;
    return result;
}

DQN_API uint64_t Dqn_Safe_MulU64(uint64_t a, uint64_t b)
{
    uint64_t result = DQN_CHECKF(a <= UINT64_MAX / b, "a=%zu, b=%zu", a, b) ? (a * b) : UINT64_MAX;
    return result;
}

DQN_API uint32_t Dqn_Safe_SubU32(uint32_t a, uint32_t b)
{
    uint32_t result = DQN_CHECKF(a >= b, "a=%u, b=%u", a, b) ? (a - b) : 0;
    return result;
}

// NOTE: Dqn_Safe_SaturateCastUSizeToI* ////////////////////////////////////////////////////////////
// INT*_MAX literals will be promoted to the type of uintmax_t as uintmax_t is
// the highest possible rank (unsigned > signed).
DQN_API int Dqn_Safe_SaturateCastUSizeToInt(Dqn_usize val)
{
    int result = DQN_CHECK(DQN_CAST(uintmax_t) val <= INT_MAX) ? DQN_CAST(int) val : INT_MAX;
    return result;
}

DQN_API int8_t Dqn_Safe_SaturateCastUSizeToI8(Dqn_usize val)
{
    int8_t result = DQN_CHECK(DQN_CAST(uintmax_t) val <= INT8_MAX) ? DQN_CAST(int8_t) val : INT8_MAX;
    return result;
}

DQN_API int16_t Dqn_Safe_SaturateCastUSizeToI16(Dqn_usize val)
{
    int16_t result = DQN_CHECK(DQN_CAST(uintmax_t) val <= INT16_MAX) ? DQN_CAST(int16_t) val : INT16_MAX;
    return result;
}

DQN_API int32_t Dqn_Safe_SaturateCastUSizeToI32(Dqn_usize val)
{
    int32_t result = DQN_CHECK(DQN_CAST(uintmax_t) val <= INT32_MAX) ? DQN_CAST(int32_t) val : INT32_MAX;
    return result;
}

DQN_API int64_t Dqn_Safe_SaturateCastUSizeToI64(Dqn_usize val)
{
    int64_t result = DQN_CHECK(DQN_CAST(uintmax_t) val <= INT64_MAX) ? DQN_CAST(int64_t) val : INT64_MAX;
    return result;
}

// NOTE: Dqn_Safe_SaturateCastUSizeToU* ////////////////////////////////////////////////////////////
// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DQN_API uint8_t Dqn_Safe_SaturateCastUSizeToU8(Dqn_usize val)
{
    uint8_t result = DQN_CHECK(val <= UINT8_MAX) ? DQN_CAST(uint8_t) val : UINT8_MAX;
    return result;
}

DQN_API uint16_t Dqn_Safe_SaturateCastUSizeToU16(Dqn_usize val)
{
    uint16_t result = DQN_CHECK(val <= UINT16_MAX) ? DQN_CAST(uint16_t) val : UINT16_MAX;
    return result;
}

DQN_API uint32_t Dqn_Safe_SaturateCastUSizeToU32(Dqn_usize val)
{
    uint32_t result = DQN_CHECK(val <= UINT32_MAX) ? DQN_CAST(uint32_t) val : UINT32_MAX;
    return result;
}

DQN_API uint64_t Dqn_Safe_SaturateCastUSizeToU64(Dqn_usize val)
{
    uint64_t result = DQN_CHECK(DQN_CAST(uint64_t) val <= UINT64_MAX) ? DQN_CAST(uint64_t) val : UINT64_MAX;
    return result;
}

// NOTE: Dqn_Safe_SaturateCastU64To* ///////////////////////////////////////////////////////////////
DQN_API int Dqn_Safe_SaturateCastU64ToInt(uint64_t val)
{
    int result = DQN_CHECK(val <= INT_MAX) ? DQN_CAST(int)val : INT_MAX;
    return result;
}

DQN_API int64_t Dqn_Safe_SaturateCastU64ToI64(uint64_t val)
{
    int64_t result = DQN_CHECK(val <= INT64_MAX) ? DQN_CAST(int64_t)val : INT64_MAX;
    return result;
}

// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DQN_API unsigned int Dqn_Safe_SaturateCastU64ToUInt(uint64_t val)
{
    unsigned int result = DQN_CHECK(val <= UINT8_MAX) ? DQN_CAST(unsigned int) val : UINT_MAX;
    return result;
}

DQN_API uint8_t Dqn_Safe_SaturateCastU64ToU8(uint64_t val)
{
    uint8_t result = DQN_CHECK(val <= UINT8_MAX) ? DQN_CAST(uint8_t) val : UINT8_MAX;
    return result;
}

DQN_API uint16_t Dqn_Safe_SaturateCastU64ToU16(uint64_t val)
{
    uint16_t result = DQN_CHECK(val <= UINT16_MAX) ? DQN_CAST(uint16_t) val : UINT16_MAX;
    return result;
}

DQN_API uint32_t Dqn_Safe_SaturateCastU64ToU32(uint64_t val)
{
    uint32_t result = DQN_CHECK(val <= UINT32_MAX) ? DQN_CAST(uint32_t) val : UINT32_MAX;
    return result;
}

// NOTE: Dqn_Safe_SaturateCastISizeToI* ////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DQN_API int Dqn_Safe_SaturateCastISizeToInt(Dqn_isize val)
{
    DQN_ASSERT(val >= INT_MIN && val <= INT_MAX);
    int result = DQN_CAST(int) DQN_CLAMP(val, INT_MIN, INT_MAX);
    return result;
}

DQN_API int8_t Dqn_Safe_SaturateCastISizeToI8(Dqn_isize val)
{
    DQN_ASSERT(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DQN_CAST(int8_t) DQN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DQN_API int16_t Dqn_Safe_SaturateCastISizeToI16(Dqn_isize val)
{
    DQN_ASSERT(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DQN_CAST(int16_t) DQN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DQN_API int32_t Dqn_Safe_SaturateCastISizeToI32(Dqn_isize val)
{
    DQN_ASSERT(val >= INT32_MIN && val <= INT32_MAX);
    int32_t result = DQN_CAST(int32_t) DQN_CLAMP(val, INT32_MIN, INT32_MAX);
    return result;
}

DQN_API int64_t Dqn_Safe_SaturateCastISizeToI64(Dqn_isize val)
{
    DQN_ASSERT(DQN_CAST(int64_t) val >= INT64_MIN && DQN_CAST(int64_t) val <= INT64_MAX);
    int64_t result = DQN_CAST(int64_t) DQN_CLAMP(DQN_CAST(int64_t) val, INT64_MIN, INT64_MAX);
    return result;
}

// NOTE: Dqn_Safe_SaturateCastISizeToU* ////////////////////////////////////////////////////////////
// If the value is a negative integer, we clamp to 0. Otherwise, we know that
// the value is >=0, we can upcast safely to bounds check against the maximum
// allowed value.
DQN_API unsigned int Dqn_Safe_SaturateCastISizeToUInt(Dqn_isize val)
{
    unsigned int result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT_MAX))
            result = DQN_CAST(unsigned int) val;
        else
            result = UINT_MAX;
    }
    return result;
}

DQN_API uint8_t Dqn_Safe_SaturateCastISizeToU8(Dqn_isize val)
{
    uint8_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DQN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DQN_API uint16_t Dqn_Safe_SaturateCastISizeToU16(Dqn_isize val)
{
    uint16_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DQN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DQN_API uint32_t Dqn_Safe_SaturateCastISizeToU32(Dqn_isize val)
{
    uint32_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT32_MAX))
            result = DQN_CAST(uint32_t) val;
        else
            result = UINT32_MAX;
    }
    return result;
}

DQN_API uint64_t Dqn_Safe_SaturateCastISizeToU64(Dqn_isize val)
{
    uint64_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT64_MAX))
            result = DQN_CAST(uint64_t) val;
        else
            result = UINT64_MAX;
    }
    return result;
}

// NOTE: Dqn_Safe_SaturateCastI64To* ///////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DQN_API Dqn_isize Dqn_Safe_SaturateCastI64ToISize(int64_t val)
{
    DQN_CHECK(val >= DQN_ISIZE_MIN && val <= DQN_ISIZE_MAX);
    Dqn_isize result = DQN_CAST(int64_t) DQN_CLAMP(val, DQN_ISIZE_MIN, DQN_ISIZE_MAX);
    return result;
}

DQN_API int8_t Dqn_Safe_SaturateCastI64ToI8(int64_t val)
{
    DQN_CHECK(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DQN_CAST(int8_t) DQN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DQN_API int16_t Dqn_Safe_SaturateCastI64ToI16(int64_t val)
{
    DQN_CHECK(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DQN_CAST(int16_t) DQN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DQN_API int32_t Dqn_Safe_SaturateCastI64ToI32(int64_t val)
{
    DQN_CHECK(val >= INT32_MIN && val <= INT32_MAX);
    int32_t result = DQN_CAST(int32_t) DQN_CLAMP(val, INT32_MIN, INT32_MAX);
    return result;
}

DQN_API unsigned int Dqn_Safe_SaturateCastI64ToUInt(int64_t val)
{
    unsigned int result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT_MAX))
            result = DQN_CAST(unsigned int) val;
        else
            result = UINT_MAX;
    }
    return result;
}

DQN_API Dqn_isize Dqn_Safe_SaturateCastI64ToUSize(int64_t val)
{
    Dqn_usize result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= DQN_USIZE_MAX))
            result = DQN_CAST(Dqn_usize) val;
        else
            result = DQN_USIZE_MAX;
    }
    return result;
}

DQN_API uint8_t Dqn_Safe_SaturateCastI64ToU8(int64_t val)
{
    uint8_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DQN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DQN_API uint16_t Dqn_Safe_SaturateCastI64ToU16(int64_t val)
{
    uint16_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DQN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DQN_API uint32_t Dqn_Safe_SaturateCastI64ToU32(int64_t val)
{
    uint32_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT32_MAX))
            result = DQN_CAST(uint32_t) val;
        else
            result = UINT32_MAX;
    }
    return result;
}

DQN_API uint64_t Dqn_Safe_SaturateCastI64ToU64(int64_t val)
{
    uint64_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(int64_t) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT64_MAX))
            result = DQN_CAST(uint64_t) val;
        else
            result = UINT64_MAX;
    }
    return result;
}

// NOTE: Dqn_Safe_SaturateCastIntTo* ///////////////////////////////////////////////////////////////
DQN_API int8_t Dqn_Safe_SaturateCastIntToI8(int val)
{
    DQN_CHECK(val >= INT8_MIN && val <= INT8_MAX);
    int8_t result = DQN_CAST(int8_t) DQN_CLAMP(val, INT8_MIN, INT8_MAX);
    return result;
}

DQN_API int16_t Dqn_Safe_SaturateCastIntToI16(int val)
{
    DQN_CHECK(val >= INT16_MIN && val <= INT16_MAX);
    int16_t result = DQN_CAST(int16_t) DQN_CLAMP(val, INT16_MIN, INT16_MAX);
    return result;
}

DQN_API uint8_t Dqn_Safe_SaturateCastIntToU8(int val)
{
    uint8_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT8_MAX))
            result = DQN_CAST(uint8_t) val;
        else
            result = UINT8_MAX;
    }
    return result;
}

DQN_API uint16_t Dqn_Safe_SaturateCastIntToU16(int val)
{
    uint16_t result = 0;
    if (DQN_CHECK(val >= DQN_CAST(Dqn_isize) 0)) {
        if (DQN_CHECK(DQN_CAST(uintmax_t) val <= UINT16_MAX))
            result = DQN_CAST(uint16_t) val;
        else
            result = UINT16_MAX;
    }
    return result;
}

DQN_API uint32_t Dqn_Safe_SaturateCastIntToU32(int val)
{
    static_assert(sizeof(val) <= sizeof(uint32_t), "Sanity check to allow simplifying of casting");
    uint32_t result = 0;
    if (DQN_CHECK(val >= 0))
        result = DQN_CAST(uint32_t) val;
    return result;
}

DQN_API uint64_t Dqn_Safe_SaturateCastIntToU64(int val)
{
    static_assert(sizeof(val) <= sizeof(uint64_t), "Sanity check to allow simplifying of casting");
    uint64_t result = 0;
    if (DQN_CHECK(val >= 0))
        result = DQN_CAST(uint64_t) val;
    return result;
}

// NOTE: [$MISC] Misc //////////////////////////////////////////////////////////////////////////////
DQN_API int Dqn_FmtBuffer3DotTruncate(char *buffer, int size, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size_required = DQN_VSNPRINTF(buffer, size, fmt, args);
    int result        = DQN_MAX(DQN_MIN(size_required, size - 1), 0);
    if (result == size - 1) {
        buffer[size - 2] = '.';
        buffer[size - 3] = '.';
    }
    va_end(args);
    return result;
}

DQN_API Dqn_U64Str8 Dqn_U64ToStr8(uint64_t val, char separator)
{
    Dqn_U64Str8 result = {};
    if (val == 0) {
        result.data[result.size++] = '0';
    } else {
        // NOTE: The number is written in reverse because we form the string by
        // dividing by 10, so we write it in, then reverse it out after all is
        // done.
        Dqn_U64Str8 temp = {};
        for (Dqn_usize digit_count = 0; val > 0; digit_count++) {
            if (separator && (digit_count != 0) && (digit_count % 3 == 0))
                temp.data[temp.size++] = separator;

            auto digit             = DQN_CAST(char)(val % 10);
            temp.data[temp.size++] = '0' + digit;
            val /= 10;
        }

        // NOTE: Reverse the string
        DQN_MSVC_WARNING_PUSH
        DQN_MSVC_WARNING_DISABLE(6293) // Ill-defined for-loop
        DQN_MSVC_WARNING_DISABLE(
            6385) // Reading invalid data from 'temp.data' NOTE(doyle): Unsigned overflow is valid for loop termination
        for (Dqn_usize temp_index = temp.size - 1; temp_index < temp.size; temp_index--) {
            char ch                    = temp.data[temp_index];
            result.data[result.size++] = ch;
        }
        DQN_MSVC_WARNING_POP
    }

    return result;
}

DQN_API Dqn_U64ByteSize Dqn_U64ToByteSize(uint64_t bytes, Dqn_U64ByteSizeType desired_type)
{
    Dqn_U64ByteSize result = {};
    result.bytes           = DQN_CAST(Dqn_f64)bytes;
    if (!DQN_CHECK(desired_type != Dqn_U64ByteSizeType_Count)) {
        result.suffix = Dqn_U64ByteSizeTypeString(result.type);
        return result;
    }

    if (desired_type == Dqn_U64ByteSizeType_Auto) {
        for (; result.type < Dqn_U64ByteSizeType_Count && result.bytes >= 1024.0; result.type = DQN_CAST(Dqn_U64ByteSizeType)(DQN_CAST(Dqn_usize)result.type + 1))
            result.bytes /= 1024.0;
    } else {
        for (; result.type < desired_type; result.type = DQN_CAST(Dqn_U64ByteSizeType)(DQN_CAST(Dqn_usize)result.type + 1))
            result.bytes /= 1024.0;
    }

    result.suffix = Dqn_U64ByteSizeTypeString(result.type);
    return result;
}

DQN_API Dqn_Str8 Dqn_U64ToByteSizeStr8(Dqn_Arena *arena, uint64_t bytes, Dqn_U64ByteSizeType desired_type)
{
    Dqn_U64ByteSize byte_size = Dqn_U64ToByteSize(bytes, desired_type);
    Dqn_Str8        result    = Dqn_Str8_InitF(arena, "%.2f%.*s", byte_size.bytes, DQN_STR_FMT(byte_size.suffix));
    return result;
}

DQN_API Dqn_Str8 Dqn_U64ByteSizeTypeString(Dqn_U64ByteSizeType type)
{
    Dqn_Str8 result = DQN_STR8("");
    switch (type) {
        case Dqn_U64ByteSizeType_B:     result = DQN_STR8("B");   break;
        case Dqn_U64ByteSizeType_KiB:   result = DQN_STR8("KiB"); break;
        case Dqn_U64ByteSizeType_MiB:   result = DQN_STR8("MiB"); break;
        case Dqn_U64ByteSizeType_GiB:   result = DQN_STR8("GiB"); break;
        case Dqn_U64ByteSizeType_TiB:   result = DQN_STR8("TiB"); break;
        case Dqn_U64ByteSizeType_Count: result = DQN_STR8("");    break;
        case Dqn_U64ByteSizeType_Auto:  result = DQN_STR8("");    break;
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_U64ToAge(Dqn_Arena *arena, uint64_t age_s, Dqn_U64AgeUnit unit)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;

    Dqn_TLSTMem     tmem = Dqn_TLS_TMem(arena);
    Dqn_Str8Builder builder = {};
    builder.arena           = arena;
    uint64_t remainder      = age_s;

    if (unit & Dqn_U64AgeUnit_Year) {
        Dqn_usize value = remainder / DQN_YEARS_TO_S(1);
        remainder -= DQN_YEARS_TO_S(value);
        if (value)
            Dqn_Str8Builder_AddF(&builder, "%s%I64uyr", builder.string_size ? " " : "", value);
    }

    if (unit & Dqn_U64AgeUnit_Week) {
        Dqn_usize value = remainder / DQN_WEEKS_TO_S(1);
        remainder -= DQN_WEEKS_TO_S(value);
        if (value)
            Dqn_Str8Builder_AddF(&builder, "%s%I64uw", builder.string_size ? " " : "", value);
    }

    if (unit & Dqn_U64AgeUnit_Day) {
        Dqn_usize value = remainder / DQN_DAYS_TO_S(1);
        remainder -= DQN_DAYS_TO_S(value);
        if (value)
            Dqn_Str8Builder_AddF(&builder, "%s%I64ud", builder.string_size ? " " : "", value);
    }

    if (unit & Dqn_U64AgeUnit_Hr) {
        Dqn_usize value = remainder / DQN_HOURS_TO_S(1);
        remainder -= DQN_HOURS_TO_S(value);
        if (value)
            Dqn_Str8Builder_AddF(&builder, "%s%I64uh", builder.string_size ? " " : "", value);
    }

    if (unit & Dqn_U64AgeUnit_Min) {
        Dqn_usize value = remainder / DQN_MINS_TO_S(1);
        remainder -= DQN_MINS_TO_S(value);
        if (value)
            Dqn_Str8Builder_AddF(&builder, "%s%I64um", builder.string_size ? " " : "", value);
    }

    if (unit & Dqn_U64AgeUnit_Sec) {
        Dqn_usize value = remainder;
        Dqn_Str8Builder_AddF(&builder, "%s%I64us", builder.string_size ? " " : "", value);
    }

    result = Dqn_Str8Builder_Build(&builder, arena);
    return result;
}

DQN_API uint64_t Dqn_HexToU64(Dqn_Str8 hex)
{
    Dqn_Str8  real_hex     = Dqn_Str8_TrimPrefix(Dqn_Str8_TrimPrefix(hex, DQN_STR8("0x")), DQN_STR8("0X"));
    Dqn_usize max_hex_size = sizeof(uint64_t) * 2 /*hex chars per byte*/;
    DQN_ASSERT(real_hex.size <= max_hex_size);

    Dqn_usize size   = DQN_MIN(max_hex_size, real_hex.size);
    uint64_t  result = 0;
    for (Dqn_usize index = 0; index < size; index++) {
        char            ch  = real_hex.data[index];
        Dqn_CharHexToU8 val = Dqn_Char_HexToU8(ch);
        if (!val.success)
            break;
        result = (result << 4) | val.value;
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_U64ToHex(Dqn_Arena *arena, uint64_t number, uint32_t flags)
{
    Dqn_Str8 prefix = {};
    if ((flags & Dqn_HexU64Str8Flags_0xPrefix))
        prefix = DQN_STR8("0x");

    char const *fmt           = (flags & Dqn_HexU64Str8Flags_UppercaseHex) ? "%I64X" : "%I64x";
    Dqn_usize   required_size = Dqn_CStr8_FSize(fmt, number) + prefix.size;
    Dqn_Str8    result        = Dqn_Str8_Alloc(arena, required_size, Dqn_ZeroMem_No);

    if (Dqn_Str8_HasData(result)) {
        DQN_MEMCPY(result.data, prefix.data, prefix.size);
        int space = DQN_CAST(int) DQN_MAX((result.size - prefix.size) + 1, 0); /*null-terminator*/
        DQN_SNPRINTF(result.data + prefix.size, space, fmt, number);
    }
    return result;
}

DQN_API Dqn_U64HexStr8 Dqn_U64ToHexStr8(uint64_t number, uint32_t flags)
{
    Dqn_Str8 prefix = {};
    if (flags & Dqn_HexU64Str8Flags_0xPrefix)
        prefix = DQN_STR8("0x");

    Dqn_U64HexStr8 result = {};
    DQN_MEMCPY(result.data, prefix.data, prefix.size);
    result.size += DQN_CAST(int8_t) prefix.size;

    char const *fmt = (flags & Dqn_HexU64Str8Flags_UppercaseHex) ? "%I64X" : "%I64x";
    int size        = DQN_SNPRINTF(result.data + result.size, DQN_ARRAY_UCOUNT(result.data) - result.size, fmt, number);
    result.size += DQN_CAST(uint8_t) size;
    DQN_ASSERT(result.size < DQN_ARRAY_UCOUNT(result.data));

    // NOTE: snprintf returns the required size of the format string
    // irrespective of if there's space or not, but, always null terminates so
    // the last byte is wasted.
    result.size = DQN_MIN(result.size, DQN_ARRAY_UCOUNT(result.data) - 1);
    return result;
}

DQN_API bool Dqn_BytesToHexPtr(void const *src, Dqn_usize src_size, char *dest, Dqn_usize dest_size)
{
    if (!src || !dest)
        return false;

    if (!DQN_CHECK(dest_size >= src_size * 2))
        return false;

    char const          *HEX    = "0123456789abcdef";
    unsigned char const *src_u8 = DQN_CAST(unsigned char const *) src;
    for (Dqn_usize src_index = 0, dest_index = 0; src_index < src_size; src_index++) {
        char byte          = src_u8[src_index];
        char hex01         = (byte >> 4) & 0b1111;
        char hex02         = (byte >> 0) & 0b1111;
        dest[dest_index++] = HEX[(int)hex01];
        dest[dest_index++] = HEX[(int)hex02];
    }

    return true;
}

DQN_API Dqn_Str8 Dqn_BytesToHex(Dqn_Arena *arena, void const *src, Dqn_usize size)
{
    Dqn_Str8 result = {};
    if (!src || size <= 0)
        return result;

    result                   = Dqn_Str8_Alloc(arena, size * 2, Dqn_ZeroMem_No);
    result.data[result.size] = 0;
    bool converted           = Dqn_BytesToHexPtr(src, size, result.data, result.size);
    DQN_ASSERT(converted);
    return result;
}

DQN_API Dqn_usize Dqn_HexToBytesPtrUnchecked(Dqn_Str8 hex, void *dest, Dqn_usize dest_size)
{
    Dqn_usize      result  = 0;
    unsigned char *dest_u8 = DQN_CAST(unsigned char *) dest;

    for (Dqn_usize hex_index = 0; hex_index < hex.size; hex_index += 2, result += 1) {
        char hex01      = hex.data[hex_index];
        char hex02      = (hex_index + 1 < hex.size) ? hex.data[hex_index + 1] : 0;
        char bit4_01    = Dqn_Char_HexToU8(hex01).value;
        char bit4_02    = Dqn_Char_HexToU8(hex02).value;
        char byte       = (bit4_01 << 4) | (bit4_02 << 0);
        dest_u8[result] = byte;
    }

    DQN_ASSERT(result <= dest_size);
    return result;
}

DQN_API Dqn_usize Dqn_HexToBytesPtr(Dqn_Str8 hex, void *dest, Dqn_usize dest_size)
{
    hex = Dqn_Str8_TrimPrefix(hex, DQN_STR8("0x"));
    hex = Dqn_Str8_TrimPrefix(hex, DQN_STR8("0X"));

    Dqn_usize result = 0;
    if (!Dqn_Str8_HasData(hex))
        return result;

    // NOTE: Trimmed hex can be "0xf" -> "f" or "0xAB" -> "AB"
    // Either way, the size can be odd or even, hence we round up to the nearest
    // multiple of two to ensure that we calculate the min buffer size orrectly.
    Dqn_usize hex_size_rounded_up = hex.size + (hex.size % 2);
    Dqn_usize min_buffer_size     = hex_size_rounded_up / 2;
    if (hex.size <= 0 || !DQN_CHECK(dest_size >= min_buffer_size)) {
        return result;
    }

    result = Dqn_HexToBytesPtrUnchecked(hex, dest, dest_size);
    return result;
}

DQN_API Dqn_Str8 Dqn_HexToBytesUnchecked(Dqn_Arena *arena, Dqn_Str8 hex)
{
    Dqn_Str8 result = Dqn_Str8_Alloc(arena, hex.size / 2, Dqn_ZeroMem_No);
    if (result.data) {
        Dqn_usize bytes_written = Dqn_HexToBytesPtr(hex, result.data, result.size);
        DQN_ASSERT(bytes_written == result.size);
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_HexToBytes(Dqn_Arena *arena, Dqn_Str8 hex)
{
    hex = Dqn_Str8_TrimPrefix(hex, DQN_STR8("0x"));
    hex = Dqn_Str8_TrimPrefix(hex, DQN_STR8("0X"));

    Dqn_Str8 result = {};
    if (!DQN_CHECK(Dqn_Str8_IsAll(hex, Dqn_Str8IsAll_Hex)))
        return result;

    result = Dqn_HexToBytesUnchecked(arena, hex);
    return result;
}


// NOTE: [$DLIB] Dqn_Library ///////////////////////////////////////////////////////////////////////
Dqn_Library *g_dqn_library;

DQN_API Dqn_Library *Dqn_Library_Init(Dqn_LibraryOnInit on_init)
{
    if (!g_dqn_library) {
        static Dqn_Library default_instance = {};
        g_dqn_library                       = &default_instance;
    }

    // NOTE: Init check ///////////////////////////////////////////////////////////////////////////
    Dqn_Library *result = g_dqn_library;
    Dqn_TicketMutex_Begin(&result->lib_mutex);
    DQN_DEFER {
        Dqn_TicketMutex_End(&result->lib_mutex);
    };

    if (result->lib_init)
        return result;

    #define DQN_CPU_FEAT_XENTRY(label) g_dqn_cpu_feature_decl[Dqn_CPUFeature_##label] = {Dqn_CPUFeature_##label, DQN_STR8(#label)};
    DQN_CPU_FEAT_XMACRO
    #undef DQN_CPU_FEAT_XENTRY

    result->lib_init   = true;
    result->cpu_report = Dqn_CPU_Report();

    // NOTE: Query OS info /////////////////////////////////////////////////////////////////////////
    {
    #if defined(DQN_OS_WIN32)
        SYSTEM_INFO system_info = {};
        GetSystemInfo(&system_info);
        result->os_page_size         = system_info.dwPageSize;
        result->os_alloc_granularity = system_info.dwAllocationGranularity;

        QueryPerformanceFrequency(&result->win32_qpc_frequency);
    #else
        // TODO(doyle): Get the proper page size from the OS.
        result->os_page_size         = DQN_KILOBYTES(4);
        result->os_alloc_granularity = DQN_KILOBYTES(64);
    #endif
    }

    // NOTE Initialise fields //////////////////////////////////////////////////////////////////////
    #if !defined(DQN_NO_PROFILER)
    result->profiler = &result->profiler_default_instance;
    #endif

    // NOTE: BEGIN IMPORTANT ORDER OF STATEMENTS ///////////////////////////////////////////////////
    #if defined(DQN_LEAK_TRACKING)
    // NOTE: Setup the allocation table with allocation tracking turned off on
    // the arena we're using to initialise the table.
    result->alloc_table_arena.flags |= Dqn_ArenaFlag_NoAllocTrack;
    result->alloc_table              = Dqn_DSMap_Init<Dqn_DebugAlloc>(&result->alloc_table_arena, 4096, Dqn_DSMapFlags_Nil);
    #endif

    result->arena = Dqn_Arena_InitSize(DQN_MEGABYTES(2), DQN_KILOBYTES(64), Dqn_ArenaFlag_AllocCanLeak);
    result->pool  = Dqn_ChunkPool_Init(&result->arena, /*align*/ 0);
    Dqn_ArenaCatalog_Init(&result->arena_catalog, &result->pool);
    Dqn_ArenaCatalog_AddF(&result->arena_catalog, &result->arena, "Dqn Library");

    #if defined(DQN_LEAK_TRACING)
    Dqn_ArenaCatalog_AddF(&result->arena_catalog, &result->alloc_table_arena, "Dqn Allocation Table");
    #endif

    // NOTE: Initialise tmem arenas which allocate memory and will be
    // recorded to the now initialised allocation table. The initialisation
    // of tmem memory may request tmem memory itself in leak tracing mode.
    // This is supported as the tmem arenas defer allocation tracking until
    // initialisation is done.
    Dqn_TLS_Init(&result->tls);
    Dqn_OS_ThreadSetTLS(&result->tls);
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
    // NOTE: END IMPORTANT ORDER OF STATEMENTS /////////////////////////////////////////////////////

    result->exe_dir = Dqn_OS_EXEDir(&result->arena);

    // NOTE: Print out init features ///////////////////////////////////////////////////////////////
    Dqn_Str8Builder builder = {};
    builder.arena           = tmem.arena;
    if (on_init & Dqn_LibraryOnInit_LogLibFeatures) {
        Dqn_Str8Builder_AddRef(&builder, DQN_STR8("Dqn Library initialised:\n"));

        Dqn_f64 page_size_kib         = result->os_page_size / 1024.0;
        Dqn_f64 alloc_granularity_kib = result->os_alloc_granularity / 1024.0;
        Dqn_Str8Builder_AddF(
            &builder, "  OS Page Size/Alloc Granularity: %.1f/%.1fKiB\n", page_size_kib, alloc_granularity_kib);

        #if DQN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
        if (DQN_ASAN_POISON) {
            Dqn_Str8Builder_AddF(
                &builder, "  ASAN manual poisoning%s\n", DQN_ASAN_VET_POISON ? " (+vet sanity checks)" : "");
            Dqn_Str8Builder_AddF(&builder, "  ASAN poison guard size: %u\n", DQN_ASAN_POISON_GUARD_SIZE);
        }
        #endif

        #if defined(DQN_LEAK_TRACKING)
        Dqn_Str8Builder_AddRef(&builder, DQN_STR8("  Allocation leak tracing\n"));
        #endif

        #if !defined(DQN_NO_PROFILER)
        Dqn_Str8Builder_AddRef(&builder, DQN_STR8("  TSC profiler available\n"));
        #endif

        #if defined(DQN_USE_STD_PRINTF)
        Dqn_Str8Builder_AddRef(&builder, DQN_STR8("  Using stdio's printf functions\n"));
        #else
        Dqn_Str8Builder_AddRef(&builder, DQN_STR8("  Using stb_sprintf functions\n"));
        #endif

        // TODO(doyle): Add stacktrace feature log
    }

    if (on_init & Dqn_LibraryOnInit_LogCPUFeatures) {
        Dqn_CPUReport const *report = &result->cpu_report;
        Dqn_Str8 brand              = Dqn_Str8_TrimWhitespaceAround(Dqn_Str8_Init(report->brand, sizeof(report->brand) - 1));
        Dqn_Str8Builder_AddF(&builder,
                             "  CPU '%.*s' from '%s' detected:\n",
                             DQN_STR_FMT(brand),
                             report->vendor);

        Dqn_usize longest_feature_name = 0;
        DQN_FOR_UINDEX(feature_index, Dqn_CPUFeature_Count) {
            Dqn_CPUFeatureDecl feature_decl = g_dqn_cpu_feature_decl[feature_index];
            longest_feature_name = DQN_MAX(longest_feature_name, feature_decl.label.size);
        }

        DQN_FOR_UINDEX(feature_index, Dqn_CPUFeature_Count) {
            Dqn_CPUFeatureDecl feature_decl = g_dqn_cpu_feature_decl[feature_index];
            bool               has_feature  = Dqn_CPU_HasFeature(report, feature_decl.value);
            Dqn_Str8Builder_AddF(&builder,
                                 "    %.*s:%*s%s\n",
                                 DQN_STR_FMT(feature_decl.label),
                                 DQN_CAST(int)(longest_feature_name - feature_decl.label.size),
                                 "",
                                 has_feature ? "available" : "not available");
        }
    }

    Dqn_Str8 info_log = Dqn_Str8Builder_Build(&builder, tmem.arena);
    if (Dqn_Str8_HasData(info_log))
        Dqn_Log_DebugF("%.*s", DQN_STR_FMT(info_log));
    return result;
}

DQN_API void Dqn_Library_SetPointer(Dqn_Library *library)
{
    if (library) {
        g_dqn_library = library;
        Dqn_OS_ThreadSetTLS(&library->tls);
    }
}

#if !defined(DQN_NO_PROFILER)
DQN_API void Dqn_Library_SetProfiler(Dqn_Profiler *profiler)
{
    if (profiler)
        g_dqn_library->profiler = profiler;
}
#endif

DQN_API void Dqn_Library_SetLogCallback(Dqn_LogProc *proc, void *user_data)
{
    g_dqn_library->log_callback  = proc;
    g_dqn_library->log_user_data = user_data;
}

DQN_API void Dqn_Library_DumpThreadContextArenaStat(Dqn_Str8 file_path)
{
#if defined(DQN_DEBUG_THREAD_CONTEXT)
    // NOTE: Open a file to write the arena stats to
    FILE *file = nullptr;
    fopen_s(&file, file_path.data, "a+b");
    if (file) {
        Dqn_Log_ErrorF("Failed to dump thread context arenas [file=%.*s]", DQN_STR_FMT(file_path));
        return;
    }

    // NOTE: Copy the stats from library book-keeping
    // NOTE: Extremely short critical section, copy the stats then do our
    // work on it.
    Dqn_ArenaStat stats[Dqn_CArray_CountI(g_dqn_library->thread_context_arena_stats)];
    int           stats_size = 0;

    Dqn_TicketMutex_Begin(&g_dqn_library->thread_context_mutex);
    stats_size = g_dqn_library->thread_context_arena_stats_count;
    DQN_MEMCPY(stats, g_dqn_library->thread_context_arena_stats, sizeof(stats[0]) * stats_size);
    Dqn_TicketMutex_End(&g_dqn_library->thread_context_mutex);

    // NOTE: Print the cumulative stat
    Dqn_DateHMSTimeStr now = Dqn_Date_HMSLocalTimeStrNow();
    fprintf(file,
            "Time=%.*s %.*s | Thread Context Arenas | Count=%d\n",
            now.date_size,
            now.date,
            now.hms_size,
            now.hms,
            g_dqn_library->thread_context_arena_stats_count);

    // NOTE: Write the cumulative thread arena data
    {
        Dqn_ArenaStat stat = {};
        for (Dqn_usize index = 0; index < stats_size; index++) {
            Dqn_ArenaStat const *current = stats + index;
            stat.capacity += current->capacity;
            stat.used += current->used;
            stat.wasted += current->wasted;
            stat.blocks += current->blocks;

            stat.capacity_hwm = DQN_MAX(stat.capacity_hwm, current->capacity_hwm);
            stat.used_hwm     = DQN_MAX(stat.used_hwm, current->used_hwm);
            stat.wasted_hwm   = DQN_MAX(stat.wasted_hwm, current->wasted_hwm);
            stat.blocks_hwm   = DQN_MAX(stat.blocks_hwm, current->blocks_hwm);
        }

        Dqn_ArenaStatStr stats_string = Dqn_Arena_StatStr(&stat);
        fprintf(file, "  [ALL] CURR %.*s\n", stats_string.size, stats_string.data);
    }

    // NOTE: Print individual thread arena data
    for (Dqn_usize index = 0; index < stats_size; index++) {
        Dqn_ArenaStat const *current        = stats + index;
        Dqn_ArenaStatStr     current_string = Dqn_Arena_StatStr(current);
        fprintf(file, "  [%03d] CURR %.*s\n", DQN_CAST(int) index, current_string.size, current_string.data);
    }

    fclose(file);
    Dqn_Log_InfoF("Dumped thread context arenas [file=%.*s]", DQN_STR_FMT(file_path));
#else
    (void)file_path;
#endif // #if defined(DQN_DEBUG_THREAD_CONTEXT)
}

DQN_API Dqn_Arena *Dqn_Library_AllocArenaF(Dqn_usize reserve, Dqn_usize commit, uint8_t arena_flags, char const *fmt, ...)
{
    DQN_ASSERT(g_dqn_library->lib_init);
    va_list args;
    va_start(args, fmt);
    Dqn_ArenaCatalog *catalog = &g_dqn_library->arena_catalog;
    Dqn_Arena        *result  = Dqn_ArenaCatalog_AllocFV(catalog, reserve, commit, arena_flags, fmt, args);
    va_end(args);
    return result;
}

DQN_API bool Dqn_Library_EraseArena(Dqn_Arena *arena, Dqn_ArenaCatalogFreeArena free_arena)
{
    Dqn_ArenaCatalog *catalog = &g_dqn_library->arena_catalog;
    bool result = Dqn_ArenaCatalog_Erase(catalog, arena, free_arena);
    return result;
}

#if !defined(DQN_NO_PROFILER)
// NOTE: [$PROF] Dqn_Profiler //////////////////////////////////////////////////////////////////////
DQN_API Dqn_ProfilerZoneScope::Dqn_ProfilerZoneScope(Dqn_Str8 name, uint16_t anchor_index)
{
    zone = Dqn_Profiler_BeginZoneAtIndex(name, anchor_index);
}

DQN_API Dqn_ProfilerZoneScope::~Dqn_ProfilerZoneScope()
{
    Dqn_Profiler_EndZone(zone);
}

DQN_API Dqn_ProfilerAnchor *Dqn_Profiler_ReadBuffer()
{
    Dqn_ProfilerAnchor *result = Dqn_Profiler_AnchorBuffer(Dqn_ProfilerAnchorBuffer_Front);
    return result;
}

DQN_API Dqn_ProfilerAnchor *Dqn_Profiler_WriteBuffer()
{
    Dqn_ProfilerAnchor *result = Dqn_Profiler_AnchorBuffer(Dqn_ProfilerAnchorBuffer_Back);
    return result;
}

DQN_API Dqn_ProfilerZone Dqn_Profiler_BeginZoneAtIndex(Dqn_Str8 name, uint16_t anchor_index)
{
    Dqn_ProfilerAnchor *anchor           = Dqn_Profiler_WriteBuffer() + anchor_index;
    if (Dqn_Str8_HasData(anchor->name))
        DQN_ASSERTF(name == anchor->name, "Potentially overwriting a zone by accident?");
    anchor->name                         = name;
    Dqn_ProfilerZone result              = {};
    result.begin_tsc                     = Dqn_CPU_TSC();
    result.anchor_index                  = anchor_index;
    result.parent_zone                   = g_dqn_library->profiler->parent_zone;
    result.elapsed_tsc_at_zone_start     = anchor->tsc_inclusive;
    g_dqn_library->profiler->parent_zone = anchor_index;
    return result;
}

DQN_API void Dqn_Profiler_EndZone(Dqn_ProfilerZone zone)
{
    uint64_t            elapsed_tsc   = Dqn_CPU_TSC() - zone.begin_tsc;
    Dqn_ProfilerAnchor *anchor_buffer = Dqn_Profiler_WriteBuffer();
    Dqn_ProfilerAnchor *anchor        = anchor_buffer + zone.anchor_index;

    anchor->hit_count++;
    anchor->tsc_inclusive = zone.elapsed_tsc_at_zone_start + elapsed_tsc;
    anchor->tsc_exclusive += elapsed_tsc;

    Dqn_ProfilerAnchor *parent_anchor = anchor_buffer + zone.parent_zone;
    parent_anchor->tsc_exclusive -= elapsed_tsc;
    g_dqn_library->profiler->parent_zone = zone.parent_zone;
}

DQN_API Dqn_ProfilerAnchor *Dqn_Profiler_AnchorBuffer(Dqn_ProfilerAnchorBuffer buffer)
{
    uint8_t offset = buffer == Dqn_ProfilerAnchorBuffer_Back ? 0 : 1;
    uint8_t anchor_buffer =
        (g_dqn_library->profiler->active_anchor_buffer + offset) % DQN_ARRAY_UCOUNT(g_dqn_library->profiler->anchors);
    Dqn_ProfilerAnchor *result = g_dqn_library->profiler->anchors[anchor_buffer];
    return result;
}

DQN_API void Dqn_Profiler_SwapAnchorBuffer()
{
    g_dqn_library->profiler->active_anchor_buffer++;
    Dqn_ProfilerAnchor *anchors = Dqn_Profiler_AnchorBuffer(Dqn_ProfilerAnchorBuffer_Back);
    DQN_MEMSET(anchors,
               0,
               DQN_ARRAY_UCOUNT(g_dqn_library->profiler->anchors[0]) * sizeof(g_dqn_library->profiler->anchors[0][0]));
}

DQN_API void Dqn_Profiler_Dump(uint64_t tsc_per_second)
{
    Dqn_ProfilerAnchor *anchors = Dqn_Profiler_ReadBuffer();
    for (size_t anchor_index = 1; anchor_index < DQN_PROFILER_ANCHOR_BUFFER_SIZE; anchor_index++) {
        Dqn_ProfilerAnchor const *anchor = anchors + anchor_index;
        if (!anchor->hit_count)
            continue;

        uint64_t tsc_exclusive              = anchor->tsc_exclusive;
        uint64_t tsc_inclusive              = anchor->tsc_inclusive;
        Dqn_f64  tsc_exclusive_milliseconds = tsc_exclusive * 1000 / DQN_CAST(Dqn_f64) tsc_per_second;
        if (tsc_exclusive == tsc_inclusive) {
            Dqn_Print_LnF("%.*s[%u]: %.1fms", DQN_STR_FMT(anchor->name), anchor->hit_count, tsc_exclusive_milliseconds);
        } else {
            Dqn_f64 tsc_inclusive_milliseconds = tsc_inclusive * 1000 / DQN_CAST(Dqn_f64) tsc_per_second;
            Dqn_Print_LnF("%.*s[%u]: %.1f/%.1fms",
                          DQN_STR_FMT(anchor->name),
                          anchor->hit_count,
                          tsc_exclusive_milliseconds,
                          tsc_inclusive_milliseconds);
        }
    }
}
#endif // !defined(DQN_NO_PROFILER)

// NOTE: [$JOBQ] Dqn_JobQueue ///////////////////////////////////////////////////////////////////////
DQN_API Dqn_JobQueueSPMC Dqn_OS_JobQueueSPMCInit()
{
    Dqn_JobQueueSPMC result               = {};
    result.thread_wait_for_job_semaphore  = Dqn_OS_SemaphoreInit(0 /*initial_count*/);
    result.wait_for_completion_semaphore  = Dqn_OS_SemaphoreInit(0 /*initial_count*/);
    result.complete_queue_write_semaphore = Dqn_OS_SemaphoreInit(DQN_ARRAY_UCOUNT(result.complete_queue));
    result.mutex                          = Dqn_OS_MutexInit();
    return result;
}

DQN_API bool Dqn_OS_JobQueueSPMCAddArray(Dqn_JobQueueSPMC *queue, Dqn_Job *jobs, uint32_t count)
{
    if (!queue)
        return false;

    uint32_t const pot_mask    = DQN_ARRAY_UCOUNT(queue->jobs) - 1;
    uint32_t       read_index  = queue->read_index;
    uint32_t       write_index = queue->write_index;
    uint32_t       size        = write_index - read_index;

    if ((size + count) > DQN_ARRAY_UCOUNT(queue->jobs))
        return false;

    for (size_t offset = 0; offset < count; offset++) {
        uint32_t wrapped_write_index     = (write_index + offset) & pot_mask;
        queue->jobs[wrapped_write_index] = jobs[offset];
    }

    Dqn_OS_MutexLock(&queue->mutex);
    queue->write_index += count;
    Dqn_OS_SemaphoreIncrement(&queue->thread_wait_for_job_semaphore, count);
    Dqn_OS_MutexUnlock(&queue->mutex);
    return true;
}

DQN_API bool Dqn_OS_JobQueueSPMCAdd(Dqn_JobQueueSPMC *queue, Dqn_Job job)
{
    bool result = Dqn_OS_JobQueueSPMCAddArray(queue, &job, 1);
    return result;
}

DQN_API int32_t Dqn_OS_JobQueueSPMCThread(Dqn_OSThread *thread)
{
    Dqn_JobQueueSPMC *queue    = DQN_CAST(Dqn_JobQueueSPMC *) thread->user_context;
    uint32_t const    pot_mask = DQN_ARRAY_UCOUNT(queue->jobs) - 1;
    static_assert(DQN_ARRAY_UCOUNT(queue->jobs) == DQN_ARRAY_UCOUNT(queue->complete_queue), "PoT mask is used to mask access to both arrays");

    for (;;) {
        Dqn_OS_SemaphoreWait(&queue->thread_wait_for_job_semaphore, DQN_OS_SEMAPHORE_INFINITE_TIMEOUT);
        if (queue->quit)
            break;

        DQN_ASSERT(queue->read_index != queue->write_index);

        Dqn_OS_MutexLock(&queue->mutex);
        uint32_t wrapped_read_index = queue->read_index & pot_mask;
        Dqn_Job  job                = queue->jobs[wrapped_read_index];
        queue->read_index += 1;
        Dqn_OS_MutexUnlock(&queue->mutex);

        job.elapsed_tsc -= Dqn_CPU_TSC();
        job.func(thread, job.user_context);
        job.elapsed_tsc += Dqn_CPU_TSC();

        if (job.add_to_completion_queue) {
            Dqn_OS_SemaphoreWait(&queue->complete_queue_write_semaphore, DQN_OS_SEMAPHORE_INFINITE_TIMEOUT);
            Dqn_OS_MutexLock(&queue->mutex);
            queue->complete_queue[(queue->complete_write_index++ & pot_mask)] = job;
            Dqn_OS_MutexUnlock(&queue->mutex);
            Dqn_OS_SemaphoreIncrement(&queue->complete_queue_write_semaphore, 1);
        }

        // NOTE: Update finish counter
        Dqn_OS_MutexLock(&queue->mutex);
        queue->finish_index += 1;

        // NOTE: If all jobs are finished and we have another thread who is
        // blocked via `WaitForCompletion` for this job queue, we will go
        // release the semaphore to wake them all up.
        bool all_jobs_finished = queue->finish_index == queue->write_index;
        if (all_jobs_finished && queue->threads_waiting_for_completion) {
            Dqn_OS_SemaphoreIncrement(&queue->wait_for_completion_semaphore,
                                      queue->threads_waiting_for_completion);
            queue->threads_waiting_for_completion = 0;
        }
        Dqn_OS_MutexUnlock(&queue->mutex);
    }

    return queue->quit_exit_code;
}

DQN_API void Dqn_OS_JobQueueSPMCWaitForCompletion(Dqn_JobQueueSPMC *queue)
{
    Dqn_OS_MutexLock(&queue->mutex);
    if (queue->finish_index == queue->write_index) {
        Dqn_OS_MutexUnlock(&queue->mutex);
        return;
    }
    queue->threads_waiting_for_completion++;
    Dqn_OS_MutexUnlock(&queue->mutex);

    Dqn_OS_SemaphoreWait(&queue->wait_for_completion_semaphore, DQN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DQN_API Dqn_usize Dqn_OS_JobQueueSPMCGetFinishedJobs(Dqn_JobQueueSPMC *queue, Dqn_Job *jobs, Dqn_usize jobs_size)
{
    Dqn_usize result = 0;
    if (!queue || !jobs || jobs_size <= 0)
        return result;

    uint32_t const pot_mask = DQN_ARRAY_UCOUNT(queue->jobs) - 1;
    Dqn_OS_MutexLock(&queue->mutex);
    while (queue->complete_read_index < queue->complete_write_index && result < jobs_size) {
        jobs[result++] = queue->complete_queue[(queue->complete_read_index++ & pot_mask)];
    }
    Dqn_OS_MutexUnlock(&queue->mutex);

    return result;
}
