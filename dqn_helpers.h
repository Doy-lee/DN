#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\ $$$$$$$$\ $$\       $$$$$$$\  $$$$$$$$\ $$$$$$$\   $$$$$$\
//   $$ |  $$ |$$  _____|$$ |      $$  __$$\ $$  _____|$$  __$$\ $$  __$$\
//   $$ |  $$ |$$ |      $$ |      $$ |  $$ |$$ |      $$ |  $$ |$$ /  \__|
//   $$$$$$$$ |$$$$$\    $$ |      $$$$$$$  |$$$$$\    $$$$$$$  |\$$$$$$\
//   $$  __$$ |$$  __|   $$ |      $$  ____/ $$  __|   $$  __$$<  \____$$\
//   $$ |  $$ |$$ |      $$ |      $$ |      $$ |      $$ |  $$ |$$\   $$ |
//   $$ |  $$ |$$$$$$$$\ $$$$$$$$\ $$ |      $$$$$$$$\ $$ |  $$ |\$$$$$$  |
//   \__|  \__|\________|\________|\__|      \________|\__|  \__| \______/
//
//   dqn_helpers.h -- Helper functions/data structures
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$PCG3] DN_PCG32        --                 -- RNG from the PCG family
// [$JSON] DN_JSONBuilder  -- DN_JSON_BUILDER -- Construct json output
// [$BHEX] DN_Bin          -- DN_BIN          -- Binary <-> hex helpers
// [$BSEA] DN_BinarySearch --                 -- Binary search
// [$BITS] DN_Bit          --                 -- Bitset manipulation
// [$SAFE] DN_Safe         --                 -- Safe arithmetic, casts, asserts
// [$MISC] Misc            --                 -- Uncategorised helper functions
// [$DLIB] DN_Library      --                 -- Globally shared runtime data for this library
// [$PROF] DN_Profiler     -- DN_PROFILER     -- Profiler that measures using a timestamp counter
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$PCGX] DN_PCG32 /////////////////////////////////////////////////////////////////////////
struct DN_PCG32 { uint64_t state; };

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: [$JSON] DN_JSONBuilder ///////////////////////////////////////////////////////////////////
enum DN_JSONBuilderItem
{
    DN_JSONBuilderItem_Empty,
    DN_JSONBuilderItem_OpenContainer,
    DN_JSONBuilderItem_CloseContainer,
    DN_JSONBuilderItem_KeyValue,
};

struct DN_JSONBuilder
{
    bool               use_stdout;        // When set, ignore the string builder and dump immediately to stdout
    DN_Str8Builder     string_builder;    // (Internal)
    int                indent_level;      // (Internal)
    int                spaces_per_indent; // The number of spaces per indent level
    DN_JSONBuilderItem last_item;
};
#endif // !defined(DN_NO_JSON_BUIDLER)

// NOTE: [$BSEA] DN_BinarySearch //////////////////////////////////////////////////////////////////
template <typename T>
using DN_BinarySearchLessThanProc = bool(T const &lhs, T const &rhs);

template <typename T>
bool DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);

enum DN_BinarySearchType
{
    // Index of the match. If no match is found, found is set to false and the
    // index is set to the index where the match should be inserted/exist, if
    // it were in the array
    DN_BinarySearchType_Match,

    // Index of the first element in the array that is `element >= find`. If no such
    // item is found or the array is empty, then, the index is set to the array
    // size and found is set to `false`.
    //
    // For example:
    //   int array[] = {0, 1, 2, 3, 4, 5};
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ARRAY_UCOUNT(array), 4, DN_BinarySearchType_LowerBound);
    //   printf("%zu\n", result.index); // Prints index '4'

    DN_BinarySearchType_LowerBound,

    // Index of the first element in the array that is `element > find`. If no such
    // item is found or the array is empty, then, the index is set to the array
    // size and found is set to `false`.
    //
    // For example:
    //   int array[] = {0, 1, 2, 3, 4, 5};
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ARRAY_UCOUNT(array), 4, DN_BinarySearchType_UpperBound);
    //   printf("%zu\n", result.index); // Prints index '5'

    DN_BinarySearchType_UpperBound,
};

struct DN_BinarySearchResult
{
    bool      found;
    DN_USize index;
};

