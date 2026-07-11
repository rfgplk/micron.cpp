//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/hash/crc.hpp"
#include "../src/hash/hash.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;
constexpr i64 SEED0 = 0x369bd65914cf0616;

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
  u32 lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
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

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * 100.0 + 0.5);
  return { s / 100, static_cast<u32>(s % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

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

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

[[gnu::cold]] void
print_col_header()
{
  line h;
  h.s("hash");
  h.s_at("N", 28);
  h.s_at("cyc/hash", 42);
  h.s_at("cyc/byte", 56);
  h.s_at("GiB/s", 68);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

[[gnu::cold]] void
print_row(const char *algo, u64 n, f64 cyc_hash, f64 cyc_byte, f64 gibs)
{
  line ln;
  ln.s(algo);
  ln.u_at(n, 28);
  ln.f2_at(to_fmt2(cyc_hash), 42);
  ln.f2_at(to_fmt2(cyc_byte), 56);
  ln.f2_at(to_fmt2(gibs), 68);
  micron::io::println(ln.str());
}

typedef u64 (*hfn)(const byte *, i64, usize);

static u64
a_noop(const byte *, i64 s, usize)
{
  return (u64)s;
}

static u64
a_z(const byte *p, i64 s, usize n)
{
  return micron::hashes::z64(p, s, n);
}

static u64
a_zz(const byte *p, i64 s, usize n)
{
  return micron::hashes::zz64(p, s, n);
}

static u64
a_zzz(const byte *p, i64 s, usize n)
{
  return micron::hashes::zzz64(p, s, n);
}

static u64
a_zzzf(const byte *p, i64 s, usize n)
{
  return micron::hashes::zzzf64(p, s, n);
}

static u64
a_xxh(const byte *p, i64 s, usize n)
{
  return micron::hashes::xxhash64_rtseed(p, (u64)s, n);
}

static u64
a_rapid(const byte *p, i64 s, usize n)
{
  return micron::hashes::rapidhash(p, (u64)s, n);
}

static u64
a_rapidm(const byte *p, i64 s, usize n)
{
  return micron::hashes::rapidhash_micro(p, (u64)s, n);
}

static u64
a_rapidn(const byte *p, i64 s, usize n)
{
  return micron::hashes::rapidhash_nano(p, (u64)s, n);
}

static u64
a_zzz128(const byte *p, i64 s, usize n)
{
  auto h = micron::hashes::zzz128(p, s, n);
  return h.a ^ h.b;
}

static u64
a_zzzf128(const byte *p, i64 s, usize n)
{
  auto h = micron::hashes::zzzf128(p, s, n);
  return h.a ^ h.b;
}

static u64
a_murmur(const byte *p, i64 s, usize n)
{
  auto h = micron::hashes::murmur(p, n, (u32)s);
  return h.a ^ h.b;
}

static u64
a_fnv32(const byte *p, i64 s, usize n)
{
  return (u64)micron::fnv1a_32(p, (u32)s, n);
}

static u64
a_bern(const byte *p, i64, usize n)
{
  return (u64)micron::bernstein_32(p, n);
}

static u64
a_crc32c(const byte *p, i64 s, usize n)
{
  return (u64)micron::crc32_iscsi((u32)s, p, n);
}

static u64
a_crc64(const byte *p, i64 s, usize n)
{
  return micron::crc64_ecma_refl((u64)s, p, n);
}
#if defined(__micron_arch_x86_any)
static u64
a_meow64(const byte *p, i64 s, usize n)
{
  return micron::hashes::meowhash64(p, (u64)s, n);
}

static u64
a_meow128(const byte *p, i64 s, usize n)
{
  auto h = micron::hashes::meowhash128(p, (u64)s, n);
  return h.a ^ h.b;
}
#endif

constexpr usize SIZES[] = { 4, 8, 16, 32, 64, 256, 1024, 4096, 65536, 1u << 20 };
constexpr usize MAXN = 1u << 20;
alignas(64) static byte g_buf[MAXN + 64];
static volatile u64 g_sink = 0;

[[gnu::always_inline]] inline u64
iters_for(usize n) noexcept
{
  const u64 budget = 1ull << 24;
  u64 r = budget / (n ? n : 1);
  if ( r < 64 ) r = 64;
  if ( r > (1ull << 20) ) r = 1ull << 20;
  return r;
}

static void
measure(hfn f, usize n, u64 iters, f64 &ns_out, f64 &cyc_out) noexcept
{
  u64 sink = 0;
  for ( u64 w = 0; w < WARMUP_REPS; ++w )
    for ( u64 i = 0; i < iters; ++i ) {
      g_buf[0] = (byte)i;
      sink ^= f(g_buf, (i64)(SEED0 ^ (i64)i), n);
    }
  f64 ns_s[K_MEASUREMENTS], cyc_s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    const u64 t0 = now_ns(), c0 = rdtsc();
    for ( u64 i = 0; i < iters; ++i ) {
      g_buf[0] = (byte)i;
      sink ^= f(g_buf, (i64)(SEED0 ^ (i64)i), n);
    }
    const u64 c1 = rdtsc(), t1 = now_ns();
    ns_s[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(iters);
    cyc_s[m] = static_cast<f64>(c1 - c0) / static_cast<f64>(iters);
  }
  g_sink ^= sink;
  ns_out = median_f64(ns_s, K_MEASUREMENTS);
  cyc_out = median_f64(cyc_s, K_MEASUREMENTS);
}

static void
bench_one(const char *name, hfn f)
{
  for ( usize n : SIZES ) {
    const u64 iters = iters_for(n);
    f64 base_ns, base_cyc, full_ns, full_cyc;
    measure(a_noop, n, iters, base_ns, base_cyc);
    measure(f, n, iters, full_ns, full_cyc);
    const f64 cyc_hash = full_cyc > base_cyc ? full_cyc - base_cyc : 0.0;
    const f64 ns_hash = full_ns > base_ns ? full_ns - base_ns : 0.0;
    const f64 cyc_byte = cyc_hash / static_cast<f64>(n);
    const f64 gibs = ns_hash > 0.0 ? (static_cast<f64>(n) * 1e9 / ns_hash) / static_cast<f64>(1ull << 30) : 0.0;
    print_row(name, n, cyc_hash, cyc_byte, gibs);
  }
}

struct col {
  const char *name;
  hfn fn;
};

}      // namespace

int
main()
{
  for ( usize i = 0; i < sizeof(g_buf); ++i ) g_buf[i] = (byte)(i * 37 + 11);

  micron::io::println("=== MICRON HASH THROUGHPUT (median-of-5, baseline-subtracted) ===");

  static const col g64[] = {
    { "z64", a_z },
    { "zz64", a_zz },
    { "zzz64", a_zzz },
    { "zzzf64", a_zzzf },
    { "xxhash64", a_xxh },
    { "rapidhash", a_rapid },
    { "rapidhashMicro", a_rapidm },
    { "rapidhashNano", a_rapidn },
#if defined(__micron_arch_x86_any)
    { "meowhash64", a_meow64 },
#endif
  };
  static const col g128[] = {
    { "zzz128", a_zzz128 },
    { "zzzf128", a_zzzf128 },
    { "murmur3", a_murmur },
#if defined(__micron_arch_x86_any)
    { "meowhash128", a_meow128 },
#endif
  };
  static const col g32[] = { { "fnv1a_32", a_fnv32 }, { "bernstein_32", a_bern } };
  static const col gcrc[] = { { "crc32c", a_crc32c }, { "crc64_ecma", a_crc64 } };

  print_header("64-bit output");
  for ( const auto &c : g64 ) bench_one(c.name, c.fn);
  print_header("128-bit output (folded a^b)");
  for ( const auto &c : g128 ) bench_one(c.name, c.fn);
  print_header("32-bit output");
  for ( const auto &c : g32 ) bench_one(c.name, c.fn);
  print_header("CRC (table-driven)");
  for ( const auto &c : gcrc ) bench_one(c.name, c.fn);

  micron::io::println("");
  micron::io::println("[done] sink=", (u64)g_sink);
  return 0;
}
