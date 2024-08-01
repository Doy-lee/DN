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

// NOTE: [$ASAN] Dqn_Asan ////////////////////////////////////////////////////////////////////////// ///
DQN_API void Dqn_ASAN_PoisonMemoryRegion(void const volatile *ptr, Dqn_usize size)
{
    if (!ptr || !size)
        return;

    #if DQN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    __asan_poison_memory_region(ptr, size);
    if (DQN_ASAN_VET_POISON) {
        DQN_HARD_ASSERT(__asan_address_is_poisoned(ptr));
        DQN_HARD_ASSERT(__asan_address_is_poisoned((char *)ptr + (size - 1)));
    }
    #else
    (void)ptr; (void)size;
    #endif
}

DQN_API void Dqn_ASAN_UnpoisonMemoryRegion(void const volatile *ptr, Dqn_usize size)
{
    if (!ptr || !size)
        return;

    #if DQN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    __asan_unpoison_memory_region(ptr, size);
    if (DQN_ASAN_VET_POISON) {
        DQN_HARD_ASSERT(__asan_region_is_poisoned((void *)ptr, size) == 0);
    }
    #else
    (void)ptr; (void)size;
    #endif
}

DQN_API Dqn_StackTraceWalkResult Dqn_StackTrace_Walk(Dqn_Arena *arena, uint16_t limit)
{
    Dqn_StackTraceWalkResult result = {};
    #if defined(DQN_OS_WIN32)
    if (!arena)
        return result;

    static Dqn_TicketMutex mutex   = {};
    Dqn_TicketMutex_Begin(&mutex);

    HANDLE thread  = GetCurrentThread();
    result.process = GetCurrentProcess();

    if (!g_dqn_library->win32_sym_initialised) {
        g_dqn_library->win32_sym_initialised = true;
        SymSetOptions(SYMOPT_LOAD_LINES);
        if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
            Dqn_TLSTMem  tmem  = Dqn_TLS_TMem(arena);
            Dqn_WinError error = Dqn_Win_LastError(tmem.arena);
            Dqn_Log_ErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DQN_STR_FMT(error.msg));
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

    Dqn_FArray<uint64_t, 256> raw_frames = {};
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
        Dqn_FArray_Add(&raw_frames, frame.AddrPC.Offset);
    }
    Dqn_TicketMutex_End(&mutex);

    result.base_addr = Dqn_Arena_NewArray(arena, uint64_t, raw_frames.size, Dqn_ZeroMem_No);
    result.size      = DQN_CAST(uint16_t)raw_frames.size;
    DQN_MEMCPY(result.base_addr, raw_frames.data, raw_frames.size * sizeof(raw_frames.data[0]));
    #else
    (void)limit; (void)arena;
    #endif
    return result;
}

DQN_API Dqn_Str8 Dqn_StackTrace_WalkStr8CRT(uint16_t limit, uint16_t skip)
{
    Dqn_TLSTMem              tmem        = Dqn_TLS_TMem(nullptr);
    Dqn_StackTraceWalkResult walk_result = Dqn_StackTrace_Walk(tmem.arena, limit);
    Dqn_Str8                 result      = Dqn_StackTrace_WalkResultStr8CRT(&walk_result, skip);
    return result;
}

static void Dqn_StackTrace_AddWalkToStr8Builder_(Dqn_StackTraceWalkResult const *walk, Dqn_Str8Builder *builder, Dqn_usize skip)
{
    Dqn_StackTraceRawFrame raw_frame = {};
    raw_frame.process                = walk->process;
    for (Dqn_usize index = skip; index < walk->size; index++) {
        raw_frame.base_addr       = walk->base_addr[index];
        Dqn_StackTraceFrame frame = Dqn_StackTrace_RawFrameToFrame(builder->arena, raw_frame);
        Dqn_Str8Builder_AddF(builder, "%.*s(%zu): %.*s%s", DQN_STR_FMT(frame.file_name), frame.line_number, DQN_STR_FMT(frame.function_name), (DQN_CAST(int)index == walk->size - 1) ? "" : "\n");
    }
}

