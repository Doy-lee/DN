#if !defined(DN_KC_H)
#define DN_KC_H

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
//     #define DN_KC_IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

#include <stdint.h>

#if !defined(DN_KC_MEMCPY)
    #include <string.h>
    #define DN_KC_MEMCPY(dest, src, count) memcpy(dest, src, count)
#endif

#if !defined(DN_KC_MEMCMP)
    #include <string.h>
    #define DN_KC_MEMCMP(dest, src, count) memcmp(dest, src, count)
#endif

#if !defined(DN_KC_MEMSET)
    #include <string.h>
    #define DN_KC_MEMSET(dest, byte, count) memset(dest, byte, count)
#endif

#if !defined(DN_KC_ASSERT)
    #if defined(NDEBUG)
        #define DN_KC_ASSERT(expr)
    #else
        #define DN_KC_ASSERT(expr)                                                            \
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
    DN_KCString64 string = {};
    printf("%.*s\n", DN_KC_STRING64_FMT(string));
*/
#define DN_KC_STRING56_FMT(string) 56, string
#define DN_KC_STRING64_FMT(string) 64, string
#define DN_KC_STRING96_FMT(string) 96, string
#define DN_KC_STRING128_FMT(string) 128, string

typedef struct DN_KCBytes28  { char data[28]; } DN_KCBytes28; // 224 bit
typedef struct DN_KCBytes32  { char data[32]; } DN_KCBytes32; // 256 bit
typedef struct DN_KCBytes48  { char data[48]; } DN_KCBytes48; // 384 bit
typedef struct DN_KCBytes64  { char data[64]; } DN_KCBytes64; // 512 bit

typedef struct DN_KCString56  { char data[(sizeof(DN_KCBytes28) * 2) + 1]; } DN_KCString56;
typedef struct DN_KCString64  { char data[(sizeof(DN_KCBytes32) * 2) + 1]; } DN_KCString64;
typedef struct DN_KCString96  { char data[(sizeof(DN_KCBytes48) * 2) + 1]; } DN_KCString96;
typedef struct DN_KCString128 { char data[(sizeof(DN_KCBytes64) * 2) + 1]; } DN_KCString128;

#define DN_KC_LANE_SIZE_U64 5
typedef struct DN_KCState {
    uint8_t state[DN_KC_LANE_SIZE_U64 * DN_KC_LANE_SIZE_U64 * sizeof(uint64_t)];
    int     state_size;       // The number of bytes written to the state
    int     absorb_size;      // The amount of bytes to absorb/sponge in/from the state
    int     hash_size_bits;   // The size of the hash the context was initialised for in bits
    char    delimited_suffix; // The delimited suffix of the current hash
} DN_KCState;

// NOTE: SHA3/Keccak Streaming API /////////////////////////////////////////////////////////////////
// Setup a hashing state for either
// - FIPS 202 SHA3
// - Non-finalized SHA3 (only difference is delimited suffix of 0x1 instead of 0x6 in SHA3)
// The non-finalized SHA3 version is the one adopted by many cryptocurrencies
// such as Ethereum and Monero as they adopted SHA3 before it was finalized.
//
// The state produced from this function is to be used alongside the
// 'KeccakUpdate' and 'KC_HashFinish' functions.
//
// sha3: If true, setup the state for FIPS 202 SHA3, otherwise the non-finalized version
// hash_size_bits: The number of bits to setup the context for, available sizes are 224, 256, 384 and 512.
DN_KCState   DN_KC_HashInit(bool sha3, uint16_t hash_size_bits);

// After initialising a 'DN_KCState' via 'DN_KeccakSHA3Init', iteratively
// update the hash with new data by calling 'DN_KC_HashUpdate'. On completion,
// call 'DN_KC_HashFinish' to generate the hash from the state. The 'dest_size'
// must be at-least the (bit-size / 8), i.e. for 'DN_Keccak512Init' it must be
// atleast (512 / 8) bytes, 64 bytes.
void         DN_KC_HashUpdate(DN_KCState *keccak, void const *data, size_t data_size);
DN_KCBytes32 DN_KC_HashFinish(DN_KCState const *keccak);
void         DN_KC_HashFinishPtr(DN_KCState *keccak, void *dest, size_t dest_size);

