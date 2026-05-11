//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Benchmark harness for the freestanding micron memory routines:
//   mc::memcpy, mc::memset, mc::memcmp, mc::memmove (scalar cmemory entry points)
//   simd::memcpy{128,256,512}, amemcpy*, ntmemcpy*
//   simd::memset{128,256,512}, amemset*, ntmemset*
//   simd::memcmp{128,256,512}, amemcmp*
//   simd::memmove{128,256,512}, amemmove*

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"

// IMPORTANT: at -Ofast, GCC's tree-loop-distribute-patterns optimization
// will silently turn `mc::memcpy` / `mc::memmove`'s scalar 4-stride byte
// loop into a libc memcpy@plt / memmove@plt call — defeating the purpose
// of benchmarking the freestanding micron path. Disable that pass for the
// whole TU. (The same flag is applied to the freestanding C-ABI fallback
// in src/memory/cmemory/memcpy.hpp:599-609.)
#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

namespace
{

constexpr u64 BUF_BYTES = 16ULL << 20;
alignas(64) static u8 g_src[BUF_BYTES];
alignas(64) static u8 g_dst[BUF_BYTES];

constexpr u64 TARGET_BYTES_PER_MEAS = 1ULL << 28;
constexpr u64 MIN_REPS = 16;
constexpr u64 WARMUP_REPS = 8;
constexpr u32 K_MEASUREMENTS = 7;

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

struct row {
  const char *name;
  u64 size;
  u64 bytes_per_op;
  f64 cyc_per_byte;
  f64 ipc;
  u64 latency_cyc;
  f64 bmiss_rate;
};

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v)
{
  if ( v < 0 ) v = 0;
  u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) {}

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else {
      u64 vv = v;
      while ( vv ) {
        tmp[n++] = '0' + (vv % 10);
        vv /= 10;
      }
    }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = '0' + (w % 10);
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 / 10);
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 % 10);
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  void
  s_lj_at(const char *p, u32 end_col) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
    while ( pos < end_col ) buf[pos++] = ' ';
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

