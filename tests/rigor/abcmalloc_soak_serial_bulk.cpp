//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC SERIAL BULK SOAK — single-threaded, ~8 GiB live working set.
//
// Derived from abcmalloc_soak.cpp but reshaped for *serial bulk* pressure:
//   - heavy long SERIAL allocation chains (runs of 64..1024 allocs) whose sizes
//     are interleaved across every bucket/tier (precise..huge),
//   - sporadic realloc chains and free chains,
//   - a stochastic sweep, two independent probabilities (do NOT conflate them):
//       * per sweep-op TRIGGER: each chain has a 3-5% chance (BULK_SWEEP_PERMILLE)
//         to fire a free cascade at all;
//       * per-POINTER: once a cascade fires, EACH live pointer is independently
//         freed with 20% probability (BULK_SWEEP_FREE_PCT) — ~20% go, ~80% stay;
//     both driven by our xoshiro256 RNG,
//   - the live set is driven up to and held around BULK_TARGET_BYTES (~8 GiB).
//
// Fingerprinting is DEEP and BIT-EXACT: every block is filled with a
// splitmix64(key,gen) byte stream (written/compared 8 bytes at a time for speed,
// so 8 GiB stays tractable). Every free, every realloc, and a periodic full
// sweep re-verify each live block end to end; the first byte that differs by even
// one bit is located (offset + expected/got + which bit flipped) and the test
// ABORTS immediately.
//
// Box has 32 GiB; target ~8 GiB of *requested* live bytes (musage runs higher due
// to tier rounding + sticky sheets). A soft cap pauses allocation and a hard cap
// aborts before the box is endangered.
//
// Build: duck build tests/rigor/abcmalloc_soak_serial_bulk.cpp
// Tune : -D BULK_TARGET_BYTES=<n> -D BULK_OPS=<n> -D BULK_VERIFY_EVERY=<n>
//        -D BULK_SWEEP_PERMILLE=<0..1000> -D BULK_SWEEP_FREE_PCT=<0..100>

#include "../../src/io/console.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;
using rng_t = micron::math::rng::xoshiro256ss;
using micron::math::rng::splitmix64;

namespace
{

#ifndef BULK_TARGET_BYTES
#define BULK_TARGET_BYTES (8ull << 30)      // ~8 GiB requested-live target
#endif
#ifndef BULK_OPS
#define BULK_OPS 4000000ull      // total ops; fill to ~8 GiB needs ~1.3M, rest churns
#endif
#ifndef BULK_VERIFY_EVERY
#define BULK_VERIFY_EVERY 500000ull      // deep full-sweep verify cadence
#endif
#ifndef BULK_SWEEP_PERMILLE
#define BULK_SWEEP_PERMILLE 35u      // 3.5% of chains fire a stochastic sweep (in [3,5]%)
#endif
#ifndef BULK_SWEEP_FREE_PCT
#define BULK_SWEEP_FREE_PCT 20u      // a fired sweep frees each live ptr w/ 20% prob
#endif

// chains are LONG: allocation throughput per inter-sweep window must exceed the
// 20% a sweep removes, otherwise live equilibrates well below target instead of
// climbing to it. (At 64..1024 it stalled around ~1-2 GiB.)
constexpr usize BULK_CHAIN_MIN = 1024;
constexpr usize BULK_CHAIN_MAX = 8192;
constexpr u64 BULK_SOFT_CAP = 24ull << 30;      // pause allocation past this musage
constexpr u64 BULK_HARD_CAP = 30ull << 30;      // abort past this musage (protect the box)

constexpr usize NSLOT = 1u << 20;      // 1,048,576 owned-pointer slots (20 MiB of table)

byte *g_ptr[NSLOT];
usize g_sz[NSLOT];
u32 g_gen[NSLOT];
usize g_live_count = 0;
u64 g_live_bytes = 0;

// ── bit-exact per-block fingerprint via splitmix64(key,gen) ─────────────────

inline u64
bulk_seed(usize key, u32 gen) noexcept
{
  return (static_cast<u64>(key) * 0x9E3779B97F4A7C15ull) ^ (static_cast<u64>(gen) * 0xD1B54A32D192ED03ull) ^ 0xCAFEF00DBAADC0DEull;
}

// abc returns >=16-byte-aligned pointers, so the 8-byte stores/loads are aligned.
inline void
bulk_fill(byte *p, usize n, usize key, u32 gen) noexcept
{
  splitmix64 sm{ bulk_seed(key, gen) };
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) *reinterpret_cast<u64 *>(p + i) = sm.next();
  if ( i < n ) {
    const u64 v = sm.next();
    for ( usize j = 0; i < n; ++i, ++j ) p[i] = static_cast<byte>(v >> (j * 8));
  }
}

