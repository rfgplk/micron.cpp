//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Queue benchmark — every queue under src/queue/ is exercised against the
// same battery of operations and the same fixed workload, so the resulting
// table is a direct cross-implementation comparison rather than a per-impl
// micro-report.
//
//   queues under test:
//     queue (queue.hpp)             — mutable dynamic FIFO, single-threaded
//     conqueue (conqueue.hpp)       — mutex-guarded mutable FIFO
//     spsc_queue (spsc_queue.hpp)   — lock-free wait-free SPSC ring
//     disruptor (disruptor.hpp)     — LMAX-style SP + batched consumer
//     crossbeam (crossbeam.hpp)     — Vyukov cell-tag MPMC ring
//     immutable_queue (iqueue.hpp)  — Hood-Melville persistent queue
//
//   per (queue, op, N) cell the harness reports
//     cyc/op   IPC   bmiss%
//   medians across K_MEASUREMENTS samples; bbench 4-event group.
//
//   operations:
//     push           single-element enqueue
//     pop            single-element dequeue
//     push+pop       interleaved (steady-state ring)
//     push (batch)   buffered enqueue (where supported)
//     pop (batch)    buffered dequeue (where supported)
//     iterate        full begin()/end() walk; sums elements where iterable
//     copy-ctor      container-level copy/move
//     move-ctor      container-level move
//     clear          drain to empty

#if !defined(QUEUE_BENCH_EXTERNAL_PERF)
#include "../external/bbench/bench.hpp"
#endif

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/queue/conqueue.hpp"
#include "../src/queue/crossbeam.hpp"
#include "../src/queue/disruptor.hpp"
#include "../src/queue/iqueue.hpp"
#include "../src/queue/lambda_queue.hpp"
#include "../src/queue/queue.hpp"
#include "../src/queue/spsc_queue.hpp"
#include "../src/queue/static_mpmc.hpp"
#include "../src/std.hpp"

