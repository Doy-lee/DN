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

// NOTE: [$TCTX] Dqn_TLS /////////////////////////////////////////////////////////////////
Dqn_TLSTMem::Dqn_TLSTMem(Dqn_TLS *tls, uint8_t arena_index, Dqn_TLSPushTMem push_tmem)
{
    DQN_ASSERT(arena_index == Dqn_TLSArena_TMem0 || arena_index == Dqn_TLSArena_TMem1);
    arena      = tls->arenas + arena_index;
    temp_mem   = Dqn_Arena_TempMemBegin(arena);
    destructed = false;
    push_arena = push_tmem;
    if (push_arena)
        Dqn_TLS_PushArena(arena);
}

Dqn_TLSTMem::~Dqn_TLSTMem()
{
    DQN_ASSERT(destructed == false);
    Dqn_Arena_TempMemEnd(temp_mem);
    destructed = true;
    if (push_arena)
        Dqn_TLS_PopArena();
}

DQN_API void Dqn_TLS_Init(Dqn_TLS *tls)
{
    DQN_CHECK(tls);
    if (tls->init)
        return;

    DQN_FOR_UINDEX (index, Dqn_TLSArena_Count) {
        Dqn_Arena *arena = tls->arenas + index;
        switch (DQN_CAST(Dqn_TLSArena)index) {
            default:                     *arena = Dqn_Arena_InitSize(DQN_MEGABYTES(4), DQN_KILOBYTES(4), Dqn_ArenaFlag_AllocCanLeak); break;
            case Dqn_TLSArena_ErrorSink: *arena = Dqn_Arena_InitSize(DQN_KILOBYTES(64), DQN_KILOBYTES(4), Dqn_ArenaFlag_AllocCanLeak); break;
            case Dqn_TLSArena_Count: DQN_INVALID_CODE_PATH; break;
        }
    }

    tls->thread_id        = Dqn_OS_ThreadID();
    tls->error_sink.arena = tls->arenas + Dqn_TLSArena_ErrorSink;
    tls->init             = true;
}

DQN_API Dqn_TLS *Dqn_TLS_Get()
{
    Dqn_TLS *result = g_dqn_os_thread_tls;
    // TODO(doyle): Fix stack-trace infinite loop with requiring the TLS that is not initialised
    // yet.
    DQN_ASSERT(
        g_dqn_library->lib_init &&
        "Library context must be be initialised first by calling Dqn_Library_Init. This "
        "initialises the main thread's TLS for you (no need to call Dqn_OS_ThreadSetTLS on main)");

    DQN_ASSERT(result &&
               "Thread must be assigned the TLS with Dqn_OS_ThreadSetTLS. If the library is "
               "initialised, then, this thread was created without calling the set TLS function "
               "for the spawned thread.");
    return result;
}

DQN_API Dqn_Arena *Dqn_TLS_Arena()
{
    Dqn_TLS   *tls    = Dqn_TLS_Get();
    Dqn_Arena *result = tls->arenas + Dqn_TLSArena_Main;
    return result;
}

// TODO: Is there a way to handle conflict arenas without the user needing to
// manually pass it in?
DQN_API Dqn_TLSTMem Dqn_TLS_GetTMem(void const *conflict_arena, Dqn_TLSPushTMem push_tmem)
{
    Dqn_TLS *tls       = Dqn_TLS_Get();
    uint8_t  tls_index = (uint8_t)-1;
    for (uint8_t index = Dqn_TLSArena_TMem0; index <= Dqn_TLSArena_TMem1; index++) {
        Dqn_Arena *arena = tls->arenas + index;
        if (!conflict_arena || arena != conflict_arena) {
            tls_index = index;
            break;
        }
    }

    DQN_ASSERT(tls_index != (uint8_t)-1);
    return Dqn_TLSTMem(tls, tls_index, push_tmem);
}

DQN_API void Dqn_TLS_PushArena(Dqn_Arena *arena)
{
    DQN_ASSERT(arena);
    Dqn_TLS *tls = Dqn_TLS_Get();
    DQN_ASSERT(tls->arena_stack_index < DQN_ARRAY_UCOUNT(tls->arena_stack));
    tls->arena_stack[tls->arena_stack_index++] = arena;
}

DQN_API void Dqn_TLS_PopArena()
{
    Dqn_TLS *tls = Dqn_TLS_Get();
    DQN_ASSERT(tls->arena_stack_index > 0);
    tls->arena_stack_index--;
}

DQN_API Dqn_Arena *Dqn_TLS_TopArena()
{
    Dqn_TLS   *tls    = Dqn_TLS_Get();
    Dqn_Arena *result = nullptr;
    if (tls->arena_stack_index)
        result = tls->arena_stack[tls->arena_stack_index - 1];
    return result;
}
