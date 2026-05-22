//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC REALLOC / QUERY_SIZE RIGOR.
//
// Targeted guard for the 2026-05-22 buddy realloc bug: __size_of_alloc /
// abc::query_size decoded a buddy block's order from the HEAD (addr - hdr) — the
// preceding block's tail header — instead of the authoritative block_tags. That
// made resize()'s in-place branch trust a neighbour's size and return a
// too-small block in place, so the caller's write trampled the adjacent live
// allocation. See abcmalloc_realloc_overlap_bug.md.
//
// This suite pins the two invariants that bug violated:
//   1. query_size(p) is the TRUE usable extent — every one of those bytes is
//      writable and isolated (writing them never disturbs a neighbour).
//   2. realloc preserves the prefix, reports a usable size >= request, and never
//      hands back a block that overlaps / corrupts another live allocation,
//      across in-place, move, grow, shrink, and tier-crossing cases.
//
// Build: duck build tests/rigor/abcmalloc_realloc.cpp ; run bin/abcmalloc_realloc

#include "../../src/io/console.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;
using abctest::rng_t;

namespace
{

// FULL (every-byte) fingerprint — not the size-adaptive canary. query_size
// correctness demands that *all* claimed bytes are validated, since an
// over-report only shows up as a trampled neighbour past the true end.
inline void
full_fill(byte *p, usize n, usize key, u32 gen) noexcept
{
  for ( usize i = 0; i < n; ++i ) p[i] = abctest::fp_byte(key, gen, i);
}

inline bool
full_check(const byte *p, usize n, usize key, u32 gen) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] != abctest::fp_byte(key, gen, i) ) return false;
  return true;
}

