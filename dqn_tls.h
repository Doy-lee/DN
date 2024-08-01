#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$$\ $$\   $$\ $$$$$$$\  $$$$$$$$\  $$$$$$\  $$$$$$$\
//   \__$$  __|$$ |  $$ |$$  __$$\ $$  _____|$$  __$$\ $$  __$$\
//      $$ |   $$ |  $$ |$$ |  $$ |$$ |      $$ /  $$ |$$ |  $$ |
//      $$ |   $$$$$$$$ |$$$$$$$  |$$$$$\    $$$$$$$$ |$$ |  $$ |
//      $$ |   $$  __$$ |$$  __$$< $$  __|   $$  __$$ |$$ |  $$ |
//      $$ |   $$ |  $$ |$$ |  $$ |$$ |      $$ |  $$ |$$ |  $$ |
//      $$ |   $$ |  $$ |$$ |  $$ |$$$$$$$$\ $$ |  $$ |$$$$$$$  |
//      \__|   \__|  \__|\__|  \__|\________|\__|  \__|\_______/
//
//    $$$$$$\   $$$$$$\  $$\   $$\ $$$$$$$$\ $$$$$$$$\ $$\   $$\ $$$$$$$$\
//   $$  __$$\ $$  __$$\ $$$\  $$ |\__$$  __|$$  _____|$$ |  $$ |\__$$  __|
//   $$ /  \__|$$ /  $$ |$$$$\ $$ |   $$ |   $$ |      \$$\ $$  |   $$ |
//   $$ |      $$ |  $$ |$$ $$\$$ |   $$ |   $$$$$\     \$$$$  /    $$ |
//   $$ |      $$ |  $$ |$$ \$$$$ |   $$ |   $$  __|    $$  $$<     $$ |
//   $$ |  $$\ $$ |  $$ |$$ |\$$$ |   $$ |   $$ |      $$  /\$$\    $$ |
//   \$$$$$$  | $$$$$$  |$$ | \$$ |   $$ |   $$$$$$$$\ $$ /  $$ |   $$ |
//    \______/  \______/ \__|  \__|   \__|   \________|\__|  \__|   \__|
//
//   dqn_thread_context.h -- Per thread data (e.g. scratch arenas)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

enum Dqn_TLSArena
{
    Dqn_TLSArena_Main,      // NOTE: Arena for Permanent allocations
    Dqn_TLSArena_ErrorSink, // NOTE: Arena for logging error information for this thread

    // NOTE: Per-thread scratch arenas (2 to prevent aliasing)
    Dqn_TLSArena_TMem0,
    Dqn_TLSArena_TMem1,

    Dqn_TLSArena_Count,
};

struct Dqn_TLS
{
    Dqn_b32           init;       // Flag to track if Thread has been initialised
    uint64_t          thread_id;
    Dqn_CallSite      call_site;  // Stores call-site information when requested by thread
    Dqn_ErrorSink     error_sink; // Error handling state
    Dqn_Arena         arenas[Dqn_TLSArena_Count];

    // Push and pop arenas onto the stack. Functions suffixed 'TLS' will use
    // these arenas for memory allocation.
    Dqn_Arena *arena_stack[8];
    Dqn_usize  arena_stack_index;
};

// Push the temporary memory arena when retrieved, popped when the arena goes
// out of scope. Pushed arenas are used automatically as the allocator in TLS
// suffixed function.
enum Dqn_TLSPushTMem
{
    Dqn_TLSPushTMem_No,
    Dqn_TLSPushTMem_Yes,
};

struct Dqn_TLSTMem
{
    Dqn_TLSTMem(Dqn_TLS *context, uint8_t context_index, Dqn_TLSPushTMem push_scratch);
    ~Dqn_TLSTMem();

    Dqn_Arena        *arena;
    Dqn_b32           destructed;
    Dqn_TLSPushTMem   push_arena;
    Dqn_ArenaTempMem  temp_mem;
};

DQN_API void        Dqn_TLS_Init(Dqn_TLS *tls);
DQN_API Dqn_TLS *   Dqn_TLS_Get();
#define             Dqn_TLS_SaveCallSite do { Dqn_TLS_Get()->call_site = DQN_CALL_SITE; } while (0)
DQN_API Dqn_TLSTMem Dqn_TLS_GetTMem(void const *conflict_arena, Dqn_TLSPushTMem push_tmp_mem);
#define             Dqn_TLS_TMem(...)     Dqn_TLS_GetTMem(__VA_ARGS__, Dqn_TLSPushTMem_No)
#define             Dqn_TLS_PushTMem(...) Dqn_TLS_GetTMem(__VA_ARGS__, Dqn_TLSPushTMem_Yes)
DQN_API void        Dqn_TLS_PushArena(Dqn_Arena *arena);
DQN_API void        Dqn_TLS_PopArena();
DQN_API Dqn_Arena * Dqn_TLS_TopArena();