// NOTE: Simple API ////////////////////////////////////////////////////////////////////////////////
// Helper function that combines the Init, Update and Finish step in one shot,
// i.e. hashing a singlular contiguous buffer. Use the streaming API if data
// is split across different buffers.
void         DN_KC_Hash(bool sha3, uint16_t hash_size_bits, void const *src, uint64_t src_size, void *dest, int dest_size);

// NOTE: SHA3 Helpers //////////////////////////////////////////////////////////////////////////////
// NOTE: SHA3 Streaming API
#define DN_KC_SHA3_224Init(src, size, dest, dest_size) DN_KC_HashInit(true /*sha3*/, 224)
#define DN_KC_SHA3_224Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_SHA3_224Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_SHA3_224FinishPtr(state, dest, size)     DN_KC_HashFinish(state, dest, size)

#define DN_KC_SHA3_256Init(src, size, dest, dest_size) DN_KC_HashInit(true /*sha3*/, 256)
#define DN_KC_SHA3_256Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_SHA3_256Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_SHA3_256FinishPtr(state, dest, size)     DN_KC_HashFinish(state, dest, size)

#define DN_KC_SHA3_384Init(src, size, dest, dest_size) DN_KC_HashInit(true /*sha3*/, 384)
#define DN_KC_SHA3_384Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_SHA3_384Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_SHA3_384FinishPtr(state, dest, size)     DN_KC_HashFinish(state, dest, size)

#define DN_KC_SHA3_512Init(src, size, dest, dest_size) DN_KC_HashInit(true /*sha3*/, 512)
#define DN_KC_SHA3_512Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_SHA3_512Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_SHA3_512FinishPtr(state, dest, size)     DN_KC_HashFinishPtr(state, dest, size)

// NOTE: SHA3 one-shot API
#define DN_KC_SHA3_224Ptr(src, size, dest, dest_size)  DN_KC_Hash(true /*sha3*/, 224, src, size, dest, dest_size)
#define DN_KC_SHA3_256Ptr(src, size, dest, dest_size)  DN_KC_Hash(true /*sha3*/, 256, src, size, dest, dest_size)
#define DN_KC_SHA3_384Ptr(src, size, dest, dest_size)  DN_KC_Hash(true /*sha3*/, 384, src, size, dest, dest_size)
#define DN_KC_SHA3_512Ptr(src, size, dest, dest_size)  DN_KC_Hash(true /*sha3*/, 512, src, size, dest, dest_size)

DN_KCBytes28 DN_KC_SHA3_224(void const *src, uint64_t size);
DN_KCBytes32 DN_KC_SHA3_256(void const *src, uint64_t size);
DN_KCBytes48 DN_KC_SHA3_384(void const *src, uint64_t size);
DN_KCBytes64 DN_KC_SHA3_512(void const *src, uint64_t size);

// NOTE: Keccak Helpers ////////////////////////////////////////////////////////////////////////////
// NOTE: SHA3 Streaming API
#define DN_KC_Keccak224Init()                           DN_KC_HashInit(false /*sha3*/, 224)
#define DN_KC_Keccak224Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_Keccak224Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_Keccak224FinishPtr(state, dest, size)     DN_KC_HashFinishPtr(state, dest, size)

#define DN_KC_Keccak256Init()                           DN_KC_HashInit(false /*sha3*/, 256)
#define DN_KC_Keccak256Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_Keccak256Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_Keccak256FinishPtr(state, dest, size)     DN_KC_HashFinishPtr(state, dest, size)

#define DN_KC_Keccak384Init()                           DN_KC_HashInit(false /*sha3*/, 384)
#define DN_KC_Keccak384Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_Keccak384Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_Keccak384FinishPtr(state, dest, size)     DN_KC_HashFinishPtr(state, dest, size)

