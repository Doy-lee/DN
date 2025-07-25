#if !defined(DN_BASE_OS_H)
#define DN_BASE_OS_H

#include "../dn_base_inc.h"

// NOTE: OS primitives that the OS layer can provide for the base layer but is optional.

struct DN_StackTraceFrame
{
  DN_U64  address;
  DN_U64  line_number;
  DN_Str8 file_name;
  DN_Str8 function_name;
};

struct DN_StackTraceRawFrame
{
  void  *process;
  DN_U64 base_addr;
};

struct DN_StackTraceWalkResult
{
  void   *process;   // [Internal] Windows handle to the process
  DN_U64 *base_addr; // The addresses of the functions in the stack trace
  DN_U16  size;      // The number of `base_addr`'s stored from the walk
};

struct DN_StackTraceWalkResultIterator
{
  DN_StackTraceRawFrame raw_frame;
  DN_U16                index;
};


#if defined(DN_FREESTANDING)
#define                             DN_StackTrace_WalkStr8FromHeap(...) DN_STR8("N/A")
#define                             DN_StackTrace_Walk(...)
#define                             DN_StackTrace_WalkResultIterate(...)
#define                             DN_StackTrace_WalkResultToStr8(...) DN_STR8("N/A")
#define                             DN_StackTrace_WalkStr8(...) DN_STR8("N/A")
#define                             DN_StackTrace_WalkStr8FromHeap(...) DN_STR8("N/A")
#define                             DN_StackTrace_GetFrames(...)
#define                             DN_StackTrace_RawFrameToFrame(...)
#define                             DN_StackTrace_Print(...)
#define                             DN_StackTrace_ReloadSymbols(...)
#else
DN_API DN_StackTraceWalkResult      DN_StackTrace_Walk             (struct DN_Arena *arena, DN_U16 limit);
DN_API bool                         DN_StackTrace_WalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk);
DN_API DN_Str8                      DN_StackTrace_WalkResultToStr8 (struct DN_Arena *arena, DN_StackTraceWalkResult const *walk, DN_U16 skip);
DN_API DN_Str8                      DN_StackTrace_WalkStr8         (struct DN_Arena *arena, DN_U16 limit, DN_U16 skip);
DN_API DN_Str8                      DN_StackTrace_WalkStr8FromHeap (DN_U16 limit, DN_U16 skip);
DN_API DN_Slice<DN_StackTraceFrame> DN_StackTrace_GetFrames        (struct DN_Arena *arena, DN_U16 limit);
DN_API DN_StackTraceFrame           DN_StackTrace_RawFrameToFrame  (struct DN_Arena *arena, DN_StackTraceRawFrame raw_frame);
DN_API void                         DN_StackTrace_Print            (DN_U16 limit);
DN_API void                         DN_StackTrace_ReloadSymbols    ();
#endif
#endif // !defined(DN_BASE_OS_H)