// returns true and fills (off,exp,got) at the first byte that differs
inline bool
bulk_first_bad(const byte *p, usize n, usize key, u32 gen, usize &off, byte &exp, byte &got) noexcept
{
  splitmix64 sm{ bulk_seed(key, gen) };
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    const u64 v = sm.next();
    const u64 r = *reinterpret_cast<const u64 *>(p + i);
    if ( r != v ) {
      for ( usize j = 0; j < 8; ++j ) {
        const byte e = static_cast<byte>(v >> (j * 8));
        if ( p[i + j] != e ) {
          off = i + j;
          exp = e;
          got = p[i + j];
          return true;
        }
      }
    }
  }
  if ( i < n ) {
    const u64 v = sm.next();
    for ( usize j = 0; i < n; ++i, ++j ) {
      const byte e = static_cast<byte>(v >> (j * 8));
      if ( p[i] != e ) {
        off = i;
        exp = e;
        got = p[i];
        return true;
      }
    }
  }
  return false;
}

// deep verify; on ANY single-bit divergence: locate it, report, and abort now.
inline void
bulk_verify(const byte *p, usize n, usize key, u32 gen, usize slot, const char *where)
{
  usize off = 0;
  byte exp = 0, got = 0;
  if ( bulk_first_bad(p, n, key, gen, off, exp, got) ) [[unlikely]] {
    const unsigned x = static_cast<unsigned>(exp ^ got);
    sb::print("  *** BIT CORRUPTION *** during ", where);
    sb::print("     slot=", slot, " gen=", static_cast<usize>(gen), " off=", off, " of n=", n);
    sb::print("     expected=", static_cast<usize>(exp), " got=", static_cast<usize>(got), " xor=", static_cast<usize>(x),
              " first-flipped-bit=", static_cast<usize>(__builtin_ctz(x)));
    sb::print("     ptr=", reinterpret_cast<const void *>(p), " qsize=", abc::query_size(const_cast<byte *>(p)),
              " live_count=", g_live_count, " live_bytes=", g_live_bytes);
    require_true(false);      // immediate abort, as requested
  }
}

// ── size sampler: interleave across all buckets, weighted to reach ~8 GiB ────

// abc tier routing (arena.hpp __vmap_alloc): precise sz<=512, small <4096,
// medium <=32768, large <=262144, huge >262144. The medium tier has the most
// sheets (512) and the large tier only 64 (~1 GiB cap), so a few % of large
// blocks already dominate the byte volume and exhaust that tier — to actually
// hold ~8 GiB the distribution is medium-dominant, with large/huge a thin tail
// for genuine bucket interleave (their allocs gracefully fail once capped).
inline usize
sample_size_bulk(rng_t &r) noexcept
{
  const u32 t = static_cast<u32>(r.next() % 1000u);
  usize lo, hi;
  if ( t < 100u ) {      // 10% precise (<=512)
    lo = 1;
    hi = abctest::SZ_PRECISE;
  } else if ( t < 350u ) {      // 25% small (513..4095)
    lo = abctest::SZ_PRECISE + 1;
    hi = abctest::SZ_MEDIUM - 1;
  } else if ( t < 980u ) {      // 63% medium (4096..32768) — the workhorse tier
    lo = abctest::SZ_MEDIUM;
    hi = abctest::SZ_LARGE;
  } else if ( t < 995u ) {      // 1.5% large (32769..262144)
    lo = abctest::SZ_LARGE + 1;
    hi = abctest::SZ_HUGE;
  } else {      // 0.5% huge (>262144)
    lo = abctest::SZ_HUGE + 1;
    hi = abctest::SZ_HUGE + abctest::SZ_HUGE;      // up to 512 KiB
  }
  return lo + static_cast<usize>(r.next() % (hi - lo + 1));
}

// ── slot ops (each verifies bit-exact where it must) ────────────────────────

inline bool
alloc_slot(usize s, usize n)
{
  byte *p = abc::alloc(n);
  if ( !p ) [[unlikely]]
    return false;
  const u32 g = ++g_gen[s];
  bulk_fill(p, n, s, g);
  g_ptr[s] = p;
  g_sz[s] = n;
  ++g_live_count;
  g_live_bytes += n;
  return true;
}

inline void
free_slot(usize s, const char *where)
{
  bulk_verify(g_ptr[s], g_sz[s], s, g_gen[s], s, where);      // must be intact at free
  abc::dealloc(g_ptr[s]);
  g_live_bytes -= g_sz[s];
  g_ptr[s] = nullptr;
  g_sz[s] = 0;
  --g_live_count;
}

