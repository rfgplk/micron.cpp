//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Detailed memset/memcpy/memmove benchmark: dense small sizes, pow2 span,
// alignment matrix, ERMS window, NT crossover discovery, cold/streaming,
// and memmove overlap — micron vs glibc (PLT) vs raw simd:: variants.
//
// usage: memory_detail_bench [--csv] [--full] [--cpu N] [--hot-only] [--cold-only]
//   (duck run does not forward args; build then run the binary directly)

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/memory/memory.hpp"
#include "../src/memory/mman.hpp"
#include "../src/std.hpp"

// IMPORTANT: at -Ofast, GCC's tree-loop-distribute-patterns optimization
// rewrites micron's scalar byte loops into libc memcpy@plt/memset@plt calls,
// which would silently benchmark glibc against itself. Disable for the TU.
#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

extern "C" {
void *memcpy(void *, const void *, __SIZE_TYPE__) noexcept;
void *memmove(void *, const void *, __SIZE_TYPE__) noexcept;
void *memset(void *, int, __SIZE_TYPE__) noexcept;
}

namespace
{

constexpr u32 K_MEASUREMENTS = 5;

static bool g_csv = false;
static bool g_full = false;
static bool g_hot_only = false;
static bool g_cold_only = false;

static u8 *g_dring = nullptr;
static u8 *g_sring = nullptr;
static u64 g_ring = 1ull << 27;
constexpr u64 MAXN = 1ull << 26;
constexpr u64 RING_SLACK = MAXN + 8192;

static volatile u64 g_vsink = 0;

[[gnu::always_inline]] inline u64
cycles() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  u32 lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
#elif defined(__micron_arch_arm64)
  u64 v;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#else
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
#endif
}

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

struct fmtd {
  u64 whole;
  u32 frac;
};

[[gnu::always_inline]] inline fmtd
to_fmt(f64 v, u32 scale) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * static_cast<f64>(scale) + 0.5);
  return { s / scale, static_cast<u32>(s % scale) };
}

struct line {
  char buf[320];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  u(u64 v) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else
      while ( v ) {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
      }
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  i(i64 v) noexcept
  {
    if ( v < 0 ) {
      buf[pos++] = '-';
      u(static_cast<u64>(-v));
    } else
      u(static_cast<u64>(v));
  }

  void
  f(fmtd fm, u32 digits) noexcept
  {
    u(fm.whole);
    buf[pos++] = '.';
    u32 div = 1;
    for ( u32 k = 1; k < digits; ++k ) div *= 10;
    for ( u32 k = 0; k < digits; ++k ) {
      buf[pos++] = '0' + static_cast<char>((fm.frac / div) % 10);
      div /= 10;
    }
  }