template <typename T>
using DN_QSortLessThanProc = bool(T const &a, T const &b, void *user_context);

// NOTE: [$MISC] Misc //////////////////////////////////////////////////////////////////////////////
struct DN_U64Str8
{
    char    data[27+1]; // NOTE(dn): 27 is the maximum size of uint64_t including a separator
    uint8_t size;
};

enum DN_U64ByteSizeType
{
    DN_U64ByteSizeType_B,
    DN_U64ByteSizeType_KiB,
    DN_U64ByteSizeType_MiB,
    DN_U64ByteSizeType_GiB,
    DN_U64ByteSizeType_TiB,
    DN_U64ByteSizeType_Count,
    DN_U64ByteSizeType_Auto,
};

struct DN_U64ByteSize
{
    DN_U64ByteSizeType type;
    DN_Str8            suffix; // "KiB", "MiB", "GiB" .. e.t.c
    DN_F64             bytes;
};

typedef uint32_t DN_U64AgeUnit;
enum DN_U64AgeUnit_
{
    DN_U64AgeUnit_Sec  = 1 << 0,
    DN_U64AgeUnit_Min  = 1 << 1,
    DN_U64AgeUnit_Hr   = 1 << 2,
    DN_U64AgeUnit_Day  = 1 << 3,
    DN_U64AgeUnit_Week = 1 << 4,
    DN_U64AgeUnit_Year = 1 << 5,
    DN_U64AgeUnit_HMS  = DN_U64AgeUnit_Sec | DN_U64AgeUnit_Min | DN_U64AgeUnit_Hr,
    DN_U64AgeUnit_All  = DN_U64AgeUnit_HMS | DN_U64AgeUnit_Day | DN_U64AgeUnit_Week | DN_U64AgeUnit_Year,
};

struct DN_U64HexStr8
{
    char    data[2 /*0x*/ + 16 /*hex*/ + 1 /*null-terminator*/];
    uint8_t size;
};

typedef uint32_t DN_U64HexStr8Flags;
enum DN_U64HexStr8Flags_
{
    DN_HexU64Str8Flags_Nil         = 0,
    DN_HexU64Str8Flags_0xPrefix     = 1 << 0, /// Add the '0x' prefix from the string
    DN_HexU64Str8Flags_UppercaseHex = 1 << 1, /// Use uppercase ascii characters for hex
};

#if !defined(DN_NO_PROFILER)
// NOTE: [$PROF] DN_Profiler //////////////////////////////////////////////////////////////////////
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
    uint64_t tsc_inclusive;
    uint64_t tsc_exclusive;
    uint16_t hit_count;
    DN_Str8 name;
};

struct DN_ProfilerZone
{
    uint16_t anchor_index;
    uint64_t begin_tsc;
    uint16_t parent_zone;
    uint64_t elapsed_tsc_at_zone_start;
};

#if defined(__cplusplus)
struct DN_ProfilerZoneScope
{
    DN_ProfilerZoneScope(DN_Str8 name, uint16_t anchor_index);
    ~DN_ProfilerZoneScope();
    DN_ProfilerZone zone;
};
#define DN_Profiler_ZoneScopeAtIndex(name, anchor_index) auto DN_UNIQUE_NAME(profile_zone_) = DN_ProfilerZoneScope(DN_STR8(name), anchor_index)
#define DN_Profiler_ZoneScope(name) DN_Profiler_ZoneScopeAtIndex(name, __COUNTER__ + 1)
#endif

#define DN_Profiler_ZoneBlockIndex(name, index) \
    for (DN_ProfilerZone DN_UNIQUE_NAME(profile_zone__) = DN_Profiler_BeginZoneAtIndex(name, index), DN_UNIQUE_NAME(dummy__) = {}; \
         DN_UNIQUE_NAME(dummy__).begin_tsc == 0; \
         DN_Profiler_EndZone(DN_UNIQUE_NAME(profile_zone__)), DN_UNIQUE_NAME(dummy__).begin_tsc = 1)

#define DN_Profiler_ZoneBlock(name) DN_Profiler_ZoneBlockIndex(DN_STR8(name), __COUNTER__ + 1)

enum DN_ProfilerAnchorBuffer
{
    DN_ProfilerAnchorBuffer_Back,
    DN_ProfilerAnchorBuffer_Front,
};