inline void
realloc_slot(usize s, usize nn, const char *where)
{
  bulk_verify(g_ptr[s], g_sz[s], s, g_gen[s], s, where);      // old must be intact
  // abc::realloc THROWS memory_error_abc_realloc_unknown when the destination
  // tier is exhausted (vs abc::alloc which returns null). resize() does push(new)
  // BEFORE pop(old), so on failure the OLD block is untouched and still live — so
  // a failed realloc is safe: keep the old block and move on.
  byte *q = nullptr;
  try {
    q = static_cast<byte *>(abc::realloc(g_ptr[s], nn));
  } catch ( ... ) {
    q = nullptr;
  }
  if ( !q ) [[unlikely]] {
    bulk_verify(g_ptr[s], g_sz[s], s, g_gen[s], s, "realloc-failed-old-survives");
    return;
  }
  g_live_bytes += static_cast<u64>(nn) - static_cast<u64>(g_sz[s]);
  const u32 g = ++g_gen[s];
  bulk_fill(q, nn, s, g);      // re-fingerprint whole new block
  g_ptr[s] = q;
  g_sz[s] = nn;
}

inline usize
find_free(rng_t &r) noexcept
{
  for ( int t = 0; t < 64; ++t ) {
    const usize s = static_cast<usize>(r.next() % NSLOT);
    if ( !g_ptr[s] ) return s;
  }
  return NSLOT;
}

