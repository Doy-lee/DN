#pragma once
#include "dqn.h"

#if !defined(SHEREDOM_JSON_H_INCLUDED)
    #error Sheredom json.h (github.com/sheredom/json.h) must be included before this file
#endif

// NOTE: DN_JSON //////////////////////////////////////////////////////////////////////////////////

void       *DN_JSON_ArenaAllocFunc (void *user_data, size_t count);
char const *DN_JSON_TypeEnumCString(json_type_e type, size_t *size);
bool        DN_JSON_String8Cmp     (json_string_s const *lhs, DN_Str8 rhs);

// NOTE: DN_JSON_It /////////////////////////////////////////////////////////////////////////
enum DN_JSONItEntryType
{
    DN_JSON_ItEntryTypeObjElement,
    DN_JSON_ItEntryTypeObj,
    DN_JSON_ItEntryTypeArrayElement,
    DN_JSON_ItEntryTypeArray,
    DN_JSON_ItEntryTypeString,
    DN_JSON_ItEntryTypeNumber,
};

struct DN_JSONItEntry
{
    DN_JSONItEntryType type;
    void               *value;
};

struct DN_JSONIt
{
    DN_JSONItEntry stack[128];
    int             stack_count;
    size_t          flags;
};

DN_JSONIt DN_JSON_LoadFileToIt(DN_Arena *arena, DN_Str8 json);

// NOTE: DN_JSON_ItPush/Pop /////////////////////////////////////////////////////////////////
bool DN_JSON_ItPushObjElement  (DN_JSONIt *it, json_object_element_s *element);
bool DN_JSON_ItPushObj         (DN_JSONIt *it, json_object_s *obj);
bool DN_JSON_ItPushArrayElement(DN_JSONIt *it, json_array_element_s *element);
bool DN_JSON_ItPushArray       (DN_JSONIt *it, json_value_s *value);
bool DN_JSON_ItPushValue       (DN_JSONIt *it, json_value_s *value);
void DN_JSON_ItPop             (DN_JSONIt *it);

// NOTE: DN_JSON_It tree navigation /////////////////////////////////////////////////////////
json_value_s *DN_JSON_ItPushCurrValue(DN_JSONIt *it);
bool          DN_JSON_ItNext(DN_JSONIt *it);

#define DN_JSON_ItPushCurrValueIterateThenPop(it) \
    for(void *DN_UNIQUE_NAME(ptr) = DN_JSON_ItPushCurrValue(it); DN_UNIQUE_NAME(ptr); DN_JSON_ItPop(it), DN_UNIQUE_NAME(ptr) = nullptr) \
        while (DN_JSON_ItNext(it))

// NOTE: DN_JSON_ItCurr /////////////////////////////////////////////////////////////////////
DN_JSONItEntry        *DN_JSON_ItCurr(DN_JSONIt *it);
json_value_s          *DN_JSON_ItCurrValue(DN_JSONIt *it);
json_object_element_s *DN_JSON_ItCurrObjElement(DN_JSONIt *it);

// NOTE: DN_JSON_ItValueIs //////////////////////////////////////////////////////////////////
json_value_s  *DN_JSON_ItValueIs(DN_JSONIt *it, json_type_e type);
json_object_s *DN_JSON_ItValueIsObj(DN_JSONIt *it);
json_array_s  *DN_JSON_ItValueIsArray(DN_JSONIt *it);
json_string_s *DN_JSON_ItValueIsString(DN_JSONIt *it);
json_number_s *DN_JSON_ItValueIsNumber(DN_JSONIt *it);
json_value_s  *DN_JSON_ItValueIsBool(DN_JSONIt *it);
json_value_s  *DN_JSON_ItValueIsNull(DN_JSONIt *it);

size_t         DN_JSON_ItValueArraySize(DN_JSONIt *it);

// NOTE: DN_JSON_ItKeyValueIs ///////////////////////////////////////////////////////////////
DN_Str8        DN_JSON_ItKey(DN_JSONIt *it);
bool           DN_JSON_ItKeyIs(DN_JSONIt *it, DN_Str8 key);
json_object_s *DN_JSON_ItKeyValueIsObj(DN_JSONIt *it, DN_Str8 key);
json_array_s  *DN_JSON_ItKeyValueIsArray(DN_JSONIt *it, DN_Str8 key);
json_string_s *DN_JSON_ItKeyValueIsString(DN_JSONIt *it, DN_Str8 key);
json_number_s *DN_JSON_ItKeyValueIsNumber(DN_JSONIt *it, DN_Str8 key);
json_value_s  *DN_JSON_ItKeyValueIsBool(DN_JSONIt *it, DN_Str8 key);
json_value_s  *DN_JSON_ItKeyValueIsNull(DN_JSONIt *it, DN_Str8 key);

// NOTE: DN_JSON_ItValueTo //////////////////////////////////////////////////////////////////
DN_Str8       DN_JSON_ItValueToString(DN_JSONIt *it);
int64_t       DN_JSON_ItValueToI64(DN_JSONIt *it);
uint64_t      DN_JSON_ItValueToU64(DN_JSONIt *it);
bool          DN_JSON_ItValueToBool(DN_JSONIt *it);

#define DN_JSON_ItErrorUnknownKeyValue(it) DN_JSON_ItErrorUnknownKeyValue_(it, DN_CALL_SITE)
void DN_JSON_ItErrorUnknownKeyValue_(DN_JSONIt *it, DN_CallSite call_site);
