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
//  dn_base.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
#if !defined(DN_PLATFORM_ARM64) && !defined(DN_PLATFORM_EMSCRIPTEN)
#if defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
#include <cpuid.h>
#endif

DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];

DN_API DN_CPUIDResult DN_CPU_ID(DN_CPUIDArgs args)
{
    DN_CPUIDResult result = {};
    __cpuidex(result.values, args.eax, args.ecx);
    return result;
}

DN_API DN_USize DN_CPU_HasFeatureArray(DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size)
{
    DN_USize       result = 0;
    DN_USize const BITS   = sizeof(report->features[0]) * 8;
    DN_FOR_UINDEX(feature_index, features_size) {
        DN_CPUFeatureQuery *query       = features + feature_index;
        DN_USize            chunk_index = query->feature / BITS;
        DN_USize            chunk_bit   = query->feature % BITS;
        uint64_t             chunk       = report->features[chunk_index];
        query->available                 = chunk & (1ULL << chunk_bit);
        result                          += DN_CAST(int)query->available;
    }

    return result;
}

DN_API bool DN_CPU_HasFeature(DN_CPUReport const *report, DN_CPUFeature feature)
{
    DN_CPUFeatureQuery query = {};
    query.feature             = feature;
    bool result               = DN_CPU_HasFeatureArray(report, &query, 1) == 1;
    return result;
}

DN_API bool DN_CPU_HasAllFeatures(DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size)
{
    bool result = true;
    for (DN_USize index = 0; result && index < features_size; index++)
        result &= DN_CPU_HasFeature(report, features[index]);
    return result;
}

DN_API void DN_CPU_SetFeature(DN_CPUReport *report, DN_CPUFeature feature)
{
    DN_ASSERT(feature < DN_CPUFeature_Count);
    DN_USize const BITS        = sizeof(report->features[0]) * 8;
    DN_USize       chunk_index = feature / BITS;
    DN_USize       chunk_bit   = feature % BITS;
    report->features[chunk_index] |= (1ULL << chunk_bit);
}

