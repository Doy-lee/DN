#if !defined(DN_TYPE_INFO_H)
#define DN_TYPE_INFO_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
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
//    dn_type_info.h -- C++ type introspection
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

enum DN_TypeKind
{
  DN_TypeKind_Nil,
  DN_TypeKind_Basic,
  DN_TypeKind_Enum,
  DN_TypeKind_Struct,
};

struct DN_TypeField
{
  uint16_t            index;
  DN_Str8             name;
  DN_Str8             label;
  DN_ISize            value;
  DN_USize            offset_of;
  DN_USize            size_of;
  DN_USize            align_of;
  DN_Str8             type_decl;
  uint32_t            type_enum;
  bool                is_pointer;
  uint16_t            array_size;
  DN_TypeField const *array_size_field;
};

struct DN_TypeInfo
{
  DN_Str8             name;
  DN_TypeKind         kind;
  DN_USize            size_of;
  DN_TypeField const *fields;
  uint16_t            fields_count;
};

struct DN_TypeGetField
{
  bool          success;
  DN_USize      index;
  DN_TypeField *field;
};

DN_TypeGetField DN_Type_GetField(DN_TypeInfo const *type_info, DN_Str8 name);

#endif // !defined(DN_TYPE_INFO_H)

