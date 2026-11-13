//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0x3c6ef372u
#endif

#include "../../src/memory/cmemory.hpp"
#include "../../src/simd/bitwise.hpp"
#include "../../src/simd/memory.hpp"
#include "../../src/simd/shift.hpp"
#include "../../src/simd/simd.hpp"
#include "fuzz_support.hpp"

namespace
{
namespace s = micron::simd;

template<usize N> struct bytes {
  u8 data[N];
};

static_assert(!s::is_int_flag_type<bool> && !s::is_flag_type<bool>);
static_assert(!s::is_flag_type<const bool> && !s::is_flag_type<char16_t> && !s::is_flag_type<char32_t>);
static_assert(s::is_flag_type<i8> && s::is_flag_type<u8> && s::is_flag_type<i16> && s::is_flag_type<u16>);
static_assert(s::is_flag_type<i32> && s::is_flag_type<u32> && s::is_flag_type<i64> && s::is_flag_type<u64>);
static_assert(s::is_flag_type<float> && s::is_flag_type<double>);

u64
mask_for(unsigned width)
{
  return width == 64 ? ~u64(0) : (u64(1) << width) - 1;
}

u64
product_oracle(u64 a, u64 b, unsigned width)
{
  u64 result = 0;
  for ( unsigned offset = 0; offset < width; offset += 8 ) {
    result += ((a & 255u) * b) << offset;
    a >>= 8;
  }
  return result & mask_for(width);
}

u64
shift_oracle(u64 value, unsigned width, u64 count, bool right, bool arithmetic)
{
  const u64 high = u64(1) << (width - 1), mask = mask_for(width);
  const u64 fill = arithmetic && (value & high) ? high : 0;
  if ( count >= width ) return right && fill ? mask : 0;
  for ( u64 i = 0; i < count; ++i ) value = right ? (value >> 1) | fill : (value << 1) & mask;
  return value;
}

u64
funnel_oracle(u64 a, u64 b, unsigned width, u64 count, bool right)
{
  count %= width;
  const u64 high = u64(1) << (width - 1), mask = mask_for(width);
  for ( u64 i = 0; i < count; ++i ) {
    if ( right ) {
      a = (a >> 1) | ((b & 1) ? high : 0);
      b >>= 1;
    } else {
      a = ((a << 1) | ((b & high) ? 1 : 0)) & mask;
      b = (b << 1) & mask;
    }
  }
  return a;
}

template<typename B, typename F>
bool
vectors(u32 &state)
{
  using T = s::__wasm_vector<B, F>;
  using U = micron::make_unsigned_t<F>;
  constexpr usize count = sizeof(B) / sizeof(F);
  constexpr unsigned width = sizeof(F) * 8;
  F a[count], b[count], divisor[count], counts[count], stored[count];
  for ( usize i = 0; i < count; ++i ) {
    a[i] = F(wasm_fuzz::next64(state));
    b[i] = F(wasm_fuzz::next64(state));
    divisor[i] = F(1 + wasm_fuzz::next(state) % 7);
    counts[i] = F(wasm_fuzz::next(state) % (width + 3));
    if ( wasm_fuzz::step % 16 == 0 ) {
      const u64 high = u64(1) << (width - 1), mask = mask_for(width);
      const u64 edges[] = { 0, 1, mask, high, high - 1, high + 1, mask - 1, 0x5555555555555555ull & mask };
      a[i] = F(edges[(i + wasm_fuzz::step / 16) % 8]);
      b[i] = F(edges[(i + wasm_fuzz::step / 16 + 3) % 8]);
      counts[i] = F(mask);
    }
  }
  T av, bv, dv, cv;
  av.load(a);
  bv.load(b);
  dv.load(divisor);
  cv.load(counts);
  const T add = av + bv, sub = av - bv, mul = av * bv, div = av / dv;
  const T both = av & bv, either = av | bv, different = av ^ bv;
  av.get(stored);
  const int n = int(wasm_fuzz::step % (width + 3)) - 1;
  const T left = av << n, right = av >> n;
  for ( usize i = 0; i < count; ++i ) {
    const u64 x = U(a[i]), y = U(b[i]);
    if ( !WASM_FUZZ_CHECK(U(add[i]) == U(x + y) && U(sub[i]) == U(x - y)) || !WASM_FUZZ_CHECK(U(mul[i]) == U(product_oracle(x, y, width)))
         || !WASM_FUZZ_CHECK(div[i] == F(a[i] / divisor[i]))
         || !WASM_FUZZ_CHECK(U(both[i]) == U(x & y) && U(either[i]) == U(x | y) && U(different[i]) == U(x ^ y))
         || !WASM_FUZZ_CHECK(stored[i] == a[i]) || !WASM_FUZZ_CHECK(U(left[i]) == U(shift_oracle(x, width, u64(n), false, false)))
         || !WASM_FUZZ_CHECK(U(right[i]) == U(shift_oracle(x, width, u64(n), true, micron::is_signed_v<F>))) )
      return false;
  }
  u64 equal = 0, less = 0, signs = 0;
  for ( usize i = 0; i < count; ++i )
    for ( usize j = 0; j < sizeof(F); ++j ) {
      if ( a[i] == b[i] ) equal |= u64(1) << (i * sizeof(F) + j);
      if ( a[i] < b[i] ) less |= u64(1) << (i * sizeof(F) + j);
    }
  const auto raw = __builtin_bit_cast(bytes<sizeof(B)>, B(av));
  for ( usize i = 0; i < sizeof(B); ++i )
    if ( raw.data[i] & 128 ) signs |= u64(1) << i;
  if ( !WASM_FUZZ_CHECK((av == bv) == equal && (av < bv) == less && av.movemask() == signs) ) return false;
  T zero;
  if ( !WASM_FUZZ_CHECK(zero.all_zeroes() && !zero.all_ones()) ) return false;
  zero.to_ones();
  if ( !WASM_FUZZ_CHECK(zero.all_ones() && !zero.all_zeroes()) ) return false;

  if constexpr ( sizeof(F) >= 2 ) {
    const T vl = s::sllv(av, cv), vr = s::srlv(av, cv), va = s::srav(av, cv);
    const T fl = s::shldi(av, bv, n), fr = s::shrdi(av, bv, n);
    for ( usize i = 0; i < count; ++i ) {
      const u64 x = U(a[i]), y = U(b[i]), c = U(counts[i]);
      if ( !WASM_FUZZ_CHECK(U(vl[i]) == U(shift_oracle(x, width, c, false, false)))
           || !WASM_FUZZ_CHECK(U(vr[i]) == U(shift_oracle(x, width, c, true, false)))
           || !WASM_FUZZ_CHECK(U(va[i]) == U(shift_oracle(x, width, c, true, true)))
           || !WASM_FUZZ_CHECK(U(fl[i]) == U(funnel_oracle(x, y, width, u64(n), false)))
           || !WASM_FUZZ_CHECK(U(fr[i]) == U(funnel_oracle(x, y, width, u64(n), true))) )
        return false;
    }
  }
  if constexpr ( sizeof(F) >= 4 ) {
    const T rl = s::rol(av, n), rr = s::ror(av, n);
    for ( usize i = 0; i < count; ++i ) {
      const u64 x = U(a[i]);
      if ( !WASM_FUZZ_CHECK(U(rl[i]) == U(funnel_oracle(x, x, width, u64(n), false)))
           || !WASM_FUZZ_CHECK(U(rr[i]) == U(funnel_oracle(x, x, width, u64(n), true))) )
        return false;
    }
  }
  return true;
}

template<typename B>
bool
integer_width(u32 &state)
{
  return vectors<B, i8>(state) && vectors<B, u8>(state) && vectors<B, i16>(state) && vectors<B, u16>(state) && vectors<B, i32>(state)
         && vectors<B, u32>(state) && vectors<B, i64>(state) && vectors<B, u64>(state);
}

template<typename I, typename F, typename D>
bool
masked(u32 &state)
{
  int words[sizeof(I) / 4];
  long long longs[sizeof(I) / 8];
  float reals[sizeof(I) / 4];
  double doubles[sizeof(I) / 8];
  u32 mask32[sizeof(I) / 4];
  u64 mask64[sizeof(I) / 8];
  for ( usize i = 0; i < sizeof(I) / 4; ++i ) {
    const u32 value = wasm_fuzz::next(state);
    words[i] = int(value);
    reals[i] = __builtin_bit_cast(float, value);
    mask32[i] = wasm_fuzz::next(state);
  }
  for ( usize i = 0; i < sizeof(I) / 8; ++i ) {
    const u64 value = wasm_fuzz::next64(state);
    longs[i] = i64(value);
    doubles[i] = __builtin_bit_cast(double, value);
    mask64[i] = wasm_fuzz::next64(state);
  }
  const I mi = s::loadu<I>(mask32), ml = s::loadu<I>(mask64);
  const auto a = __builtin_bit_cast(bytes<sizeof(I)>, s::maskload_32(words, mi));
  const auto b = __builtin_bit_cast(bytes<sizeof(I)>, s::maskload_64(longs, ml));
  const auto c = __builtin_bit_cast(bytes<sizeof(F)>, s::maskload_ps<F>(reals, mi));
  const auto d = __builtin_bit_cast(bytes<sizeof(D)>, s::maskload_pd<D>(doubles, ml));
  const auto expected32 = __builtin_bit_cast(bytes<sizeof(I)>, words);
  const auto expected64 = __builtin_bit_cast(bytes<sizeof(I)>, longs);
  for ( usize i = 0; i < sizeof(I); ++i ) {
    const u8 x = mask32[i / 4] >> 31 ? expected32.data[i] : 0;
    const u8 y = mask64[i / 8] >> 63 ? expected64.data[i] : 0;
    if ( !WASM_FUZZ_CHECK(a.data[i] == x && c.data[i] == x && b.data[i] == y && d.data[i] == y) ) return false;
  }
  const auto inactive = s::maskload_32(reinterpret_cast<const int *>(~usize(0)), I{});
  const auto inactive64 = s::maskload_64(reinterpret_cast<const long long *>(~usize(0)), I{});
  const auto off = __builtin_bit_cast(bytes<sizeof(I)>, inactive);
  const auto off64 = __builtin_bit_cast(bytes<sizeof(I)>, inactive64);
  for ( usize i = 0; i < sizeof(I); ++i )
    if ( !WASM_FUZZ_CHECK(off.data[i] == 0 && off64.data[i] == 0) ) return false;
  return true;
}

bool
gather(u32 &state)
{
  int data[96], index[4], mask[4];
  for ( auto &x : data ) x = int(wasm_fuzz::next(state));
  for ( unsigned i = 0; i < 4; ++i ) {
    index[i] = int(wasm_fuzz::next(state) % 32) - 16;
    mask[i] = int(wasm_fuzz::next(state));
  }
  const auto indices = s::loadu<s::i128>(index);
  const s::v32 full(s::gather_32_i32<s::i128, 8>(data + 48, indices));
  const s::v32 source(0x12345678);
  const s::v32 selected(s::maskgather_32_i32<s::i128, 8>(s::i128(source), data + 48, indices, s::loadu<s::i128>(mask)));
  for ( unsigned i = 0; i < 4; ++i )
    if ( !WASM_FUZZ_CHECK(full[i] == data[48 + index[i] * 2])
         || !WASM_FUZZ_CHECK(selected[i] == (mask[i] < 0 ? data[48 + index[i] * 2] : 0x12345678)) )
      return false;
  const s::v64 invalid_indices(i64(0x8000000000000000ull));
  const s::v32 off(
      s::maskgather_32_i64<s::i128, 8>(s::i128(source), reinterpret_cast<const int *>(~usize(0)), s::i128(invalid_indices), s::i128{}));
  if ( !WASM_FUZZ_CHECK(off[0] == 0x12345678 && off[1] == 0x12345678 && off[2] == 0 && off[3] == 0) ) return false;
  return true;
}

bool
same(const u8 *a, const u8 *b, usize count)
{
  for ( usize i = 0; i < count; ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

bool
memory(u32 &state, usize length, usize offset)
{
  u8 input[1024], actual[1024], expected[1024], needle[32];
  for ( unsigned i = 0; i < 1024; ++i ) {
    input[i] = u8(wasm_fuzz::next(state));
    actual[i] = expected[i] = 0xcd;
  }
  for ( usize i = 0; i < length; ++i ) expected[128 + offset + i] = input[128 + offset + i];
  micron::memcpy(actual + 128 + offset, input + 128 + offset, length);
  if ( !WASM_FUZZ_CHECK(same(actual, expected, 1024)) ) return false;
  const int delta = int(wasm_fuzz::next(state) % 129) - 64;
  for ( unsigned i = 0; i < 1024; ++i ) actual[i] = expected[i] = input[i];
  for ( usize i = 0; i < length; ++i ) expected[256 + delta + i] = input[256 + i];
  micron::memmove(actual + 256 + delta, actual + 256, length);
  if ( !WASM_FUZZ_CHECK(same(actual, expected, 1024)) ) return false;
  const u8 value = u8(wasm_fuzz::next(state));
  for ( unsigned i = 0; i < 1024; ++i ) actual[i] = expected[i] = 0xcd;
  for ( usize i = 0; i < length; ++i ) expected[128 + offset + i] = value;
  micron::memset(actual + 128 + offset, value, length);
  if ( !WASM_FUZZ_CHECK(same(actual, expected, 1024)) ) return false;

  const u8 *first = nullptr, *last = nullptr;
  usize occurrences = 0;
  for ( usize i = 0; i < length; ++i )
    if ( input[128 + offset + i] == value ) {
      if ( !first ) first = input + 128 + offset + i;
      last = input + 128 + offset + i;
      ++occurrences;
    }
  if ( !WASM_FUZZ_CHECK(s::memchr128(input + 128 + offset, value, length) == first)
       || !WASM_FUZZ_CHECK(s::memrchr128(input + 128 + offset, value, length) == last)
       || !WASM_FUZZ_CHECK(s::count_set_128(input + 128 + offset, length, char(value)) == occurrences)
       || !WASM_FUZZ_CHECK(s::find_first_set_128(input + 128 + offset, length, char(value))
                           == (first ? usize(first - input - 128 - offset) : length)) )
    return false;
  const usize nlen = wasm_fuzz::next(state) % 32;
  for ( usize i = 0; i < nlen; ++i ) needle[i] = input[128 + offset + (length ? wasm_fuzz::next(state) % length : 0)];
  const u8 *found = nlen == 0 ? input + 128 + offset : nullptr;
  if ( nlen && nlen <= length )
    for ( usize i = 0; i <= length - nlen; ++i )
      if ( same(input + 128 + offset + i, needle, nlen) ) {
        found = input + 128 + offset + i;
        break;
      }
  if ( !WASM_FUZZ_CHECK(s::memmem128(input + 128 + offset, length, needle, nlen) == found) ) return false;

  for ( usize i = 0; i < length; ++i ) actual[i] = input[128 + offset + i];
  if ( length ) actual[length - 1] ^= 1;
  int comparison = 0;
  for ( usize i = 0; i < length; ++i )
    if ( actual[i] != input[128 + offset + i] ) {
      comparison = int(actual[i]) - int(input[128 + offset + i]);
      break;
    }
  if ( !WASM_FUZZ_CHECK(s::memcmp128(actual, input + 128 + offset, length) == comparison) ) return false;
  const u64 word = wasm_fuzz::next64(state);
  for ( unsigned i = 0; i < 1024; ++i ) actual[i] = expected[i] = 0xcd;
  for ( usize i = 0; i < length; ++i ) expected[128 + offset + i] = u8(word >> ((i % 8) * 8));
  s::wordset128(actual + 128 + offset, word, length);
  return WASM_FUZZ_CHECK(same(actual, expected, 1024));
}
}      // namespace

int
main()
{
  u32 state = wasm_fuzz::seed;
  for ( usize length = 0; length <= 300; ++length )
    for ( usize offset = 0; offset < 16; ++offset ) {
      wasm_fuzz::step = u32(length * 16 + offset);
      if ( !memory(state, length, offset) ) return 6;
    }
  for ( u32 i = 0; i < wasm_fuzz::steps; ++i ) {
    wasm_fuzz::step = i;
    if ( !integer_width<s::i128>(state) || !integer_width<s::i256>(state) || !integer_width<s::i512>(state)
         || !masked<s::i128, s::f128, s::d128>(state) || !masked<s::i256, s::f256, s::d256>(state)
         || !masked<s::i512, s::f512, s::d512>(state) || !gather(state)
         || !memory(state, wasm_fuzz::next(state) % 513, wasm_fuzz::next(state) % 16) )
      return 6;
  }
  return wasm_fuzz::finish("simd");
}
