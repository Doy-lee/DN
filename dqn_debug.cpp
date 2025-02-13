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
//   dqn_debug.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$ASAN] DN_Asan ////////////////////////////////////////////////////////////////////////// ///
DN_API void DN_ASAN_PoisonMemoryRegion(void const volatile *ptr, DN_USize size)
{
    if (!ptr || !size)
        return;

    #if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    DN_ASSERTF(DN_IsPowerOfTwoAligned(ptr, 8),
                "Poisoning requires the pointer to be aligned on an 8 byte boundary");

    __asan_poison_memory_region(ptr, size);
    if (DN_ASAN_VET_POISON) {
        DN_HARD_ASSERT(__asan_address_is_poisoned(ptr));
        DN_HARD_ASSERT(__asan_address_is_poisoned((char *)ptr + (size - 1)));
    }
    #else
    (void)ptr; (void)size;
    #endif
}

DN_API void DN_ASAN_UnpoisonMemoryRegion(void const volatile *ptr, DN_USize size)
{
    if (!ptr || !size)
        return;

    #if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    __asan_unpoison_memory_region(ptr, size);
    if (DN_ASAN_VET_POISON) {
        DN_HARD_ASSERT(__asan_region_is_poisoned((void *)ptr, size) == 0);
    }
    #else
    (void)ptr; (void)size;
    #endif
}

DN_API DN_StackTraceWalkResult DN_StackTrace_Walk(DN_Arena *arena, uint16_t limit)
{
    DN_StackTraceWalkResult result = {};
    #if defined(DN_OS_WIN32)
    if (!arena)
        return result;

    static DN_TicketMutex mutex   = {};
    DN_TicketMutex_Begin(&mutex);

    HANDLE thread  = GetCurrentThread();
    result.process = GetCurrentProcess();

    if (!g_dn_core->win32_sym_initialised) {
        g_dn_core->win32_sym_initialised = true;
        SymSetOptions(SYMOPT_LOAD_LINES);
        if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
            DN_TLSTMem  tmem  = DN_TLS_TMem(arena);
            DN_WinError error = DN_Win_LastError(tmem.arena);
            DN_Log_ErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DN_STR_FMT(error.msg));
        }
    }

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 frame     = {};
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode   = AddrModeFlat;

    DN_FArray<uint64_t, 256> raw_frames = {};
    while (raw_frames.size < limit) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                         result.process,
                         thread,
                         &frame,
                         &context,
                         nullptr /*ReadMemoryRoutine*/,
                         SymFunctionTableAccess64,
                         SymGetModuleBase64,
                         nullptr /*TranslateAddress*/)) {
            break;
        }

        // NOTE: It might be useful one day to use frame.AddrReturn.Offset.
        // If AddrPC.Offset == AddrReturn.Offset then we can detect recursion.
        DN_FArray_Add(&raw_frames, frame.AddrPC.Offset);
    }
    DN_TicketMutex_End(&mutex);

    result.base_addr = DN_Arena_NewArray(arena, uint64_t, raw_frames.size, DN_ZeroMem_No);
    result.size      = DN_CAST(uint16_t)raw_frames.size;
    DN_MEMCPY(result.base_addr, raw_frames.data, raw_frames.size * sizeof(raw_frames.data[0]));
    #else
    (void)limit; (void)arena;
    #endif
    return result;
}