namespace
{

#if !defined(QUEUE_BENCH_EXTERNAL_PERF)
using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
#endif

constexpr u32 K_MEASUREMENTS = 7;
constexpr u64 WARMUP_REPS = 2;

const char *g_impl = nullptr;
const char *g_case = nullptr;
const char *g_state = "half";
u64 g_capacity = 0;
u64 g_batch = 64;
u64 g_capture = 16;
u32 g_cpu = 0;
u32 g_measurements = K_MEASUREMENTS;
u64 g_lambda_calls = 0;

bool
eq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

u64
number(const char *s) noexcept
{
  u64 value = 0;
  while ( *s >= '0' && *s <= '9' ) value = value * 10u + static_cast<u64>(*s++ - '0');
  return value;
}

bool
wants(const char *selected, const char *value) noexcept
{
  return selected == nullptr || eq(selected, value);
}

struct anomaly {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  const char *reason;
};

static anomaly g_anomalies[256];
static u32 g_anomaly_count = 0;

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

[[gnu::always_inline]] inline u64
val_u64(u64 i) noexcept
{
  return splitmix64(i + 1);
}

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline u64
bench_ticks() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
#elif defined(__micron_arch_arm64)
  u64 value;
  asm volatile("isb; mrs %0, cntvct_el0" : "=r"(value));
  return value;
#else
  // armv7-a: PMCCNTR is PL0-gated behind PMUSERENR.EN (default 0) -> SIGILL. Use the clock.
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
#endif
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
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
print_col_header()
{
  line h;
  h.s("op");
  h.pad_to(24, 0);
  h.s_lj_at("impl", 42);
  h.s_at("N", 52);
  h.s_at("cyc/call", 64);
  h.s_at("cyc/item", 76);
  h.s_at("IPC", 86);
  h.s_at("bmiss%", 96);
  micron::io::println(h.str());
  micron::io::println("------------------------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

struct row {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_call;
  f64 cyc_per_item;
  f64 ipc;
  f64 bmiss_rate;
  bool unstable;
  f64 mad;
  f64 relative_mad;
  f64 acceptance_threshold;
  u32 sample_count;
  f64 samples[K_MEASUREMENTS];
};

[[gnu::cold]] void
print_row(const row &r)
{
  if ( r.unstable ) {
    line ln;
    ln.s_lj_at(r.op, 24);
    ln.s_lj_at(r.impl, 42);
    ln.u_at(r.size, 52);
    ln.pad_to(56, 0);
    ln.s("(unstable)");
    micron::io::println(ln.str());
    return;
  }
  const fmt2 cpo = to_fmt2(r.cyc_per_call);
  const fmt2 ipc = to_fmt2(r.ipc);
  const fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.s_lj_at(r.op, 24);
  ln.s_lj_at(r.impl, 42);
  ln.u_at(r.size, 52);
  ln.f2_at(cpo, 64);
  ln.f2_at(to_fmt2(r.cyc_per_item), 76);
  ln.f2_at(ipc, 86);
  ln.f2_at(bm, 96);
  micron::io::println(ln.str());

  micron::io::print("  spread: MAD=");
  const fmt2 mad = to_fmt2(r.mad);
  micron::io::print(mad.whole, ".", mad.frac_x100 < 10 ? "0" : "", mad.frac_x100, " cyc/call; relative-MAD=");
  const fmt2 relative = to_fmt2(r.relative_mad);
  micron::io::print(relative.whole, ".", relative.frac_x100 < 10 ? "0" : "", relative.frac_x100, "% threshold=");
  const fmt2 threshold = to_fmt2(r.acceptance_threshold);
  micron::io::print(threshold.whole, ".", threshold.frac_x100 < 10 ? "0" : "", threshold.frac_x100, "% raw=");
  for ( u32 i = 0; i < r.sample_count; ++i ) {
    const fmt2 sample = to_fmt2(r.samples[i]);
    micron::io::print(i ? "," : "", sample.whole, ".", sample.frac_x100 < 10 ? "0" : "", sample.frac_x100);
  }
  micron::io::println("");

  if ( r.unstable ) return;
  const char *reason = nullptr;
  if ( r.bmiss_rate > 0.05 )
    reason = "bmiss%>5 (predictor stress)";
  else if ( r.ipc > 0 && r.ipc < 0.7 )
    reason = "IPC<0.7 (memory-bound)";
  else if ( r.cyc_per_call > 1000.0 && !(r.op[0] == 'i' && r.op[1] == 'n' && r.op[2] == 's' && r.op[3] == 'e')
            && !(r.op[0] == 'c' && r.op[1] == 'o' && r.op[2] == 'p') )
    reason = "cyc/op>1000 outside path-copy ops";

  if ( reason && g_anomaly_count < 256 ) {
    g_anomalies[g_anomaly_count++] = anomaly{ r.op, r.impl, r.size, r.cyc_per_call, r.ipc, r.bmiss_rate, reason };
  }
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

template<typename Setup, typename Kernel, typename Validate>
[[gnu::noinline]] row
measure(const char *op, const char *impl, u64 size, u64 calls_per_rep, u64 items_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel,
        Validate &&validate) noexcept
{
  try {
    for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
      setup();
      kernel();
      validate();
    }
  } catch ( ... ) {
    return row{ op, impl, size, 0.0, 0.0, 0.0, 0.0, true, 0.0, 0.0, 0.0, 0, {} };
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < g_measurements; ++m ) {
#if defined(QUEUE_BENCH_EXTERNAL_PERF)
    u64 cyc = 0;
    try {
      setup();
      const u64 begin = bench_ticks();
      for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
      const u64 end = bench_ticks();
      validate();
      cyc = end - begin;
    } catch ( ... ) {
      return row{ op, impl, size, 0.0, 0.0, 0.0, 0.0, true, 0.0, 0.0, 0.0, 0, {} };
    }
    const f64 total_calls = static_cast<f64>(reps_per_meas) * static_cast<f64>(calls_per_rep);
    cpo_samples[m] = total_calls > 0 ? static_cast<f64>(cyc) / total_calls : static_cast<f64>(cyc);
    ipc_samples[m] = 0.0;
    bm_samples[m] = 0.0;
#else
    c_events evs{ bbench::quiet{} };
    evs.open();
    try {
      setup();
      evs.begin();
      for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
      evs.end();
      validate();
    } catch ( ... ) {
      evs.end();
      return row{ op, impl, size, 0.0, 0.0, 0.0, 0.0, true, 0.0, 0.0, 0.0, 0, {} };
    }
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    const f64 total_calls = static_cast<f64>(reps_per_meas) * static_cast<f64>(calls_per_rep);
    cpo_samples[m] = total_calls > 0 ? static_cast<f64>(cyc) / total_calls : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
#endif
  }

  f64 raw[K_MEASUREMENTS];
  for ( u32 i = 0; i < g_measurements; ++i ) raw[i] = cpo_samples[i];
  const f64 call_median = median_f64(cpo_samples, g_measurements);
  f64 deviations[K_MEASUREMENTS];
  for ( u32 i = 0; i < g_measurements; ++i ) {
    const f64 delta = raw[i] - call_median;
    deviations[i] = delta < 0 ? -delta : delta;
  }
  const f64 mad = median_f64(deviations, g_measurements);
  const f64 relative_mad = call_median > 0 ? 100.0 * mad / call_median : 0.0;
  const f64 acceptance = 3.0 * relative_mad > 3.0 ? 3.0 * relative_mad : 3.0;
  const f64 per_item = items_per_rep ? call_median * static_cast<f64>(calls_per_rep) / static_cast<f64>(items_per_rep) : call_median;
  row result{ op,
              impl,
              size,
              call_median,
              per_item,
              median_f64(ipc_samples, g_measurements),
              median_f64(bm_samples, g_measurements),
              false,
              mad,
              relative_mad,
              acceptance,
              g_measurements,
              {} };
  for ( u32 i = 0; i < g_measurements; ++i ) result.samples[i] = raw[i];
  return result;
}

[[gnu::always_inline]] inline u64
reps_for(u64 ops_per_rep) noexcept
{
  constexpr u64 TARGET = 1ULL << 17;
  if ( ops_per_rep == 0 ) return 16;
  u64 r = TARGET / ops_per_rep;
  if ( r < 2 ) r = 2;
  if ( r > 256 ) r = 256;
  return r;
}

template<typename Q, typename Trait>
void
sweep_mutable_queue(const char *impl_tag, u64 N)
{
  Q q;
  u64 *input = new u64[N ? N : 1];
  u64 *output = new u64[N ? N : 1];
  u64 expected_sum = 0;
  for ( u64 i = 0; i < N; ++i ) {
    input[i] = val_u64(i);
    expected_sum += input[i];
  }

  if constexpr ( Trait::is_dynamic ) {
    if ( wants(g_case, "growth") ) {
      usize before = 0;
      auto setup = [&] {
        q = Q{};
        before = q.max_size();
      };
      auto kernel = [&] {
        for ( usize i = 0; i < N; ++i ) Trait::push(q, input[i]);
      };
      auto validate = [&] {
        if ( q.size() != N || (N > before && q.max_size() <= before) ) throw "dynamic growth validation";
      };
      print_row(measure("growth", impl_tag, N, N, N, 1, setup, kernel, validate));
      if ( g_case != nullptr ) {
        delete[] output;
        delete[] input;
        return;
      }
    }

    q.reserve(N);
    if ( wants(g_case, "compaction") ) {
      const usize count = N / 2;
      auto setup = [&] {
        Trait::clear(q);
        for ( usize i = 0; i < N; ++i ) Trait::push(q, input[i]);
        u64 value = 0;
        for ( usize i = 0; i < count; ++i ) Trait::try_pop(q, value);
      };
      auto kernel = [&] {
        for ( usize i = 0; i < count; ++i ) Trait::push(q, input[i]);
      };
      auto validate = [&] {
        if ( q.size() != N ) throw "dynamic compaction validation";
      };
      print_row(measure("compaction", impl_tag, N, count, count, 1, setup, kernel, validate));
      if ( g_case != nullptr ) {
        delete[] output;
        delete[] input;
        return;
      }
    }
  }

  if ( wants(g_case, "push") ) {
    auto setup = [&] { Trait::clear(q); };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, input[i]);
      clobber(&q);
    };
    auto validate = [&] {
      if ( q.size() != N ) throw "push row did not transfer N items";
    };
    print_row(measure("push", impl_tag, N, N, N, 1, setup, kernel, validate));
  }

  if ( wants(g_case, "pop") ) {
    auto setup = [&] {
      Trait::clear(q);
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, input[i]);
    };
    u64 acc = 0;
    auto kernel = [&] {
      acc = 0;
      for ( u64 i = 0; i < N; ++i ) acc += Trait::pop(q);
      sink_u64 += acc;
    };
    auto validate = [&] {
      if ( !q.empty() || acc != expected_sum ) throw "pop row FIFO/result validation";
    };
    print_row(measure("pop", impl_tag, N, N, N, 1, setup, kernel, validate));
  }