DQN_API Dqn_Str8 Dqn_StackTrace_WalkStr8CRTNoScratch(uint16_t limit, uint16_t skip)
{
    Dqn_Arena arena                = {};
    arena.flags                   |= Dqn_ArenaFlag_NoAllocTrack;
    Dqn_StackTraceWalkResult walk  = Dqn_StackTrace_Walk(&arena, limit);

    Dqn_Str8Builder builder = {};
    builder.arena           = &arena;
    Dqn_StackTrace_AddWalkToStr8Builder_(&walk, &builder, skip);
    Dqn_Str8 result = Dqn_Str8Builder_BuildCRT(&builder);
    Dqn_Arena_Deinit(&arena);
    return result;
}

DQN_API bool Dqn_StackTrace_WalkResultIterate(Dqn_StackTraceWalkResultIterator *it, Dqn_StackTraceWalkResult const *walk)
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

DQN_API Dqn_Str8 Dqn_StackTrace_WalkResultStr8(Dqn_Arena *arena, Dqn_StackTraceWalkResult const *walk, uint16_t skip)
{
    Dqn_Str8 result  {};
    if (!walk || !arena)
        return result;

    Dqn_TLSTMem     tmem    = Dqn_TLS_TMem(arena);
    Dqn_Str8Builder builder = Dqn_Str8Builder_Init(tmem.arena);
    Dqn_StackTrace_AddWalkToStr8Builder_(walk, &builder, skip);
    result = Dqn_Str8Builder_Build(&builder, arena);
    return result;
}

DQN_API Dqn_Str8 Dqn_StackTrace_WalkResultStr8CRT(Dqn_StackTraceWalkResult const *walk, uint16_t skip)
{
    Dqn_Str8 result  {};
    if (!walk)
        return result;

    Dqn_TLSTMem     tmem    = Dqn_TLS_TMem(nullptr);
    Dqn_Str8Builder builder = Dqn_Str8Builder_Init(tmem.arena);
    Dqn_StackTrace_AddWalkToStr8Builder_(walk, &builder, skip);
    result = Dqn_Str8Builder_BuildCRT(&builder);
    return result;
}


DQN_API Dqn_Slice<Dqn_StackTraceFrame> Dqn_StackTrace_GetFrames(Dqn_Arena *arena, uint16_t limit)
{
    Dqn_Slice<Dqn_StackTraceFrame> result = {};
    if (!arena)
        return result;

    Dqn_TLSTMem              tmem = Dqn_TLS_TMem(arena);
    Dqn_StackTraceWalkResult walk = Dqn_StackTrace_Walk(tmem.arena, limit);
    if (!walk.size)
        return result;

    Dqn_usize slice_index = 0;
    result                = Dqn_Slice_Alloc<Dqn_StackTraceFrame>(arena, walk.size, Dqn_ZeroMem_No);
    for (Dqn_StackTraceWalkResultIterator it = {}; Dqn_StackTrace_WalkResultIterate(&it, &walk); ) {
        result.data[slice_index++] = Dqn_StackTrace_RawFrameToFrame(arena, it.raw_frame);
    }
    return result;
}

DQN_API Dqn_StackTraceFrame Dqn_StackTrace_RawFrameToFrame(Dqn_Arena *arena, Dqn_StackTraceRawFrame raw_frame)
{
    #if defined(DQN_OS_WIN32)
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
    SYMBOL_INFOW *symbol = DQN_CAST(SYMBOL_INFOW *)buffer;
    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen   = sizeof(buffer) - sizeof(*symbol);

    uint64_t symbol_displacement = 0; // Offset to the beginning of the symbol to the address
    SymFromAddrW(raw_frame.process, raw_frame.base_addr, &symbol_displacement, symbol);

    // NOTE: Construct result //////////////////////////////////////////////////////////////////////

    Dqn_Str16 file_name16     = Dqn_Str16{line.FileName, Dqn_CStr16_Size(line.FileName)};
    Dqn_Str16 function_name16 = Dqn_Str16{symbol->Name, symbol->NameLen};

    Dqn_StackTraceFrame result = {};
    result.address             = raw_frame.base_addr;
    result.line_number         = line.LineNumber;
    result.file_name           = Dqn_Win_Str16ToStr8(arena, file_name16);
    result.function_name       = Dqn_Win_Str16ToStr8(arena, function_name16);

    if (!Dqn_Str8_HasData(result.function_name))
        result.function_name = DQN_STR8("<unknown function>");
    if (!Dqn_Str8_HasData(result.file_name))
        result.file_name = DQN_STR8("<unknown file>");
    #else
    Dqn_StackTraceFrame result = {};
    #endif
    return result;
}

