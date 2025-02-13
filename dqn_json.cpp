#pragma once
#include "dqn.h"

// NOTE: DN_JSON //////////////////////////////////////////////////////////////////////////////////
void *DN_JSON_ArenaAllocFunc(void *user_data, size_t count)
{
    void *result = NULL;
    if (!user_data)
        return result;

    DN_Arena *arena = DN_CAST(DN_Arena*)user_data;
    result = DN_Arena_Alloc(arena, count, alignof(json_value_s), DN_ZeroMem_No);
    return result;
}

char const *DN_JSON_TypeEnumCString(json_type_e type, size_t *size)
{
    switch (type) {
        case json_type_string: { if (size) { *size = sizeof("string") - 1; } return "string"; }
        case json_type_number: { if (size) { *size = sizeof("number") - 1; } return "number"; }
        case json_type_object: { if (size) { *size = sizeof("object") - 1; } return "object"; }
        case json_type_array:  { if (size) { *size = sizeof("array") - 1; } return "array"; }
        case json_type_true:   { if (size) { *size = sizeof("true (boolean)") - 1; } return "true (boolean)"; }
        case json_type_false:  { if (size) { *size = sizeof("false (boolean)") - 1; } return "false (boolean)"; }

        default: /*FALLTHRU*/
        case json_type_null: { if (size) { *size = sizeof("(null)") - 1; } return "(null)"; }
    }
}

bool DN_JSON_String8Cmp(json_string_s const *lhs, DN_Str8 key)
{
    bool result = false;
    if (lhs && DN_Str8_HasData(key)) {
        DN_Str8 lhs_string = DN_Str8_Init(lhs->string, lhs->string_size);
        result              = DN_Str8_Eq(lhs_string, key);
    }
    return result;
}

// NOTE: DN_JSON_It ///////////////////////////////////////////////////////////////////////////////
DN_JSONIt DN_JSON_LoadFileToIt(DN_Arena *arena, DN_Str8 json)
{
    json_parse_result_s parse_result = {};
    json_value_ex_s    *ex_value     =
        DN_CAST(json_value_ex_s *) json_parse_ex(json.data,
                                                  json.size,
                                                  json_parse_flags_allow_location_information,
                                                  DN_JSON_ArenaAllocFunc,
                                                  arena,
                                                  &parse_result);

    DN_JSONIt result = {};
    DN_JSON_ItPushValue(&result, &ex_value->value);
    return result;
}

// NOTE: DN_JSON_ItPush/Pop ///////////////////////////////////////////////////////////////////////
bool DN_JSON_ItPushObjElement(DN_JSONIt *it, json_object_element_s *element)
{
    if (!it || !element)
        return false;
    DN_ASSERT(it->stack_count < DN_ARRAY_ICOUNT(it->stack));
    it->stack[it->stack_count++] = {DN_JSON_ItEntryTypeObjElement, element};
    return true;
}

bool DN_JSON_ItPushObj(DN_JSONIt *it, json_object_s *obj)
{
    if (!it || !obj)
        return false;
    DN_ASSERT(it->stack_count < DN_ARRAY_ICOUNT(it->stack));
    it->stack[it->stack_count++] = {DN_JSON_ItEntryTypeObj, obj};
    return true;
}

bool DN_JSON_ItPushArrayElement(DN_JSONIt *it, json_array_element_s *element)
{
    if (!it || !element)
        return false;
    DN_ASSERT(it->stack_count < DN_ARRAY_ICOUNT(it->stack));
    it->stack[it->stack_count++] = {DN_JSON_ItEntryTypeArrayElement, element};
    return true;
}

bool DN_JSON_ItPushArray(DN_JSONIt *it, json_value_s *value)
{
    if (!it || !value || json_value_as_array(value) == nullptr)
        return false;
    DN_ASSERT(it->stack_count < DN_ARRAY_ICOUNT(it->stack));
    it->stack[it->stack_count++] = {DN_JSON_ItEntryTypeArray, value};
    return true;
}