  void
  lj(const char *p, u32 end_col) noexcept
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
emit_row(const char *op, const char *impl, u64 n, u64 d_al, u64 s_al, const char *mode, i64 delta, f64 cyc_pb, f64 gibs)
{
  line ln;
  if ( g_csv ) {
    ln.s(op);
    ln.s(",");
    ln.s(impl);
    ln.s(",");
    ln.u(n);
    ln.s(",");
    ln.u(d_al);
    ln.s(",");
    ln.u(s_al);
    ln.s(",");
    ln.s(mode);
    ln.s(",");
    ln.i(delta);
    ln.s(",");
    ln.f(to_fmt(cyc_pb, 10000), 4);
    ln.s(",");
    ln.f(to_fmt(gibs, 100), 2);
  } else {
    ln.lj(op, 10);
    ln.lj(impl, 26);
    ln.u(n);
    ln.lj("", ln.pos < 36 ? 36 : ln.pos + 1);
    ln.u(d_al);
    ln.lj("", ln.pos < 41 ? 41 : ln.pos + 1);
    ln.u(s_al);
    ln.lj("", ln.pos < 46 ? 46 : ln.pos + 1);
    ln.lj(mode, 52);
    ln.i(delta);
    ln.lj("", ln.pos < 60 ? 60 : ln.pos + 1);
    ln.f(to_fmt(cyc_pb, 10000), 4);
    ln.lj("", ln.pos < 72 ? 72 : ln.pos + 1);
    ln.f(to_fmt(gibs, 100), 2);
  }
  micron::io::println(ln.str());
}

[[gnu::cold]] void
section(const char *name)
{
  if ( g_csv ) return;
  micron::io::println("");
  micron::io::println("[", name, "]");
  line h;
  h.lj("op", 10);
  h.lj("impl", 26);
  h.lj("size", 36);
  h.lj("d_al", 41);
  h.lj("s_al", 46);
  h.lj("mode", 52);
  h.lj("delta", 60);
  h.lj("cyc/B", 72);
  h.lj("GiB/s", 80);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------");
}

typedef void (*cfn)(u8 *d, const u8 *s, u64 n);
typedef void (*sfn)(u8 *d, u64 n);

static void
c_noop(u8 *, const u8 *, u64)
{
}

static void
c_mc(u8 *d, const u8 *s, u64 n)
{
  mc::memcpy(d, s, n);
}

static void
c_mc_move(u8 *d, const u8 *s, u64 n)
{
  mc::memmove(d, const_cast<u8 *>(s), n);
}

static void
c_libc(u8 *d, const u8 *s, u64 n)
{
  ::memcpy(d, s, n);
}

static void
c_libc_move(u8 *d, const u8 *s, u64 n)
{
  ::memmove(d, s, n);
}

static void
c_raw32(u8 *d, const u8 *s, u64 n)
{
  micron::__memcpy_32(d, s, n);
}

static void
c_simd128(u8 *d, const u8 *s, u64 n)
{
  mc::memcpy128<u8>(d, s, n);
}

static void
c_simd_a128(u8 *d, const u8 *s, u64 n)
{
  mc::amemcpy128<u8>(d, s, n);
}

static void
c_simd_nt128(u8 *d, const u8 *s, u64 n)
{
  mc::ntmemcpy128<u8>(d, s, n);
}

#if defined(__micron_x86_avx2)
static void
c_simd256(u8 *d, const u8 *s, u64 n)
{
  mc::memcpy256<u8>(d, s, n);
}

static void
c_simd_a256(u8 *d, const u8 *s, u64 n)
{
  mc::amemcpy256<u8>(d, s, n);
}

static void
c_simd_nt256(u8 *d, const u8 *s, u64 n)
{
  mc::ntmemcpy256<u8>(d, s, n);
}
#endif
#if defined(__micron_x86_avx512f)
static void
c_simd512(u8 *d, const u8 *s, u64 n)
{
  mc::memcpy512<u8>(d, s, n);
}
#endif

template<u64 N>
static void
c_builtin(u8 *d, const u8 *s, u64)
{
  __builtin_memcpy(d, s, N);
}

static void
s_noop(u8 *, u64)
{
}

static void
s_mc0(u8 *d, u64 n)
{
  mc::memset(d, static_cast<byte>(0), n);
}

static void
s_mc5A(u8 *d, u64 n)
{
  mc::memset(d, static_cast<byte>(0x5A), n);
}

static void
s_mcFF(u8 *d, u64 n)
{
  mc::memset(d, static_cast<byte>(0xFF), n);
}

static void
s_libc0(u8 *d, u64 n)
{
  ::memset(d, 0, n);
}

static void
s_libc5A(u8 *d, u64 n)
{
  ::memset(d, 0x5A, n);
}

static void
s_simd128_0(u8 *d, u64 n)
{
  mc::memset128(d, static_cast<u8>(0), n);
}

static void
s_simd_nt128_0(u8 *d, u64 n)
{
  mc::ntmemset128(d, static_cast<u8>(0), n);
}

#if defined(__micron_x86_avx2)
static void
s_simd256_0(u8 *d, u64 n)
{
  mc::memset256(d, static_cast<u8>(0), n);
}

static void
s_simd_nt256_0(u8 *d, u64 n)
{
  mc::ntmemset256(d, static_cast<u8>(0), n);
}
#endif

#if defined(__micron_arch_x86_any)
static void
s_wordset(u8 *d, u64 n)
{
#if defined(__micron_x86_avx2)
  mc::wordset256(d, 0x0102030405060708ULL, n);
#else
  mc::wordset128(d, 0x0102030405060708ULL, n);
#endif
}

// dispatch-path wordset (__memset_words -> __wordset_bulk), n must be 8-aligned
static void
s_wordset_mc(u8 *d, u64 n)
{
  mc::typeset<u64>(d, 0x0102030405060708ULL, n / 8);
}
#endif

struct mparams {
  u8 *dst;
  const u8 *src;
  u64 n;
  u64 iters;
  u64 mask;
  u64 stride;
};

[[gnu::noinline]] static void
measure_copy(cfn volatile *pf, const mparams &p, f64 &ns_out, f64 &cyc_out) noexcept
{
  f64 ns_s[K_MEASUREMENTS], cyc_s[K_MEASUREMENTS];
  u64 sink = 0;
  for ( u32 m = 0; m <= K_MEASUREMENTS; ++m ) {
    cfn f = *pf;
    u64 cur = 0;
    const u64 t0 = now_ns(), c0 = cycles();
    for ( u64 i = 0; i < p.iters; ++i ) {
      u8 *d = p.dst + cur;
      f(d, p.src + cur, p.n);
      sink += *reinterpret_cast<volatile const u8 *>(d);
      clobber(d);
      cur = (cur + p.stride) & p.mask;
    }
    const u64 c1 = cycles(), t1 = now_ns();
    if ( m == 0 ) continue;
    ns_s[m - 1] = static_cast<f64>(t1 - t0) / static_cast<f64>(p.iters);
    cyc_s[m - 1] = static_cast<f64>(c1 - c0) / static_cast<f64>(p.iters);
  }
  g_vsink ^= sink;
  ns_out = median_f64(ns_s, K_MEASUREMENTS);
  cyc_out = median_f64(cyc_s, K_MEASUREMENTS);
}

[[gnu::noinline]] static void
measure_set(sfn volatile *pf, const mparams &p, f64 &ns_out, f64 &cyc_out) noexcept
{
  f64 ns_s[K_MEASUREMENTS], cyc_s[K_MEASUREMENTS];
  u64 sink = 0;
  for ( u32 m = 0; m <= K_MEASUREMENTS; ++m ) {
    sfn f = *pf;
    u64 cur = 0;
    const u64 t0 = now_ns(), c0 = cycles();
    for ( u64 i = 0; i < p.iters; ++i ) {
      u8 *d = p.dst + cur;
      f(d, p.n);
      sink += *reinterpret_cast<volatile const u8 *>(d);
      clobber(d);
      cur = (cur + p.stride) & p.mask;
    }
    const u64 c1 = cycles(), t1 = now_ns();
    if ( m == 0 ) continue;
    ns_s[m - 1] = static_cast<f64>(t1 - t0) / static_cast<f64>(p.iters);
    cyc_s[m - 1] = static_cast<f64>(c1 - c0) / static_cast<f64>(p.iters);
  }
  g_vsink ^= sink;
  ns_out = median_f64(ns_s, K_MEASUREMENTS);
  cyc_out = median_f64(cyc_s, K_MEASUREMENTS);
}

static cfn volatile g_cf = nullptr;
static sfn volatile g_sf = nullptr;

[[gnu::always_inline]] inline u64
budget_for(u64 n) noexcept
{
  if ( n <= 64 ) return 1ull << 20;
  if ( n < (1ull << 20) ) return 1ull << 24;
  return 1ull << 27;
}

[[gnu::always_inline]] inline u64
iters_for(u64 n, u64 stride) noexcept
{
  u64 r = budget_for(n) / (n ? n : 1);
  if ( r < 4 ) r = 4;
  if ( r > (1ull << 22) ) r = 1ull << 22;
  if ( stride ) {
    u64 cap = (2 * g_ring) / stride;
    if ( cap < 4 ) cap = 4;
    if ( r > cap ) r = cap;
  }
  return r;
}

static void
cell_copy(const char *op, const char *impl, cfn f, u64 n, u64 d_off, u64 s_off, bool cold, i64 delta, u8 *dbase = nullptr,
          const u8 *sbase = nullptr)
{
  if ( cold && g_hot_only ) return;
  if ( !cold && g_cold_only ) return;
  mparams p;
  p.dst = (dbase ? dbase : g_dring) + d_off;
  p.src = (sbase ? sbase : g_sring) + s_off;
  p.n = n;
  p.stride = cold ? (((n + 4095) & ~4095ull) + 4096) : 0;
  p.mask = cold ? (g_ring - 1) : 0;
  p.iters = iters_for(n, p.stride);
  f64 bns, bcyc, ns, cyc;
  g_cf = c_noop;
  measure_copy(&g_cf, p, bns, bcyc);
  g_cf = f;
  measure_copy(&g_cf, p, ns, cyc);
  const f64 dcyc = cyc > bcyc ? cyc - bcyc : 0.0;
  const f64 dns = ns > bns ? ns - bns : 0.0;
  const f64 cyc_pb = n ? dcyc / static_cast<f64>(n) : dcyc;
  const f64 gibs = (n && dns > 0.0) ? (static_cast<f64>(n) * 1e9 / dns) / static_cast<f64>(1ull << 30) : 0.0;
  emit_row(op, impl, n, d_off & 4095, s_off & 4095, cold ? "cold" : "hot", delta, cyc_pb, gibs);
}

static void
cell_set(const char *op, const char *impl, sfn f, u64 n, u64 d_off, bool cold)
{
  if ( cold && g_hot_only ) return;
  if ( !cold && g_cold_only ) return;
  mparams p;
  p.dst = g_dring + d_off;
  p.src = nullptr;
  p.n = n;
  p.stride = cold ? (((n + 4095) & ~4095ull) + 4096) : 0;
  p.mask = cold ? (g_ring - 1) : 0;
  p.iters = iters_for(n, p.stride);
  f64 bns, bcyc, ns, cyc;
  g_sf = s_noop;
  measure_set(&g_sf, p, bns, bcyc);
  g_sf = f;
  measure_set(&g_sf, p, ns, cyc);
  const f64 dcyc = cyc > bcyc ? cyc - bcyc : 0.0;
  const f64 dns = ns > bns ? ns - bns : 0.0;
  const f64 cyc_pb = n ? dcyc / static_cast<f64>(n) : dcyc;
  const f64 gibs = (n && dns > 0.0) ? (static_cast<f64>(n) * 1e9 / dns) / static_cast<f64>(1ull << 30) : 0.0;
  emit_row(op, impl, n, d_off & 4095, 0, cold ? "cold" : "hot", 0, cyc_pb, gibs);
}

static u64
detect_l3() noexcept
{
#if defined(__micron_arch_x86_any)
  u32 a, b, c, d;
  u64 best = 0;
  for ( u32 pass = 0; pass < 2 && !best; ++pass ) {
    const u32 leaf = pass == 0 ? 4u : 0x8000001Du;
    if ( pass == 1 ) {
      asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(0x80000000u), "2"(0u));
      if ( a < 0x8000001Du ) break;
    }
    for ( u32 sub = 0; sub < 8; ++sub ) {
      asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(leaf), "2"(sub));
      if ( (a & 0x1Fu) == 0 ) break;
      const u32 level = (a >> 5) & 7u;
      const u64 ways = ((b >> 22) & 0x3FFu) + 1;
      const u64 parts = ((b >> 12) & 0x3FFu) + 1;
      const u64 line_sz = (b & 0xFFFu) + 1;
      const u64 sets = static_cast<u64>(c) + 1;
      if ( level == 3 ) {
        const u64 sz = ways * parts * line_sz * sets;
        if ( sz > best ) best = sz;
      }
    }
  }
  return best ? best : (32ull << 20);
