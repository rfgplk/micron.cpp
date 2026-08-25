//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// arbint operation benchmark: throughput and per-operation latency distribution.

//   throughput  one clean untimed-instrumentation run -> cyc/op, ns/op, Mops/s. no per-op timer, so
//               the cycle count is the kernel's and not the timer's.
//   latency     a separate rdtsc-per-op run -> p10 / p50 / p90 / p99 / p99.9 / max in ns. the timer
//               overhead is in these numbers by construction, which is why they do not contaminate
//               the throughput ones.

#if defined(ARBINT_BENCH_PERF)
#include "../external/bbench/bench.hpp"
#endif

#include "../src/chrono.hpp"
#include "../src/io/console.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/arbint.hpp"
#include "../src/std.hpp"

namespace
{

namespace mpn = micron::math::mpn;
using U = micron::math::arbuint<>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP = 3;
constexpr u64 PCTL_FLOOR = 20000ull;
constexpr u64 MAX_LAT = 1u << 17;

alignas(64) u64 g_lat[MAX_LAT];
u64 g_lat_n = 0;
f64 g_ns_per_tick = 1.0;

[[gnu::always_inline]] inline u64
ticks() noexcept
{
  return micron::math::__asm_op::rdtsc64();
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::cold]] void
calibrate() noexcept
{
  micron::system_clock<micron::system_clocks::monotonic> clk;
  clk.start();
  const u64 t0 = ticks();
  u64 spin = 0;

  for ( u64 i = 0; i < 20000000ull; ++i ) {
    spin += i;
    clobber(&spin);
  }
  const u64 t1 = ticks();
  const f64 ns = clk.elapsed<micron::unit::nanoseconds>();
  const u64 dt = t1 - t0;
  g_ns_per_tick = dt ? (ns / static_cast<f64>(dt)) : 1.0;
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 k = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > k ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = k;
  }
  return xs[n / 2];
}

void
sort_u64(u64 *a, u64 n) noexcept
{
  static const u64 gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };
  for ( u64 g : gaps ) {
    for ( u64 i = g; i < n; ++i ) {
      const u64 t = a[i];
      u64 j = i;
      while ( j >= g && a[j - g] > t ) {
        a[j] = a[j - g];
        j -= g;
      }
      a[j] = t;
    }
  }
}

[[gnu::always_inline]] inline u64
pick(const u64 *sorted, u64 n, f64 q) noexcept
{
  if ( n == 0 ) return 0;
  u64 idx = static_cast<u64>(q * static_cast<f64>(n - 1) + 0.5);
  if ( idx >= n ) idx = n - 1;
  return sorted[idx];
}

struct sample {
  f64 cyc_per_op;
  f64 ns_per_op;
  f64 mops;
};

struct pctl {
  f64 p10, p50, p90, p99, p999, mx;
};

struct fmt2 {
  u64 whole;
  u32 frac;
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