struct DN_Profiler
{
    DN_ProfilerAnchor anchors[2][DN_PROFILER_ANCHOR_BUFFER_SIZE];
    uint8_t           active_anchor_buffer;
    uint16_t          parent_zone;
};
#endif // !defined(DN_NO_PROFILER)

// NOTE: [$JOBQ] DN_JobQueue ///////////////////////////////////////////////////////////////////////
typedef void (DN_JobQueueFunc)(DN_OSThread *thread, void *user_context);
struct DN_Job
{
    DN_JobQueueFunc *func;                    // The function to invoke for the job
    void            *user_context;            // Pointer user can set to use in their `job_func`
    uint64_t         elapsed_tsc;
    uint16_t         user_tag;                // Arbitrary value the user can set to identiy the type of `user_context` this job has
    bool             add_to_completion_queue; // When true, on job completion, job must be dequeued from the completion queue via `GetFinishedJobs`
};

#if !defined(DN_JOB_QUEUE_SPMC_SIZE)
    #define DN_JOB_QUEUE_SPMC_SIZE 128
#endif

struct DN_JobQueueSPMC
{
    DN_OSMutex      mutex;
    DN_OSSemaphore  thread_wait_for_job_semaphore;
    DN_OSSemaphore  wait_for_completion_semaphore;
    DN_U32          threads_waiting_for_completion;

    DN_Job          jobs[DN_JOB_QUEUE_SPMC_SIZE];
    DN_B32          quit;
    DN_U32          quit_exit_code;
    DN_U32 volatile read_index;
    DN_U32 volatile finish_index;
    DN_U32 volatile write_index;

    DN_OSSemaphore  complete_queue_write_semaphore;
    DN_Job          complete_queue[DN_JOB_QUEUE_SPMC_SIZE];
    DN_U32 volatile complete_read_index;
    DN_U32 volatile complete_write_index;
};

// NOTE: [$CORE] DN_Core //////////////////////////////////////////////////////////////////////////
// Book-keeping data for the library and allow customisation of certain features
// provided.
struct DN_Core
{
    bool                    init;                     // True if the library has been initialised via `DN_Library_Init`
    DN_OSMutex              init_mutex;
    DN_Str8                 exe_dir;                  // The directory of the current executable
    DN_Arena                arena;
    DN_Pool                 pool;
    DN_ArenaCatalog         arena_catalog;
    bool                    slow_verification_checks; // Enable expensive library verification checks
    DN_CPUReport            cpu_report;
    DN_TLS                  tls;                      // Thread local storage state for the main thread.

    // NOTE: Logging ///////////////////////////////////////////////////////////////////////////////
    DN_LogProc *            log_callback;             // Set this pointer to override the logging routine
    void *                  log_user_data;            // User pointer passed into 'log_callback'
    bool                    log_to_file;              // Output logs to file as well as standard out
    DN_OSFile               log_file;                 // TODO(dn): Hmmm, how should we do this... ?
    DN_TicketMutex          log_file_mutex;           // Is locked when instantiating the log_file for the first time
    bool                    log_no_colour;            // Disable colours in the logging output

    // NOTE: Memory ////////////////////////////////////////////////////////////////////////////////
    // Total OS mem allocs in lifetime of program (e.g. malloc, VirtualAlloc, HeapAlloc ...). This
    // only includes allocations routed through the library such as the growing nature of arenas or
    // using the memory allocation routines in the library like DN_OS_MemCommit and so forth.
    uint64_t                mem_allocs_total;
    uint64_t                mem_allocs_frame;         // Total OS mem allocs since the last 'DN_Core_FrameBegin' was invoked

    // NOTE: Leak Tracing //////////////////////////////////////////////////////////////////////////
    #if defined(DN_LEAK_TRACKING)
    DN_DSMap<DN_DebugAlloc> alloc_table;
    DN_TicketMutex          alloc_table_mutex;
    DN_Arena                alloc_table_arena;
    #endif

    // NOTE: Win32 /////////////////////////////////////////////////////////////////////////////////
    #if defined(DN_OS_WIN32)
    LARGE_INTEGER           win32_qpc_frequency;
    DN_TicketMutex          win32_bcrypt_rng_mutex;
    void *                  win32_bcrypt_rng_handle;
    bool                    win32_sym_initialised;
    #endif