DN_API DN_CPUReport DN_CPU_Report()
{
    DN_CPUReport   result                 = {};
    DN_CPUIDResult fn_0000_[500]          = {};
    DN_CPUIDResult fn_8000_[500]          = {};
    int const       EXTENDED_FUNC_BASE_EAX = 0x8000'0000;
    int const       REGISTER_SIZE          = sizeof(fn_0000_[0].reg.eax);

    // NOTE: Query standard/extended numbers ///////////////////////////////////////////////////////
    {
        DN_CPUIDArgs args = {};

        // NOTE: Query standard function (e.g. eax = 0x0)         for function count + cpu vendor
        args        = {};
        fn_0000_[0] = DN_CPU_ID(args);

        // NOTE: Query extended function (e.g. eax = 0x8000'0000) for function count + cpu vendor
        args        = {};
        args.eax    = DN_CAST(int) EXTENDED_FUNC_BASE_EAX;
        fn_8000_[0] = DN_CPU_ID(args);
    }

    // NOTE: Extract function count ////////////////////////////////////////////////////////////////
    int const STANDARD_FUNC_MAX_EAX = fn_0000_[0x0000].reg.eax;
    int const EXTENDED_FUNC_MAX_EAX = fn_8000_[0x0000].reg.eax;

    // NOTE: Enumerate all CPUID results for the known function counts /////////////////////////////
    {
        DN_ASSERTF((STANDARD_FUNC_MAX_EAX + 1) <= DN_ARRAY_ICOUNT(fn_0000_),
                    "Max standard count is %d", STANDARD_FUNC_MAX_EAX + 1);
        DN_ASSERTF((DN_CAST(DN_ISize)EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1) <= DN_ARRAY_ICOUNT(fn_8000_),
                    "Max extended count is %zu", DN_CAST(DN_ISize)EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1);

        for (int eax = 1; eax <= STANDARD_FUNC_MAX_EAX; eax++) {
            DN_CPUIDArgs args = {};
            args.eax           = eax;
            fn_0000_[eax]      = DN_CPU_ID(args);
        }

        for (int eax = EXTENDED_FUNC_BASE_EAX + 1, index = 1; eax <= EXTENDED_FUNC_MAX_EAX; eax++, index++) {
            DN_CPUIDArgs args = {};
            args.eax           = eax;
            fn_8000_[index]    = DN_CPU_ID(args);
        }
    }

    // NOTE: Query CPU vendor //////////////////////////////////////////////////////////////////////
    {
        DN_MEMCPY(result.vendor + 0, &fn_8000_[0x0000].reg.ebx, REGISTER_SIZE);
        DN_MEMCPY(result.vendor + 4, &fn_8000_[0x0000].reg.edx, REGISTER_SIZE);
        DN_MEMCPY(result.vendor + 8, &fn_8000_[0x0000].reg.ecx, REGISTER_SIZE);
    }

    // NOTE: Query CPU brand ///////////////////////////////////////////////////////////////////////
    if (EXTENDED_FUNC_MAX_EAX >= (EXTENDED_FUNC_BASE_EAX + 4)) {
        DN_MEMCPY(result.brand + 0,  &fn_8000_[0x0002].reg.eax, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 4,  &fn_8000_[0x0002].reg.ebx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 8,  &fn_8000_[0x0002].reg.ecx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 12, &fn_8000_[0x0002].reg.edx, REGISTER_SIZE);

        DN_MEMCPY(result.brand + 16, &fn_8000_[0x0003].reg.eax, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 20, &fn_8000_[0x0003].reg.ebx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 24, &fn_8000_[0x0003].reg.ecx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 28, &fn_8000_[0x0003].reg.edx, REGISTER_SIZE);

        DN_MEMCPY(result.brand + 32, &fn_8000_[0x0004].reg.eax, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 36, &fn_8000_[0x0004].reg.ebx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 40, &fn_8000_[0x0004].reg.ecx, REGISTER_SIZE);
        DN_MEMCPY(result.brand + 44, &fn_8000_[0x0004].reg.edx, REGISTER_SIZE);

        DN_ASSERT(result.brand[sizeof(result.brand) - 1] == 0);
    }

    // NOTE: Query CPU features //////////////////////////////////////////////////////////////////
    for (DN_USize ext_index = 0; ext_index < DN_CPUFeature_Count; ext_index++) {
        bool available = false;

        // NOTE: Mask bits taken from various manuals
        //   - AMD64 Architecture Programmer's Manual, Volumes 1-5
        //   - https://en.wikipedia.org/wiki/CPUID#Calling_CPUID
        switch (DN_CAST(DN_CPUFeature)ext_index) {
            case DN_CPUFeature_3DNow:              available = (fn_8000_[0x0001].reg.edx & (1 << 31)); break;
            case DN_CPUFeature_3DNowExt:           available = (fn_8000_[0x0001].reg.edx & (1 << 30)); break;
            case DN_CPUFeature_ABM:                available = (fn_8000_[0x0001].reg.ecx & (1 <<  5)); break;
            case DN_CPUFeature_AES:                available = (fn_0000_[0x0001].reg.ecx & (1 << 25)); break;
            case DN_CPUFeature_AVX:                available = (fn_0000_[0x0001].reg.ecx & (1 << 28)); break;
            case DN_CPUFeature_AVX2:               available = (fn_0000_[0x0007].reg.ebx & (1 <<  0)); break;
            case DN_CPUFeature_AVX512F:            available = (fn_0000_[0x0007].reg.ebx & (1 << 16)); break;
            case DN_CPUFeature_AVX512DQ:           available = (fn_0000_[0x0007].reg.ebx & (1 << 17)); break;
            case DN_CPUFeature_AVX512IFMA:         available = (fn_0000_[0x0007].reg.ebx & (1 << 21)); break;
            case DN_CPUFeature_AVX512PF:           available = (fn_0000_[0x0007].reg.ebx & (1 << 26)); break;
            case DN_CPUFeature_AVX512ER:           available = (fn_0000_[0x0007].reg.ebx & (1 << 27)); break;
            case DN_CPUFeature_AVX512CD:           available = (fn_0000_[0x0007].reg.ebx & (1 << 28)); break;
            case DN_CPUFeature_AVX512BW:           available = (fn_0000_[0x0007].reg.ebx & (1 << 30)); break;
            case DN_CPUFeature_AVX512VL:           available = (fn_0000_[0x0007].reg.ebx & (1 << 31)); break;
            case DN_CPUFeature_AVX512VBMI:         available = (fn_0000_[0x0007].reg.ecx & (1 <<  1)); break;
            case DN_CPUFeature_AVX512VBMI2:        available = (fn_0000_[0x0007].reg.ecx & (1 <<  6)); break;
            case DN_CPUFeature_AVX512VNNI:         available = (fn_0000_[0x0007].reg.ecx & (1 << 11)); break;
            case DN_CPUFeature_AVX512BITALG:       available = (fn_0000_[0x0007].reg.ecx & (1 << 12)); break;
            case DN_CPUFeature_AVX512VPOPCNTDQ:    available = (fn_0000_[0x0007].reg.ecx & (1 << 14)); break;
            case DN_CPUFeature_AVX5124VNNIW:       available = (fn_0000_[0x0007].reg.edx & (1 <<  2)); break;
            case DN_CPUFeature_AVX5124FMAPS:       available = (fn_0000_[0x0007].reg.edx & (1 <<  3)); break;
            case DN_CPUFeature_AVX512VP2INTERSECT: available = (fn_0000_[0x0007].reg.edx & (1 <<  8)); break;
            case DN_CPUFeature_AVX512FP16:         available = (fn_0000_[0x0007].reg.edx & (1 << 23)); break;
            case DN_CPUFeature_CLZERO:             available = (fn_8000_[0x0008].reg.ebx & (1 <<  0)); break;
            case DN_CPUFeature_CMPXCHG8B:          available = (fn_0000_[0x0001].reg.edx & (1 <<  8)); break;
            case DN_CPUFeature_CMPXCHG16B:         available = (fn_0000_[0x0001].reg.ecx & (1 << 13)); break;
            case DN_CPUFeature_F16C:               available = (fn_0000_[0x0001].reg.ecx & (1 << 29)); break;
            case DN_CPUFeature_FMA:                available = (fn_0000_[0x0001].reg.ecx & (1 << 12)); break;
            case DN_CPUFeature_FMA4:               available = (fn_8000_[0x0001].reg.ecx & (1 << 16)); break;
            case DN_CPUFeature_FP128:              available = (fn_8000_[0x001A].reg.eax & (1 <<  0)); break;
            case DN_CPUFeature_FP256:              available = (fn_8000_[0x001A].reg.eax & (1 <<  2)); break;
            case DN_CPUFeature_FPU:                available = (fn_0000_[0x0001].reg.edx & (1 <<  0)); break;
            case DN_CPUFeature_MMX:                available = (fn_0000_[0x0001].reg.edx & (1 << 23)); break;
            case DN_CPUFeature_MONITOR:            available = (fn_0000_[0x0001].reg.ecx & (1 <<  3)); break;
            case DN_CPUFeature_MOVBE:              available = (fn_0000_[0x0001].reg.ecx & (1 << 22)); break;
            case DN_CPUFeature_MOVU:               available = (fn_8000_[0x001A].reg.eax & (1 <<  1)); break;
            case DN_CPUFeature_MmxExt:             available = (fn_8000_[0x0001].reg.edx & (1 << 22)); break;
            case DN_CPUFeature_PCLMULQDQ:          available = (fn_0000_[0x0001].reg.ecx & (1 <<  1)); break;
            case DN_CPUFeature_POPCNT:             available = (fn_0000_[0x0001].reg.ecx & (1 << 23)); break;
            case DN_CPUFeature_RDRAND:             available = (fn_0000_[0x0001].reg.ecx & (1 << 30)); break;
            case DN_CPUFeature_RDSEED:             available = (fn_0000_[0x0007].reg.ebx & (1 << 18)); break;
            case DN_CPUFeature_RDTSCP:             available = (fn_8000_[0x0001].reg.edx & (1 << 27)); break;
            case DN_CPUFeature_SHA:                available = (fn_0000_[0x0007].reg.ebx & (1 << 29)); break;
            case DN_CPUFeature_SSE:                available = (fn_0000_[0x0001].reg.edx & (1 << 25)); break;
            case DN_CPUFeature_SSE2:               available = (fn_0000_[0x0001].reg.edx & (1 << 26)); break;
            case DN_CPUFeature_SSE3:               available = (fn_0000_[0x0001].reg.ecx & (1 <<  0)); break;
            case DN_CPUFeature_SSE41:              available = (fn_0000_[0x0001].reg.ecx & (1 << 19)); break;
            case DN_CPUFeature_SSE42:              available = (fn_0000_[0x0001].reg.ecx & (1 << 20)); break;
            case DN_CPUFeature_SSE4A:              available = (fn_8000_[0x0001].reg.ecx & (1 <<  6)); break;
            case DN_CPUFeature_SSSE3:              available = (fn_0000_[0x0001].reg.ecx & (1 <<  9)); break;
            case DN_CPUFeature_TSC:                available = (fn_0000_[0x0001].reg.edx & (1 <<  4)); break;
            case DN_CPUFeature_TscInvariant:       available = (fn_8000_[0x0007].reg.edx & (1 <<  8)); break;
            case DN_CPUFeature_VAES:               available = (fn_0000_[0x0007].reg.ecx & (1 <<  9)); break;
            case DN_CPUFeature_VPCMULQDQ:          available = (fn_0000_[0x0007].reg.ecx & (1 << 10)); break;
            case DN_CPUFeature_Count:              DN_INVALID_CODE_PATH; break;
        }

        if (available)
            DN_CPU_SetFeature(&result, DN_CAST(DN_CPUFeature)ext_index);
    }

    return result;
}
#endif // !defined(DN_PLATFORM_ARM64) && !defined(DN_PLATFORM_EMSCRIPTEN)

