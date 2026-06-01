#if !defined(DN_HELPERS_H)
#define DN_HELPERS_H

#if defined(_CLANGD)
  #include "../dn.h"
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

#if !defined(DN_NO_JSON_BUILDER)
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
  bool     found;
  DN_USize index;
};

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: DN_JSONBuilder
#define DN_JSONBuilder_Object(builder)              \
  DN_DeferLoop(DN_JSONBuilder_ObjectBegin(builder), \
                DN_JSONBuilder_ObjectEnd(builder))

#define DN_JSONBuilder_ObjectNamed(builder, name)              \
  DN_DeferLoop(DN_JSONBuilder_ObjectBeginNamed(builder, name), \
                DN_JSONBuilder_ObjectEnd(builder))

#define DN_JSONBuilder_Array(builder)              \
  DN_DeferLoop(DN_JSONBuilder_ArrayBegin(builder), \
                DN_JSONBuilder_ArrayEnd(builder))

#define DN_JSONBuilder_ArrayNamed(builder, name)              \
  DN_DeferLoop(DN_JSONBuilder_ArrayBeginNamed(builder, name), \
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

#define                    DN_JSONBuilder_ObjectBegin(builder)    DN_JSONBuilder_ObjectBeginNamed(builder, DN_Str8Lit(""))
#define                    DN_JSONBuilder_ArrayBegin(builder)     DN_JSONBuilder_ArrayBeginNamed(builder, DN_Str8Lit(""))
#define                    DN_JSONBuilder_Str8(builder, value)    DN_JSONBuilder_Str8Named(builder, DN_Str8Lit(""), value)
#define                    DN_JSONBuilder_Literal(builder, value) DN_JSONBuilder_LiteralNamed(builder, DN_Str8Lit(""), value)
#define                    DN_JSONBuilder_U64(builder, value)     DN_JSONBuilder_U64Named(builder, DN_Str8Lit(""), value)
#define                    DN_JSONBuilder_I64(builder, value)     DN_JSONBuilder_I64Named(builder, DN_Str8Lit(""), value)
#define                    DN_JSONBuilder_F64(builder, value)     DN_JSONBuilder_F64Named(builder, DN_Str8Lit(""), value)
#define                    DN_JSONBuilder_Bool(builder, value)    DN_JSONBuilder_BoolNamed(builder, DN_Str8Lit(""), value)
#endif // !defined(DN_NO_JSON_BUILDER)

// NOTE: DN_BinarySearch
template <typename T> bool                  DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> DN_BinarySearchResult DN_BinarySearch                (T const *array, DN_USize array_size, T const &find, DN_BinarySearchType type = DN_BinarySearchType_Match, DN_BinarySearchLessThanProc<T> less_than = DN_BinarySearch_DefaultLessThan);

// NOTE: DN_BinarySearch
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
#endif // !defined(DN_HELPERS_H)
