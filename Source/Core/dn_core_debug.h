#if !defined(DN_CORE_DEBUG_H)
#define DN_CORE_DEBUG_H

#include "../dn_base_inc.h"

// NOTE: DN_Debug
enum DN_DebugAllocFlag
{
  DN_DebugAllocFlag_Freed         = 1 << 0,
  DN_DebugAllocFlag_LeakPermitted = 1 << 1,
};

struct DN_DebugAlloc
{
  void     *ptr;               // 8  Pointer to the allocation being tracked
  DN_USize  size;              // 16 Size of the allocation
  DN_USize  freed_size;        // 24 Store the size of the allocation when it is freed
  DN_Str8   stack_trace;       // 40 Stack trace at the point of allocation
  DN_Str8   freed_stack_trace; // 56 Stack trace of where the allocation was freed
  DN_U16    flags;             // 72 Bit flags from `DN_DebugAllocFlag`
};

static_assert(sizeof(DN_DebugAlloc) == 64 || sizeof(DN_DebugAlloc) == 32, // NOTE: 64 bit vs 32 bit pointers respectively
              "We aim to keep the allocation record as light as possible as "
              "memory tracking can get expensive. Enforce that there is no "
              "unexpected padding.");

// NOTE: DN_Profiler
struct DN_ProfilerAnchor
{
  // Inclusive refers to the time spent to complete the function call
  // including all children functions.
  //
  // Exclusive refers to the time spent in the function, not including any
  // time spent in children functions that we call that are also being
  // profiled. If we recursively call into ourselves, the time we spent in
  // our function is accumulated.
  DN_U64  tsc_inclusive;
  DN_U64  tsc_exclusive;
  DN_U16  hit_count;
  DN_Str8 name;
};

struct DN_ProfilerZone
{
  DN_U16 anchor_index;
  DN_U64 begin_tsc;
  DN_U16 parent_zone;
  DN_U64 elapsed_tsc_at_zone_start;
};

struct DN_ProfilerAnchorArray
{
  DN_ProfilerAnchor *data;
  DN_USize           count;
};

enum DN_ProfilerTSC
{
  DN_ProfilerTSC_RDTSC,
  DN_ProfilerTSC_OSPerformanceCounter,
};

struct DN_Profiler
{
  DN_USize           frame_index;
  DN_ProfilerAnchor *anchors;
  DN_USize           anchors_count;
  DN_USize           anchors_per_frame;
  DN_U16             parent_zone;
  bool               paused;
  DN_ProfilerTSC     tsc;
  DN_U64             tsc_frequency;
  DN_ProfilerZone    frame_zone;
  DN_F64             frame_avg_tsc;
};

#define DN_Profiler_ZoneLoop(prof, name, index)                                                                         \
  DN_ProfilerZone DN_UniqueName(zone_) = DN_Profiler_BeginZone(prof, DN_STR8(name), index), DN_UniqueName(dummy_) = {}; \
  DN_UniqueName(dummy_).begin_tsc == 0;                                                                                 \
  DN_Profiler_EndZone(prof, DN_UniqueName(zone_)), DN_UniqueName(dummy_).begin_tsc = 1

#define DN_Profiler_ZoneLoopAuto(prof, name) DN_Profiler_ZoneLoop(prof, name, __COUNTER__ + 1)

DN_API DN_Profiler            DN_Profiler_Init                      (DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTSC tsc, DN_U64 tsc_frequency);
DN_API DN_ProfilerZone        DN_Profiler_BeginZone                 (DN_Profiler *profiler, DN_Str8 name, DN_U16 anchor_index);
#define                       DN_Profiler_BeginZoneAuto(prof, name) DN_Profiler_BeginZone(prof, DN_STR8(name), __COUNTER__ + 1)
DN_API void                   DN_Profiler_EndZone                   (DN_Profiler *profiler, DN_ProfilerZone zone);
DN_API DN_USize               DN_Profiler_FrameCount                (DN_Profiler const *profiler);
DN_API DN_ProfilerAnchorArray DN_Profiler_FrameAnchorsFromIndex     (DN_Profiler *profiler, DN_USize frame_index);
DN_API DN_ProfilerAnchorArray DN_Profiler_FrameAnchors              (DN_Profiler *profiler);
DN_API void                   DN_Profiler_NewFrame                  (DN_Profiler *profiler);
DN_API void                   DN_Profiler_Dump                      (DN_Profiler *profiler);
DN_API DN_F64                 DN_Profiler_SecFromTSC                (DN_Profiler *profiler, DN_U64 duration_tsc);
DN_API DN_F64                 DN_Profiler_MsFromTSC                 (DN_Profiler *profiler, DN_U64 duration_tsc);


#if defined(DN_LEAK_TRACKING)
DN_API void            DN_DBGTrackAlloc                            (void *ptr, DN_USize size, bool alloc_can_leak);
DN_API void            DN_DBGTrackDealloc                          (void *ptr);
DN_API void            DN_DBGDumpLeaks                             ();
#else
#define                DN_DBGTrackAlloc(ptr, size, alloc_can_leak) do { (void)ptr; (void)size; (void)alloc_can_leak; } while (0)
#define                DN_DBGTrackDealloc(ptr)                     do { (void)ptr;                                   } while (0)
#define                DN_DBGDumpLeaks()                           do {                                              } while (0)
#endif
#endif // DN_CORE_DEBUG_H