  if ( wants(g_case, "steady") ) {
    auto setup = [&] { Trait::clear(q); };
    u64 acc = 0;
    auto kernel = [&] {
      acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        Trait::push(q, input[i]);
        acc += Trait::pop(q);
      }
      sink_u64 += acc;
    };
    auto validate = [&] {
      if ( !q.empty() || acc != expected_sum ) throw "steady row result validation";
    };
    print_row(measure("push+pop", impl_tag, N, 2 * N, 2 * N, reps_for(2 * N), setup, kernel, validate));
  }

  if constexpr ( Trait::is_bounded ) {
    if ( wants(g_case, "mixed") ) {
      usize groups = N ? (131072u / N) : 1;
      if ( groups == 0 ) groups = 1;
      if ( groups > 1024 ) groups = 1024;
      Q *states = new Q[groups];
      usize successes = 0;
      auto setup = [&] {
        for ( usize g = 0; g < groups; ++g ) {
          Trait::clear(states[g]);
          for ( usize i = 0; i < N - 9; ++i ) Trait::push(states[g], input[i]);
        }
      };
      auto kernel = [&] {
        successes = 0;
        for ( usize g = 0; g < groups; ++g )
          for ( usize i = 0; i < 10; ++i ) successes += Trait::push(states[g], input[i]);
      };
      auto validate = [&] {
        if ( successes != groups * 9 ) throw "deterministic 90/10 push validation";
        for ( usize g = 0; g < groups; ++g )
          if ( states[g].size() != N ) throw "deterministic 90/10 queue state";
      };
      print_row(measure("mixed-90/10", impl_tag, N, groups * 10, groups * 9, 1, setup, kernel, validate));
      delete[] states;
    }

    if ( g_case != nullptr && (eq(g_case, "probe-push") || eq(g_case, "probe-pop")) ) {
      usize occupancy = N / 2;
      if ( eq(g_state, "empty") )
        occupancy = 0;
      else if ( eq(g_state, "one") )
        occupancy = 1;
      else if ( eq(g_state, "last") )
        occupancy = N - 1;
      else if ( eq(g_state, "full") )
        occupancy = N;
      usize groups = N ? (131072u / N) : 1;
      if ( groups == 0 ) groups = 1;
      if ( groups > 1024 ) groups = 1024;
      Q *states = new Q[groups];
      usize successes = 0;
      u64 value = 0;
      auto setup = [&] {
        for ( usize g = 0; g < groups; ++g ) {
          Trait::clear(states[g]);
          for ( usize i = 0; i < occupancy; ++i ) Trait::push(states[g], input[i]);
        }
      };
      const bool pushing = eq(g_case, "probe-push");
      auto kernel = [&] {
        successes = 0;
        if ( pushing ) {
          for ( usize g = 0; g < groups; ++g ) successes += Trait::push(states[g], input[0]);
        } else {
          for ( usize g = 0; g < groups; ++g ) successes += Trait::try_pop(states[g], value);
        }
      };
      const usize expected = pushing ? (occupancy < N ? groups : 0) : (occupancy ? groups : 0);
      auto validate = [&] {
        if ( successes != expected ) throw "boundary probe result validation";
      };
      print_row(measure(pushing ? "probe-push" : "probe-pop", impl_tag, N, groups, expected, 1, setup, kernel, validate));
      delete[] states;
    }
  }

  if constexpr ( Trait::has_batch ) {
    const u64 batch = g_batch;
    const u64 calls = (N + batch - 1) / batch;
    if ( wants(g_case, "batch-push") ) {
      auto setup = [&] { Trait::clear(q); };
      usize transferred = 0;
      auto kernel = [&] {
        transferred = 0;
        while ( transferred < N ) {
          const usize count = (N - transferred) < batch ? (N - transferred) : batch;
          transferred += Trait::push_batch(q, input + transferred, count);
        }
        clobber(&q);
      };
      auto validate = [&] {
        if ( transferred != N || q.size() != N ) throw "batch push validation";
      };
      print_row(measure("batch-push", impl_tag, N, calls, N, 1, setup, kernel, validate));
    }
    if ( wants(g_case, "batch-pop") ) {
      auto setup = [&] {
        Trait::clear(q);
        usize made = 0;
        while ( made < N ) {
          const usize count = (N - made) < batch ? (N - made) : batch;
          made += Trait::push_batch(q, input + made, count);
        }
      };
      usize consumed = 0;
      auto kernel = [&] {
        consumed = 0;
        while ( consumed < N ) {
          const usize count = (N - consumed) < batch ? (N - consumed) : batch;
          consumed += Trait::pop_batch(q, output + consumed, count);
        }
        sink_u64 += consumed;
      };
      auto validate = [&] {
        if ( consumed != N || !q.empty() ) throw "batch pop count validation";
        for ( usize i = 0; i < N; ++i )
          if ( output[i] != input[i] ) throw "batch pop FIFO validation";
      };
      print_row(measure("batch-pop", impl_tag, N, calls, N, 1, setup, kernel, validate));
    }
    if constexpr ( Trait::is_ring ) {
      if ( wants(g_case, "batch-wrap") ) {
        const usize count = batch < N ? batch : N;
        const usize advance = N - count / 2;
        usize transferred = 0;
        auto setup = [&] {
          Trait::clear(q);
          for ( usize i = 0; i < advance; ++i ) Trait::push(q, input[i]);
          u64 value = 0;
          for ( usize i = 0; i < advance; ++i ) Trait::try_pop(q, value);
        };
        auto kernel = [&] { transferred = Trait::push_batch(q, input, count); };
        auto validate = [&] {
          if ( transferred != count || Trait::pop_batch(q, output, count) != count ) throw "wrapped batch count validation";
          for ( usize i = 0; i < count; ++i )
            if ( output[i] != input[i] ) throw "wrapped batch FIFO validation";
        };
        print_row(measure("batch-wrap", impl_tag, N, 1, count, 1, setup, kernel, validate));
      }
    }
  }

  if constexpr ( Trait::has_iter ) {
    if ( wants(g_case, "iterate") ) {
      auto setup = [&] {
        Trait::clear(q);
        for ( u64 i = 0; i < N; ++i ) Trait::push(q, input[i]);
      };
      u64 sum = 0;
      auto kernel = [&] {
        sum = Trait::iterate_sum(q);
        sink_u64 += sum;
      };
      auto validate = [&] {
        if ( sum != expected_sum || q.size() != N ) throw "iterate validation";
      };
      print_row(measure("iterate", impl_tag, N, 1, N, reps_for(N), setup, kernel, validate));
    }
  }

  if ( wants(g_case, "clear") ) {
    auto setup = [&] {
      Trait::clear(q);
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, input[i]);
    };
    auto kernel = [&] {
      Trait::clear(q);
      clobber(&q);
    };
    auto validate = [&] {
      if ( !q.empty() ) throw "clear validation";
    };
    print_row(measure("clear", impl_tag, N, 1, N, 1, setup, kernel, validate));
  }

  delete[] output;
  delete[] input;
}