DN_API DN_StackTraceWalkResult DN_StackTrace_WalkCRT(uint16_t limit)
{
    DN_StackTraceWalkResult result = {};
    #if defined(DN_OS_WIN32)
    static DN_TicketMutex mutex   = {};
    DN_TicketMutex_Begin(&mutex);

    HANDLE thread  = GetCurrentThread();
    result.process = GetCurrentProcess();

    if (!g_dn_core->win32_sym_initialised) {
        g_dn_core->win32_sym_initialised = true;
        SymSetOptions(SYMOPT_LOAD_LINES);
        if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
            DN_WinError error = DN_Win_LastErrorAlloc();
            DN_Log_ErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DN_STR_FMT(error.msg));
            DN_OS_MemDealloc(error.msg.data);
        }
    }

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 frame     = {};
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode   = AddrModeFlat;

    struct FrameChunk
    {
        uint64_t    frames[128];
        FrameChunk *next;
        uint8_t     size;
    };

    DN_USize   total_frames   = 0;
    FrameChunk  frame_chunk    = {};
    FrameChunk *frame_chunk_it = &frame_chunk;
    for (; total_frames < limit; total_frames++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                         result.process,
                         thread,
                         &frame,
                         &context,
                         nullptr /*ReadMemoryRoutine*/,
                         SymFunctionTableAccess64,
                         SymGetModuleBase64,
                         nullptr /*TranslateAddress*/)) {
            break;
        }

        // NOTE: It might be useful one day to use frame.AddrReturn.Offset.
        // If AddrPC.Offset == AddrReturn.Offset then we can detect recursion.
        if (frame_chunk_it->size == DN_ARRAY_UCOUNT(frame_chunk_it->frames)) {
            FrameChunk *next = DN_CAST(FrameChunk *) DN_OS_MemAlloc(sizeof(*next), DN_ZeroMem_No);
            frame_chunk_it = next;
        }

        if (!frame_chunk_it)
            break;
        frame_chunk_it->frames[frame_chunk_it->size++] = frame.AddrPC.Offset;
    }
    DN_TicketMutex_End(&mutex);

    result.base_addr = DN_CAST(uint64_t *)DN_OS_MemAlloc(sizeof(*result.base_addr) * total_frames, DN_ZeroMem_No);
    for (FrameChunk *it = &frame_chunk; it; ) {
        FrameChunk *next = it->next;

        // NOTE: Copy
        DN_MEMCPY(result.base_addr, it->frames, it->size * sizeof(it->frames[0]));
        result.size += it->size;

        // NOTE: Free
        if (it != &frame_chunk)
            DN_OS_MemDealloc(it);
        it = next;
    }
    #else
    DN_INVALID_CODE_PATH;
    (void)limit;
    #endif
    return result;
}

DN_API DN_Str8 DN_StackTrace_WalkStr8CRT(uint16_t limit, uint16_t skip)
{
    DN_StackTraceWalkResult walk_result = DN_StackTrace_WalkCRT(limit);
    DN_Str8                 result      = DN_StackTrace_WalkResultStr8CRT(&walk_result, skip);
    return result;
}

static void DN_StackTrace_AddWalkToStr8Builder_(DN_StackTraceWalkResult const *walk, DN_Str8Builder *builder, DN_USize skip)
{
    DN_StackTraceRawFrame raw_frame = {};
    raw_frame.process                = walk->process;
    for (DN_USize index = skip; index < walk->size; index++) {
        raw_frame.base_addr       = walk->base_addr[index];
        DN_StackTraceFrame frame = DN_StackTrace_RawFrameToFrame(builder->arena, raw_frame);
        DN_Str8Builder_AppendF(builder, "%.*s(%zu): %.*s%s", DN_STR_FMT(frame.file_name), frame.line_number, DN_STR_FMT(frame.function_name), (DN_CAST(int)index == walk->size - 1) ? "" : "\n");
    }
}

DN_API DN_Str8 DN_StackTrace_WalkStr8CRTNoScratch(uint16_t limit, uint16_t skip)
{
    DN_Arena arena          = {};
    arena.flags             |= DN_ArenaFlags_NoAllocTrack;
    DN_DEFER { DN_Arena_Deinit(&arena); };

    DN_Str8Builder builder  = {};
    builder.arena            = &arena;

    DN_StackTraceWalkResult walk    = DN_StackTrace_Walk(&arena, limit);
    DN_StackTraceRawFrame raw_frame = {};
    raw_frame.process                = walk.process;
    for (DN_USize index = skip; index < walk.size; index++) {
        raw_frame.base_addr       = walk.base_addr[index];
        DN_StackTraceFrame frame = DN_StackTrace_RawFrameToFrame(builder.arena, raw_frame);
        DN_Str8Builder_AppendF(&builder, "%.*s(%zu): %.*s%s", DN_STR_FMT(frame.file_name), frame.line_number, DN_STR_FMT(frame.function_name), (DN_CAST(int)index == walk.size - 1) ? "" : "\n");
    }

    DN_Str8 result = {};
    result.data     = DN_CAST(char *)DN_OS_MemReserve(builder.string_size + 1, DN_OSMemCommit_Yes, DN_OSMemPage_ReadWrite);
    if (result.data) {
        for (DN_Str8Link *it = builder.head; it; it = it->next) {
            DN_MEMCPY(result.data + result.size, it->string.data, it->string.size);
            result.size += it->string.size;
        }
    }

    return result;
}