// NOTE: [$TMUT] DN_TicketMutex ///////////////////////////////////////////////////////////////////
DN_API void DN_TicketMutex_Begin(DN_TicketMutex *mutex)
{
    unsigned int ticket = DN_Atomic_AddU32(&mutex->ticket, 1);
    DN_TicketMutex_BeginTicket(mutex, ticket);
}

DN_API void DN_TicketMutex_End(DN_TicketMutex *mutex)
{
    DN_Atomic_AddU32(&mutex->serving, 1);
}

DN_API DN_UInt DN_TicketMutex_MakeTicket(DN_TicketMutex *mutex)
{
    DN_UInt result = DN_Atomic_AddU32(&mutex->ticket, 1);
    return result;
}

DN_API void DN_TicketMutex_BeginTicket(DN_TicketMutex const *mutex, DN_UInt ticket)
{
    DN_ASSERTF(mutex->serving <= ticket,
                "Mutex skipped ticket? Was ticket generated by the correct mutex via MakeTicket? ticket = %u, "
                "mutex->serving = %u",
                ticket,
                mutex->serving);
    while (ticket != mutex->serving) {
        // NOTE: Use spinlock intrinsic
        _mm_pause();
    }
}

DN_API bool DN_TicketMutex_CanLock(DN_TicketMutex const *mutex, DN_UInt ticket)
{
    bool result = (ticket == mutex->serving);
    return result;
}

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #if !defined(DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED)
        #undef _CRT_SECURE_NO_WARNINGS
    #endif
