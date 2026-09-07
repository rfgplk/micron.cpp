//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// The barebones allocator against abcmalloc, on the paths micron actually uses.
//
// ONE BINARY CANNOT HOLD BOTH ALLOCATORS -- the choice is a compile-time #if in defs.hpp -- so the
// comparison is between two builds of THIS file and the header line says which one you are reading:
//
//   duck build benches/bb_alloc_bench.cpp --perf --fp --no-ssp --no-lto -o bin/b       # abcmalloc
//   duck build benches/bb_alloc_bench.cpp --perf --fp --no-ssp --no-lto \
//        --def MICRON_BAREBONES_ALLOC --def MICRON_NO_ZZZ_HASH -o bin/bb              # bbmalloc
//   taskset -c 2 bin/b/bb_alloc_bench ; taskset -c 2 bin/bb/bb_alloc_bench
//
// Every one of --perf --fp --no-ssp --no-lto matters: duck defaults to -fstack-protector-all, a
// canary on every function, and it does not cancel out of a ratio.
//
// MICRON_NO_ZZZ_HASH is pinned on the barebones side for the same reason
// verify_compile_barebones.duck pins it -- micron::hashes::zzz loses hopscotch inserts above
// -march=x86-64-v2 (ISSUES.md), and a map bench that silently holds fewer keys is not measuring the
// allocator. Pass it to BOTH builds or the map rows compare different workloads.

#include "../src/maps.hpp"
#include "../src/memory/allocation/__internal.hpp"
#include "../src/print.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector.hpp"

#include <cstdio>

namespace
{

[[gnu::always_inline]] inline u64
cycles() noexcept
{
#if defined(__x86_64__) || defined(__i386__)
  u32 lo = 0;
  u32 hi = 0;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
#elif defined(__aarch64__)
  u64 v = 0;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#else
  return static_cast<u64>(micron::port::mono_ticks());
#endif
}

template<typename T>
[[gnu::always_inline]] inline void
clobber(T *p) noexcept
{
  __asm__ __volatile__("" : : "g"(p) : "memory");
}

constexpr int k_measure = 7;
constexpr int k_warmup = 2;

u64
median(u64 *v, int n) noexcept
{
  for ( int i = 1; i < n; ++i ) {
    const u64 x = v[i];
    int j = i - 1;
    while ( j >= 0 && v[j] > x ) {
      v[j + 1] = v[j];
      --j;
    }
    v[j + 1] = x;
  }
  return v[n / 2];
}

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E37'79B9'7F4A'7C15ull;
  x = (x ^ (x >> 30)) * 0xBF58'476D'1CE4'E5B9ull;
  x = (x ^ (x >> 27)) * 0x94D0'49BB'1331'11EBull;
  return x ^ (x >> 31);
}

void
report(const char *name, usize sz, u64 cyc, u64 ops)
{
  printf("[%s] size=%zu cycles/op=%.2f\n", name, static_cast<size_t>(sz),
         static_cast<double>(cyc) / static_cast<double>(ops));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (a) the raw pair, straight at the seam every container reaches through

constexpr usize k_sizes[] = { 16, 64, 256, 1024, 4096, 65536 };
constexpr usize k_pair_ops = 200000;

void
bench_pair()
{
  for ( usize s : k_sizes ) {
    u64 runs[k_measure];
    for ( int m = 0; m < k_warmup + k_measure; ++m ) {
      const u64 t0 = cycles();
      for ( usize i = 0; i < k_pair_ops; ++i ) {
        byte *p = micron::__alloc(s);
        clobber(p);
        micron::__free(p);
      }
      const u64 t1 = cycles();
      if ( m >= k_warmup ) runs[m - k_warmup] = t1 - t0;
    }
    report("pair alloc+free", s, median(runs, k_measure), k_pair_ops);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (b) live churn -- a working set the allocator has to keep, not a hot single block

constexpr usize k_live = 4096;
byte *live[k_live];
constexpr usize k_churn_ops = 200000;

void
bench_churn()
{
  for ( usize s : k_sizes ) {
    if ( s > 4096 ) break;
    u64 runs[k_measure];
    for ( int m = 0; m < k_warmup + k_measure; ++m ) {
      for ( usize i = 0; i < k_live; ++i ) live[i] = micron::__alloc(s);
      u64 r = 0x5040'A11ull;
      const u64 t0 = cycles();
      for ( usize i = 0; i < k_churn_ops; ++i ) {
        const usize slot = static_cast<usize>(splitmix64(r += 1)) % k_live;
        micron::__free(live[slot]);
        live[slot] = micron::__alloc(s);
        clobber(live[slot]);
      }
      const u64 t1 = cycles();
      for ( usize i = 0; i < k_live; ++i ) micron::__free(live[i]);
      if ( m >= k_warmup ) runs[m - k_warmup] = t1 - t0;
    }
    report("churn 4096-live", s, median(runs, k_measure), k_churn_ops);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (c) the container path -- what the allocation policy actually asks for

constexpr usize k_push = 200000;
constexpr usize k_keys = 100000;
constexpr usize k_appends = 100000;

void
bench_containers()
{
  {
    u64 runs[k_measure];
    for ( int m = 0; m < k_warmup + k_measure; ++m ) {
      const u64 t0 = cycles();
      micron::vector<u64> v;
      for ( u64 i = 0; i < k_push; ++i ) v.push_back(i);
      const u64 t1 = cycles();
      clobber(&v);
      if ( m >= k_warmup ) runs[m - k_warmup] = t1 - t0;
    }
    report("vector push_back", k_push, median(runs, k_measure), k_push);
  }
  {
    u64 runs[k_measure];
    for ( int m = 0; m < k_warmup + k_measure; ++m ) {
      const u64 t0 = cycles();
      micron::hopscotch_map<u64, u64> h;
      for ( u64 i = 0; i < k_keys; ++i ) h.insert(splitmix64(i), i);
      const u64 t1 = cycles();
      clobber(&h);
      if ( m >= k_warmup ) runs[m - k_warmup] = t1 - t0;
    }
    report("hopscotch insert", k_keys, median(runs, k_measure), k_keys);
  }
  {
    u64 runs[k_measure];
    for ( int m = 0; m < k_warmup + k_measure; ++m ) {
      const u64 t0 = cycles();
      micron::string s = "b";
      for ( usize i = 0; i < k_appends; ++i ) s += "0123456789";
      const u64 t1 = cycles();
      clobber(&s);
      if ( m >= k_warmup ) runs[m - k_warmup] = t1 - t0;
    }
    report("string append10", k_appends, median(runs, k_measure), k_appends);
  }
}

}

int
main()
{
#if defined(__micron_bb_alloc)
  printf("# allocator: micron::bb (bbmalloc -- buddy + TLSF over fixed regions)\n");
  printf("# profile: class_small=%zu min_block=%zu max_regions=%u growable=%d\n", static_cast<size_t>(bb::__class_small),
         static_cast<size_t>(bb::__default_min_block), static_cast<unsigned>(bb::__max_regions),
         static_cast<int>(bb::__growable));
#else
  printf("# allocator: abcmalloc\n");
#endif
  printf("# estimator=median of %d, %d warm-ups, cycles via rdtsc/cntvct\n", k_measure, k_warmup);
  bench_pair();
  bench_churn();
  bench_containers();
#if defined(__micron_bb_alloc)
  printf("# heap: capacity=%zu regions=%u\n", static_cast<size_t>(bb::capacity()),
         static_cast<unsigned>(bb::__the_heap.count));
#endif
  return 1;
}
