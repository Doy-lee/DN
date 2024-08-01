#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  $$$$$$$\
//  $$  __$$\
//  $$ |  $$ | $$$$$$\   $$$$$$$\  $$$$$$\
//  $$$$$$$\ | \____$$\ $$  _____|$$  __$$\
//  $$  __$$\  $$$$$$$ |\$$$$$$\  $$$$$$$$ |
//  $$ |  $$ |$$  __$$ | \____$$\ $$   ____|
//  $$$$$$$  |\$$$$$$$ |$$$$$$$  |\$$$$$$$\
//  \_______/  \_______|\_______/  \_______|
//
//  dqn_base.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
#if !defined(DQN_PLATFORM_ARM64) && !defined(DQN_PLATFORM_EMSCRIPTEN)
#if defined(DQN_COMPILER_GCC) || defined(DQN_COMPILER_CLANG)
#include <cpuid.h>
#endif

Dqn_CPUFeatureDecl g_dqn_cpu_feature_decl[Dqn_CPUFeature_Count];

DQN_API Dqn_CPUIDResult Dqn_CPU_ID(Dqn_CPUIDArgs args)
{
    Dqn_CPUIDResult result = {};
    __cpuidex(result.values, args.eax, args.ecx);
    return result;
}

DQN_API Dqn_usize Dqn_CPU_HasFeatureArray(Dqn_CPUReport const *report, Dqn_CPUFeatureQuery *features, Dqn_usize features_size)
{
    Dqn_usize       result = 0;
    Dqn_usize const BITS   = sizeof(report->features[0]) * 8;
    DQN_FOR_UINDEX(feature_index, features_size) {
        Dqn_CPUFeatureQuery *query       = features + feature_index;
        Dqn_usize            chunk_index = query->feature / BITS;
        Dqn_usize            chunk_bit   = query->feature % BITS;
        uint64_t             chunk       = report->features[chunk_index];
        query->available                 = chunk & (1ULL << chunk_bit);
        result                          += DQN_CAST(int)query->available;
    }

    return result;
}

DQN_API bool Dqn_CPU_HasFeature(Dqn_CPUReport const *report, Dqn_CPUFeature feature)
{
    Dqn_CPUFeatureQuery query = {};
    query.feature             = feature;
    bool result               = Dqn_CPU_HasFeatureArray(report, &query, 1) == 1;
    return result;
}

DQN_API bool Dqn_CPU_HasAllFeatures(Dqn_CPUReport const *report, Dqn_CPUFeature const *features, Dqn_usize features_size)
{
    bool result = true;
    for (Dqn_usize index = 0; result && index < features_size; index++)
        result &= Dqn_CPU_HasFeature(report, features[index]);
    return result;
}

DQN_API void Dqn_CPU_SetFeature(Dqn_CPUReport *report, Dqn_CPUFeature feature)
{
    DQN_ASSERT(feature < Dqn_CPUFeature_Count);
    Dqn_usize const BITS        = sizeof(report->features[0]) * 8;
    Dqn_usize       chunk_index = feature / BITS;
    Dqn_usize       chunk_bit   = feature % BITS;
    report->features[chunk_index] |= (1ULL << chunk_bit);
}