#endif

// NOTE: [$PRIN] DN_Print /////////////////////////////////////////////////////////////////////////
DN_API DN_PrintStyle DN_Print_StyleColour(uint8_t r, uint8_t g, uint8_t b, DN_PrintBold bold)
{
    DN_PrintStyle result = {};
    result.bold           = bold;
    result.colour         = true;
    result.r              = r;
    result.g              = g;
    result.b              = b;
    return result;
}

DN_API DN_PrintStyle DN_Print_StyleColourU32(uint32_t rgb, DN_PrintBold bold)
{
    uint8_t r             = (rgb >> 24) & 0xFF;
    uint8_t g             = (rgb >> 16) & 0xFF;
    uint8_t b             = (rgb >>  8) & 0xFF;
    DN_PrintStyle result = DN_Print_StyleColour(r, g, b, bold);
    return result;
}

DN_API DN_PrintStyle DN_Print_StyleBold()
{
    DN_PrintStyle result = {};
    result.bold           = DN_PrintBold_Yes;
    return result;
}

DN_API void DN_Print_Std(DN_PrintStd std_handle, DN_Str8 string)
{
    DN_ASSERT(std_handle == DN_PrintStd_Out || std_handle == DN_PrintStd_Err);

    #if defined(DN_OS_WIN32)
    // NOTE: Get the output handles from kernel ////////////////////////////////////////////////////
    DN_THREAD_LOCAL void *std_out_print_handle     = nullptr;
    DN_THREAD_LOCAL void *std_err_print_handle     = nullptr;
    DN_THREAD_LOCAL bool  std_out_print_to_console = false;
    DN_THREAD_LOCAL bool  std_err_print_to_console = false;

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
    if (std_handle == DN_PrintStd_Err) {
        print_handle     = std_err_print_handle;
        print_to_console = std_err_print_to_console;
    }

    // NOTE: Write the string //////////////////////////////////////////////////////////////////////
    DN_ASSERT(string.size < DN_CAST(unsigned long)-1);
    unsigned long bytes_written = 0; (void)bytes_written;
    if (print_to_console) {
        WriteConsoleA(print_handle, string.data, DN_CAST(unsigned long)string.size, &bytes_written, nullptr);
    } else {
        WriteFile(print_handle, string.data, DN_CAST(unsigned long)string.size, &bytes_written, nullptr);
    }
    #else
    fprintf(std_handle == DN_PrintStd_Out ? stdout : stderr, "%.*s", DN_STR_FMT(string));
    #endif
}

DN_API void DN_Print_StdStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_Str8 string)
{
    if (string.data && string.size) {
        if (style.colour)
            DN_Print_Std(std_handle, DN_Print_ESCColourFgStr8(style.r, style.g, style.b));
        if (style.bold == DN_PrintBold_Yes)
            DN_Print_Std(std_handle, DN_Print_ESCBoldStr8);
        DN_Print_Std(std_handle, string);
        if (style.colour || style.bold == DN_PrintBold_Yes)
            DN_Print_Std(std_handle, DN_Print_ESCResetStr8);
    }
}

DN_API void DN_Print_StdF(DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Print_StdFV(std_handle, fmt, args);
    va_end(args);
}

DN_API void DN_Print_StdFStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Print_StdFVStyle(std_handle, style, fmt, args);
    va_end(args);
}

#if !defined(DN_USE_STD_PRINTF)
DN_FILE_SCOPE char *DN_Print_VSPrintfChunker_(const char *buf, void *user, int len)
{
    DN_Str8 string = {};
    string.data        = DN_CAST(char *)buf;
    string.size        = len;

    DN_PrintStd std_handle = DN_CAST(DN_PrintStd)DN_CAST(uintptr_t)user;
    DN_Print_Std(std_handle, string);
    return (char *)buf;
}
#endif

