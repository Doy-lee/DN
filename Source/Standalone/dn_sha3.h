#if !defined(DN_SHA3_H)
#define DN_SHA3_H

// NOTE: DN_Sha3 -- FIPS202 SHA3 + non-finalized SHA3 (aka. Keccak) hashing algorithms
//
// Overview
//   Single header file implementation of the Keccak hashing algorithms from the Keccak and SHA3
//   families (including the FIPS202 published algorithms and the non-finalized ones, i.e. the ones
//   used in Ethereum and Monero which adopted SHA3 before it was finalized. The only difference
//   between the 2 is a different delimited suffix).
//
// Configuration
//   Define this in one and only one C++ file to enable the implementation code of the header file.
//
//     #define DN_SHA3_IMPLEMENTATION
//
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
#if !defined(DN_SHA3Memcpy)
  #include <string.h>
  #define DN_SHA3Memcpy(dest, src, count) memcpy(dest, src, count)
#endif

#if !defined(DN_SHA3Memcmp)
  #include <string.h>
  #define DN_SHA3Memcmp(dest, src, count) memcmp(dest, src, count)
#endif

#if !defined(DN_SHA3Memset)
  #include <string.h>
  #define DN_SHA3Memset(dest, byte, count) memset(dest, byte, count)
#endif

#if !defined(DN_SHA3Assert)
  #if defined(NDEBUG)
    #define DN_SHA3Assert(expr)
  #else
    #define DN_SHA3Assert(expr)     \
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

enum DN_SHA3Family
{
  DN_SHA3Family_SHA3,   // FIPS 202 SHA3 (delimited suffix is 0x6)
  DN_SHA3Family_Keccak, // Non-finalized SHA3 (only difference is delimited suffix of 0x1 instead of 0x6)
};

// hash_size_bits: Number of bits to hash to. Available sizes are 224, 256, 384 and 512.
DN_SHA3State    DN_SHA3FamilyInit      (DN_SHA3Family type, size_t hash_size_bits);
DN_SHA3State    DN_SHA3FamilyInitSHA3  (size_t hash_size_bits);
DN_SHA3State    DN_SHA3FamilyInitKeccak(size_t hash_size_bits);
void            DN_SHA3FamilyUpdate    (DN_SHA3State *sha3, void const *data, size_t data_size);
void            DN_SHA3FamilyFinish    (DN_SHA3State *sha3, void *dest, size_t dest_size);
void            DN_SHA3FamilyHash      (DN_SHA3Family type, size_t hash_size_bits, void const *src, size_t src_size, void *dest, int dest_size);

