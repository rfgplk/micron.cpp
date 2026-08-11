//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// text -> float throughput, per input class and per API tier.
// the mirror of io_echo_fmt_bench, which only measures the write direction.
//
// build:  duck build benches/format_parse_bench.cpp --perf --fp --no-ssp --no-lto -o bin/b -f
// run  :  taskset -c 2 ./bin/b/format_parse_bench
// csv  :  taskset -c 2 ./bin/b/format_parse_bench --csv > benches/results/format_parse.csv
//
// --fp matters more here than in most benches: -Ofast's -ffinite-math-only would let gcc fold the
// inf/nan lanes out of the measured loop, and -fno-signed-zeros would merge the +-0 inputs.
// --no-ssp because duck defaults to -fstack-protector-all, a canary on every function, which does
// not cancel out of a ratio.

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/string/conversions/floating_point.hpp"
#include "../src/string/format.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 7;
constexpr u64 WARMUP_REPS = 2;
constexpr usize N_ITEMS = 4096;
constexpr usize ITEM_CAP = 320;

bool g_csv = false;
volatile u64 g_sink = 0;
volatile f64 g_fsink = 0.0;

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

// one contiguous arena plus a length table, so the loop measures the parser and not the generator
struct corpus {
  char text[N_ITEMS * ITEM_CAP];
  u32 len[N_ITEMS];
  u32 off[N_ITEMS];
  u32 n = 0;

  void
  add(const char *s, usize l) noexcept
  {
    if ( n >= N_ITEMS ) return;
    const u32 o = n * ITEM_CAP;
    for ( usize i = 0; i < l && i < ITEM_CAP; ++i ) text[o + i] = s[i];
    off[n] = o;
    len[n] = static_cast<u32>(l < ITEM_CAP ? l : ITEM_CAP);
    ++n;
  }
};

u64 g_state = 0x9E3779B97F4A7C15ull;

u64
rnd() noexcept
{
  g_state ^= g_state >> 12;
  g_state ^= g_state << 25;
  g_state ^= g_state >> 27;
  return g_state * 0x2545F4914F6CDD1Dull;
}

usize
emit_u64(u64 v, char *out) noexcept
{
  char tmp[24];
  usize n = 0;
  if ( v == 0 ) tmp[n++] = '0';
  while ( v ) {
    tmp[n++] = static_cast<char>('0' + (v % 10));
    v /= 10;
  }
  for ( usize i = 0; i < n; ++i ) out[i] = tmp[n - 1 - i];
  return n;
}

usize
emit_i32(i32 v, char *out) noexcept
{
  usize k = 0;
  if ( v < 0 ) {
    out[k++] = '-';
    v = -v;
  }
  return k + emit_u64(static_cast<u64>(v), out + k);
}

enum class kind { int_short, pi_class, sig17, sig19_tie, d2s_rt, subnormal, long768, hex, fail, f32_rt };

void
build(corpus &c, kind k) noexcept
{
  c.n = 0;
  g_state = 0x9E3779B97F4A7C15ull;      // same corpus every run
  char buf[ITEM_CAP];
  for ( usize i = 0; i < N_ITEMS; ++i ) {
    usize n = 0;
    switch ( k ) {
    case kind::int_short:
      n = emit_u64(rnd() % 1000000ull, buf);
      break;
    case kind::pi_class: {
      n = emit_u64(rnd() % 1000ull, buf);
      buf[n++] = '.';
      n += emit_u64(1000 + (rnd() % 9000ull), buf + n);
      break;
    }
    case kind::sig17: {
      n = emit_u64(10000000000000000ull + (rnd() % 90000000000000000ull), buf);
      buf[n++] = 'e';
      n += emit_i32(static_cast<i32>(rnd() % 60ull) - 30, buf + n);
      break;
    }
    case kind::sig19_tie: {
      n = emit_u64(1000000000000000000ull + (rnd() % 9000000000000000000ull), buf);
      for ( usize j = 0; j < 6; ++j ) buf[n++] = '0';
      buf[n++] = '5';
      buf[n++] = 'e';
      n += emit_i32(static_cast<i32>(rnd() % 40ull) - 20, buf + n);
      break;
    }
    case kind::d2s_rt: {
      u64 b = rnd();
      if (((b >> 52) & 0x7FF) == 0x7FF ) b &= ~(1ull << 62);
      f64 v;
      __builtin_memcpy(&v, &b, 8);
      n = micron::__impl::__ryu::d2s_buffered(v, buf);
      break;
    }
    case kind::f32_rt: {
      u32 b = static_cast<u32>(rnd() >> 32);
      if (((b >> 23) & 0xFF) == 0xFF ) b &= ~(1u << 30);
      f32 v;
      __builtin_memcpy(&v, &b, 4);
      n = micron::__impl::__ryu::__f32::f2s_buffered(v, buf);
      break;
    }
    case kind::subnormal: {
      u64 b = (rnd() & 0x000FFFFFFFFFFFFFull) | 1ull;
      f64 v;
      __builtin_memcpy(&v, &b, 8);
      n = micron::__impl::__ryu::d2s_buffered(v, buf);
      break;
    }
    case kind::long768: {
      // >19 significant digits with a nonzero tail: eisel-lemire cannot bound it, so every one
      // of these lands in the shift-and-round decimal tier
      buf[n++] = '1';
      buf[n++] = '.';
      for ( usize j = 0; j < 250; ++j ) buf[n++] = static_cast<char>('0' + (rnd() % 10));
      buf[n++] = 'e';
      n += emit_i32(static_cast<i32>(rnd() % 200ull) - 100, buf + n);
      break;
    }
    case kind::hex: {
      buf[n++] = '0';
      buf[n++] = 'x';
      buf[n++] = '1';
      buf[n++] = '.';
      for ( usize j = 0; j < 8; ++j ) buf[n++] = "0123456789abcdef"[rnd() & 15];
      buf[n++] = 'p';
      n += emit_i32(static_cast<i32>(rnd() % 60ull) - 30, buf + n);
      break;
    }
    case kind::fail: {
      const char *bad[] = { "abc", "", "1e", "1.5abc", "+", "e5" };
      const char *s = bad[rnd() % 6];
      while ( s[n] ) {
        buf[n] = s[n];
        ++n;
      }
      break;
    }
    }
    c.add(buf, n);
  }
}