DN_API bool DN_StackTrace_WalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk)
{
    bool result = false;
    if (!it || !walk || !walk->base_addr || !walk->process)
        return result;

    if (it->index >= walk->size)
        return false;

    result                  = true;
    it->raw_frame.process   = walk->process;
    it->raw_frame.base_addr = walk->base_addr[it->index++];
    return result;
}

DN_API DN_Str8 DN_StackTrace_WalkResultStr8(DN_Arena *arena, DN_StackTraceWalkResult const *walk, uint16_t skip)
{
    DN_Str8 result  {};
    if (!walk || !arena)
        return result;

    DN_TLSTMem     tmem    = DN_TLS_TMem(arena);
    DN_Str8Builder builder = DN_Str8Builder_Init(tmem.arena);
    DN_StackTrace_AddWalkToStr8Builder_(walk, &builder, skip);
    result = DN_Str8Builder_Build(&builder, arena);
    return result;
}

DN_API DN_Str8 DN_StackTrace_WalkResultStr8CRT(DN_StackTraceWalkResult const *walk, uint16_t skip)
{
    DN_Str8 result  {};
    if (!walk)
        return result;

    DN_TLSTMem     tmem    = DN_TLS_TMem(nullptr);
    DN_Str8Builder builder = DN_Str8Builder_Init(tmem.arena);
    DN_StackTrace_AddWalkToStr8Builder_(walk, &builder, skip);
    result = DN_Str8Builder_BuildCRT(&builder);
    return result;
}


DN_API DN_Slice<DN_StackTraceFrame> DN_StackTrace_GetFrames(DN_Arena *arena, uint16_t limit)
{
    DN_Slice<DN_StackTraceFrame> result = {};
    if (!arena)
        return result;

    DN_TLSTMem              tmem = DN_TLS_TMem(arena);
    DN_StackTraceWalkResult walk = DN_StackTrace_Walk(tmem.arena, limit);
    if (!walk.size)
        return result;

    DN_USize slice_index = 0;
    result                = DN_Slice_Alloc<DN_StackTraceFrame>(arena, walk.size, DN_ZeroMem_No);
    for (DN_StackTraceWalkResultIterator it = {}; DN_StackTrace_WalkResultIterate(&it, &walk); ) {
        result.data[slice_index++] = DN_StackTrace_RawFrameToFrame(arena, it.raw_frame);
    }
    return result;
}

DN_API DN_StackTraceFrame DN_StackTrace_RawFrameToFrame(DN_Arena *arena, DN_StackTraceRawFrame raw_frame)
{
    #if defined(DN_OS_WIN32)
    // NOTE: Get line+filename /////////////////////////////////////////////////////////////////////

    // TODO: Why does zero-initialising this with `line = {};` cause
    // SymGetLineFromAddr64 function to fail once we are at
    // __scrt_commain_main_seh and hit BaseThreadInitThunk frame? The
    // line and file number are still valid in the result which we use, so,
    // we silently ignore this error.
    IMAGEHLP_LINEW64 line;
    line.SizeOfStruct       = sizeof(line);
    DWORD line_displacement = 0;
    if (!SymGetLineFromAddrW64(raw_frame.process, raw_frame.base_addr, &line_displacement, &line)) {
        line = {};
    }

    // NOTE: Get function name /////////////////////////////////////////////////////////////////////

    alignas(SYMBOL_INFOW) char buffer[sizeof(SYMBOL_INFOW) + (MAX_SYM_NAME * sizeof(wchar_t))] = {};
    SYMBOL_INFOW *symbol = DN_CAST(SYMBOL_INFOW *)buffer;
    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen   = sizeof(buffer) - sizeof(*symbol);

    uint64_t symbol_displacement = 0; // Offset to the beginning of the symbol to the address
    SymFromAddrW(raw_frame.process, raw_frame.base_addr, &symbol_displacement, symbol);

    // NOTE: Construct result //////////////////////////////////////////////////////////////////////

    DN_Str16 file_name16     = DN_Str16{line.FileName, DN_CStr16_Size(line.FileName)};
    DN_Str16 function_name16 = DN_Str16{symbol->Name, symbol->NameLen};

    DN_StackTraceFrame result = {};
    result.address             = raw_frame.base_addr;
    result.line_number         = line.LineNumber;
    result.file_name           = DN_Win_Str16ToStr8(arena, file_name16);
    result.function_name       = DN_Win_Str16ToStr8(arena, function_name16);

    if (!DN_Str8_HasData(result.function_name))
        result.function_name = DN_STR8("<unknown function>");
    if (!DN_Str8_HasData(result.file_name))
        result.file_name = DN_STR8("<unknown file>");
    #else
    DN_StackTraceFrame result = {};
    #endif
    return result;
}

