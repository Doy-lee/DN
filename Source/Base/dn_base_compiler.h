#if !defined(DN_BASE_COMPILER_H)
#define DN_BASE_COMPILER_H

// NOTE: Compiler identification ///////////////////////////////////////////////////////////////////
// Warning! Order is important here, clang-cl on Windows defines _MSC_VER
#if defined(_MSC_VER)
  #if defined(__clang__)
    #define DN_COMPILER_CLANG_CL
    #define DN_COMPILER_CLANG
  #else
    #define DN_COMPILER_MSVC
  #endif
#elif defined(__clang__)
  #define DN_COMPILER_CLANG
#elif defined(__GNUC__)
  #define DN_COMPILER_GCC
#endif

// NOTE: __has_feature /////////////////////////////////////////////////////////////////////////////
// MSVC for example does not support the feature detection macro for instance so we compile it out
#if defined(__has_feature)
  #define DN_HAS_FEATURE(expr) __has_feature(expr)
#else
  #define DN_HAS_FEATURE(expr) 0
#endif

// NOTE: __has_builtin /////////////////////////////////////////////////////////////////////////////
// MSVC for example does not support the feature detection macro for instance so we compile it out
#if defined(__has_builtin)
  #define DN_HAS_BUILTIN(expr) __has_builtin(expr)
#else
  #define DN_HAS_BUILTIN(expr) 0
#endif

// NOTE: Warning suppression macros ////////////////////////////////////////////////////////////////
#if defined(DN_COMPILER_MSVC)
  #define DN_MSVC_WARNING_PUSH         __pragma(warning(push))
  #define DN_MSVC_WARNING_DISABLE(...) __pragma(warning(disable :##__VA_ARGS__))
  #define DN_MSVC_WARNING_POP          __pragma(warning(pop))
#else
  #define DN_MSVC_WARNING_PUSH
  #define DN_MSVC_WARNING_DISABLE(...)
  #define DN_MSVC_WARNING_POP
#endif

#if defined(DN_COMPILER_CLANG) || defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG_CL)
  #define DN_GCC_WARNING_PUSH                _Pragma("GCC diagnostic push")
  #define DN_GCC_WARNING_DISABLE_HELPER_0(x) #x
  #define DN_GCC_WARNING_DISABLE_HELPER_1(y) DN_GCC_WARNING_DISABLE_HELPER_0(GCC diagnostic ignored #y)
  #define DN_GCC_WARNING_DISABLE(warning)    _Pragma(DN_GCC_WARNING_DISABLE_HELPER_1(warning))
  #define DN_GCC_WARNING_POP                 _Pragma("GCC diagnostic pop")
#else
  #define DN_GCC_WARNING_PUSH
  #define DN_GCC_WARNING_DISABLE(...)
  #define DN_GCC_WARNING_POP
#endif

// NOTE: Host OS identification ////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
  #define DN_OS_WIN32
#elif defined(__gnu_linux__) || defined(__linux__)
  #define DN_OS_UNIX
#endif

// NOTE: Platform identification ///////////////////////////////////////////////////////////////////
#if !defined(DN_PLATFORM_EMSCRIPTEN) && \
    !defined(DN_PLATFORM_POSIX) &&      \
    !defined(DN_PLATFORM_WIN32)
  #if defined(__aarch64__) || defined(_M_ARM64)
    #define DN_PLATFORM_ARM64
  #elif defined(__EMSCRIPTEN__)
    #define DN_PLATFORM_EMSCRIPTEN
  #elif defined(DN_OS_WIN32)
    #define DN_PLATFORM_WIN32
  #else
    #define DN_PLATFORM_POSIX
  #endif
#endif

// NOTE: Windows crap //////////////////////////////////////////////////////////////////////////////
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #if defined(_CRT_SECURE_NO_WARNINGS)
    #define DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED
  #else
    #define _CRT_SECURE_NO_WARNINGS
  #endif
#endif

// NOTE: Force Inline //////////////////////////////////////////////////////////////////////////////
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #define DN_FORCE_INLINE __forceinline
#else
  #define DN_FORCE_INLINE inline __attribute__((always_inline))
#endif

// NOTE: Function/Variable Annotations /////////////////////////////////////////////////////////////
#if defined(DN_STATIC_API)
  #define DN_API static
#else
  #define DN_API
#endif

// NOTE: C/CPP Literals ////////////////////////////////////////////////////////////////////////////
// Declare struct literals that work in both C and C++ because the syntax is different between
// languages.
#if 0
  struct Foo { int a; }
  struct Foo foo = DN_LITERAL(Foo){32}; // Works on both C and C++
#endif

#if defined(__cplusplus)
  #define DN_Literal(T) T
#else
  #define DN_Literal(T) (T)
#endif

// NOTE: Thread Locals /////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
  #define DN_THREAD_LOCAL thread_local
#else
  #define DN_THREAD_LOCAL _Thread_local
#endif

// NOTE: C variadic argument annotations ///////////////////////////////////////////////////////////
// TODO: Other compilers
#if defined(DN_COMPILER_MSVC)
  #define DN_FMT_ATTRIB _Printf_format_string_
#else
  #define DN_FMT_ATTRIB
#endif

// NOTE: Type Cast /////////////////////////////////////////////////////////////////////////////////
#define DN_Cast(val) (val)

// NOTE: Zero initialisation macro /////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
  #define DN_ZeroInit {}
#else
  #define DN_ZeroInit {0}
#endif

// NOTE: Address sanitizer /////////////////////////////////////////////////////////////////////////
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

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  #include <sanitizer/asan_interface.h>
#endif
#endif // !defined(DN_BASE_COMPILER_H)