    // NOTE: OS ////////////////////////////////////////////////////////////////////////////////////
    uint32_t                os_page_size;
    uint32_t                os_alloc_granularity;

    // NOTE: Profiler //////////////////////////////////////////////////////////////////////////////
    #if !defined(DN_NO_PROFILER)
    DN_Profiler *           profiler;
    DN_Profiler             profiler_default_instance;
    #endif
};

enum DN_CoreOnInit
{
    DN_CoreOnInit_Nil            = 0,
    DN_CoreOnInit_LogLibFeatures = 1 << 0,
    DN_CoreOnInit_LogCPUFeatures = 1 << 1,
    DN_CoreOnInit_LogAllFeatures = DN_CoreOnInit_LogLibFeatures | DN_CoreOnInit_LogCPUFeatures,
};

// NOTE: [$PCGX] DN_PCG32 /////////////////////////////////////////////////////////////////////////
DN_API DN_PCG32            DN_PCG32_Init                          (uint64_t seed);
DN_API uint32_t            DN_PCG32_Next                          (DN_PCG32 *rng);
DN_API uint64_t            DN_PCG32_Next64                        (DN_PCG32 *rng);
DN_API uint32_t            DN_PCG32_Range                         (DN_PCG32 *rng, uint32_t low, uint32_t high);
DN_API DN_F32              DN_PCG32_NextF32                       (DN_PCG32 *rng);
DN_API DN_F64              DN_PCG32_NextF64                       (DN_PCG32 *rng);
DN_API void                DN_PCG32_Advance                       (DN_PCG32 *rng, uint64_t delta);

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: [$JSON] DN_JSONBuilder ///////////////////////////////////////////////////////////////////
#define DN_JSONBuilder_Object(builder)                  \
    DN_DEFER_LOOP(DN_JSONBuilder_ObjectBegin(builder), \
                   DN_JSONBuilder_ObjectEnd(builder))

#define DN_JSONBuilder_ObjectNamed(builder, name)                  \
    DN_DEFER_LOOP(DN_JSONBuilder_ObjectBeginNamed(builder, name), \
                   DN_JSONBuilder_ObjectEnd(builder))

#define DN_JSONBuilder_Array(builder)                  \
    DN_DEFER_LOOP(DN_JSONBuilder_ArrayBegin(builder), \
                   DN_JSONBuilder_ArrayEnd(builder))

#define DN_JSONBuilder_ArrayNamed(builder, name)                  \
    DN_DEFER_LOOP(DN_JSONBuilder_ArrayBeginNamed(builder, name), \
                   DN_JSONBuilder_ArrayEnd(builder))


DN_API DN_JSONBuilder      DN_JSONBuilder_Init                    (DN_Arena *arena, int spaces_per_indent);
DN_API DN_Str8             DN_JSONBuilder_Build                   (DN_JSONBuilder const *builder, DN_Arena *arena);
DN_API void                DN_JSONBuilder_KeyValue                (DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value);
DN_API void                DN_JSONBuilder_KeyValueF               (DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, ...);
DN_API void                DN_JSONBuilder_ObjectBeginNamed        (DN_JSONBuilder *builder, DN_Str8 name);
DN_API void                DN_JSONBuilder_ObjectEnd               (DN_JSONBuilder *builder);
DN_API void                DN_JSONBuilder_ArrayBeginNamed         (DN_JSONBuilder *builder, DN_Str8 name);
DN_API void                DN_JSONBuilder_ArrayEnd                (DN_JSONBuilder *builder);
DN_API void                DN_JSONBuilder_Str8Named               (DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value);
DN_API void                DN_JSONBuilder_LiteralNamed            (DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value);
DN_API void                DN_JSONBuilder_U64Named                (DN_JSONBuilder *builder, DN_Str8 key, uint64_t value);
DN_API void                DN_JSONBuilder_I64Named                (DN_JSONBuilder *builder, DN_Str8 key, int64_t value);
DN_API void                DN_JSONBuilder_F64Named                (DN_JSONBuilder *builder, DN_Str8 key, double value, int decimal_places);
DN_API void                DN_JSONBuilder_BoolNamed               (DN_JSONBuilder *builder, DN_Str8 key, bool value);

