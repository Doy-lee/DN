/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$\   $$$$$$\   $$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\ $$  __$$\ $$  __$$\
//   $$ |  $$ |$$ /  $$ |$$ /  \__|$$ /  \__|
//   $$ |  $$ |$$ |  $$ |$$ |      \$$$$$$\
//   $$ |  $$ |$$ |  $$ |$$ |       \____$$\
//   $$ |  $$ |$$ |  $$ |$$ |  $$\ $$\   $$ |
//   $$$$$$$  | $$$$$$  |\$$$$$$  |\$$$$$$  |
//   \_______/  \______/  \______/  \______/
//
//   dn_docs.cpp -- Library documentation via real code examples
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Use this file for documentation and examples of the various APIs in this
// library. Normally docs are written as inline comments in header files,
// however, these quickly go out of date as APIs change. Instead, I provide
// some example code that compiles here that serves to also document the API.
//
// The library header files then become a very minimal reference of exactly the
// function prototypes and definitions instead of massive reams of inline
// comments that visually space out the functions and hinders discoverability
// and/or conciseness of being able to learn the breadth of the APIs.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4702) // unreachable code

void DN_Docs_Demo()
{
// NOTE: Before using anything in the library, DN_Core_Init() must be
// called, for example:
#if 0
  DN_Core core = {};
  DN_Core_Init(&core, DN_CoreOnInit_Nil);
#endif

  // NOTE: DN_AtomicSetValue64 /////////////////////////////////////////////////////////////////
  // NOTE: DN_AtomicSetValue32 /////////////////////////////////////////////////////////////////
  // Atomically set the value into the target using an atomic compare and swap
  // idiom. The return value of the function is the value that was last stored
  // in the target.
  {
    uint64_t target       = 8;
    uint64_t value_to_set = 0xCAFE;
    if (DN_AtomicSetValue64(&target, value_to_set) == 8) {
      // Atomic swap was successful, e.g. the last value that this thread
      // observed was '8' which is the value we initialised with e.g. no
      // other thread has modified the value.
    }
  }

  // NOTE: DN_CVT_HexFromBytes ////////////////////////////////////////////////////////////////////////
  {
    DN_OSTLSTMem    tmem     = DN_OS_TLSTMem(nullptr);
    unsigned char bytes[2] = {0xFA, 0xCE};
    DN_Str8       hex      = DN_CVT_HexFromBytes(tmem.arena, bytes, sizeof(bytes));
    DN_Assert(hex == DN_STR8("face")); // NOTE: Guaranteed to be null-terminated
  }

  // NOTE: DN_Check /////////////////////////////////////////////////////////////////////////////
  //
  // Check the expression trapping in debug, whilst in release- trapping is
  // removed and the expression is evaluated as if it were a normal 'if' branch.
  //
  // This allows handling of the condition gracefully when compiled out but
  // traps to notify the developer in builds when it's compiled in.
  {
    bool flag = true;
    if (DN_CheckF(flag, "Flag was false!")) {
      /// This branch will execute!
    }
  }

  // NOTE: DN_CPUID /////////////////////////////////////////////////////////////////////////////
  // Execute the 'CPUID' instruction which lets you query the capabilities of
  // the current CPU.

  // NOTE: DN_DEFER
  //
  // A macro that expands to a C++ lambda that executes arbitrary code on
  // scope exit.
  {
    int x = 0;
    DN_DEFER
    {
      x = 3;
    };
    x = 1;
    // On scope exit, DN_DEFER object executes and assigns x = 3
  }

  // NOTE: DN_DSMap /////////////////////////////////////////////////////////////////////////////
  //
  // A hash table configured using the presets recommended by Demitri Spanos
  // from the Handmade Network (HMN),
  //
  // - power of two capacity
  // - grow by 2x on load >= 75%
  // - open-addressing with linear probing
  // - separate large values (esp. variable length values) into a separate table
  // - use a well-known hash function: MurmurHash3 (or xxhash, city, spooky ...)
  // - chain-repair on delete (rehash items in the probe chain after delete)
  // - shrink by 1/2 on load < 25% (suggested by Martins Mmozeiko of HMN)
  //
  // Source: discord.com/channels/239737791225790464/600063880533770251/941835678424129597
  //
  // This hash-table stores slots (values) separate from the hash mapping.
  // Hashes are mapped to slots using the hash-to-slot array which is an array
  // of slot indexes. This array intentionally only stores indexes to maximise
  // usage of the cache line. Linear probing on collision will only cost a
  // couple of cycles to fetch from L1 cache the next slot index to attempt.
  //
  // The slots array stores values contiguously, non-sorted allowing iteration
  // of the map. On element erase, the last element is swapped into the
  // deleted element causing the non-sorted property of this table.
  //
  // The 0th slot (DN_DS_MAP_SENTINEL_SLOT) in the slots array is reserved
  // for a sentinel value, e.g. all zeros value. After map initialisation the
  // 'occupied' value of the array will be set to 1 to exclude the sentinel
  // from the capacity of the table. Skip the first value if you are iterating
  // the hash table!
  //
  // This hash-table accept either a U64 or a buffer (ptr + len) as the key.
  // In practice this covers a majority of use cases (with string, buffer and
  // number keys). It also allows us to minimise our C++ templates to only
  // require 1 variable which is the Value part of the hash-table simplifying
  // interface complexity and cruft brought by C++.
  //
  // Keys are value-copied into the hash-table. If the key uses a pointer to a
  // buffer, this buffer must be valid throughout the lifetime of the hash
  // table!
  {
    // NOTE: DN_DSMap_Init   //////////////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_Deinit //////////////////////////////////////////////////////////////////
    //
    // Initialise a hash table where the table size *must* be a
    // power-of-two, otherwise an assert will be triggered. If
    // initialisation fails (e.g. memory allocation failure) the table is
    // returned zero-initialised where a call to 'IsValid' will return
    // false.
    //
    // The map takes ownership of the arena. This means in practice that if the
    // map needs to resize (e.g. because the load threshold of the table is
    // exceeded), the arena associated with it will be released and the memory
    // will be reallocated with the larger capacity and reassigned to the arena.
    //
    // In simple terms, when the map resizes it invalidates all memory that was
    // previously allocated with the given arena!
    //
    // A 'Deinit' of the map will similarly deallocate the passed in arena (as
    // the map takes ownership of the arena).
    DN_Arena      arena = DN_Arena_FromVMem(0, 0, DN_ArenaFlags_Nil);
    DN_DSMap<int> map   = DN_DSMap_Init<int>(&arena, /*size*/ 1024, DN_DSMapFlags_Nil); // Size must be PoT!
    DN_Assert(DN_DSMap_IsValid(&map));                                                  // Valid if no initialisation failure (e.g. mem alloc failure)

    // NOTE: DN_DSMap_KeyCStringLit ///////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_KeyU64        ///////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_KeyU64NoHash  ///////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_KeyBuffer     ///////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_KeyStr8       ///////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_KeyStr8Copy   ///////////////////////////////////////////////////////////
    // Create a hash-table key where:
    //
    //   KeyCStringLit: Uses a Hash(cstring literal)
    //   KeyU64:        Uses a Hash(U64)
    //   KeyU64NoHash:  Uses a U64 (where it's truncated to 4 bytes)
    //   KeyBuffer:     Uses a Hash(ptr+len) slice of bytes
    //   KeyStr8:       Uses a Hash(string)
    //   KeyStr8Copy:   Uses a Hash(string) that is copied first using the arena
    //
    // Buffer-based keys memory must persist throughout lifetime of the map.
    // Keys are valued copied into the map, alternatively, copy the
    // key/buffer before constructing the key.
    //
    // You *can't* use the map's arena to allocate keys because on resize it
    // will deallocate then reallocate the entire arena.
    //
    // KeyU64NoHash may be useful if you have a source of data that is
    // already sufficiently uniformly distributed already (e.g. using 8
    // bytes taken from a SHA256 hash as the key) and the first 4 bytes
    // will be used verbatim.
    DN_DSMapKey key = DN_DSMap_KeyStr8(&map, DN_STR8("Sample Key"));

    // NOTE: DN_DSMap_Find ////////////////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_Make ////////////////////////////////////////////////////////////////////
    // NOTE: DN_DSMap_Set  ////////////////////////////////////////////////////////////////////
    //
    // Query or commit key-value pair to the table, where:
    //
    //   Find: does a key-lookup     on the table and returns the hash table slot's value
    //   Make: assigns the key       to the table and returns the hash table slot's value
    //   Set:  assigns the key-value to the table and returns the hash table slot's value
    //
    // A find query will set 'found' to false if it does not exist.
    //
    // For 'Make' and 'Set', 'found' can be set to 'true' if the item already
    // existed in the map prior to the call. If it's the first time the
    // key-value pair is being inserted 'found' will be set to 'false'.
    //
    // If by adding the key-value pair to the table puts the table over 75% load,
    // the table will be grown to 2x the current the size before insertion
    // completes.
    {
      DN_DSMapResult<int> set_result = DN_DSMap_Set(&map, key, 0xCAFE);
      DN_Assert(!set_result.found); // First time we are setting the key-value pair, it wasn't previously in the table
      DN_Assert(map.occupied == 2); // Sentinel + new element == 2
    }

    // Iterating elements in the array, note that index '0' is the sentinel
    // slot! You typically don't care about it!
    for (DN_USize index = 1; index < map.occupied; index++) {
      DN_DSMapSlot<int> *it       = map.slots + index;
      DN_DSMapKey        it_key   = it->key;
      int               *it_value = &it->value;
      DN_Assert(*it_value == 0xCAFE);

      DN_Assert(DN_Str8_Init(it_key.buffer_data, it_key.buffer_size) == DN_STR8("Sample Key"));
    }

    // NOTE: DN_DSMap_Erase ///////////////////////////////////////////////////////////////////
    //
    // Remove the key-value pair from the table. If by erasing the key-value
    // pair from the table puts the table under 25% load, the table will be
    // shrunk by 1/2 the current size after erasing. The table will not shrink
    // below the initial size that the table was initialised as.
    {
      bool erased = DN_DSMap_Erase(&map, key);
      DN_Assert(erased);
      DN_Assert(map.occupied == 1); // Sentinel element
    }

    DN_DSMap_Deinit(&map, DN_ZeroMem_Yes); // Deallocates the 'arena' for us!
  }

// NOTE: DN_DSMap_Hash ////////////////////////////////////////////////////////////////////////
//
// Hash the input key using the custom hash function if it's set on the map,
// otherwise uses the default hashing function (32bit Murmur3).

// NOTE: DN_DSMap_HashToSlotIndex /////////////////////////////////////////////////////////////
//
// Calculate the index into the map's 'slots' array from the given hash.

// NOTE: DN_DSMap_Resize //////////////////////////////////////////////////////////////////////
//
// Resize the table and move all elements to the new map, note that the new
// size must be a power of two. This function wil fail on memory allocation
// failure, or the requested size is smaller than the current number of
// elements in the map to resize.

// NOTE: DN_OSErrSink /////////////////////////////////////////////////////////////////////////
//
// Error sinks are a way of accumulating errors from API calls related or
// unrelated into 1 unified error handling pattern. The implemenation of a
// sink requires 2 fundamental design constraints on the APIs supporting
// this pattern.
//
// 1. Pipelining of errors
//    Errors emitted over the course of several API calls are accumulated
//    into a sink which save the error code and message of the first error
//    encountered and can be checked later.
//
// 2. Error proof APIs
//    Functions that produce errors must return objects/handles that are
//    marked to trigger no-ops used in subsequent functions dependent on it.
//
// Consider the following example demonstrating a conventional error
// handling approach (error values by return/sentinel values) and error
// handling using error-proof and pipelining.

// (A) Conventional error checking patterns using return/sentinel values
#if 0
      DN_OSFile *file = DN_OS_FileOpen("/path/to/file", ...);
      if (file) {
          if (!DN_OS_FileWrite(file, "abc")) {
              // Error handling!
          }
          Dnq_OS_FileClose(file);
      } else {
          // Error handling!
      }
#endif

  // (B) Error handling using pipelining and and error proof APIs. APIs that
  // produce errors take in the error sink as a parameter.
  if (0) {
    DN_OSErrSink *error = DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil);
    DN_OSFile   file  = DN_OS_FileOpen(DN_STR8("/path/to/file"), DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_ReadWrite, error);
    DN_OS_FileWrite(&file, DN_STR8("abc"), error);
    DN_OS_FileClose(&file);
    if (DN_OS_ErrSinkEndAndLogErrorF(error, "Failed to write to file")) {
      // Do error handling!
    }
  }

  // Pipeling and error-proof APIs lets you write sequence of instructions and
  // defer error checking until it is convenient or necessary. Functions are
  // *guaranteed* to return an object that is usable. There are no hidden
  // exceptions to be thrown. Functions may opt to still return error values
  // by way of return values thereby *not* precluding the ability to check
  // every API call either.
  //
  // Ultimately, this error handling approach gives more flexibility on the
  // manner in how errors are handled with less code.
  //
  // Error sinks can nest begin and end statements. This will open a new scope
  // whereby the current captured error pushed onto a stack and the sink will
  // be populated by the first error encountered in that scope.

  if (0) {
    DN_OSErrSink *error = DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil);
    DN_OSFile   file  = DN_OS_FileOpen(DN_STR8("/path/to/file"), DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_ReadWrite, error);
    DN_OS_FileWrite(&file, DN_STR8("abc"), error);
    DN_OS_FileClose(&file);

    {
      // NOTE: My error sinks are thread-local, so the returned 'error' is
      // the same as the 'error' value above.
      DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil);
      DN_OS_WriteAll(DN_STR8("/path/to/another/file"), DN_STR8("123"), error);
      DN_OS_ErrSinkEndAndLogErrorF(error, "Failed to write to another file");
    }

    if (DN_OS_ErrSinkEndAndLogErrorF(error, "Failed to write to file")) {
      // Do error handling!
    }
  }

  // NOTE: DN_CVT_BytesFromHex ////////////////////////////////////////////////////////////////////////
  {
    unsigned char bytes[2];
    DN_USize      bytes_written = DN_CVT_BytesFromHexPtr(DN_STR8("0xFACE"), bytes, sizeof(bytes));
    DN_Assert(bytes_written == 2);
    DN_Assert(bytes[0] == 0xFA);
    DN_Assert(bytes[1] == 0xCE);
  }

  // NOTE: DN_JSONBuilder_Build /////////////////////////////////////////////////////////////////
  //
  // Convert the internal JSON buffer in the builder into a string.

  // NOTE: DN_JSONBuilder_KeyValue, DN_JSONBuilder_KeyValueF
  //
  // Add a JSON key value pair untyped. The value is emitted directly without
  // checking the contents of value.
  //
  // All other functions internally call into this function which is the main
  // workhorse of the builder.

  // NOTE: DN_JSON_Builder_ObjectEnd
  //
  // End a JSON object in the builder, generates internally a '}' string

  // NOTE: DN_JSON_Builder_ArrayEnd
  //
  // End a JSON array in the builder, generates internally a ']' string

  // NOTE: DN_JSONBuilder_LiteralNamed
  //
  // Add a named JSON key-value object whose value is directly written to
  // the following '"<key>": <value>' (e.g. useful for emitting the 'null'
  // value)

  // NOTE: DN_JSONBuilder_U64       /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_U64Named  /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_I64       /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_I64Named  /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_F64       /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_F64Named  /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_Bool      /////////////////////////////////////////////////////////////
  // NOTE: DN_JSONBuilder_BoolNamed /////////////////////////////////////////////////////////////
  //
  // Add the named JSON data type as a key-value object. The named variants
  // generates internally the key-value pair, e.g.
  //
  // "<name>": <value>
  //
  // And the non-named version emit just the 'value' portion

  // NOTE: DN_List_Iterate //////////////////////////////////////////////////////////////////////
  {
    DN_OSTLSTMem   tmem = DN_OS_TLSTMem(nullptr);
    DN_List<int> list = DN_List_Init<int>(/*chunk_size*/ 128);
    for (DN_ListIterator<int> it = {}; DN_List_Iterate(&list, &it, 0);) {
      int *item = it.data;
      (void)item;
    }
  }

  // NOTE: DN_LOGProc ///////////////////////////////////////////////////////////////////////////
  //
  // Function prototype of the logging interface exposed by this library. Logs
  // emitted using the DN_LOG_* family of functions are routed through this
  // routine.

  // NOTE: DN_FNV1A /////////////////////////////////////////////////////////////////////////////