DQN_API void Dqn_StackTrace_Print(uint16_t limit)
{
    Dqn_TLSTMem                    tmem        = Dqn_TLS_TMem(nullptr);
    Dqn_Slice<Dqn_StackTraceFrame> stack_trace = Dqn_StackTrace_GetFrames(tmem.arena, limit);
    for (Dqn_StackTraceFrame &frame : stack_trace)
        Dqn_Print_ErrLnF("%.*s(%I64u): %.*s", DQN_STR_FMT(frame.file_name), frame.line_number, DQN_STR_FMT(frame.function_name));
}

DQN_API void Dqn_StackTrace_ReloadSymbols()
{
    #if defined(DQN_OS_WIN32)
    HANDLE process = GetCurrentProcess();
    SymRefreshModuleList(process);
    #endif
}

// NOTE: [$DEBG] Dqn_Debug /////////////////////////////////////////////////////////////////////////
#if defined(DQN_LEAK_TRACKING)
DQN_API void Dqn_Debug_TrackAlloc(void *ptr, Dqn_usize size, bool leak_permitted)
{
    if (!ptr)
        return;

    Dqn_TicketMutex_Begin(&g_dqn_library->alloc_table_mutex);
    DQN_DEFER {
        Dqn_TicketMutex_End(&g_dqn_library->alloc_table_mutex);
    };

    // NOTE: If the entry was not added, we are reusing a pointer that has been freed.
    // TODO: Add API for always making the item but exposing a var to indicate if the item was newly created or it
    // already existed.
    Dqn_Str8                        stack_trace = Dqn_StackTrace_WalkStr8CRTNoScratch(128, 3 /*skip*/);
    Dqn_DSMap<Dqn_DebugAlloc>      *alloc_table = &g_dqn_library->alloc_table;
    Dqn_DSMapResult<Dqn_DebugAlloc> alloc_entry = Dqn_DSMap_MakeKeyU64(alloc_table, DQN_CAST(uint64_t) ptr);
    Dqn_DebugAlloc                 *alloc       = alloc_entry.value;
    if (alloc_entry.found) {
        if ((alloc->flags & Dqn_DebugAllocFlag_Freed) == 0) {
            Dqn_Str8 alloc_size     = Dqn_U64ToByteSizeStr8(alloc_table->arena, alloc->size, Dqn_U64ByteSizeType_Auto);
            Dqn_Str8 new_alloc_size = Dqn_U64ToByteSizeStr8(alloc_table->arena, size, Dqn_U64ByteSizeType_Auto);
            DQN_HARD_ASSERTF(
                alloc->flags & Dqn_DebugAllocFlag_Freed,
                "This pointer is already in the leak tracker, however it has not "
                "been freed yet. This same pointer is being ask to be tracked "
                "twice in the allocation table, e.g. one if its previous free "
                "calls has not being marked freed with an equivalent call to "
                "Dqn_Debug_TrackDealloc()\n"
                "\n"
                "The pointer (0x%p) originally allocated %.*s at:\n"
                "\n"
                "%.*s\n"
                "\n"
                "The pointer is allocating %.*s again at:\n"
                "\n"
                "%.*s\n"
                ,
                ptr, DQN_STR_FMT(alloc_size),
                DQN_STR_FMT(alloc->stack_trace),
                DQN_STR_FMT(new_alloc_size),
                DQN_STR_FMT(stack_trace));
        }

        // NOTE: Pointer was reused, clean up the prior entry
        free(alloc->stack_trace.data);
        free(alloc->freed_stack_trace.data);
        *alloc = {};
    }

    alloc->ptr          = ptr;
    alloc->size         = size;
    alloc->stack_trace  = stack_trace;
    alloc->flags       |= leak_permitted ? Dqn_DebugAllocFlag_LeakPermitted : 0;
}