const char *
kind_name(kind k) noexcept
{
  switch ( k ) {
  case kind::int_short: return "int-short";
  case kind::pi_class: return "pi-class";
  case kind::sig17: return "sig17";
  case kind::sig19_tie: return "sig19-tie";
  case kind::d2s_rt: return "d2s-rt";
  case kind::subnormal: return "subnormal";
  case kind::long768: return "long250";
  case kind::hex: return "hex";
  case kind::fail: return "fail";
  case kind::f32_rt: return "f32-rt";
  }
  return "?";
}

enum class tier { legacy, try_bool, opt };

const char *
tier_name(tier t) noexcept
{
  switch ( t ) {
  case tier::legacy: return "to_double";
  case tier::try_bool: return "try_parse_double";
  case tier::opt: return "parse_double";
  }
  return "?";
}

f64
measure(const corpus &c, kind k, tier t) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 s = 0; s < K_MEASUREMENTS + WARMUP_REPS; ++s ) {
    const u64 t0 = now_ns();
    f64 acc = 0.0;
    u64 hits = 0;
    for ( u32 i = 0; i < c.n; ++i ) {
      const char *p = c.text + c.off[i];
      const u32 l = c.len[i];
      if ( k == kind::f32_rt ) {
        f32 v = 0.0f;
        if ( t == tier::legacy )
          v = micron::format::to_float(p, l);
        else if ( t == tier::try_bool )
          hits += micron::try_parse_float(p, l, v);
        else {
          auto o = micron::format::parse_float(p, l);
          hits += o.is_first();
          if ( o.is_first() ) v = o.cast<f32>();
        }
        acc += static_cast<f64>(v);
      } else {
        f64 v = 0.0;
        if ( t == tier::legacy )
          v = micron::format::to_double(p, l);
        else if ( t == tier::try_bool )
          hits += micron::try_parse_double(p, l, v);
        else {
          auto o = micron::format::parse_double(p, l);
          hits += o.is_first();
          if ( o.is_first() ) v = o.cast<f64>();
        }
        acc += v;
      }
    }
    const u64 t1 = now_ns();
    g_fsink = acc;
    g_sink = hits;
    if ( s >= WARMUP_REPS ) samples[s - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(c.n);
  }
  return median_f64(samples, K_MEASUREMENTS);
}

void
row(kind k, tier t, f64 ns, u32 avg_len) noexcept
{
  if ( g_csv ) {
    micron::io::println(kind_name(k), ",", tier_name(t), ",", ns, ",", avg_len);
    return;
  }
  micron::io::print("  ");
  micron::hstring<schar> a(kind_name(k));
  while ( a.size() < 12 ) a += " ";
  micron::hstring<schar> b(tier_name(t));
  while ( b.size() < 18 ) b += " ";
  micron::io::println(a, b, ns, " ns/parse   (avg ", avg_len, " bytes)");
}

};      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i )
    if ( argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == 'c' ) g_csv = true;

  if ( !g_csv ) micron::io::println("=== format parse (text -> float) — ns per parse, median of ", K_MEASUREMENTS, " ===");

  static corpus c;
  const kind kinds[] = { kind::int_short, kind::pi_class,  kind::sig17, kind::sig19_tie, kind::d2s_rt,
                         kind::subnormal, kind::long768,   kind::hex,   kind::fail,      kind::f32_rt };
  const tier tiers[] = { tier::legacy, tier::try_bool, tier::opt };

  for ( kind k : kinds ) {
    build(c, k);
    u64 tot = 0;
    for ( u32 i = 0; i < c.n; ++i ) tot += c.len[i];
    const u32 avg = static_cast<u32>(tot / (c.n ? c.n : 1));
    for ( tier t : tiers ) row(k, t, measure(c, k, t), avg);
  }

  return 0;
}