#if 0
  {
    // Using the default hash as defined by DN_FNV1A32_SEED and
    // DN_FNV1A64_SEED for 32/64bit hashes respectively
    uint32_t buffer1 = 0xCAFE0000;
    uint32_t buffer2 = 0xDEAD0000;
    {
      uint64_t hash = DN_FNV1A64_Hash(&buffer1, sizeof(buffer1));
      hash          = DN_FNV1A64_Iterate(&buffer2, sizeof(buffer2), hash); // Chained hashing
      (void)hash;
    }

    // You can use a custom seed by skipping the 'Hash' call and instead
    // calling 'Iterate' immediately.
    {
      uint64_t custom_seed = 0xABCDEF12;
      uint64_t hash        = DN_FNV1A64_Iterate(&buffer1, sizeof(buffer1), custom_seed);
      hash                 = DN_FNV1A64_Iterate(&buffer2, sizeof(buffer2), hash);
      (void)hash;
    }
  }
#endif

  // NOTE: DN_FmtBuffer3DotTruncate //////////////////////////////////////////////////////////////
  {
    char buffer[8]            = {};
    int  buffer_chars_written = DN_CVT_FmtBuffer3DotTruncate(buffer, sizeof(buffer), "This string is longer than %d characters", DN_CAST(int)(sizeof(buffer) - 1));
    if (0) // Prints "This ..." which is exactly 8 characters long
      printf("%.*s", buffer_chars_written, buffer);
  }

  // NOTE: DN_MurmurHash3 ///////////////////////////////////////////////////////////////////////
  // MurmurHash3 was written by Austin Appleby, and is placed in the public
  // domain. The author (Austin Appleby) hereby disclaims copyright to this source
  // code.
  //
  // Note - The x86 and x64 versions do _not_ produce the same results, as the
  // algorithms are optimized for their respective platforms. You can still
  // compile and run any of them on any platform, but your performance with the
  // non-native version will be less than optimal.

  // NOTE: DN_OS_DateUnixTime
  //
  // Produce the time elapsed since the unix epoch
  {
    uint64_t now = DN_OS_DateUnixTimeS();
    (void)now;
  }

  // NOTE: DN_OS_DirIterate /////////////////////////////////////////////////////////////////////
  //
  // Iterate the files within the passed in folder
  for (DN_OSDirIterator it = {}; DN_OS_DirIterate(DN_STR8("."), &it);) {
    // printf("%.*s\n", DN_STR_FMT(it.file_name));
  }

  // NOTE: DN_OS_FileDelete
  //
  // This function can only delete files and it can *only* delete directories
  // if it is empty otherwise this function fails.

  // NOTE: DN_OS_WriteAllSafe
  // Writes the file at the path first by appending '.tmp' to the 'path' to
  // write to. If the temporary file is written successfully then the file is
  // copied into 'path', for example:
  //
  //   path:     C:/Home/my.txt
  //   tmp_path: C:/Home/my.txt.tmp
  //
  // If 'tmp_path' is written to successfuly, the file will be copied over into
  // 'path'.
  if (0) {
    DN_OSTLSTMem  tmem  = DN_OS_TLSTMem(nullptr);
    DN_OSErrSink *error = DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil);
    DN_OS_WriteAllSafe(/*path*/ DN_STR8("C:/Home/my.txt"), /*buffer*/ DN_STR8("Hello world"), error);
    DN_OS_ErrSinkEndAndLogErrorF(error, "");
  }

  // NOTE: DN_OS_EstimateTSCPerSecond ///////////////////////////////////////////////////////////
  //
  // Estimate how many timestamp count's (TSC) there are per second. TSC
  // is evaluated by calling __rdtsc() or the equivalent on the platform. This
  // value can be used to convert TSC durations into seconds.
  //
  // The 'duration_ms_to_gauge_tsc_frequency' parameter specifies how many
  // milliseconds to spend measuring the TSC rate of the current machine.
  // 100ms is sufficient to produce a fairly accurate result with minimal
  // blocking in applications if calculated on startup..
  //
  // This may return 0 if querying the CPU timestamp counter is not supported
  // on the platform (e.g. __rdtsc() or __builtin_readcyclecounter() returns 0).

  // NOTE: DN_OS_EXEDir /////////////////////////////////////////////////////////////////////////
  //
  // Retrieve the executable directory without the trailing '/' or ('\' for
  // windows). If this fails an empty string is returned.

  // NOTE: DN_OS_PerfCounterFrequency ///////////////////////////////////////////////////////////
  //
  // Get the number of ticks in the performance counter per second for the
  // operating system you're running on. This value can be used to calculate
  // duration from OS performance counter ticks.

  // NOTE: DN_OS_Path* //////////////////////////////////////////////////////////////////////////
  // Construct paths ensuring the native OS path separators are used in the
  // string. In 99% of cases you can use 'PathConvertF' which converts the
  // given path in one shot ensuring native path separators in the string.
  //
  //   path:      C:\Home/My/Folder
  //   converted: C:/Home/My/Folder (On Unix)
  //              C:\Home\My\Folder (On Windows)
  //
  // If you need to construct a path dynamically you can use the builder-esque
  // interface to build a path's step-by-step using the 'OSPath' data structure.
  // With this API you can append paths piece-meal to build the path after all
  // pieces are appended.
  //
  // You may append a singular or nested path to the builder. In the builder,
  // the string is scanned and separated into path separated chunks and stored
  // in the builder, e.g. these are all valid to pass into 'PathAdd',
  // 'PathAddRef' ... e.t.c
  //
  //   "path/to/your/desired/folder" is valid
  //   "path"                        is valid
  //   "path/to\your/desired\folder" is valid
  //
  // 'PathPop' removes the last appended path from the current path stored in
  // the 'OSPath':
  //
  //   path:        path/to/your/desired/folder
  //   popped_path: path/to/your/desired

  // NOTE: DN_OS_SecureRNGBytes /////////////////////////////////////////////////////////////////
  //
  // Generate cryptographically secure bytes