DQN_API void Dqn_Debug_TrackDealloc(void *ptr)
{
    if (!ptr)
        return;

    Dqn_TicketMutex_Begin(&g_dqn_library->alloc_table_mutex);
    DQN_DEFER { Dqn_TicketMutex_End(&g_dqn_library->alloc_table_mutex); };

    Dqn_Str8                        stack_trace = Dqn_StackTrace_WalkStr8CRTNoScratch(128, 3 /*skip*/);
    Dqn_DSMap<Dqn_DebugAlloc>      *alloc_table = &g_dqn_library->alloc_table;
    Dqn_DSMapResult<Dqn_DebugAlloc> alloc_entry = Dqn_DSMap_FindKeyU64(alloc_table, DQN_CAST(uintptr_t) ptr);
    DQN_HARD_ASSERTF(alloc_entry.found,
                     "Allocated pointer can not be removed as it does not exist in the "
                     "allocation table. When this memory was allocated, the pointer was "
                     "not added to the allocation table [ptr=%p]",
                     ptr);

    Dqn_DebugAlloc *alloc = alloc_entry.value;
    if (alloc->flags & Dqn_DebugAllocFlag_Freed) {
        Dqn_Str8 freed_size = Dqn_U64ToByteSizeStr8(alloc_table->arena, alloc->freed_size, Dqn_U64ByteSizeType_Auto);
        DQN_HARD_ASSERTF((alloc->flags & Dqn_DebugAllocFlag_Freed) == 0,
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
                         ptr, DQN_STR_FMT(freed_size),
                         DQN_STR_FMT(alloc->stack_trace),
                         DQN_STR_FMT(alloc->freed_stack_trace),
                         DQN_STR_FMT(stack_trace));
    }

    DQN_ASSERT(!Dqn_Str8_HasData(alloc->freed_stack_trace));
    alloc->flags             |= Dqn_DebugAllocFlag_Freed;
    alloc->freed_stack_trace  = stack_trace;
}

DQN_API void Dqn_Debug_DumpLeaks()
{
    uint64_t leak_count   = 0;
    uint64_t leaked_bytes = 0;
    for (Dqn_usize index = 1; index < g_dqn_library->alloc_table.occupied; index++) {
        Dqn_DSMapSlot<Dqn_DebugAlloc> *slot           = g_dqn_library->alloc_table.slots + index;
        Dqn_DebugAlloc                *alloc          = &slot->value;
        bool                           alloc_leaked   = (alloc->flags & Dqn_DebugAllocFlag_Freed) == 0;
        bool                           leak_permitted = (alloc->flags & Dqn_DebugAllocFlag_LeakPermitted);
        if (alloc_leaked && !leak_permitted) {
            leaked_bytes += alloc->size;
            leak_count++;
            Dqn_Str8 alloc_size = Dqn_U64ToByteSizeStr8(g_dqn_library->alloc_table.arena, alloc->size, Dqn_U64ByteSizeType_Auto);
            Dqn_Log_WarningF("Pointer (0x%p) leaked %.*s at:\n"
                             "%.*s",
                             alloc->ptr, DQN_STR_FMT(alloc_size),
                             DQN_STR_FMT(alloc->stack_trace));
        }
    }

    if (leak_count) {
        Dqn_Str8 leak_size = Dqn_U64ToByteSizeStr8(&g_dqn_library->arena, leaked_bytes, Dqn_U64ByteSizeType_Auto);
        Dqn_Log_WarningF("There were %I64u leaked allocations totalling %.*s", leak_count, DQN_STR_FMT(leak_size));
    }
}
#endif // DQN_LEAK_TRACKING