#define DN_KC_Keccak512Init()                           DN_KC_HashInit(false /*sha3*/, 512)
#define DN_KC_Keccak512Update(state, src, size)         DN_KC_HashUpdate(state, src, size)
#define DN_KC_Keccak512Finish(state)                    DN_KC_HashFinish(state)
#define DN_KC_Keccak512FinishPtr(state, dest, size)     DN_KC_HashFinishPtr(state, dest, size)

// NOTE: SHA3 one-shot API
#define DN_KC_Keccak224Ptr(src, size, dest, dest_size)  DN_KC_Hash(false /*sha3*/, 224, src, size, dest, dest_size)
#define DN_KC_Keccak256Ptr(src, size, dest, dest_size)  DN_KC_Hash(false /*sha3*/, 256, src, size, dest, dest_size)
#define DN_KC_Keccak384Ptr(src, size, dest, dest_size)  DN_KC_Hash(false /*sha3*/, 384, src, size, dest, dest_size)
#define DN_KC_Keccak512Ptr(src, size, dest, dest_size)  DN_KC_Hash(false /*sha3*/, 512, src, size, dest, dest_size)

DN_KCBytes28 DN_KC_Keccak224(void const *src, uint64_t size);
DN_KCBytes32 DN_KC_Keccak256(void const *src, uint64_t size);
DN_KCBytes48 DN_KC_Keccak384(void const *src, uint64_t size);
DN_KCBytes64 DN_KC_Keccak512(void const *src, uint64_t size);

#if defined(DN_BASE_H)
// NOTE: SHA3 - Helpers for DN data structures ////////////////////////////////////////////////////
DN_KCBytes28 DN_KC_SHA3_224Str8(DN_Str8 string);
DN_KCBytes32 DN_KC_SHA3_256Str8(DN_Str8 string);
DN_KCBytes48 DN_KC_SHA3_384Str8(DN_Str8 string);
DN_KCBytes64 DN_KC_SHA3_512Str8(DN_Str8 string);

// NOTE: Keccak - Helpers for DN data structures //////////////////////////////////////////////////
DN_KCBytes28 DN_KC_Keccak224Str8(DN_Str8 string);
DN_KCBytes32 DN_KC_Keccak256Str8(DN_Str8 string);
DN_KCBytes48 DN_KC_Keccak384Str8(DN_Str8 string);
DN_KCBytes64 DN_KC_Keccak512Str8(DN_Str8 string);
#endif // DN_BASE_H

// NOTE: Helper functions //////////////////////////////////////////////////////////////////////////
// Convert a binary buffer into its hex representation into dest. The dest
// buffer must be large enough to contain the hex representation, i.e.
// atleast src_size * 2). This function does *not* null-terminate the buffer.
void DN_KC_HexFromBytes(void const *src, uint64_t src_size, char *dest, uint64_t dest_size);

// Converts a fixed amount of bytes into a hexadecimal string. Helper functions
// that call into DN_KCHexFromBytes.
// return: The hexadecimal string of the bytes, null-terminated.
DN_KCString56  DN_KC_Bytes28ToHex(DN_KCBytes28 const *bytes);
DN_KCString64  DN_KC_Bytes32ToHex(DN_KCBytes32 const *bytes);
DN_KCString96  DN_KC_Bytes48ToHex(DN_KCBytes48 const *bytes);
DN_KCString128 DN_KC_Bytes64ToHex(DN_KCBytes64 const *bytes);

// Compares byte data structures for byte equality (via memcmp).
// return: 1 if the contents are equal otherwise 0.
int DN_KC_Bytes28Equals(DN_KCBytes28 const *a, DN_KCBytes28 const *b);
int DN_KC_Bytes32Equals(DN_KCBytes32 const *a, DN_KCBytes32 const *b);
int DN_KC_Bytes48Equals(DN_KCBytes48 const *a, DN_KCBytes48 const *b);
int DN_KC_Bytes64Equals(DN_KCBytes64 const *a, DN_KCBytes64 const *b);

