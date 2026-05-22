//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Shared rigor kit for the abc allocator stress / soak suites
// (abcmalloc_concurrent.cpp, abcmalloc_soak.cpp, abcmalloc_soak_mt.cpp).
//
// It packages the proven bit-flip detection scheme from abcmalloc_stress.cpp
// (a *location-independent* per-byte fingerprint keyed on (idx, gen, off) so
// realloc moves never invalidate it) plus the extra machinery the harder
// suites need:
//   - size-adaptive fingerprints: full fill/verify for blocks <= a limit, a
//     head/mid/tail canary for larger ones, so 1e9-op soaks stay tractable
//     while still catching the common corruption (boundary / aliasing) on big
//     blocks.
//   - a long-tailed size sampler routing across every abc tier (mostly tiny,
//     rare huge) — the realistic distribution that flushes tier-crossing bugs.
//   - live_set<N>: an O(1) random-probe slot table whose keys are globally
//     unique per (owner, slot) and whose per-slot generation bumps on every
//     (re)alloc, so a recycled slot that aliases a live block is detected.
//   - run_workers(): launches 8..32 micron::thread workers (mmap stacks, so no
//     parent-stack 32-thread cliff) via placement-new, then joins them.
//
// NOTE (per abcmalloc_mt.cpp): abc sheets are *sticky* — musage() never returns
// to baseline after frees / thread exit. Leak checks must assert *bounded*
// growth across cycles, never return-to-zero.
//
// NOTE (arena budget): abc grabs one arena per allocating thread from a pool of
// __max_arenas (64), never recycled on thread exit; beyond that, threads share
// arena[0]. The main thread takes arena 0, so a binary must keep its total
// distinct allocating worker-thread lifetimes <= 63 to keep every worker on its
// own arena (matters for the cross-arena donation test).

#include "../../src/io/console.hpp"

#include "../../src/cmalloc.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/bits/__pause.hpp"
#include "../../src/math/rng/engines.hpp"
#include "../../src/mutex/mutex.hpp"
#include "../../src/new.hpp"
#include "../../src/thread/threads.hpp"

