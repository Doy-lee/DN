#if !defined(DN_CORE_DEBUG_H)
#define DN_CORE_DEBUG_H

// NOTE: DN_StackTrace /////////////////////////////////////////////////////////////////////////////
// NOTE: DN_Debug //////////////////////////////////////////////////////////////////////////////////
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

// NOTE: DN_Profiler ///////////////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_PROFILER)
#if !defined(DN_PROFILER_ANCHOR_BUFFER_SIZE)
  #define DN_PROFILER_ANCHOR_BUFFER_SIZE 256
#endif

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

#if defined(__cplusplus)
struct DN_ProfilerZoneScope
{
  DN_ProfilerZoneScope(DN_Str8 name, DN_U16 anchor_index);
  ~DN_ProfilerZoneScope();
  DN_ProfilerZone zone;
};

#define DN_Profiler_ZoneScopeAtIndex(name, anchor_index) auto DN_UniqueName(profile_zone_) = DN_ProfilerZoneScope(DN_STR8(name), anchor_index)
#define DN_Profiler_ZoneScope(name)                      DN_Profiler_ZoneScopeAtIndex(name, __COUNTER__ + 1)
#endif

#define DN_Profiler_ZoneBlockIndex(name, index)                                                                                \
  for (DN_ProfilerZone DN_UniqueName(profile_zone__) = DN_Profiler_BeginZoneAtIndex(name, index), DN_UniqueName(dummy__) = {}; \
       DN_UniqueName(dummy__).begin_tsc == 0;                                                                                  \
       DN_Profiler_EndZone(DN_UniqueName(profile_zone__)), DN_UniqueName(dummy__).begin_tsc = 1)

#define DN_Profiler_ZoneBlock(name) DN_Profiler_ZoneBlockIndex(DN_STR8(name), __COUNTER__ + 1)

enum DN_ProfilerAnchorBuffer
{
  DN_ProfilerAnchorBuffer_Back,
  DN_ProfilerAnchorBuffer_Front,
};

struct DN_Profiler
{
  DN_ProfilerAnchor anchors[2][DN_PROFILER_ANCHOR_BUFFER_SIZE];
  DN_U8             active_anchor_buffer;
  DN_U16            parent_zone;
};

DN_API DN_ProfilerAnchor * DN_Profiler_ReadBuffer                  ();
DN_API DN_ProfilerAnchor * DN_Profiler_WriteBuffer                 ();
#define                    DN_Profiler_BeginZone(name)             DN_Profiler_BeginZoneAtIndex(DN_STR8(name), __COUNTER__ + 1)
DN_API DN_ProfilerZone     DN_Profiler_BeginZoneAtIndex            (DN_Str8 name, DN_U16 anchor_index);
DN_API void                DN_Profiler_EndZone                     (DN_ProfilerZone zone);
DN_API DN_ProfilerAnchor * DN_Profiler_AnchorBuffer                (DN_ProfilerAnchorBuffer buffer);
DN_API void                DN_Profiler_SwapAnchorBuffer            ();
DN_API void                DN_Profiler_Dump                        (DN_U64 tsc_per_second);

#endif // !defined(DN_NO_PROFILER)
#endif // DN_CORE_DEBUG_H
