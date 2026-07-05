#if !defined(DN_SHA3_H)
#define DN_SHA3_H

// NOTE: DN_Sha3 -- FIPS202 SHA3 + non-finalized SHA3 (aka. Keccak) hashing algorithms

// Overview
//   Single header file implementation of the Keccak hashing algorithms from the Keccak and SHA3
//   families (including the FIPS202 published algorithms and the non-finalized ones, i.e. the ones
//   used in Ethereum and Monero which adopted SHA3 before it was finalized. The only difference
//   between the 2 is a different delimited suffix).

// Configuration
//   Define this in one and only one C++ file to enable the implementation code of the header file.
//
//     #define DN_SHA3_IMPLEMENTATION
//
//   Define this to enable unit tests in the implementation, it requires dn.h to be visible in the
//
//     #define DN_SHA3_WITH_TESTS

// License
//   MIT License
//
//   Copyright (c) 2021 github.com/doy-lee
//
//   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//   and associated documentation files (the "Software"), to deal in the Software without
//   restriction, including without limitation the rights to use, copy, modify, merge, publish,
//   distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
//   Software is furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in all copies or
//   substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//   BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdint.h>
#if !defined(DN_SHA3_Memcpy)
  #include <string.h>
  #define DN_SHA3_Memcpy(dest, src, count) memcpy(dest, src, count)
#endif

#if !defined(DN_SHA3_Memcmp)
  #include <string.h>
  #define DN_SHA3_Memcmp(dest, src, count) memcmp(dest, src, count)
#endif

#if !defined(DN_SHA3_Memset)
  #include <string.h>
  #define DN_SHA3_Memset(dest, byte, count) memset(dest, byte, count)
#endif

#if !defined(DN_SHA3_Assert)
  #if defined(NDEBUG)
    #define DN_SHA3_Assert(expr)
  #else
    #define DN_SHA3_Assert(expr)     \
      do {                          \
        if (!(expr)) {              \
          (*(volatile int *)0) = 0; \
        }                           \
      } while (0)
  #endif
#endif

typedef struct DN_SHA3U8x28    { char data[28];                             } DN_SHA3U8x28; // 224 bit
typedef struct DN_SHA3U8x32    { char data[32];                             } DN_SHA3U8x32; // 256 bit
typedef struct DN_SHA3U8x48    { char data[48];                             } DN_SHA3U8x48; // 384 bit
typedef struct DN_SHA3U8x64    { char data[64];                             } DN_SHA3U8x64; // 512 bit
typedef struct DN_SHA3Str8x56  { char data[(sizeof(DN_SHA3U8x28) * 2) + 1]; } DN_SHA3Str8x56;
typedef struct DN_SHA3Str8x64  { char data[(sizeof(DN_SHA3U8x32) * 2) + 1]; } DN_SHA3Str8x64;
typedef struct DN_SHA3Str8x96  { char data[(sizeof(DN_SHA3U8x48) * 2) + 1]; } DN_SHA3Str8x96;
typedef struct DN_SHA3Str8x128 { char data[(sizeof(DN_SHA3U8x64) * 2) + 1]; } DN_SHA3Str8x128;

#define DN_SHA3_LANE_SIZE_U64 5
typedef struct DN_SHA3State {
  size_t  hash_size_bits;   // The size of the hash the context was initialised for in bits
  size_t  state_size;       // The number of bytes written to the state
  size_t  absorb_size;      // The amount of bytes to absorb/sponge in/from the state
  uint8_t state[DN_SHA3_LANE_SIZE_U64 * DN_SHA3_LANE_SIZE_U64 * sizeof(uint64_t)];
  char    delimited_suffix; // The delimited suffix of the current hash
} DN_SHA3State;

enum DN_SHA3Type
{
  DN_SHA3Type_SHA3,   // FIPS 202 SHA3 (delimited suffix is 0x6)
  DN_SHA3Type_Keccak, // Non-finalized SHA3 (only difference is delimited suffix of 0x1 instead of 0x6)
};

// hash_size_bits: Number of bits to hash to. Available sizes are 224, 256, 384 and 512.
DN_SHA3State    DN_SHA3_Init             (DN_SHA3Type type, size_t hash_size_bits);
DN_SHA3State    DN_SHA3_InitSHA3         (size_t hash_size_bits);
DN_SHA3State    DN_SHA3_InitKeccak       (size_t hash_size_bits);
void            DN_SHA3_Update           (DN_SHA3State *sha3, void const *data, size_t data_size);
void            DN_SHA3_Finish           (DN_SHA3State *sha3, void *dest, size_t dest_size);
void            DN_SHA3_Hash             (DN_SHA3Type type, size_t hash_size_bits, void const *src, size_t src_size, void *dest, int dest_size);