template<typename Q, typename Trait>
void
sweep_immutable_queue(const char *impl_tag, u64 N)
{
  u64 *input = new u64[N ? N : 1];
  u64 expected_sum = 0;
  for ( u64 i = 0; i < N; ++i ) {
    input[i] = val_u64(i);
    expected_sum += input[i];
  }

  if ( wants(g_case, "push") ) {
    Q base;
    auto setup = [&] { base = Q{}; };
    Q cur;
    auto kernel = [&] {
      cur = base;
      for ( u64 i = 0; i < N; ++i ) cur = Trait::push(cur, input[i]);
      clobber(&cur);
    };
    auto validate = [&] {
      if ( cur.size() != N || (N && (cur.front() != input[0] || cur.last() != input[N - 1])) ) throw "immutable build validation";
    };
    print_row(measure("push", impl_tag, N, N, N, 1, setup, kernel, validate));
  }

  Q filled;
  for ( u64 i = 0; i < N; ++i ) filled = Trait::push(filled, input[i]);

  if ( wants(g_case, "pop") ) {
    auto setup = [] { };
    u64 acc = 0;
    Q cur;
    auto kernel = [&] {
      cur = filled;
      acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        u64 v;
        cur = Trait::pop(cur, v);
        acc += v;
      }
      sink_u64 += acc;
    };
    auto validate = [&] {
      if ( !cur.empty() || acc != expected_sum ) throw "immutable drain validation";
    };
    print_row(measure("pop", impl_tag, N, N, N, 1, setup, kernel, validate));
  }

  if constexpr ( Trait::has_iter ) {
    if ( wants(g_case, "iterate") ) {
      auto setup = [] { };
      u64 sum = 0;
      auto kernel = [&] {
        sum = Trait::iterate_sum(filled);
        sink_u64 += sum;
      };
      auto validate = [&] {
        if ( sum != expected_sum ) throw "immutable traversal validation";
      };
      print_row(measure("iterate", impl_tag, N, 1, N, reps_for(N), setup, kernel, validate));
    }
  }

  if ( wants(g_case, "copy") ) {
    auto setup = [] { };
    const void *identity = nullptr;
    auto kernel = [&] {
      Q tmp(filled);
      identity = tmp.identity();
      clobber(&tmp);
    };
    auto validate = [&] {
      if ( identity != filled.identity() ) throw "immutable O(1) copy validation";
    };
    print_row(measure("copy-ctor", impl_tag, N, 1, N, reps_for(1), setup, kernel, validate));
  }

  delete[] input;
}

