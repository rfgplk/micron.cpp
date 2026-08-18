//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Bench for micron::function<R(Args...)> -- construct, copy, move, invoke, across the three storage
// regimes the type actually has:
//
//   sbo         sizeof <= 48 and alignof <= 16   -> lives in __buf, zero allocator traffic
//   heap        sizeof  > 48                     -> ::operator new(sz, align_val_t) -> abc::alloc
//   overaligned alignof > 32                     -> abc::aligned_alloc's SHIFTED-pointer scheme, and
//                                                   it must come back through abc::aligned_free
//
// Why this exists: the heap regime's allocation shape changed on 2026-08-18. It used to allocate with
// the aligned operator new and free with the plain one (silent corruption above alignof 32), and the
// copy ctor used to allocate a flat 192 bytes regardless of the target (a heap overflow above 192).
// Both now go through __alloc_for/__free_for, which pass sizeof/alignof from the vtable. The rows to
// watch after any further edit are copy-heap and construct-overaligned -- everything else is
// allocator-free and should not move at all.
//
// The SBO rows are the ones that matter for the fp layer: src/algorithm/fp*.hpp all take
// micron::function BY VALUE, so a copy sits on every one of those call paths.
//
// Build with:  duck build benches/function_bench.cpp --perf --fp --no-ssp --no-lto -o bin/b -f
// and pin it:  taskset -c 2 bin/b/function_bench
//
// -f ON EVERY BUILD. duck's cache keys off the .cpp mtime and this measures a HEADER, so an A/B
// without -f silently re-runs the previous arm's binary.

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"

#include "../src/function.hpp"

namespace mc = micron;

namespace
{

constexpr u32 K_MEASUREMENTS = 7;
constexpr u32 WARMUP_REPS = 2;
constexpr u64 REPS_ALLOC = 200000;      // construct / copy: allocator-bound
constexpr u64 REPS_CALL = 2000000;      // invoke: dispatch-bound

volatile int sink_i = 0;

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

// MINIMUM, not median: noise only ever adds time, so on a box that is not guaranteed quiet the
// minimum is the closest available reading of the machine's real speed.
f64
min_f64(const f64 *xs, u32 n) noexcept
{
  f64 best = xs[0];
  for ( u32 i = 1; i < n; ++i )
    if ( xs[i] < best ) best = xs[i];
  return best;
}

void
row(const char *op, u64 n, f64 ns_per_op)
{
  micron::io::print("  ");
  micron::io::print(op);
  for ( usize i = micron::strlen(op); i < 30; ++i ) micron::io::print(" ");
  micron::io::print(static_cast<u64>(n));
  micron::io::print("  ");
  micron::io::print(static_cast<u64>(ns_per_op));
  micron::io::print(".");
  const u64 frac = static_cast<u64>((ns_per_op - static_cast<f64>(static_cast<u64>(ns_per_op))) * 100.0);
  if ( frac < 10 ) micron::io::print("0");
  micron::io::print(frac);
  micron::io::println(" ns/op");
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// targets, one per storage regime. all-unsigned-char payloads so sizeof is exactly N and there is no
// padding to reason about; operator() touches only the first byte so the row measures the wrapper,
// not the payload.

template<usize N> struct sbo_like {
  unsigned char pad[N];

  explicit sbo_like(int b) noexcept
  {
    for ( usize i = 0; i < N; ++i ) pad[i] = static_cast<unsigned char>((b + static_cast<int>(i)) & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

template<usize A> struct aligned_like {
  alignas(A) unsigned char pad[A];

  explicit aligned_like(int b) noexcept
  {
    for ( usize i = 0; i < A; ++i ) pad[i] = static_cast<unsigned char>((b + static_cast<int>(i)) & 0xff);
  }

  int
  operator()(int x) const noexcept
  {
    return x + static_cast<int>(pad[0]);
  }
};

static_assert(sizeof(sbo_like<16>) <= mc::__function_smallobj_size);
static_assert(sizeof(sbo_like<256>) > mc::__function_smallobj_size * 4);      // past the old constant
static_assert(alignof(aligned_like<64>) > 32);                                // shifted-pointer route

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename G>
void
bench_construct(const char *name)
{
  f64 m[K_MEASUREMENTS];
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_ALLOC; ++i ) {
      mc::function<int(int)> f{ G(static_cast<int>(i)) };
      sink_i += f(1);
    }
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_ALLOC);
  }
  row(name, REPS_ALLOC, min_f64(m, K_MEASUREMENTS));
}

template<typename G>
void
bench_copy(const char *name)
{
  mc::function<int(int)> src{ G(7) };
  f64 m[K_MEASUREMENTS];
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_ALLOC; ++i ) {
      mc::function<int(int)> c(src);
      sink_i += c(1);
    }
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_ALLOC);
  }
  row(name, REPS_ALLOC, min_f64(m, K_MEASUREMENTS));
}