#define                    DN_JSONBuilder_ObjectBegin(builder)    DN_JSONBuilder_ObjectBeginNamed(builder, DN_STR8(""))
#define                    DN_JSONBuilder_ArrayBegin(builder)     DN_JSONBuilder_ArrayBeginNamed(builder, DN_STR8(""))
#define                    DN_JSONBuilder_Str8(builder, value)    DN_JSONBuilder_Str8Named(builder, DN_STR8(""), value)
#define                    DN_JSONBuilder_Literal(builder, value) DN_JSONBuilder_LiteralNamed(builder, DN_STR8(""), value)
#define                    DN_JSONBuilder_U64(builder, value)     DN_JSONBuilder_U64Named(builder, DN_STR8(""), value)
#define                    DN_JSONBuilder_I64(builder, value)     DN_JSONBuilder_I64Named(builder, DN_STR8(""), value)
#define                    DN_JSONBuilder_F64(builder, value)     DN_JSONBuilder_F64Named(builder, DN_STR8(""), value)
#define                    DN_JSONBuilder_Bool(builder, value)    DN_JSONBuilder_BoolNamed(builder, DN_STR8(""), value)
#endif // !defined(DN_NO_JSON_BUILDER)

// NOTE: [$BSEA] DN_BinarySearch //////////////////////////////////////////////////////////////////
template <typename T> bool                   DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> DN_BinarySearchResult DN_BinarySearch                (T const *array,
                                                                              DN_USize array_size,
                                                                              T const &find,
                                                                              DN_BinarySearchType type = DN_BinarySearchType_Match,
                                                                              DN_BinarySearchLessThanProc<T> less_than = DN_BinarySearch_DefaultLessThan);

// NOTE: [$QSOR] DN_QSort /////////////////////////////////////////////////////////////////////////
template <typename T> bool DN_QSort_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> void DN_QSort                (T *array,
                                                     DN_USize array_size,
                                                     void *user_context,
                                                     DN_QSortLessThanProc<T> less_than = DN_QSort_DefaultLessThan);

// NOTE: [$BITS] DN_Bit ///////////////////////////////////////////////////////////////////////////
DN_API void                DN_Bit_UnsetInplace                     (DN_USize *flags, DN_USize bitfield);
DN_API void                DN_Bit_SetInplace                       (DN_USize *flags, DN_USize bitfield);
DN_API bool                DN_Bit_IsSet                            (DN_USize bits, DN_USize bits_to_set);
DN_API bool                DN_Bit_IsNotSet                         (DN_USize bits, DN_USize bits_to_check);
#define                    DN_Bit_ClearNextLSB(value)              (value) & ((value) - 1)

// NOTE: [$SAFE] DN_Safe //////////////////////////////////////////////////////////////////////////
DN_API int64_t             DN_Safe_AddI64                          (int64_t a,  int64_t b);
DN_API int64_t             DN_Safe_MulI64                          (int64_t a,  int64_t b);

DN_API uint64_t            DN_Safe_AddU64                          (uint64_t a, uint64_t b);
DN_API uint64_t            DN_Safe_MulU64                          (uint64_t a, uint64_t b);

DN_API uint64_t            DN_Safe_SubU64                          (uint64_t a, uint64_t b);
DN_API uint32_t            DN_Safe_SubU32                          (uint32_t a, uint32_t b);

DN_API int                 DN_Safe_SaturateCastUSizeToInt          (DN_USize val);
DN_API int8_t              DN_Safe_SaturateCastUSizeToI8           (DN_USize val);
DN_API int16_t             DN_Safe_SaturateCastUSizeToI16          (DN_USize val);
DN_API int32_t             DN_Safe_SaturateCastUSizeToI32          (DN_USize val);
DN_API int64_t             DN_Safe_SaturateCastUSizeToI64          (DN_USize val);

DN_API int                 DN_Safe_SaturateCastU64ToInt            (uint64_t val);
DN_API int8_t              DN_Safe_SaturateCastU8ToI8              (uint64_t val);
DN_API int16_t             DN_Safe_SaturateCastU16ToI16            (uint64_t val);
DN_API int32_t             DN_Safe_SaturateCastU32ToI32            (uint64_t val);
DN_API int64_t             DN_Safe_SaturateCastU64ToI64            (uint64_t val);
DN_API unsigned int        DN_Safe_SaturateCastU64ToUInt           (uint64_t val);
DN_API uint8_t             DN_Safe_SaturateCastU64ToU8             (uint64_t val);
DN_API uint16_t            DN_Safe_SaturateCastU64ToU16            (uint64_t val);
DN_API uint32_t            DN_Safe_SaturateCastU64ToU32            (uint64_t val);