DQN_API Dqn_CPUReport Dqn_CPU_Report()
{
    Dqn_CPUReport   result                 = {};
    Dqn_CPUIDResult fn_0000_[512]          = {};
    Dqn_CPUIDResult fn_8000_[512]          = {};
    int const       EXTENDED_FUNC_BASE_EAX = 0x8000'0000;
    int const       REGISTER_SIZE          = sizeof(fn_0000_[0].reg.eax);

    // NOTE: Query standard/extended numbers ///////////////////////////////////////////////////////
    {
        Dqn_CPUIDArgs args = {};

        // NOTE: Query standard function (e.g. eax = 0x0)         for function count + cpu vendor
        args        = {};
        fn_0000_[0] = Dqn_CPU_ID(args);

        // NOTE: Query extended function (e.g. eax = 0x8000'0000) for function count + cpu vendor
        args        = {};
        args.eax    = DQN_CAST(int) EXTENDED_FUNC_BASE_EAX;
        fn_8000_[0] = Dqn_CPU_ID(args);
    }

    // NOTE: Extract function count ////////////////////////////////////////////////////////////////
    int const STANDARD_FUNC_MAX_EAX = fn_0000_[0x0000].reg.eax;
    int const EXTENDED_FUNC_MAX_EAX = fn_8000_[0x0000].reg.eax;

    // NOTE: Enumerate all CPUID results for the known function counts /////////////////////////////
    {
        DQN_ASSERTF((STANDARD_FUNC_MAX_EAX                                             + 1) <= DQN_ARRAY_ICOUNT(fn_0000_),
                    "Max standard count is %zu", STANDARD_FUNC_MAX_EAX + 1);
        DQN_ASSERTF((DQN_CAST(Dqn_isize)EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1) <= DQN_ARRAY_ICOUNT(fn_8000_),
                    "Max extended count is %zu", DQN_CAST(Dqn_isize)EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1);

        for (int eax = 1; eax <= STANDARD_FUNC_MAX_EAX; eax++) {
            Dqn_CPUIDArgs args = {};
            args.eax           = eax;
            fn_0000_[eax]      = Dqn_CPU_ID(args);
        }

        for (int eax = EXTENDED_FUNC_BASE_EAX + 1, index = 1; eax <= EXTENDED_FUNC_MAX_EAX; eax++, index++) {
            Dqn_CPUIDArgs args = {};
            args.eax           = eax;
            fn_8000_[index]    = Dqn_CPU_ID(args);
        }
    }

    // NOTE: Query CPU vendor //////////////////////////////////////////////////////////////////////
    {
        DQN_MEMCPY(result.vendor + 0, &fn_8000_[0x0000].reg.ebx, REGISTER_SIZE);
        DQN_MEMCPY(result.vendor + 4, &fn_8000_[0x0000].reg.edx, REGISTER_SIZE);
        DQN_MEMCPY(result.vendor + 8, &fn_8000_[0x0000].reg.ecx, REGISTER_SIZE);
    }

    // NOTE: Query CPU brand ///////////////////////////////////////////////////////////////////////
    if (EXTENDED_FUNC_MAX_EAX >= (EXTENDED_FUNC_BASE_EAX + 4)) {
        DQN_MEMCPY(result.brand + 0,  &fn_8000_[0x0002].reg.eax, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 4,  &fn_8000_[0x0002].reg.ebx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 8,  &fn_8000_[0x0002].reg.ecx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 12, &fn_8000_[0x0002].reg.edx, REGISTER_SIZE);

        DQN_MEMCPY(result.brand + 16, &fn_8000_[0x0003].reg.eax, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 20, &fn_8000_[0x0003].reg.ebx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 24, &fn_8000_[0x0003].reg.ecx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 28, &fn_8000_[0x0003].reg.edx, REGISTER_SIZE);

        DQN_MEMCPY(result.brand + 32, &fn_8000_[0x0004].reg.eax, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 36, &fn_8000_[0x0004].reg.ebx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 40, &fn_8000_[0x0004].reg.ecx, REGISTER_SIZE);
        DQN_MEMCPY(result.brand + 44, &fn_8000_[0x0004].reg.edx, REGISTER_SIZE);

        DQN_ASSERT(result.brand[sizeof(result.brand) - 1] == 0);
    }

    // NOTE: Query CPU features //////////////////////////////////////////////////////////////////
    for (Dqn_usize ext_index = 0; ext_index < Dqn_CPUFeature_Count; ext_index++) {
        bool available = false;

        // NOTE: Mask bits taken from various manuals
        //   - AMD64 Architecture Programmer's Manual, Volumes 1-5
        //   - https://en.wikipedia.org/wiki/CPUID#Calling_CPUID
        switch (DQN_CAST(Dqn_CPUFeature)ext_index) {
            case Dqn_CPUFeature_3DNow:              available = (fn_8000_[0x0001].reg.edx & (1 << 31)); break;
            case Dqn_CPUFeature_3DNowExt:           available = (fn_8000_[0x0001].reg.edx & (1 << 30)); break;
            case Dqn_CPUFeature_ABM:                available = (fn_8000_[0x0001].reg.ecx & (1 <<  5)); break;
            case Dqn_CPUFeature_AES:                available = (fn_0000_[0x0001].reg.ecx & (1 << 25)); break;
            case Dqn_CPUFeature_AVX:                available = (fn_0000_[0x0001].reg.ecx & (1 << 28)); break;
            case Dqn_CPUFeature_AVX2:               available = (fn_0000_[0x0007].reg.ebx & (1 <<  0)); break;
            case Dqn_CPUFeature_AVX512F:            available = (fn_0000_[0x0007].reg.ebx & (1 << 16)); break;
            case Dqn_CPUFeature_AVX512DQ:           available = (fn_0000_[0x0007].reg.ebx & (1 << 17)); break;
            case Dqn_CPUFeature_AVX512IFMA:         available = (fn_0000_[0x0007].reg.ebx & (1 << 21)); break;
            case Dqn_CPUFeature_AVX512PF:           available = (fn_0000_[0x0007].reg.ebx & (1 << 26)); break;
            case Dqn_CPUFeature_AVX512ER:           available = (fn_0000_[0x0007].reg.ebx & (1 << 27)); break;
            case Dqn_CPUFeature_AVX512CD:           available = (fn_0000_[0x0007].reg.ebx & (1 << 28)); break;
            case Dqn_CPUFeature_AVX512BW:           available = (fn_0000_[0x0007].reg.ebx & (1 << 30)); break;
            case Dqn_CPUFeature_AVX512VL:           available = (fn_0000_[0x0007].reg.ebx & (1 << 31)); break;
            case Dqn_CPUFeature_AVX512VBMI:         available = (fn_0000_[0x0007].reg.ecx & (1 <<  1)); break;
            case Dqn_CPUFeature_AVX512VBMI2:        available = (fn_0000_[0x0007].reg.ecx & (1 <<  6)); break;
            case Dqn_CPUFeature_AVX512VNNI:         available = (fn_0000_[0x0007].reg.ecx & (1 << 11)); break;
            case Dqn_CPUFeature_AVX512BITALG:       available = (fn_0000_[0x0007].reg.ecx & (1 << 12)); break;
            case Dqn_CPUFeature_AVX512VPOPCNTDQ:    available = (fn_0000_[0x0007].reg.ecx & (1 << 14)); break;
            case Dqn_CPUFeature_AVX5124VNNIW:       available = (fn_0000_[0x0007].reg.edx & (1 <<  2)); break;
            case Dqn_CPUFeature_AVX5124FMAPS:       available = (fn_0000_[0x0007].reg.edx & (1 <<  3)); break;
            case Dqn_CPUFeature_AVX512VP2INTERSECT: available = (fn_0000_[0x0007].reg.edx & (1 <<  8)); break;
            case Dqn_CPUFeature_AVX512FP16:         available = (fn_0000_[0x0007].reg.edx & (1 << 23)); break;
            case Dqn_CPUFeature_CLZERO:             available = (fn_8000_[0x0008].reg.ebx & (1 <<  0)); break;
            case Dqn_CPUFeature_CMPXCHG8B:          available = (fn_0000_[0x0001].reg.edx & (1 <<  8)); break;
            case Dqn_CPUFeature_CMPXCHG16B:         available = (fn_0000_[0x0001].reg.ecx & (1 << 13)); break;
            case Dqn_CPUFeature_F16C:               available = (fn_0000_[0x0001].reg.ecx & (1 << 29)); break;
            case Dqn_CPUFeature_FMA:                available = (fn_0000_[0x0001].reg.ecx & (1 << 12)); break;
            case Dqn_CPUFeature_FMA4:               available = (fn_8000_[0x0001].reg.ecx & (1 << 16)); break;
            case Dqn_CPUFeature_FP128:              available = (fn_8000_[0x001A].reg.eax & (1 <<  0)); break;
            case Dqn_CPUFeature_FP256:              available = (fn_8000_[0x001A].reg.eax & (1 <<  2)); break;
            case Dqn_CPUFeature_FPU:                available = (fn_0000_[0x0001].reg.edx & (1 <<  0)); break;
            case Dqn_CPUFeature_MMX:                available = (fn_0000_[0x0001].reg.edx & (1 << 23)); break;
            case Dqn_CPUFeature_MONITOR:            available = (fn_0000_[0x0001].reg.ecx & (1 <<  3)); break;
            case Dqn_CPUFeature_MOVBE:              available = (fn_0000_[0x0001].reg.ecx & (1 << 22)); break;
            case Dqn_CPUFeature_MOVU:               available = (fn_8000_[0x001A].reg.eax & (1 <<  1)); break;
            case Dqn_CPUFeature_MmxExt:             available = (fn_8000_[0x0001].reg.edx & (1 << 22)); break;
            case Dqn_CPUFeature_PCLMULQDQ:          available = (fn_0000_[0x0001].reg.ecx & (1 <<  1)); break;
            case Dqn_CPUFeature_POPCNT:             available = (fn_0000_[0x0001].reg.ecx & (1 << 23)); break;
            case Dqn_CPUFeature_RDRAND:             available = (fn_0000_[0x0001].reg.ecx & (1 << 30)); break;
            case Dqn_CPUFeature_RDSEED:             available = (fn_0000_[0x0007].reg.ebx & (1 << 18)); break;
            case Dqn_CPUFeature_RDTSCP:             available = (fn_8000_[0x0001].reg.edx & (1 << 27)); break;
            case Dqn_CPUFeature_SHA:                available = (fn_0000_[0x0007].reg.ebx & (1 << 29)); break;
            case Dqn_CPUFeature_SSE:                available = (fn_0000_[0x0001].reg.edx & (1 << 25)); break;
            case Dqn_CPUFeature_SSE2:               available = (fn_0000_[0x0001].reg.edx & (1 << 26)); break;
            case Dqn_CPUFeature_SSE3:               available = (fn_0000_[0x0001].reg.ecx & (1 <<  0)); break;
            case Dqn_CPUFeature_SSE41:              available = (fn_0000_[0x0001].reg.ecx & (1 << 19)); break;
            case Dqn_CPUFeature_SSE42:              available = (fn_0000_[0x0001].reg.ecx & (1 << 20)); break;
            case Dqn_CPUFeature_SSE4A:              available = (fn_8000_[0x0001].reg.ecx & (1 <<  6)); break;
            case Dqn_CPUFeature_SSSE3:              available = (fn_0000_[0x0001].reg.ecx & (1 <<  9)); break;
            case Dqn_CPUFeature_TSC:                available = (fn_0000_[0x0001].reg.edx & (1 <<  4)); break;
            case Dqn_CPUFeature_TscInvariant:       available = (fn_8000_[0x0007].reg.edx & (1 <<  8)); break;
            case Dqn_CPUFeature_VAES:               available = (fn_0000_[0x0007].reg.ecx & (1 <<  9)); break;
            case Dqn_CPUFeature_VPCMULQDQ:          available = (fn_0000_[0x0007].reg.ecx & (1 << 10)); break;
            case Dqn_CPUFeature_Count:              DQN_INVALID_CODE_PATH; break;
        }

        if (available)
            Dqn_CPU_SetFeature(&result, DQN_CAST(Dqn_CPUFeature)ext_index);
    }

    return result;
}
#endif // !defined(DQN_PLATFORM_ARM64) && !defined(DQN_PLATFORM_EMSCRIPTEN)