template<typename G>
void
bench_move(const char *name)
{
  f64 m[K_MEASUREMENTS];
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_ALLOC; ++i ) {
      mc::function<int(int)> a{ G(static_cast<int>(i)) };
      mc::function<int(int)> b(mc::move(a));      // heap: pointer steal. sbo: a real G move.
      sink_i += b(1);
    }
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_ALLOC);
  }
  row(name, REPS_ALLOC, min_f64(m, K_MEASUREMENTS));
}

template<typename G>
void
bench_invoke(const char *name)
{
  mc::function<int(int)> f{ G(3) };
  f64 m[K_MEASUREMENTS];
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_CALL; ++i ) sink_i += f(static_cast<int>(i & 0xff));
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_CALL);
  }
  row(name, REPS_CALL, min_f64(m, K_MEASUREMENTS));
}

}      // namespace

int
main()
{
  micron::io::println("=== micron::function BENCH ===");
  micron::io::print("  sizeof(function<int(int)>)=");
  micron::io::print(static_cast<u64>(sizeof(mc::function<int(int)>)));
  micron::io::print("  SBO=");
  micron::io::print(static_cast<u64>(mc::__function_smallobj_size));
  micron::io::print("B/align ");
  micron::io::println(static_cast<u64>(mc::__function_smallobj_align));
  micron::io::println("");
  micron::io::println("  op                            reps      cost");

  // ---- construct: emplace + destroy. sbo is allocator-free; the rest is one alloc/free pair. ----
  bench_construct<sbo_like<16>>("construct sbo/16B");
  bench_construct<sbo_like<48>>("construct sbo/48B");
  bench_construct<sbo_like<64>>("construct heap/64B");
  bench_construct<sbo_like<256>>("construct heap/256B");
  bench_construct<sbo_like<1024>>("construct heap/1024B");
  bench_construct<aligned_like<16>>("construct sbo/align16");
  bench_construct<aligned_like<64>>("construct aligned/64");
  bench_construct<aligned_like<128>>("construct aligned/128");
  micron::io::println("");

  // ---- copy: this is the path every src/algorithm/fp*.hpp entry point sits on ----
  bench_copy<sbo_like<16>>("copy sbo/16B");
  bench_copy<sbo_like<48>>("copy sbo/48B");
  bench_copy<sbo_like<64>>("copy heap/64B");
  bench_copy<sbo_like<256>>("copy heap/256B");
  bench_copy<sbo_like<1024>>("copy heap/1024B");
  bench_copy<aligned_like<64>>("copy aligned/64");
  bench_copy<aligned_like<128>>("copy aligned/128");
  micron::io::println("");

  // ---- move: heap steals the pointer, sbo runs G's move ctor ----
  bench_move<sbo_like<16>>("move sbo/16B");
  bench_move<sbo_like<256>>("move heap/256B");
  bench_move<aligned_like<64>>("move aligned/64");
  micron::io::println("");

  // ---- invoke: pure dispatch, no allocator involvement in any regime ----
  bench_invoke<sbo_like<16>>("invoke sbo/16B");
  bench_invoke<sbo_like<256>>("invoke heap/256B");
  bench_invoke<aligned_like<64>>("invoke aligned/64");

  micron::io::println("");
  micron::io::print("sink=");
  micron::io::println(static_cast<u64>(sink_i));
  return 1;
}