DN_API void DN_Print_StdFV(DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    #if defined(DN_USE_STD_PRINTF)
    vfprintf(std_handle == DN_PrintStd_Out ? stdout : stderr, fmt, args);
    #else
    char buffer[STB_SPRINTF_MIN];
    STB_SPRINTF_DECORATE(vsprintfcb)(DN_Print_VSPrintfChunker_, DN_CAST(void *)DN_CAST(uintptr_t)std_handle, buffer, fmt, args);
    #endif
}

DN_API void DN_Print_StdFVStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (fmt) {
        if (style.colour)
            DN_Print_Std(std_handle, DN_Print_ESCColourFgStr8(style.r, style.g, style.b));
        if (style.bold == DN_PrintBold_Yes)
            DN_Print_Std(std_handle, DN_Print_ESCBoldStr8);
        DN_Print_StdFV(std_handle, fmt, args);
        if (style.colour || style.bold == DN_PrintBold_Yes)
            DN_Print_Std(std_handle, DN_Print_ESCResetStr8);
    }
}

DN_API void DN_Print_StdLn(DN_PrintStd std_handle, DN_Str8 string)
{
    DN_Print_Std(std_handle, string);
    DN_Print_Std(std_handle, DN_STR8("\n"));
}

DN_API void DN_Print_StdLnF(DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Print_StdLnFV(std_handle, fmt, args);
    va_end(args);
}

DN_API void DN_Print_StdLnFV(DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_Print_StdFV(std_handle, fmt, args);
    DN_Print_Std(std_handle, DN_STR8("\n"));
}

DN_API void DN_Print_StdLnStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_Str8 string)
{
    DN_Print_StdStyle(std_handle, style, string);
    DN_Print_Std(std_handle, DN_STR8("\n"));
}

DN_API void DN_Print_StdLnFStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Print_StdLnFVStyle(std_handle, style, fmt, args);
    va_end(args);
}

DN_API void DN_Print_StdLnFVStyle(DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_Print_StdFVStyle(std_handle, style, fmt, args);
    DN_Print_Std(std_handle, DN_STR8("\n"));
}

DN_API DN_Str8 DN_Print_ESCColourStr8(DN_PrintESCColour colour, uint8_t r, uint8_t g, uint8_t b)
{
    DN_THREAD_LOCAL char buffer[32];
    buffer[0]       = 0;
    DN_Str8 result = {};
    result.size     = DN_SNPRINTF(buffer,
                                   DN_ARRAY_UCOUNT(buffer),
                                   "\x1b[%d;2;%u;%u;%um",
                                   colour == DN_PrintESCColour_Fg ? 38 : 48,
                                   r, g, b);
    result.data     = buffer;
    return result;
}

DN_API DN_Str8 DN_Print_ESCColourU32Str8(DN_PrintESCColour colour, uint32_t value)
{
    uint8_t r       = DN_CAST(uint8_t)(value >> 24);
    uint8_t g       = DN_CAST(uint8_t)(value >> 16);
    uint8_t b       = DN_CAST(uint8_t)(value >>  8);
    DN_Str8 result = DN_Print_ESCColourStr8(colour, r, g, b);
    return result;
}

// NOTE: [$LLOG] DN_Log ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_Log_MakeStr8(DN_Arena                 *arena,
                                  bool                       colour,
                                  DN_Str8                   type,
                                  int                        log_type,
                                  DN_CallSite               call_site,
                                  DN_FMT_ATTRIB char const *fmt,
                                  va_list                    args)
{
    DN_LOCAL_PERSIST DN_USize max_type_length = 0;
    max_type_length                             = DN_MAX(max_type_length, type.size);
    int type_padding                            = DN_CAST(int)(max_type_length - type.size);

    DN_Str8 colour_esc = {};
    DN_Str8 bold_esc   = {};
    DN_Str8 reset_esc  = {};
    if (colour) {
        bold_esc  = DN_Print_ESCBoldStr8;
        reset_esc = DN_Print_ESCResetStr8;
        switch (log_type) {
            case DN_LogType_Debug:                                                                            break;
            case DN_LogType_Info:    colour_esc = DN_Print_ESCColourFgU32Str8(DN_LogTypeColourU32_Info);    break;
            case DN_LogType_Warning: colour_esc = DN_Print_ESCColourFgU32Str8(DN_LogTypeColourU32_Warning); break;
            case DN_LogType_Error:   colour_esc = DN_Print_ESCColourFgU32Str8(DN_LogTypeColourU32_Error);   break;
        }
    }

    DN_Str8 file_name            = DN_Str8_FileNameFromPath(call_site.file);
    DN_OSDateTimeStr8 const time = DN_OS_DateLocalTimeStr8Now();
    DN_Str8 header               = DN_Str8_InitF(arena,
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
                                                   DN_CAST(int)time.date_size - 2, time.date + 2, // date
                                                   DN_CAST(int)time.hms_size,      time.hms,      // hms
                                                   DN_STR_FMT(colour_esc),                        // colour
                                                   DN_STR_FMT(bold_esc),                          // bold
                                                   DN_STR_FMT(type),                              // type
                                                   DN_CAST(int)type_padding,  "",                 // type padding
                                                   DN_STR_FMT(reset_esc),                         // reset
                                                   DN_STR_FMT(file_name),                         // file name
                                                   call_site.line);                                // line number
    DN_USize header_size_no_ansi_codes = header.size - colour_esc.size - DN_Print_ESCResetStr8.size;

    // NOTE: Header padding ////////////////////////////////////////////////////////////////////////
    DN_LOCAL_PERSIST DN_USize max_header_length = 0;
    max_header_length                             = DN_MAX(max_header_length, header_size_no_ansi_codes);
    DN_USize header_padding                      = max_header_length - header_size_no_ansi_codes;

    // NOTE: Construct final log ///////////////////////////////////////////////////////////////////
    DN_Str8 user_msg = DN_Str8_InitFV(arena, fmt, args);
    DN_Str8 result   = DN_Str8_Alloc(arena, header.size + header_padding + user_msg.size, DN_ZeroMem_No);
    if (DN_Str8_HasData(result)) {
        DN_MEMCPY(result.data,               header.data, header.size);
        DN_MEMSET(result.data + header.size, ' ',         header_padding);
        if (DN_Str8_HasData(user_msg))
            DN_MEMCPY(result.data + header.size + header_padding, user_msg.data, user_msg.size);
    }
    return result;
}

