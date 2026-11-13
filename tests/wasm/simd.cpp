// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/simd/simd.hpp"
#include "../../src/simd/bitwise.hpp"
#include "../../src/simd/memory.hpp"
#include "../../src/simd/shift.hpp"

namespace s = micron::simd;

static u32
next(u32 &state)
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

template<typename B, typename F>
bool
integer_vectors()
{
  using V = s::__wasm_vector<B, F>;
  using U = micron::make_unsigned_t<F>;
  constexpr unsigned width = sizeof(F) * 8;
  u32 seed = 0x243f6a88u;
  F a[V::size()], b[V::size()];
  for ( unsigned iteration = 0; iteration < 128; ++iteration ) {
    for ( usize i = 0; i < V::size(); ++i ) {
      a[i] = F(u64(next(seed)) << 32 | next(seed));
      b[i] = F(next(seed));
    }
    V av, bv;
    av.load(a);
    bv.load(b);
    V add = av + bv, sub = av - bv, mul = av * bv;
    for ( usize i = 0; i < V::size(); ++i ) {
      if ( U(add[i]) != U(U(a[i]) + U(b[i])) || U(sub[i]) != U(U(a[i]) - U(b[i])) ) return false;
      if ( U(mul[i]) != U(u64(U(a[i])) * u64(U(b[i]))) ) return false;
    }
    for ( unsigned count = 0; count <= width + 2; ++count ) {
      const V left = av << int(count), right = av >> int(count);
      for ( usize i = 0; i < V::size(); ++i ) {
        U l = count >= width ? 0 : U(u64(U(a[i])) << count);
        U r = U(a[i]);
        for ( unsigned j = 0; j < count; ++j ) r = U((r >> 1) | (micron::is_signed_v<F> && a[i] < 0 ? U(1) << (width - 1) : 0));
        if ( U(left[i]) != l || U(right[i]) != r ) return false;
      }
    }
  }
  return true;
}

template<typename B>
bool
fp_vectors()
{
  using F = typename s::__wasm_packet_traits<B>::element;
  using V = s::__wasm_vector<B, F>;
  F a[V::size()], b[V::size()];
  for ( usize i = 0; i < V::size(); ++i ) {
    a[i] = F(i + 1);
    b[i] = F(i + 3);
  }
  V av, bv;
  av.load(a);
  bv.load(b);
  auto c = (av + bv) * bv - av;
  for ( usize i = 0; i < V::size(); ++i )
    if ( c[i] != (a[i] + b[i]) * b[i] - a[i] ) return false;
  return true;
}

int
main()
{
#define CHECK(B, F)                                                                                                                        \
  do {                                                                                                                                     \
    if ( !integer_vectors<s::B, F>() ) return 6;                                                                                           \
  } while ( false )
  CHECK(i128, i8);
  CHECK(i128, u8);
  CHECK(i128, i16);
  CHECK(i128, u16);
  CHECK(i128, i32);
  CHECK(i128, u32);
  CHECK(i128, i64);
  CHECK(i128, u64);
  CHECK(i256, i8);
  CHECK(i256, u8);
  CHECK(i256, i16);
  CHECK(i256, u16);
  CHECK(i256, i32);
  CHECK(i256, u32);
  CHECK(i256, i64);
  CHECK(i256, u64);
  CHECK(i512, i8);
  CHECK(i512, u8);
  CHECK(i512, i16);
  CHECK(i512, u16);
  CHECK(i512, i32);
  CHECK(i512, u32);
  CHECK(i512, i64);
  CHECK(i512, u64);
#undef CHECK
  if ( !fp_vectors<s::f128>() || !fp_vectors<s::d128>() || !fp_vectors<s::f256>() || !fp_vectors<s::d256>() || !fp_vectors<s::f512>()
       || !fp_vectors<s::d512>() )
    return 6;
  u8 input[1024], output[1024], expected[1024];
  for ( usize length = 0; length <= 300; ++length )
    for ( usize offset = 0; offset < 16; ++offset ) {
      for ( usize i = 0; i < 1024; ++i ) {
        input[i] = u8(i * 71 + length);
        output[i] = 0xcd;
        expected[i] = 0xcd;
      }
      for ( usize i = 0; i < length; ++i ) expected[offset + i] = input[offset + i];
      s::memcpy128(output + offset, input + offset, length);
      for ( usize i = 0; i < 1024; ++i )
        if ( output[i] != expected[i] ) return 6;
      usize matches = 0, first = length;
      for ( usize i = 0; i < length; ++i )
        if ( input[offset + i] == 0x37 ) {
          ++matches;
          if ( first == length ) first = i;
        }
      if ( s::count_set_128(input + offset, length, u8(0x37)) != matches ) return 6;
      if ( s::find_first_set_128(input + offset, length, u8(0x37)) != first ) return 6;
    }
  for ( int delta = -64; delta <= 64; ++delta )
    for ( usize length = 0; length <= 300; ++length ) {
      for ( usize i = 0; i < 1024; ++i ) input[i] = output[i] = expected[i] = u8(i * 37);
      for ( usize i = 0; i < length; ++i ) expected[256 + delta + i] = input[256 + i];
      s::memmove128(output + 256 + delta, output + 256, length);
      for ( usize i = 0; i < 1024; ++i )
        if ( output[i] != expected[i] ) return 6;
    }
  s::v32 signs(-1, -2147483647 - 1, 1, 0), counts(32, 33, 0, -1);
  const auto logical = s::srlv_32(signs, counts), arithmetic = s::srav_32(signs, counts);
  if ( logical[0] || logical[1] || logical[2] != 1 || logical[3] ) return 6;
  if ( arithmetic[0] != -1 || arithmetic[1] != -1 || arithmetic[2] != 1 || arithmetic[3] ) return 6;
  s::v32 allones(-1), hugecounts(0, 1, 32, 33);
  const auto shifts = s::sllv_32(allones, hugecounts);
  if ( shifts[0] != -1 || shifts[1] != -2 || shifts[2] || shifts[3] ) return 6;
  s::v32 inactive;
  auto masked = s::maskload_32(reinterpret_cast<const int *>(~usize(0)), s::i128(inactive));
  if ( !s::v32(masked).all_zeroes() ) return 6;
  s::v32 badindices(-1, -1, -1, -1);
  auto gathered
      = s::maskgather_32_i32<s::i128, 4>(s::i128(signs), reinterpret_cast<const int *>(~usize(0)), s::i128(badindices), s::i128(inactive));
  if ( (s::v32(gathered) == signs) != 0xffff ) return 6;
  return 1;
}