struct queue_trait_u64 {
  using queue_t = micron::queue<u64>;
  static constexpr bool is_dynamic = true;
  static constexpr bool is_bounded = false;
  static constexpr bool is_ring = false;
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = true;

  static bool
  push(queue_t &q, u64 v)
  {
    q.push(micron::move(v));
    return true;
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = q.last();
    q.pop();
    return v;
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    if ( q.empty() ) return false;
    v = pop(q);
    return true;
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }

  static u64
  iterate_sum(const queue_t &q)
  {
    u64 acc = 0;
    for ( auto it = q.cbegin(); it != q.cend(); ++it ) acc += *it;
    return acc;
  }
};

struct conqueue_trait_u64 {
  using queue_t = micron::conqueue<u64>;
  static constexpr bool is_dynamic = true;
  static constexpr bool is_bounded = false;
  static constexpr bool is_ring = false;
  static constexpr bool has_batch = true;
  static constexpr bool has_iter = true;

  static bool
  push(queue_t &q, u64 v)
  {
    q.push(v);
    return true;
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = 0;
    q.pop(v);
    return v;
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    return q.pop(v);
  }

  static usize
  push_batch(queue_t &q, const u64 *items, usize n)
  {
    return q.push_batch(items, n);
  }

  static usize
  pop_batch(queue_t &q, u64 *items, usize n)
  {
    return q.pop_batch(items, n);
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }

  static u64
  iterate_sum(queue_t &q)
  {
    u64 acc = 0;
    for ( auto it = q.begin(); it != q.end(); ++it ) acc += *it;
    return acc;
  }
};

template<usize Cap> struct spsc_trait_u64 {
  using queue_t = micron::spsc_queue<u64, Cap>;
  static constexpr bool is_dynamic = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_ring = true;
  static constexpr bool has_batch = true;
  static constexpr bool has_iter = false;

  static bool
  push(queue_t &q, u64 v)
  {
    return q.push(v);
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    return q.pop(v);
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = 0;
    q.pop(v);
    return v;
  }

  static usize
  push_batch(queue_t &q, const u64 *items, usize n)
  {
    return q.push_batch(items, n);
  }

  static usize
  pop_batch(queue_t &q, u64 *items, usize n)
  {
    return q.pop_batch(items, n);
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }
};

template<usize Cap> struct disruptor_trait_u64 {
  using queue_t = micron::disruptor<u64, Cap>;
  static constexpr bool is_dynamic = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_ring = true;
  static constexpr bool has_batch = true;
  static constexpr bool has_iter = false;

  static queue_t
  make_empty()
  {
    return queue_t{};
  }

  static bool
  push(queue_t &q, u64 v)
  {
    return q.publish(v);
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    return q.consume(v);
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = 0;
    q.consume(v);
    return v;
  }

  static usize
  push_batch(queue_t &q, const u64 *items, usize n)
  {
    return q.try_publish_batch(items, n);
  }

  static usize
  pop_batch(queue_t &q, u64 *items, usize n)
  {
    return q.try_consume_batch(items, n);
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }
};

template<usize Cap> struct static_mpmc_trait_u64 {
  using queue_t = micron::static_mpmc<u64, Cap>;
  static constexpr bool is_dynamic = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_ring = false;
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = false;

  static bool
  push(queue_t &q, u64 v)
  {
    return q.push(v);
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    return q.pop(v);
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = 0;
    q.pop(v);
    return v;
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }
};

template<usize Cap> struct crossbeam_trait_u64 {
  using queue_t = micron::crossbeam<u64, Cap>;
  static constexpr bool is_dynamic = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_ring = false;
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = false;

  static bool
  push(queue_t &q, u64 v)
  {
    return q.push(v);
  }

  static bool
  try_pop(queue_t &q, u64 &v)
  {
    return q.pop(v);
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = 0;
    q.pop(v);
    return v;
  }

  static void
  clear(queue_t &q)
  {
    q.clear();
  }
};

struct iqueue_trait_u64 {
  using queue_t = micron::immutable_queue<u64>;
  static constexpr bool has_iter = true;

  static queue_t
  push(const queue_t &q, u64 v)
  {
    return q.push(v);
  }

  static queue_t
  pop(const queue_t &q, u64 &out)
  {
    out = q.front();
    return q.pop();
  }

  static u64
  iterate_sum(const queue_t &q)
  {
    u64 acc = 0;
    q.for_each([&](const u64 &v) { acc += v; });
    return acc;
  }
};

struct lambda_capture0 {
  explicit lambda_capture0(u64 = 0) noexcept { }

  void
  operator()() const noexcept
  {
    ++g_lambda_calls;
  }
};

struct lambda_capture16 {
  u64 value;
  u64 check;

  explicit lambda_capture16(u64 v = 0) noexcept : value(v), check(~v) { }