void            DN_SHA3Hash224bPtr     (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x28    DN_SHA3Hash224b        (void const *src, size_t src_size);
void            DN_SHA3Hash256bPtr     (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x32    DN_SHA3Hash256b        (void const *src, size_t src_size);
void            DN_SHA3Hash384bPtr     (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x48    DN_SHA3Hash384b        (void const *src, size_t src_size);
void            DN_SHA3Hash512bPtr     (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x64    DN_SHA3Hash512b        (void const *src, size_t src_size);

void            DN_KeccakHash224bPtr   (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x28    DN_KeccakHash224b      (void const *src, size_t src_size);
void            DN_KeccakHash256bPtr   (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x32    DN_KeccakHash256b      (void const *src, size_t src_size);
void            DN_KeccakHash384bPtr   (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x48    DN_KeccakHash384b      (void const *src, size_t src_size);
void            DN_KeccakHash512bPtr   (void const *src, size_t src_size, void *dest, size_t dest_size);
DN_SHA3U8x64    DN_KeccakHash512b      (void const *src, size_t src_size);

void            DN_SHA3HexFromBytes    (void const *src, uint64_t src_size, char *dest, uint64_t dest_size);
DN_SHA3Str8x56  DN_SHA3HexFromU8x28    (DN_SHA3U8x28 const *bytes);
DN_SHA3Str8x64  DN_SHA3HexFromU8x32    (DN_SHA3U8x32 const *bytes);
DN_SHA3Str8x96  DN_SHA3HexFromU8x48    (DN_SHA3U8x48 const *bytes);
DN_SHA3Str8x128 DN_SHA3HexFromU8x64    (DN_SHA3U8x64 const *bytes);
bool            DN_SHA3U8x28Eq         (DN_SHA3U8x28 const *a, DN_SHA3U8x28 const *b);
bool            DN_SHA3U8x32Eq         (DN_SHA3U8x32 const *a, DN_SHA3U8x32 const *b);
bool            DN_SHA3U8x48Eq         (DN_SHA3U8x48 const *a, DN_SHA3U8x48 const *b);
bool            DN_SHA3U8x64Eq         (DN_SHA3U8x64 const *a, DN_SHA3U8x64 const *b);
#endif // DN_SHA3_H

#if defined(DN_SHA3_IMPLEMENTATION) || 1
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
static void DN_SHA3FamilyPermute_(void *state)
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

DN_SHA3State DN_SHA3FamilyInit(DN_SHA3Family type, size_t hash_size_bits)
{
  DN_SHA3Assert(hash_size_bits == 224 ||
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
  result.delimited_suffix = type == DN_SHA3Family_SHA3 ? SHA3_DELIMITED_SUFFIX : KECCAK_DELIMITED_SUFFIX;
  DN_SHA3Assert(bitrate + (hash_size_bits * 2) /*capacity*/ == 1600);
  return result;
}

DN_SHA3State DN_SHA3FamilyInitSHA3(size_t hash_size_bits)
{
  DN_SHA3State result = DN_SHA3FamilyInit(DN_SHA3Family_SHA3, hash_size_bits);
  return result;
}

DN_SHA3State DN_SHA3FamilyInitKeccak(size_t hash_size_bits)
{
  DN_SHA3State result = DN_SHA3FamilyInit(DN_SHA3Family_Keccak, hash_size_bits);
  return result;
}

void DN_SHA3FamilyUpdate(DN_SHA3State *sha3, void const *data, size_t data_size)
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
      DN_SHA3Assert(sha3->state_size == sha3->absorb_size);
      DN_SHA3FamilyPermute_(state);
      sha3->state_size = 0;
    }
  }
}

void DN_SHA3FamilyFinish(DN_SHA3State *sha3, void *dest, size_t dest_size)
{
  DN_SHA3Assert(dest_size >= (size_t)(sha3->hash_size_bits / 8));

  // Sponge Finalization Step: Final padding bit
  size_t const INDEX_OF_0X80_BYTE     = sha3->absorb_size - 1;
  size_t const delimited_suffix_index = sha3->state_size;
  DN_SHA3Assert(delimited_suffix_index < sha3->absorb_size);

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
  DN_SHA3FamilyPermute_(state);

  // Squeeze Step: Squeeze bytes from the state into our hash
  uint8_t     *dest_u8       = (uint8_t *)dest;
  size_t const squeeze_count = dest_size / sha3->absorb_size;
  size_t       squeeze_index = 0;
  for (; squeeze_index < squeeze_count; squeeze_index++) {
    if (squeeze_index)
      DN_SHA3FamilyPermute_(state);
    DN_SHA3Memcpy(dest_u8, state, sha3->absorb_size);
    dest_u8 += sha3->absorb_size;
  }

  // Squeeze Finalisation Step: Remainder bytes in hash
  size_t const remainder = dest_size % sha3->absorb_size;
  if (remainder) {
    if (squeeze_index)
      DN_SHA3FamilyPermute_(state);
    DN_SHA3Memcpy(dest_u8, state, remainder);
  }
}

void DN_SHA3FamilyHash(DN_SHA3Family type, size_t hash_size_bits, void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3State state = DN_SHA3FamilyInit(type, hash_size_bits);
  DN_SHA3FamilyUpdate(&state, src, src_size);
  DN_SHA3FamilyFinish(&state, dest, dest_size);
}

void DN_SHA3Hash224bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 224, src, src_size, dest, dest_size);
}

DN_SHA3U8x28 DN_SHA3Hash224b(void const *src, size_t src_size)
{
  DN_SHA3U8x28 result = {};
  DN_SHA3Hash224bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3Hash256bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 256, src, src_size, dest, dest_size);
}

DN_SHA3U8x32 DN_SHA3Hash256b(void const *src, size_t src_size)
{
  DN_SHA3U8x32 result = {};
  DN_SHA3Hash256bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3Hash384bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 384, src, src_size, dest, dest_size);
}

DN_SHA3U8x48 DN_SHA3Hash384b(void const *src, size_t src_size)
{
  DN_SHA3U8x48 result = {};
  DN_SHA3Hash384bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3Hash512bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 512, src, src_size, dest, dest_size);
}

