#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$\  $$$$$$$$\ $$$$$$$\  $$\   $$\  $$$$$$\
//   $$  __$$\ $$  _____|$$  __$$\ $$ |  $$ |$$  __$$\
//   $$ |  $$ |$$ |      $$ |  $$ |$$ |  $$ |$$ /  \__|
//   $$ |  $$ |$$$$$\    $$$$$$$\ |$$ |  $$ |$$ |$$$$\
//   $$ |  $$ |$$  __|   $$  __$$\ $$ |  $$ |$$ |\_$$ |
//   $$ |  $$ |$$ |      $$ |  $$ |$$ |  $$ |$$ |  $$ |
//   $$$$$$$  |$$$$$$$$\ $$$$$$$  |\$$$$$$  |\$$$$$$  |
//   \_______/ \________|\_______/  \______/  \______/
//
//   dqn_debug.h -- Tools for debugging
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$ASAN] DN_Asan       -- Helpers to manually poison memory using ASAN
// [$STKT] DN_StackTrace -- Create stack traces
// [$DEBG] DN_Debug      -- Allocation leak tracking API
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$ASAN] DN_Asan //////////////////////////////////////////////////////////////////////////
#if !defined(DN_ASAN_POISON)
    #define DN_ASAN_POISON 0
#endif

#if !defined(DN_ASAN_VET_POISON)
    #define DN_ASAN_VET_POISON 0
#endif

#define DN_ASAN_POISON_ALIGNMENT 8

#if !defined(DN_ASAN_POISON_GUARD_SIZE)
    #define DN_ASAN_POISON_GUARD_SIZE 128
#endif
static_assert(DN_IsPowerOfTwoAligned(DN_ASAN_POISON_GUARD_SIZE, DN_ASAN_POISON_ALIGNMENT),
              "ASAN poison guard size must be a power-of-two and aligned to ASAN's alignment"
              "requirement (8 bytes)");

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    #include <sanitizer/asan_interface.h>
#endif

// NOTE: [$STKT] DN_StackTrace ////////////////////////////////////////////////////////////////////
struct DN_StackTraceFrame
{
    uint64_t address;
    uint64_t line_number;
    DN_Str8 file_name;
    DN_Str8 function_name;
};

struct DN_StackTraceRawFrame
{
    void     *process;
    uint64_t  base_addr;
};

struct DN_StackTraceWalkResult
{
    void     *process;   // [Internal] Windows handle to the process
    uint64_t *base_addr; // The addresses of the functions in the stack trace
    uint16_t  size;      // The number of `base_addr`'s stored from the walk
};

struct DN_StackTraceWalkResultIterator
{
    DN_StackTraceRawFrame raw_frame;
    uint16_t               index;
};

// NOTE: [$DEBG] DN_Debug /////////////////////////////////////////////////////////////////////////
enum DN_DebugAllocFlag
{
    DN_DebugAllocFlag_Freed         = 1 << 0,
    DN_DebugAllocFlag_LeakPermitted = 1 << 1,
};

struct DN_DebugAlloc
{
    void      *ptr;               // 8  Pointer to the allocation being tracked
    DN_USize  size;              // 16 Size of the allocation
    DN_USize  freed_size;        // 24 Store the size of the allocation when it is freed
    DN_Str8   stack_trace;       // 40 Stack trace at the point of allocation
    DN_Str8   freed_stack_trace; // 56 Stack trace of where the allocation was freed
    uint16_t   flags;             // 72 Bit flags from `DN_DebugAllocFlag`
};

static_assert(sizeof(DN_DebugAlloc) == 64 || sizeof(DN_DebugAlloc) == 32, // NOTE: 64 bit vs 32 bit pointers respectively
              "We aim to keep the allocation record as light as possible as "
              "memory tracking can get expensive. Enforce that there is no "
              "unexpected padding.");

// NOTE: [$ASAN] DN_Asan //////////////////////////////////////////////////////////////////////////
DN_API void                         DN_ASAN_PoisonMemoryRegion     (void const volatile *ptr, DN_USize size);
DN_API void                         DN_ASAN_UnpoisonMemoryRegion   (void const volatile *ptr, DN_USize size);

// NOTE: [$STKT] DN_StackTrace ////////////////////////////////////////////////////////////////////
DN_API DN_StackTraceWalkResult      DN_StackTrace_Walk             (DN_Arena *arena, uint16_t limit);
DN_API DN_Str8                      DN_StackTrace_WalkStr8CRT      (uint16_t limit, uint16_t skip);
DN_API bool                         DN_StackTrace_WalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk);
DN_API DN_Str8                      DN_StackTrace_WalkResultStr8   (DN_Arena *arena, DN_StackTraceWalkResult const *walk, uint16_t skip);
DN_API DN_Str8                      DN_StackTrace_WalkResultStr8CRT(DN_StackTraceWalkResult const *walk, uint16_t skip);
DN_API DN_Slice<DN_StackTraceFrame> DN_StackTrace_GetFrames        (DN_Arena *arena, uint16_t limit);
DN_API DN_StackTraceFrame           DN_StackTrace_RawFrameToFrame  (DN_Arena *arena, DN_StackTraceRawFrame raw_frame);
DN_API void                         DN_StackTrace_Print            (uint16_t limit);
DN_API void                         DN_StackTrace_ReloadSymbols    ();

// NOTE: [$DEBG] DN_Debug /////////////////////////////////////////////////////////////////////////
#if defined(DN_LEAK_TRACKING)
DN_API void                         DN_Debug_TrackAlloc            (void *ptr, DN_USize size, bool alloc_can_leak);
DN_API void                         DN_Debug_TrackDealloc          (void *ptr);
DN_API void                         DN_Debug_DumpLeaks             ();
#else
#define                             DN_Debug_TrackAlloc(ptr, size, alloc_can_leak) do { (void)ptr; (void)size; (void)alloc_can_leak; } while (0)
#define                             DN_Debug_TrackDealloc(ptr)                     do { (void)ptr;                                   } while (0)
#define                             DN_Debug_DumpLeaks()                           do {                                              } while (0)
#endif