// NOTE: [$TMUT] Dqn_TicketMutex ///////////////////////////////////////////////////////////////////
DQN_API void Dqn_TicketMutex_Begin(Dqn_TicketMutex *mutex)
{
    unsigned int ticket = Dqn_Atomic_AddU32(&mutex->ticket, 1);
    Dqn_TicketMutex_BeginTicket(mutex, ticket);
}

DQN_API void Dqn_TicketMutex_End(Dqn_TicketMutex *mutex)
{
    Dqn_Atomic_AddU32(&mutex->serving, 1);
}

DQN_API Dqn_uint Dqn_TicketMutex_MakeTicket(Dqn_TicketMutex *mutex)
{
    Dqn_uint result = Dqn_Atomic_AddU32(&mutex->ticket, 1);
    return result;
}

DQN_API void Dqn_TicketMutex_BeginTicket(Dqn_TicketMutex const *mutex, Dqn_uint ticket)
{
    DQN_ASSERTF(mutex->serving <= ticket,
                "Mutex skipped ticket? Was ticket generated by the correct mutex via MakeTicket? ticket = %u, "
                "mutex->serving = %u",
                ticket,
                mutex->serving);
    while (ticket != mutex->serving) {
        // NOTE: Use spinlock intrinsic
        _mm_pause();
    }
}