DN_SHA3U8x64 DN_SHA3Hash512b(void const *src, size_t src_size)
{
  DN_SHA3U8x64 result = {};
  DN_SHA3Hash512bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_KeccakHash224bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 224, src, src_size, dest, dest_size);
}

DN_SHA3U8x28 DN_KeccakHash224b(void const *src, size_t src_size)
{
  DN_SHA3U8x28 result = {};
  DN_KeccakHash224bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_KeccakHash256bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 256, src, src_size, dest, dest_size);
}

DN_SHA3U8x32 DN_KeccakHash256b(void const *src, size_t src_size)
{
  DN_SHA3U8x32 result = {};
  DN_KeccakHash256bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_KeccakHash384bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 384, src, src_size, dest, dest_size);
}

DN_SHA3U8x48 DN_KeccakHash384b(void const *src, size_t src_size)
{
  DN_SHA3U8x48 result = {};
  DN_KeccakHash384bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_KeccakHash512bPtr(void const *src, size_t src_size, void *dest, size_t dest_size)
{
  DN_SHA3FamilyHash(DN_SHA3Family_SHA3, /*hash_size_bits=*/ 512, src, src_size, dest, dest_size);
}

DN_SHA3U8x64 DN_KeccakHash512b(void const *src, size_t src_size)
{
  DN_SHA3U8x64 result = {};
  DN_KeccakHash512bPtr(src, src_size, result.data, sizeof(result.data));
  return result;
}

void DN_SHA3HexFromBytes(void const *src, size_t src_size, char *dest, size_t dest_size)
{
  (void)src_size;
  (void)dest_size;
  DN_SHA3Assert(dest_size >= src_size * 2);

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

DN_SHA3Str8x56 DN_SHA3HexFromU8x28(DN_SHA3U8x28 const *bytes)
{
  DN_SHA3Str8x56 result;
  DN_SHA3HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x64 DN_SHA3HexFromU8x32(DN_SHA3U8x32 const *bytes)
{
  DN_SHA3Str8x64 result;
  DN_SHA3HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x96 DN_SHA3HexFromU8x48(DN_SHA3U8x48 const *bytes)
{
  DN_SHA3Str8x96 result;
  DN_SHA3HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

DN_SHA3Str8x128 DN_SHA3HexFromU8x64(DN_SHA3U8x64 const *bytes)
{
  DN_SHA3Str8x128 result;
  DN_SHA3HexFromBytes(bytes->data, sizeof(bytes->data), result.data, sizeof(result.data));
  result.data[sizeof(result.data) - 1] = 0;
  return result;
}

bool DN_SHA3U8x32Eq(DN_SHA3U8x28 const *a, DN_SHA3U8x28 const *b)
{
  int result = DN_SHA3Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3U8x32Eq(DN_SHA3U8x32 const *a, DN_SHA3U8x32 const *b)
{
  int result = DN_SHA3Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3U8x48Eq(DN_SHA3U8x48 const *a, DN_SHA3U8x48 const *b)
{
  int result = DN_SHA3Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}

bool DN_SHA3U8x64Eq(DN_SHA3U8x64 const *a, DN_SHA3U8x64 const *b)
{
  int result = DN_SHA3Memcmp(a->data, b->data, sizeof(*a)) == 0;
  return result;
}
#endif // DN_SHA3_IMPLEMENTATION