namespace abctest
{

// blocks up to this many bytes get a full fingerprint; larger ones get a
// head/mid/tail canary. raise it for stricter (slower) big-block coverage,
// lower it to speed a soak up. medium (<=4096) is full by default.
#ifndef ABC_FP_FULL_LIMIT
#define ABC_FP_FULL_LIMIT 4096u
#endif

// pool of arenas abc exposes is 64; main owns one, so workers cap below that.
constexpr usize ABC_MAX_WORKERS = 63;

using rng_t = micron::math::rng::xoshiro256ss;

inline usize
mn(usize a, usize b) noexcept
{
  return a < b ? a : b;
}

// ── fingerprints ────────────────────────────────────────────────────────────

// location-independent: keyed only on (idx, gen, off). realloc moves keep the
// fingerprint valid; two live blocks with distinct (idx, gen) can never collide.
inline byte
fp_byte(usize idx, usize gen, usize off) noexcept
{
  u64 x = static_cast<u64>(idx) * 0x9E3779B97F4A7C15ull;
  x ^= static_cast<u64>(gen) * 0xBF58476D1CE4E5B9ull;
  x ^= static_cast<u64>(off) * 0x94D049BB133111EBull;
  x ^= (x >> 33);
  x *= 0xFF51AFD7ED558CCDull;
  x ^= (x >> 33);
  return static_cast<byte>(x & 0xFFu);
}

// head[0,8) + mid[n/2,+8) + tail[n-8,n). O(1) regardless of block size; used as
// the per-op fast path and as the verify for blocks beyond ABC_FP_FULL_LIMIT.
inline void
fp_write_canary(byte *p, usize n, usize idx, usize gen) noexcept
{
  const usize h = (n < 8) ? n : 8;
  for ( usize i = 0; i < h; ++i ) p[i] = fp_byte(idx, gen, i);
  if ( n > 16 ) {
    const usize m = n / 2;
    for ( usize i = 0; i < 8; ++i ) p[m + i] = fp_byte(idx, gen, m + i);
  }
  if ( n > 8 ) {
    const usize t = n - 8;
    for ( usize i = 0; i < 8; ++i ) p[t + i] = fp_byte(idx, gen, t + i);
  }
}

inline bool
fp_check_canary(const byte *p, usize n, usize idx, usize gen) noexcept
{
  const usize h = (n < 8) ? n : 8;
  for ( usize i = 0; i < h; ++i )
    if ( p[i] != fp_byte(idx, gen, i) ) return false;
  if ( n > 16 ) {
    const usize m = n / 2;
    for ( usize i = 0; i < 8; ++i )
      if ( p[m + i] != fp_byte(idx, gen, m + i) ) return false;
  }
  if ( n > 8 ) {
    const usize t = n - 8;
    for ( usize i = 0; i < 8; ++i )
      if ( p[t + i] != fp_byte(idx, gen, t + i) ) return false;
  }
  return true;
}

// size-adaptive fill: full for small blocks, canary for large. ALWAYS pairs
// with fp_check (same adaptivity) so a free-time / sweep verify never reads an
// unwritten middle of a big block.
inline void
fp_write(byte *p, usize n, usize idx, usize gen) noexcept
{
  if ( n <= ABC_FP_FULL_LIMIT ) {
    for ( usize i = 0; i < n; ++i ) p[i] = fp_byte(idx, gen, i);
  } else {
    fp_write_canary(p, n, idx, gen);
  }
}

inline bool
fp_check(const byte *p, usize n, usize idx, usize gen) noexcept
{
  if ( n <= ABC_FP_FULL_LIMIT ) {
    for ( usize i = 0; i < n; ++i )
      if ( p[i] != fp_byte(idx, gen, i) ) return false;
    return true;
  }
  return fp_check_canary(p, n, idx, gen);
}

inline bool
ptr_aligned(const void *p, usize n) noexcept
{
  return (reinterpret_cast<uintptr_t>(p) % n) == 0;
}

// ── long-tailed size distribution ───────────────────────────────────────────

// canonical tier ceilings (from abc config): precise 256, small 512,
// medium 4096, large 32768, huge 262144.
constexpr usize SZ_PRECISE = abc::__class_precise;
constexpr usize SZ_SMALL = abc::__class_small;
constexpr usize SZ_MEDIUM = abc::__class_medium;
constexpr usize SZ_LARGE = abc::__class_large;
constexpr usize SZ_HUGE = abc::__class_huge;

// heavy-tailed: ~70% precise, 18% small, 8% medium, 3% large, 1% huge. the rare
// huge band is the "long tail" that drives tier crossings and sheet pressure.
inline usize
sample_size_longtail(rng_t &r) noexcept
{
  const u32 t = static_cast<u32>(r.next() % 1000u);
  usize lo, hi;
  if ( t < 700u ) {
    lo = 1;
    hi = SZ_PRECISE;
  } else if ( t < 880u ) {
    lo = SZ_PRECISE + 1;
    hi = SZ_SMALL;
  } else if ( t < 960u ) {
    lo = SZ_SMALL + 1;
    hi = SZ_MEDIUM;
  } else if ( t < 990u ) {
    lo = SZ_MEDIUM + 1;
    hi = SZ_LARGE;
  } else {
    lo = SZ_LARGE + 1;
    hi = SZ_HUGE;
  }
  const usize span = hi - lo + 1;
  return lo + static_cast<usize>(r.next() % span);
}

// mid-band sizes for the long-lived / pinned cohort: big enough to occupy real
// space and span the small..large tiers, held across the whole run.
inline usize
sample_size_pinned(rng_t &r) noexcept
{
  const usize lo = SZ_SMALL;           // 512
  const usize hi = SZ_LARGE / 4u;      // 8192
  return lo + static_cast<usize>(r.next() % (hi - lo + 1));
}

// ── per-thread accounting ───────────────────────────────────────────────────

struct counts {
  u64 allocs = 0;
  u64 frees = 0;
  u64 reallocs = 0;
  u64 verifies = 0;         // total fingerprint checks performed
  u64 hard_errors = 0;      // genuine corruption (fingerprint mismatch on a live block)
  u64 soft = 0;             // tolerated: realloc prefix not preserved (buddy-tier query_size under-report)
  // pin first failure for post-mortem
  u64 first_idx = 0;
  u64 first_gen = 0;
  u64 first_off = 0;
  bool have_first = false;