DQN_API bool Dqn_TicketMutex_CanLock(Dqn_TicketMutex const *mutex, Dqn_uint ticket)
{
    bool result = (ticket == mutex->serving);
    return result;
}

#if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    #if !defined(DQN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED)
        #undef _CRT_SECURE_NO_WARNINGS
    #endif
#endif

// NOTE: [$PRIN] Dqn_Print /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_PrintStyle Dqn_Print_StyleColour(uint8_t r, uint8_t g, uint8_t b, Dqn_PrintBold bold)
{
    Dqn_PrintStyle result = {};
    result.bold           = bold;
    result.colour         = true;
    result.r              = r;
    result.g              = g;
    result.b              = b;
    return result;
}

DQN_API Dqn_PrintStyle Dqn_Print_StyleColourU32(uint32_t rgb, Dqn_PrintBold bold)
{
    uint8_t r             = (rgb >> 24) & 0xFF;
    uint8_t g             = (rgb >> 16) & 0xFF;
    uint8_t b             = (rgb >>  8) & 0xFF;
    Dqn_PrintStyle result = Dqn_Print_StyleColour(r, g, b, bold);
    return result;
}

DQN_API Dqn_PrintStyle Dqn_Print_StyleBold()
{
    Dqn_PrintStyle result = {};
    result.bold           = Dqn_PrintBold_Yes;
    return result;
}