DN_FILE_SCOPE void DN_Log_FVDefault_(DN_Str8 type, int log_type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_Core *core = g_dn_core;
    (void)log_type;
    (void)user_data;

    // NOTE: Open log file for appending if requested //////////////////////////
    DN_TicketMutex_Begin(&core->log_file_mutex);
    if (core->log_to_file && !core->log_file.handle && !core->log_file.error) {
        DN_TLSTMem tmem  = DN_TLS_TMem(nullptr);
        DN_Str8 log_path = DN_OS_PathF(tmem.arena, "%.*s/dn.log", DN_STR_FMT(core->exe_dir));
        core->log_file    = DN_OS_FileOpen(log_path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_AppendOnly, nullptr);
    }
    DN_TicketMutex_End(&core->log_file_mutex);

    // NOTE: Generate the log header ///////////////////////////////////////////
    DN_TLSTMem tmem  = DN_TLS_TMem(nullptr);
    DN_Str8    log_line = DN_Log_MakeStr8(tmem.arena, !core->log_no_colour, type, log_type, call_site, fmt, args);

    // NOTE: Print log /////////////////////////////////////////////////////////
    DN_Print_StdLn(log_type == DN_LogType_Error ? DN_PrintStd_Err : DN_PrintStd_Out, log_line);

    DN_TicketMutex_Begin(&core->log_file_mutex);
    DN_OS_FileWrite(&core->log_file, log_line, nullptr);
    DN_OS_FileWrite(&core->log_file, DN_STR8("\n"), nullptr);
    DN_TicketMutex_End(&core->log_file_mutex);
}

DN_API void DN_Log_FVCallSite(DN_Str8 type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (g_dn_core) {
        DN_LogProc *logging_function = g_dn_core->log_callback ? g_dn_core->log_callback : DN_Log_FVDefault_;
        logging_function(type, -1 /*log_type*/, g_dn_core->log_user_data, call_site, fmt, args);
    } else {
        // NOTE: Rarely taken branch, only when trying to use this library without initialising it
        DN_Print_StdLnFV(DN_PrintStd_Out, fmt, args);
    }
}

DN_API void DN_Log_FCallSite(DN_Str8 type, DN_CallSite call_site, DN_FMT_ATTRIB char const  *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Log_FVCallSite(type, call_site, fmt, args);
    va_end(args);
}

DN_API void DN_Log_TypeFVCallSite(DN_LogType type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_Str8 type_string = DN_STR8("DN-BAD-LOG-TYPE");
    switch (type) {
        case DN_LogType_Error:   type_string = DN_STR8("ERROR"); break;
        case DN_LogType_Info:    type_string = DN_STR8("INFO"); break;
        case DN_LogType_Warning: type_string = DN_STR8("WARN"); break;
        case DN_LogType_Debug:   type_string = DN_STR8("DEBUG"); break;
        case DN_LogType_Count:   type_string = DN_STR8("BADXX"); break;
    }
    DN_Log_FVCallSite(type_string, call_site, fmt, args);
}

DN_API void DN_Log_TypeFCallSite(DN_LogType type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Log_TypeFVCallSite(type, call_site, fmt, args);
    va_end(args);
}

