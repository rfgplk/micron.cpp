// Shared harness for the per-class string benches (sstring/string/istring/rope).
//
// Cloned from benches/string_bench.cpp so all four files emit the same
// columns and the same statistical contract:
//
//   - perf_event group: cycles + instructions + branches + branch-misses
//     (matches the abcmalloc / cmemory benches)
//   - K_MEASUREMENTS=7 median samples per cell, WARMUP_REPS=4
//   - 4 MiB target work per measurement, capped 32 .. 2^18 reps
//   - sched_setaffinity to CPU 0 (callers invoke `pin_cpu0()`)
//   - alignas(64) static corpora; sink_u64 defeats DCE of bool/usize-returning
//     ops; clobber() defeats DCE of mutating ops
//
// The `line` formatter and column layout match string_bench.cpp:70-168
// byte-for-byte; tags emitted in the "impl" column are repurposed by each
// caller (e.g. "early", "late", "miss", "k=8", "flat", "tree", "share").
//
// One-time include per TU. No state, no globals beyond corpora + sink. The
// file is named with a leading underscore so it does not look like a bench
// when listed with `ls benches/`.

#pragma once

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/std.hpp"

namespace mb
{

constexpr u32 K_MEASUREMENTS = 7;
constexpr u64 WARMUP_REPS = 4;
constexpr u64 TARGET_BYTES_PER_MEAS = 1ULL << 22;
constexpr u64 MIN_REPS = 32;
constexpr u64 MAX_REPS = 1ULL << 18;

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

struct row {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
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

[[gnu::cold]] inline void
print_header()
{
  line h;
  h.s_at("size(B)", 10);
  h.pad_to(12, 0);
  h.s_lj_at("op", 40);
  h.s_lj_at("impl", 50);
  h.s_at("cyc/op", 62);
  h.s_at("IPC", 72);
  h.s_at("bmiss%", 82);
  micron::io::println(h.str());
  micron::io::println("----------------------------------------------------------------------------------");
}

[[gnu::cold]] inline void
print_row(const row &r)
{
  fmt2 cpo = to_fmt2(r.cyc_per_op);
  fmt2 ipc = to_fmt2(r.ipc);
  fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.u_at(r.size, 10);
  ln.pad_to(12, 0);
  ln.s_lj_at(r.op, 40);
  ln.s_lj_at(r.impl, 50);
  ln.f2_at(cpo, 62);
  ln.f2_at(ipc, 72);
  ln.f2_at(bm, 82);
  micron::io::println(ln.str());
}

inline volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
sink_bool(bool b) noexcept
{
  sink_u64 += static_cast<u64>(b);
}

[[gnu::always_inline]] inline void
sink_size(usize v) noexcept
{
  sink_u64 += static_cast<u64>(v);
}

[[gnu::always_inline]] inline void
sink_char(char c) noexcept
{
  sink_u64 += static_cast<u64>(c);
}

[[gnu::always_inline]] inline void
sink_ptr(const void *p) noexcept
{
  sink_u64 += reinterpret_cast<u64>(p);
}

inline f64
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

template<typename Fn>
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

template<typename Fn>
row
bench_one(const char *op, const char *impl, u64 size, u64 bytes_per_op, Fn &&fn, u64 max_reps_override = MAX_REPS) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; i++ ) fn();

  u64 reps = TARGET_BYTES_PER_MEAS / (bytes_per_op == 0 ? 1 : bytes_per_op);
  if ( reps < MIN_REPS ) reps = MIN_REPS;
  if ( reps > max_reps_override ) reps = max_reps_override;

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; m++ ) {
    sample s = measure_once(fn, reps);
    cpo_samples[m] = static_cast<f64>(s.cyc) / static_cast<f64>(reps);
    ipc_samples[m] = s.cyc > 0 ? static_cast<f64>(s.inst) / static_cast<f64>(s.cyc) : 0.0;
    bm_samples[m] = s.br > 0 ? static_cast<f64>(s.bm) / static_cast<f64>(s.br) : 0.0;
  }
  return row{
    op, impl, size, median_f64(cpo_samples, K_MEASUREMENTS), median_f64(ipc_samples, K_MEASUREMENTS), median_f64(bm_samples, K_MEASUREMENTS)
  };
}

constexpr u64 CORPUS_BYTES = 1ULL << 14;

alignas(64) inline char g_lower_ascii[CORPUS_BYTES + 1];
alignas(64) inline char g_upper_ascii[CORPUS_BYTES + 1];
alignas(64) inline char g_mixed_ascii[CORPUS_BYTES + 1];
alignas(64) inline char g_padded_ws[CORPUS_BYTES + 1];

inline void
init_corpus()
{
  for ( u64 i = 0; i < CORPUS_BYTES; ++i ) {
    g_lower_ascii[i] = 'a' + static_cast<char>(i % 26);
    g_upper_ascii[i] = 'A' + static_cast<char>(i % 26);
    char m = (i & 1) ? ('A' + static_cast<char>(i % 26)) : ('a' + static_cast<char>(i % 26));
    g_mixed_ascii[i] = m;
    if ( i < 32 || i >= CORPUS_BYTES - 32 )
      g_padded_ws[i] = ' ';
    else
      g_padded_ws[i] = m;
  }
  g_lower_ascii[CORPUS_BYTES] = 0;
  g_upper_ascii[CORPUS_BYTES] = 0;
  g_mixed_ascii[CORPUS_BYTES] = 0;
  g_padded_ws[CORPUS_BYTES] = 0;
}

inline void
pin_cpu0()
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);
}

[[gnu::cold]] inline void
print_preamble(const char *title)
{
  micron::io::println("=== ", title, " ===");
  micron::io::println("warmup ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " median samples per cell");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("");
}

[[gnu::cold]] inline void
print_epilogue()
{
  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
}

}      // namespace mb