DQN_API void Dqn_Print_Std(Dqn_PrintStd std_handle, Dqn_Str8 string)
{
    DQN_ASSERT(std_handle == Dqn_PrintStd_Out || std_handle == Dqn_PrintStd_Err);

    #if defined(DQN_OS_WIN32)
    // NOTE: Get the output handles from kernel ////////////////////////////////////////////////////
    DQN_THREAD_LOCAL void *std_out_print_handle     = nullptr;
    DQN_THREAD_LOCAL void *std_err_print_handle     = nullptr;
    DQN_THREAD_LOCAL bool  std_out_print_to_console = false;
    DQN_THREAD_LOCAL bool  std_err_print_to_console = false;

    if (!std_out_print_handle) {
        unsigned long mode = 0; (void)mode;
        std_out_print_handle     = GetStdHandle(STD_OUTPUT_HANDLE);
        std_out_print_to_console = GetConsoleMode(std_out_print_handle, &mode) != 0;

        std_err_print_handle     = GetStdHandle(STD_ERROR_HANDLE);
        std_err_print_to_console = GetConsoleMode(std_err_print_handle, &mode) != 0;
    }

    // NOTE: Select the output handle //////////////////////////////////////////////////////////////
    void *print_handle    = std_out_print_handle;
    bool print_to_console = std_out_print_to_console;
    if (std_handle == Dqn_PrintStd_Err) {
        print_handle     = std_err_print_handle;
        print_to_console = std_err_print_to_console;
    }

    // NOTE: Write the string //////////////////////////////////////////////////////////////////////
    DQN_ASSERT(string.size < DQN_CAST(unsigned long)-1);
    unsigned long bytes_written = 0; (void)bytes_written;
    if (print_to_console) {
        WriteConsoleA(print_handle, string.data, DQN_CAST(unsigned long)string.size, &bytes_written, nullptr);
    } else {
        WriteFile(print_handle, string.data, DQN_CAST(unsigned long)string.size, &bytes_written, nullptr);
    }
    #else
    fprintf(std_handle == Dqn_PrintStd_Out ? stdout : stderr, "%.*s", DQN_STR_FMT(string));
    #endif
}

DQN_API void Dqn_Print_StdStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, Dqn_Str8 string)
{
    if (string.data && string.size) {
        if (style.colour)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCColourFgStr8(style.r, style.g, style.b));
        if (style.bold == Dqn_PrintBold_Yes)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCBoldStr8);
        Dqn_Print_Std(std_handle, string);
        if (style.colour || style.bold == Dqn_PrintBold_Yes)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCResetStr8);
    }
}

DQN_API void Dqn_Print_StdF(Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Print_StdFV(std_handle, fmt, args);
    va_end(args);
}

DQN_API void Dqn_Print_StdFStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Print_StdFVStyle(std_handle, style, fmt, args);
    va_end(args);
}

#if !defined(DQN_USE_STD_PRINTF)
DQN_FILE_SCOPE char *Dqn_Print_VSPrintfChunker_(const char *buf, void *user, int len)
{
    Dqn_Str8 string = {};
    string.data        = DQN_CAST(char *)buf;
    string.size        = len;

    Dqn_PrintStd std_handle = DQN_CAST(Dqn_PrintStd)DQN_CAST(uintptr_t)user;
    Dqn_Print_Std(std_handle, string);
    return (char *)buf;
}
#endif

DQN_API void Dqn_Print_StdFV(Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    #if defined(DQN_USE_STD_PRINTF)
    vfprintf(std_handle == Dqn_PrintStd_Out ? stdout : stderr, fmt, args);
    #else
    char buffer[STB_SPRINTF_MIN];
    STB_SPRINTF_DECORATE(vsprintfcb)(Dqn_Print_VSPrintfChunker_, DQN_CAST(void *)DQN_CAST(uintptr_t)std_handle, buffer, fmt, args);
    #endif
}

DQN_API void Dqn_Print_StdFVStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (fmt) {
        if (style.colour)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCColourFgStr8(style.r, style.g, style.b));
        if (style.bold == Dqn_PrintBold_Yes)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCBoldStr8);
        Dqn_Print_StdFV(std_handle, fmt, args);
        if (style.colour || style.bold == Dqn_PrintBold_Yes)
            Dqn_Print_Std(std_handle, Dqn_Print_ESCResetStr8);
    }
}

DQN_API void Dqn_Print_StdLn(Dqn_PrintStd std_handle, Dqn_Str8 string)
{
    Dqn_Print_Std(std_handle, string);
    Dqn_Print_Std(std_handle, DQN_STR8("\n"));
}

