#if !defined(DN_BASE_ASSERT_H)
#define DN_BASE_ASSERT_H

#define DN_HardAssertF(expr, fmt, ...)                                                 \
  do {                                                                                 \
    if (!(expr)) {                                                                     \
      DN_Str8 stack_trace_ = DN_StackTraceWalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
      DN_LogErrorF("Hard assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,     \
                    DN_Str8PrintFmt(stack_trace_),                                     \
                    ##__VA_ARGS__);                                                    \
      DN_DebugBreak;                                                                   \
    }                                                                                  \
  } while (0)
#define DN_HardAssert(expr) DN_HardAssertF(expr, "")

// NOTE: Our default assert requires stack traces which has a bit of a chicken-and-egg problem if
// we're trying to detect some code related to the DN startup sequence. If we try to assert before
// the OS layer is initialised stack-traces will try to use temporary memory which requires TLS to
// be setup which belongs to the OS.
//
// This causes recursion errors as they call into each other. We use RawAsserts for these kind of
// checks.
#if defined(DN_NO_ASSERT)
  #define DN_RawAssert(...)
  #define DN_Assert(...)
  #define DN_AssertOnce(...)
  #define DN_AssertF(...)
  #define DN_AssertFOnce(...)
#else
  #define DN_RawAssert(expr) do { if (!(expr)) DN_DebugBreak; } while (0)

  #define DN_AssertF(expr, fmt, ...)                                                     \
    do {                                                                                 \
      if (!(expr)) {                                                                     \
        DN_Str8 stack_trace_ = DN_StackTraceWalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
        DN_LogErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,          \
                      DN_Str8PrintFmt(stack_trace_),                                     \
                      ##__VA_ARGS__);                                                    \
        DN_DebugBreak;                                                                   \
      }                                                                                  \
    } while (0)

  #define DN_AssertFOnce(expr, fmt, ...)                                                 \
    do {                                                                                 \
      static bool once = true;                                                           \
      if (!(expr) && once) {                                                             \
        once                 = false;                                                    \
        DN_Str8 stack_trace_ = DN_StackTraceWalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
        DN_LogErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,         \
                      DN_Str8PrintFmt(stack_trace_),                                     \
                      ##__VA_ARGS__);                                                    \
        DN_DebugBreak;                                                                   \
      }                                                                                  \
    } while (0)

  #define DN_Assert(expr)     DN_AssertF((expr), "")
  #define DN_AssertOnce(expr) DN_AssertFOnce((expr), "")
#endif

#define DN_InvalidCodePathF(fmt, ...) DN_HardAssertF(0, fmt, ##__VA_ARGS__)
#define DN_InvalidCodePath            DN_InvalidCodePathF("Invalid code path triggered")
#define DN_StaticAssert(expr)                                                     \
  DN_GCC_WARNING_PUSH                                                             \
  DN_GCC_WARNING_DISABLE(-Wunused-local-typedefs)                                 \
  typedef char DN_TokenCombine(static_assert_dummy__, __LINE__)[(expr) ? 1 : -1]; \
  DN_GCC_WARNING_POP

#define DN_Check(expr) DN_CheckF(expr, "")
#if defined(DN_NO_CHECK_BREAK)
  #define DN_CheckF(expr, fmt, ...) \
    ((expr) ? true : (DN_LogWarningF(fmt, ##__VA_ARGS__), false))
#else
  #define DN_CheckF(expr, fmt, ...) \
    ((expr) ? true : (DN_LogErrorF(fmt, ##__VA_ARGS__), DN_StackTracePrint(128 /*limit*/), DN_DebugBreak, false))
#endif

#endif
