//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"

#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"

#include "../snowball/snowball.hpp"

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require;
using ::sb::test_case;

namespace
{

constexpr static const usize __cap = (abc::__alloc_limit != 0) ? abc::__alloc_limit : (usize{ 1 } << 28);

constexpr static const usize __walk_lo_shift = 20;

inline bool
in_range(usize n) noexcept
{
  return n != 0 and n <= __cap;
}

inline bool
touch_verify(byte *p, usize n, byte seed) noexcept
{
  if ( p == nullptr ) return false;
  const usize mid = n >> 1;
  p[0] = seed;
  p[mid] = static_cast<byte>(seed ^ 0xFF);
  p[n - 1] = static_cast<byte>(seed + 1u);
  return p[0] == seed and p[mid] == static_cast<byte>(seed ^ 0xFF) and p[n - 1] == static_cast<byte>(seed + 1u);
}

bool
probe_size(usize n, byte seed)
{
  byte *p = abc::alloc(n);
  if ( p == nullptr ) {
    print("  FAILED: abc::alloc returned null at size ", n);
    return false;
  }
  if ( !touch_verify(p, n, seed) ) {
    print("  FAILED: touch/verify mismatch at size ", n);
    abc::dealloc(p);
    return false;
  }
  abc::dealloc(p);
  return true;
}

};      // namespace

int
main(void)
{
  print("=== ABCMALLOC HUGE-TIER BUDDY-ORDER BOUNDARY ===");
  print("  __alloc_limit = ", abc::__alloc_limit, "  walk cap = ", __cap);

  test_case("32 MiB order-8 cliff: 2^25-32, 2^25-31 and 2^25 must all allocate");
  {
    const usize last_order7 = (usize{ 1 } << 25) - 32;
    const usize first_order8 = (usize{ 1 } << 25) - 31;
    const usize exact = (usize{ 1 } << 25);
    require(in_range(last_order7) and in_range(first_order8) and in_range(exact));
    require(probe_size(last_order7, 0x5A));
    require(probe_size(first_order8, 0xA5));
    require(probe_size(exact, 0x69));
  }
  end_test_case();

  test_case("power-of-two bracket walk S-1 / S / S+1 across the huge tier");
  {
    usize walked = 0;
    for ( usize shift = __walk_lo_shift; shift < (sizeof(usize) * 8); ++shift ) {
      const usize S = usize{ 1 } << shift;
      if ( !in_range(S) ) break;

      print("  shift=", shift, " size=", S);
      const usize bracket[3] = { S - 1, S, S + 1 };
      for ( usize k = 0; k < 3; ++k ) {

        if ( !in_range(bracket[k]) ) continue;
        require(probe_size(bracket[k], static_cast<byte>(0x11u * (k + 1u))));
      }
      ++walked;
    }
    print("  bracket groups walked: ", walked);
    require(walked >= 6);
  }
  end_test_case();

  test_case("top-of-range block is reusable after free");
  {
    require(probe_size(__cap, 0x3C));
    require(probe_size(__cap, 0xC3));
    require(probe_size(__cap, 0x77));
  }
  end_test_case();

  test_case("simultaneous live bracket, freed in reverse");
  {
    const usize S = usize{ 1 } << 22;
    require(in_range(S + 1));
    byte *live[3] = { nullptr, nullptr, nullptr };
    const usize sizes[3] = { S - 1, S, S + 1 };

    for ( usize k = 0; k < 3; ++k ) {
      live[k] = abc::alloc(sizes[k]);
      require(live[k] != nullptr);
      require(touch_verify(live[k], sizes[k], static_cast<byte>(0x20u + k)));
    }

    require(live[0] != live[1] and live[1] != live[2] and live[0] != live[2]);
    for ( usize k = 0; k < 3; ++k ) {
      require(live[k][0] == static_cast<byte>(0x20u + k));
      require(live[k][sizes[k] - 1] == static_cast<byte>(0x20u + k + 1u));
    }
    for ( usize k = 3; k-- > 0; ) abc::dealloc(live[k]);
  }
  end_test_case();

  test_case("small tier grows past its initial sheet (513..4095 band)");
  {

    constexpr usize N = 16384;
    constexpr usize lo = 513;
    constexpr usize hi = 4095;
    static byte *live[N];

    for ( usize i = 0; i < N; ++i ) {
      const usize sz = lo + (i % (hi - lo + 1));
      live[i] = abc::alloc(sz);
      if ( live[i] == nullptr ) {
        print("  FAILED: small-tier alloc returned null at index ", i, " size ", sz);
        print("  (the tier could not expand past its initial sheet)");
      }
      require(live[i] != nullptr);
      live[i][0] = static_cast<byte>(i & 0xFFu);
      live[i][sz - 1] = static_cast<byte>((i >> 8) & 0xFFu);
    }

    for ( usize i = 0; i < N; ++i ) {
      const usize sz = lo + (i % (hi - lo + 1));
      require(live[i][0] == static_cast<byte>(i & 0xFFu));
      require(live[i][sz - 1] == static_cast<byte>((i >> 8) & 0xFFu));
    }
    for ( usize i = N; i-- > 0; ) abc::dealloc(live[i]);
    print("  small-tier blocks held live: ", N);
  }
  end_test_case();

  test_case("balloc chunk length covers the request at the tier boundaries");
  {
    const usize probes[3] = { (usize{ 1 } << 25) - 31, (usize{ 1 } << 25), __cap };
    for ( usize k = 0; k < 3; ++k ) {
      if ( !in_range(probes[k]) ) continue;
      micron::__chunk<byte> chunk = abc::balloc(probes[k]);
      require(chunk.ptr != nullptr);
      require(chunk.len >= probes[k]);
      print("  requested=", probes[k], " observed_len=", chunk.len);
      abc::dealloc(chunk.ptr);
    }
  }
  end_test_case();

  print("[ABCMALLOC HUGE-TIER BOUNDARY OK]");
  return 1;
}