#else
  return 32ull << 20;
#endif
}

static void
sec_a_small()
{
  section("A dense small 0..64, hot, aligned");
  for ( u64 n = 0; n <= 64; ++n ) {
    cell_copy("memcpy", "micron", c_mc, n, 0, 0, false, 0);
    cell_copy("memcpy", "libc", c_libc, n, 0, 0, false, 0);
    cell_copy("memcpy", "raw32", c_raw32, n, 0, 0, false, 0);
    cell_copy("memmove", "micron", c_mc_move, n, 0, 0, false, 0);
    cell_copy("memmove", "libc", c_libc_move, n, 0, 0, false, 0);
    cell_set("memset0", "micron", s_mc0, n, 0, false);
    cell_set("memset0", "libc", s_libc0, n, 0, false);
    cell_set("memset5A", "micron", s_mc5A, n, 0, false);
  }
}

static void
sec_b_pow2()
{
  section("B pow2 span 64..64MiB (+/-1 up to 1MiB), hot");
  u64 sizes[128];
  u32 ns = 0;
  for ( u32 k = 6; k <= 26; ++k ) sizes[ns++] = 1ull << k;
  for ( u32 k = 6; k <= 20; ++k ) {
    sizes[ns++] = (1ull << k) - 1;
    sizes[ns++] = (1ull << k) + 1;
  }
  for ( u32 s = 0; s < ns; ++s ) {
    const u64 n = sizes[s];
    cell_copy("memcpy", "micron", c_mc, n, 0, 0, false, 0);
    cell_copy("memcpy", "libc", c_libc, n, 0, 0, false, 0);
    cell_copy("memcpy", "simd128", c_simd128, n, 0, 0, false, 0);
    cell_copy("memcpy", "simd_a128", c_simd_a128, n, 0, 0, false, 0);
#if defined(__micron_x86_avx2)
    cell_copy("memcpy", "simd256", c_simd256, n, 0, 0, false, 0);
    cell_copy("memcpy", "simd_a256", c_simd_a256, n, 0, 0, false, 0);
#endif
#if defined(__micron_x86_avx512f)
    cell_copy("memcpy", "simd512", c_simd512, n, 0, 0, false, 0);
#endif
    if ( n >= 1024 ) {
      cell_copy("memcpy", "simd_nt128", c_simd_nt128, n, 0, 0, false, 0);
#if defined(__micron_x86_avx2)
      cell_copy("memcpy", "simd_nt256", c_simd_nt256, n, 0, 0, false, 0);
#endif
    }
    if ( n == 64 ) cell_copy("memcpy", "builtin", c_builtin<64>, n, 0, 0, false, 0);
    if ( n == 256 ) cell_copy("memcpy", "builtin", c_builtin<256>, n, 0, 0, false, 0);
    if ( n == 4096 ) cell_copy("memcpy", "builtin", c_builtin<4096>, n, 0, 0, false, 0);
    cell_copy("memmove", "micron", c_mc_move, n, 0, 0, false, 0);
    cell_copy("memmove", "libc", c_libc_move, n, 0, 0, false, 0);
    cell_set("memset0", "micron", s_mc0, n, 0, false);
    cell_set("memset0", "libc", s_libc0, n, 0, false);
    cell_set("memset0", "simd128", s_simd128_0, n, 0, false);
#if defined(__micron_x86_avx2)
    cell_set("memset0", "simd256", s_simd256_0, n, 0, false);
#endif
    if ( n >= 1024 ) {
      cell_set("memset0", "simd_nt128", s_simd_nt128_0, n, 0, false);
#if defined(__micron_x86_avx2)
      cell_set("memset0", "simd_nt256", s_simd_nt256_0, n, 0, false);
#endif
    }
    cell_set("memset5A", "micron", s_mc5A, n, 0, false);
    cell_set("memset5A", "libc", s_libc5A, n, 0, false);
    if ( g_full ) cell_set("memsetFF", "micron", s_mcFF, n, 0, false);
#if defined(__micron_arch_x86_any)
    cell_set("wordset", "simd", s_wordset, n, 0, false);
#endif
  }
}

