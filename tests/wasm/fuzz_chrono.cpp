//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0xa54ff53au
#endif
#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 65536
#endif

#include "../../src/sync/pause.hpp"
#include "fuzz_support.hpp"

namespace
{
using wide = unsigned __int128;
constexpr u64 maximum = 0x7fffffffffffffffull;
constexpr i64 untouched = 0x123456789abcdefll;

[[gnu::noinline]] wide
shift_down(wide value, unsigned distance)
{
  // LLVM 22 can speculate a selected-away wide shift and lower its out-of-range
  // count to __lshrti3. Keep the oracle's checked operation opaque to that pass.
  if ( distance >= 128 ) __builtin_trap();
  return value >> distance;
}

bool
floating_oracle(u64 bits, i64 &out)
{
  const unsigned exponent = unsigned(bits >> 52) & 2047u;
  if ( exponent == 2047 ) return false;
  if ( bits >> 63 ) {
    out = 0;
    return true;
  }
  wide product = ((bits & 0xfffffffffffffull) | (exponent ? 1ull << 52 : 0)) * wide(1000000);
  int power = exponent ? int(exponent) - 1075 : -1074;
  unsigned leading = 0;
  for ( wide t = product; t > 1; t >>= 1 ) ++leading;
  if ( leading > 52 ) {
    const unsigned shift = leading - 52;
    const wide low = product & ((wide(1) << shift) - 1);
    const wide halfway = wide(1) << (shift - 1);
    product >>= shift;
    if ( low > halfway || (low == halfway && (product & 1)) ) ++product;
    power += int(shift);
  }
  if ( power >= 0 ) {
    if ( power >= 64 || product > (wide(maximum) >> power) ) return false;
    product <<= power;
  } else {
    product = power <= -128 ? 0 : shift_down(product, unsigned(-power));
    if ( product > maximum ) return false;
  }
  out = static_cast<i64>(product);
  return true;
}

bool
check_floating(u64 bits)
{
  i64 expected = untouched, actual = untouched;
  const bool wanted = floating_oracle(bits, expected);
  const bool result = micron::__wasm_sleep_milliseconds(__builtin_bit_cast(double, bits), actual);
  return WASM_FUZZ_CHECK(result == wanted) && WASM_FUZZ_CHECK(actual == expected);
}

template<u64 Scale>
bool
check_integer(u64 value)
{
  const wide product = wide(value) * Scale;
  const bool wanted = product <= maximum;
  i64 actual = untouched;
  const bool result = micron::__wasm_sleep_unsigned<Scale>(value, actual);
  return WASM_FUZZ_CHECK(result == wanted) && WASM_FUZZ_CHECK(actual == (wanted ? i64(product) : untouched));
}

template<u64 Scale>
bool
integer_boundaries()
{
  const u64 limit = maximum / Scale;
  return check_integer<Scale>(0) && check_integer<Scale>(1) && check_integer<Scale>(limit - 1) && check_integer<Scale>(limit)
         && check_integer<Scale>(limit + 1) && check_integer<Scale>(~u64(0));
}

#if defined(__micron_wasm_eh)
template<typename F>
bool
rejects(F fn)
{
  try {
    fn();
  } catch ( const micron::except::thread_error & ) {
    return true;
  } catch ( ... ) {
  }
  return false;
}
#endif

bool
public_boundaries()
{
  micron::cpu_pause<0>();
  micron::sleep_nano(0);
  micron::sleep_for(0);
  micron::sleep(0);
  micron::ssleep(0);
  micron::sleep_duration(0.0);
  micron::sleep_duration(-0.0);
  micron::sleep_duration(-1.0);
  micron::wait_for(-1.0);
  micron::spin_for(-1.0);
  micron::sleep_duration(__builtin_bit_cast(double, 0xffefffffffffffffull));
  micron::sleep_duration(__builtin_bit_cast(double, 1ull));
#if defined(__micron_wasm_threads)
  micron::sleep_nano(1);
  micron::cpu_pause<1>();
#endif
#if defined(__micron_wasm_eh)
  if ( !WASM_FUZZ_CHECK(rejects([] { micron::sleep_for(9223372036855ull); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::ssleep(9223372037ull); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::sleep_nano(0x8000000000000000ull); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::cpu_pause<0x8000000000000000ull>(); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::sleep_duration(1e13); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::wait_for(__builtin_bit_cast(double, 0x7ff0000000000000ull)); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::spin_for(__builtin_bit_cast(double, 0xfff0000000000000ull)); }))
       || !WASM_FUZZ_CHECK(rejects([] { micron::sleep_duration(__builtin_bit_cast(double, 0x7ff8000000000001ull)); })) )
    return false;
#endif
  return WASM_FUZZ_CHECK(true);
}
}      // namespace

int
main()
{
  if ( !integer_boundaries<1>() || !integer_boundaries<1000000>() || !integer_boundaries<1000000000>() || !public_boundaries() ) return 6;

  const u64 boundary = __builtin_bit_cast(u64, 0x1p63 / 1000000.0);
  for ( u64 bits = boundary - 256; bits <= boundary + 256; ++bits )
    if ( !check_floating(bits) ) return 6;
  for ( u64 exponent = 0; exponent < 2048; ++exponent ) {
    if ( !check_floating(exponent << 52) || !check_floating((exponent << 52) | 0xfffffffffffffull)
         || !check_floating(0x8000000000000000ull | (exponent << 52)) )
      return 6;
  }
  u32 state = wasm_fuzz::seed;
  for ( u32 i = 0; i < wasm_fuzz::steps; ++i ) {
    wasm_fuzz::step = i;
    const u64 value = wasm_fuzz::next64(state);
    if ( !check_integer<1>(value) || !check_integer<1000000>(value) || !check_integer<1000000000>(value) || !check_floating(value) )
      return 6;
  }
  return wasm_fuzz::finish("chrono");
}
