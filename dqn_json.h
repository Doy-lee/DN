#pragma once
#include "dqn.h"

#if !defined(SHEREDOM_JSON_H_INCLUDED)
    #error Sheredom json.h (github.com/sheredom/json.h) must be included before this file
#endif

// NOTE: Dqn_JSON //////////////////////////////////////////////////////////////////////////////////

void       *Dqn_JSON_ArenaAllocFunc (void *user_data, size_t count);
char const *Dqn_JSON_TypeEnumCString(json_type_e type, size_t *size);
bool        Dqn_JSON_String8Cmp     (json_string_s const *lhs, Dqn_Str8 rhs);

// NOTE: Dqn_JSON_It /////////////////////////////////////////////////////////////////////////
enum Dqn_JSONItEntryType
{
    Dqn_JSON_ItEntryTypeObjElement,
    Dqn_JSON_ItEntryTypeObj,
    Dqn_JSON_ItEntryTypeArrayElement,
    Dqn_JSON_ItEntryTypeArray,
    Dqn_JSON_ItEntryTypeString,
    Dqn_JSON_ItEntryTypeNumber,
};

struct Dqn_JSONItEntry
{
    Dqn_JSONItEntryType type;
    void               *value;
};

struct Dqn_JSONIt
{
    Dqn_JSONItEntry stack[128];
    int             stack_count;
    size_t          flags;
};

Dqn_JSONIt Dqn_JSON_LoadFileToIt(Dqn_Arena *arena, Dqn_Str8 json);

// NOTE: Dqn_JSON_ItPush/Pop /////////////////////////////////////////////////////////////////
bool Dqn_JSON_ItPushObjElement  (Dqn_JSONIt *it, json_object_element_s *element);
bool Dqn_JSON_ItPushObj         (Dqn_JSONIt *it, json_object_s *obj);
bool Dqn_JSON_ItPushArrayElement(Dqn_JSONIt *it, json_array_element_s *element);
bool Dqn_JSON_ItPushArray       (Dqn_JSONIt *it, json_value_s *value);
bool Dqn_JSON_ItPushValue       (Dqn_JSONIt *it, json_value_s *value);
void Dqn_JSON_ItPop             (Dqn_JSONIt *it);

// NOTE: Dqn_JSON_It tree navigation /////////////////////////////////////////////////////////
json_value_s *Dqn_JSON_ItPushCurrValue(Dqn_JSONIt *it);
bool          Dqn_JSON_ItNext(Dqn_JSONIt *it);

#define Dqn_JSON_ItPushCurrValueIterateThenPop(it) \
    for(void *DQN_UNIQUE_NAME(ptr) = Dqn_JSON_ItPushCurrValue(it); DQN_UNIQUE_NAME(ptr); Dqn_JSON_ItPop(it), DQN_UNIQUE_NAME(ptr) = nullptr) \
        while (Dqn_JSON_ItNext(it))

// NOTE: Dqn_JSON_ItCurr /////////////////////////////////////////////////////////////////////
Dqn_JSONItEntry       *Dqn_JSON_ItCurr(Dqn_JSONIt *it);
json_value_s          *Dqn_JSON_ItCurrValue(Dqn_JSONIt *it);
json_object_element_s *Dqn_JSON_ItCurrObjElement(Dqn_JSONIt *it);

// NOTE: Dqn_JSON_ItValueIs //////////////////////////////////////////////////////////////////
json_value_s  *Dqn_JSON_ItValueIs(Dqn_JSONIt *it, json_type_e type);
json_object_s *Dqn_JSON_ItValueIsObj(Dqn_JSONIt *it);
json_array_s  *Dqn_JSON_ItValueIsArray(Dqn_JSONIt *it);
json_string_s *Dqn_JSON_ItValueIsString(Dqn_JSONIt *it);
json_number_s *Dqn_JSON_ItValueIsNumber(Dqn_JSONIt *it);
json_value_s  *Dqn_JSON_ItValueIsBool(Dqn_JSONIt *it);
json_value_s  *Dqn_JSON_ItValueIsNull(Dqn_JSONIt *it);

size_t         Dqn_JSON_ItValueArraySize(Dqn_JSONIt *it);

// NOTE: Dqn_JSON_ItKeyValueIs ///////////////////////////////////////////////////////////////
Dqn_Str8       Dqn_JSON_ItKey(Dqn_JSONIt *it);
bool           Dqn_JSON_ItKeyIs(Dqn_JSONIt *it, Dqn_Str8 key);
json_object_s *Dqn_JSON_ItKeyValueIsObj(Dqn_JSONIt *it, Dqn_Str8 key);
json_array_s  *Dqn_JSON_ItKeyValueIsArray(Dqn_JSONIt *it, Dqn_Str8 key);
json_string_s *Dqn_JSON_ItKeyValueIsString(Dqn_JSONIt *it, Dqn_Str8 key);
json_number_s *Dqn_JSON_ItKeyValueIsNumber(Dqn_JSONIt *it, Dqn_Str8 key);
json_value_s  *Dqn_JSON_ItKeyValueIsBool(Dqn_JSONIt *it, Dqn_Str8 key);
json_value_s  *Dqn_JSON_ItKeyValueIsNull(Dqn_JSONIt *it, Dqn_Str8 key);

// NOTE: Dqn_JSON_ItValueTo //////////////////////////////////////////////////////////////////
Dqn_Str8       Dqn_JSON_ItValueToString(Dqn_JSONIt *it);
int64_t        Dqn_JSON_ItValueToI64(Dqn_JSONIt *it);
uint64_t       Dqn_JSON_ItValueToU64(Dqn_JSONIt *it);
bool           Dqn_JSON_ItValueToBool(Dqn_JSONIt *it);

#define Dqn_JSON_ItErrorUnknownKeyValue(it) Dqn_JSON_ItErrorUnknownKeyValue_(it, DQN_CALL_SITE)
void Dqn_JSON_ItErrorUnknownKeyValue_(Dqn_JSONIt *it, Dqn_CallSite call_site);
