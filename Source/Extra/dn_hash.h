#if !defined(DN_HASH_H)
#define DN_HASH_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\  $$$$$$\   $$$$$$\  $$\   $$\
//   $$ |  $$ |$$  __$$\ $$  __$$\ $$ |  $$ |
//   $$ |  $$ |$$ /  $$ |$$ /  \__|$$ |  $$ |
//   $$$$$$$$ |$$$$$$$$ |\$$$$$$\  $$$$$$$$ |
//   $$  __$$ |$$  __$$ | \____$$\ $$  __$$ |
//   $$ |  $$ |$$ |  $$ |$$\   $$ |$$ |  $$ |
//   $$ |  $$ |$$ |  $$ |\$$$$$$  |$$ |  $$ |
//   \__|  \__|\__|  \__| \______/ \__|  \__|
//
//   dn_hash.h -- Hashing functions
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_FNV1A //////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_FNV1A32_SEED)
    #define DN_FNV1A32_SEED 2166136261U
#endif

#if !defined(DN_FNV1A64_SEED)
    #define DN_FNV1A64_SEED 14695981039346656037ULL
#endif

// NOTE: DN_MurmurHash3 ////////////////////////////////////////////////////////////////////////////
struct DN_MurmurHash3 { uint64_t e[2]; };

// NOTE: DN_FNV1A //////////////////////////////////////////////////////////////////////////////////
DN_API uint32_t       DN_FNV1A32_Hash            (void const *bytes, DN_USize size);
DN_API uint64_t       DN_FNV1A64_Hash            (void const *bytes, DN_USize size);
DN_API uint32_t       DN_FNV1A32_Iterate         (void const *bytes, DN_USize size, uint32_t hash);
DN_API uint64_t       DN_FNV1A64_Iterate         (void const *bytes, DN_USize size, uint64_t hash);

// NOTE: DN_MurmurHash3 ////////////////////////////////////////////////////////////////////////////
DN_API uint32_t       DN_MurmurHash3_x86U32      (void const *key, int len, uint32_t seed);
DN_API DN_MurmurHash3 DN_MurmurHash3_x64U128     (void const *key, int len, uint32_t seed);
#define               DN_MurmurHash3_x64U128AsU64(key, len, seed) (DN_MurmurHash3_x64U128(key, len, seed).e[0])
#define               DN_MurmurHash3_x64U128AsU32(key, len, seed) (DN_CAST(uint32_t)DN_MurmurHash3_x64U128(key, len, seed).e[0])

#endif // !defined(DN_HASH_H)
