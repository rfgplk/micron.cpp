// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Raw TLSF/buddy microbenchmarks.  The public abcmalloc fast path has a
// per-tier cache which deliberately hides these allocators on immediate
// alloc/free pairs; this binary bypasses that layer to expose splitting,
// bitmap maintenance, free-list removal, coalescing, metadata locality and
// independent-stream instruction-level parallelism directly.
//
// Build:
//   duck build benches/abcmalloc_core_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/abc
//
// Measure one kernel at a time so perf stat does not mix workloads:
//   perf stat -r 5 -e cycles,instructions,branches,branch-misses,cache-references,cache-misses
//     bin/abc/abcmalloc_core_bench tlsf_fragment

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/memory/allocation/abcmalloc/cache_list.hpp"
#include "../src/memory/allocation/abcmalloc/free_list.hpp"
#include "../src/types.hpp"

namespace
{

constexpr usize POOL_BYTES = 64ULL << 20;
constexpr usize MAX_LIVE = 1ULL << 16;
constexpr usize BATCH_LIVE = 1ULL << 12;
constexpr u64 ROUND_TRIPS = 4'000'000ULL;
constexpr u64 CHURN_OPS = 4'000'000ULL;

alignas(4096) static byte g_pool[4][POOL_BYTES];
alignas(64) static micron::__chunk<byte> g_live[MAX_LIVE];

using tlsf_t = abc::__tlsf_list<micron::__chunk<byte>, 256, 64>;
using buddy_t = abc::__buddy_list<micron::__chunk<byte>, 4096, 64>;

struct result {
  u64 cycles;
  u64 ops;
  u64 checksum;
};

[[gnu::always_inline]] inline u64
tsc() noexcept
{
  asm volatile("" ::: "memory");
  const u64 v = micron::math::__asm_op::rdtsc64();
  asm volatile("" ::: "memory");
  return v;
}

[[gnu::always_inline]] inline void
escape(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline u32
mix(u32 x) noexcept
{
  x ^= x >> 16;
  x *= 0x7FEB352Du;
  x ^= x >> 15;
  x *= 0x846CA68Bu;
  return x ^ (x >> 16);
}

bool
same(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

[[gnu::noinline]] result
tlsf_roundtrip() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < ROUND_TRIPS; ++i ) {
    micron::__chunk<byte> p = alloc.allocate(1 + (mix(static_cast<u32>(i)) & 511u));
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 255u;
    alloc.deallocate(p.ptr);
  }
  const u64 end = tsc();
  return { end - begin, ROUND_TRIPS * 2, sum };
}

[[gnu::noinline]] result
tlsf_temporal_hit() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  micron::__chunk<byte> first = alloc.temporal_allocate(64);
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < ROUND_TRIPS; ++i ) {
    micron::__chunk<byte> p = alloc.temporal_allocate(64);
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 255u;
  }
  const u64 end = tsc();
  alloc.deallocate(first.ptr);
  return { end - begin, ROUND_TRIPS, sum };
}

[[gnu::noinline]] result
tlsf_temporal_churn() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < CHURN_OPS; ++i ) {
    micron::__chunk<byte> p = alloc.temporal_allocate(1 + (mix(static_cast<u32>(i)) & 511u));
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 255u;
    alloc.deallocate(p.ptr);
  }
  const u64 end = tsc();
  return { end - begin, CHURN_OPS * 2, sum };
}

template<bool Reverse>
[[gnu::noinline]] result
tlsf_batch() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  constexpr u64 rounds = 512;
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 r = 0; r < rounds; ++r ) {
    for ( usize i = 0; i < BATCH_LIVE; ++i ) {
      g_live[i] = alloc.allocate(1 + (mix(static_cast<u32>(i + r * BATCH_LIVE)) & 511u));
      escape(g_live[i].ptr);
      sum += reinterpret_cast<uintptr_t>(g_live[i].ptr) & 255u;
    }
    for ( usize i = 0; i < BATCH_LIVE; ++i ) {
      const usize pos = Reverse ? BATCH_LIVE - i - 1 : i;
      alloc.deallocate(g_live[pos].ptr);
    }
  }
  const u64 end = tsc();
  return { end - begin, rounds * BATCH_LIVE * 2, sum };
}

