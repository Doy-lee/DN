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
#define                             DN_StackTraceWalkStr8FromHeap(...) DN_Str8Lit("N/A")
#define                             DN_StackTraceWalk(...)
#define                             DN_StackTraceWalkResultIterate(...)
#define                             DN_StackTraceWalkResultToStr8(...) DN_Str8Lit("N/A")
#define                             DN_StackTraceWalkStr8(...) DN_Str8Lit("N/A")
#define                             DN_StackTraceWalkStr8FromHeap(...) DN_Str8Lit("N/A")
#define                             DN_StackTraceGetFrames(...)
#define                             DN_StackTraceRawFrameToFrame(...)
#define                             DN_StackTracePrint(...)
#define                             DN_StackTraceReloadSymbols(...)
#else
DN_API DN_StackTraceWalkResult      DN_StackTraceWalk             (struct DN_Arena *arena, DN_U16 limit);
DN_API bool                         DN_StackTraceWalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk);
DN_API DN_Str8                      DN_StackTraceWalkResultToStr8 (struct DN_Arena *arena, DN_StackTraceWalkResult const *walk, DN_U16 skip);
DN_API DN_Str8                      DN_StackTraceWalkStr8         (struct DN_Arena *arena, DN_U16 limit, DN_U16 skip);
DN_API DN_Str8                      DN_StackTraceWalkStr8FromHeap (DN_U16 limit, DN_U16 skip);
DN_API DN_Slice<DN_StackTraceFrame> DN_StackTraceGetFrames        (struct DN_Arena *arena, DN_U16 limit);
DN_API DN_StackTraceFrame           DN_StackTraceRawFrameToFrame  (struct DN_Arena *arena, DN_StackTraceRawFrame raw_frame);
DN_API void                         DN_StackTracePrint            (DN_U16 limit);
DN_API void                         DN_StackTraceReloadSymbols    ();
#endif
#endif // !defined(DN_BASE_OS_H)