bool DN_JSON_ItPushValue(DN_JSONIt *it, json_value_s *value)
{
    bool result = false;
    if (!it || !value)
        return result;

    if (value->type == json_type_object) {
        result = DN_JSON_ItPushObj(it, json_value_as_object(value));
    } else if (value->type == json_type_array) {
        result = DN_JSON_ItPushArray(it, value);
    }

    return result;
}

void DN_JSON_ItPop(DN_JSONIt *it)
{
    if (!it)
        return;
    DN_ASSERT(it->stack_count > 0);
    if (it->stack_count > 0)
        it->stack_count--;
}

// NOTE: DN_JSON_It JSON tree navigation //////////////////////////////////////////////////////////
json_value_s *DN_JSON_ItPushCurrValue(DN_JSONIt *it)
{
    json_value_s          *result = nullptr;
    DN_JSONItEntry *curr   = DN_JSON_ItCurr(it);
    if (!curr)
        return result;

    if (curr->type == DN_JSON_ItEntryTypeObjElement) {
        json_object_element_s *element = DN_CAST(json_object_element_s *) curr->value;
        result                         = element->value;
    } else if (curr->type == DN_JSON_ItEntryTypeArrayElement) {
        json_array_element_s *element = DN_CAST(json_array_element_s *) curr->value;
        result                        = element->value;
    } else {
        result = DN_CAST(json_value_s *) curr->value;
    }

    if (result->type == json_type_array) {
        json_array_s *array = json_value_as_array(result);
        DN_ASSERT(array);
        DN_JSON_ItPushArray(it, result);
    } else if (result->type == json_type_object) {
        json_object_s *obj = json_value_as_object(result);
        DN_ASSERT(obj);
        DN_JSON_ItPushObj(it, obj);
    }

    return result;
}

bool DN_JSON_ItNext(DN_JSONIt *it)
{
    DN_JSONItEntry *curr = DN_JSON_ItCurr(it);
    if (!curr)
        return false;

    json_object_element_s *obj_element   = nullptr;
    json_array_element_s  *array_element = nullptr;
    if (curr->type == DN_JSON_ItEntryTypeObj) {
        auto *obj   = DN_CAST(json_object_s *) curr->value;
        obj_element = obj->start;
    } else if (curr->type == DN_JSON_ItEntryTypeObjElement) {
        auto *element = DN_CAST(json_object_element_s *) curr->value;
        obj_element   = element->next;
        DN_JSON_ItPop(it);
    } else if (curr->type == DN_JSON_ItEntryTypeArray) {
        auto *value   = DN_CAST(json_value_s *) curr->value;
        auto *array   = json_value_as_array(value);
        array_element = array->start;
    } else if (curr->type == DN_JSON_ItEntryTypeArrayElement) {
        auto *element = DN_CAST(json_array_element_s *) curr->value;
        array_element = element->next;
        DN_JSON_ItPop(it);
    } else {
        DN_JSON_ItPop(it);
    }

    if (obj_element)
        DN_JSON_ItPushObjElement(it, obj_element);
    else if (array_element)
        DN_JSON_ItPushArrayElement(it, array_element);

    bool result = obj_element || array_element;
    return result;
}

// NOTE: DN_JSON_ItCurr ///////////////////////////////////////////////////////////////////////////
DN_JSONItEntry *DN_JSON_ItCurr(DN_JSONIt *it)
{
    DN_JSONItEntry *result = nullptr;
    if (!it || it->stack_count <= 0)
        return result;

    result = &it->stack[it->stack_count - 1];
    return result;
}

