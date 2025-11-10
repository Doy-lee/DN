#if !defined(DN_OS_TLS_H)
#define DN_OS_TLS_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
  #include "../dn_os_inc.h"
#endif

// NOTE: DN_OSErrSink
enum DN_OSErrSinkMode
{
  DN_OSErrSinkMode_Nil,                   // Default behaviour to accumulate errors into the sink
  DN_OSErrSinkMode_DebugBreakOnEndAndLog, // Debug break (int3) when error is encountered and the sink is ended by the 'end and log' functions.
  DN_OSErrSinkMode_ExitOnError,           // When an error is encountered, exit the program with the error code of the error that was caught.
};

struct DN_OSErrSinkMsg
{
  DN_I32           error_code;
  DN_Str8          msg;
  DN_CallSite      call_site;
  DN_OSErrSinkMsg *next;
  DN_OSErrSinkMsg *prev;
};

struct DN_OSErrSinkNode
{
  DN_CallSite      call_site;    // Call site that the node was created
  DN_OSErrSinkMode mode;         // Controls how the sink behaves when an error is registered onto the sink.
  DN_OSErrSinkMsg *msg_sentinel; // List of error messages accumulated for the current scope
  DN_U64           arena_pos;    // Position to reset the arena when the scope is ended
};

struct DN_OSErrSink
{
  DN_Arena        * arena;      // Dedicated allocator from the thread's local storage
  DN_OSErrSinkNode  stack[128]; // Each entry contains errors accumulated between a [begin, end] region of the active sink.
  DN_USize          stack_size;
};

enum DN_OSTLSArena
{
  DN_OSTLSArena_Main,      // NOTE: Arena for permanent allocations
  DN_OSTLSArena_ErrorSink, // NOTE: Arena for logging error information for this thread

  // NOTE: Per-thread scratch arenas (2 to prevent aliasing)
  DN_OSTLSArena_TMem0,
  DN_OSTLSArena_TMem1,

  DN_OSTLSArena_Count,
};

struct DN_OSTLS
{
  DN_B32       init;                        // Flag to track if TLS has been initialised
  DN_U64       thread_id;
  DN_CallSite  call_site;                   // Stores call-site information when requested by thread
  DN_OSErrSink err_sink;                    // Error handling state
  DN_Arena     arenas[DN_OSTLSArena_Count]; // Default arenas that the thread has access to implicitly
  DN_Arena *   arena_stack[8];              // Active stack of arenas push/popped arenas on into the TLS
  DN_USize     arena_stack_index;

  DN_Arena *   frame_arena;
  char         name[64];
  DN_U8        name_size;
};

// Push the temporary memory arena when retrieved, popped when the arena goes
// out of scope. Pushed arenas are used automatically as the allocator in TLS
// suffixed function.
enum DN_OSTLSPushTMem
{
  DN_OSTLSPushTMem_No,
  DN_OSTLSPushTMem_Yes,
};

struct DN_OSTLSTMem
{
  DN_OSTLSTMem(DN_OSTLS *context, uint8_t context_index, DN_OSTLSPushTMem push_scratch);
  ~DN_OSTLSTMem();
  DN_Arena        *arena;
  DN_B32           destructed;
  DN_OSTLSPushTMem push_arena;
  DN_ArenaTempMem  temp_mem;
};

struct DN_OSTLSInitArgs
{
  DN_U64 reserve;
  DN_U64 commit;
  DN_U64 err_sink_reserve;
  DN_U64 err_sink_commit;
};