static void
sec_c_align()
{
  section("C alignment matrix, hot");
  const u64 offs_dflt[] = { 0, 1, 8, 15, 32, 63 };
  const u64 offs_full[] = { 0, 1, 4, 7, 8, 11, 15, 16, 24, 31, 32, 33, 48, 63 };
  const u64 *offs = g_full ? offs_full : offs_dflt;
  const u32 noffs = g_full ? 14 : 6;
  const u64 sizes[] = { 64, 256, 4096, 1ull << 20 };
  for ( u64 n : sizes )
    for ( u32 di = 0; di < noffs; ++di )
      for ( u32 si = 0; si < noffs; ++si ) {
        cell_copy("memcpy", "micron", c_mc, n, offs[di], offs[si], false, 0);
        cell_copy("memcpy", "libc", c_libc, n, offs[di], offs[si], false, 0);
        cell_copy("memmove", "micron", c_mc_move, n, offs[di], offs[si], false, 0);
      }
  for ( u64 n : sizes )
    for ( u32 di = 0; di < noffs; ++di ) {
      cell_set("memset0", "micron", s_mc0, n, offs[di], false);
      cell_set("memset0", "libc", s_libc0, n, offs[di], false);
    }

#if defined(__micron_arch_x86_any)
  for ( u64 n : sizes )
    for ( u32 di = 0; di < noffs; ++di ) cell_set("wordsetd", "micron", s_wordset_mc, n, offs[di], false);
#endif

  for ( u64 n : { (u64)64, (u64)256 } )
    for ( u64 off : { (u64)4095, (u64)4065 } ) {
      cell_copy("memcpy", "micron", c_mc, n, off, 0, false, 0);
      cell_copy("memcpy", "micron", c_mc, n, 0, off, false, 0);
      cell_copy("memcpy", "libc", c_libc, n, off, 0, false, 0);
      cell_copy("memcpy", "libc", c_libc, n, 0, off, false, 0);
    }

  for ( u64 delta : { (u64)0, (u64)8, (u64)60 } ) {
    for ( u64 n : { (u64)512, (u64)2048, (u64)4096 } ) {
      cell_copy("memcpy", "micron", c_mc, n, 0, 0, false, 0, g_dring, g_dring + 4096 + delta);
      cell_copy("memcpy", "libc", c_libc, n, 0, 0, false, 0, g_dring, g_dring + 4096 + delta);
    }

    cell_copy("memmove", "micron", c_mc_move, 16384, 0, 0, false, (i64)(-(4096 + delta)), g_dring, g_dring + 4096 + delta);
    cell_copy("memmove", "libc", c_libc_move, 16384, 0, 0, false, (i64)(-(4096 + delta)), g_dring, g_dring + 4096 + delta);
  }
}