  constexpr line() noexcept : buf{}, pos(0) { }

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
  s_lj(const char *p, u32 width) noexcept
  {
    u32 n = 0;
    while ( p[n] ) buf[pos++] = p[n++];
    while ( n < width ) {
      buf[pos++] = ' ';
      ++n;
    }
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char t[24];
    u32 n = 0;
    if ( v == 0 )
      t[n++] = '0';
    else
      while ( v ) {
        t[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
      }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = t[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char t[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      t[n++] = '0';
    else
      while ( w ) {
        t[n++] = static_cast<char>('0' + (w % 10));
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = t[--n];
    buf[pos++] = '.';
    buf[pos++] = static_cast<char>('0' + (f.frac / 10));
    buf[pos++] = static_cast<char>('0' + (f.frac % 10));
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

template<typename Fn>
sample
throughput(u64 reps, Fn &&fn) noexcept
{
  for ( u64 i = 0; i < WARMUP; ++i ) fn();

  f64 cyc_s[K_MEASUREMENTS];
  f64 ns_s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    clk.start();
    asm volatile("" ::: "memory");
    const u64 t0 = ticks();
    for ( u64 i = 0; i < reps; ++i ) fn();
    asm volatile("" ::: "memory");
    const u64 dt = ticks() - t0;
    const f64 ns = clk.elapsed<micron::unit::nanoseconds>();
    cyc_s[m] = static_cast<f64>(dt) / static_cast<f64>(reps);
    ns_s[m] = ns / static_cast<f64>(reps);
  }
  const f64 c = median_f64(cyc_s, K_MEASUREMENTS);
  const f64 n = median_f64(ns_s, K_MEASUREMENTS);
  return sample{ c, n, n > 0.0 ? 1000.0 / n : 0.0 };
}

template<typename Fn>
pctl
latency(Fn &&fn) noexcept
{
  g_lat_n = 0;
  while ( g_lat_n < PCTL_FLOOR && g_lat_n < MAX_LAT ) {
    const u64 t0 = ticks();
    fn();
    const u64 t1 = ticks();
    g_lat[g_lat_n++] = t1 - t0;
  }
  sort_u64(g_lat, g_lat_n);
  const f64 k = g_ns_per_tick;
  return pctl{ static_cast<f64>(pick(g_lat, g_lat_n, 0.10)) * k,  static_cast<f64>(pick(g_lat, g_lat_n, 0.50)) * k,
               static_cast<f64>(pick(g_lat, g_lat_n, 0.90)) * k,  static_cast<f64>(pick(g_lat, g_lat_n, 0.99)) * k,
               static_cast<f64>(pick(g_lat, g_lat_n, 0.999)) * k, static_cast<f64>(g_lat[g_lat_n - 1]) * k };
}

[[gnu::cold]] void
tput_header(const char *title)
{
  micron::io::println("");
  micron::io::println("[", title, "]  throughput -- cyc/op is rdtsc, ns/op is the monotonic clock");
  line h;
  h.s("  ");
  h.s_lj("op", 14);
  h.s_at("bits", 24);
  h.s_at("limbs", 32);
  h.s_at("cyc/op", 46);
  h.s_at("ns/op", 60);
  h.s_at("Mops/s", 72);
  micron::io::println(h.str());
  micron::io::println("  ------------------------------------------------------------------------");
}

[[gnu::cold]] void
lat_header(const char *title)
{
  micron::io::println("");
  micron::io::println("[", title, "]  per-op latency, nanoseconds (rdtsc per op, TSC-calibrated)");
  line h;
  h.s("  ");
  h.s_lj("op", 14);
  h.s_at("bits", 24);
  h.s_at("p10", 34);
  h.s_at("p50", 44);
  h.s_at("p90", 54);
  h.s_at("p99", 66);
  h.s_at("p99.9", 78);
  h.s_at("max", 90);
  micron::io::println(h.str());
  micron::io::println("  ------------------------------------------------------------------------------------");
}

[[gnu::cold]] void
tput_row(const char *op, u64 bits, u64 limbs, const sample &s)
{
  line l;
  l.s("  ");
  l.s_lj(op, 14);
  l.u_at(bits, 24);
  l.u_at(limbs, 32);
  l.f2_at(to_fmt2(s.cyc_per_op), 46);
  l.f2_at(to_fmt2(s.ns_per_op), 60);
  l.f2_at(to_fmt2(s.mops), 72);
  micron::io::println(l.str());
}

[[gnu::cold]] void
lat_row(const char *op, u64 bits, const pctl &p)
{
  line l;
  l.s("  ");
  l.s_lj(op, 14);
  l.u_at(bits, 24);
  l.f2_at(to_fmt2(p.p10), 34);
  l.f2_at(to_fmt2(p.p50), 44);
  l.f2_at(to_fmt2(p.p90), 54);
  l.f2_at(to_fmt2(p.p99), 66);
  l.f2_at(to_fmt2(p.p999), 78);
  l.f2_at(to_fmt2(p.mx), 90);
  micron::io::println(l.str());
}

constexpr u64 BITS[] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
constexpr usize N_BITS = sizeof(BITS) / sizeof(BITS[0]);

[[gnu::always_inline]] inline u64
lcg(u64 &s) noexcept
{
  s = s * 6364136223846793005ull + 1442695040888963407ull;
  return s;
}

U
make(u64 bits, u64 seed)
{
  U a;
  u64 s = seed;
  for ( u64 i = 0; i < bits; i += 64 ) {
    a <<= 64;
    a += U(lcg(s));
  }
  a |= U::power_of_two(bits - 1u);
  return a;
}

[[nodiscard]] u64
reps_for(u64 bits) noexcept
{
  const u64 limbs = bits / mpn::limb_bits;
  const u64 work = limbs * limbs + 1u;
  u64 r = (1ull << 22) / work;
  if ( r < 32 ) r = 32;
  if ( r > (1ull << 18) ) r = 1ull << 18;
  return r;
}

}      // namespace

int
main()
{
  calibrate();

  micron::io::println("=== arbint operation benchmark ===");
  micron::io::println("limb width: ", static_cast<u64>(mpn::limb_bits), " bits    measurements: ", static_cast<u64>(K_MEASUREMENTS),
                      " (median)    latency samples: >= ", static_cast<u64>(PCTL_FLOOR));
  micron::io::println("mul ladder built through nussbaumer; crossovers karatsuba/toom3/toom4/nussbaumer: ",
                      static_cast<u64>(mpn::threshold::mul_karatsuba), "/", static_cast<u64>(mpn::threshold::mul_toom3), "/",
                      static_cast<u64>(mpn::threshold::mul_toom4), "/", static_cast<u64>(mpn::threshold::mul_nussbaumer));
  micron::io::println("sqr crossovers karatsuba/toom3/toom4/nussbaumer: ", static_cast<u64>(mpn::threshold::sqr_karatsuba), "/",
                      static_cast<u64>(mpn::threshold::sqr_toom3), "/", static_cast<u64>(mpn::threshold::sqr_toom4), "/",
                      static_cast<u64>(mpn::threshold::sqr_nussbaumer));

  tput_header("arbuint<>, dynamic width");
  for ( usize i = 0; i < N_BITS; ++i ) {
    const u64 bits = BITS[i];
    const u64 limbs = bits / mpn::limb_bits;
    const u64 reps = reps_for(bits);
    const U a = make(bits, 0x1234567ull + bits);
    const U b = make(bits, 0x89ABCDEFull + bits);
    const U small(0xC0FFEEull);
    U sink;

    tput_row("add", bits, limbs, throughput(reps * 8u, [&] {
               sink = a + b;
               clobber(&sink);
             }));
    tput_row("sub", bits, limbs, throughput(reps * 8u, [&] {
               sink = a - b;
               clobber(&sink);
             }));
    tput_row("mul", bits, limbs, throughput(reps, [&] {
               sink = a * b;
               clobber(&sink);
             }));
    tput_row("sqr", bits, limbs, throughput(reps, [&] {
               sink = micron::math::sqr(a);
               clobber(&sink);
             }));
    tput_row("divrem", bits, limbs, throughput(reps, [&] {
               sink = a / b;
               clobber(&sink);
             }));
    tput_row("mod_small", bits, limbs, throughput(reps * 4u, [&] {
               sink = a % small;
               clobber(&sink);
             }));
    tput_row("shl 1", bits, limbs, throughput(reps * 8u, [&] {
               sink = a << 1;
               clobber(&sink);
             }));
    tput_row("shr 67", bits, limbs, throughput(reps * 8u, [&] {
               sink = a >> 67;
               clobber(&sink);
             }));
    tput_row("cmp", bits, limbs, throughput(reps * 32u, [&] {
               const int c = micron::math::cmp(a, b);
               clobber(&c);
             }));
  }

  lat_header("arbuint<>, dynamic width");
  for ( usize i = 0; i < N_BITS; ++i ) {
    const u64 bits = BITS[i];
    const U a = make(bits, 0x1234567ull + bits);
    const U b = make(bits, 0x89ABCDEFull + bits);
    U sink;
    lat_row("mul", bits, latency([&] {
              sink = a * b;
              clobber(&sink);
            }));
    lat_row("divrem", bits, latency([&] {
              sink = a / b;
              clobber(&sink);
            }));
  }

  tput_header("arbuint<N>, bounded and inline (no allocator on any path)");
  {
    const auto cell = [&]<usize N>() {
      using B = micron::math::arbuint<N>;
      const u64 reps = reps_for(N);
      B a, b;
      u64 s = 0xFEEDFACEull;
      for ( u64 i = 0; i < N; i += 64 ) {
        a <<= 64;
        a += B(lcg(s));
        b <<= 64;
        b += B(lcg(s));
      }
      B sink;
      tput_row("add", N, N / mpn::limb_bits, throughput(reps * 8u, [&] {
                 sink = a + b;
                 clobber(&sink);
               }));
      tput_row("mul", N, N / mpn::limb_bits, throughput(reps, [&] {
                 sink = a * b;
                 clobber(&sink);
               }));
      tput_row("sqr", N, N / mpn::limb_bits, throughput(reps, [&] {
                 sink = micron::math::sqr(a);
                 clobber(&sink);
               }));
      tput_row("divrem", N, N / mpn::limb_bits, throughput(reps, [&] {
                 sink = a / b;
                 clobber(&sink);
               }));
    };
    cell.template operator()<256>();
    cell.template operator()<512>();
    cell.template operator()<1024>();
    cell.template operator()<2048>();
    cell.template operator()<4096>();
  }

  tput_header("base conversion");
  {
    static char buf[140000];
    for ( usize i = 0; i < N_BITS; ++i ) {
      const u64 bits = BITS[i];
      const u64 limbs = bits / mpn::limb_bits;
      const U a = make(bits, 0xABCDEFull + bits);
      const u64 reps = reps_for(bits) / 2u + 1u;
      usize len = 0;
      tput_row("to_chars 10", bits, limbs, throughput(reps, [&] {
                 len = micron::math::to_chars(buf, sizeof buf, a, 10);
                 clobber(buf);
               }));
      tput_row("to_chars 16", bits, limbs, throughput(reps, [&] {
                 len = micron::math::to_chars(buf, sizeof buf, a, 16);
                 clobber(buf);
               }));

      micron::string str;
      tput_row("to_string 10", bits, limbs, throughput(reps, [&] {
                 str = micron::math::to_string(a, 10);
                 clobber(str.data());
               }));
      len = micron::math::to_chars(buf, sizeof buf, a, 10);
      U parsed;
      tput_row("from_chars 10", bits, limbs, throughput(reps, [&] {
                 (void)micron::math::from_chars(parsed, buf, len, 10);
                 clobber(&parsed);
               }));
    }
  }

  micron::io::println("");
  return 0;
}