DN_API void              DN_OS_TLSInit                       (DN_OSTLS *tls, DN_OSTLSInitArgs args);
DN_API void              DN_OS_TLSDeinit                     (DN_OSTLS *tls);
DN_API DN_OSTLS *        DN_OS_TLSGet                        ();
DN_API void              DN_OS_TLSSetCurrentThreadTLS        (DN_OSTLS *tls);
DN_API DN_Arena *        DN_OS_TLSArena                      ();
DN_API DN_OSTLSTMem      DN_OS_TLSGetTMem                    (void const *conflict_arena, DN_OSTLSPushTMem push_tmp_mem);
DN_API void              DN_OS_TLSPushArena                  (DN_Arena *arena);
DN_API void              DN_OS_TLSPopArena                   ();
DN_API DN_Arena *        DN_OS_TLSTopArena                   ();
DN_API void              DN_OS_TLSBeginFrame                 (DN_Arena *frame_arena);
DN_API DN_Arena *        DN_OS_TLSFrameArena                 ();
#define                  DN_OS_TLSSaveCallSite               do { DN_OS_TLSGet()->call_site = DN_CALL_SITE; } while (0)
#define                  DN_OS_TLSTMem(...)                  DN_OS_TLSGetTMem(__VA_ARGS__, DN_OSTLSPushTMem_No)
#define                  DN_OS_TLSPushTMem(...)              DN_OS_TLSGetTMem(__VA_ARGS__, DN_OSTLSPushTMem_Yes)

DN_API DN_OSErrSink *    DN_OS_ErrSinkBegin_                 (DN_OSErrSinkMode mode, DN_CallSite call_site);
#define                  DN_OS_ErrSinkBegin(mode)            DN_OS_ErrSinkBegin_(mode, DN_CALL_SITE)
#define                  DN_OS_ErrSinkBeginDefault()         DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil)
DN_API bool              DN_OS_ErrSinkHasError               (DN_OSErrSink *err);
DN_API DN_OSErrSinkMsg * DN_OS_ErrSinkEnd                    (DN_Arena *arena, DN_OSErrSink *err);
DN_API DN_Str8           DN_OS_ErrSinkEndStr8                (DN_Arena *arena, DN_OSErrSink *err);
DN_API void              DN_OS_ErrSinkEndAndIgnore           (DN_OSErrSink *err);
DN_API bool              DN_OS_ErrSinkEndAndLogError_        (DN_OSErrSink *err, DN_CallSite call_site, DN_Str8 msg);
DN_API bool              DN_OS_ErrSinkEndAndLogErrorFV_      (DN_OSErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool              DN_OS_ErrSinkEndAndLogErrorF_       (DN_OSErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void              DN_OS_ErrSinkEndAndExitIfErrorF_    (DN_OSErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void              DN_OS_ErrSinkEndAndExitIfErrorFV_   (DN_OSErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void              DN_OS_ErrSinkAppendFV_              (DN_OSErrSink *err, DN_U32 error_code, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void              DN_OS_ErrSinkAppendF_               (DN_OSErrSink *err, DN_U32 error_code, DN_FMT_ATTRIB char const *fmt, ...);
#define                  DN_OS_ErrSinkEndAndLogError(err, err_msg)                  DN_OS_ErrSinkEndAndLogError_(err, DN_CALL_SITE, err_msg)
#define                  DN_OS_ErrSinkEndAndLogErrorFV(err, fmt, args)              DN_OS_ErrSinkEndAndLogErrorFV_(err, DN_CALL_SITE, fmt, args)
#define                  DN_OS_ErrSinkEndAndLogErrorF(err, fmt, ...)                DN_OS_ErrSinkEndAndLogErrorF_(err, DN_CALL_SITE, fmt, ##__VA_ARGS__)
#define                  DN_OS_ErrSinkEndAndExitIfErrorFV(err, exit_val, fmt, args) DN_OS_ErrSinkEndAndExitIfErrorFV_(err, DN_CALL_SITE, exit_val, fmt, args)
#define                  DN_OS_ErrSinkEndAndExitIfErrorF(err, exit_val, fmt, ...)   DN_OS_ErrSinkEndAndExitIfErrorF_(err, DN_CALL_SITE, exit_val, fmt, ##__VA_ARGS__)

#define DN_OS_ErrSinkAppendFV(error, error_code, fmt, args) \
  do {                                                      \
    DN_OS_TLSSaveCallSite;                                  \
    DN_OS_ErrSinkAppendFV_(error, error_code, fmt, args);   \
  } while (0)

#define DN_OS_ErrSinkAppendF(error, error_code, fmt, ...)         \
  do {                                                            \
    DN_OS_TLSSaveCallSite;                                        \
    DN_OS_ErrSinkAppendF_(error, error_code, fmt, ##__VA_ARGS__); \
  } while (0)

#endif // defined(DN_OS_TLS_H)