[[gnu::noinline]] result
tlsf_fragment() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  constexpr usize live = 32'768;
  constexpr u64 rounds = CHURN_OPS / live;
  u64 sum = 0;
  for ( usize i = 0; i < live; ++i ) g_live[i] = alloc.allocate(17 + (mix(static_cast<u32>(i)) & 1007u));

  const u64 begin = tsc();
  for ( u64 r = 0; r < rounds; ++r ) {
    const usize parity = static_cast<usize>(r & 1u);
    for ( usize i = parity; i < live; i += 2 ) alloc.deallocate(g_live[i].ptr);
    for ( usize i = parity; i < live; i += 2 ) {
      g_live[i] = alloc.allocate(17 + (mix(static_cast<u32>(i + r * live)) & 1007u));
      escape(g_live[i].ptr);
      sum += reinterpret_cast<uintptr_t>(g_live[i].ptr) & 255u;
    }
  }
  const u64 end = tsc();

  for ( usize i = 0; i < live; ++i ) alloc.deallocate(g_live[i].ptr);
  return { end - begin, rounds * live, sum };
}

template<usize Live>
[[gnu::noinline]] result
tlsf_working_set() noexcept
{
  tlsf_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  for ( usize i = 0; i < Live; ++i ) {
    g_live[i] = alloc.allocate(64);
    g_live[i].ptr[0] = static_cast<byte>(i);
  }

  u32 state = 0xA5A55A5Au;
  const u64 iterations = CHURN_OPS / 2;
  const u64 begin = tsc();
  for ( u64 i = 0; i < iterations; ++i ) {
    state = mix(state + static_cast<u32>(i));
    const usize pos = static_cast<usize>(state) & (Live - 1);
    sum += g_live[pos].ptr[0];
    alloc.deallocate(g_live[pos].ptr);
    g_live[pos] = alloc.allocate(64);
    g_live[pos].ptr[0] = static_cast<byte>(state);
  }
  const u64 end = tsc();

  for ( usize i = 0; i < Live; ++i ) alloc.deallocate(g_live[i].ptr);
  return { end - begin, iterations * 2, sum };
}

template<usize Streams>
[[gnu::noinline]] result
tlsf_pipeline() noexcept
{
  tlsf_t a0({ g_pool[0], POOL_BYTES });
  tlsf_t a1({ g_pool[1], POOL_BYTES });
  tlsf_t a2({ g_pool[2], POOL_BYTES });
  tlsf_t a3({ g_pool[3], POOL_BYTES });
  u64 sum = 0;
  constexpr u64 iterations = ROUND_TRIPS / Streams;
  const u64 begin = tsc();
  for ( u64 i = 0; i < iterations; ++i ) {
    const usize n = 1 + (mix(static_cast<u32>(i)) & 511u);
    micron::__chunk<byte> p0 = a0.allocate(n);
    micron::__chunk<byte> p1;
    micron::__chunk<byte> p2;
    micron::__chunk<byte> p3;
    if constexpr ( Streams > 1 ) p1 = a1.allocate(n + 1);
    if constexpr ( Streams > 2 ) p2 = a2.allocate(n + 2);
    if constexpr ( Streams > 3 ) p3 = a3.allocate(n + 3);
    escape(p0.ptr);
    if constexpr ( Streams > 1 ) escape(p1.ptr);
    if constexpr ( Streams > 2 ) escape(p2.ptr);
    if constexpr ( Streams > 3 ) escape(p3.ptr);
    sum += reinterpret_cast<uintptr_t>(p0.ptr);
    a0.deallocate(p0.ptr);
    if constexpr ( Streams > 1 ) {
      sum += reinterpret_cast<uintptr_t>(p1.ptr);
      a1.deallocate(p1.ptr);
    }
    if constexpr ( Streams > 2 ) {
      sum += reinterpret_cast<uintptr_t>(p2.ptr);
      a2.deallocate(p2.ptr);
    }
    if constexpr ( Streams > 3 ) {
      sum += reinterpret_cast<uintptr_t>(p3.ptr);
      a3.deallocate(p3.ptr);
    }
  }
  const u64 end = tsc();
  return { end - begin, iterations * Streams * 2, sum };
}

[[gnu::noinline]] result
buddy_roundtrip() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < ROUND_TRIPS; ++i ) {
    micron::__chunk<byte> p = alloc.allocate(513 + (mix(static_cast<u32>(i)) & 16'383u));
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 4095u;
    alloc.deallocate(p.ptr);
  }
  const u64 end = tsc();
  return { end - begin, ROUND_TRIPS * 2, sum };
}

[[gnu::noinline]] result
buddy_temporal_hit() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  micron::__chunk<byte> first = alloc.temporal_allocate(1024);
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < ROUND_TRIPS; ++i ) {
    micron::__chunk<byte> p = alloc.temporal_allocate(1024);
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 4095u;
  }
  const u64 end = tsc();
  alloc.deallocate(first.ptr);
  return { end - begin, ROUND_TRIPS, sum };
}

