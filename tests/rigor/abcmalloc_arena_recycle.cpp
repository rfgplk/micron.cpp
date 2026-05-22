//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/cmalloc.hpp"
#include "../../src/math/rng.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/std.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr usize WAVES = 12;
constexpr usize WORKERS = 8;
constexpr usize WSET = 4096;
constexpr u64 OPS = 200000;

alignas(64) static byte *g_slots[WORKERS * WSET];
alignas(64) static u32 g_szs[WORKERS * WSET];

struct wctx {
  u64 seed;
  u32 tag;
  u32 wid;
  micron::atomic_token<u64> *errors;
};

[[gnu::always_inline]] inline byte
fingerprint(u32 tag, u32 slot, u32 sz) noexcept
{
  return static_cast<byte>((tag * 2654435761u) ^ (slot * 40503u) ^ (sz * 97u));
}

void
worker(wctx *c)
{
  byte **slots = g_slots + static_cast<usize>(c->wid) * WSET;
  u32 *szs = g_szs + static_cast<usize>(c->wid) * WSET;
  for ( usize i = 0; i < WSET; ++i ) {
    slots[i] = nullptr;
    szs[i] = 0;
  }

  auto rng = micron::math::rng::xoshiro256ss::from_seed(c->seed);
  u64 errs = 0;

  for ( u64 i = 0; i < OPS; ++i ) {
    const u32 s = micron::math::rng::dist::uniform_int<u32>(rng, 0u, WSET - 1);
    if ( slots[s] ) {
      const byte want = fingerprint(c->tag, s, szs[s]);
      for ( u32 k = 0; k < szs[s]; ++k )
        if ( slots[s][k] != want ) {
          ++errs;
          break;
        }
      abc::dealloc(slots[s]);
      slots[s] = nullptr;
    } else {
      const u32 sz = micron::math::rng::dist::uniform_int<u32>(rng, 1u, 128u);
      byte *p = abc::alloc(sz);
      if ( !p ) {
        ++errs;
        continue;
      }
      const byte v = fingerprint(c->tag, s, sz);
      for ( u32 k = 0; k < sz; ++k ) p[k] = v;
      slots[s] = p;
      szs[s] = sz;
    }
  }

  for ( usize i = 0; i < WSET; ++i ) {
    if ( !slots[i] ) continue;
    const byte want = fingerprint(c->tag, static_cast<u32>(i), szs[i]);
    for ( u32 k = 0; k < szs[i]; ++k )
      if ( slots[i][k] != want ) {
        ++errs;
        break;
      }
    abc::dealloc(slots[i]);
    slots[i] = nullptr;
  }

  if ( errs ) c->errors->fetch_add(errs, micron::memory_order_relaxed);
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC ARENA-RECYCLE (96 lifetime threads, 8 concurrent) ===");

  micron::atomic_token<u64> errors{ 0 };

  test_case("12 waves x 8 concurrent churning arenas survive with intact fingerprints");
  {
    static wctx ctx[WORKERS];
    for ( usize w = 0; w < WAVES; ++w ) {
      for ( usize i = 0; i < WORKERS; ++i ) {
        ctx[i].seed = 0xC0FFEEull ^ (w * 131u) ^ (i * 0x9E3779B97F4A7C15ull);
        ctx[i].tag = static_cast<u32>(w * WORKERS + i + 1);
        ctx[i].wid = static_cast<u32>(i);
        ctx[i].errors = &errors;
      }
      abctest::run_workers(worker, ctx, WORKERS);
    }
    require(errors.get(micron::memory_order_acquire), static_cast<u64>(0));
  }
  end_test_case();

  test_case("allocator usable after 96 thread-arenas reclaimed");
  {
    byte *p = abc::alloc(64);
    require_true(p != nullptr);
    abc::dealloc(p);
  }
  end_test_case();

  sb::print("=== ABCMALLOC ARENA-RECYCLE PASSED ===");
  return 0;
}
