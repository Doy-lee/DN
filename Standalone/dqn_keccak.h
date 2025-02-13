#if !defined(DN_KECCAK_H)
#define DN_KECCAK_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\ $$$$$$$$\  $$$$$$\   $$$$$$\   $$$$$$\  $$\   $$\
//   $$ | $$  |$$  _____|$$  __$$\ $$  __$$\ $$  __$$\ $$ | $$  |
//   $$ |$$  / $$ |      $$ /  \__|$$ /  \__|$$ /  $$ |$$ |$$  /
//   $$$$$  /  $$$$$\    $$ |      $$ |      $$$$$$$$ |$$$$$  /
//   $$  $$<   $$  __|   $$ |      $$ |      $$  __$$ |$$  $$<
//   $$ |\$$\  $$ |      $$ |  $$\ $$ |  $$\ $$ |  $$ |$$ |\$$\
//   $$ | \$$\ $$$$$$$$\ \$$$$$$  |\$$$$$$  |$$ |  $$ |$$ | \$$\
//   \__|  \__|\________| \______/  \______/ \__|  \__|\__|  \__|
//
//   dn_keccak.h -- FIPS202 SHA3 + non-finalized SHA3 (aka. Keccak) hashing algorithms
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Implementation of the Keccak hashing algorithms from the Keccak and SHA3
// families (including the FIPS202 published algorithms and the non-finalized
// ones, i.e. the ones used in Ethereum and Monero which adopted SHA3 before it
// was finalized. The only difference between the 2 is a different delimited
// suffix).
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MIT License
//
// Copyright (c) 2021 github.com/doy-lee
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$$$$$$\ $$$$$$$$\ $$$$$$\  $$$$$$\  $$\   $$\  $$$$$$\
//   $$  __$$\ $$  __$$\\__$$  __|\_$$  _|$$  __$$\ $$$\  $$ |$$  __$$\
//   $$ /  $$ |$$ |  $$ |  $$ |     $$ |  $$ /  $$ |$$$$\ $$ |$$ /  \__|
//   $$ |  $$ |$$$$$$$  |  $$ |     $$ |  $$ |  $$ |$$ $$\$$ |\$$$$$$\
//   $$ |  $$ |$$  ____/   $$ |     $$ |  $$ |  $$ |$$ \$$$$ | \____$$\
//   $$ |  $$ |$$ |        $$ |     $$ |  $$ |  $$ |$$ |\$$$ |$$\   $$ |
//    $$$$$$  |$$ |        $$ |   $$$$$$\  $$$$$$  |$$ | \$$ |\$$$$$$  |
//    \______/ \__|        \__|   \______| \______/ \__|  \__| \______/
//
//   Options -- Compile time build customisation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// - Define this in one and only one C++ file to enable the implementation
//   code of the header file.
//
//     #define DN_KECCAK_IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

#if !defined(DN_KECCAK_MEMCPY)
    #include <string.h>
    #define DN_KECCAK_MEMCPY(dest, src, count) memcpy(dest, src, count)
#endif

#if !defined(DN_KECCAK_MEMCMP)
    #include <string.h>
    #define DN_KECCAK_MEMCMP(dest, src, count) memcmp(dest, src, count)
#endif

#if !defined(DN_KECCAK_MEMSET)
    #include <string.h>
    #define DN_KECCAK_MEMSET(dest, byte, count) memset(dest, byte, count)
#endif

#if !defined(DN_KECCAK_ASSERT)
    #if defined(NDEBUG)
        #define DN_KECCAK_ASSERT(expr)
    #else
        #define DN_KECCAK_ASSERT(expr)                                                            \
            do                                                                                     \
            {                                                                                      \
                if (!(expr))                                                                       \
                {                                                                                  \
                    (*(volatile int *)0) = 0;                                                      \
                }                                                                                  \
            } while (0)
    #endif
#endif

// Use this macro in a printf-like function,
/*
    DN_KeccakString64 string = {};
    printf("%.*s\n", DN_KECCAK_STRING64_FMT(string));
*/
#define DN_KECCAK_STRING56_FMT(string) 56, string
#define DN_KECCAK_STRING64_FMT(string) 64, string
#define DN_KECCAK_STRING96_FMT(string) 96, string
#define DN_KECCAK_STRING128_FMT(string) 128, string

