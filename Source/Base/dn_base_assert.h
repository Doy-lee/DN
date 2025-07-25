#if !defined(DN_BASE_ASSERT_H)
#define DN_BASE_ASSERT_H

#define DN_HardAssertF(expr, fmt, ...)                                                  \
  do {                                                                                  \
    if (!(expr)) {                                                                      \
      DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
      DN_LOG_ErrorF("Hard assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,     \
                    DN_STR_FMT(stack_trace_),                                           \
                    ##__VA_ARGS__);                                                     \
      DN_DebugBreak;                                                                    \
    }                                                                                   \
  } while (0)
#define DN_HardAssert(expr) DN_HardAssertF(expr, "")

#if defined(DN_NO_ASSERT)
  #define DN_Assert(...)
  #define DN_AssertOnce(...)
  #define DN_AssertF(...)
  #define DN_AssertFOnce(...)
#else
  #define DN_AssertF(expr, fmt, ...)                                                      \
    do {                                                                                  \
      if (!(expr)) {                                                                      \
        DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
        DN_LOG_ErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,          \
                      DN_STR_FMT(stack_trace_),                                           \
                      ##__VA_ARGS__);                                                     \
        DN_DebugBreak;                                                                    \
      }                                                                                   \
    } while (0)

  #define DN_AssertFOnce(expr, fmt, ...)                                                  \
    do {                                                                                  \
      static bool once = true;                                                            \
      if (!(expr) && once) {                                                              \
        once                 = false;                                                     \
        DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8FromHeap(128 /*limit*/, 2 /*skip*/); \
        DN_LOG_ErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,          \
                      DN_STR_FMT(stack_trace_),                                           \
                      ##__VA_ARGS__);                                                     \
        DN_DebugBreak;                                                                    \
      }                                                                                   \
    } while (0)

  #define DN_Assert(expr)     DN_AssertF((expr), "")
  #define DN_AssertOnce(expr) DN_AssertFOnce((expr), "")
#endif

#define DN_InvalidCodePathF(fmt, ...) DN_HardAssertF(0, fmt, ##__VA_ARGS__)
#define DN_InvalidCodePath            DN_InvalidCodePathF("Invalid code path triggered")

#define DN_Check(expr) DN_CheckF(expr, "")
#if defined(DN_NO_CHECK_BREAK)
  #define DN_CheckF(expr, fmt, ...) \
    ((expr) ? true : (DN_LOG_WarningF(fmt, ##__VA_ARGS__), false))
#else
  #define DN_CheckF(expr, fmt, ...) \
    ((expr) ? true : (DN_LOG_ErrorF(fmt, ##__VA_ARGS__), DN_StackTrace_Print(128 /*limit*/), DN_DebugBreak, false))
#endif

#endif