  void
  operator()() const noexcept
  {
    g_lambda_calls += (check == ~value);
    sink_u64 += value;
  }
};

struct lambda_capture64 {
  u64 value;
  byte pad[56];

  explicit lambda_capture64(u64 v = 0) noexcept : value(v), pad{} { }

  void
  operator()() const noexcept
  {
    ++g_lambda_calls;
    sink_u64 += value + pad[0];
  }
};

static_assert(sizeof(lambda_capture0) == 1);
static_assert(sizeof(lambda_capture16) == 16);
static_assert(sizeof(lambda_capture64) == 64);

template<usize Cap, typename Task>
void
sweep_lambda()
{
  using queue_t = micron::lambda_queue<Cap>;
  queue_t *queue = new queue_t;
  if ( wants(g_case, "push") ) {
    auto setup = [&] { queue->clear(); };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) queue->push(Task(i));
    };
    auto validate = [&] {
      if ( queue->size() != Cap ) throw "lambda push validation";
    };
    print_row(measure("push", "lambda", Cap, Cap, Cap, 1, setup, kernel, validate));
  }
  if ( wants(g_case, "execute") ) {
    auto setup = [&] {
      queue->clear();
      g_lambda_calls = 0;
      for ( usize i = 0; i < Cap; ++i ) queue->push(Task(i));
    };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) queue->execute();
    };
    auto validate = [&] {
      if ( !queue->empty() || g_lambda_calls != Cap ) throw "lambda execute validation";
    };
    print_row(measure("execute", "lambda", Cap, Cap, Cap, 1, setup, kernel, validate));
  }
  if ( wants(g_case, "pop") ) {
    auto setup = [&] {
      queue->clear();
      for ( usize i = 0; i < Cap; ++i ) queue->push(Task(i));
    };
    usize popped = 0;
    auto kernel = [&] {
      popped = 0;
      while ( auto *task = queue->pop() ) {
        task->~node_base_t();
        ++popped;
      }
    };
    auto validate = [&] {
      if ( popped != Cap || !queue->empty() ) throw "lambda pop/destruction validation";
    };
    print_row(measure("pop+destroy", "lambda", Cap, Cap, Cap, 1, setup, kernel, validate));
  }
  if ( wants(g_case, "clear") ) {
    auto setup = [&] {
      queue->clear();
      for ( usize i = 0; i < Cap; ++i ) queue->push(Task(i));
    };
    auto kernel = [&] { queue->clear(); };
    auto validate = [&] {
      if ( !queue->empty() ) throw "lambda clear validation";
    };
    print_row(measure("clear", "lambda", Cap, 1, Cap, 1, setup, kernel, validate));
  }
  delete queue;
}

template<usize Cap>
void
run_capacity()
{
  if ( wants(g_impl, "queue") ) {
    print_header("queue<u64>");
    sweep_mutable_queue<typename queue_trait_u64::queue_t, queue_trait_u64>("queue", Cap);
  }
  if ( wants(g_impl, "conqueue") ) {
    print_header("conqueue<u64>");
    sweep_mutable_queue<typename conqueue_trait_u64::queue_t, conqueue_trait_u64>("conqueue", Cap);
  }
  if ( wants(g_impl, "spsc") ) {
    print_header("spsc_queue<u64>");
    using trait = spsc_trait_u64<Cap>;
    sweep_mutable_queue<typename trait::queue_t, trait>("spsc", Cap);
  }
  if ( wants(g_impl, "disruptor") ) {
    print_header("disruptor<u64>");
    using trait = disruptor_trait_u64<Cap>;
    sweep_mutable_queue<typename trait::queue_t, trait>("disruptor", Cap);
  }
  if ( wants(g_impl, "crossbeam") ) {
    print_header("crossbeam<u64>");
    using trait = crossbeam_trait_u64<Cap>;
    sweep_mutable_queue<typename trait::queue_t, trait>("crossbeam", Cap);
  }
  if ( wants(g_impl, "static_mpmc") ) {
    print_header("static_mpmc<u64>");
    using trait = static_mpmc_trait_u64<Cap>;
    sweep_mutable_queue<typename trait::queue_t, trait>("static_mpmc", Cap);
  }
  if ( wants(g_impl, "immutable") ) {
    print_header("immutable_queue<u64>");
    sweep_immutable_queue<typename iqueue_trait_u64::queue_t, iqueue_trait_u64>("immutable", Cap);
  }
  if ( wants(g_impl, "lambda") ) {
    print_header("lambda_queue");
    if ( g_capture == 0 )
      sweep_lambda<Cap, lambda_capture0>();
    else if ( g_capture == 16 )
      sweep_lambda<Cap, lambda_capture16>();
    else
      sweep_lambda<Cap, lambda_capture64>();
  }
}

