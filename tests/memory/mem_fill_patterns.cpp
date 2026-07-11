//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// typeset / wordset pattern-fill validation. Pins the pattern-phase
// semantics of __memset_words / __typeset_dispatch: the byte at offset j
// (relative to the destination start) equals pattern_bytes[j % sizeof(T)],
// for every count crossing the ladder rungs and for misaligned starts.
// A full-storage sentinel scan catches stray wide stores.

#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

namespace
{

constexpr byte SENT = 0xEE;
constexpr u64 GUARD = 64;
constexpr u64 MAXB = 4099 * 8;      // largest fill in bytes (4099 u64 elements)

alignas(64) byte store[GUARD + 16 + MAXB + 64 + GUARD];

template<typename T>
bool
check_typeset(T pattern, u64 cnt, u64 d_off) noexcept
{
  byte *d = store + GUARD + d_off;
  const u64 bytes = cnt * sizeof(T);
  byte pat_bytes[sizeof(T)];
  __builtin_memcpy(pat_bytes, &pattern, sizeof(T));
  for ( u64 i = 0; i < sizeof(store); ++i ) store[i] = SENT;
  micron::typeset<T>(d, pattern, cnt);
  for ( u64 i = 0; i < sizeof(store); ++i ) {
    const byte *p = store + i;
    if ( p >= d && p < d + bytes ) {
      if ( *p != pat_bytes[static_cast<u64>(p - d) % sizeof(T)] ) return false;
    } else if ( *p != SENT )
      return false;
  }
  return true;
}

template<typename T>
bool
sweep_type(const char *name) noexcept
{
  const T patterns[] = { static_cast<T>(~static_cast<T>(0)), static_cast<T>(0x0102030405060708ULL), static_cast<T>(0x00000000000000FFULL) };
  for ( T pat : patterns ) {
    for ( u64 cnt = 1; cnt <= 129; ++cnt )
      for ( u64 d_off = 0; d_off < 16; ++d_off )
        if ( !check_typeset<T>(pat, cnt, d_off) ) {
          sb::print("typeset failed: type=", name, " cnt=", cnt, " d_off=", d_off);
          return false;
        }
    for ( u64 cnt : { (u64)256, (u64)1000, (u64)4099 } )
      for ( u64 d_off : { (u64)0, (u64)1, (u64)7, (u64)15 } )
        if ( !check_typeset<T>(pat, cnt, d_off) ) {
          sb::print("typeset failed: type=", name, " cnt=", cnt, " d_off=", d_off);
          return false;
        }
  }
  return true;
}

}      // namespace

int
main()
{
  sb::test_case("typeset u16 patterns");
  sb::require_true(sweep_type<u16>("u16"));
  sb::end_test_case();

  sb::test_case("typeset u32 patterns");
  sb::require_true(sweep_type<u32>("u32"));
  sb::end_test_case();

  sb::test_case("typeset u64 patterns");
  sb::require_true(sweep_type<u64>("u64"));
  sb::end_test_case();

  // all-bytes-equal pattern must behave exactly like memset (broadcast_byte
  // redirect in __memset_words)
  sb::test_case("typeset splat == memset");
  for ( u64 cnt : { (u64)1, (u64)3, (u64)16, (u64)33, (u64)129, (u64)1000 } )
    for ( u64 d_off : { (u64)0, (u64)5, (u64)13 } ) sb::require_true(check_typeset<u64>(0x4242424242424242ULL, cnt, d_off));
  sb::end_test_case();

  sb::print("mem_fill_patterns: all cases passed");
  return 1;
}