DQN_API void Dqn_Print_StdLnF(Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Print_StdLnFV(std_handle, fmt, args);
    va_end(args);
}

DQN_API void Dqn_Print_StdLnFV(Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Print_StdFV(std_handle, fmt, args);
    Dqn_Print_Std(std_handle, DQN_STR8("\n"));
}

DQN_API void Dqn_Print_StdLnStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, Dqn_Str8 string)
{
    Dqn_Print_StdStyle(std_handle, style, string);
    Dqn_Print_Std(std_handle, DQN_STR8("\n"));
}

DQN_API void Dqn_Print_StdLnFStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Print_StdLnFVStyle(std_handle, style, fmt, args);
    va_end(args);
}

DQN_API void Dqn_Print_StdLnFVStyle(Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Print_StdFVStyle(std_handle, style, fmt, args);
    Dqn_Print_Std(std_handle, DQN_STR8("\n"));
}

DQN_API Dqn_Str8 Dqn_Print_ESCColourStr8(Dqn_PrintESCColour colour, uint8_t r, uint8_t g, uint8_t b)
{
    DQN_THREAD_LOCAL char buffer[32];
    buffer[0]       = 0;
    Dqn_Str8 result = {};
    result.size     = DQN_SNPRINTF(buffer,
                                   DQN_ARRAY_UCOUNT(buffer),
                                   "\x1b[%d;2;%u;%u;%um",
                                   colour == Dqn_PrintESCColour_Fg ? 38 : 48,
                                   r, g, b);
    result.data     = buffer;
    return result;
}

DQN_API Dqn_Str8 Dqn_Print_ESCColourU32Str8(Dqn_PrintESCColour colour, uint32_t value)
{
    uint8_t r       = DQN_CAST(uint8_t)(value >> 24);
    uint8_t g       = DQN_CAST(uint8_t)(value >> 16);
    uint8_t b       = DQN_CAST(uint8_t)(value >>  8);
    Dqn_Str8 result = Dqn_Print_ESCColourStr8(colour, r, g, b);
    return result;
}

// NOTE: [$LLOG] Dqn_Log ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8 Dqn_Log_MakeStr8(Dqn_Arena                 *arena,
                                  bool                       colour,
                                  Dqn_Str8                   type,
                                  int                        log_type,
                                  Dqn_CallSite               call_site,
                                  DQN_FMT_ATTRIB char const *fmt,
                                  va_list                    args)
{
    DQN_LOCAL_PERSIST Dqn_usize max_type_length = 0;
    max_type_length                             = DQN_MAX(max_type_length, type.size);
    int type_padding                            = DQN_CAST(int)(max_type_length - type.size);

    Dqn_Str8 colour_esc = {};
    Dqn_Str8 bold_esc   = {};
    Dqn_Str8 reset_esc  = {};
    if (colour) {
        bold_esc  = Dqn_Print_ESCBoldStr8;
        reset_esc = Dqn_Print_ESCResetStr8;
        switch (log_type) {
            case Dqn_LogType_Debug:                                                                            break;
            case Dqn_LogType_Info:    colour_esc = Dqn_Print_ESCColourFgU32Str8(Dqn_LogTypeColourU32_Info);    break;
            case Dqn_LogType_Warning: colour_esc = Dqn_Print_ESCColourFgU32Str8(Dqn_LogTypeColourU32_Warning); break;
            case Dqn_LogType_Error:   colour_esc = Dqn_Print_ESCColourFgU32Str8(Dqn_LogTypeColourU32_Error);   break;
        }
    }

    Dqn_Str8 file_name            = Dqn_Str8_FileNameFromPath(call_site.file);
    Dqn_OSDateTimeStr8 const time = Dqn_OS_DateLocalTimeStr8Now();
    Dqn_Str8 header               = Dqn_Str8_InitF(arena,
                                                   "%.*s "     // date
                                                   "%.*s "     // hms
                                                   "%.*s"      // colour
                                                   "%.*s"      // bold
                                                   "%.*s"      // type
                                                   "%*s"       // type padding
                                                   "%.*s"      // reset
                                                   " %.*s"     // file name
                                                   ":%05I32u " // line number
                                                   ,
                                                   DQN_CAST(int)time.date_size - 2, time.date + 2, // date
                                                   DQN_CAST(int)time.hms_size,      time.hms,      // hms
                                                   DQN_STR_FMT(colour_esc),                        // colour
                                                   DQN_STR_FMT(bold_esc),                          // bold
                                                   DQN_STR_FMT(type),                              // type
                                                   DQN_CAST(int)type_padding,  "",                 // type padding
                                                   DQN_STR_FMT(reset_esc),                         // reset
                                                   DQN_STR_FMT(file_name),                         // file name
                                                   call_site.line);                                // line number
    Dqn_usize header_size_no_ansi_codes = header.size - colour_esc.size - Dqn_Print_ESCResetStr8.size;

    // NOTE: Header padding ////////////////////////////////////////////////////////////////////////
    DQN_LOCAL_PERSIST Dqn_usize max_header_length = 0;
    max_header_length                             = DQN_MAX(max_header_length, header_size_no_ansi_codes);
    Dqn_usize header_padding                      = max_header_length - header_size_no_ansi_codes;

    // NOTE: Construct final log ///////////////////////////////////////////////////////////////////
    Dqn_Str8 user_msg = Dqn_Str8_InitFV(arena, fmt, args);
    Dqn_Str8 result   = Dqn_Str8_Alloc(arena, header.size + header_padding + user_msg.size, Dqn_ZeroMem_No);
    if (Dqn_Str8_HasData(result)) {
        DQN_MEMCPY(result.data,               header.data, header.size);
        DQN_MEMSET(result.data + header.size, ' ',         header_padding);
        if (Dqn_Str8_HasData(user_msg))
            DQN_MEMCPY(result.data + header.size + header_padding, user_msg.data, user_msg.size);
    }
    return result;
}

