//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// extensive abcmalloc test: hot/cold tier ceiling, multi-word __space_mask
// invariants, allocator_small routing, tombstone reclaim, provenance & freeze,
// alignment & redzone-friendly patterns.

#include "../../src/io/console.hpp"

#include "../../src/cmalloc.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"

#include "../../src/array.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

inline bool
ptr_aligned(const void *p, usize n) noexcept
{
  return (reinterpret_cast<uintptr_t>(p) % n) == 0;
}

inline bool
region_is_byte(const void *p, usize n, byte v) noexcept
{
  const byte *q = static_cast<const byte *>(p);
  for ( usize i = 0; i < n; ++i ) {
    if ( q[i] != v ) return false;
  }
  return true;
}

inline void
fill_pattern(byte *p, usize n, byte seed) noexcept
{
  for ( usize i = 0; i < n; ++i ) p[i] = static_cast<byte>(seed + (i & 0x7Fu));
}

inline bool
verify_pattern(const byte *p, usize n, byte seed) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    if ( p[i] != static_cast<byte>(seed + (i & 0x7Fu)) ) return false;
  }
  return true;
}

}      // anonymous namespace

int
main(int, char **)
{
  sb::print("=== ABCMALLOC TESTS ===");

  // ─────────────────────────────────────────────────────────────────────────
  // tier routing — request size determines which tier serves the allocation
  // ─────────────────────────────────────────────────────────────────────────

  test_case("precise tier: 1B request returns valid non-null pointer");
  {
    byte *p = abc::alloc(1);
    require(p != nullptr, true);
    require(abc::within(p), true);
    p[0] = 0xAB;
    require(static_cast<unsigned>(p[0]), 0xABu);
    abc::dealloc(p);
  }
  end_test_case();

  test_case("precise tier: 256B (class_precise) — pointer naturally aligned");
  {
    byte *p = abc::alloc(abc::__class_precise);
    require(p != nullptr, true);
    require(ptr_aligned(p, 16), true);
    abc::dealloc(p);
  }
  end_test_case();

  test_case("small tier: 513B — routed to TLSF small tier");
  {
    byte *p = abc::alloc(513);
    require(p != nullptr, true);
    fill_pattern(p, 513, 0x10);
    require(verify_pattern(p, 513, 0x10), true);
    abc::dealloc(p);
  }
  end_test_case();

  test_case("medium tier: 4096B — buddy block for page-sized request");
  {
    byte *p = abc::alloc(abc::__class_medium);
    require(p != nullptr, true);
    fill_pattern(p, abc::__class_medium, 0x55);
    require(verify_pattern(p, abc::__class_medium, 0x55), true);
    abc::dealloc(p);
  }
  end_test_case();

  test_case("large tier: 32 KiB (class_large)");
  {
    byte *p = abc::alloc(abc::__class_large);
    require(p != nullptr, true);
    p[0] = 0x12;
    p[abc::__class_large - 1] = 0x34;
    require(static_cast<unsigned>(p[0]), 0x12u);
    require(static_cast<unsigned>(p[abc::__class_large - 1]), 0x34u);
    abc::dealloc(p);
  }
  end_test_case();

  test_case("huge tier: 256 KiB (class_huge)");
  {
    byte *p = abc::alloc(abc::__class_huge);
    require(p != nullptr, true);
    fill_pattern(p, 4096, 0xC1);      // touch first page
    require(verify_pattern(p, 4096, 0xC1), true);
    abc::dealloc(p);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // provenance — `within()` must report true for live allocations
  // ─────────────────────────────────────────────────────────────────────────

  test_case("provenance: within() positive on live allocation across all tiers");
  {
    const usize sizes[] = { 128u, 384u, 1024u, 8192u, 65536u };
    for ( usize sz : sizes ) {
      byte *p = abc::alloc(sz);
      require(p != nullptr, true);
      require(abc::within(p), true);
      abc::dealloc(p);
    }
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // boundary writes — usable region matches requested size, no slop
  // ─────────────────────────────────────────────────────────────────────────

  test_case("boundary: first & last byte writable in each tier");
  {
    const usize sizes[] = { 2u, 64u, 255u, 256u, 257u, 511u, 512u, 513u, 4095u, 4096u, 4097u, 32768u, 65536u };
    for ( usize sz : sizes ) {
      byte *p = abc::alloc(sz);
      require(p != nullptr, true);
      p[0] = 0x77;
      p[sz - 1] = 0x88;
      require(static_cast<unsigned>(p[0]), 0x77u);
      require(static_cast<unsigned>(p[sz - 1]), 0x88u);
      abc::dealloc(p);
    }
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // multi-word bitmap stress — drive each hot tier past 64 simultaneous
  // allocations so the expanded __space_mask is actually exercised
  // ─────────────────────────────────────────────────────────────────────────

  test_case("multi-word bitmap: 200 simultaneous 4 KiB allocations stay live");
  {
    constexpr usize N = 200;
    byte *ptrs[N];
    for ( usize i = 0; i < N; ++i ) {
      ptrs[i] = abc::alloc(4096);
      require(ptrs[i] != nullptr, true);
      ptrs[i][0] = static_cast<byte>(i & 0xFFu);
      ptrs[i][4095] = static_cast<byte>((i ^ 0xAA) & 0xFFu);
    }
    for ( usize i = 0; i < N; ++i ) {
      require(static_cast<unsigned>(ptrs[i][0]), static_cast<unsigned>(i & 0xFFu));
      require(static_cast<unsigned>(ptrs[i][4095]), static_cast<unsigned>((i ^ 0xAA) & 0xFFu));
    }
    for ( usize i = 0; i < N; ++i ) abc::dealloc(ptrs[i]);
  }
  end_test_case();

  test_case("multi-word bitmap: 500 alternating-size allocations (256B/4KiB)");
  {
    constexpr usize N = 500;
    byte *ptrs[N];
    for ( usize i = 0; i < N; ++i ) {
      usize sz = (i & 1u) ? 256u : 4096u;
      ptrs[i] = abc::alloc(sz);
      require(ptrs[i] != nullptr, true);
    }
    for ( usize i = 0; i < N; ++i ) abc::dealloc(ptrs[i]);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // churn — many alloc/free cycles must not leak sheets nor corrupt the index
  // ─────────────────────────────────────────────────────────────────────────

  test_case("churn: 4096 cycles of alloc/free at varied sizes");
  {
    constexpr usize N = 4096;
    for ( usize i = 0; i < N; ++i ) {
      usize sz = 32u + ((i * 113u) & 0x1FFFu);      // 32..8223
      byte *p = abc::alloc(sz);
      require(p != nullptr, true);
      p[0] = 0xEE;
      p[sz - 1] = 0xCC;
      require(static_cast<unsigned>(p[0]), 0xEEu);
      require(static_cast<unsigned>(p[sz - 1]), 0xCCu);
      abc::dealloc(p);
    }
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // calloc — zero-initialised memory
  // ─────────────────────────────────────────────────────────────────────────

  /*
   * calloc means chunk alloc not calloc (ie posix), the testing harness hallucinated
   * or technically i poorly named the fn, i don't want to rename it because i'd need to sed too many places (and the form itself is
  ambiguous) test_case("calloc: returned region is zeroed");
  {
    const usize sizes[] = { 64u, 1024u, 8192u };
    for ( usize sz : sizes ) {
      auto chunk = abc::__abc_allocator<byte>::calloc(sz);
      require(chunk.ptr != nullptr, true);
      require(region_is_byte(chunk.ptr, sz, 0), true);
      abc::dealloc(chunk.ptr, chunk.len);
    }
  }
  end_test_case();
 */
  // ─────────────────────────────────────────────────────────────────────────
  // realloc — content preserved, may move
  // ─────────────────────────────────────────────────────────────────────────

  test_case("realloc: shrink preserves prefix");
  {
    constexpr usize OLD = 4096;
    constexpr usize NEW = 256;
    auto *p = static_cast<byte *>(abc::malloc(OLD));
    require(p != nullptr, true);
    fill_pattern(p, OLD, 0x33);
    auto *q = static_cast<byte *>(abc::realloc(p, NEW));
    require(q != nullptr, true);
    require(verify_pattern(q, NEW, 0x33), true);
    abc::dealloc(reinterpret_cast<byte *>(q));
  }
  end_test_case();

  test_case("realloc: grow preserves prefix");
  {
    constexpr usize OLD = 200;
    constexpr usize NEW = 4000;
    auto *p = static_cast<byte *>(abc::malloc(OLD));
    require(p != nullptr, true);
    fill_pattern(p, OLD, 0x44);
    auto *q = static_cast<byte *>(abc::realloc(p, NEW));
    require(q != nullptr, true);
    require(verify_pattern(q, OLD, 0x44), true);
    abc::dealloc(reinterpret_cast<byte *>(q));
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // allocator_small — sub-page granularity for strings & similar
  // ─────────────────────────────────────────────────────────────────────────

  test_case("allocator_small: granularity matches policy (512)");
  {
    require(micron::allocator_small<>::auto_size(), 512u);
  }
  end_test_case();

  test_case("allocator_small: create services sub-page requests (>= 512B usable)");
  {
    auto c = micron::allocator_small<>::create(100);
    require(c.ptr != nullptr, true);
    require(c.len >= 512u, true);
    micron::allocator_small<>::destroy(c);
  }
  end_test_case();

  test_case("allocator_small: create with 1024 returns >=1024B payload");
  {
    auto c = micron::allocator_small<>::create(1024);
    require(c.ptr != nullptr, true);
    require(c.len >= 1024u, true);
    micron::allocator_small<>::destroy(c);
  }
  end_test_case();

  test_case("hstring default uses allocator_small");
  {
    using string_alloc = micron::string;
    static_assert(__is_base_of(micron::allocator_small<>, string_alloc),
                  "micron::string default Alloc should be allocator_small after Mitigation A");
    require(true, true);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // hstring churn — many short keys must NOT saturate the medium tier
  // (was the original failure mode with allocator_serial)
  // ─────────────────────────────────────────────────────────────────────────

  test_case("hstring churn: 8192 short strings, fully alive at peak");
  {
    constexpr usize N = 8192;
    micron::vector<micron::string> bag;
    bag.reserve(N);
    for ( usize i = 0; i < N; ++i ) {
      micron::string s("key_");
      s += micron::int_to_string<usize>(i);
      bag.emplace_back(micron::move(s));
    }
    require(bag.size(), N);
    micron::string prefix("key_");
    require(bag[0].find(prefix), 0u);
    require(bag[N - 1].find(prefix), 0u);
    bag.clear();
  }
  end_test_case();

  test_case("hstring churn: 32768 short strings (multi-word bitmap saturation)");
  {
    constexpr usize N = 32768;
    micron::vector<micron::string> bag;
    bag.reserve(N);
    for ( usize i = 0; i < N; ++i ) {
      micron::string s("k");
      s += micron::int_to_string<usize>(i);
      bag.emplace_back(micron::move(s));
    }
    require(bag.size(), N);
    bag.clear();
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // alignment — every chunk respects 16-byte alignment (SIMD-safe)
  // ─────────────────────────────────────────────────────────────────────────

  test_case("alignment: every allocation is at least 16-byte aligned");
  {
    const usize sizes[] = { 1u, 7u, 33u, 65u, 129u, 257u, 513u, 1025u, 4097u };
    for ( usize sz : sizes ) {
      byte *p = abc::alloc(sz);
      require(p != nullptr, true);
      require(ptr_aligned(p, 16), true);
      abc::dealloc(p);
    }
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  // mixed pattern — interleaved allocations across all tiers, reverse free
  // ─────────────────────────────────────────────────────────────────────────

  test_case("mixed pattern: 1024 interleaved allocations across all 5 tiers");
  {
    constexpr usize N = 1024;
    const usize sizes[] = { 64u, 384u, 1500u, 8192u, 65536u };
    constexpr usize NSZ = sizeof(sizes) / sizeof(sizes[0]);

    byte *ptrs[N];
    for ( usize i = 0; i < N; ++i ) {
      ptrs[i] = abc::alloc(sizes[i % NSZ]);
      require(ptrs[i] != nullptr, true);
    }
    for ( usize i = N; i > 0; --i ) abc::dealloc(ptrs[i - 1]);
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC TESTS PASSED ===");
  return 1;
}
