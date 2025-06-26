#if !defined(DN_SIMD_AVX512F_H)
#define DN_SIMD_AVX512F_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$\    $$\ $$\   $$\        $$$$$$$\    $$\    $$$$$$\  $$$$$$$$\
//   $$  __$$\ $$ |   $$ |$$ |  $$ |       $$  ____| $$$$ |  $$  __$$\ $$  _____|
//   $$ /  $$ |$$ |   $$ |\$$\ $$  |       $$ |      \_$$ |  \__/  $$ |$$ |
//   $$$$$$$$ |\$$\  $$  | \$$$$  /$$$$$$\ $$$$$$$\    $$ |   $$$$$$  |$$$$$\
//   $$  __$$ | \$$\$$  /  $$  $$< \______|\_____$$\   $$ |  $$  ____/ $$  __|
//   $$ |  $$ |  \$$$  /  $$  /\$$\        $$\   $$ |  $$ |  $$ |      $$ |
//   $$ |  $$ |   \$  /   $$ /  $$ |       \$$$$$$  |$$$$$$\ $$$$$$$$\ $$ |
//   \__|  \__|    \_/    \__|  \__|        \______/ \______|\________|\__|
//
//   dn_avx512f.h -- Functions implemented w/ AVX512
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_API DN_Str8FindResult        DN_Str8_FindStr8AVX512F       (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8FindResult        DN_Str8_FindLastStr8AVX512F   (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BinarySplitResult DN_Str8_BinarySplitAVX512F    (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BinarySplitResult DN_Str8_BinarySplitLastAVX512F(DN_Str8 string, DN_Str8 find);
DN_API DN_USize                 DN_Str8_SplitAVX512F          (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Slice<DN_Str8>        DN_Str8_SplitAllocAVX512F     (DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);

#endif // DN_SIMD_AVX512F_H