DQN_FILE_SCOPE void Dqn_Log_FVDefault_(Dqn_Str8 type, int log_type, void *user_data, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Library *lib = g_dqn_library;
    (void)log_type;
    (void)user_data;

    // NOTE: Open log file for appending if requested //////////////////////////
    Dqn_TicketMutex_Begin(&lib->log_file_mutex);
    if (lib->log_to_file && !lib->log_file.handle && !lib->log_file.error) {
        Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
        Dqn_Str8 log_path = Dqn_OS_PathF(t_mem.arena, "%.*s/dqn.log", DQN_STR_FMT(lib->exe_dir));
        lib->log_file     = Dqn_OS_FileOpen(log_path, Dqn_OSFileOpen_CreateAlways, Dqn_OSFileAccess_AppendOnly, nullptr);
    }
    Dqn_TicketMutex_End(&lib->log_file_mutex);

    // NOTE: Generate the log header ///////////////////////////////////////////
    Dqn_TLSTMem t_mem  = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    log_line = Dqn_Log_MakeStr8(t_mem.arena, !lib->log_no_colour, type, log_type, call_site, fmt, args);

    // NOTE: Print log /////////////////////////////////////////////////////////
    Dqn_Print_StdLn(Dqn_PrintStd_Out, log_line);

    Dqn_TicketMutex_Begin(&lib->log_file_mutex);
    Dqn_OS_FileWrite(&lib->log_file, log_line, nullptr);
    Dqn_OS_FileWrite(&lib->log_file, DQN_STR8("\n"), nullptr);
    Dqn_TicketMutex_End(&lib->log_file_mutex);
}

DQN_API void Dqn_Log_FVCallSite(Dqn_Str8 type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (g_dqn_library) {
        Dqn_LogProc *logging_function = g_dqn_library->log_callback ? g_dqn_library->log_callback : Dqn_Log_FVDefault_;
        logging_function(type, -1 /*log_type*/, g_dqn_library->log_user_data, call_site, fmt, args);
    } else {
        // NOTE: Rarely taken branch, only when trying to use this library without initialising it
        Dqn_Print_StdLnFV(Dqn_PrintStd_Out, fmt, args);
    }
}

DQN_API void Dqn_Log_FCallSite(Dqn_Str8 type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const  *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Log_FVCallSite(type, call_site, fmt, args);
    va_end(args);
}

DQN_API void Dqn_Log_TypeFVCallSite(Dqn_LogType type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Str8 type_string = DQN_STR8("DQN-BAD-LOG-TYPE");
    switch (type) {
        case Dqn_LogType_Error:   type_string = DQN_STR8("ERROR"); break;
        case Dqn_LogType_Info:    type_string = DQN_STR8("INFO"); break;
        case Dqn_LogType_Warning: type_string = DQN_STR8("WARN"); break;
        case Dqn_LogType_Debug:   type_string = DQN_STR8("DEBUG"); break;
        case Dqn_LogType_Count:   type_string = DQN_STR8("BADXX"); break;
    }
    Dqn_Log_FVCallSite(type_string, call_site, fmt, args);
}

DQN_API void Dqn_Log_TypeFCallSite(Dqn_LogType type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Log_TypeFVCallSite(type, call_site, fmt, args);
    va_end(args);
}