DN_API uint8_t             DN_Safe_SaturateCastUSizeToU8           (DN_USize val);
DN_API uint16_t            DN_Safe_SaturateCastUSizeToU16          (DN_USize val);
DN_API uint32_t            DN_Safe_SaturateCastUSizeToU32          (DN_USize val);
DN_API uint64_t            DN_Safe_SaturateCastUSizeToU64          (DN_USize val);

DN_API int                 DN_Safe_SaturateCastISizeToInt          (DN_ISize val);
DN_API int8_t              DN_Safe_SaturateCastISizeToI8           (DN_ISize val);
DN_API int16_t             DN_Safe_SaturateCastISizeToI16          (DN_ISize val);
DN_API int32_t             DN_Safe_SaturateCastISizeToI32          (DN_ISize val);
DN_API int64_t             DN_Safe_SaturateCastISizeToI64          (DN_ISize val);

DN_API unsigned int        DN_Safe_SaturateCastISizeToUInt         (DN_ISize val);
DN_API uint8_t             DN_Safe_SaturateCastISizeToU8           (DN_ISize val);
DN_API uint16_t            DN_Safe_SaturateCastISizeToU16          (DN_ISize val);
DN_API uint32_t            DN_Safe_SaturateCastISizeToU32          (DN_ISize val);
DN_API uint64_t            DN_Safe_SaturateCastISizeToU64          (DN_ISize val);

DN_API DN_ISize            DN_Safe_SaturateCastI64ToISize          (int64_t val);
DN_API int8_t              DN_Safe_SaturateCastI64ToI8             (int64_t val);
DN_API int16_t             DN_Safe_SaturateCastI64ToI16            (int64_t val);
DN_API int32_t             DN_Safe_SaturateCastI64ToI32            (int64_t val);

DN_API unsigned int        DN_Safe_SaturateCastI64ToUInt           (int64_t val);
DN_API DN_ISize            DN_Safe_SaturateCastI64ToUSize          (int64_t val);
DN_API uint8_t             DN_Safe_SaturateCastI64ToU8             (int64_t val);
DN_API uint16_t            DN_Safe_SaturateCastI64ToU16            (int64_t val);
DN_API uint32_t            DN_Safe_SaturateCastI64ToU32            (int64_t val);
DN_API uint64_t            DN_Safe_SaturateCastI64ToU64            (int64_t val);

DN_API int8_t              DN_Safe_SaturateCastIntToI8             (int val);
DN_API int16_t             DN_Safe_SaturateCastIntToI16            (int val);
DN_API uint8_t             DN_Safe_SaturateCastIntToU8             (int val);
DN_API uint16_t            DN_Safe_SaturateCastIntToU16            (int val);
DN_API uint32_t            DN_Safe_SaturateCastIntToU32            (int val);
DN_API uint64_t            DN_Safe_SaturateCastIntToU64            (int val);

