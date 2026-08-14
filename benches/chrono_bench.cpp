//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono benchmark.
//
//   1. clock read cost: raw syscall vs vDSO vs the counter. This is the number that motivates the
//      whole tier -- micron::clock_gettime is a real syscall, not the vDSO.
//   2. counter read cost per serialisation policy, which is what a benchmark harness has to budget
//      for and subtract-or-not (see the lfence/lfence warning in cycles.hpp).
//   3. formatting and parsing throughput.
//
//   methodology: the harness pins itself, warms the core, then takes the MEDIAN of K_MEASUREMENTS
//   runs of REPS iterations. Reported per operation. Timing uses CLOCK_MONOTONIC for wall time and
//   the counter for ticks; ticks are converted with the calibrated tick_hz, never through an f64.
//
//   build: duck run benches/chrono_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/b

#include "../src/chrono.hpp"
#include "../src/chrono/calibrate.hpp"
#include "../src/chrono/measure.hpp"
#include "../src/chrono/vdso.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

namespace ch = micron::chrono;

namespace
{

constexpr u32 K_MEASUREMENTS = 7;
constexpr u64 WARMUP_REPS = 2000;

u64
median_u64(u64 *v, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const u64 k = v[i];
    u32 j = i;
    while ( j > 0 && v[j - 1] > k ) {
      v[j] = v[j - 1];
      --j;
    }
    v[j] = k;
  }
  return v[n / 2];
}

// picoseconds per op, so a sub-nanosecond result still has three digits of signal
template<typename F>
u64
measure_ps(u64 reps, F &&f) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) f();
  u64 s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    const i64 t0 = ch::mono_ns();
    for ( u64 i = 0; i < reps; ++i ) f();
    const i64 t1 = ch::mono_ns();
    s[m] = static_cast<u64>(t1 - t0) * 1000ull / reps;
  }
  return median_u64(s, K_MEASUREMENTS);
}

void
row(const char *name, u64 ps)
{
  micron::sstring<48> pad{};
  usize n = 0;
  while ( name[n] ) ++n;
  for ( usize i = 0; i < n && i < 40; ++i ) pad.push_back(name[i]);
  for ( usize i = n; i < 40; ++i ) pad.push_back(' ');
  mc::console(pad.c_str(), "  ", (long long)(ps / 1000), ".", (long long)((ps % 1000) / 100), " ns/op");
}

void
bench_clock_reads(void)
{
  mc::console("");
  mc::console("[clock read cost]");
  mc::console("op                                        ns/op");
  mc::console("--------------------------------------------------");

  micron::timespec_t ts{};
  row("clock_gettime(REALTIME)   syscall", measure_ps(200000, [&] {
        micron::clock_gettime(micron::clock_realtime, ts);
        ch::sink_ptr(&ts);
      }));
  row("clock_gettime(MONOTONIC)  syscall", measure_ps(200000, [&] {
        micron::clock_gettime(micron::clock_monotonic, ts);
        ch::sink_ptr(&ts);
      }));
  row("clock_gettime(MONO_COARSE) syscall", measure_ps(200000, [&] {
        micron::clock_gettime(micron::clock_monotonic_coarse, ts);
        ch::sink_ptr(&ts);
      }));

  if ( ch::vdso::available() ) {
    row("clock_gettime(REALTIME)   vDSO", measure_ps(2000000, [&] {
          ch::vdso::clock_gettime(micron::clock_realtime, ts);
          ch::sink_ptr(&ts);
        }));
    row("clock_gettime(MONOTONIC)  vDSO", measure_ps(2000000, [&] {
          ch::vdso::clock_gettime(micron::clock_monotonic, ts);
          ch::sink_ptr(&ts);
        }));
  } else {
    mc::console("  (no vDSO resolved on this machine)");
  }

  row("chrono::mono_ns()", measure_ps(200000, [] { ch::sink(ch::mono_ns()); }));
  row("chrono::now_ns()", measure_ps(200000, [] { ch::sink(ch::now_ns()); }));
}

void
bench_counter(void)
{
  mc::console("");
  mc::console("[counter read cost, by serialisation policy]");
  mc::console("op                                        ns/op");
  mc::console("--------------------------------------------------");

  row("tick<none>()", measure_ps(5000000, [] { ch::sink(ch::tick<ch::serial::none>()); }));
  row("tick<lfence>()", measure_ps(5000000, [] { ch::sink(ch::tick<ch::serial::lfence>()); }));
  row("tick<mfence_lfence>()", measure_ps(2000000, [] { ch::sink(ch::tick<ch::serial::mfence_lfence>()); }));
#if defined(__micron_arch_x86_any)
  row("tick<rdtscp>()", measure_ps(2000000, [] { ch::sink(ch::tick<ch::serial::rdtscp>()); }));
  row("tick<cpuid>()", measure_ps(500000, [] { ch::sink(ch::tick<ch::serial::cpuid>()); }));
  u32 aux = 0;
  row("tick_aux()", measure_ps(2000000, [&] { ch::sink(ch::tick_aux(aux)); }));
#endif
  row("tick_start<>() + tick_end<>()", measure_ps(2000000, [] {
        const u64 a = ch::tick_start<>();
        const u64 b = ch::tick_end<>();
        ch::sink(b - a);
      }));

  mc::console("");
  const auto ov = ch::timer_overhead<>();
  const auto rs = ch::timer_resolution<>();
  mc::console("  tick_hz               = ", (long long)ch::tick_hz());
  mc::console("  core_hz               = ", (long long)ch::core_hz());
  mc::console("  overhead (min ticks)  = ", (long long)ov.min_ticks, "  = ", (long long)ov.min_ns, " ns");
  mc::console("  resolution (min ticks)= ", (long long)rs.min_ticks, "  = ", (long long)rs.min_ns, " ns");
  const auto sp = ch::core_hz_spread();
  mc::console("  core_hz spread        = ", (long long)sp.min_hz, " .. ", (long long)sp.max_hz, "  (median ", (long long)sp.median_hz, ")");
}