static void
sec_d_erms()
{
  section("D ERMS window 2048..65536, hot");
  const u64 step = g_full ? 64 : 256;
  for ( u64 n = 2048; n <= 65536; n += step ) {
    cell_copy("memcpy", "micron", c_mc, n, 0, 0, false, 0);
    cell_copy("memcpy", "libc", c_libc, n, 0, 0, false, 0);
    cell_set("memset0", "micron", s_mc0, n, 0, false);
    cell_set("memset0", "libc", s_libc0, n, 0, false);
  }
}

static void
sec_e_nt(u64 l3)
{
  section("E NT crossover: L3/8 .. 4xL3, hot+cold");
  const u32 kstep = g_full ? 1 : 2;
  for ( u32 k = 0; k <= 20; k += kstep ) {

    u64 n = l3 >> 3;
    for ( u32 j = 0; j < k / 4; ++j ) n <<= 1;
    const u32 rem = k % 4;
    if ( rem == 1 ) n += n / 5;
    if ( rem == 2 ) n += (n * 2) / 5;
    if ( rem == 3 ) n += (n * 7) / 10;
    n &= ~4095ull;
    if ( n > g_ring / 2 ) break;
    for ( u32 mode = 0; mode < 2; ++mode ) {
      const bool cold = mode == 1;
      cell_copy("memcpy", "micron", c_mc, n, 0, 0, cold, 0);
      cell_copy("memcpy", "libc", c_libc, n, 0, 0, cold, 0);
      cell_copy("memcpy", "simd_a128", c_simd_a128, n, 0, 0, cold, 0);
      cell_copy("memcpy", "simd_nt128", c_simd_nt128, n, 0, 0, cold, 0);
#if defined(__micron_x86_avx2)
      cell_copy("memcpy", "simd_a256", c_simd_a256, n, 0, 0, cold, 0);
      cell_copy("memcpy", "simd_nt256", c_simd_nt256, n, 0, 0, cold, 0);
#endif
      cell_set("memset0", "micron", s_mc0, n, 0, cold);
      cell_set("memset0", "libc", s_libc0, n, 0, cold);
      cell_set("memset0", "simd128", s_simd128_0, n, 0, cold);
      cell_set("memset0", "simd_nt128", s_simd_nt128_0, n, 0, cold);
#if defined(__micron_x86_avx2)
      cell_set("memset0", "simd256", s_simd256_0, n, 0, cold);
      cell_set("memset0", "simd_nt256", s_simd_nt256_0, n, 0, cold);
#endif
    }
  }
}