#if defined(DN_BASE_H)
// NOTE: Other helper functions for DN data structures ////////////////////////////////////////////
// Converts a 64 character hex string into the 32 byte binary representation.
// Invalid hex characters in the string will be represented as 0.
// hex: Must be exactly a 64 character hex string.
DN_KCBytes32 DN_KC_Hex64ToBytes(DN_Str8 hex);
#endif // DN_BASE_H
#endif // DN_KC_H

#if defined(DN_KC_IMPLEMENTATION)
uint64_t const DN_KC_ROUNDS[] = {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808A, 0x8000000080008000, 0x000000000000808B,
    0x0000000080000001, 0x8000000080008081, 0x8000000000008009, 0x000000000000008A, 0x0000000000000088,
    0x0000000080008009, 0x000000008000000A, 0x000000008000808B, 0x800000000000008B, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 0x000000000000800A, 0x800000008000000A,
    0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
};

uint64_t const DN_KC_ROTATIONS[][5] =
{
    {0, 36, 3, 41, 18},
    {1, 44, 10, 45, 2},
    {62, 6, 43, 15, 61},
    {28, 55, 25, 21, 56},
    {27, 20, 39, 8, 14},
};

#define DN_KC_ROL64(val, rotate) (((val) << (rotate)) | (((val) >> (64 - (rotate)))))

static void DN_KC_Permute_(void *state)
{
    // TODO(dn): Do some profiling on unrolling and can we SIMD some part of
    // this? Unroll loop, look at data dependencies and investigate.

    uint64_t *lanes_u64 = (uint64_t *)state;
    for (int round_index = 0; round_index < 24; round_index++)
    {
        #define LANE_INDEX(x, y) ((x) + ((y) * DN_KC_LANE_SIZE_U64))
        // ?? step //////////////////////////////////////////////////////////////////////////////////
#if 1
        uint64_t c[DN_KC_LANE_SIZE_U64];
        for (int x = 0; x < DN_KC_LANE_SIZE_U64; x++) {
            c[x] = lanes_u64[LANE_INDEX(x, 0)] ^
                   lanes_u64[LANE_INDEX(x, 1)] ^
                   lanes_u64[LANE_INDEX(x, 2)] ^
                   lanes_u64[LANE_INDEX(x, 3)] ^
                   lanes_u64[LANE_INDEX(x, 4)];
        }

        uint64_t d[DN_KC_LANE_SIZE_U64];
        for (int x = 0; x < DN_KC_LANE_SIZE_U64; x++)
            d[x] = c[(x + 4) % DN_KC_LANE_SIZE_U64] ^ DN_KC_ROL64(c[(x + 1) % DN_KC_LANE_SIZE_U64], 1);

        for (int y = 0; y < DN_KC_LANE_SIZE_U64; y++)
            for (int x = 0; x < DN_KC_LANE_SIZE_U64; x++)
                lanes_u64[LANE_INDEX(x, y)] ^= d[x];
#else
        uint64_t c[5], d[5];
        c[0] = lanes_u64[0 * 5 + 0] ^ lanes_u64[1 * 5 + 0] ^ lanes_u64[2 * 5 + 0] ^ lanes_u64[3 * 5 + 0] ^ lanes_u64[4 * 5 + 0];
        c[1] = lanes_u64[0 * 5 + 1] ^ lanes_u64[1 * 5 + 1] ^ lanes_u64[2 * 5 + 1] ^ lanes_u64[3 * 5 + 1] ^ lanes_u64[4 * 5 + 1];
        c[2] = lanes_u64[0 * 5 + 2] ^ lanes_u64[1 * 5 + 2] ^ lanes_u64[2 * 5 + 2] ^ lanes_u64[3 * 5 + 2] ^ lanes_u64[4 * 5 + 2];
        c[3] = lanes_u64[0 * 5 + 3] ^ lanes_u64[1 * 5 + 3] ^ lanes_u64[2 * 5 + 3] ^ lanes_u64[3 * 5 + 3] ^ lanes_u64[4 * 5 + 3];
        c[4] = lanes_u64[0 * 5 + 4] ^ lanes_u64[1 * 5 + 4] ^ lanes_u64[2 * 5 + 4] ^ lanes_u64[3 * 5 + 4] ^ lanes_u64[4 * 5 + 4];

        d[0] = c[4] ^ DN_KC_ROL64(c[1], 1);
        d[1] = c[0] ^ DN_KC_ROL64(c[2], 1);
        d[2] = c[1] ^ DN_KC_ROL64(c[3], 1);
        d[3] = c[2] ^ DN_KC_ROL64(c[4], 1);
        d[4] = c[3] ^ DN_KC_ROL64(c[0], 1);
#endif

        // ?? and ?? steps ///////////////////////////////////////////////////////////////////////////
        uint64_t b[DN_KC_LANE_SIZE_U64 * DN_KC_LANE_SIZE_U64];
        for (int y = 0; y < DN_KC_LANE_SIZE_U64; y++) {
            for (int x = 0; x < DN_KC_LANE_SIZE_U64; x++) {
                uint64_t lane                         = lanes_u64[LANE_INDEX(x, y)];
                uint64_t rotate_count                 = DN_KC_ROTATIONS[x][y];
                b[LANE_INDEX(y, (2 * x + 3 * y) % 5)] = DN_KC_ROL64(lane, rotate_count);
            }
        }

        // ?? step //////////////////////////////////////////////////////////////////////////////////
        for (int y = 0; y < DN_KC_LANE_SIZE_U64; y++) {
            for (int x = 0; x < DN_KC_LANE_SIZE_U64; x++) {
                uint64_t rhs = ~b[LANE_INDEX((x + 1) % 5, y)] & b[LANE_INDEX((x + 2) % 5, y)];

                lanes_u64[LANE_INDEX(x, y)] = b[LANE_INDEX(x, y)] ^ rhs;
            }
        }

        // ?? step //////////////////////////////////////////////////////////////////////////////////
        lanes_u64[LANE_INDEX(0, 0)] ^= DN_KC_ROUNDS[round_index];
        #undef LANE_INDEX
        #undef DN_KC_ROL64
    }
}