#if 0
  // NOTE: DN_PCG32 /////////////////////////////////////////////////////////////////////////////
  //
  // Random number generator of the PCG family. Implementation taken from
  // Martins Mmozeiko from Handmade Network.
  // https://gist.github.com/mmozeiko/1561361cd4105749f80bb0b9223e9db8
  {
    DN_PCG32 rng = DN_PCG32_Init(0xb917'a66c'1d9b'3bd8);

    // NOTE: DN_PCG32_Range ///////////////////////////////////////////////////////////////////
    //
    // Generate a value in the [low, high) interval
    uint32_t u32_value = DN_PCG32_Range(&rng, 32, 64);
    DN_Assert(u32_value >= 32 && u32_value < 64);

    // NOTE: DN_PCG32_NextF32 /////////////////////////////////////////////////////////////////
    // NOTE: DN_PCG32_NextF64 /////////////////////////////////////////////////////////////////
    //
    // Generate a float/double in the [0, 1) interval
    DN_F64 f64_value = DN_PCG32_NextF64(&rng);
    DN_Assert(f64_value >= 0.f && f64_value < 1.f);

    // NOTE: DN_PCG32_Advance /////////////////////////////////////////////////////////////////
    //
    // Step the random number generator by 'delta' steps
    DN_PCG32_Advance(&rng, /*delta*/ 5);
  }
#endif

#if 0
#if !defined(DN_NO_PROFILER)
  // NOTE: DN_Profiler /////////////////////////////////////////////////////////////////////////////
  //
  // A profiler based off Casey Muratori's Computer Enhance course, Performance
  // Aware Programming. This profiler measures function elapsed time using the
  // CPU's time stamp counter (e.g. rdtsc) providing a rough cycle count
  // that can be converted into a duration.
  //
  // This profiler uses a double buffer scheme for storing profiling markers.
  // After an application's typical update/frame cycle you can swap the
  // profiler's buffer whereby the front buffer contains the previous frames
  // profiling metrics and the back buffer will be populated with the new
  // frame's profiling metrics.
  {
    enum Zone
    {
      Zone_MainLoop,
      Zone_Count
    };

    DN_ProfilerZone profiler_zone_main_update = DN_Profiler_BeginZone(Zone_MainLoop);

    // NOTE: DN_Profiler_AnchorBuffer /////////////////////////////////////////////////////
    //
    // Retrieve the requested buffer from the profiler for
    // writing/reading profiling metrics. Pass in the enum to specify
    // which buffer to grab from the profiler.
    //
    // The front buffer contains the previous frame's profiling metrics
    // and the back buffer is where the profiler is currently writing
    // to.
    //
    // For end user intents and purposes, you likely only need to read
    // the front buffer which contain the metrics that you can visualise
    // regarding the most profiling metrics recorded.

    // NOTE: DN_Profiler_ReadBuffer ///////////////////////////////////////////////////////////
    //
    // Retrieve the buffer of anchors of which there are
    // `DN_PROFILER_ANCHOR_BUFFER_SIZE` anchors from the most recent run
    // of the profiler after you have called `SwapAnchorBuffer` to trigger
    // the double buffer
    DN_ProfilerAnchor *read_anchors = DN_Profiler_ReadBuffer();
    for (DN_USize index = 0; index < DN_PROFILER_ANCHOR_BUFFER_SIZE; index++) {
      DN_ProfilerAnchor *anchor = read_anchors + index;
      if (DN_Str8_HasData(anchor->name)) {
        // ...
      }
    }

    // NOTE: DN_Profiler_WriteBuffer //////////////////////////////////////////////////////////
    //
    // Same as `ReadBuffer` however we return the buffer that the profiler
    // is currently writing anchors into.
    DN_ProfilerAnchor *write_anchors = DN_Profiler_WriteBuffer();
    for (DN_USize index = 0; index < DN_PROFILER_ANCHOR_BUFFER_SIZE; index++) {
      DN_ProfilerAnchor *anchor = write_anchors + index;
      if (DN_Str8_HasData(anchor->name)) {
        // ...
      }
    }

    DN_Profiler_EndZone(profiler_zone_main_update);
    DN_Profiler_SwapAnchorBuffer(); // Should occur after all profiling zones are ended!
    DN_Memset(g_dn_core->profiler, 0, sizeof(*g_dn_core->profiler));
  }
#endif // !defined(DN_NO_PROFILER)
#endif

  // NOTE: DN_Raycast_LineIntersectV2 ///////////////////////////////////////////////////////////
  // Calculate the intersection point of 2 rays returning a `t` value
  // which is how much along the direction of the 'ray' did the intersection
  // occur.
  //
  // The arguments passed in do not need to be normalised for the function to
  // work.

  // NOTE: DN_Safe_* ////////////////////////////////////////////////////////////////////////////
  //
  // Performs the arithmetic operation and uses DN_Check on the operation to
  // check if it overflows. If it overflows the MAX value of the integer is
  // returned in add and multiply operations, and, the minimum is returned in
  // subtraction and division.

  // NOTE: DN_SaturateCast* ////////////////////////////////////////////////////////////////
  //
  // Truncate the passed in value to the return type clamping the resulting
  // value to the max value of the desired data type. It DN_Check's the
  // truncation.
  //
  // The following sentinel values are returned when saturated,
  // USize -> Int:  INT_MAX
  // USize -> I8:   INT8_MAX
  // USize -> I16:  INT16_MAX
  // USize -> I32:  INT32_MAX
  // USize -> I64:  INT64_MAX
  //
  // U64   -> UInt: UINT_MAX
  // U64   -> U8:   UINT8_MAX
  // U64   -> U16:  UINT16_MAX
  // U64   -> U32:  UINT32_MAX
  //
  // USize -> U8:   UINT8_MAX
  // USize -> U16:  UINT16_MAX
  // USize -> U32:  UINT32_MAX
  // USize -> U64:  UINT64_MAX
  //
  // ISize -> Int:  INT_MIN   or INT_MAX
  // ISize -> I8:   INT8_MIN  or INT8_MAX
  // ISize -> I16:  INT16_MIN or INT16_MAX
  // ISize -> I32:  INT32_MIN or INT32_MAX
  // ISize -> I64:  INT64_MIN or INT64_MAX
  //
  // ISize -> UInt: 0 or UINT_MAX
  // ISize -> U8:   0 or UINT8_MAX
  // ISize -> U16:  0 or UINT16_MAX
  // ISize -> U32:  0 or UINT32_MAX
  // ISize -> U64:  0 or UINT64_MAX
  //
  // I64 -> ISize:  DN_ISIZE_MIN or DN_ISIZE_MAX
  // I64 -> I8:     INT8_MIN      or INT8_MAX
  // I64 -> I16:    INT16_MIN     or INT16_MAX
  // I64 -> I32:    INT32_MIN     or INT32_MAX
  //
  // Int -> I8:     INT8_MIN  or INT8_MAX
  // Int -> I16:    INT16_MIN or INT16_MAX
  // Int -> U8:     0         or UINT8_MAX
  // Int -> U16:    0         or UINT16_MAX
  // Int -> U32:    0         or UINT32_MAX
  // Int -> U64:    0         or UINT64_MAX

  // NOTE: DN_StackTrace ////////////////////////////////////////////////////////////////////////
  // Emit stack traces at the calling site that these functions are invoked
  // from.
  //
  // For some applications, it may be viable to generate raw stack traces and
  // store just the base addresses of the call stack from the 'Walk'
  // functions. This reduces the memory overhead and required to hold onto
  // stack traces and resolve the addresses on-demand when required.
  //
  // However if your application is loading and/or unloading shared libraries,
  // on Windows it may be impossible for the application to resolve raw base
  // addresses if they become invalid over time. In these applications you
  // must convert the raw stack traces before the unloading occurs, and when
  // loading new shared libraries, 'ReloadSymbols' must be called to ensure
  // the debug APIs are aware of how to resolve the new addresses imported
  // into the address space.
  {
    DN_OSTLSTMem tmem = DN_OS_TLSTMem(nullptr);

    // NOTE: DN_StackTrace_Walk ///////////////////////////////////////////////////////////////
    //
    // Generate a stack trace as a series of addresses to the base of the
    // functions on the call-stack at the current instruction pointer. The
    // addresses are stored in order from the current executing function
    // first to the most ancestor function last in the walk.
    DN_StackTraceWalkResult walk = DN_StackTrace_Walk(tmem.arena, /*depth limit*/ 128);

    // Loop over the addresses produced in the stack trace
    for (DN_StackTraceWalkResultIterator it = {}; DN_StackTrace_WalkResultIterate(&it, &walk);) {
      // NOTE: DN_StackTrace_RawFrameToFrame ////////////////////////////////////////////////
      //
      // Converts the base address into a human readable stack trace
      // entry (e.g. address, line number, file and function name).
      DN_StackTraceFrame frame = DN_StackTrace_RawFrameToFrame(tmem.arena, it.raw_frame);

      // You may then print out the frame like so
      if (0)
        printf("%.*s(%" PRIu64 "): %.*s\n", DN_STR_FMT(frame.file_name), frame.line_number, DN_STR_FMT(frame.function_name));
    }

    // If you load new shared-libraries into the address space it maybe
    // necessary to call into 'ReloadSymbols' to ensure that the OS is able
    // to resolve the new addresses.
    DN_StackTrace_ReloadSymbols();

    // NOTE: DN_StackTrace_GetFrames //////////////////////////////////////////////////////////
    //
    // Helper function to create a stack trace and automatically convert the
    // raw frames into human readable frames. This function effectively
    // calls 'Walk' followed by 'RawFrameToFrame'.
    DN_Slice<DN_StackTraceFrame> frames = DN_StackTrace_GetFrames(tmem.arena, /*depth limit*/ 128);
    (void)frames;
  }

  // NOTE: DN_Str8_Alloc ////////////////////////////////////////////////////////////////////////
  //
  // Allocates a string with the requested 'size'. An additional byte is
  // always requested from the allocator to null-terminate the buffer. This
  // allows the string to be used with C-style string APIs.
  //
  // The returned string's 'size' member variable does *not* include this
  // additional null-terminating byte.
  {
    DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
    DN_Str8      string = DN_Str8_Alloc(tmem.arena, /*size*/ 1, DN_ZeroMem_Yes);
    DN_Assert(string.size == 1);
    DN_Assert(string.data[string.size] == 0); // It is null-terminated!
  }

  // NOTE: DN_Str8_BSplit //////////////////////////////////////////////////////////////////
  //
  // Splits a string into 2 substrings occuring prior and after the first
  // occurence of the delimiter. Neither strings include the matched
  // delimiter. If no delimiter is found, the 'rhs' of the split will be
  // empty.
  {
    DN_Str8BSplitResult dot_split   = DN_Str8_BSplit(/*string*/ DN_STR8("abc.def.ghi"), /*delimiter*/ DN_STR8("."));
    DN_Str8BSplitResult slash_split = DN_Str8_BSplit(/*string*/ DN_STR8("abc.def.ghi"), /*delimiter*/ DN_STR8("/"));
    DN_Assert(dot_split.lhs == DN_STR8("abc") && dot_split.rhs == DN_STR8("def.ghi"));
    DN_Assert(slash_split.lhs == DN_STR8("abc.def.ghi") && slash_split.rhs == DN_STR8(""));

    // Loop that walks the string and produces ("abc", "def", "ghi")
    for (DN_Str8 it = DN_STR8("abc.def.ghi"); it.size;) {
      DN_Str8BSplitResult split = DN_Str8_BSplit(it, DN_STR8("."));
      DN_Str8                  chunk = split.lhs; // "abc", "def", ...
      it                             = split.rhs;
      (void)chunk;
    }
  }

  // NOTE: DN_Str8_FileNameFromPath /////////////////////////////////////////////////////////////
  //
  // Takes a slice to the file name from a file path. The file name is
  // evaluated by searching from the end of the string backwards to the first
  // occurring path separator '/' or '\'. If no path separator is found, the
  // original string is returned. This function preserves the file extension
  // if there were any.
  {
    {
      DN_Str8 string = DN_Str8_FileNameFromPath(DN_STR8("C:/Folder/item.txt"));
      DN_Assert(string == DN_STR8("item.txt"));
    }
    {
      // TODO(doyle): Intuitively this seems incorrect. Empty string instead?
      DN_Str8 string = DN_Str8_FileNameFromPath(DN_STR8("C:/Folder/"));
      DN_Assert(string == DN_STR8("C:/Folder"));
    }
    {
      DN_Str8 string = DN_Str8_FileNameFromPath(DN_STR8("C:/Folder"));
      DN_Assert(string == DN_STR8("Folder"));
    }
  }

  // NOTE: DN_Str8_FilePathNoExtension //////////////////////////////////////////////////////////
  //
  // This function preserves the original string if no extension was found.
  // An extension is defined as the substring after the last '.' encountered
  // in the string.
  {
    DN_Str8 string = DN_Str8_FilePathNoExtension(DN_STR8("C:/Folder/item.txt.bak"));
    DN_Assert(string == DN_STR8("C:/Folder/item.txt"));
  }

  // NOTE: DN_Str8_FileNameNoExtension //////////////////////////////////////////////////////////
  //
  // This function is the same as calling 'FileNameFromPath' followed by
  // 'FilePathNoExtension'
  {
    DN_Str8 string = DN_Str8_FileNameNoExtension(DN_STR8("C:/Folder/item.txt.bak"));
    DN_Assert(string == DN_STR8("item.txt"));
  }

  // NOTE: DN_Str8_Replace            ///////////////////////////////////////////////////////////
  // NOTE: DN_Str8_ReplaceInsensitive ///////////////////////////////////////////////////////////
  //
  // Replace any matching substring 'find' with 'replace' in the passed in
  // 'string'. The 'start_index' may be specified to offset which index the
  // string will start doing replacements from.
  //
  // String replacements are not done inline and the returned string will
  // always be a newly allocated copy, irrespective of if any replacements
  // were done or not.
  {
    DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
    DN_Str8      string = DN_Str8_Replace(/*string*/ DN_STR8("Foo Foo Bar"),
                                     /*find*/ DN_STR8("Foo"),
                                     /*replace*/ DN_STR8("Moo"),
                                     /*start_index*/ 1,
                                     /*arena*/ tmem.arena,
                                     /*eq_case*/ DN_Str8EqCase_Sensitive);
    DN_Assert(string == DN_STR8("Foo Moo Bar"));
  }

  // NOTE: DN_Str8_Segment //////////////////////////////////////////////////////////////////////
  //
  // Add a delimiting 'segment_char' every 'segment_size' number of characters
  // in the string.
  //
  // Reverse segment delimits the string counting 'segment_size' from the back
  // of the string.
  {
    DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
    DN_Str8      string = DN_Str8_Segment(tmem.arena, /*string*/ DN_STR8("123456789"), /*segment_size*/ 3, /*segment_char*/ ',');
    DN_Assert(string == DN_STR8("123,456,789"));
  }

  // NOTE: DN_Str8_Split ////////////////////////////////////////////////////////////////////////
  {
    // Splits the string at each delimiter into substrings occuring prior and
    // after until the next delimiter.
    DN_OSTLSTMem tmem = DN_OS_TLSTMem(nullptr);
    {
      DN_Slice<DN_Str8> splits = DN_Str8_SplitAlloc(/*arena*/ tmem.arena,
                                                    /*string*/ DN_STR8("192.168.8.1"),
                                                    /*delimiter*/ DN_STR8("."),
                                                    /*mode*/ DN_Str8SplitIncludeEmptyStrings_No);
      DN_Assert(splits.size == 4);
      DN_Assert(splits.data[0] == DN_STR8("192") && splits.data[1] == DN_STR8("168") && splits.data[2] == DN_STR8("8") && splits.data[3] == DN_STR8("1"));
    }

    // You can include empty strings that occur when splitting by setting
    // the split mode to include empty strings.
    {
      DN_Slice<DN_Str8> splits = DN_Str8_SplitAlloc(/*arena*/ tmem.arena,
                                                    /*string*/ DN_STR8("a--b"),
                                                    /*delimiter*/ DN_STR8("-"),
                                                    /*mode*/ DN_Str8SplitIncludeEmptyStrings_Yes);
      DN_Assert(splits.size == 3);
      DN_Assert(splits.data[0] == DN_STR8("a") && splits.data[1] == DN_STR8("") && splits.data[2] == DN_STR8("b"));
    }
  }

  // NOTE: DN_Str8_ToI64 ////////////////////////////////////////////////////////////////////////
  // NOTE: DN_Str8_ToU64 ////////////////////////////////////////////////////////////////////////
  //
  // Convert a number represented as a string to a signed 64 bit number.
  //
  // The 'separator' is an optional digit separator for example, if
  // 'separator' is set to ',' then '1,234' will successfully be parsed to
  // '1234'. If no separator is desired, you may pass in '0' in which
  // '1,234' will *not* be succesfully parsed.
  //
  // Real numbers are truncated. Both '-' and '+' prefixed strings are permitted,
  // i.e. "+1234" -> 1234 and "-1234" -> -1234. Strings must consist entirely of
  // digits, the seperator or the permitted prefixes as previously mentioned
  // otherwise this function will return false, i.e. "1234 dog" will cause the
  // function to return false, however, the output is greedily converted and
  // will be evaluated to "1234".
  //
  // 'ToU64' only   '+'        prefix is permitted
  // 'ToI64' either '+' or '-' prefix is permitted
  {
    {
      DN_Str8ToI64Result result = DN_Str8_ToI64(DN_STR8("-1,234"), /*separator*/ ',');
      DN_Assert(result.success && result.value == -1234);
    }
    {
      DN_Str8ToI64Result result = DN_Str8_ToI64(DN_STR8("-1,234"), /*separator*/ 0);
      DN_Assert(!result.success && result.value == 1); // 1 because it's a greedy conversion
    }
  }

  // NOTE: DN_Str8_TrimByteOrderMark ////////////////////////////////////////////////////////////
  //
  // Removes a leading UTF8, UTF16 BE/LE, UTF32 BE/LE byte order mark from the
  // string if it's present.

  // NOTE: DN_STR_FMT ///////////////////////////////////////////////////////////////////////////
  //
  // Unpacks a string struct that has the fields {.data, .size} for printing a
  // pointer and length style string using the printf format specifier "%.*s"
  //
  //   printf("%.*s\n", DN_STR_FMT(DN_STR8("Hello world")));

  // NOTE: DN_Str8Builder_AppendF    ////////////////////////////////////////////////////////////
  // NOTE: DN_Str8Builder_AppendFV   ////////////////////////////////////////////////////////////
  // NOTE: DN_Str8Builder_AppendRef  ////////////////////////////////////////////////////////////
  // NOTE: DN_Str8Builder_AppendCopy ////////////////////////////////////////////////////////////
  //
  // - Appends a string to the string builder as follows
  //
  //     AppendRef:  Stores the string slice by value
  //     AppendCopy: Stores the string slice by copy (with builder's arena)
  //     AppendF/V:  Constructs a format string and calls 'AppendRef'

  // NOTE: DN_Str8Builder_Build    ///////////////////////////////////////////////////////////
  // NOTE: DN_Str8Builder_BuildCRT ///////////////////////////////////////////////////////////
  //
  // Constructs the final string by merging all the appended strings into
  // one merged string.
  //
  // The CRT variant calls into 'malloc' and the string *must* be released
  // using 'free'.

  // NOTE: DN_Str8Builder_BuildSlice  ///////////////////////////////////////////////////////////
  //
  // Constructs the final string into an array of strings (e.g. a slice)

  // NOTE: DN_TicketMutex ///////////////////////////////////////////////////////////////////////
  //
  // A mutex implemented using an atomic compare and swap on tickets handed
  // out for each critical section.
  //
  // This mutex serves ticket in order and will block all other threads until
  // the tickets are returned in order. The thread with the oldest ticket that
  // has not been returned has right of way to execute, all other threads will
  // be blocked in an atomic compare and swap loop. block execution by going
  // into an atomic
  //
  // When a thread is blocked by this mutex, a spinlock intrinsic '_mm_pause' is
  // used to yield the CPU and reduce spinlock on the thread. This mutex is not
  // ideal for long blocking operations. This mutex does not issue any syscalls
  // and relies entirely on atomic instructions.
  {
    DN_TicketMutex mutex = {};
    DN_TicketMutex_Begin(&mutex); // Simple procedural mutual exclusion lock
    DN_TicketMutex_End(&mutex);

    // NOTE: DN_TicketMutex_MakeTicket ////////////////////////////////////////////////////////
    //
    // Request the next available ticket for locking from the mutex.
    DN_UInt ticket = DN_TicketMutex_MakeTicket(&mutex);

    if (DN_TicketMutex_CanLock(&mutex, ticket)) {
      // NOTE: DN_TicketMutex_BeginTicket ///////////////////////////////////////////////////
      //
      // Locks the mutex using the given ticket if possible. If it's not
      // the next ticket to be locked the executing thread will block
      // until the mutex can lock the ticket, i.e. All prior tickets are
      // returned, in sequence, to the mutex.
      DN_TicketMutex_BeginTicket(&mutex, ticket);
      DN_TicketMutex_End(&mutex);
    }
  }

  // NOTE: DN_ThreadContext /////////////////////////////////////////////////////////////////////
  //
  // Each thread is assigned in their thread-local storage (TLS) tmem and
  // permanent arena allocators. These can be used for allocations with a
  // lifetime scoped to the lexical scope or for storing data permanently
  // using the arena paradigm.
  //
  // TLS in this implementation is implemented using the `thread_local` C/C++
  // keyword.
  //
  // 99% of the time you will want DN_OS_TLSTMem(...) which returns you a
  // temporary arena for function lifetime allocations. On scope exit, the
  // arena is cleared out.
  //
  // This library's paradigm revolves heavily around arenas including tmem
  // arenas into child functions for temporary calculations. If an arena is
  // passed into a function, this poses a problem sometimes known as
  // 'arena aliasing'.
  //
  // If an arena aliases another arena (e.g. the arena passed in) is the same
  // as the tmem arena requested in the function, we risk the tmem arena
  // on scope exit deallocating memory belonging to the caller.
  //
  // To avoid this we the 'DN_OS_TLSTMem(...)' API takes in a list of arenas
  // to ensure that we provide a tmem arena that *won't* alias with the
  // caller's arena. If arena aliasing occurs, with ASAN on, generally
  // the library will trap and report use-after-poison once violated.
  {
    DN_OSTLSTMem tmem_a = DN_OS_TLSTMem(nullptr);

    // Now imagine we call a function where we pass tmem_a.arena down
    // into it .. If we call tmem again, we need to pass in the arena
    // to prevent aliasing.
    DN_OSTLSTMem tmem_b = DN_OS_TLSTMem(tmem_a.arena);
    DN_Assert(tmem_a.arena != tmem_b.arena);
  }

  // @proc DN_Thread_GetTMem
  //   @desc Retrieve the per-thread temporary arena allocator that is reset on scope
  //   exit.

  //   The tmem arena must be deconflicted with any existing arenas in the
  //   function to avoid trampling over each other's memory. Consider the situation
  //   where the tmem arena is passed into the function. Inside the function, if
  //   the same arena is reused then, if both arenas allocate, when the inner arena
  //   is reset, this will undo the passed in arena's allocations in the function.

  //   @param[in] conflict_arena A pointer to the arena currently being used in the
  //   function

  // NOTE: DN_CVT_Str8FromU64 /////////////////////////////////////////////////////////////////////////
  {
    DN_CVTU64Str8 string = DN_CVT_Str8FromU64(123123, ',');
    if (0) // Prints "123,123"
      printf("%.*s", DN_STR_FMT(string));
  }

  // NOTE: DN_CVT_AgeFromU64 //////////////////////////////////////////////////////////////////////////
  {
    DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
    DN_Str8      string = DN_CVT_AgeFromU64(tmem.arena, DN_HoursToSec(2) + DN_MinutesToSec(30), DN_CVTU64AgeUnit_All);
    DN_Assert(DN_Str8_Eq(string, DN_STR8("2h 30m")));
  }

  // NOTE: DN_VArray ////////////////////////////////////////////////////////////////////////////
  //
  // An array that is backed by virtual memory by reserving addressing space
  // and comitting pages as items are allocated in the array. This array never
  // reallocs, instead you should reserve the upper bound of the memory you
  // will possibly ever need (e.g. 16GB) and let the array commit physical
  // pages on demand.
  //
  // On 64 bit operating systems you are given 48 bits of addressable space
  // giving you 256 TB of reservable memory. This gives you practically
  // an unlimited array capacity that avoids reallocs and only consumes memory
  // that is actually occupied by the array.
  //
  // Each page that is committed into the array will be at page/allocation
  // granularity which are always cache aligned. This array essentially retains
  // all the benefits of normal arrays,
  //
  // - contiguous memory
  // - O(1) random access
  // - O(N) iterate
  //
  // In addition to no realloc on expansion or shrinking.
  //
  {
    // NOTE: DN_VArray_Init         ///////////////////////////////////////////////////////////
    // NOTE: DN_VArray_InitByteSize ///////////////////////////////////////////////////////////
    //
    // Initialise an array with the requested byte size or item capacity
    // respectively. The returned array may have a higher capacity than the
    // requested amount since requested memory from the OS may have a certain
    // alignment requirement (e.g. on Windows reserve/commit are 64k/4k
    // aligned).
    DN_VArray<int> array = DN_VArray_Init<int>(1024);
    DN_Assert(array.size == 0 && array.max >= 1024);

    // NOTE: DN_VArray_Make      //////////////////////////////////////////////////////////////
    // NOTE: DN_VArray_Add       //////////////////////////////////////////////////////////////
    // NOTE: DN_VArray_MakeArray //////////////////////////////////////////////////////////////
    // NOTE: DN_VArray_AddArray  //////////////////////////////////////////////////////////////
    //
    // Allocate items from the array where:
    //
    //   Make: creates a zero-init item from the array
    //   Add:  creates a zero-init item and memcpy passed in data into the item
    //
    // If the array has run out of capacity or was never initialised, a null
    // pointer is returned.
    int *item = DN_VArray_Add(&array, 0xCAFE);
    DN_Assert(*item == 0xCAFE && array.size == 1);

    // NOTE: DN_VArray_AddCArray  /////////////////////////////////////////////////////////////
    DN_VArray_AddCArray(&array, {1, 2, 3});
    DN_Assert(array.size == 4);

// TODO(doyle): There's a bug here with the negative erase!
// Loop over the array items and erase 1 item.
#if 0
      for (DN_USize index = 0; index < array.size; index++) {
          if (index != 1)
              continue;

          // NOTE: DN_VArray_EraseRange /////////////////////////////////////////////////////////
          //
          // Erase the next 'count' items at 'begin_index' in the array.
          // 'count' can be positive or negative which dictates the if we
          // erase forward from the 'begin_index' or in reverse.
          //
          // This operation will invalidate all pointers to the array!
          //
          // A stable erase will shift all elements after the erase ranged
          // into the range preserving the order of prior elements. Unstable
          // erase will move the tail elements into the range being erased.
          //
          // Erase range returns a result that contains the next iterator
          // index that can be used to update the your for loop index if you
          // are trying to iterate over the array.

          // TODO(doyle): There's a bug here! This doesn't work.
          // Erase index 0 with the negative count!
          DN_ArrayEraseResult erase_result = DN_VArray_EraseRange(&array,
                                                                    /*begin_index*/ index,
                                                                    /*count*/ -1,
                                                                    /*erase*/ DN_ArrayErase_Stable);
          DN_Assert(erase_result.items_erased == 1);

          // Use the index returned to continue linearly iterating the array
          index = erase_result.it_index;
          DN_Assert(array.data[index + 1] == 2); // Next loop iteration will process item '2'
      }

      DN_Assert(array.size    == 3 &&
                array.data[0] == 1 &&
                array.data[1] == 2 &&
                array.data[2] == 3);
#endif

    // NOTE: DN_VArray_Reserve ////////////////////////////////////////////////////////////////////
    //
    // Ensure that the requested number of items are backed by physical pages
    // from the OS. Calling this pre-emptively will minimise syscalls into the
    // kernel to request memory. The requested items will be rounded up to the
    // in bytes to the allocation granularity of OS allocation APIs hence the
    // reserved space may be greater than the requested amount (e.g. this is 4k
    // on Windows).
    DN_VArray_Reserve(&array, /*count*/ 8);

    DN_VArray_Deinit(&array);
  }

  // NOTE: DN_W32_LastError         /////////////////////////////////////////////////////////////
  // NOTE: DN_W32_ErrorCodeToMsg    /////////////////////////////////////////////////////////////
  #if defined(DN_PLATFORM_WIN32)
    if (0) {
      // Generate the error string for the last Win32 API called that return
      // an error value.
      DN_OSTLSTMem  tmem           = DN_OS_TLSTMem(nullptr);
      DN_W32Error get_last_error = DN_W32_LastError(tmem.arena);
      printf("Error (%lu): %.*s", get_last_error.code, DN_STR_FMT(get_last_error.msg));

      // Alternatively, pass in the error code directly
      DN_W32Error error_msg_for_code = DN_W32_ErrorCodeToMsg(tmem.arena, /*error_code*/ 0);
      printf("Error (%lu): %.*s", error_msg_for_code.code, DN_STR_FMT(error_msg_for_code.msg));
    }

  // NOTE: DN_W32_MakeProcessDPIAware ///////////////////////////////////////////////////////////
  //
  // Call once at application start-up to ensure that the application is DPI
  // aware on Windows and ensure that application UI is scaled up
  // appropriately for the monitor.

  // NOTE: DN_W32_Str8ToStr16       /////////////////////////////////////////////////////////////
  // NOTE: DN_W32_Str8ToStr16Buffer /////////////////////////////////////////////////////////////
  // NOTE: DN_W32_Str16ToStr8       /////////////////////////////////////////////////////////////
  // NOTE: DN_W32_Str16ToStr8Buffer /////////////////////////////////////////////////////////////
  //
  // Convert a UTF8 <-> UTF16 string.
  //
  // The exact size buffer required for this function can be determined by
  // calling this function with the 'dest' set to null and 'dest_size' set to
  // 0, the return size is the size required for conversion not-including
  // space for the null-terminator. This function *always* null-terminates the
  // input buffer.
  //
  // Returns the number of u8's (for UTF16->8) OR u16's (for UTF8->16)
  // written/required for conversion. 0 if there was a conversion error and can be
  // queried using 'DN_W32_LastError'
  #endif
}

DN_MSVC_WARNING_POP