bool
parse_args(int argc, char **argv) noexcept
{
  for ( int i = 1; i < argc; ++i ) {
    if ( i + 1 == argc ) return false;
    const char *key = argv[i++];
    const char *value = argv[i];
    if ( eq(key, "--impl") )
      g_impl = eq(value, "all") ? nullptr : value;
    else if ( eq(key, "--case") )
      g_case = eq(value, "all") ? nullptr : value;
    else if ( eq(key, "--state") )
      g_state = value;
    else if ( eq(key, "--capacity") )
      g_capacity = eq(value, "all") ? 0 : number(value);
    else if ( eq(key, "--batch") )
      g_batch = number(value);
    else if ( eq(key, "--capture") )
      g_capture = number(value);
    else if ( eq(key, "--cpu") )
      g_cpu = static_cast<u32>(number(value));
    else if ( eq(key, "--reps") )
      g_measurements = static_cast<u32>(number(value));
    else if ( eq(key, "--payload") ) {
      if ( number(value) != 8 ) return false;
    } else
      return false;
  }
  const bool capacity_ok = g_capacity == 0 || g_capacity == 64 || g_capacity == 512 || g_capacity == 4096 || g_capacity == 32768;
  const bool batch_ok = g_batch == 1 || g_batch == 4 || g_batch == 16 || g_batch == 64 || g_batch == 256;
  const bool capture_ok = g_capture == 0 || g_capture == 16 || g_capture == 64;
  const bool state_ok = eq(g_state, "empty") || eq(g_state, "one") || eq(g_state, "half") || eq(g_state, "last") || eq(g_state, "full");
  return capacity_ok && batch_ok && capture_ok && state_ok && g_measurements > 0 && g_measurements <= K_MEASUREMENTS;
}

}      // namespace

int
main(int argc, char **argv)
{
  if ( !parse_args(argc, argv) ) {
    micron::io::println(
        "usage: queues_bench --impl queue|conqueue|spsc|disruptor|crossbeam|static_mpmc|immutable|lambda|all"
        " --case "
        "push|pop|execute|steady|mixed|probe-push|probe-pop|batch-push|batch-pop|batch-wrap|iterate|clear|copy|growth|compaction|all"
        " --state empty|one|half|last|full --capacity 64|512|4096|32768|all"
        " --payload 8 --batch 1|4|16|64|256 --capture 0|16|64 --cpu N --reps 1..7");
    return 1;
  }
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(g_cpu);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  micron::io::println("=== micron queues benchmark ===");
#if defined(QUEUE_BENCH_EXTERNAL_PERF)
  micron::io::println("timing: TSC; PMU events supplied by the external perf group");
#else
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
#endif
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", g_measurements, " measurements per cell (median); batch=", g_batch,
                      "; cpu=", g_cpu);

  if ( g_capacity == 0 || g_capacity == 64 ) run_capacity<64>();
  if ( g_capacity == 0 || g_capacity == 512 ) run_capacity<512>();
  if ( g_capacity == 0 || g_capacity == 4096 ) run_capacity<4096>();
  if ( g_capacity == 0 || g_capacity == 32768 ) run_capacity<32768>();

  micron::io::println("");
  micron::io::println("[anomalies] (rows flagged during run)");
  if ( g_anomaly_count == 0 ) {
    micron::io::println("  (none)");
  } else {
    line head;
    head.s("  ");
    head.s_at("op", 26);
    head.s_lj_at("impl", 44);
    head.s_at("N", 54);
    head.s_at("cyc/op", 66);
    head.s_at("IPC", 76);
    head.s_at("bmiss%", 86);
    head.s("  ");
    head.s("reason");
    micron::io::println(head.str());
    for ( u32 i = 0; i < g_anomaly_count; ++i ) {
      const auto &a = g_anomalies[i];
      const fmt2 cpo = to_fmt2(a.cyc_per_op);
      const fmt2 ipc = to_fmt2(a.ipc);
      const fmt2 bm = to_fmt2(a.bmiss_rate * 100.0);
      line ln;
      ln.s("  ");
      ln.s_lj_at(a.op, 26);
      ln.s_lj_at(a.impl, 44);
      ln.u_at(a.size, 54);
      ln.f2_at(cpo, 66);
      ln.f2_at(ipc, 76);
      ln.f2_at(bm, 86);
      ln.s("  ");
      ln.s(a.reason);
      micron::io::println(ln.str());
    }
  }
  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