static void
sec_f_cold()
{
  section("F cold/streaming general");
  for ( u64 n : { (u64)4096, (u64)65536, (u64)1ull << 20, (u64)16ull << 20 } ) {
    cell_copy("memcpy", "micron", c_mc, n, 0, 0, true, 0);
    cell_copy("memcpy", "libc", c_libc, n, 0, 0, true, 0);
    cell_copy("memcpy", "simd_nt128", c_simd_nt128, n, 0, 0, true, 0);
#if defined(__micron_x86_avx2)
    cell_copy("memcpy", "simd_nt256", c_simd_nt256, n, 0, 0, true, 0);
#endif
    cell_copy("memmove", "micron", c_mc_move, n, 0, 0, true, 0);
    cell_set("memset0", "micron", s_mc0, n, 0, true);
    cell_set("memset0", "libc", s_libc0, n, 0, true);
    cell_set("memset0", "simd_nt128", s_simd_nt128_0, n, 0, true);
#if defined(__micron_x86_avx2)
    cell_set("memset0", "simd_nt256", s_simd_nt256_0, n, 0, true);
#endif
  }
}

static void
sec_g_overlap()
{
  section("G memmove overlap (delta = dst - src)");
  u8 *base = g_dring + (1ull << 20);
  for ( u64 delta : { (u64)1, (u64)8, (u64)15, (u64)31, (u64)63, (u64)64, (u64)4096, (u64)4104 } )
    for ( u64 n : { (u64)64, (u64)256, (u64)4096, (u64)65536, (u64)1ull << 20 } ) {
      if ( delta >= n ) continue;

      cell_copy("memmove", "micron", c_mc_move, n, 0, 0, false, (i64)delta, base + delta, base);
      cell_copy("memmove", "libc", c_libc_move, n, 0, 0, false, (i64)delta, base + delta, base);

      cell_copy("memmove", "micron", c_mc_move, n, 0, 0, false, -(i64)delta, base, base + delta);
      cell_copy("memmove", "libc", c_libc_move, n, 0, 0, false, -(i64)delta, base, base + delta);
    }
}