inline usize
find_live(rng_t &r) noexcept
{
  for ( int t = 0; t < 64; ++t ) {
    const usize s = static_cast<usize>(r.next() % NSLOT);
    if ( g_ptr[s] ) return s;
  }
  return NSLOT;
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC SERIAL BULK SOAK (single-threaded, ~8 GiB) ===");
  sb::print("    target=", static_cast<usize>(BULK_TARGET_BYTES), " ops=", static_cast<usize>(BULK_OPS),
            " verify_every=", static_cast<usize>(BULK_VERIFY_EVERY));
  sb::print("    sweep=", static_cast<usize>(BULK_SWEEP_PERMILLE), "/1000 chains, frees ", static_cast<usize>(BULK_SWEEP_FREE_PCT),
            "% of live; slots=", static_cast<usize>(NSLOT));

  rng_t r = rng_t::from_seed(0xB01CA11FEED5040ull);
  const usize base_usage = abc::musage();
  bool reached = false;
  bool capped = false;      // tiers can't grow live further right now (a sweep re-opens room)
  u64 stall = 0;            // consecutive alloc chains that allocated nothing
  u64 sweeps = 0, alloc_chains = 0, realloc_chains = 0, free_chains = 0;
  u64 total_allocs = 0, total_frees = 0, total_reallocs = 0, total_verifies = 0;
  usize peak_usage = base_usage;
  u64 peak_live_bytes = 0;

  test_case("serial bulk: long alloc chains + sporadic realloc/free + stochastic sweep");
  u64 ops = 0, last_verify = 0;
  while ( ops < static_cast<u64>(BULK_OPS) ) {
    const u32 roll = static_cast<u32>(r.next() % 1000u);

    if ( roll < static_cast<u32>(BULK_SWEEP_PERMILLE) ) {
      // ── stochastic sweep: free ~BULK_SWEEP_FREE_PCT% of ALL owned pointers ──
      ++sweeps;
      for ( usize s = 0; s < NSLOT; ++s )
        if ( g_ptr[s] && (static_cast<u32>(r.next() % 100u) < static_cast<u32>(BULK_SWEEP_FREE_PCT)) ) {
          free_slot(s, "stochastic-sweep");
          ++total_frees;
          ++total_verifies;
          ++ops;
        }
      capped = false;      // freeing ~20% reopens tier room for fresh growth
      stall = 0;
    } else if ( !capped && g_live_bytes < static_cast<u64>(BULK_TARGET_BYTES) && abc::musage() < BULK_SOFT_CAP ) {
      // ── below target: heavy serial alloc chain, with sporadic realloc mixed in ──
      const u32 sub = static_cast<u32>(r.next() % 100u);
      const usize L = BULK_CHAIN_MIN + static_cast<usize>(r.next() % (BULK_CHAIN_MAX - BULK_CHAIN_MIN + 1));
      if ( sub < 97u ) {      // heavy alloc; realloc kept sporadic (fragments the buddy)
        ++alloc_chains;
        usize succ = 0;
        for ( usize k = 0; k < L; ++k ) {
          const usize s = find_free(r);
          if ( s == NSLOT ) break;
          if ( !alloc_slot(s, sample_size_bulk(r)) ) continue;      // this size's tier is full; try another
          ++total_allocs;
          ++ops;
          ++succ;
          if ( g_live_bytes >= static_cast<u64>(BULK_TARGET_BYTES) ) {
            if ( !reached ) {
              reached = true;
              sb::print("   reached target: live_bytes=", g_live_bytes, " live_count=", g_live_count, " musage=", abc::musage());
            }
            break;
          }
        }
        if ( succ == 0 ) {
          if ( ++stall >= 4 && !capped ) {
            capped = true;
            sb::print("   capacity plateau: live_bytes=", g_live_bytes, " live_count=", g_live_count, " musage=", abc::musage());
          }
        } else {
          stall = 0;
        }
      } else {
        ++realloc_chains;
        for ( usize k = 0; k < L; ++k ) {
          const usize s = find_live(r);
          if ( s == NSLOT ) break;
          realloc_slot(s, sample_size_bulk(r), "realloc-chain");
          ++total_reallocs;
          ++total_verifies;
          ++ops;
        }
      }
    } else if ( g_live_count > 0 ) {
      // ── at target (or capacity-capped): sporadic realloc / free chains ──
      // when capped we bias to realloc so the live set churns in place rather than
      // draining; at target we mix frees in to make room for refills.
      const usize L = BULK_CHAIN_MIN + static_cast<usize>(r.next() % (BULK_CHAIN_MAX - BULK_CHAIN_MIN + 1));
      // bias to free so live oscillates down (then the alloc branch refills);
      // realloc stays sporadic to avoid buddy fragmentation inflating musage.
      // once musage is over the soft budget, free ONLY (realloc also maps new
      // blocks via resize and would creep musage toward the hard cap).
      bool do_free;
      if ( abc::musage() >= BULK_SOFT_CAP )
        do_free = true;
      else if ( capped )
        do_free = false;
      else
        do_free = (static_cast<u32>(r.next() % 100u) < 90u);
      if ( !do_free ) {
        ++realloc_chains;
        for ( usize k = 0; k < L; ++k ) {
          const usize s = find_live(r);
          if ( s == NSLOT ) break;
          realloc_slot(s, sample_size_bulk(r), "realloc-chain");
          ++total_reallocs;
          ++total_verifies;
          ++ops;
        }
      } else {
        ++free_chains;
        for ( usize k = 0; k < L; ++k ) {
          const usize s = find_live(r);
          if ( s == NSLOT ) break;
          free_slot(s, "free-chain");
          ++total_frees;
          ++total_verifies;
          ++ops;
        }
      }
    } else {
      // nothing live and not allocating (shouldn't persist) — nudge one alloc
      const usize s = find_free(r);
      if ( s != NSLOT && alloc_slot(s, sample_size_bulk(r)) ) {
        ++total_allocs;
        ++ops;
        capped = false;
        stall = 0;
      } else {
        ++ops;      // guarantee forward progress
      }
    }

    if ( g_live_bytes > peak_live_bytes ) peak_live_bytes = g_live_bytes;

    // ── periodic DEEP full-sweep verify of every live block + safety caps ──
    if ( ops - last_verify >= static_cast<u64>(BULK_VERIFY_EVERY) ) {
      usize checked = 0;
      for ( usize s = 0; s < NSLOT; ++s )
        if ( g_ptr[s] ) {
          bulk_verify(g_ptr[s], g_sz[s], s, g_gen[s], s, "periodic-sweep");
          ++total_verifies;
          ++checked;
        }
      const usize u = abc::musage();
      if ( u > peak_usage ) peak_usage = u;
      sb::print("   ops=", static_cast<usize>(ops), " live_count=", g_live_count, " live_bytes=", g_live_bytes, " musage=", u,
                " verified=", checked, " sweeps=", static_cast<usize>(sweeps));
      require_true(u < BULK_HARD_CAP);      // never endanger the box
      last_verify = ops;
    }
  }
  end_test_case();

  // ── teardown: deep-verify and free everything still live ──
  for ( usize s = 0; s < NSLOT; ++s )
    if ( g_ptr[s] ) {
      free_slot(s, "teardown");
      ++total_frees;
      ++total_verifies;
    }
  const usize end_usage = abc::musage();

  sb::print("    chains: alloc=", static_cast<usize>(alloc_chains), " realloc=", static_cast<usize>(realloc_chains),
            " free=", static_cast<usize>(free_chains), " sweeps=", static_cast<usize>(sweeps));
  sb::print("    ops: allocs=", static_cast<usize>(total_allocs), " frees=", static_cast<usize>(total_frees),
            " reallocs=", static_cast<usize>(total_reallocs), " verifies=", static_cast<usize>(total_verifies));
  sb::print("    peak live_bytes=", peak_live_bytes, " peak musage=", peak_usage, " end musage=", end_usage, " base=", base_usage);

  require(g_live_count, static_cast<usize>(0));
  require(g_live_bytes, static_cast<u64>(0));
  require_true(reached);      // we actually hit the ~8 GiB target
  require_true(end_usage > 0);
  sb::print("=== ABCMALLOC SERIAL BULK SOAK PASSED ===");
  return 1;
}