json_value_s *DN_JSON_ItCurrValue(DN_JSONIt *it)
{
    json_value_s *result = nullptr;
    DN_JSONItEntry *curr = DN_JSON_ItCurr(it);
    if (!curr)
        return result;

    if (curr->type == DN_JSON_ItEntryTypeObjElement) {
        auto *element = DN_CAST(json_object_element_s *)curr->value;
        result = element->value;
    } else if (curr->type == DN_JSON_ItEntryTypeArrayElement) {
        auto *element = DN_CAST(json_array_element_s *)curr->value;
        result = element->value;
    } else if (curr->type == DN_JSON_ItEntryTypeString ||
               curr->type == DN_JSON_ItEntryTypeNumber ||
               curr->type == DN_JSON_ItEntryTypeObj    ||
               curr->type == DN_JSON_ItEntryTypeArray)
    {
        result = DN_CAST(json_value_s *)curr->value;
    }

    return result;
}

json_object_element_s *DN_JSON_ItCurrObjElement(DN_JSONIt *it)
{
    DN_JSONItEntry *curr   = DN_JSON_ItCurr(it);
    auto                  *result = (curr && curr->type == DN_JSON_ItEntryTypeObjElement)
                                        ? DN_CAST(json_object_element_s *) curr->value
                                        : nullptr;
    return result;
}

// NOTE: DN_JSON_ItValueIs ////////////////////////////////////////////////////////////////////////
json_value_s *DN_JSON_ItValueIs(DN_JSONIt *it, json_type_e type)
{
    json_value_s  *curr  = DN_JSON_ItCurrValue(it);
    json_value_s *result = (curr && type == curr->type) ? curr : nullptr;
    return result;
}

json_object_s *DN_JSON_ItValueIsObj(DN_JSONIt *it)
{
    json_value_s *curr    = DN_JSON_ItCurrValue(it);
    json_object_s *result = curr ? json_value_as_object(curr) : nullptr;
    return result;
}

json_array_s *DN_JSON_ItValueIsArray(DN_JSONIt *it)
{
    json_value_s *curr   = DN_JSON_ItCurrValue(it);
    json_array_s *result = curr ? json_value_as_array(curr) : nullptr;
    return result;
}

json_string_s *DN_JSON_ItValueIsString(DN_JSONIt *it)
{
    json_value_s *curr    = DN_JSON_ItCurrValue(it);
    json_string_s *result = curr ? json_value_as_string(curr) : nullptr;
    return result;
}

json_number_s *DN_JSON_ItValueIsNumber(DN_JSONIt *it)
{
    json_value_s *curr    = DN_JSON_ItCurrValue(it);
    json_number_s *result = curr ? json_value_as_number(curr) : nullptr;
    return result;
}

json_value_s *DN_JSON_ItValueIsBool(DN_JSONIt *it)
{
    json_value_s *curr   = DN_JSON_ItCurrValue(it);
    json_value_s *result = (curr && (curr->type == json_type_true || curr->type == json_type_false)) ? curr : nullptr;
    return result;
}

json_value_s *DN_JSON_ItValueIsNull(DN_JSONIt *it)
{
    json_value_s *curr   = DN_JSON_ItCurrValue(it);
    json_value_s *result = (curr && (curr->type == json_type_null)) ? curr : nullptr;
    return result;
}

size_t DN_JSON_ItValueArraySize(DN_JSONIt *it)
{
    size_t result = 0;
    if (json_array_s *curr = DN_JSON_ItValueIsArray(it))
        result = curr->length;
    return result;
}


// NOTE: DN_JSON_ItKeyValueIs /////////////////////////////////////////////////////////////////////
DN_Str8 DN_JSON_ItKey(DN_JSONIt *it)
{
    json_object_element_s *curr = DN_JSON_ItCurrObjElement(it);
    DN_Str8 result = {};
    if (curr) {
        result.data = DN_CAST(char *)curr->name->string;
        result.size = curr->name->string_size;
    }
    return result;
}

bool DN_JSON_ItKeyIs(DN_JSONIt *it, DN_Str8 key)
{
    json_object_element_s *curr = DN_JSON_ItCurrObjElement(it);
    bool result                 = DN_JSON_String8Cmp(curr->name, key);
    return result;
}

json_object_s *DN_JSON_ItKeyValueIsObj(DN_JSONIt *it, DN_Str8 key)
{
    json_object_s         *result = nullptr;
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = json_value_as_object(curr->value);
    return result;
}

