#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\  $$\   $$\ $$$$$$$$\  $$$$$$\  $$$$$$\ $$\   $$\ $$$$$$$$\ $$$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\ $$$\  $$ |\__$$  __|$$  __$$\ \_$$  _|$$$\  $$ |$$  _____|$$  __$$\ $$  __$$\
//   $$ /  \__|$$ /  $$ |$$$$\ $$ |   $$ |   $$ /  $$ |  $$ |  $$$$\ $$ |$$ |      $$ |  $$ |$$ /  \__|
//   $$ |      $$ |  $$ |$$ $$\$$ |   $$ |   $$$$$$$$ |  $$ |  $$ $$\$$ |$$$$$\    $$$$$$$  |\$$$$$$\
//   $$ |      $$ |  $$ |$$ \$$$$ |   $$ |   $$  __$$ |  $$ |  $$ \$$$$ |$$  __|   $$  __$$<  \____$$\
//   $$ |  $$\ $$ |  $$ |$$ |\$$$ |   $$ |   $$ |  $$ |  $$ |  $$ |\$$$ |$$ |      $$ |  $$ |$$\   $$ |
//   \$$$$$$  | $$$$$$  |$$ | \$$ |   $$ |   $$ |  $$ |$$$$$$\ $$ | \$$ |$$$$$$$$\ $$ |  $$ |\$$$$$$  |
//    \______/  \______/ \__|  \__|   \__|   \__|  \__|\______|\__|  \__|\________|\__|  \__| \______/
//
//   dqn_containers.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$SLIC] Dqn_Slice /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8 Dqn_Slice_Str8Render(Dqn_Arena *arena, Dqn_Slice<Dqn_Str8> array, Dqn_Str8 separator)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;

    Dqn_usize total_size = 0;
    for (Dqn_usize index = 0; index < array.size; index++) {
        if (index)
            total_size += separator.size;
        Dqn_Str8 item  = array.data[index];
        total_size    += item.size;
    }

    result = Dqn_Str8_Alloc(arena, total_size, Dqn_ZeroMem_No);
    if (result.data) {
        Dqn_usize write_index = 0;
        for (Dqn_usize index = 0; index < array.size; index++) {
            if (index) {
                DQN_MEMCPY(result.data + write_index, separator.data, separator.size);
                write_index += separator.size;
            }
            Dqn_Str8 item = array.data[index];
            DQN_MEMCPY(result.data + write_index, item.data, item.size);
            write_index += item.size;
        }
    }

    return result;
}

DQN_API Dqn_Str8 Dqn_Slice_Str8RenderSpaceSeparated(Dqn_Arena *arena, Dqn_Slice<Dqn_Str8> array)
{
    Dqn_Str8 result = Dqn_Slice_Str8Render(arena, array, DQN_STR8(" "));
    return result;
}

DQN_API Dqn_Str16 Dqn_Slice_Str16Render(Dqn_Arena *arena, Dqn_Slice<Dqn_Str16> array, Dqn_Str16 separator)
{
    Dqn_Str16 result = {};
    if (!arena)
        return result;

    Dqn_usize total_size = 0;
    for (Dqn_usize index = 0; index < array.size; index++) {
        if (index)
            total_size += separator.size;
        Dqn_Str16 item  = array.data[index];
        total_size    += item.size;
    }

    result = {Dqn_Arena_NewArray(arena, wchar_t, total_size + 1, Dqn_ZeroMem_No), total_size};
    if (result.data) {
        Dqn_usize write_index = 0;
        for (Dqn_usize index = 0; index < array.size; index++) {
            if (index) {
                DQN_MEMCPY(result.data + write_index, separator.data, separator.size * sizeof(result.data[0]));
                write_index += separator.size;
            }
            Dqn_Str16 item = array.data[index];
            DQN_MEMCPY(result.data + write_index, item.data, item.size * sizeof(result.data[0]));
            write_index += item.size;
        }
    }

    result.data[total_size] = 0;
    return result;
}

DQN_API Dqn_Str16 Dqn_Slice_Str16RenderSpaceSeparated(Dqn_Arena *arena, Dqn_Slice<Dqn_Str16> array)
{
    Dqn_Str16 result = Dqn_Slice_Str16Render(arena, array, DQN_STR16(L" "));
    return result;
}

#if !defined(DQN_NO_DSMAP)
// NOTE: [$DMAP] Dqn_DSMap /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_DSMapKey Dqn_DSMap_KeyU64NoHash(uint64_t u64)
{
    Dqn_DSMapKey result = {};
    result.type         = Dqn_DSMapKeyType_U64NoHash;
    result.u64          = u64;
    result.hash         = DQN_CAST(uint32_t) u64;
    return result;
}

DQN_API bool Dqn_DSMap_KeyEquals(Dqn_DSMapKey lhs, Dqn_DSMapKey rhs)
{
    bool result = false;
    if (lhs.type == rhs.type && lhs.hash == rhs.hash) {
        switch (lhs.type)  {
            case Dqn_DSMapKeyType_Invalid:    result = true; break;
            case Dqn_DSMapKeyType_U64NoHash:  result = true; break;
            case Dqn_DSMapKeyType_U64:        result = lhs.u64 == rhs.u64; break;

            case Dqn_DSMapKeyType_BufferAsU64NoHash: /*FALLTHRU*/
            case Dqn_DSMapKeyType_Buffer: {
                if (lhs.buffer_size == rhs.buffer_size)
                    result = DQN_MEMCMP(lhs.buffer_data, rhs.buffer_data, lhs.buffer_size) == 0;
            } break;
        }
    }
    return result;
}

DQN_API bool operator==(Dqn_DSMapKey lhs, Dqn_DSMapKey rhs)
{
    bool result = Dqn_DSMap_KeyEquals(lhs, rhs);
    return result;
}
#endif // !defined(DQN_NO_DSMAP)
