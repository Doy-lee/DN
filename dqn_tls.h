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

enum DN_TLSArena
{
    DN_TLSArena_Main,      // NOTE: Arena for Permanent allocations
    DN_TLSArena_ErrorSink, // NOTE: Arena for logging error information for this thread

    // NOTE: Per-thread scratch arenas (2 to prevent aliasing)
    DN_TLSArena_TMem0,
    DN_TLSArena_TMem1,

    DN_TLSArena_Count,
};

struct DN_TLS
{
    DN_B32       init; // Flag to track if Thread has been initialised
    uint64_t      thread_id;
    DN_CallSite  call_site;  // Stores call-site information when requested by thread
    DN_ErrSink   err_sink;   // Error handling state
    DN_Arena     arenas[DN_TLSArena_Count];

    // Push and pop arenas onto the stack. Functions suffixed 'TLS' will use
    // these arenas for memory allocation.
    DN_Arena *arena_stack[8];
    DN_USize  arena_stack_index;

    DN_Arena *frame_arena;
    char       name[64];
    uint8_t    name_size;
};

// Push the temporary memory arena when retrieved, popped when the arena goes
// out of scope. Pushed arenas are used automatically as the allocator in TLS
// suffixed function.
enum DN_TLSPushTMem
{
    DN_TLSPushTMem_No,
    DN_TLSPushTMem_Yes,
};

struct DN_TLSTMem
{
    DN_TLSTMem(DN_TLS *context, uint8_t context_index, DN_TLSPushTMem push_scratch);
    ~DN_TLSTMem();

    DN_Arena        *arena;
    DN_B32           destructed;
    DN_TLSPushTMem   push_arena;
    DN_ArenaTempMem  temp_mem;
};

DN_API void       DN_TLS_Init(DN_TLS *tls);
DN_API void       DN_TLS_Deinit(DN_TLS *tls);
DN_API DN_TLS *   DN_TLS_Get();
DN_API DN_Arena * DN_TLS_Arena();
#define           DN_TLS_SaveCallSite do { DN_TLS_Get()->call_site = DN_CALL_SITE; } while (0)
DN_API DN_TLSTMem DN_TLS_GetTMem(void const *conflict_arena, DN_TLSPushTMem push_tmp_mem);
#define           DN_TLS_TMem(...)     DN_TLS_GetTMem(__VA_ARGS__, DN_TLSPushTMem_No)
#define           DN_TLS_PushTMem(...) DN_TLS_GetTMem(__VA_ARGS__, DN_TLSPushTMem_Yes)
DN_API void       DN_TLS_PushArena(DN_Arena *arena);
DN_API void       DN_TLS_PopArena();
DN_API DN_Arena * DN_TLS_TopArena();

DN_API void       DN_TLS_BeginFrame(DN_Arena *frame_arena);
DN_API DN_Arena * DN_TLS_FrameArena();