[[gnu::noinline]] result
buddy_temporal_churn() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 i = 0; i < CHURN_OPS; ++i ) {
    micron::__chunk<byte> p = alloc.temporal_allocate(513 + (mix(static_cast<u32>(i)) & 16'383u));
    escape(p.ptr);
    sum += reinterpret_cast<uintptr_t>(p.ptr) & 4095u;
    alloc.deallocate(p.ptr);
  }
  const u64 end = tsc();
  return { end - begin, CHURN_OPS * 2, sum };
}

template<bool Reverse>
[[gnu::noinline]] result
buddy_batch() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  constexpr u64 rounds = 512;
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 r = 0; r < rounds; ++r ) {
    for ( usize i = 0; i < BATCH_LIVE; ++i ) {
      g_live[i] = alloc.allocate(513 + (mix(static_cast<u32>(i + r * BATCH_LIVE)) & 2047u));
      escape(g_live[i].ptr);
      sum += reinterpret_cast<uintptr_t>(g_live[i].ptr) & 4095u;
    }
    for ( usize i = 0; i < BATCH_LIVE; ++i ) {
      const usize pos = Reverse ? BATCH_LIVE - i - 1 : i;
      alloc.deallocate(g_live[pos].ptr);
    }
  }
  const u64 end = tsc();
  return { end - begin, rounds * BATCH_LIVE * 2, sum };
}

[[gnu::noinline]] result
buddy_adversarial_coalesce() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  constexpr usize live = 4096;
  constexpr u64 rounds = 64;
  u64 sum = 0;
  const u64 begin = tsc();
  for ( u64 r = 0; r < rounds; ++r ) {
    for ( usize i = 0; i < live; ++i ) {
      g_live[i] = alloc.allocate(1024);
      escape(g_live[i].ptr);
      sum += reinterpret_cast<uintptr_t>(g_live[i].ptr) & 4095u;
    }
    for ( usize i = 0; i < live; i += 2 ) alloc.deallocate(g_live[i].ptr);
    for ( usize i = 1; i < live; i += 2 ) alloc.deallocate(g_live[i].ptr);
  }
  const u64 end = tsc();
  return { end - begin, rounds * live * 2, sum };
}

template<usize Live>
[[gnu::noinline]] result
buddy_working_set() noexcept
{
  buddy_t alloc({ g_pool[0], POOL_BYTES });
  u64 sum = 0;
  for ( usize i = 0; i < Live; ++i ) {
    g_live[i] = alloc.allocate(1024);
    g_live[i].ptr[0] = static_cast<byte>(i);
  }

  u32 state = 0xC001D00Du;
  const u64 iterations = CHURN_OPS / 2;
  const u64 begin = tsc();
  for ( u64 i = 0; i < iterations; ++i ) {
    state = mix(state + static_cast<u32>(i));
    const usize pos = static_cast<usize>(state) & (Live - 1);
    sum += g_live[pos].ptr[0];
    alloc.deallocate(g_live[pos].ptr);
    g_live[pos] = alloc.allocate(1024);
    g_live[pos].ptr[0] = static_cast<byte>(state);
  }
  const u64 end = tsc();

  for ( usize i = 0; i < Live; ++i ) alloc.deallocate(g_live[i].ptr);
  return { end - begin, iterations * 2, sum };
}

