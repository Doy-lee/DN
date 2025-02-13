#include "dqn.h"

#define DN_CPP

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   /$$$$$$\ $$\      $$\ $$$$$$$\  $$\
//   \_$$  _|$$$\    $$$ |$$  __$$\ $$ |
//     $$ |  $$$$\  $$$$ |$$ |  $$ |$$ |
//     $$ |  $$\$$\$$ $$ |$$$$$$$  |$$ |
//     $$ |  $$ \$$$  $$ |$$  ____/ $$ |
//     $$ |  $$ |\$  /$$ |$$ |      $$ |
//   $$$$$$\ $$ | \_/ $$ |$$ |      $$$$$$$$\
//   \______|\__|     \__|\__|      \________|
//
//   Implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

#if defined(DN_WITH_CGEN)
    #if !defined(DN_NO_METADESK)
        DN_MSVC_WARNING_PUSH
        DN_MSVC_WARNING_DISABLE(4505) // warning C4505: '<function>': unreferenced function with internal linkage has been removed

        DN_GCC_WARNING_PUSH
        DN_GCC_WARNING_DISABLE(-Wwrite-strings)
        DN_GCC_WARNING_DISABLE(-Wunused-but-set-variable)
        DN_GCC_WARNING_DISABLE(-Wsign-compare)
        DN_GCC_WARNING_DISABLE(-Wunused-function)
        DN_GCC_WARNING_DISABLE(-Wunused-result)

        #include "External/metadesk/md.c"

        DN_GCC_WARNING_POP
        DN_MSVC_WARNING_POP
    #endif
    #define DN_CPP_FILE_IMPLEMENTATION
    #include "Standalone/dqn_cpp_file.h"
    #include "dqn_cgen.cpp"
#endif

#if defined(DN_WITH_JSON)
    #include "dqn_json.cpp"
#endif

#include "dqn_base.cpp"
#include "dqn_external.cpp"
#include "dqn_allocator.cpp"
#include "dqn_debug.cpp"
#include "dqn_string.cpp"
#include "dqn_containers.cpp"
#include "dqn_type_info.cpp"
#include "dqn_os.cpp"

#if defined(DN_PLATFORM_EMSCRIPTEN) || defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_ARM64)
    #include "dqn_os_posix.cpp"
#elif defined(DN_PLATFORM_WIN32)
    #include "dqn_os_win32.cpp"
#else
    #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif

#include "dqn_tls.cpp"
#include "dqn_math.cpp"
#include "dqn_hash.cpp"
#include "dqn_helpers.cpp"

#if defined(DN_WITH_UNIT_TESTS)
    #include "dqn_unit_tests.cpp"
#endif

#include "dqn_docs.cpp"