// NOTE: [$ERRS] DN_ErrSink /////////////////////////////////////////////////////////////////////
DN_FILE_SCOPE void DN_ErrSink_Check_(DN_ErrSink const *err)
{
    DN_ASSERTF(err->arena, "Arena should be assigned in TLS init");
    if (!err->stack)
        return;

    DN_ErrSinkNode *node = err->stack;
    DN_ASSERT(node->mode >= DN_ErrSinkMode_Nil && node->mode <= DN_ErrSinkMode_ExitOnError);
    DN_ASSERTF(node->next != node, "Error nodes cannot point to themselves");
    DN_ASSERT(node->msg_sentinel);

    // NOTE: Walk the list ensuring we eventually terminate at the sentinel (e.g. we have a
    // well formed doubly-linked-list terminated by a sentinel, or otherwise we will hit the
    // walk limit or dereference a null pointer and assert)
    size_t WALK_LIMIT = 99'999;
    size_t walk       = 0;
    for (DN_ErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next, walk++) {
        DN_ASSERTF(it, "Encountered null pointer which should not happen in a sentinel DLL");
        DN_ASSERT(walk < WALK_LIMIT);
    }
}

DN_API DN_ErrSink *DN_ErrSink_Begin(DN_ErrSinkMode mode)
{
    DN_TLS         *tls       = DN_TLS_Get();
    DN_ErrSink     *result    = &tls->err_sink;
    DN_USize        arena_pos = DN_Arena_Pos(result->arena);
    DN_ErrSinkNode *node      = DN_Arena_New(result->arena, DN_ErrSinkNode, DN_ZeroMem_Yes);
    if (node) {
        node->next      = result->stack;
        node->arena_pos = arena_pos;
        node->mode      = mode;
        DN_SentinelDLL_InitArena(node->msg_sentinel, DN_ErrSinkMsg, result->arena);

        // NOTE: Put the node at the front of the stack (e.g. sorted newest to oldest)
        result->stack = node;
        result->debug_open_close_counter++;
    }

    // NOTE: Handle allocation error
    if (!DN_CHECK(node && node->msg_sentinel))
        DN_Arena_PopTo(result->arena, arena_pos);

    return result;
}

DN_API bool DN_ErrSink_HasError(DN_ErrSink *err)
{
    bool result = err && DN_SentinelDLL_HasItems(err->stack->msg_sentinel);
    return result;
}

DN_API DN_ErrSinkMsg *DN_ErrSink_End(DN_Arena *arena, DN_ErrSink *err)
{
    DN_ErrSinkMsg *result = nullptr;
    DN_ErrSink_Check_(err);
    if (!err || !err->stack)
        return result;

    DN_ASSERTF(arena != err->arena,
                "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");

    // NOTE: Walk the list and allocate it onto the user's arena
    DN_ErrSinkNode *node = err->stack;
    DN_ErrSinkMsg  *prev = nullptr;
    for (DN_ErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next) {
        DN_ErrSinkMsg *entry = DN_Arena_New(arena, DN_ErrSinkMsg, DN_ZeroMem_Yes);
        entry->msg            = DN_Str8_Copy(arena, it->msg);
        entry->call_site      = it->call_site;
        entry->error_code     = it->error_code;
        if (!result)
            result = entry;     // Assign first entry if we haven't yet
        if (prev)
            prev->next = entry; // Link the prev message to the current one
        prev = entry;           // Update prev to latest
    }

    // NOTE: Deallocate all the memory for this scope
    err->stack = err->stack->next;
    DN_ASSERT(err->debug_open_close_counter);
    err->debug_open_close_counter--;
    DN_Arena_PopTo(err->arena, node->arena_pos);
    return result;
}

static void DN_ErrSink_AddMsgToStr8Builder_(DN_Str8Builder *builder, DN_ErrSinkMsg *msg, DN_ErrSinkMsg *end)
{
    if (msg == end) // NOTE: No error messages to add
        return;

    if (msg->next == end) {
        DN_ErrSinkMsg *it        = msg;
        DN_Str8        file_name = DN_Str8_FileNameFromPath(it->call_site.file);
        DN_Str8Builder_AppendF(builder,
                             "%.*s:%05I32u:%.*s %.*s",
                             DN_STR_FMT(file_name),
                             it->call_site.line,
                             DN_STR_FMT(it->call_site.function),
                             DN_STR_FMT(it->msg));
    } else {
        // NOTE: More than one message
        for (DN_ErrSinkMsg *it = msg; it != end; it = it->next) {
            DN_Str8 file_name = DN_Str8_FileNameFromPath(it->call_site.file);
            DN_Str8Builder_AppendF(builder,
                                 "%s  - %.*s:%05I32u:%.*s%s%.*s",
                                 it == msg ? "" : "\n",
                                 DN_STR_FMT(file_name),
                                 it->call_site.line,
                                 DN_STR_FMT(it->call_site.function),
                                 DN_Str8_HasData(it->msg) ? " " : "",
                                 DN_STR_FMT(it->msg));
        }
    }
}