// NOTE: [$MISC] Misc //////////////////////////////////////////////////////////////////////////////
DN_API int                 DN_FmtBuffer3DotTruncate                (char *buffer, int size, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_U64Str8          DN_U64ToStr8                            (uint64_t val, char separator);
DN_API DN_U64ByteSize      DN_U64ToByteSize                        (uint64_t bytes, DN_U64ByteSizeType type);
DN_API DN_Str8             DN_U64ToByteSizeStr8                    (DN_Arena *arena, uint64_t bytes, DN_U64ByteSizeType desired_type);
DN_API DN_Str8             DN_U64ByteSizeTypeString                (DN_U64ByteSizeType type);
DN_API DN_Str8             DN_U64ToAge                             (DN_Arena *arena, uint64_t age_s, DN_U64AgeUnit unit);
DN_API DN_Str8             DN_F64ToAge                             (DN_Arena *arena, DN_F64 age_s, DN_U64AgeUnit unit);

DN_API uint64_t            DN_HexToU64                             (DN_Str8 hex);
DN_API DN_Str8             DN_U64ToHex                             (DN_Arena *arena, uint64_t number, DN_U64HexStr8Flags flags);
DN_API DN_U64HexStr8       DN_U64ToHexStr8                         (uint64_t number, uint32_t flags);

DN_API bool                DN_BytesToHexPtr                        (void const *src, DN_USize src_size, char *dest);
DN_API DN_Str8             DN_BytesToHex                           (DN_Arena *arena, void const *src, DN_USize size);
#define                    DN_BytesToHex_TLS(...)                  DN_BytesToHex(DN_TLS_TopArena(), __VA_ARGS__)

DN_API DN_USize            DN_HexToBytesPtrUnchecked               (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_USize            DN_HexToBytesPtr                        (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_Str8             DN_HexToBytesUnchecked                  (DN_Arena *arena, DN_Str8 hex);
#define                    DN_HexToBytesUnchecked_TLS(...)         DN_HexToBytesUnchecked(DN_TLS_TopArena(), __VA_ARGS__)
DN_API DN_Str8             DN_HexToBytes                           (DN_Arena *arena, DN_Str8 hex);
#define                    DN_HexToBytes_TLS(...)                  DN_HexToBytes(DN_TLS_TopArena(), __VA_ARGS__)

// NOTE: [$PROF] DN_Profiler //////////////////////////////////////////////////////////////////////
DN_API DN_ProfilerAnchor * DN_Profiler_ReadBuffer                  ();
DN_API DN_ProfilerAnchor * DN_Profiler_WriteBuffer                 ();
#define                    DN_Profiler_BeginZone(name)             DN_Profiler_BeginZoneAtIndex(DN_STR8(name), __COUNTER__ + 1)
DN_API DN_ProfilerZone     DN_Profiler_BeginZoneAtIndex            (DN_Str8 name, uint16_t anchor_index);
DN_API void                DN_Profiler_EndZone                     (DN_ProfilerZone zone);
DN_API DN_ProfilerAnchor * DN_Profiler_AnchorBuffer                (DN_ProfilerAnchorBuffer buffer);
DN_API void                DN_Profiler_SwapAnchorBuffer            ();
DN_API void                DN_Profiler_Dump                        (uint64_t tsc_per_second);

// NOTE: [$JOBQ] DN_JobQueue ///////////////////////////////////////////////////////////////////////
DN_API DN_JobQueueSPMC     DN_OS_JobQueueSPMCInit                  ();
DN_API bool                DN_OS_JobQueueSPMCCanAdd                (DN_JobQueueSPMC const *queue, uint32_t count);
DN_API bool                DN_OS_JobQueueSPMCAddArray              (DN_JobQueueSPMC *queue, DN_Job *jobs, uint32_t count);
DN_API bool                DN_OS_JobQueueSPMCAdd                   (DN_JobQueueSPMC *queue, DN_Job job);
DN_API void                DN_OS_JobQueueSPMCWaitForCompletion     (DN_JobQueueSPMC *queue);
DN_API int32_t             DN_OS_JobQueueSPMCThread                (DN_OSThread *thread);
DN_API DN_USize            DN_OS_JobQueueSPMCGetFinishedJobs       (DN_JobQueueSPMC *queue, DN_Job *jobs, DN_USize jobs_size);

// NOTE: DN_Core ///////////////////////////////////////////////////////////////////////////////
DN_API void                DN_Core_Init                            (DN_Core *core, DN_CoreOnInit on_init);
DN_API void                DN_Core_BeginFrame                      ();
DN_API void                DN_Core_SetPointer                      (DN_Core *core);
#if !defined(DN_NO_PROFILER)
DN_API void                DN_Core_SetProfiler                     (DN_Profiler *profiler);
#endif
DN_API void                DN_Core_SetLogCallback                  (DN_LogProc *proc, void *user_data);
DN_API void                DN_Core_DumpThreadContextArenaStat      (DN_Str8 file_path);
DN_API DN_Arena *          DN_Core_AllocArenaF                     (DN_USize reserve, DN_USize commit, uint8_t arena_flags, char const *fmt, ...);
DN_API bool                DN_Core_EraseArena                      (DN_Arena *arena, DN_ArenaCatalogFreeArena free_arena);

// NOTE: [$BSEA] DN_BinarySearch //////////////////////////////////////////////////////////////////
template <typename T>
bool DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs)
{
    bool result = lhs < rhs;
    return result;
}

template <typename T>
DN_BinarySearchResult DN_BinarySearch(T const                         *array,
                                        DN_USize                        array_size,
                                        T const                         &find,
                                        DN_BinarySearchType             type,
                                        DN_BinarySearchLessThanProc<T>  less_than)
{
    DN_BinarySearchResult result = {};
    if (!array || array_size <= 0 || !less_than)
        return result;

    T const *end   = array + array_size;
    T const *first = array;
    T const *last  = end;
    while (first != last) {
        DN_USize count = last - first;
        T const *it     = first + (count / 2);

        bool advance_first = false;
        if (type == DN_BinarySearchType_UpperBound)
            advance_first = !less_than(find, it[0]);
        else
            advance_first = less_than(it[0], find);

        if (advance_first)
            first = it + 1;
        else
            last  = it;
    }

    switch (type) {
        case DN_BinarySearchType_Match: {
            result.found = first != end && !less_than(find, *first);
        } break;

        case DN_BinarySearchType_LowerBound: /*FALLTHRU*/
        case DN_BinarySearchType_UpperBound: {
            result.found = first != end;
        } break;
    }

    result.index = first - array;
    return result;
}

// NOTE: [$QSOR] DN_QSort /////////////////////////////////////////////////////////////////////////
template <typename T>
bool DN_QSort_DefaultLessThan(T const &lhs, T const &rhs, void *user_context)
{
    (void)user_context;
    bool result = lhs < rhs;
    return result;
}

template <typename T>
void DN_QSort(T *array, DN_USize array_size, void *user_context, DN_QSortLessThanProc<T> less_than)
{
    if (!array || array_size <= 1 || !less_than)
        return;

    // NOTE: Insertion Sort, under 24->32 is an optimal amount /////////////////////////////////////
    const DN_USize QSORT_THRESHOLD = 24;
    if (array_size < QSORT_THRESHOLD) {
        for (DN_USize item_to_insert_index = 1; item_to_insert_index < array_size; item_to_insert_index++) {
            for (DN_USize index = 0; index < item_to_insert_index; index++) {
                if (!less_than(array[index], array[item_to_insert_index], user_context)) {
                    T item_to_insert = array[item_to_insert_index];
                    for (DN_USize i = item_to_insert_index; i > index; i--)
                        array[i] = array[i - 1];

                    array[index] = item_to_insert;
                    break;
                }
            }
        }
        return;
    }

    // NOTE: Quick sort, under 24->32 is an optimal amount /////////////////////////////////////////
    DN_USize last_index      = array_size - 1;
    DN_USize pivot_index     = array_size / 2;
    DN_USize partition_index = 0;
    DN_USize start_index     = 0;

    // Swap pivot with last index, so pivot is always at the end of the array.
    // This makes logic much simpler.
    DN_SWAP(array[last_index], array[pivot_index]);
    pivot_index = last_index;

    // 4^, 8, 7, 5, 2, 3, 6
    if (less_than(array[start_index], array[pivot_index], user_context))
        partition_index++;
    start_index++;

    // 4, |8, 7, 5^, 2, 3, 6*
    // 4, 5, |7, 8, 2^, 3, 6*
    // 4, 5, 2, |8, 7, ^3, 6*
    // 4, 5, 2, 3, |7, 8, ^6*
    for (DN_USize index = start_index; index < last_index; index++) {
        if (less_than(array[index], array[pivot_index], user_context)) {
            DN_SWAP(array[partition_index], array[index]);
            partition_index++;
        }
    }

    // Move pivot to right of partition
    // 4, 5, 2, 3, |6, 8, ^7*
    DN_SWAP(array[partition_index], array[pivot_index]);
    DN_QSort(array, partition_index, user_context, less_than);

    // Skip the value at partion index since that is guaranteed to be sorted.
    // 4, 5, 2, 3, (x), 8, 7
    DN_USize one_after_partition_index = partition_index + 1;
    DN_QSort(array + one_after_partition_index, (array_size - one_after_partition_index), user_context, less_than);
}
