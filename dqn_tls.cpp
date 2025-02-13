#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$$\ $$\       $$$$$$\
//   \__$$  __|$$ |     $$  __$$\
//      $$ |   $$ |     $$ /  \__|
//      $$ |   $$ |     \$$$$$$\
//      $$ |   $$ |      \____$$\
//      $$ |   $$ |     $$\   $$ |
//      $$ |   $$$$$$$$\\$$$$$$  |
//      \__|   \________|\______/
//
//   dqn_tls.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$TCTX] DN_TLS /////////////////////////////////////////////////////////////////
DN_TLSTMem::DN_TLSTMem(DN_TLS *tls, uint8_t arena_index, DN_TLSPushTMem push_tmem)
{
    DN_ASSERT(arena_index == DN_TLSArena_TMem0 || arena_index == DN_TLSArena_TMem1);
    arena      = tls->arenas + arena_index;
    temp_mem   = DN_Arena_TempMemBegin(arena);
    destructed = false;
    push_arena = push_tmem;
    if (push_arena)
        DN_TLS_PushArena(arena);
}

DN_TLSTMem::~DN_TLSTMem()
{
    DN_ASSERT(destructed == false);
    DN_Arena_TempMemEnd(temp_mem);
    destructed = true;
    if (push_arena)
        DN_TLS_PopArena();
}

DN_API void DN_TLS_Init(DN_TLS *tls)
{
    DN_CHECK(tls);
    if (tls->init)
        return;

    DN_FOR_UINDEX (index, DN_TLSArena_Count) {
        DN_Arena *arena = tls->arenas + index;
        switch (DN_CAST(DN_TLSArena)index) {
            default:                     *arena = DN_Arena_InitSize(DN_MEGABYTES(4), DN_KILOBYTES(4), DN_ArenaFlags_AllocCanLeak); break;
            case DN_TLSArena_ErrorSink: *arena = DN_Arena_InitSize(DN_KILOBYTES(64), DN_KILOBYTES(4), DN_ArenaFlags_AllocCanLeak); break;
            case DN_TLSArena_Count:     DN_INVALID_CODE_PATH; break;
        }
    }

    tls->thread_id      = DN_OS_ThreadID();
    tls->err_sink.arena = tls->arenas + DN_TLSArena_ErrorSink;
    tls->init           = true;
}

DN_API void DN_TLS_Deinit(DN_TLS *tls)
{
    tls->init              = false;
    tls->err_sink          = {};
    tls->arena_stack_index = {};
    DN_FOR_UINDEX(index, DN_TLSArena_Count)
    {
        DN_Arena *arena = tls->arenas + index;
        DN_Arena_Deinit(arena);
    }
}

DN_API DN_TLS *DN_TLS_Get()
{
    DN_TLS *result = g_dn_os_thread_tls;
    DN_ASSERT(
        g_dn_core->init &&
        "Library context must be be initialised first by calling DN_Library_Init. This "
        "initialises the main thread's TLS for you (no need to call DN_OS_ThreadSetTLS on main)");

    DN_ASSERT(result &&
               "Thread must be assigned the TLS with DN_OS_ThreadSetTLS. If the library is "
               "initialised, then, this thread was created without calling the set TLS function "
               "for the spawned thread.");
    return result;
}

DN_API DN_Arena *DN_TLS_Arena()
{
    DN_TLS   *tls    = DN_TLS_Get();
    DN_Arena *result = tls->arenas + DN_TLSArena_Main;
    return result;
}

// TODO: Is there a way to handle conflict arenas without the user needing to
// manually pass it in?
DN_API DN_TLSTMem DN_TLS_GetTMem(void const *conflict_arena, DN_TLSPushTMem push_tmem)
{
    DN_TLS *tls       = DN_TLS_Get();
    uint8_t  tls_index = (uint8_t)-1;
    for (uint8_t index = DN_TLSArena_TMem0; index <= DN_TLSArena_TMem1; index++) {
        DN_Arena *arena = tls->arenas + index;
        if (!conflict_arena || arena != conflict_arena) {
            tls_index = index;
            break;
        }
    }

    DN_ASSERT(tls_index != (uint8_t)-1);
    return DN_TLSTMem(tls, tls_index, push_tmem);
}

DN_API void DN_TLS_PushArena(DN_Arena *arena)
{
    DN_ASSERT(arena);
    DN_TLS *tls = DN_TLS_Get();
    DN_ASSERT(tls->arena_stack_index < DN_ARRAY_UCOUNT(tls->arena_stack));
    tls->arena_stack[tls->arena_stack_index++] = arena;
}

DN_API void DN_TLS_PopArena()
{
    DN_TLS *tls = DN_TLS_Get();
    DN_ASSERT(tls->arena_stack_index > 0);
    tls->arena_stack_index--;
}

DN_API DN_Arena *DN_TLS_TopArena()
{
    DN_TLS   *tls    = DN_TLS_Get();
    DN_Arena *result = nullptr;
    if (tls->arena_stack_index)
        result = tls->arena_stack[tls->arena_stack_index - 1];
    return result;
}

DN_API void DN_TLS_BeginFrame(DN_Arena *frame_arena)
{
    DN_TLS *tls     = DN_TLS_Get();
    tls->frame_arena = frame_arena;
}

DN_API DN_Arena *DN_TLS_FrameArena()
{
    DN_TLS   *tls    = DN_TLS_Get();
    DN_Arena *result = tls->frame_arena;
    return result;
}