DN_API DN_Str8 DN_ErrSink_EndStr8(DN_Arena *arena, DN_ErrSink *err)
{
    DN_Str8 result = {};
    DN_ErrSink_Check_(err);
    if (!err || !err->stack)
        return result;

    DN_ASSERTF(arena != err->arena,
                "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");

    // NOTE: Walk the list and allocate it onto the user's arena
    DN_TLSTMem     tmem    = DN_TLS_PushTMem(arena);
    DN_Str8Builder builder = DN_Str8Builder_Init_TLS();
    DN_ErrSink_AddMsgToStr8Builder_(&builder, err->stack->msg_sentinel->next, err->stack->msg_sentinel);

    // NOTE: Deallocate all the memory for this scope
    uint64_t arena_pos = err->stack->arena_pos;
    err->stack         = err->stack->next;
    DN_Arena_PopTo(err->arena, arena_pos);
    DN_ASSERT(err->debug_open_close_counter);
    err->debug_open_close_counter--;

    result = DN_Str8Builder_Build(&builder, arena);
    return result;
}

DN_API void DN_ErrSink_EndAndIgnore(DN_ErrSink *err)
{
    DN_ErrSink_End(nullptr, err);
}

DN_API bool DN_ErrSink_EndAndLogError_(DN_ErrSink *err, DN_CallSite call_site, DN_Str8 err_msg)
{
    DN_ASSERTF(err->stack && err->stack->msg_sentinel, "Begin must be called before calling end");
    DN_TLSTMem     tmem = DN_TLS_PushTMem(nullptr);
    DN_ErrSinkMode mode = err->stack->mode;
    DN_ErrSinkMsg *msg  = DN_ErrSink_End(tmem.arena, err);
    if (!msg)
        return false;

    DN_Str8Builder builder = DN_Str8Builder_Init_TLS();
    if (DN_Str8_HasData(err_msg)) {
        DN_Str8Builder_AppendRef(&builder, err_msg);
        DN_Str8Builder_AppendRef(&builder, DN_STR8(":"));
    } else {
        DN_Str8Builder_AppendRef(&builder, DN_STR8("Error(s) encountered:"));
    }

    if (msg->next) // NOTE: More than 1 message
        DN_Str8Builder_AppendRef(&builder, DN_STR8("\n"));
    DN_ErrSink_AddMsgToStr8Builder_(&builder, msg, nullptr);

    DN_Str8 log = DN_Str8Builder_Build_TLS(&builder);
    DN_Log_TypeFCallSite(DN_LogType_Error, call_site, "%.*s", DN_STR_FMT(log));

    if (mode == DN_ErrSinkMode_DebugBreakOnEndAndLog) {
        DN_DEBUG_BREAK;
    }
    return true;
}

DN_API bool DN_ErrSink_EndAndLogErrorFV_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_TLSTMem tmem   = DN_TLS_TMem(nullptr);
    DN_Str8    log    = DN_Str8_InitFV(tmem.arena, fmt, args);
    bool        result = DN_ErrSink_EndAndLogError_(err, call_site, log);
    return result;
}

DN_API bool DN_ErrSink_EndAndLogErrorF_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_TLSTMem tmem   = DN_TLS_TMem(nullptr);
    DN_Str8    log    = DN_Str8_InitFV(tmem.arena, fmt, args);
    bool        result = DN_ErrSink_EndAndLogError_(err, call_site, log);
    va_end(args);
    return result;
}

DN_API void DN_ErrSink_EndAndExitIfErrorFV_(DN_ErrSink *err, DN_CallSite call_site, uint32_t exit_val, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (DN_ErrSink_EndAndLogErrorFV_(err, call_site, fmt, args)) {
        DN_DEBUG_BREAK;
        DN_OS_Exit(exit_val);
    }
}

DN_API void DN_ErrSink_EndAndExitIfErrorF_(DN_ErrSink *err, DN_CallSite call_site, uint32_t exit_val, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_ErrSink_EndAndExitIfErrorFV_(err, call_site, exit_val, fmt, args);
    va_end(args);
}

DN_API void DN_ErrSink_AppendFV_(DN_ErrSink *err, uint32_t error_code, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    if (!err)
        return;

    DN_ErrSinkNode *node = err->stack;
    DN_ASSERTF(node, "Error sink must be begun by calling 'Begin' before using this function.");

    DN_ErrSinkMsg *msg = DN_Arena_New(err->arena, DN_ErrSinkMsg, DN_ZeroMem_Yes);
    if (DN_CHECK(msg)) {
        msg->msg        = DN_Str8_InitFV(err->arena, fmt, args);
        msg->error_code = error_code;
        msg->call_site  = DN_TLS_Get()->call_site;
        DN_SentinelDLL_Prepend(node->msg_sentinel, msg);

        if (node->mode == DN_ErrSinkMode_ExitOnError)
            DN_ErrSink_EndAndExitIfErrorF_(err, msg->call_site, error_code, "Fatal error %u", error_code);
    }
}

DN_API void DN_ErrSink_AppendF_(DN_ErrSink *err, uint32_t error_code, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_ErrSink_AppendFV_(err, error_code, fmt, args);
    va_end(args);
}