void            DN_SHA3_Hash224bPtr      (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x28    DN_SHA3_Hash224b         (void const *src, size_t src_size);
void            DN_SHA3_Hash256bPtr      (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x32    DN_SHA3_Hash256b         (void const *src, size_t src_size);
void            DN_SHA3_Hash384bPtr      (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x48    DN_SHA3_Hash384b         (void const *src, size_t src_size);
void            DN_SHA3_Hash512bPtr      (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x64    DN_SHA3_Hash512b         (void const *src, size_t src_size);

void            DN_SHA3_HashKeccak224bPtr(void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x28    DN_SHA3_HashKeccak224b   (void const *src, size_t src_size);
void            DN_SHA3_HashKeccak256bPtr(void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x32    DN_SHA3_HashKeccak256b   (void const *src, size_t src_size);
void            DN_SHA3_HashKeccak384bPtr(void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x48    DN_SHA3_HashKeccak384b   (void const *src, size_t src_size);
void            DN_SHA3_HashKeccak512bPtr(void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x64    DN_SHA3_HashKeccak512b   (void const *src, size_t src_size);

void            DN_SHA3_HexFromBytes     (void const *src, uint64_t src_size, char *dest, uint64_t dest_size);
DN_SHA3Str8x56  DN_SHA3_HexFromU8x28     (DN_SHA3U8x28 const *bytes);
DN_SHA3Str8x64  DN_SHA3_HexFromU8x32     (DN_SHA3U8x32 const *bytes);
DN_SHA3Str8x96  DN_SHA3_HexFromU8x48     (DN_SHA3U8x48 const *bytes);
DN_SHA3Str8x128 DN_SHA3_HexFromU8x64     (DN_SHA3U8x64 const *bytes);
bool            DN_SHA3_U8x28Eq          (DN_SHA3U8x28 const *a, DN_SHA3U8x28 const *b);
bool            DN_SHA3_U8x32Eq          (DN_SHA3U8x32 const *a, DN_SHA3U8x32 const *b);
bool            DN_SHA3_U8x48Eq          (DN_SHA3U8x48 const *a, DN_SHA3U8x48 const *b);
bool            DN_SHA3_U8x64Eq          (DN_SHA3U8x64 const *a, DN_SHA3U8x64 const *b);

#if defined(DN_SHA3_WITH_TESTS)
DN_TestCore     DN_SHA3_TestSuite(DN_Arena *arena);
void            DN_SHA3_TestSuiteThenOutput(DN_Str8FromTestCoreFlags flags);
#endif
#endif // DN_SHA3_H

#if defined(DN_SHA3_IMPLEMENTATION)
uint64_t const DN_SHA3_ROUNDS[] = {
  0x0000000000000001, 0x0000000000008082, 0x800000000000808A, 0x8000000080008000, 0x000000000000808B,
  0x0000000080000001, 0x8000000080008081, 0x8000000000008009, 0x000000000000008A, 0x0000000000000088,
  0x0000000080008009, 0x000000008000000A, 0x000000008000808B, 0x800000000000008B, 0x8000000000008089,
  0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 0x000000000000800A, 0x800000008000000A,
  0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
};

uint64_t const DN_SHA3_ROTATIONS[][5] =
{
  {0, 36, 3, 41, 18},
  {1, 44, 10, 45, 2},
  {62, 6, 43, 15, 61},
  {28, 55, 25, 21, 56},
  {27, 20, 39, 8, 14},
};

#define DN_SHA3_ROL64(val, rotate) (((val) << (rotate)) | (((val) >> (64 - (rotate)))))
static void DN_SHA3_Permute_(void *state)
{
  uint64_t *lanes_u64 = (uint64_t *)state;
  for (int round_index = 0; round_index < 24; round_index++) {
  #define DN_SHA3_LANE_INDEX(x, y) ((x) + ((y) * DN_SHA3_LANE_SIZE_U64))
  // ?? step
  #if 1
    uint64_t c[DN_SHA3_LANE_SIZE_U64];
    for (int x = 0; x < DN_SHA3_LANE_SIZE_U64; x++)
      c[x] = lanes_u64[DN_SHA3_LANE_INDEX(x, 0)] ^
             lanes_u64[DN_SHA3_LANE_INDEX(x, 1)] ^
             lanes_u64[DN_SHA3_LANE_INDEX(x, 2)] ^
             lanes_u64[DN_SHA3_LANE_INDEX(x, 3)] ^
             lanes_u64[DN_SHA3_LANE_INDEX(x, 4)];

    uint64_t d[DN_SHA3_LANE_SIZE_U64];
    for (int x = 0; x < DN_SHA3_LANE_SIZE_U64; x++)
      d[x] = c[(x + 4) % DN_SHA3_LANE_SIZE_U64] ^ DN_SHA3_ROL64(c[(x + 1) % DN_SHA3_LANE_SIZE_U64], 1);

    for (int y = 0; y < DN_SHA3_LANE_SIZE_U64; y++)
      for (int x = 0; x < DN_SHA3_LANE_SIZE_U64; x++)
        lanes_u64[DN_SHA3_LANE_INDEX(x, y)] ^= d[x];
  #else
    uint64_t c[5], d[5];
    c[0] = lanes_u64[0 * 5 + 0] ^ lanes_u64[1 * 5 + 0] ^ lanes_u64[2 * 5 + 0] ^ lanes_u64[3 * 5 + 0] ^ lanes_u64[4 * 5 + 0];
    c[1] = lanes_u64[0 * 5 + 1] ^ lanes_u64[1 * 5 + 1] ^ lanes_u64[2 * 5 + 1] ^ lanes_u64[3 * 5 + 1] ^ lanes_u64[4 * 5 + 1];
    c[2] = lanes_u64[0 * 5 + 2] ^ lanes_u64[1 * 5 + 2] ^ lanes_u64[2 * 5 + 2] ^ lanes_u64[3 * 5 + 2] ^ lanes_u64[4 * 5 + 2];
    c[3] = lanes_u64[0 * 5 + 3] ^ lanes_u64[1 * 5 + 3] ^ lanes_u64[2 * 5 + 3] ^ lanes_u64[3 * 5 + 3] ^ lanes_u64[4 * 5 + 3];
    c[4] = lanes_u64[0 * 5 + 4] ^ lanes_u64[1 * 5 + 4] ^ lanes_u64[2 * 5 + 4] ^ lanes_u64[3 * 5 + 4] ^ lanes_u64[4 * 5 + 4];

    d[0] = c[4] ^ DN_SHA3_ROL64(c[1], 1);
    d[1] = c[0] ^ DN_SHA3_ROL64(c[2], 1);
    d[2] = c[1] ^ DN_SHA3_ROL64(c[3], 1);
    d[3] = c[2] ^ DN_SHA3_ROL64(c[4], 1);
    d[4] = c[3] ^ DN_SHA3_ROL64(c[0], 1);
  #endif

    // ?? and ?? steps
    uint64_t b[DN_SHA3_LANE_SIZE_U64 * DN_SHA3_LANE_SIZE_U64];
    for (int y = 0; y < DN_SHA3_LANE_SIZE_U64; y++) {
      for (int x = 0; x < DN_SHA3_LANE_SIZE_U64; x++) {
        uint64_t lane                                 = lanes_u64[DN_SHA3_LANE_INDEX(x, y)];
        uint64_t rotate_count                         = DN_SHA3_ROTATIONS[x][y];
        b[DN_SHA3_LANE_INDEX(y, (2 * x + 3 * y) % 5)] = DN_SHA3_ROL64(lane, rotate_count);
      }
    }

    // ?? step
    for (int y = 0; y < DN_SHA3_LANE_SIZE_U64; y++) {
      for (int x = 0; x < DN_SHA3_LANE_SIZE_U64; x++) {
        uint64_t rhs = ~b[DN_SHA3_LANE_INDEX((x + 1) % 5, y)] & b[DN_SHA3_LANE_INDEX((x + 2) % 5, y)];

        lanes_u64[DN_SHA3_LANE_INDEX(x, y)] = b[DN_SHA3_LANE_INDEX(x, y)] ^ rhs;
      }
    }

    // ?? step
    lanes_u64[DN_SHA3_LANE_INDEX(0, 0)] ^= DN_SHA3_ROUNDS[round_index];
  #undef DN_SHA3_LANE_INDEX
  #undef DN_SHA3_ROL64
  }
}

DN_SHA3State DN_SHA3_Init(DN_SHA3Type type, size_t hash_size_bits)
{
  DN_SHA3_Assert(hash_size_bits == 224 ||
                 hash_size_bits == 256 ||
                 hash_size_bits == 384 ||
                 hash_size_bits == 512);

  char const SHA3_DELIMITED_SUFFIX   = 0x06;
  char const KECCAK_DELIMITED_SUFFIX = 0x01;
  size_t const  bitrate              = 1600 - (hash_size_bits * 2);

  #if defined(__cplusplus)
  DN_SHA3State result = {};
  #else
  DN_SHA3State result = {0};
  #endif
  result.hash_size_bits   = hash_size_bits;
  result.absorb_size      = bitrate / 8;
  result.delimited_suffix = type == DN_SHA3Type_SHA3 ? SHA3_DELIMITED_SUFFIX : KECCAK_DELIMITED_SUFFIX;
  DN_SHA3_Assert(bitrate + (hash_size_bits * 2) /*capacity*/ == 1600);
  return result;
}

DN_SHA3State DN_SHA3_InitSHA3(size_t hash_size_bits)
{
  DN_SHA3State result = DN_SHA3_Init(DN_SHA3Type_SHA3, hash_size_bits);
  return result;
}

DN_SHA3State DN_SHA3_InitKeccak(size_t hash_size_bits)
{
  DN_SHA3State result = DN_SHA3_Init(DN_SHA3Type_Keccak, hash_size_bits);
  return result;
}

void DN_SHA3_Update(DN_SHA3State *sha3, void const *data, size_t data_size)
{
  uint8_t       *state    = sha3->state;
  uint8_t const *ptr      = (uint8_t *)data;
  size_t         ptr_size = data_size;
  while (ptr_size > 0) {
    size_t space           = sha3->absorb_size - sha3->state_size;
    int    bytes_to_absorb = (int)(space < ptr_size ? space : ptr_size);

    for (int index = 0; index < bytes_to_absorb; index++)
      state[sha3->state_size + index] ^= ptr[index];

    ptr += bytes_to_absorb;
    sha3->state_size += bytes_to_absorb;
    ptr_size -= bytes_to_absorb;

    if (sha3->state_size >= sha3->absorb_size) {
      DN_SHA3_Assert(sha3->state_size == sha3->absorb_size);
      DN_SHA3_Permute_(state);
      sha3->state_size = 0;
    }
  }
}

void DN_SHA3_Finish(DN_SHA3State *sha3, void *dest, size_t dest_size)
{
  DN_SHA3_Assert(dest_size >= (size_t)(sha3->hash_size_bits / 8));

  // Sponge Finalization Step: Final padding bit
  size_t const INDEX_OF_0X80_BYTE     = sha3->absorb_size - 1;
  size_t const delimited_suffix_index = sha3->state_size;
  DN_SHA3_Assert(delimited_suffix_index < sha3->absorb_size);

  uint8_t *state = sha3->state;
  state[delimited_suffix_index] ^= sha3->delimited_suffix;

  // NOTE: In the reference implementation, it checks that if the
  // delimited suffix is set to the padding bit (0x80), then we need to
  // permute twice. Once for the delimited suffix, and a second time for
  // the "padding" permute.
  //
  // However all standard algorithms either specify a 0x01, or 0x06, 0x04
  // delimited suffix and so forth- so this case is never hit. We can omit
  // this from the implementation here.

  state[INDEX_OF_0X80_BYTE] ^= 0x80;
  DN_SHA3_Permute_(state);

  // Squeeze Step: Squeeze bytes from the state into our hash
  uint8_t     *dest_u8       = (uint8_t *)dest;
  size_t const squeeze_count = dest_size / sha3->absorb_size;
  size_t       squeeze_index = 0;
  for (; squeeze_index < squeeze_count; squeeze_index++) {
    if (squeeze_index)
      DN_SHA3_Permute_(state);
    DN_SHA3_Memcpy(dest_u8, state, sha3->absorb_size);
    dest_u8 += sha3->absorb_size;
  }

  // Squeeze Finalisation Step: Remainder bytes in hash
  size_t const remainder = dest_size % sha3->absorb_size;
  if (remainder) {
    if (squeeze_index)
      DN_SHA3_Permute_(state);
    DN_SHA3_Memcpy(dest_u8, state, remainder);
  }
}

void DN_SHA3_Hash(DN_SHA3Type type, size_t hash_size_bits, void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3State state = DN_SHA3_Init(type, hash_size_bits);
  DN_SHA3_Update(&state, src, src_size);
  DN_SHA3_Finish(&state, dest, dest_size);
}

void DN_SHA3_Hash224bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_SHA3, /*hash_size_bits=*/ 224, src, src_size, dest, dest_size);
}

DN_SHA3U8x28 DN_SHA3_Hash224b(void const *src, size_t src_size)
{
  DN_SHA3U8x28 result = {};
  DN_SHA3_Hash224bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_Hash256bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_SHA3, /*hash_size_bits=*/ 256, src, src_size, dest, dest_size);
}

DN_SHA3U8x32 DN_SHA3_Hash256b(void const *src, size_t src_size)
{
  DN_SHA3U8x32 result = {};
  DN_SHA3_Hash256bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_Hash384bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_SHA3, /*hash_size_bits=*/ 384, src, src_size, dest, dest_size);
}

DN_SHA3U8x48 DN_SHA3_Hash384b(void const *src, size_t src_size)
{
  DN_SHA3U8x48 result = {};
  DN_SHA3_Hash384bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_Hash512bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_SHA3, /*hash_size_bits=*/ 512, src, src_size, dest, dest_size);
}

DN_SHA3U8x64 DN_SHA3_Hash512b(void const *src, size_t src_size)
{
  DN_SHA3U8x64 result = {};
  DN_SHA3_Hash512bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_HashKeccak224bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_Keccak, /*hash_size_bits=*/ 224, src, src_size, dest, dest_size);
}

DN_SHA3U8x28 DN_SHA3_HashKeccak224b(void const *src, size_t src_size)
{
  DN_SHA3U8x28 result = {};
  DN_SHA3_HashKeccak224bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_HashKeccak256bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_Keccak, /*hash_size_bits=*/ 256, src, src_size, dest, dest_size);
}

DN_SHA3U8x32 DN_SHA3_HashKeccak256b(void const *src, size_t src_size)
{
  DN_SHA3U8x32 result = {};
  DN_SHA3_HashKeccak256bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_HashKeccak384bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_Keccak, /*hash_size_bits=*/ 384, src, src_size, dest, dest_size);
}

