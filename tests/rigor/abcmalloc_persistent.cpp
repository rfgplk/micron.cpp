//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/math/rng.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr usize SIZES[] = { 64, 384, 2048 };
constexpr usize NSIZES = sizeof(SIZES) / sizeof(SIZES[0]);
constexpr usize BIG_SIZES[] = { 16384, 262144 };
constexpr usize NBIG = sizeof(BIG_SIZES) / sizeof(BIG_SIZES[0]);
constexpr usize BIG_EACH = 4;

constexpr usize ROUNDS = 6;
constexpr usize PER_ROUND = 128;
constexpr usize MAXSEEN = ROUNDS * PER_ROUND + NBIG * BIG_EACH + 8;

alignas(64) static byte *g_seen[MAXSEEN];
static usize g_nseen = 0;

[[gnu::always_inline]] inline bool
seen_contains(byte *p) noexcept
{
  for ( usize i = 0; i < g_nseen; ++i )
    if ( g_seen[i] == p ) return true;
  return false;
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC PERSISTENT MODE ===");

  test_case("allocator basic usability (alloc / write / free across tiers)");
  {
    byte *a = abc::alloc(64);
    byte *b = abc::alloc(4096);
    byte *c = abc::alloc(200000);
    require_true(a != nullptr);
    require_true(b != nullptr);
    require_true(c != nullptr);
    a[0] = 0x11;
    a[63] = 0x22;
    b[0] = 0x33;
    b[4095] = 0x44;
    c[0] = 0x55;
    c[199999] = 0x66;
    require_true(a[0] == 0x11 and a[63] == 0x22 and b[4095] == 0x44 and c[199999] == 0x66);
    abc::dealloc(a);
    abc::dealloc(b);
    abc::dealloc(c);
  }
  end_test_case();

  if constexpr ( abc::__default_persistent_mode ) {

    test_case("persistent: freed addresses are never regranted (no address ever recurs)");
    {
      auto rng = micron::math::rng::xoshiro256ss::from_seed(0xABC9051Eull);
      bool reuse = false;
      bool null_alloc = false;

      for ( usize bi = 0; bi < NBIG; ++bi ) {
        for ( usize h = 0; h < BIG_EACH; ++h ) {
          byte *p = abc::alloc(BIG_SIZES[bi]);
          if ( !p ) {
            null_alloc = true;
            continue;
          }
          if ( seen_contains(p) ) reuse = true;
          if ( g_nseen < MAXSEEN ) g_seen[g_nseen++] = p;
          p[0] = static_cast<byte>(h);
          p[BIG_SIZES[bi] - 1] = static_cast<byte>(bi);
          abc::dealloc(p);
        }
      }

      for ( usize r = 0; r < ROUNDS; ++r ) {
        byte *live[PER_ROUND];
        for ( usize k = 0; k < PER_ROUND; ++k ) {
          const usize sz = SIZES[micron::math::rng::dist::uniform_int<u32>(rng, 0u, static_cast<u32>(NSIZES) - 1)];
          byte *p = abc::alloc(sz);
          if ( !p ) {
            null_alloc = true;
            live[k] = nullptr;
            continue;
          }
          if ( seen_contains(p) ) reuse = true;
          if ( g_nseen < MAXSEEN ) g_seen[g_nseen++] = p;
          p[0] = static_cast<byte>(r ^ k);
          p[sz - 1] = static_cast<byte>(k);
          live[k] = p;
        }

        for ( usize k = 0; k < PER_ROUND; ++k )
          if ( live[k] ) abc::dealloc(live[k]);
      }

      require_true(!reuse);
      require_true(!null_alloc);
    }
    end_test_case();

    if constexpr ( abc::__default_fail_result == 2 ) {
      test_case("persistent: explicit relinquish() does not unmap (sheet stays mapped + intact)");
      {
        byte *v = abc::alloc(4096);
        require_true(v != nullptr);
        for ( u32 k = 0; k < 4096; ++k ) v[k] = static_cast<byte>(k & 0xFF);
        abc::relinquish(v);
        bool intact = true;
        for ( u32 k = 0; k < 4096; ++k )
          if ( v[k] != static_cast<byte>(k & 0xFF) ) {
            intact = false;
            break;
          }
        require_true(intact);
      }
      end_test_case();
    }

    if constexpr ( abc::__default_double_free_action == 0 ) {
      test_case("persistent: re-free of a tombstoned block is terminal (no resurrection)");
      {
        byte *v = abc::alloc(128);
        require_true(v != nullptr);
        abc::dealloc(v);
        abc::dealloc(v);
        bool got_v = false;
        for ( usize k = 0; k < 512; ++k ) {
          byte *p = abc::alloc(128);
          if ( p == v ) got_v = true;
        }
        require_true(!got_v);
      }
      end_test_case();
    }
  } else {
    test_case("persistent assertions skipped (rebuild with abc::__default_persistent_mode = true)");
    {
      sb::print("note: __default_persistent_mode is false; strict guarantee assertions are compiled out.");
      require_true(true);
    }
    end_test_case();
  }

  sb::print("=== ABCMALLOC PERSISTENT MODE PASSED ===");
  return 1;
}