DN_API void DN_StackTrace_Print(uint16_t limit)
{
    DN_TLSTMem                    tmem        = DN_TLS_TMem(nullptr);
    DN_Slice<DN_StackTraceFrame> stack_trace = DN_StackTrace_GetFrames(tmem.arena, limit);
    for (DN_StackTraceFrame &frame : stack_trace)
        DN_Print_ErrLnF("%.*s(%I64u): %.*s", DN_STR_FMT(frame.file_name), frame.line_number, DN_STR_FMT(frame.function_name));
}

DN_API void DN_StackTrace_ReloadSymbols()
{
    #if defined(DN_OS_WIN32)
    HANDLE process = GetCurrentProcess();
    SymRefreshModuleList(process);
    #endif
}

// NOTE: [$DEBG] DN_Debug /////////////////////////////////////////////////////////////////////////
#if defined(DN_LEAK_TRACKING)
DN_API void DN_Debug_TrackAlloc(void *ptr, DN_USize size, bool leak_permitted)
{
    if (!ptr)
        return;

    DN_TicketMutex_Begin(&g_dn_core->alloc_table_mutex);
    DN_DEFER {
        DN_TicketMutex_End(&g_dn_core->alloc_table_mutex);
    };

    // NOTE: If the entry was not added, we are reusing a pointer that has been freed.
    // TODO: Add API for always making the item but exposing a var to indicate if the item was newly created or it
    // already existed.
    DN_Str8                        stack_trace = DN_StackTrace_WalkStr8CRTNoScratch(128, 3 /*skip*/);
    DN_DSMap<DN_DebugAlloc>      *alloc_table = &g_dn_core->alloc_table;
    DN_DSMapResult<DN_DebugAlloc> alloc_entry = DN_DSMap_MakeKeyU64(alloc_table, DN_CAST(uint64_t) ptr);
    DN_DebugAlloc                 *alloc       = alloc_entry.value;
    if (alloc_entry.found) {
        if ((alloc->flags & DN_DebugAllocFlag_Freed) == 0) {
            DN_Str8 alloc_size     = DN_U64ToByteSizeStr8(alloc_table->arena, alloc->size, DN_U64ByteSizeType_Auto);
            DN_Str8 new_alloc_size = DN_U64ToByteSizeStr8(alloc_table->arena, size, DN_U64ByteSizeType_Auto);
            DN_HARD_ASSERTF(
                alloc->flags & DN_DebugAllocFlag_Freed,
                "This pointer is already in the leak tracker, however it has not "
                "been freed yet. This same pointer is being ask to be tracked "
                "twice in the allocation table, e.g. one if its previous free "
                "calls has not being marked freed with an equivalent call to "
                "DN_Debug_TrackDealloc()\n"
                "\n"
                "The pointer (0x%p) originally allocated %.*s at:\n"
                "\n"
                "%.*s\n"
                "\n"
                "The pointer is allocating %.*s again at:\n"
                "\n"
                "%.*s\n"
                ,
                ptr, DN_STR_FMT(alloc_size),
                DN_STR_FMT(alloc->stack_trace),
                DN_STR_FMT(new_alloc_size),
                DN_STR_FMT(stack_trace));
        }

        // NOTE: Pointer was reused, clean up the prior entry
        DN_OS_MemRelease(alloc->stack_trace.data, alloc->stack_trace.size);
        DN_OS_MemRelease(alloc->freed_stack_trace.data, alloc->freed_stack_trace.size);
        *alloc = {};
    }

    alloc->ptr          = ptr;
    alloc->size         = size;
    alloc->stack_trace  = stack_trace;
    alloc->flags       |= leak_permitted ? DN_DebugAllocFlag_LeakPermitted : 0;
}

