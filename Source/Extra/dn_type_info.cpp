#define DN_TYPE_INFO_CPP

#if defined(_CLANGD)
  #include "dn_type_info.h"
#endif

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
//    dn_type_info.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_TypeGetField DN_Type_GetField(DN_TypeInfo const *type_info, DN_Str8 name)
{
    DN_TypeGetField result = {};
    for (DN_USize index = 0; index < type_info->fields_count; index++) {
        DN_TypeField const *type_field = type_info->fields + index;
        if (DN_Str8Eq(type_field->name, name)) {
            result.success = true;
            result.index   = index;
            result.field   = DN_Cast(DN_TypeField *)type_field;
            break;
        }
    }
    return result;
}