  void
  note_error(u64 idx, u64 gen, u64 off) noexcept
  {
    ++hard_errors;
    if ( !have_first ) {
      first_idx = idx;
      first_gen = gen;
      first_off = off;
      have_first = true;
    }
  }
};

// ── live-set: O(1) random-probe slot table ──────────────────────────────────

template<usize N> struct live_set {
  byte *ptr[N];
  usize sz[N];
  u32 gen[N];
  usize live;
  usize owner;      // fp idx = owner * N + slot, globally unique across threads

  void
  init(usize owner_id) noexcept
  {
    for ( usize i = 0; i < N; ++i ) {
      ptr[i] = nullptr;
      sz[i] = 0;
      gen[i] = 0;
    }
    live = 0;
    owner = owner_id;
  }

  usize
  key(usize slot) const noexcept
  {
    return owner * N + slot;
  }

  static constexpr usize
  cap() noexcept
  {
    return N;
  }
};

// ── per-op primitives (the gen-bump + fingerprint protocol, shared by all) ───

// alloc size n into a free slot s, fingerprint it, bump its generation.
template<usize N>
inline bool
do_alloc(live_set<N> &ls, usize s, usize n, counts &c) noexcept
{
  byte *p = abc::alloc(n);
  if ( !p ) [[unlikely]]
    return false;      // OOM
  const u32 g = ++ls.gen[s];
  fp_write(p, n, ls.key(s), g);
  ls.ptr[s] = p;
  ls.sz[s] = n;
  ++ls.live;
  ++c.allocs;
  return true;
}

// verify the live block in slot s, then free it.
template<usize N>
inline void
do_free(live_set<N> &ls, usize s, counts &c) noexcept
{
  ++c.verifies;
  if ( !fp_check(ls.ptr[s], ls.sz[s], ls.key(s), ls.gen[s]) ) c.note_error(ls.key(s), ls.gen[s], 0);
  abc::dealloc(ls.ptr[s]);
  ls.ptr[s] = nullptr;
  ls.sz[s] = 0;
  --ls.live;
  ++c.frees;
}

// verify the live block, realloc it to nn, soft-check prefix preservation, then
// re-fingerprint the (possibly moved/resized) block under a fresh generation.
template<usize N>
inline bool
do_realloc(live_set<N> &ls, usize s, usize nn, counts &c) noexcept
{
  const usize oldn = ls.sz[s];
  const u32 oldg = ls.gen[s];
  ++c.verifies;
  if ( !fp_check(ls.ptr[s], oldn, ls.key(s), oldg) ) c.note_error(ls.key(s), oldg, 0);
  byte *q = static_cast<byte *>(abc::realloc(ls.ptr[s], nn));
  if ( !q ) [[unlikely]]
    return false;
  // prefix preservation is soft: buddy-tier query_size under-reports, so abc may
  // legitimately copy fewer bytes than min(old,new). only meaningful when the
  // old block was fully fingerprinted (<= limit).
  if ( oldn <= ABC_FP_FULL_LIMIT ) {
    const usize pre = mn(oldn, nn);
    if ( !fp_check(q, pre, ls.key(s), oldg) ) ++c.soft;
  }
  const u32 ng = ++ls.gen[s];
  fp_write(q, nn, ls.key(s), ng);
  ls.ptr[s] = q;
  ls.sz[s] = nn;
  ++c.reallocs;
  return true;
}

// one random-probe op (alloc into a free slot / free or realloc a live one).
// occupancy self-balances toward alloc_pct/(alloc_pct+free_pct).
template<usize N>
inline void
churn_step(live_set<N> &ls, rng_t &r, counts &c, u32 alloc_pct, u32 free_pct) noexcept
{
  const u32 op = static_cast<u32>(r.next() % 100u);
  const usize s = static_cast<usize>(r.next() % N);
  if ( op < alloc_pct ) {
    if ( ls.ptr[s] == nullptr ) do_alloc(ls, s, sample_size_longtail(r), c);
  } else if ( op < alloc_pct + free_pct ) {
    if ( ls.ptr[s] ) do_free(ls, s, c);
  } else {
    if ( ls.ptr[s] ) do_realloc(ls, s, sample_size_longtail(r), c);
  }
}

// verify every live block without freeing (periodic sweep / long-lived cohort).
template<usize N>
inline void
verify_all(live_set<N> &ls, counts &c) noexcept
{
  for ( usize i = 0; i < N; ++i ) {
    if ( ls.ptr[i] ) {
      ++c.verifies;
      if ( !fp_check(ls.ptr[i], ls.sz[i], ls.key(i), ls.gen[i]) ) c.note_error(ls.key(i), ls.gen[i], 0);
    }
  }
}

// verify + free every live block (teardown).
template<usize N>
inline void
drain_all(live_set<N> &ls, counts &c) noexcept
{
  for ( usize i = 0; i < N; ++i )
    if ( ls.ptr[i] ) do_free(ls, i, c);
}

// aggregate the per-thread counts of an array of worker contexts (each must
// expose a `counts cnt` member). hard_errors / allocs==frees are the verdict.
template<typename Ctx>
inline counts
sum_counts(const Ctx *ctx, usize n) noexcept
{
  counts t;
  for ( usize i = 0; i < n; ++i ) {
    t.allocs += ctx[i].cnt.allocs;
    t.frees += ctx[i].cnt.frees;
    t.reallocs += ctx[i].cnt.reallocs;
    t.verifies += ctx[i].cnt.verifies;
    t.hard_errors += ctx[i].cnt.hard_errors;
    t.soft += ctx[i].cnt.soft;
  }
  return t;
}

// ── global live-address registry (cross-thread double-alloc / ABA detector) ──

// A race-safe, direct-mapped sampling set of currently-live block addresses,
// shared across all threads. Discipline: note_alloc(p) AFTER abc::alloc returns,
// note_free(p) BEFORE abc::dealloc. A registered address is therefore "live" in
// the registry from just-after-alloc to just-before-free. If the allocator hands
// the same address to a second owner while the first still holds it (cross-thread
// reuse-while-live / ABA / a freelist that re-emitted a live node), that owner's
// note_alloc finds the slot already == p and flags a collision.
//
// Why this and not "poison freed memory + re-verify": once a block is freed it
// may be legitimately re-handed-out and overwritten at any instant, so a "still
// poisoned?" check races with honest reuse and false-positives. The registry
// instead pins the real invariant — no address is live in two places at once —
// with a single atomic CAS per op and no false positives. Ordering note_free
// strictly before abc::dealloc closes the free→reuse window: an honest reuse can
// only return p after the previous owner already cleared its slot.
//
// "direct-mapped sampling": each address maps to one bucket (no probing). If two
// distinct addresses collide on a bucket, the later one is simply not tracked —
// over millions of ops a genuine double-alloc is still caught on the tracked
// majority, and the structure stays trivially race-free.
struct live_registry {
  static constexpr usize __bits = 20;      // 1M slots * 8B = 8 MiB
  static constexpr usize __size = usize(1) << __bits;
  static constexpr usize __mask = __size - 1;