DN_API void DN_Debug_TrackDealloc(void *ptr)
{
    if (!ptr)
        return;

    DN_TicketMutex_Begin(&g_dn_core->alloc_table_mutex);
    DN_DEFER { DN_TicketMutex_End(&g_dn_core->alloc_table_mutex); };

    DN_Str8                        stack_trace = DN_StackTrace_WalkStr8CRTNoScratch(128, 3 /*skip*/);
    DN_DSMap<DN_DebugAlloc>      *alloc_table = &g_dn_core->alloc_table;
    DN_DSMapResult<DN_DebugAlloc> alloc_entry = DN_DSMap_FindKeyU64(alloc_table, DN_CAST(uintptr_t) ptr);
    DN_HARD_ASSERTF(alloc_entry.found,
                     "Allocated pointer can not be removed as it does not exist in the "
                     "allocation table. When this memory was allocated, the pointer was "
                     "not added to the allocation table [ptr=%p]",
                     ptr);

    DN_DebugAlloc *alloc = alloc_entry.value;
    if (alloc->flags & DN_DebugAllocFlag_Freed) {
        DN_Str8 freed_size = DN_U64ToByteSizeStr8(alloc_table->arena, alloc->freed_size, DN_U64ByteSizeType_Auto);
        DN_HARD_ASSERTF((alloc->flags & DN_DebugAllocFlag_Freed) == 0,
                         "Double free detected, pointer to free was already marked "
                         "as freed. Either the pointer was reallocated but not "
                         "traced, or, the pointer was freed twice.\n"
                         "\n"
                         "The pointer (0x%p) originally allocated %.*s at:\n"
                         "\n"
                         "%.*s\n"
                         "\n"
                         "The pointer was freed at:\n"
                         "\n"
                         "%.*s\n"
                         "\n"
                         "The pointer is being freed again at:\n"
                         "\n"
                         "%.*s\n"
                         ,
                         ptr, DN_STR_FMT(freed_size),
                         DN_STR_FMT(alloc->stack_trace),
                         DN_STR_FMT(alloc->freed_stack_trace),
                         DN_STR_FMT(stack_trace));
    }

    DN_ASSERT(!DN_Str8_HasData(alloc->freed_stack_trace));
    alloc->flags             |= DN_DebugAllocFlag_Freed;
    alloc->freed_stack_trace  = stack_trace;
}

DN_API void DN_Debug_DumpLeaks()
{
    uint64_t leak_count   = 0;
    uint64_t leaked_bytes = 0;
    for (DN_USize index = 1; index < g_dn_core->alloc_table.occupied; index++) {
        DN_DSMapSlot<DN_DebugAlloc> *slot           = g_dn_core->alloc_table.slots + index;
        DN_DebugAlloc                *alloc          = &slot->value;
        bool                           alloc_leaked   = (alloc->flags & DN_DebugAllocFlag_Freed) == 0;
        bool                           leak_permitted = (alloc->flags & DN_DebugAllocFlag_LeakPermitted);
        if (alloc_leaked && !leak_permitted) {
            leaked_bytes += alloc->size;
            leak_count++;
            DN_Str8 alloc_size = DN_U64ToByteSizeStr8(g_dn_core->alloc_table.arena, alloc->size, DN_U64ByteSizeType_Auto);
            DN_Log_WarningF("Pointer (0x%p) leaked %.*s at:\n"
                             "%.*s",
                             alloc->ptr, DN_STR_FMT(alloc_size),
                             DN_STR_FMT(alloc->stack_trace));
        }
    }

    if (leak_count) {
        DN_Str8 leak_size = DN_U64ToByteSizeStr8(&g_dn_core->arena, leaked_bytes, DN_U64ByteSizeType_Auto);
        DN_Log_WarningF("There were %I64u leaked allocations totalling %.*s", leak_count, DN_STR_FMT(leak_size));
    }
}
#endif // DN_LEAK_TRACKING