// NOTE: Streaming API /////////////////////////////////////////////////////////////////////////////
DN_KCState DN_KC_HashInit(bool sha3, uint16_t hash_size_bits)
{
    char const SHA3_DELIMITED_SUFFIX   = 0x06;
    char const KECCAK_DELIMITED_SUFFIX = 0x01;
    int const  bitrate                 = 1600 - (hash_size_bits * 2);

#if defined(__cplusplus)
    DN_KCState result  = {};
#else
    DN_KCState result  = {0};
#endif
    result.hash_size_bits   = hash_size_bits;
    result.absorb_size      = bitrate / 8;
    result.delimited_suffix = sha3 ? SHA3_DELIMITED_SUFFIX : KECCAK_DELIMITED_SUFFIX;
    DN_KC_ASSERT(bitrate + (hash_size_bits * 2) /*capacity*/ == 1600);
    return result;
}

void DN_KC_HashUpdate(DN_KCState *keccak, void const *data, size_t data_size)
{
    uint8_t *state     = keccak->state;
    uint8_t const *ptr = (uint8_t *)data;
    size_t ptr_size  = data_size;
    while (ptr_size > 0) {
        size_t space           = keccak->absorb_size - keccak->state_size;
        int    bytes_to_absorb = (int)(space < ptr_size ? space : ptr_size);

        for (int index = 0; index < bytes_to_absorb; index++)
            state[keccak->state_size + index] ^= ptr[index];

        ptr += bytes_to_absorb;
        keccak->state_size += bytes_to_absorb;
        ptr_size -= bytes_to_absorb;

        if (keccak->state_size >= keccak->absorb_size) {
            DN_KC_ASSERT(keccak->state_size == keccak->absorb_size);
            DN_KC_Permute_(state);
            keccak->state_size = 0;
        }
    }
}

DN_KCBytes32 DN_KC_HashFinish(DN_KCState *keccak)
{
    DN_KCBytes32 result = {};
    DN_KC_HashFinishPtr(keccak, result.data, sizeof(result.data));
    return result;
}

