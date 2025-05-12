#if !defined(DN_HELPERS_H)
#define DN_HELPERS_H

#if !defined(DN_BASE_H)
  #error dn_base_inc.h must be included before this
#endif

#if !defined(DN_MATH_H)
  #error dn_math.h must be included before this
#endif

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
//   dn_helpers.h -- Helper functions/data structures
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_PCG32 //////////////////////////////////////////////////////////////////////////////////
struct DN_PCG32 { uint64_t state; };

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: DN_JSONBuilder ////////////////////////////////////////////////////////////////////////////
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

// NOTE: DN_BinarySearch ///////////////////////////////////////////////////////////////////////////
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
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ArrayCountU(array), 4, DN_BinarySearchType_LowerBound);
    //   printf("%zu\n", result.index); // Prints index '4'

    DN_BinarySearchType_LowerBound,

    // Index of the first element in the array that is `element > find`. If no such
    // item is found or the array is empty, then, the index is set to the array
    // size and found is set to `false`.
    //
    // For example:
    //   int array[] = {0, 1, 2, 3, 4, 5};
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ArrayCountU(array), 4, DN_BinarySearchType_UpperBound);
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

// NOTE: Misc //////////////////////////////////////////////////////////////////////////////////////
// NOTE: DN_JobQueue ///////////////////////////////////////////////////////////////////////////////
typedef void(DN_JobQueueFunc)(DN_OSThread *thread, void *user_context);
struct DN_Job
{
  DN_JobQueueFunc *func;         // The function to invoke for the job
  void            *user_context; // Pointer user can set to use in their `job_func`
  uint64_t         elapsed_tsc;
  uint16_t         user_tag;                // Arbitrary value the user can set to identiy the type of `user_context` this job has
  bool             add_to_completion_queue; // When true, on job completion, job must be dequeued from the completion queue via `GetFinishedJobs`
};

#if !defined(DN_JOB_QUEUE_SPMC_SIZE)
  #define DN_JOB_QUEUE_SPMC_SIZE 128
#endif

struct DN_JobQueueSPMC
{
  DN_OSMutex     mutex;
  DN_OSSemaphore thread_wait_for_job_semaphore;
  DN_OSSemaphore wait_for_completion_semaphore;
  DN_U32         threads_waiting_for_completion;

  DN_Job jobs[DN_JOB_QUEUE_SPMC_SIZE];
  DN_B32 quit;
  DN_U32 quit_exit_code;
  DN_U32 volatile read_index;
  DN_U32 volatile finish_index;
  DN_U32 volatile write_index;

  DN_OSSemaphore complete_queue_write_semaphore;
  DN_Job         complete_queue[DN_JOB_QUEUE_SPMC_SIZE];
  DN_U32 volatile complete_read_index;
  DN_U32 volatile complete_write_index;
};

// NOTE: DN_PCG32 //////////////////////////////////////////////////////////////////////////////////
DN_API DN_PCG32            DN_PCG32_Init                          (uint64_t seed);
DN_API uint32_t            DN_PCG32_Next                          (DN_PCG32 *rng);
DN_API uint64_t            DN_PCG32_Next64                        (DN_PCG32 *rng);
DN_API uint32_t            DN_PCG32_Range                         (DN_PCG32 *rng, uint32_t low, uint32_t high);
DN_API DN_F32              DN_PCG32_NextF32                       (DN_PCG32 *rng);
DN_API DN_F64              DN_PCG32_NextF64                       (DN_PCG32 *rng);
DN_API void                DN_PCG32_Advance                       (DN_PCG32 *rng, uint64_t delta);

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: DN_JSONBuilder ////////////////////////////////////////////////////////////////////////////
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

// NOTE: DN_BinarySearch ///////////////////////////////////////////////////////////////////////////
template <typename T> bool                  DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> DN_BinarySearchResult DN_BinarySearch                (T const *array,
                                                                              DN_USize array_size,
                                                                              T const &find,
                                                                              DN_BinarySearchType type = DN_BinarySearchType_Match,
                                                                              DN_BinarySearchLessThanProc<T> less_than = DN_BinarySearch_DefaultLessThan);

// NOTE: DN_QSort //////////////////////////////////////////////////////////////////////////////////
template <typename T> bool DN_QSort_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> void DN_QSort                (T *array,
                                                     DN_USize array_size,
                                                     void *user_context,
                                                     DN_QSortLessThanProc<T> less_than = DN_QSort_DefaultLessThan);

// NOTE: DN_JobQueue ///////////////////////////////////////////////////////////////////////////////
DN_API DN_JobQueueSPMC     DN_OS_JobQueueSPMCInit                  ();
DN_API bool                DN_OS_JobQueueSPMCCanAdd                (DN_JobQueueSPMC const *queue, uint32_t count);
DN_API bool                DN_OS_JobQueueSPMCAddArray              (DN_JobQueueSPMC *queue, DN_Job *jobs, uint32_t count);
DN_API bool                DN_OS_JobQueueSPMCAdd                   (DN_JobQueueSPMC *queue, DN_Job job);
DN_API void                DN_OS_JobQueueSPMCWaitForCompletion     (DN_JobQueueSPMC *queue);
DN_API int32_t             DN_OS_JobQueueSPMCThread                (DN_OSThread *thread);
DN_API DN_USize            DN_OS_JobQueueSPMCGetFinishedJobs       (DN_JobQueueSPMC *queue, DN_Job *jobs, DN_USize jobs_size);

// NOTE: DN_BinarySearch ///////////////////////////////////////////////////////////////////////////
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

// NOTE: DN_QSort //////////////////////////////////////////////////////////////////////////////////
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
    DN_Swap(array[last_index], array[pivot_index]);
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
            DN_Swap(array[partition_index], array[index]);
            partition_index++;
        }
    }

    // Move pivot to right of partition
    // 4, 5, 2, 3, |6, 8, ^7*
    DN_Swap(array[partition_index], array[pivot_index]);
    DN_QSort(array, partition_index, user_context, less_than);

    // Skip the value at partion index since that is guaranteed to be sorted.
    // 4, 5, 2, 3, (x), 8, 7
    DN_USize one_after_partition_index = partition_index + 1;
    DN_QSort(array + one_after_partition_index, (array_size - one_after_partition_index), user_context, less_than);
}

#endif // !defined(DN_HELPERS_H)