void
bench_format(void)
{
  mc::console("");
  mc::console("[formatting]");
  mc::console("op                                        ns/op");
  mc::console("--------------------------------------------------");

  char buf[128];
  const ch::civil c = ch::civil_utc(1768000000L);
  ch::civil cn = c;
  cn.ns = 123456789u;

  i64 epoch = 1768000000L;
  row("civil_utc(epoch)", measure_ps(2000000, [&] {
        ch::modify(epoch);
        ch::sink(ch::civil_utc(epoch).d);
      }));
  row("civil_secs(civil)", measure_ps(2000000, [&] {
        ch::civil v = c;
        ch::modify(v.s);
        ch::sink(ch::civil_secs(v));
      }));
  row("write_rfc3339 (no subsec)", measure_ps(1000000, [&] { ch::sink(ch::write_rfc3339(buf, sizeof(buf), c, 0)); }));
  row("write_rfc3339 (9 subsec)", measure_ps(1000000, [&] { ch::sink(ch::write_rfc3339(buf, sizeof(buf), cn, 9)); }));
  row("write_iso_full", measure_ps(1000000, [&] { ch::sink(ch::write_iso_full(buf, sizeof(buf), cn, 9)); }));
  row("write_rfc2822", measure_ps(1000000, [&] { ch::sink(ch::write_rfc2822(buf, sizeof(buf), c)); }));
  row("write_compact", measure_ps(1000000, [&] { ch::sink(ch::write_compact(buf, sizeof(buf), c)); }));
  row("write_strftime (\"%F %T\")", measure_ps(1000000, [&] { ch::sink(ch::write_strftime(buf, sizeof(buf), "%F %T", c)); }));
  row("write_duration_units", measure_ps(1000000, [&] { ch::sink(ch::write_duration_units(buf, sizeof(buf), 5400000000000ull)); }));

  mc::console("");
  mc::console("[parsing]");
  mc::console("op                                        ns/op");
  mc::console("--------------------------------------------------");

  const char *s_iso = "2026-08-14T14:22:11.123456789+02:00";
  const char *s_3339 = "2026-08-14T14:22:11Z";
  const char *s_2822 = "Fri, 09 Jan 2026 23:06:40 +0000";
  const char *s_d1 = "1h30m";
  const char *s_d2 = "2.5s";
  row("parse_iso8601 (full + offset)", measure_ps(1000000, [&] {
        ch::modify(s_iso);
        ch::sink(ch::parse_iso8601(s_iso, 35).ok());
      }));
  row("parse_rfc3339 (Z)", measure_ps(1000000, [&] {
        ch::modify(s_3339);
        ch::sink(ch::parse_rfc3339(s_3339, 20).ok());
      }));
  row("parse_rfc2822", measure_ps(1000000, [&] {
        ch::modify(s_2822);
        ch::sink(ch::parse_rfc2822(s_2822, 31).ok());
      }));
  row("parse_duration_ns (\"1h30m\")", measure_ps(2000000, [&] {
        ch::modify(s_d1);
        ch::sink(ch::parse_duration_ns(s_d1, 5).ns);
      }));
  row("parse_duration_ns (\"2.5s\")", measure_ps(2000000, [&] {
        ch::modify(s_d2);
        ch::sink(ch::parse_duration_ns(s_d2, 4).ns);
      }));

  mc::console("");
  mc::console("[posix layer]");
  mc::console("op                                        ns/op");
  mc::console("--------------------------------------------------");
  micron::time_t t = 1768000000;
  micron::posix::tm_t tm{};
  row("gmtime_r", measure_ps(1000000, [&] {
        ch::modify(t);
        micron::posix::gmtime_r(&t, &tm);
        ch::sink_ptr(&tm);
      }));
  micron::posix::gmtime_r(&t, &tm);
  row("timegm", measure_ps(1000000, [&] { ch::sink(static_cast<i64>(micron::posix::timegm(&tm))); }));
  row("strftime (\"%F %T\")", measure_ps(1000000, [&] { ch::sink(micron::posix::strftime(buf, sizeof(buf), "%F %T", &tm)); }));
  row("strptime (\"%Y-%m-%d %H:%M:%S\")",
      measure_ps(500000, [&] { ch::sink(micron::posix::strptime("2026-08-14 14:22:11", "%Y-%m-%d %H:%M:%S", &tm) != nullptr); }));
}

};      // namespace

int
main(void)
{
  ch::prepare_here();

  mc::console("=== micron::chrono benchmark ===");
  mc::console("pinned to cpu ", (long long)ch::current_cpu(), ", warmed, median of ", (long long)K_MEASUREMENTS, " runs");
  mc::console("counter: ", ch::counter_is_native ? "native" : "clock_gettime fallback",
              ",  invariant: ", ch::counter_traits().invariant ? "yes" : "no");
  mc::console("vDSO: ", ch::vdso::available() ? "resolved" : "unavailable");

  bench_clock_reads();
  bench_counter();
  bench_format();

  mc::console("");
  mc::console("=== done ===");
  return 0;
}