// representative sizes that straddle every tier boundary (±a few bytes), where
// order rounding and the head/tail header math are most error-prone.
constexpr usize kBoundaries[] = {
  1,     8,      32,     255,    256,   257,      // precise edge
  511,   512,    513,                             // small edge
  1024,  4095,   4096,   4097,                    // medium edge
  8192,  16384,  32767,  32768,  32769,           // large edge
  65536, 131072, 262143, 262144,                  // huge edge
};
constexpr usize kNB = sizeof(kBoundaries) / sizeof(kBoundaries[0]);

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC REALLOC / QUERY_SIZE RIGOR ===");

  // ── 1. query_size is the true, isolated usable extent (all tiers) ──────────
  // Densely allocate many blocks, fill each across its FULL reported query_size,
  // then verify every one. If query_size over-reports for any block, its fill
  // runs past the real end into a neighbour and that neighbour's check fails —
  // exactly the corruption the head/tail bug produced.
  test_case("query_size: full usable region is writable and isolated");
  {
    rng_t r = rng_t::from_seed(0x5EA11ull);
    constexpr usize N = 4096;
    static byte *p[N];
    static usize qs[N];
    usize live = 0;

    for ( usize i = 0; i < N; ++i ) {
      const usize n = abctest::sample_size_longtail(r);
      byte *b = abc::alloc(n);
      if ( !b ) {
        p[i] = nullptr;
        continue;
      }
      const usize q = abc::query_size(b);
      require_true(q >= n);      // never under-reports the request
      require_true(abctest::ptr_aligned(b, 16));
      p[i] = b;
      qs[i] = q;
      full_fill(b, q, i, 1u);      // write EVERY claimed byte, keyed by slot
      ++live;
    }
    sb::print("   allocated live blocks: ", live);

    for ( usize i = 0; i < N; ++i )
      if ( p[i] ) require_true(full_check(p[i], qs[i], i, 1u));

    for ( usize i = 0; i < N; ++i )
      if ( p[i] ) abc::dealloc(p[i]);
  }
  end_test_case();

  // ── 2. realloc preserves prefix + reports honest size (grow/shrink) ────────
  test_case("realloc: prefix preserved, query_size honest, full region usable");
  {
    rng_t r = rng_t::from_seed(0x12340002ull);
    for ( usize it = 0; it < 20000; ++it ) {
      const usize n0 = abctest::sample_size_longtail(r);
      byte *b = abc::alloc(n0);
      if ( !b ) continue;
      usize q = abc::query_size(b);
      require_true(q >= n0);
      // fill the requested prefix with a known pattern
      full_fill(b, n0, 0xAA, static_cast<u32>(it));

      const usize n1 = abctest::sample_size_longtail(r);
      byte *c = static_cast<byte *>(abc::realloc(b, n1));
      require_true(c != nullptr);
      const usize q1 = abc::query_size(c);
      require_true(q1 >= n1);

      // the min(n0,n1) prefix must survive the realloc (now that query_size,
      // which bounds resize's copy, is authoritative)
      const usize keep = abctest::mn(n0, n1);
      require_true(full_check(c, keep, 0xAA, static_cast<u32>(it)));

      // the whole new usable region must be writable
      full_fill(c, q1, 0xBB, static_cast<u32>(it));
      require_true(full_check(c, q1, 0xBB, static_cast<u32>(it)));

      abc::dealloc(c);
    }
  }
  end_test_case();

  // ── 3. realloc neighbour-safety: growing one block never tramples another ──
  // This is the bug's exact shape. Lay down a dense field of fingerprinted
  // blocks, then repeatedly realloc-GROW random members; after each grow,
  // re-verify the grown block AND sweep every other live block for damage.
  test_case("realloc: in-place/move grow never corrupts an adjacent live block");
  {
    rng_t r = rng_t::from_seed(0x33330003ull);
    constexpr usize N = 2048;
    static byte *p[N];
    static usize qs[N];
    static u32 gen[N];

    for ( usize i = 0; i < N; ++i ) {
      const usize n = abctest::sample_size_longtail(r);
      byte *b = abc::alloc(n);
      p[i] = b;
      if ( !b ) continue;
      qs[i] = abc::query_size(b);
      gen[i] = 1u;
      full_fill(b, qs[i], i, gen[i]);
    }

    for ( usize round = 0; round < 8; ++round ) {
      for ( usize k = 0; k < 1024; ++k ) {
        const usize i = static_cast<usize>(r.next() % N);
        if ( !p[i] ) {
          const usize n = abctest::sample_size_longtail(r);
          p[i] = abc::alloc(n);
          if ( p[i] ) {
            qs[i] = abc::query_size(p[i]);
            gen[i] = 1u;
            full_fill(p[i], qs[i], i, gen[i]);
          }
          continue;
        }
        // verify the block is intact, then grow it (bias larger to force moves)
        require_true(full_check(p[i], qs[i], i, gen[i]));
        const usize n = abctest::SZ_SMALL + static_cast<usize>(r.next() % abctest::SZ_LARGE);
        byte *c = static_cast<byte *>(abc::realloc(p[i], n));
        if ( !c ) continue;
        p[i] = c;
        qs[i] = abc::query_size(c);
        ++gen[i];
        full_fill(c, qs[i], i, gen[i]);
      }
      // full sweep: every live block must still match its own fingerprint
      for ( usize i = 0; i < N; ++i )
        if ( p[i] ) require_true(full_check(p[i], qs[i], i, gen[i]));
    }

    for ( usize i = 0; i < N; ++i )
      if ( p[i] ) abc::dealloc(p[i]);
  }
  end_test_case();

  // ── 4. tier-crossing realloc roundtrip preserves the head prefix ───────────
  test_case("realloc: ascending/descending tier crossings keep the surviving prefix");
  {
    for ( usize start = 0; start < kNB; ++start ) {
      byte *b = abc::alloc(kBoundaries[start]);
      if ( !b ) continue;
      const usize head = abctest::mn(kBoundaries[start], static_cast<usize>(64));
      full_fill(b, head, 0xC0u + static_cast<u32>(start), 7u);

      // walk every boundary size. realloc only preserves min(old,new), so a
      // shrink permanently truncates the data: the still-valid prefix is the
      // running minimum of head and every size seen so far.
      usize surviving = head;
      for ( usize j = 0; j < kNB; ++j ) {
        byte *c = static_cast<byte *>(abc::realloc(b, kBoundaries[j]));
        require_true(c != nullptr);
        require_true(abc::query_size(c) >= kBoundaries[j]);
        b = c;
        surviving = abctest::mn(surviving, kBoundaries[j]);
        require_true(full_check(b, surviving, 0xC0u + static_cast<u32>(start), 7u));
      }
      abc::dealloc(b);
    }
  }
  end_test_case();

  // ── 5. realloc edge corners ────────────────────────────────────────────────
  test_case("realloc: nullptr->alloc, size 0->free, identity");
  {
    byte *a = static_cast<byte *>(abc::realloc(nullptr, 1000));      // acts as alloc
    require_true(a != nullptr);
    require_true(abc::query_size(a) >= 1000);
    full_fill(a, 1000, 0xD0u, 1u);

    byte *b = static_cast<byte *>(abc::realloc(a, 0));      // acts as free
    require_true(b == nullptr);

    byte *c = abc::alloc(4096);
    require_true(c != nullptr);
    const usize qc = abc::query_size(c);
    byte *d = static_cast<byte *>(abc::realloc(c, qc));      // request == usable: no move needed
    require_true(d != nullptr);
    abc::dealloc(d);
  }
  end_test_case();

  sb::print("=== ABCMALLOC REALLOC / QUERY_SIZE RIGOR PASSED ===");
  return 1;
}