json_array_s *DN_JSON_ItKeyValueIsArray(DN_JSONIt *it, DN_Str8 key)
{
    json_array_s          *result = nullptr;
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = json_value_as_array(curr->value);
    return result;
}

json_string_s *DN_JSON_ItKeyValueIsString(DN_JSONIt *it, DN_Str8 key)
{
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    json_string_s         *result = nullptr;
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = json_value_as_string(curr->value);
    return result;
}

json_number_s *DN_JSON_ItKeyValueIsNumber(DN_JSONIt *it, DN_Str8 key)
{
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    json_number_s         *result = nullptr;
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = json_value_as_number(curr->value);
    return result;
}

json_value_s *DN_JSON_ItKeyValueIsBool(DN_JSONIt *it, DN_Str8 key)
{
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    json_value_s          *result = nullptr;
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = curr->value->type == json_type_true || curr->value->type == json_type_false ? curr->value : nullptr;
    return result;
}

json_value_s *DN_JSON_ItKeyValueIsNull(DN_JSONIt *it, DN_Str8 key)
{
    json_object_element_s *curr   = DN_JSON_ItCurrObjElement(it);
    json_value_s          *result = nullptr;
    if (curr && DN_JSON_String8Cmp(curr->name, key))
        result = curr->value->type == json_type_null ? curr->value : nullptr;
    return result;
}


// NOTE: DN_JSON_ItValueTo ////////////////////////////////////////////////////////////////////////
DN_Str8 DN_JSON_ItValueToString(DN_JSONIt *it)
{
    DN_Str8 result = {};
    if (json_string_s *curr = DN_JSON_ItValueIsString(it))
        result = DN_Str8_Init(curr->string, curr->string_size);
    return result;
}

int64_t DN_JSON_ItValueToI64(DN_JSONIt *it)
{
    int64_t result = {};
    if (json_number_s *curr = DN_JSON_ItValueIsNumber(it))
        result = DN_Str8_ToI64(DN_Str8_Init(curr->number, curr->number_size), 0 /*separator*/).value;
    return result;
}

uint64_t DN_JSON_ItValueToU64(DN_JSONIt *it)
{
    uint64_t result = {};
    if (json_number_s *curr = DN_JSON_ItValueIsNumber(it))
        result = DN_Str8_ToU64(DN_Str8_Init(curr->number, curr->number_size), 0 /*separator*/).value;
    return result;
}

bool DN_JSON_ItValueToBool(DN_JSONIt *it)
{
    bool result = {};
    if (json_value_s *curr = DN_JSON_ItValueIsBool(it))
        result = curr->type == json_type_true;
    return result;
}

void DN_JSON_ItErrorUnknownKeyValue_(DN_JSONIt *it, DN_CallSite call_site)
{
    if (!it)
        return;

    json_object_element_s const *curr = DN_JSON_ItCurrObjElement(it);
    if (!curr)
        return;

    size_t value_type_size = 0;
    char const *value_type = DN_JSON_TypeEnumCString(DN_CAST(json_type_e)curr->value->type, &value_type_size);

    json_string_s const *key = curr->name;
    if (it->flags & json_parse_flags_allow_location_information) {
        json_string_ex_s const *info = DN_CAST(json_string_ex_s const *)key;
        DN_Log_TypeFCallSite(DN_LogType_Warning,
                              call_site,
                              "Unknown key-value pair in object [loc=%zu:%zu, key=%.*s, value=%.*s]",
                              info->line_no,
                              info->row_no,
                              DN_CAST(int)key->string_size,
                              key->string,
                              DN_CAST(int)value_type_size,
                              value_type);
    } else {
        DN_Log_TypeFCallSite(DN_LogType_Warning,
                              call_site,
                              "Unknown key-value pair in object [key=%.*s, value=%.*s]",
                              DN_CAST(int)key->string_size,
                              key->string,
                              DN_CAST(int)value_type_size,
                              value_type);
    }
}