template<usize Streams>
[[gnu::noinline]] result
buddy_pipeline() noexcept
{
  buddy_t a0({ g_pool[0], POOL_BYTES });
  buddy_t a1({ g_pool[1], POOL_BYTES });
  buddy_t a2({ g_pool[2], POOL_BYTES });
  buddy_t a3({ g_pool[3], POOL_BYTES });
  u64 sum = 0;
  constexpr u64 iterations = ROUND_TRIPS / Streams;
  const u64 begin = tsc();
  for ( u64 i = 0; i < iterations; ++i ) {
    const usize n = 513 + (mix(static_cast<u32>(i)) & 16'383u);
    micron::__chunk<byte> p0 = a0.allocate(n);
    micron::__chunk<byte> p1;
    micron::__chunk<byte> p2;
    micron::__chunk<byte> p3;
    if constexpr ( Streams > 1 ) p1 = a1.allocate(n + 1);
    if constexpr ( Streams > 2 ) p2 = a2.allocate(n + 2);
    if constexpr ( Streams > 3 ) p3 = a3.allocate(n + 3);
    escape(p0.ptr);
    if constexpr ( Streams > 1 ) escape(p1.ptr);
    if constexpr ( Streams > 2 ) escape(p2.ptr);
    if constexpr ( Streams > 3 ) escape(p3.ptr);
    sum += reinterpret_cast<uintptr_t>(p0.ptr);
    a0.deallocate(p0.ptr);
    if constexpr ( Streams > 1 ) {
      sum += reinterpret_cast<uintptr_t>(p1.ptr);
      a1.deallocate(p1.ptr);
    }
    if constexpr ( Streams > 2 ) {
      sum += reinterpret_cast<uintptr_t>(p2.ptr);
      a2.deallocate(p2.ptr);
    }
    if constexpr ( Streams > 3 ) {
      sum += reinterpret_cast<uintptr_t>(p3.ptr);
      a3.deallocate(p3.ptr);
    }
  }
  const u64 end = tsc();
  return { end - begin, iterations * Streams * 2, sum };
}

void
usage() noexcept
{
  micron::io::println("usage: abcmalloc_core_bench <kernel>");
  micron::io::println("TLSF: tlsf_roundtrip tlsf_temporal_hit tlsf_temporal_churn");
  micron::io::println("      tlsf_batch_lifo tlsf_batch_fifo tlsf_fragment");
  micron::io::println("      tlsf_ws_l1 tlsf_ws_l2 tlsf_ws_llc tlsf_pipe1 tlsf_pipe4");
  micron::io::println("buddy: buddy_roundtrip buddy_temporal_hit buddy_temporal_churn");
  micron::io::println("       buddy_batch_lifo buddy_batch_fifo buddy_coalesce");
  micron::io::println("       buddy_ws_l1 buddy_ws_l2 buddy_ws_llc buddy_pipe1 buddy_pipe4");
}

};      // namespace

int
main(int argc, char **argv)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  if ( argc != 2 ) {
    usage();
    return 2;
  }

  result r{};
  const char *name = argv[1];
  if ( same(name, "tlsf_roundtrip") )
    r = tlsf_roundtrip();
  else if ( same(name, "tlsf_temporal_hit") )
    r = tlsf_temporal_hit();
  else if ( same(name, "tlsf_temporal_churn") )
    r = tlsf_temporal_churn();
  else if ( same(name, "tlsf_batch_lifo") )
    r = tlsf_batch<true>();
  else if ( same(name, "tlsf_batch_fifo") )
    r = tlsf_batch<false>();
  else if ( same(name, "tlsf_fragment") )
    r = tlsf_fragment();
  else if ( same(name, "tlsf_ws_l1") )
    r = tlsf_working_set<64>();
  else if ( same(name, "tlsf_ws_l2") )
    r = tlsf_working_set<4096>();
  else if ( same(name, "tlsf_ws_llc") )
    r = tlsf_working_set<65'536>();
  else if ( same(name, "tlsf_pipe1") )
    r = tlsf_pipeline<1>();
  else if ( same(name, "tlsf_pipe4") )
    r = tlsf_pipeline<4>();
  else if ( same(name, "buddy_roundtrip") )
    r = buddy_roundtrip();
  else if ( same(name, "buddy_temporal_hit") )
    r = buddy_temporal_hit();
  else if ( same(name, "buddy_temporal_churn") )
    r = buddy_temporal_churn();
  else if ( same(name, "buddy_batch_lifo") )
    r = buddy_batch<true>();
  else if ( same(name, "buddy_batch_fifo") )
    r = buddy_batch<false>();
  else if ( same(name, "buddy_coalesce") )
    r = buddy_adversarial_coalesce();
  else if ( same(name, "buddy_ws_l1") )
    r = buddy_working_set<8>();
  else if ( same(name, "buddy_ws_l2") )
    r = buddy_working_set<128>();
  else if ( same(name, "buddy_ws_llc") )
    r = buddy_working_set<4096>();
  else if ( same(name, "buddy_pipe1") )
    r = buddy_pipeline<1>();
  else if ( same(name, "buddy_pipe4") )
    r = buddy_pipeline<4>();
  else {
    usage();
    return 2;
  }

  const u64 cycles_x100 = r.ops ? (r.cycles * 100u) / r.ops : 0;
  micron::io::println(name, " ops=", r.ops, " cycles=", r.cycles, " cyc/op=", cycles_x100 / 100u, ".", cycles_x100 % 100u,
                      " checksum=", r.checksum);
  return 0;
}
