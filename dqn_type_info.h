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
//    dqn_type_info.h -- C++ type introspection
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

enum Dqn_TypeKind
{
    Dqn_TypeKind_Nil,
    Dqn_TypeKind_Basic,
    Dqn_TypeKind_Enum,
    Dqn_TypeKind_Struct,
};

struct Dqn_TypeField
{
    uint16_t                   index;
    Dqn_Str8                   name;
    Dqn_Str8                   label;
    Dqn_isize                  value;
    Dqn_usize                  offset_of;
    Dqn_usize                  size_of;
    Dqn_usize                  align_of;
    Dqn_Str8                   type_decl;
    uint32_t                   type_enum;
    bool                       is_pointer;
    uint16_t                   array_size;
    Dqn_TypeField const *      array_size_field;
};

struct Dqn_TypeInfo
{
    Dqn_Str8             name;
    Dqn_TypeKind         kind;
    Dqn_usize            size_of;
    Dqn_TypeField const *fields;
    uint16_t             fields_count;
};

struct Dqn_TypeGetField
{
    bool           success;
    Dqn_usize      index;
    Dqn_TypeField *field;
};

Dqn_TypeGetField Dqn_Type_GetField(Dqn_TypeInfo const *type_info, Dqn_Str8 name);
