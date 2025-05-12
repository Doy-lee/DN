#define DN_HASH_CPP

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
//   dn_hash.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_FNV1A //////////////////////////////////////////////////////////////////////////////////
// Default values recommended by: http://isthe.com/chongo/tech/comp/fnv/
DN_API uint32_t DN_FNV1A32_Iterate(void const *bytes, DN_USize size, uint32_t hash)
{
    auto buffer = DN_CAST(uint8_t const *)bytes;
    for (DN_USize i = 0; i < size; i++)
        hash = (buffer[i] ^ hash) * 16777619 /*FNV Prime*/;
    return hash;
}

DN_API uint32_t DN_FNV1A32_Hash(void const *bytes, DN_USize size)
{
    uint32_t result = DN_FNV1A32_Iterate(bytes, size, DN_FNV1A32_SEED);
    return result;
}

DN_API uint64_t DN_FNV1A64_Iterate(void const *bytes, DN_USize size, uint64_t hash)
{
    auto buffer = DN_CAST(uint8_t const *)bytes;
    for (DN_USize i = 0; i < size; i++)
        hash = (buffer[i] ^ hash) * 1099511628211 /*FNV Prime*/;
    return hash;
}

DN_API uint64_t DN_FNV1A64_Hash(void const *bytes, DN_USize size)
{
    uint64_t result = DN_FNV1A64_Iterate(bytes, size, DN_FNV1A64_SEED);
    return result;
}

// NOTE: DN_MurmurHash3 ////////////////////////////////////////////////////////////////////////////
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #define DN_MMH3_ROTL32(x, y) _rotl(x, y)
    #define DN_MMH3_ROTL64(x, y) _rotl64(x, y)
#else
    #define DN_MMH3_ROTL32(x, y) ((x) << (y)) | ((x) >> (32 - (y)))
    #define DN_MMH3_ROTL64(x, y) ((x) << (y)) | ((x) >> (64 - (y)))
#endif

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

DN_FORCE_INLINE uint32_t DN_MurmurHash3_GetBlock32(uint32_t const *p, int i)
{
    return p[i];
}

DN_FORCE_INLINE uint64_t DN_MurmurHash3_GetBlock64(uint64_t const *p, int i)
{
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

DN_FORCE_INLINE uint32_t DN_MurmurHash3_FMix32(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

DN_FORCE_INLINE uint64_t DN_MurmurHash3_FMix64(uint64_t k)
{
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
}

DN_API uint32_t DN_MurmurHash3_x86U32(void const *key, int len, uint32_t seed)
{
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks   = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body

    const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);

    for (int i = -nblocks; i; i++)
    {
        uint32_t k1 = DN_MurmurHash3_GetBlock32(blocks, i);

        k1 *= c1;
        k1 = DN_MMH3_ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = DN_MMH3_ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);

    uint32_t k1 = 0;

    switch (len & 3)
    {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = DN_MMH3_ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = DN_MurmurHash3_FMix32(h1);

    return h1;
}

DN_API DN_MurmurHash3 DN_MurmurHash3_x64U128(void const *key, int len, uint32_t seed)
{
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks   = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5;
    const uint64_t c2 = 0x4cf5ad432745937f;

    //----------
    // body

    const uint64_t *blocks = (const uint64_t *)(data);

    for (int i = 0; i < nblocks; i++)
    {
        uint64_t k1 = DN_MurmurHash3_GetBlock64(blocks, i * 2 + 0);
        uint64_t k2 = DN_MurmurHash3_GetBlock64(blocks, i * 2 + 1);

        k1 *= c1;
        k1 = DN_MMH3_ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = DN_MMH3_ROTL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = DN_MMH3_ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = DN_MMH3_ROTL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    //----------
    // tail

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 16);

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15)
    {
        case 15:
            k2 ^= ((uint64_t)tail[14]) << 48;
        case 14:
            k2 ^= ((uint64_t)tail[13]) << 40;
        case 13:
            k2 ^= ((uint64_t)tail[12]) << 32;
        case 12:
            k2 ^= ((uint64_t)tail[11]) << 24;
        case 11:
            k2 ^= ((uint64_t)tail[10]) << 16;
        case 10:
            k2 ^= ((uint64_t)tail[9]) << 8;
        case 9:
            k2 ^= ((uint64_t)tail[8]) << 0;
            k2 *= c2;
            k2 = DN_MMH3_ROTL64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case 8:
            k1 ^= ((uint64_t)tail[7]) << 56;
        case 7:
            k1 ^= ((uint64_t)tail[6]) << 48;
        case 6:
            k1 ^= ((uint64_t)tail[5]) << 40;
        case 5:
            k1 ^= ((uint64_t)tail[4]) << 32;
        case 4:
            k1 ^= ((uint64_t)tail[3]) << 24;
        case 3:
            k1 ^= ((uint64_t)tail[2]) << 16;
        case 2:
            k1 ^= ((uint64_t)tail[1]) << 8;
        case 1:
            k1 ^= ((uint64_t)tail[0]) << 0;
            k1 *= c1;
            k1 = DN_MMH3_ROTL64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = DN_MurmurHash3_FMix64(h1);
    h2 = DN_MurmurHash3_FMix64(h2);

    h1 += h2;
    h2 += h1;

    DN_MurmurHash3 result = {};
    result.e[0]            = h1;
    result.e[1]            = h2;
    return result;
}