typedef unsigned char  DN_KeccakU8;
typedef unsigned short DN_KeccakU16;
typedef unsigned int   DN_KeccakU32;
typedef unsigned int   DN_KeccakUint;
#ifdef _MSC_VER
    typedef unsigned __int64 DN_KeccakU64;
#else
    typedef unsigned long long DN_KeccakU64;
#endif

typedef struct DN_KeccakBytes28  { char data[28]; } DN_KeccakBytes28; // 224 bit
typedef struct DN_KeccakBytes32  { char data[32]; } DN_KeccakBytes32; // 256 bit
typedef struct DN_KeccakBytes48  { char data[48]; } DN_KeccakBytes48; // 384 bit
typedef struct DN_KeccakBytes64  { char data[64]; } DN_KeccakBytes64; // 512 bit

typedef struct DN_KeccakString56  { char data[(sizeof(DN_KeccakBytes28) * 2) + 1]; } DN_KeccakString56;
typedef struct DN_KeccakString64  { char data[(sizeof(DN_KeccakBytes32) * 2) + 1]; } DN_KeccakString64;
typedef struct DN_KeccakString96  { char data[(sizeof(DN_KeccakBytes48) * 2) + 1]; } DN_KeccakString96;
typedef struct DN_KeccakString128 { char data[(sizeof(DN_KeccakBytes64) * 2) + 1]; } DN_KeccakString128;

#define DN_KECCAK_LANE_SIZE_U64 5
typedef struct DN_KeccakState
{
    DN_KeccakU8 state[DN_KECCAK_LANE_SIZE_U64 * DN_KECCAK_LANE_SIZE_U64 * sizeof(DN_KeccakU64)];
    int          state_size;       // The number of bytes written to the state
    int          absorb_size;      // The amount of bytes to absorb/sponge in/from the state
    int          hash_size_bits;        // The size of the hash the context was initialised for in bits
    char         delimited_suffix; // The delimited suffix of the current hash
} DN_KeccakState;

// NOTE: SHA3/Keccak Streaming API /////////////////////////////////////////////////////////////////
// Setup a hashing state for either
// - FIPS 202 SHA3
// - Non-finalized SHA3 (only difference is delimited suffix of 0x1 instead of 0x6 in SHA3)
// The non-finalized SHA3 version is the one adopted by many cryptocurrencies
// such as Ethereum and Monero as they adopted SHA3 before it was finalized.
//
// The state produced from this function is to be used alongside the
// 'KeccakUpdate' and 'KeccakFinish' functions.
//
// sha3: If true, setup the state for FIPS 202 SHA3, otherwise the non-finalized version
// hash_size_bits: The number of bits to setup the context for, available sizes are 224, 256, 384 and 512.
DN_KeccakState DN_KeccakSHA3Init(bool sha3, int hash_size_bits);

// After initialising a 'DN_KeccakState' via 'DN_KeccakSHA3Init', iteratively
// update the hash with new data by calling 'DN_KeccakUpdate'. On completion,
// call 'DN_KeccakFinish' to generate the hash from the state. The 'dest_size'
// must be at-least the (bit-size / 8), i.e. for 'DN_Keccak512Init' it must be
// atleast (512 / 8) bytes, 64 bytes.
void            DN_KeccakUpdate(DN_KeccakState *keccak, void const *data, DN_KeccakU64 data_size);
void            DN_KeccakFinish(DN_KeccakState *keccak, void *dest, int dest_size);

// NOTE: Simple API ////////////////////////////////////////////////////////////////////////////////
// Helper function that combines the Init, Update and Finish step in one shot,
// i.e. hashing a singlular contiguous buffer. Use the streaming API if data
// is split across different buffers.
void            DN_KeccakSHA3Hash(bool sha3, int hash_size_bits, void const *src, DN_KeccakU64 src_size, void *dest, int dest_size);

#define DN_SHA3Hash(hash_size_bits, src, src_size, dest, dest_size) DN_KeccakSHA3Hash(true /*sha3*/, hash_size_bits, src, src_size, dest, dest_size)
#define DN_SHA3_224(src, src_size, dest, dest_size) DN_SHA3Hash(224, src, src_size, dest, dest_size)
#define DN_SHA3_256(src, src_size, dest, dest_size) DN_SHA3Hash(256, src, src_size, dest, dest_size)
#define DN_SHA3_384(src, src_size, dest, dest_size) DN_SHA3Hash(384, src, src_size, dest, dest_size)
#define DN_SHA3_512(src, src_size, dest, dest_size) DN_SHA3Hash(512, src, src_size, dest, dest_size)