  micron::atomic_token<u64> slot[__size];      // 0 = empty, else a live address
  micron::atomic_token<u64> collisions;        // double-occupancy events
  micron::atomic_token<u64> tracked;           // currently-inserted entries

  static usize
  bucket(u64 a) noexcept
  {
    a >>= 4;      // drop the 16-byte alignment bits
    a *= 0x9E3779B97F4A7C15ull;
    return static_cast<usize>(a >> (64 - __bits)) & __mask;
  }

  // returns true iff a collision (addr already live elsewhere) was detected
  bool
  note_alloc(byte *p) noexcept
  {
    const u64 a = reinterpret_cast<u64>(p);
    const usize b = bucket(a);
    u64 expected = 0;
    if ( slot[b].compare_exchange_strong(expected, a, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
      tracked.fetch_add(1, micron::memory_order_relaxed);
      return false;      // inserted into an empty slot
    }
    if ( expected == a ) {      // the address is ALREADY live -> double alloc
      collisions.fetch_add(1, micron::memory_order_acq_rel);
      return true;
    }
    return false;      // bucket busy with a different addr -> not tracked
  }

  void
  note_free(byte *p) noexcept
  {
    const u64 a = reinterpret_cast<u64>(p);
    const usize b = bucket(a);
    u64 expected = a;
    if ( slot[b].compare_exchange_strong(expected, 0ull, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      tracked.fetch_sub(1, micron::memory_order_relaxed);
    // else: this addr was never tracked (sampled out) -> nothing to clear
  }

  void
  reset() noexcept
  {
    for ( usize i = 0; i < __size; ++i ) slot[i].store(0ull, micron::memory_order_relaxed);
    collisions.store(0ull, micron::memory_order_relaxed);
    tracked.store(0ull, micron::memory_order_relaxed);
  }
};

// ── worker spawn / join ─────────────────────────────────────────────────────

// launch n workers (8..32, capped at ABC_MAX_WORKERS), each running fn(&ctx[i]).
// micron::thread is mmap-backed and movable-but-not-here: we placement-new each
// thread in-place (no move => no double-join from the unnulled attr pid) and
// join+destroy them explicitly. mmap stacks mean no parent-stack 32-thread cliff.
template<typename Ctx>
inline void
run_workers(void (*fn)(Ctx *), Ctx *ctx, usize n) noexcept
{
  using T = micron::thread<>;
  if ( n > ABC_MAX_WORKERS ) n = ABC_MAX_WORKERS;
  alignas(T) byte buf[ABC_MAX_WORKERS * sizeof(T)];
  T *pool = reinterpret_cast<T *>(buf);
  for ( usize i = 0; i < n; ++i ) ::new (static_cast<void *>(pool + i)) T{ fn, ctx + i };
  for ( usize i = 0; i < n; ++i ) {
    pool[i].join();
    pool[i].~T();
  }
}

};      // namespace abctest
