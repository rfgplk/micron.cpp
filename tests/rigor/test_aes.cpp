// test_aes.cpp
// Behavioral coverage for `micron::simd::aes::*`. AES-NI ops are
// deterministic round transforms; we verify that:
//   - aes::dec_round(aes::enc_round(state, k), k) is NOT identity (one round)
//   - aes::dec_round(aes::dec_round_last(...), ...) shape is sane
//   - keygen_assist with rcon=0x01 produces vendor's documented test vector

#include "../../src/simd/aliases/aes.hpp"
#include "../snowball/snowball.hpp"

namespace ma = ::micron::simd::aes;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

template<typename T>
[[gnu::always_inline]] inline bool
v_eq(T a, T b) noexcept
{
  alignas(16) unsigned char ba[sizeof(T)];
  alignas(16) unsigned char bb[sizeof(T)];
  __builtin_memcpy(ba, &a, sizeof(T));
  __builtin_memcpy(bb, &b, sizeof(T));
  for ( unsigned i = 0; i < sizeof(T); ++i )
    if ( ba[i] != bb[i] ) return false;
  return true;
}

int
main()
{
  print("=== TEST AES ===");

  test_case("aes: enc -> dec one round != identity (state changes)");
  __m128i state = _mm_setr_epi32(0x11223344, 0x55667788, 0x99aabbcc, (int)0xddeeff00u);
  __m128i key = _mm_set1_epi32(int(0xa5a5a5a5u));
  __m128i e1 = ma::enc_round(state, key);
  require_true(v_eq(e1, state) == false);
  __m128i e2 = ma::dec_round(e1, key);
  require_true(v_eq(e2, state) == false);      // one enc + one dec is NOT identity
  end_test_case();

  test_case("aes: zero state + zero key fixed point");
  // AESENC(0,0) = MixColumns(ShiftRows(SubBytes(0))) ^ 0 = MixColumns(ShiftRows(c)),
  // where c is the 16-byte vector with each lane = SubBytes(0) = 0x63. since
  // ShiftRows preserves an all-equal state and MixColumns of 0x63x16 is
  // 0x63x16 (linear, columns equal), AESENC(0, 0) = splat(0x63).
  __m128i z = _mm_setzero_si128();
  __m128i ez = ma::enc_round(z, z);
  alignas(16) unsigned char ezb[16];
  _mm_storeu_si128((__m128i *)ezb, ez);
  for ( int i = 0; i < 16; ++i ) require_true(ezb[i] == 0x63);

  // dec_round_last is the exact inverse of enc_round_last when state is XOR-ed
  // with k beforehand: AESDECLAST(AESENCLAST(s ^ k, 0), 0) ^ k = s. easier to
  // just verify aesdeclast undoes aesenclast for s=0, k=0.
  __m128i el = ma::enc_round_last(z, z);
  __m128i dl = ma::dec_round_last(el, z);
  require_true(v_eq(dl, z) == true);

  // inv_mix_columns is its own inverse on a state where MixColumns leaves it
  // unchanged: e.g. zero. (the proper inverse-key relation needs the schedule.)
  __m128i im = ma::inv_mix_columns(z);
  require_true(v_eq(im, z) == true);
  end_test_case();

  test_case("aes: keygen_assist<RCON=1>(zero) is constant-shape");
  __m128i kg = ma::keygen_assist<0x01>(_mm_setzero_si128());
  // for input zero, keygen_assist returns (RotWord(SubWord(0)) ^ Rcon, ...) ~
  // intel manual: RotWord(SubWord(0_u32)) = 0x63636363 rotr by 8 = 0x63636363,
  // XOR with rcon byte 0x01 in the low byte: 0x63636362
  alignas(16) unsigned int lanes[4];
  _mm_storeu_si128((__m128i *)lanes, kg);
  require_true(lanes[1] == 0x63636362u);
  end_test_case();

  print("[TEST AES OK]");
  return 1;
}