// NOTE: [$ERRS] Dqn_ErrorSink /////////////////////////////////////////////////////////////////////
DQN_API Dqn_ErrorSink *Dqn_ErrorSink_Begin(Dqn_ErrorSinkMode mode)
{
    Dqn_TLS           *tls       = Dqn_TLS_Get();
    Dqn_ErrorSink     *result    = &tls->error_sink;
    Dqn_usize          arena_pos = Dqn_Arena_Pos(result->arena);
    Dqn_ErrorSinkNode *node      = Dqn_Arena_New(result->arena, Dqn_ErrorSinkNode, Dqn_ZeroMem_Yes);
    node->next                   = result->stack;
    node->arena_pos              = arena_pos;
    node->mode                   = mode;
    result->stack                = node;
    return result;
}

DQN_API bool Dqn_ErrorSink_HasError(Dqn_ErrorSink *error)
{
    bool result = error && error->stack->error;
    return result;
}

DQN_API Dqn_ErrorSinkNode Dqn_ErrorSink_End(Dqn_Arena *arena, Dqn_ErrorSink *error)
{
    Dqn_ErrorSinkNode  result = {};
    if (!error)
        return result;

    Dqn_ErrorSinkNode *node = error->stack;
    if (!node)
        return result;

    DQN_ASSERTF(arena != error->arena,
                "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");
    result       = *error->stack;
    result.next  = nullptr;
    result.msg   = Dqn_Str8_Copy(arena, result.msg);
    error->stack = error->stack->next;
    Dqn_Arena_PopTo(error->arena, result.arena_pos);
    return result;
}

DQN_API void Dqn_ErrorSink_EndAndIgnore(Dqn_ErrorSink *error)
{
    Dqn_ErrorSink_End(nullptr, error);
}

DQN_API bool Dqn_ErrorSink_EndAndLogError(Dqn_ErrorSink *error, Dqn_Str8 error_msg)
{
    Dqn_TLSTMem       t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_ErrorSinkNode node    = Dqn_ErrorSink_End(t_mem.arena, error);
    if (node.error) {
        if (Dqn_Str8_HasData(error_msg)) {
            Dqn_Log_TypeFCallSite(Dqn_LogType_Error, node.call_site, "%.*s: %.*s", DQN_STR_FMT(error_msg), DQN_STR_FMT(node.msg));
        } else {
            Dqn_Log_TypeFCallSite(Dqn_LogType_Error, node.call_site, "%.*s", DQN_STR_FMT(node.msg));
        }
    }
    bool result = node.error;
    return result;
}

DQN_API bool Dqn_ErrorSink_EndAndLogErrorFV(Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    log     = Dqn_Str8_InitFV(t_mem.arena, fmt, args);
    bool        result  = Dqn_ErrorSink_EndAndLogError(error, log);
    return result;
}

DQN_API bool Dqn_ErrorSink_EndAndLogErrorF(Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    log     = Dqn_Str8_InitFV(t_mem.arena, fmt, args);
    bool        result  = Dqn_ErrorSink_EndAndLogError(error, log);
    va_end(args);
    return result;
}

DQN_API void Dqn_ErrorSink_EndAndExitIfErrorFV(Dqn_ErrorSink *error, uint32_t exit_code, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (Dqn_ErrorSink_EndAndLogErrorFV(error, fmt, args)) {
        DQN_DEBUG_BREAK;
        Dqn_OS_Exit(exit_code);
    }
}

DQN_API void Dqn_ErrorSink_EndAndExitIfErrorF(Dqn_ErrorSink *error, uint32_t exit_code, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_ErrorSink_EndAndExitIfErrorFV(error, exit_code, fmt, args);
    va_end(args);
}

DQN_API void Dqn_ErrorSink_MakeFV_(Dqn_ErrorSink *error, uint32_t error_code, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (!error)
        return;

    Dqn_ErrorSinkNode *node = error->stack;
    DQN_ASSERTF(node, "Error sink must be begun by calling 'Begin' before using this function.");
    if (!node->error) { // NOTE: Only preserve the first error
        node->msg        = Dqn_Str8_InitFV(error->arena, fmt, args);
        node->error_code = error_code;
        node->error      = true;
        node->call_site  = Dqn_TLS_Get()->call_site;
        if (node->mode == Dqn_ErrorSinkMode_ExitOnError)
            Dqn_ErrorSink_EndAndExitIfErrorF(error, error_code, "Fatal error %u", error_code);
    }
}

DQN_API void Dqn_ErrorSink_MakeF_(Dqn_ErrorSink *error, uint32_t error_code, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_ErrorSink_MakeFV_(error, error_code, fmt, args);
    va_end(args);
}