void DN_KC_HashFinishPtr(DN_KCState *keccak, void *dest, size_t dest_size)
{
    DN_KC_ASSERT(dest_size >= (size_t)(keccak->hash_size_bits / 8));

    // Sponge Finalization Step: Final padding bit /////////////////////////////////////////////////
    int const INDEX_OF_0X80_BYTE     = keccak->absorb_size - 1;
    int const delimited_suffix_index = keccak->state_size;
    DN_KC_ASSERT(delimited_suffix_index < keccak->absorb_size);

    uint8_t *state = keccak->state;
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
    DN_KC_Permute_(state);

    // Squeeze Step: Squeeze bytes from the state into our hash ////////////////////////////////////
    uint8_t     *dest_u8       = (uint8_t *)dest;
    size_t const squeeze_count = dest_size / keccak->absorb_size;
    size_t       squeeze_index = 0;
    for (; squeeze_index < squeeze_count; squeeze_index++) {
        if (squeeze_index)
            DN_KC_Permute_(state);
        DN_KC_MEMCPY(dest_u8, state, keccak->absorb_size);
        dest_u8 += keccak->absorb_size;
    }

    // Squeeze Finalisation Step: Remainder bytes in hash //////////////////////////////////////////
    int const remainder = dest_size % keccak->absorb_size;
    if (remainder) {
        if (squeeze_index)
            DN_KC_Permute_(state);
        DN_KC_MEMCPY(dest_u8, state, remainder);
    }
}

void DN_KC_Hash(bool sha3, uint16_t hash_size_bits, void const *src, size_t src_size, void *dest, size_t dest_size)
{
    DN_KCState state = DN_KC_HashInit(sha3, hash_size_bits);
    DN_KC_HashUpdate(&state, src, src_size);
    DN_KC_HashFinishPtr(&state, dest, dest_size);
}

// NOTE: SHA3 Helpers //////////////////////////////////////////////////////////////////////////////
DN_KCBytes28 DN_KC_SHA3_224(void const *src, size_t size)
{
    DN_KCBytes28 result;
    DN_KC_SHA3_224Ptr(src, size, result.data, sizeof(result.data));
    return result;
}

DN_KCBytes32 DN_KC_SHA3_256(void const *src, size_t size)
{
    DN_KCBytes32 result;
    DN_KC_SHA3_256Ptr(src, size, result.data, sizeof(result.data));
    return result;
}

DN_KCBytes48 DN_KC_SHA3_384(void const *src, size_t size)
{
    DN_KCBytes48 result;
    DN_KC_SHA3_384Ptr(src, size, result.data, sizeof(result.data));
    return result;
}

DN_KCBytes64 DN_KC_SHA3_512(void const *src, size_t size)
{
    DN_KCBytes64 result;
    DN_KC_SHA3_512Ptr(src, size, result.data, sizeof(result.data));
    return result;
}

// NOTE: Keccak Helpers ////////////////////////////////////////////////////////////////////////////
DN_KCBytes28 DN_KC_Keccak224(void const *src, size_t size)
{
    DN_KCBytes28 result;
    DN_KC_Keccak224Ptr(src, size, result.data, sizeof(result));
    return result;
}

DN_KCBytes32 DN_KC_Keccak256(void const *src, size_t size)
{
    DN_KCBytes32 result;
    DN_KC_Keccak256Ptr(src, size, result.data, sizeof(result));
    return result;
}

DN_KCBytes48 DN_KC_Keccak384(void const *src, size_t size)
{
    DN_KCBytes48 result;
    DN_KC_Keccak384Ptr(src, size, result.data, sizeof(result));
    return result;
}

DN_KCBytes64 DN_KC_Keccak512(void const *src, size_t size)
{
    DN_KCBytes64 result;
    DN_KC_Keccak512Ptr(src, size, result.data, sizeof(result));
    return result;
}