static bool
streq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

static u64
parse_u(const char *p) noexcept
{
  u64 v = 0;
  while ( *p >= '0' && *p <= '9' ) v = v * 10 + static_cast<u64>(*p++ - '0');
  return v;
}

}      // namespace

int
main(int argc, char **argv)
{
  u32 cpu = 0;
  for ( int i = 1; i < argc; ++i ) {
    if ( streq(argv[i], "--csv") )
      g_csv = true;
    else if ( streq(argv[i], "--full") )
      g_full = true;
    else if ( streq(argv[i], "--hot-only") )
      g_hot_only = true;
    else if ( streq(argv[i], "--cold-only") )
      g_cold_only = true;
    else if ( streq(argv[i], "--cpu") && i + 1 < argc )
      cpu = static_cast<u32>(parse_u(argv[++i]));
  }
  if ( g_full ) g_ring = 1ull << 28;

  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(cpu);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  {
    volatile u64 spin = 0;
    const u64 t0 = now_ns();
    while ( now_ns() - t0 < 1500000000ULL )
      for ( u64 i = 0; i < 65536; ++i ) spin += i;
  }

  const u64 alloc = g_ring + RING_SLACK;
  g_dring = micron::bytemap(alloc);
  g_sring = micron::bytemap(alloc);
  if ( micron::mmap_failed(g_dring) || micron::mmap_failed(g_sring) ) {
    micron::io::println("mmap failed");
    return 2;
  }
  for ( u64 i = 0; i < alloc; ++i ) {
    g_sring[i] = static_cast<u8>(i * 0x9Eu + 0x37u);
    g_dring[i] = static_cast<u8>(i * 0x6Bu + 0x9Du);
  }

  const u64 l3 = detect_l3();
  if ( g_csv ) {
    micron::io::println("# bench=memory_detail");
    micron::io::println("# l3_bytes=", l3);
    micron::io::println("# ring_bytes=", g_ring);
    micron::io::println("# k=", (u64)K_MEASUREMENTS, " warmup=1 cpu=", (u64)cpu);
    micron::io::println("op,impl,size,dst_align,src_align,mode,delta,cyc_per_byte,gib_s");
  } else {
    micron::io::println("=== MICRON MEMORY DETAIL (median-of-5, baseline-subtracted) ===");
    micron::io::println("l3=", l3, " ring=", g_ring, " cpu=", (u64)cpu);
    micron::io::println("note: cyc column is TSC ticks on x86 (invariant TSC assumed) / cntvct on arm64");
  }

  sec_a_small();
  sec_b_pow2();
  sec_c_align();
  sec_d_erms();
  sec_e_nt(l3);
  sec_f_cold();
  sec_g_overlap();

  if ( !g_csv ) micron::io::println("");
  if ( !g_csv ) micron::io::println("[done] sink=", (u64)g_vsink);
  return 0;
}

#pragma GCC pop_options