#define DN_KeccakHash(hash_size_bits, src, src_size, dest, dest_size) DN_KeccakSHA3Hash(false /*sha3*/, hash_size_bits, src, src_size, dest, dest_size)
#define DN_Keccak224(src, src_size, dest, dest_size) DN_KeccakHash(224, src, src_size, dest, dest_size)
#define DN_Keccak256(src, src_size, dest, dest_size) DN_KeccakHash(256, src, src_size, dest, dest_size)
#define DN_Keccak384(src, src_size, dest, dest_size) DN_KeccakHash(384, src, src_size, dest, dest_size)
#define DN_Keccak512(src, src_size, dest, dest_size) DN_KeccakHash(512, src, src_size, dest, dest_size)

// NOTE: SHA3 Helpers //////////////////////////////////////////////////////////////////////////////
DN_KeccakBytes28 DN_SHA3_224ToBytes28(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes32 DN_SHA3_256ToBytes32(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes48 DN_SHA3_384ToBytes48(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes64 DN_SHA3_512ToBytes64(void *bytes, DN_KeccakU64 bytes_size);

// NOTE: Keccak Helpers ////////////////////////////////////////////////////////////////////////////
DN_KeccakBytes28 DN_Keccak224ToBytes28(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes32 DN_Keccak256ToBytes32(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes48 DN_Keccak384ToBytes48(void *bytes, DN_KeccakU64 bytes_size);
DN_KeccakBytes64 DN_Keccak512ToBytes64(void *bytes, DN_KeccakU64 bytes_size);

#if defined(DN_H)
// NOTE: SHA3 - Helpers for DN data structures ////////////////////////////////////////////////////
DN_KeccakBytes28 DN_SHA3_224StringToBytes28(DN_Str8 string);
DN_KeccakBytes32 DN_SHA3_256StringToBytes32(DN_Str8 string);
DN_KeccakBytes48 DN_SHA3_384StringToBytes48(DN_Str8 string);
DN_KeccakBytes64 DN_SHA3_512StringToBytes64(DN_Str8 string);

// NOTE: Keccak - Helpers for DN data structures //////////////////////////////////////////////////
DN_KeccakBytes28 DN_Keccak224StringToBytes28(DN_Str8 string);
DN_KeccakBytes32 DN_Keccak256StringToBytes32(DN_Str8 string);
DN_KeccakBytes48 DN_Keccak384StringToBytes48(DN_Str8 string);
DN_KeccakBytes64 DN_Keccak512StringToBytes64(DN_Str8 string);
#endif // DN_H

// NOTE: Helper functions //////////////////////////////////////////////////////////////////////////
// Convert a binary buffer into its hex representation into dest. The dest
// buffer must be large enough to contain the hex representation, i.e.
// atleast src_size * 2). This function does *not* null-terminate the buffer.
void DN_KeccakBytesToHex(void const *src, DN_KeccakU64 src_size, char *dest, DN_KeccakU64 dest_size);

// Converts a fixed amount of bytes into a hexadecimal string. Helper functions
// that call into DN_KeccakBytesToHex.
// return: The hexadecimal string of the bytes, null-terminated.
DN_KeccakString56  DN_KeccakBytes28ToHex(DN_KeccakBytes28 const *bytes);
DN_KeccakString64  DN_KeccakBytes32ToHex(DN_KeccakBytes32 const *bytes);
DN_KeccakString96  DN_KeccakBytes48ToHex(DN_KeccakBytes48 const *bytes);
DN_KeccakString128 DN_KeccakBytes64ToHex(DN_KeccakBytes64 const *bytes);

// Compares byte data structures for byte equality (via memcmp).
// return: 1 if the contents are equal otherwise 0.
int DN_KeccakBytes28Equals(DN_KeccakBytes28 const *a, DN_KeccakBytes28 const *b);
int DN_KeccakBytes32Equals(DN_KeccakBytes32 const *a, DN_KeccakBytes32 const *b);
int DN_KeccakBytes48Equals(DN_KeccakBytes48 const *a, DN_KeccakBytes48 const *b);
int DN_KeccakBytes64Equals(DN_KeccakBytes64 const *a, DN_KeccakBytes64 const *b);

#if defined(DN_H) && defined(DN_WITH_HEX)
// NOTE: Other helper functions for DN data structures ////////////////////////////////////////////
// Converts a 64 character hex string into the 32 byte binary representation.
// Invalid hex characters in the string will be represented as 0.
// hex: Must be exactly a 64 character hex string.
DN_KeccakBytes32 DN_KeccakHex64StringToBytes(DN_Str8 hex);
#endif // DN_H && DN_WITH_HEX
#endif // DN_KECCAK_H

#if defined(DN_KECCAK_IMPLEMENTATION)
DN_KeccakU64 const DN_KECCAK_ROUNDS[] = {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808A, 0x8000000080008000, 0x000000000000808B,
    0x0000000080000001, 0x8000000080008081, 0x8000000000008009, 0x000000000000008A, 0x0000000000000088,
    0x0000000080008009, 0x000000008000000A, 0x000000008000808B, 0x800000000000008B, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 0x000000000000800A, 0x800000008000000A,
    0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
};

DN_KeccakU64 const DN_KECCAK_ROTATIONS[][5] =
{
    {0, 36, 3, 41, 18},
    {1, 44, 10, 45, 2},
    {62, 6, 43, 15, 61},
    {28, 55, 25, 21, 56},
    {27, 20, 39, 8, 14},
};

#define DN_KECCAK_ROL64(val, rotate) (((val) << (rotate)) | (((val) >> (64 - (rotate)))))

static void DN_Keccak__Permute(void *state)
{
    // TODO(dn): Do some profiling on unrolling and can we SIMD some part of
    // this? Unroll loop, look at data dependencies and investigate.

    DN_KeccakU64 *lanes_u64 = (DN_KeccakU64 *)state;
    for (int round_index = 0; round_index < 24; round_index++)
    {
        #define LANE_INDEX(x, y) ((x) + ((y) * DN_KECCAK_LANE_SIZE_U64))
        // ?? step //////////////////////////////////////////////////////////////////////////////////
#if 1
        DN_KeccakU64 c[DN_KECCAK_LANE_SIZE_U64];
        for (int x = 0; x < DN_KECCAK_LANE_SIZE_U64; x++)
        {
            c[x] = lanes_u64[LANE_INDEX(x, 0)] ^
                   lanes_u64[LANE_INDEX(x, 1)] ^
                   lanes_u64[LANE_INDEX(x, 2)] ^
                   lanes_u64[LANE_INDEX(x, 3)] ^
                   lanes_u64[LANE_INDEX(x, 4)];
        }

        DN_KeccakU64 d[DN_KECCAK_LANE_SIZE_U64];
        for (int x = 0; x < DN_KECCAK_LANE_SIZE_U64; x++)
            d[x] = c[(x + 4) % DN_KECCAK_LANE_SIZE_U64] ^ DN_KECCAK_ROL64(c[(x + 1) % DN_KECCAK_LANE_SIZE_U64], 1);

        for (int y = 0; y < DN_KECCAK_LANE_SIZE_U64; y++)
            for (int x = 0; x < DN_KECCAK_LANE_SIZE_U64; x++)
                lanes_u64[LANE_INDEX(x, y)] ^= d[x];
#else
        DN_KeccakU64 c[5], d[5];
        c[0] = lanes_u64[0 * 5 + 0] ^ lanes_u64[1 * 5 + 0] ^ lanes_u64[2 * 5 + 0] ^ lanes_u64[3 * 5 + 0] ^ lanes_u64[4 * 5 + 0];
        c[1] = lanes_u64[0 * 5 + 1] ^ lanes_u64[1 * 5 + 1] ^ lanes_u64[2 * 5 + 1] ^ lanes_u64[3 * 5 + 1] ^ lanes_u64[4 * 5 + 1];
        c[2] = lanes_u64[0 * 5 + 2] ^ lanes_u64[1 * 5 + 2] ^ lanes_u64[2 * 5 + 2] ^ lanes_u64[3 * 5 + 2] ^ lanes_u64[4 * 5 + 2];
        c[3] = lanes_u64[0 * 5 + 3] ^ lanes_u64[1 * 5 + 3] ^ lanes_u64[2 * 5 + 3] ^ lanes_u64[3 * 5 + 3] ^ lanes_u64[4 * 5 + 3];
        c[4] = lanes_u64[0 * 5 + 4] ^ lanes_u64[1 * 5 + 4] ^ lanes_u64[2 * 5 + 4] ^ lanes_u64[3 * 5 + 4] ^ lanes_u64[4 * 5 + 4];

        d[0] = c[4] ^ DN_KECCAK_ROL64(c[1], 1);
        d[1] = c[0] ^ DN_KECCAK_ROL64(c[2], 1);
        d[2] = c[1] ^ DN_KECCAK_ROL64(c[3], 1);
        d[3] = c[2] ^ DN_KECCAK_ROL64(c[4], 1);
        d[4] = c[3] ^ DN_KECCAK_ROL64(c[0], 1);
#endif

        // ?? and ?? steps ///////////////////////////////////////////////////////////////////////////
        DN_KeccakU64 b[DN_KECCAK_LANE_SIZE_U64 * DN_KECCAK_LANE_SIZE_U64];
        for (int y = 0; y < DN_KECCAK_LANE_SIZE_U64; y++)
        {
            for (int x = 0; x < DN_KECCAK_LANE_SIZE_U64; x++)
            {
                DN_KeccakU64 lane                          = lanes_u64[LANE_INDEX(x, y)];
                DN_KeccakU64 rotate_count                  = DN_KECCAK_ROTATIONS[x][y];
                b[LANE_INDEX(y, (2 * x + 3 * y) % 5)] = DN_KECCAK_ROL64(lane, rotate_count);

            }
        }

        // ?? step //////////////////////////////////////////////////////////////////////////////////
        for (int y = 0; y < DN_KECCAK_LANE_SIZE_U64; y++)
        {
            for (int x = 0; x < DN_KECCAK_LANE_SIZE_U64; x++)
            {
                DN_KeccakU64 rhs = ~b[LANE_INDEX((x + 1) % 5, y)] &
                               b[LANE_INDEX((x + 2) % 5, y)];

                lanes_u64[LANE_INDEX(x, y)] = b[LANE_INDEX(x, y)] ^ rhs;
            }
        }

        // ?? step //////////////////////////////////////////////////////////////////////////////////
        lanes_u64[LANE_INDEX(0, 0)] ^= DN_KECCAK_ROUNDS[round_index];
        #undef LANE_INDEX
        #undef DN_KECCAK_ROL64
    }
}

// NOTE: Streaming API /////////////////////////////////////////////////////////////////////////////
DN_KeccakState DN_KeccakSHA3Init(bool sha3, int hash_size_bits)
{
    char const SHA3_DELIMITED_SUFFIX   = 0x06;
    char const KECCAK_DELIMITED_SUFFIX = 0x01;
    int const  bitrate                 = 1600 - (hash_size_bits * 2);

#if defined(__cplusplus)
    DN_KeccakState result  = {};
#else
    DN_KeccakState result  = {0};
#endif
    result.hash_size_bits   = hash_size_bits;
    result.absorb_size      = bitrate / 8;
    result.delimited_suffix = sha3 ? SHA3_DELIMITED_SUFFIX : KECCAK_DELIMITED_SUFFIX;
    DN_KECCAK_ASSERT(bitrate + (hash_size_bits * 2) /*capacity*/ == 1600);
    return result;
}

void DN_KeccakUpdate(DN_KeccakState *keccak, void const *data, DN_KeccakU64 data_size)
{
    DN_KeccakU8 *state     = keccak->state;
    DN_KeccakU8 const *ptr = (DN_KeccakU8 *)data;
    DN_KeccakU64 ptr_size  = data_size;
    while (ptr_size > 0)
    {
        DN_KeccakU64 space = keccak->absorb_size - keccak->state_size;
        int bytes_to_absorb = (int)(space < ptr_size ? space : ptr_size);

        for (int index = 0; index < bytes_to_absorb; index++)
            state[keccak->state_size + index] ^= ptr[index];

        ptr                += bytes_to_absorb;
        keccak->state_size += bytes_to_absorb;
        ptr_size           -= bytes_to_absorb;

        if (keccak->state_size >= keccak->absorb_size)
        {
            DN_KECCAK_ASSERT(keccak->state_size == keccak->absorb_size);
            DN_Keccak__Permute(state);
            keccak->state_size = 0;
        }
    }
}

void DN_KeccakFinish(DN_KeccakState *keccak, void *dest, int dest_size)
{
    DN_KECCAK_ASSERT(dest_size >= keccak->hash_size_bits / 8);

    // Sponge Finalization Step: Final padding bit /////////////////////////////////////////////////
    int const INDEX_OF_0X80_BYTE     = keccak->absorb_size - 1;
    int const delimited_suffix_index = keccak->state_size;
    DN_KECCAK_ASSERT(delimited_suffix_index < keccak->absorb_size);

    DN_KeccakU8 *state            = keccak->state;
    state[delimited_suffix_index] ^= keccak->delimited_suffix;

    // NOTE: In the reference implementation, it checks that if the
    // delimited suffix is set to the padding bit (0x80), then we need to
    // permute twice. Once for the delimited suffix, and a second time for
    // the "padding" permute.
    //
    // However all standard algorithms either specify a 0x01, or 0x06, 0x04
    // delimited suffix and so forth- so this case is never hit. We can omit
    // this from the implementation here.

    state[INDEX_OF_0X80_BYTE] ^= 0x80;
    DN_Keccak__Permute(state);

    // Squeeze Step: Squeeze bytes from the state into our hash ////////////////////////////////////
    DN_KeccakU8 *dest_u8       = (DN_KeccakU8 *)dest;
    int const     squeeze_count = dest_size / keccak->absorb_size;
    int           squeeze_index = 0;
    for (; squeeze_index < squeeze_count; squeeze_index++)
    {
        if (squeeze_index) DN_Keccak__Permute(state);
        DN_KECCAK_MEMCPY(dest_u8, state, keccak->absorb_size);
        dest_u8 += keccak->absorb_size;
    }

    // Squeeze Finalisation Step: Remainder bytes in hash //////////////////////////////////////////
    int const remainder = dest_size % keccak->absorb_size;
    if (remainder)
    {
        if (squeeze_index) DN_Keccak__Permute(state);
        DN_KECCAK_MEMCPY(dest_u8, state, remainder);
    }
}

void DN_KeccakSHA3Hash(bool sha3, int hash_size_bits, void const *src, DN_KeccakU64 src_size, void *dest, int dest_size)
{
    DN_KeccakState keccak = DN_KeccakSHA3Init(sha3, hash_size_bits);
    DN_KeccakUpdate(&keccak, src, src_size);
    DN_KeccakFinish(&keccak, dest, dest_size);
}

// NOTE: SHA3 Helpers //////////////////////////////////////////////////////////////////////////////
DN_KeccakBytes28 DN_SHA3_224ToBytes28(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes28 result;
    DN_SHA3_224(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes32 DN_SHA3_256ToBytes32(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes32 result;
    DN_SHA3_256(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes48 DN_SHA3_384ToBytes48(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes48 result;
    DN_SHA3_384(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes64 DN_SHA3_512ToBytes64(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes64 result;
    DN_SHA3_512(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

// NOTE: Keccak Helpers ////////////////////////////////////////////////////////////////////////////
DN_KeccakBytes28 DN_Keccak224ToBytes28(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes28 result;
    DN_Keccak224(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes32 DN_Keccak256ToBytes32(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes32 result;
    DN_Keccak256(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes48 DN_Keccak384ToBytes48(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes48 result;
    DN_Keccak384(bytes, bytes_size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes64 DN_Keccak512ToBytes64(void *bytes, DN_KeccakU64 bytes_size)
{
    DN_KeccakBytes64 result;
    DN_Keccak512(bytes, bytes_size, result.data, sizeof(result));
    return result;
}


#if defined(DN_H)
// NOTE: SHA3 - Helpers for DN data structures ////////////////////////////////////////////////////
DN_KeccakBytes28 DN_SHA3_224StringToBytes28(DN_Str8 string)
{
    DN_KeccakBytes28 result;
    DN_SHA3_224(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes32 DN_SHA3_256StringToBytes32(DN_Str8 string)
{
    DN_KeccakBytes32 result;
    DN_SHA3_256(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes48 DN_SHA3_384StringToBytes48(DN_Str8 string)
{
    DN_KeccakBytes48 result;
    DN_SHA3_384(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes64 DN_SHA3_512StringToBytes64(DN_Str8 string)
{
    DN_KeccakBytes64 result;
    DN_SHA3_512(string.data, string.size, result.data, sizeof(result));
    return result;
}
#endif // DN_H

#if defined(DN_H)
// NOTE: Keccak - Helpers for DN data structures //////////////////////////////////////////////////
DN_KeccakBytes28 DN_Keccak224StringToBytes28(DN_Str8 string)
{
    DN_KeccakBytes28 result;
    DN_Keccak224(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes32 DN_Keccak256StringToBytes32(DN_Str8 string)
{
    DN_KeccakBytes32 result;
    DN_Keccak256(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes48 DN_Keccak384StringToBytes48(DN_Str8 string)
{
    DN_KeccakBytes48 result;
    DN_Keccak384(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KeccakBytes64 DN_Keccak512StringToBytes64(DN_Str8 string)
{
    DN_KeccakBytes64 result;
    DN_Keccak512(string.data, string.size, result.data, sizeof(result));
    return result;
}
#endif // DN_H

// NOTE: Helper functions //////////////////////////////////////////////////////////////////////////
void DN_KeccakBytesToHex(void const *src, DN_KeccakU64 src_size, char *dest, DN_KeccakU64 dest_size)
{
    (void)src_size; (void)dest_size;
    DN_KECCAK_ASSERT(dest_size >= src_size * 2);

    unsigned char *src_u8 = (unsigned char *)src;
    for (DN_KeccakU64 src_index = 0, dest_index = 0;
         src_index < src_size;
         src_index += 1, dest_index += 2)
    {
        char byte            = src_u8[src_index];
        char hex01           = (byte >> 4) & 0b1111;
        char hex02           = (byte >> 0) & 0b1111;
        dest[dest_index + 0] = hex01 < 10 ? (hex01 + '0') : (hex01 - 10) + 'a';
        dest[dest_index + 1] = hex02 < 10 ? (hex02 + '0') : (hex02 - 10) + 'a';
    }
}

DN_KeccakString56 DN_KeccakBytes28ToHex(DN_KeccakBytes28 const *bytes)
{
    DN_KeccakString56 result;
    DN_KeccakBytesToHex(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KeccakString64 DN_KeccakBytes32ToHex(DN_KeccakBytes32 const *bytes)
{
    DN_KeccakString64 result;
    DN_KeccakBytesToHex(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KeccakString96 DN_KeccakBytes48ToHex(DN_KeccakBytes48 const *bytes)
{
    DN_KeccakString96 result;
    DN_KeccakBytesToHex(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KeccakString128 DN_KeccakBytes64ToHex(DN_KeccakBytes64 const *bytes)
{
    DN_KeccakString128 result;
    DN_KeccakBytesToHex(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

int DN_KeccakBytes28Equals(DN_KeccakBytes28 const *a, DN_KeccakBytes28 const *b)
{
    int result = DN_KECCAK_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KeccakBytes32Equals(DN_KeccakBytes32 const *a, DN_KeccakBytes32 const *b)
{
    int result = DN_KECCAK_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KeccakBytes48Equals(DN_KeccakBytes48 const *a, DN_KeccakBytes48 const *b)
{
    int result = DN_KECCAK_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KeccakBytes64Equals(DN_KeccakBytes64 const *a, DN_KeccakBytes64 const *b)
{
    int result = DN_KECCAK_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

#if defined(DN_H) && defined(DN_WITH_HEX)
// NOTE: Other helper functions for DN data structures ////////////////////////////////////////////
DN_KeccakBytes32 DN_KeccakHex64StringToBytes(DN_Str8 hex)
{
    DN_KECCAK_ASSERT(hex.size == 64);
    DN_KeccakBytes32 result;
    DN_Hex_CString8ToByteBuffer(hex.data, hex.size, result.data, sizeof(result));
    return result;
}
#endif // DN_H && DN_WITH_HEX
#endif // DN_KECCAK_IMPLEMENTATION