#if defined(DN_BASE_H)
// NOTE: SHA3 - Helpers for DN data structures ////////////////////////////////////////////////////
DN_KCBytes28 DN_KC_SHA3_224Str8(DN_Str8 string)
{
    DN_KCBytes28 result;
    DN_KC_SHA3_224Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes32 DN_KC_SHA3_256Str8(DN_Str8 string)
{
    DN_KCBytes32 result;
    DN_KC_SHA3_256Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes48 DN_KC_SHA3_384Str8(DN_Str8 string)
{
    DN_KCBytes48 result;
    DN_KC_SHA3_384Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes64 DN_KC_SHA3_512Str8(DN_Str8 string)
{
    DN_KCBytes64 result;
    DN_KC_SHA3_512Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}
#endif // DN_BASE_H

#if defined(DN_BASE_H)
// NOTE: Keccak - Helpers for DN data structures //////////////////////////////////////////////////
DN_KCBytes28 DN_KC_Keccak224Str8(DN_Str8 string)
{
    DN_KCBytes28 result;
    DN_KC_Keccak224Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes32 DN_KC_Keccak256Str8(DN_Str8 string)
{
    DN_KCBytes32 result;
    DN_KC_Keccak256Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes48 DN_KC_Keccak384Str8(DN_Str8 string)
{
    DN_KCBytes48 result;
    DN_KC_Keccak384Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}

DN_KCBytes64 DN_KC_Keccak512Str8(DN_Str8 string)
{
    DN_KCBytes64 result;
    DN_KC_Keccak512Ptr(string.data, string.size, result.data, sizeof(result));
    return result;
}
#endif // DN_BASE_H

// NOTE: Helper functions //////////////////////////////////////////////////////////////////////////
void DN_KC_HexFromBytes(void const *src, size_t src_size, char *dest, size_t dest_size)
{
    (void)src_size; (void)dest_size;
    DN_KC_ASSERT(dest_size >= src_size * 2);

    unsigned char *src_u8 = (unsigned char *)src;
    for (size_t src_index = 0, dest_index = 0; src_index < src_size;
         src_index += 1, dest_index += 2) {
        char byte            = src_u8[src_index];
        char hex01           = (byte >> 4) & 0b1111;
        char hex02           = (byte >> 0) & 0b1111;
        dest[dest_index + 0] = hex01 < 10 ? (hex01 + '0') : (hex01 - 10) + 'a';
        dest[dest_index + 1] = hex02 < 10 ? (hex02 + '0') : (hex02 - 10) + 'a';
    }
}

DN_KCString56 DN_KC_Bytes28ToHex(DN_KCBytes28 const *bytes)
{
    DN_KCString56 result;
    DN_KC_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KCString64 DN_KC_Bytes32ToHex(DN_KCBytes32 const *bytes)
{
    DN_KCString64 result;
    DN_KC_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KCString96 DN_KC_Bytes48ToHex(DN_KCBytes48 const *bytes)
{
    DN_KCString96 result;
    DN_KC_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

DN_KCString128 DN_KC_Bytes64ToHex(DN_KCBytes64 const *bytes)
{
    DN_KCString128 result;
    DN_KC_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
    result.data[sizeof(result.data) - 1] = 0;
    return result;
}

int DN_KC_Bytes28Equals(DN_KCBytes28 const *a, DN_KCBytes28 const *b)
{
    int result = DN_KC_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KC_Bytes32Equals(DN_KCBytes32 const *a, DN_KCBytes32 const *b)
{
    int result = DN_KC_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KC_Bytes48Equals(DN_KCBytes48 const *a, DN_KCBytes48 const *b)
{
    int result = DN_KC_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

int DN_KC_Bytes64Equals(DN_KCBytes64 const *a, DN_KCBytes64 const *b)
{
    int result = DN_KC_MEMCMP(a->data, b->data, sizeof(*a)) == 0;
    return result;
}

#if defined(DN_BASE_H)
// NOTE: Other helper functions for DN data structures ////////////////////////////////////////////
DN_KCBytes32 DN_KC_Hex64ToBytes(DN_Str8 hex)
{
    DN_KC_ASSERT(hex.size == 64);
    DN_KCBytes32 result;
    DN_BytesFromHexStr8(hex, result.data, sizeof(result));
    return result;
}
#endif // DN_BASE_H
#endif // DN_KC_IMPLEMENTATION