DN_SHA3U8x48 DN_SHA3_HashKeccak384b(void const *src, size_t src_size)
{
  DN_SHA3U8x48 result = {};
  DN_SHA3_HashKeccak384bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_HashKeccak512bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3_Hash(DN_SHA3Type_Keccak, /*hash_size_bits=*/ 512, src, src_size, dest, dest_size);
}

DN_SHA3U8x64 DN_SHA3_HashKeccak512b(void const *src, size_t src_size)
{
  DN_SHA3U8x64 result = {};
  DN_SHA3_HashKeccak512bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3_HexFromBytes(void const *src, size_t src_size, char *dest, size_t dest_size)
{
  (void)src_size;
  (void)dest_size;
  DN_SHA3_Assert(dest_size >= src_size * 2);

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

DN_SHA3Str8x56 DN_SHA3_HexFromU8x28(DN_SHA3U8x28 const *bytes)
{
  DN_SHA3Str8x56 result;
  DN_SHA3_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x64 DN_SHA3_HexFromU8x32(DN_SHA3U8x32 const *bytes)
{
  DN_SHA3Str8x64 result;
  DN_SHA3_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x96 DN_SHA3_HexFromU8x48(DN_SHA3U8x48 const *bytes)
{
  DN_SHA3Str8x96 result;
  DN_SHA3_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x128 DN_SHA3_HexFromU8x64(DN_SHA3U8x64 const *bytes)
{
  DN_SHA3Str8x128 result;
  DN_SHA3_HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

bool DN_SHA3_U8x32Eq(DN_SHA3U8x28 const *a, DN_SHA3U8x28 const *b)
{
  int result = DN_SHA3_Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3_U8x32Eq(DN_SHA3U8x32 const *a, DN_SHA3U8x32 const *b)
{
  int result = DN_SHA3_Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3_U8x48Eq(DN_SHA3U8x48 const *a, DN_SHA3U8x48 const *b)
{
  int result = DN_SHA3_Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3_U8x64Eq(DN_SHA3U8x64 const *a, DN_SHA3U8x64 const *b)
{
  int result = DN_SHA3_Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

#if defined(DN_SHA3_WITH_TESTS)
#if !defined(DN_H)
  #error dn.h must be included to enable tests for dn_sha3.h
#endif
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused-parameter)
DN_GCC_WARNING_DISABLE(-Wsign-compare)

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4244)
DN_MSVC_WARNING_DISABLE(4100)
DN_MSVC_WARNING_DISABLE(6385)
// NOTE: Keccak Reference Implementation
// A very compact Keccak implementation taken from the reference implementation
// repository
// https://github.com/XKCP/XKCP/blob/master/Standalone/CompactFIPS202/C/Keccak-more-compact.c
#define FOR(i, n) for (i = 0; i < n; ++i)
void DN_SHA3RefImplKeccak_(int r, int c, const uint8_t *in, uint64_t inLen, uint8_t sfx, uint8_t *out, uint64_t outLen);

void DN_SHA3RefImplFIPS202_SHAKE128_(const uint8_t *in, uint64_t inLen, uint8_t *out, uint64_t outLen)
{
  DN_SHA3RefImplKeccak_(1344, 256, in, inLen, 0x1F, out, outLen);
}

void DN_SHA3RefImplFIPS202_SHAKE256_(const uint8_t *in, uint64_t inLen, uint8_t *out, uint64_t outLen)
{
  DN_SHA3RefImplKeccak_(1088, 512, in, inLen, 0x1F, out, outLen);
}

void DN_SHA3RefImplFIPS202_SHA3_224_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_SHA3RefImplKeccak_(1152, 448, in, inLen, 0x06, out, 28);
}

void DN_SHA3RefImplFIPS202_SHA3_256_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_SHA3RefImplKeccak_(1088, 512, in, inLen, 0x06, out, 32);
}

void DN_SHA3RefImplFIPS202_SHA3_384_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_SHA3RefImplKeccak_(832, 768, in, inLen, 0x06, out, 48);
}

void DN_SHA3RefImplFIPS202_SHA3_512_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_SHA3RefImplKeccak_(576, 1024, in, inLen, 0x06, out, 64);
}

int DN_SHA3RefImplLFSR86540_(uint8_t *R)
{
  (*R) = ((*R) << 1) ^ (((*R) & 0x80) ? 0x71 : 0);
  return ((*R) & 2) >> 1;
}

  #define ROL(a, o) ((((uint64_t)a) << o) ^ (((uint64_t)a) >> (64 - o)))

static uint64_t DN_SHA3RefImplload64_(const uint8_t *x)
{
  int      i;
  uint64_t u = 0;
  FOR(i, 8)
  {
    u <<= 8;
    u |= x[7 - i];
  }
  return u;
}

static void DN_SHA3RefImplstore64_(uint8_t *x, uint64_t u)
{
  int i;
  FOR(i, 8)
  {
    x[i] = u;
    u >>= 8;
  }
}

static void DN_SHA3RefImplxor64_(uint8_t *x, uint64_t u)
{
  int i;
  FOR(i, 8)
  {
    x[i] ^= u;
    u >>= 8;
  }
}

#define rL(x, y)    DN_SHA3RefImplload64_((uint8_t *)s + 8 * (x + 5 * y))
#define wL(x, y, l) DN_SHA3RefImplstore64_((uint8_t *)s + 8 * (x + 5 * y), l)
#define XL(x, y, l) DN_SHA3RefImplxor64_((uint8_t *)s + 8 * (x + 5 * y), l)

void DN_SHA3RefImplKeccak_F1600(void *s)
{
  int      r, x, y, i, j, Y;
  uint8_t  R = 0x01;
  uint64_t C[5], D;
  for (i = 0; i < 24; i++) {
    /*??*/ FOR(x, 5) C[x] = rL(x, 0) ^ rL(x, 1) ^ rL(x, 2) ^ rL(x, 3) ^ rL(x, 4);
    FOR(x, 5)
    {
      D = C[(x + 4) % 5] ^ ROL(C[(x + 1) % 5], 1);
      FOR(y, 5)
      XL(x, y, D);
    }
    /*????*/ x = 1;
    y = r = 0;
    D     = rL(x, y);
    FOR(j, 24)
    {
      r += j + 1;
      Y    = (2 * x + 3 * y) % 5;
      x    = y;
      y    = Y;
      C[0] = rL(x, y);
      wL(x, y, ROL(D, r % 64));
      D = C[0];
    }
    /*??*/ FOR(y, 5)
    {
      FOR(x, 5)
      C[x] = rL(x, y);
      FOR(x, 5)
      wL(x, y, C[x] ^ ((~C[(x + 1) % 5]) & C[(x + 2) % 5]));
    }
    /*??*/ FOR(j, 7) if (DN_SHA3RefImplLFSR86540_(&R)) XL(0, 0, (uint64_t)1 << ((1 << j) - 1));
  }
}

void DN_SHA3RefImplKeccak_(int r, int c, const uint8_t *in, uint64_t inLen, uint8_t sfx, uint8_t *out, uint64_t outLen)
{
  /*initialize*/ uint8_t s[200];
  int                    R = r / 8;
  int                    i, b = 0;
  FOR(i, 200)
  s[i] = 0;
  /*absorb*/ while (inLen > 0) {
    b = (inLen < R) ? inLen : R;
    FOR(i, b)
    s[i] ^= in[i];
    in += b;
    inLen -= b;
    if (b == R) {
      DN_SHA3RefImplKeccak_F1600(s);
      b = 0;
    }
  }
  /*pad*/ s[b] ^= sfx;
  if ((sfx & 0x80) && (b == (R - 1)))
    DN_SHA3RefImplKeccak_F1600(s);
  s[R - 1] ^= 0x80;
  DN_SHA3RefImplKeccak_F1600(s);
  /*squeeze*/ while (outLen > 0) {
    b = (outLen < R) ? outLen : R;
    FOR(i, b)
    out[i] = s[i];
    out += b;
    outLen -= b;
    if (outLen > 0)
      DN_SHA3RefImplKeccak_F1600(s);
  }
}

  #undef XL
  #undef wL
  #undef rL
  #undef ROL
  #undef FOR
DN_MSVC_WARNING_POP
DN_GCC_WARNING_POP

enum DN_SHA3TestHash
{
  DN_SHA3TestHash_224,
  DN_SHA3TestHash_256,
  DN_SHA3TestHash_384,
  DN_SHA3TestHash_512,
  DN_SHA3TestHash_Keccak224,
  DN_SHA3TestHash_Keccak256,
  DN_SHA3TestHash_Keccak384,
  DN_SHA3TestHash_Keccak512,
  DN_SHA3TestHash_Count,
};

static void DN_SHA3_TestHashDispatch_(DN_TestCore *test, DN_SHA3TestHash hash_type, DN_Str8 input)
{
  DN_TCScratch scratch   = DN_TCScratchBeginArena(&test->arena, 1);
  DN_Str8      input_hex = DN_Str8HexFromPtrBytesArena(input.data, input.count, &scratch.arena, DN_TrimLeadingZero_No);
  switch (hash_type) {
    case DN_SHA3TestHash_Count: DN_AssertInvalidCodePath; break;
    case DN_SHA3TestHash_224: {
      DN_SHA3U8x28 hash = DN_SHA3_Hash224b(input.data, input.count);
      DN_SHA3U8x28 expect;
      DN_SHA3RefImplFIPS202_SHA3_224_(DN_Cast(uint8_t *) input.data, input.count, (uint8_t *)expect.data);
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_256: {
      DN_SHA3U8x32 hash = DN_SHA3_Hash256b(input.data, input.count);
      DN_SHA3U8x32 expect;
      DN_SHA3RefImplFIPS202_SHA3_256_(DN_Cast(uint8_t *) input.data, input.count, (uint8_t *)expect.data);
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_384: {
      DN_SHA3U8x48 hash = DN_SHA3_Hash384b(input.data, input.count);
      DN_SHA3U8x48 expect;
      DN_SHA3RefImplFIPS202_SHA3_384_(DN_Cast(uint8_t *) input.data, input.count, (uint8_t *)expect.data);
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_512: {
      DN_SHA3U8x64 hash = DN_SHA3_Hash512b(input.data, input.count);
      DN_SHA3U8x64 expect;
      DN_SHA3RefImplFIPS202_SHA3_512_(DN_Cast(uint8_t *) input.data, input.count, (uint8_t *)expect.data);
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_Keccak224: {
      DN_SHA3U8x28 hash = DN_SHA3_HashKeccak224b(input.data, input.count);
      DN_SHA3U8x28 expect;
      DN_SHA3RefImplKeccak_(1152, 448, DN_Cast(uint8_t *) input.data, input.count, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_Keccak256: {
      DN_SHA3U8x32 hash = DN_SHA3_HashKeccak256b(input.data, input.count);
      DN_SHA3U8x32 expect;
      DN_SHA3RefImplKeccak_(1088, 512, DN_Cast(uint8_t *) input.data, input.count, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_Keccak384: {
      DN_SHA3U8x48 hash = DN_SHA3_HashKeccak384b(input.data, input.count);
      DN_SHA3U8x48 expect;
      DN_SHA3RefImplKeccak_(832, 768, DN_Cast(uint8_t *) input.data, input.count, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;

    case DN_SHA3TestHash_Keccak512: {
      DN_SHA3U8x64 hash = DN_SHA3_HashKeccak512b(input.data, input.count);
      DN_SHA3U8x64 expect;
      DN_SHA3RefImplKeccak_(576, 1024, DN_Cast(uint8_t *) input.data, input.count, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_TestVerifyBytesEqF(test, DN_Str8FromLitArray(hash.data), DN_Str8FromLitArray(expect.data), "Input: %.*s", DN_Str8PrintFmt(input_hex));
    } break;
  }
  DN_TCScratchEnd(&scratch);
}

DN_TestCore DN_SHA3_TestSuite(DN_Arena *arena)
{
  DN_TestCore result = DN_TestInit(arena);
  for (DN_TestGroupScopeF(&result, "SHA3")) {
    for (DN_ForIndexU(hash_type, DN_SHA3TestHash_Count)) {
      // NOTE: Get a name for the hash type
      DN_Str8 hash_name = {};
      switch (hash_type) {
        case DN_SHA3TestHash_224:       hash_name = DN_Str8Lit("SHA3-224");   break;
        case DN_SHA3TestHash_256:       hash_name = DN_Str8Lit("SHA3-256");   break;
        case DN_SHA3TestHash_384:       hash_name = DN_Str8Lit("SHA3-384");   break;
        case DN_SHA3TestHash_512:       hash_name = DN_Str8Lit("SHA3-512");   break;
        case DN_SHA3TestHash_Keccak224: hash_name = DN_Str8Lit("Keccak-224"); break;
        case DN_SHA3TestHash_Keccak256: hash_name = DN_Str8Lit("Keccak-256"); break;
        case DN_SHA3TestHash_Keccak384: hash_name = DN_Str8Lit("Keccak-384"); break;
        case DN_SHA3TestHash_Keccak512: hash_name = DN_Str8Lit("Keccak-512"); break;
        case DN_SHA3TestHash_Count:     DN_AssertInvalidCodePath;          break;
      }

      // NOTE: Test SHA3 against some fixed inputs
      {
        DN_Str8 const INPUTS[] = {
            DN_Str8Lit("abc"),
            DN_Str8Lit(""),
            DN_Str8Lit("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            DN_Str8Lit("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
        };

        for (DN_ForItCArray(it, DN_Str8 const, INPUTS)) {
          for (DN_TestScopeF(&result, "[%.*s] %.*s", DN_Str8PrintFmt(hash_name), DN_Str8PrintFmt(*it.data)))
            DN_SHA3_TestHashDispatch_(&result, DN_Cast(DN_SHA3TestHash)hash_type, *it.data);
        }
      }

      // NOTE: Test SHA3 against some deterministic inputs generated from a PRNG
      for (DN_TestScopeF(&result, "[%.*s] Deterministic random inputs", DN_Str8PrintFmt(hash_name))) {
        DN_PCG32 rng = DN_PCG32Init(0xd48e'be21'2af8'733d);
        for (DN_USize index = 0; index < 128; index++) {
          // NOTE: Create a 4kb buffer with the deterministic contents
          char   src[4096] = {};
          DN_U32 src_size  = DN_PCG32Range(&rng, 0, sizeof(src));
          for (DN_USize src_index = 0; src_index < src_size; src_index++)
            src[src_index] = DN_Cast(char) DN_PCG32Range(&rng, 0, 255);

          // NOTE: Do the hashing
          DN_Str8 input = DN_Str8FromPtr(src, src_size);
          DN_SHA3_TestHashDispatch_(&result, DN_Cast(DN_SHA3TestHash)hash_type, input);
        }
      }
    }
  }
  return result;
}

void DN_SHA3_TestSuiteThenOutput(DN_Str8FromTestCoreFlags flags)
{
  DN_Arena    arena  = DN_ArenaFromHeap(DN_Megabytes(1), DN_Kilobytes(64), DN_MemFlags_Nil, DN_OS_HeapInitDefault());
  DN_TestCore test   = DN_SHA3_TestSuite(&arena);
  DN_Str8     output = DN_Str8FromTestCore(&test, &arena, flags);
  printf("%.*s", DN_Str8PrintFmt(output));
  DN_ArenaDeinit(&arena);
}
#endif // #if defined(DN_SHA3_WITH_TESTS)
#endif // #if defined(DN_SHA3_IMPLEMENTATION)