[[gnu::cold]] void
print_header()
{
  line h;
  h.s_at("size(B)", 10);
  h.pad_to(12, 0);
  h.s_lj_at("routine", 32);
  h.s_at("bytes/op", 42);
  h.s_at("cyc/byte", 52);
  h.s_at("IPC", 62);
  h.s_at("lat(cyc)", 72);
  h.s_at("bmiss%", 82);
  micron::io::println(h.str());
  micron::io::println("----------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_row(const row &r)
{
  fmt2 cpb = to_fmt2(r.cyc_per_byte);
  fmt2 ipc = to_fmt2(r.ipc);
  fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.u_at(r.size, 10);
  ln.pad_to(12, 0);
  ln.s_lj_at(r.name, 32);
  ln.u_at(r.bytes_per_op, 42);
  ln.f2_at(cpb, 52);
  ln.f2_at(ipc, 62);
  ln.u_at(r.latency_cyc, 72);
  ln.f2_at(bm, 82);
  micron::io::println(ln.str());
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
clobber_val(u64 v) noexcept
{
  asm volatile("" : : "r"(v));
}

u64
median_u64(u64 *xs, u32 n) noexcept
{

  for ( u32 i = 1; i < n; i++ ) {
    u64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; i++ ) {
    f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

struct sample {
  u64 cyc;
  u64 inst;
  u64 br;
  u64 bm;
};

template <typename Fn>
[[gnu::noinline]] sample
measure_once(Fn &&fn, u64 reps) noexcept
{
  mem_events evs{ bbench::quiet{} };
  evs.open();
  evs.begin();
  for ( u64 i = 0; i < reps; i++ ) fn();
  evs.end();
  return { static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve()),
           static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve()), static_cast<u64>(evs.get<bbench::branches>().retrieve()),
           static_cast<u64>(evs.get<bbench::branch_misses>().retrieve()) };
}

template <typename Fn>
row
bench_routine(const char *name, u64 size, u64 bytes_per_op, Fn &&fn) noexcept
{

  for ( u64 i = 0; i < WARMUP_REPS; i++ ) fn();

  u64 reps = TARGET_BYTES_PER_MEAS / (bytes_per_op == 0 ? 1 : bytes_per_op);
  if ( reps < MIN_REPS ) reps = MIN_REPS;

  f64 cpb_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  u64 lat_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; m++ ) {
    sample s = measure_once(fn, reps);
    f64 total_bytes = static_cast<f64>(reps) * static_cast<f64>(bytes_per_op);
    cpb_samples[m] = static_cast<f64>(s.cyc) / total_bytes;
    ipc_samples[m] = s.cyc > 0 ? static_cast<f64>(s.inst) / static_cast<f64>(s.cyc) : 0.0;
    lat_samples[m] = s.cyc / reps;
    bm_samples[m] = s.br > 0 ? static_cast<f64>(s.bm) / static_cast<f64>(s.br) : 0.0;
  }

  return row{
    name,
    size,
    bytes_per_op,
    median_f64(cpb_samples, K_MEASUREMENTS),
    median_f64(ipc_samples, K_MEASUREMENTS),
    median_u64(lat_samples, K_MEASUREMENTS),
    median_f64(bm_samples, K_MEASUREMENTS),
  };
}

};     // namespace

static constexpr u64 SIZES[] = {
  16, 64, 256, 1ULL << 10, 4ULL << 10, 64ULL << 10, 1ULL << 20, 16ULL << 20,
};

void
sweep_memcpy()
{
  micron::io::println("");
  micron::io::println("[memcpy] dst=src+1MiB, hot cache, dst aligned 64, src aligned 64");
  print_header();

  for ( u64 sz : SIZES ) {
    if ( sz > BUF_BYTES / 2 ) continue;

    {
      auto fn = [sz]() {
        mc::memcpy(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("mc::memcpy        ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memcpy128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memcpy256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memcpy512(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memcpy512   ", sz, sz, fn));
    }
#endif
#endif
    {
      auto fn = [sz]() {
        mc::amemcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::amemcpy128  ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::amemcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::amemcpy256  ", sz, sz, fn));
    }
#endif
    if ( sz >= 1024 ) {

      auto fn = [sz]() {
        mc::ntmemcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::ntmemcpy128 ", sz, sz, fn));

#if defined(__micron_arch_x86_any)
      auto fn256 = [sz]() {
        mc::ntmemcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::ntmemcpy256 ", sz, sz, fn256));
#endif
    }
  }
}

void
sweep_memset()
{
  micron::io::println("");
  micron::io::println("[memset] dst aligned 64, hot cache");
  print_header();

  for ( u64 sz : SIZES ) {
    {
      auto fn = [sz]() {
        mc::memset(g_dst, static_cast<byte>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("mc::memset        ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memset128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memset256(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memset256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memset512(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memset512   ", sz, sz, fn));
    }
#endif
#endif
    {
      auto fn = [sz]() {
        mc::amemset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::amemset128  ", sz, sz, fn));
    }
    if ( sz >= 1024 ) {
      auto fn = [sz]() {
        mc::ntmemset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::ntmemset128 ", sz, sz, fn));
    }
  }
}

void
sweep_memcmp()
{
  micron::io::println("");
  micron::io::println("[memcmp] equal buffers (worst case for early-exit), hot cache");
  print_header();

  mc::memcpy(g_dst, g_src, BUF_BYTES);

  static volatile i64 sink = 0;

  for ( u64 sz : SIZES ) {
    {
      auto fn = [sz]() { sink = static_cast<i64>(mc::bytecmp(g_src, g_dst, sz)); };
      print_row(bench_routine("mc::bytecmp       ", sz, sz, fn));
    }
    {
      auto fn = [sz]() { sink = mc::memcmp128(g_src, g_dst, sz); };
      print_row(bench_routine("simd::memcmp128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() { sink = mc::memcmp256(g_src, g_dst, sz); };
      print_row(bench_routine("simd::memcmp256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() { sink = mc::memcmp512(g_src, g_dst, sz); };
      print_row(bench_routine("simd::memcmp512   ", sz, sz, fn));
    }
#endif
#endif
    clobber_val(static_cast<u64>(sink));
  }
}

void
sweep_memmove()
{
  micron::io::println("");
  micron::io::println("[memmove] non-overlapping (forward path), hot cache");
  print_header();

  for ( u64 sz : SIZES ) {
    if ( sz > BUF_BYTES / 2 ) continue;

    {
      auto fn = [sz]() {
        mc::memmove(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("mc::memmove       ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memmove128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memmove128  ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memmove256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memmove256  ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memmove512(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine("simd::memmove512  ", sz, sz, fn));
    }
#endif
#endif
  }

  micron::io::println("");
  micron::io::println("[memmove] overlapping (dst = src + 64, backward path)");
  print_header();

  for ( u64 sz : { (u64)1024, (u64)4096, (u64)65536, (u64)1ULL << 20 } ) {
    auto fn = [sz]() {
      mc::memmove(g_src + 64, g_src, sz);
      clobber(g_src);
    };
    print_row(bench_routine("mc::memmove[ovlp] ", sz, sz, fn));

    auto fn128 = [sz]() {
      mc::memmove128(g_src + 64, g_src, sz);
      clobber(g_src);
    };
    print_row(bench_routine("simd::memmove128[ovlp]", sz, sz, fn128));

#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn256 = [sz]() {
        mc::memmove256(g_src + 64, g_src, sz);
        clobber(g_src);
      };
      print_row(bench_routine("simd::memmove256[ovlp]", sz, sz, fn256));
    }
#endif
  }
}

int
main(void)
{

  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  for ( u64 i = 0; i < BUF_BYTES; i++ ) {
    g_src[i] = static_cast<u8>(i * 0x9Eu + 0x37u);
    g_dst[i] = static_cast<u8>(i * 0x6Bu + 0x9Du);
  }

  micron::io::println("=== micron memory benchmark ===");
  micron::io::println("buffers: 16 MiB src + 16 MiB dst, 64-byte aligned");
  micron::io::println("warmup ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell; median reported");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (bbench 4-event group)");

  sweep_memcpy();
  sweep_memset();
  sweep_memcmp();
  sweep_memmove();

  micron::io::println("");
  micron::io::println("=== done ===");

  return 0;
}

#pragma GCC pop_options
