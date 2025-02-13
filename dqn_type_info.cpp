#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$$\ $$\     $$\ $$$$$$$\  $$$$$$$$\       $$$$$$\ $$\   $$\ $$$$$$$$\  $$$$$$\
//   \__$$  __|\$$\   $$  |$$  __$$\ $$  _____|      \_$$  _|$$$\  $$ |$$  _____|$$  __$$\
//      $$ |    \$$\ $$  / $$ |  $$ |$$ |              $$ |  $$$$\ $$ |$$ |      $$ /  $$ |
//      $$ |     \$$$$  /  $$$$$$$  |$$$$$\            $$ |  $$ $$\$$ |$$$$$\    $$ |  $$ |
//      $$ |      \$$  /   $$  ____/ $$  __|           $$ |  $$ \$$$$ |$$  __|   $$ |  $$ |
//      $$ |       $$ |    $$ |      $$ |              $$ |  $$ |\$$$ |$$ |      $$ |  $$ |
//      $$ |       $$ |    $$ |      $$$$$$$$\       $$$$$$\ $$ | \$$ |$$ |       $$$$$$  |
//      \__|       \__|    \__|      \________|      \______|\__|  \__|\__|       \______/
//
//    dqn_type_info.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_TypeGetField DN_Type_GetField(DN_TypeInfo const *type_info, DN_Str8 name)
{
    DN_TypeGetField result = {};
    for (DN_USize index = 0; index < type_info->fields_count; index++) {
        DN_TypeField const *type_field = type_info->fields + index;
        if (type_field->name == name) {
            result.success = true;
            result.index   = index;
            result.field   = DN_CAST(DN_TypeField *)type_field;
            break;
        }
    }
    return result;
}
