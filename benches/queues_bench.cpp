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

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/queue/conqueue.hpp"
#include "../src/queue/crossbeam.hpp"
#include "../src/queue/disruptor.hpp"
#include "../src/queue/iqueue.hpp"
#include "../src/queue/queue.hpp"
#include "../src/queue/spsc_queue.hpp"
#include "../src/std.hpp"

namespace
{

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 3;
constexpr u64 WARMUP_REPS = 1;

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
  h.s_at("cyc/op", 64);
  h.s_at("IPC", 74);
  h.s_at("bmiss%", 84);
  micron::io::println(h.str());
  micron::io::println("-----------------------------------------------------------------------------------");
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
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  bool unstable;
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
  const fmt2 cpo = to_fmt2(r.cyc_per_op);
  const fmt2 ipc = to_fmt2(r.ipc);
  const fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.s_lj_at(r.op, 24);
  ln.s_lj_at(r.impl, 42);
  ln.u_at(r.size, 52);
  ln.f2_at(cpo, 64);
  ln.f2_at(ipc, 74);
  ln.f2_at(bm, 84);
  micron::io::println(ln.str());

  if ( r.unstable ) return;
  const char *reason = nullptr;
  if ( r.bmiss_rate > 0.05 )
    reason = "bmiss%>5 (predictor stress)";
  else if ( r.ipc > 0 && r.ipc < 0.7 )
    reason = "IPC<0.7 (memory-bound)";
  else if ( r.cyc_per_op > 1000.0 && !(r.op[0] == 'i' && r.op[1] == 'n' && r.op[2] == 's' && r.op[3] == 'e')
            && !(r.op[0] == 'c' && r.op[1] == 'o' && r.op[2] == 'p') )
    reason = "cyc/op>1000 outside path-copy ops";

  if ( reason && g_anomaly_count < 256 ) {
    g_anomalies[g_anomaly_count++] = anomaly{ r.op, r.impl, r.size, r.cyc_per_op, r.ipc, r.bmiss_rate, reason };
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

template<typename Setup, typename Kernel>
[[gnu::noinline]] row
measure(const char *op, const char *impl, u64 size, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{
  try {
    for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
      setup();
      kernel();
    }
  } catch ( ... ) {
    return row{ op, impl, size, 0.0, 0.0, 0.0, true };
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    c_events evs{ bbench::quiet{} };
    evs.open();
    try {
      setup();
      evs.begin();
      for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
      evs.end();
    } catch ( ... ) {
      evs.end();
      return row{ op, impl, size, 0.0, 0.0, 0.0, true };
    }
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    cpo_samples[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return row{ op,
              impl,
              size,
              median_f64(cpo_samples, K_MEASUREMENTS),
              median_f64(ipc_samples, K_MEASUREMENTS),
              median_f64(bm_samples, K_MEASUREMENTS),
              false };
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

  {
    auto setup = [&] { Trait::clear(q); };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, val_u64(i));
      Trait::clear(q);
      clobber(&q);
    };
    print_row(measure("push", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [&] {
      Trait::clear(q);
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) acc += Trait::pop(q);
      sink_u64 += acc;

      for ( u64 i = 0; i < N; ++i ) Trait::push(q, val_u64(i));
    };
    print_row(measure("pop", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [&] { Trait::clear(q); };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        Trait::push(q, val_u64(i));
        acc += Trait::pop(q);
      }
      sink_u64 += acc;
    };
    print_row(measure("push+pop", impl_tag, N, 2 * N, reps_for(2 * N), setup, kernel));
  }

  if constexpr ( Trait::has_batch ) {
    u64 *buf = new u64[N];
    for ( u64 i = 0; i < N; ++i ) buf[i] = val_u64(i);
    {
      auto setup = [&] { Trait::clear(q); };
      auto kernel = [&] {
        Trait::push_batch(q, buf, N);
        Trait::clear(q);
        clobber(&q);
      };
      print_row(measure("push (batch)", impl_tag, N, N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] {
        Trait::clear(q);
        Trait::push_batch(q, buf, N);
      };
      auto kernel = [&] {
        u64 dump[64];
        u64 consumed = 0;
        while ( consumed < N ) {
          u64 want = (N - consumed) < 64 ? (N - consumed) : 64;
          consumed += Trait::pop_batch(q, dump, want);
        }
        sink_u64 += consumed;

        Trait::push_batch(q, buf, N);
      };
      print_row(measure("pop (batch)", impl_tag, N, N, reps_for(N), setup, kernel));
    }
    delete[] buf;
  }

  if constexpr ( Trait::has_iter ) {
    auto setup = [&] {
      Trait::clear(q);
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, val_u64(i));
    };
    auto kernel = [&] { sink_u64 += Trait::iterate_sum(q); };
    print_row(measure("iterate", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [&] {
      Trait::clear(q);
      for ( u64 i = 0; i < N; ++i ) Trait::push(q, val_u64(i));
    };
    auto kernel = [&] {
      Trait::clear(q);
      clobber(&q);
    };
    print_row(measure("clear (cyc/call)", impl_tag, N, 1, reps_for(N), setup, kernel));
  }
}

template<typename Q, typename Trait>
void
sweep_immutable_queue(const char *impl_tag, u64 N)
{

  {
    Q base;
    auto setup = [&] { base = Q{}; };
    auto kernel = [&] {
      Q cur = base;
      for ( u64 i = 0; i < N; ++i ) cur = Trait::push(cur, val_u64(i));
      clobber(&cur);
    };
    print_row(measure("push (build)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  Q filled;
  for ( u64 i = 0; i < N; ++i ) filled = Trait::push(filled, val_u64(i));

  {
    auto setup = [] { };
    auto kernel = [&] {
      Q cur = filled;
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        u64 v;
        cur = Trait::pop(cur, v);
        acc += v;
      }
      sink_u64 += acc;
    };
    print_row(measure("pop (drain)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  if constexpr ( Trait::has_iter ) {
    auto setup = [] { };
    auto kernel = [&] { sink_u64 += Trait::iterate_sum(filled); };
    print_row(measure("iterate", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      Q tmp(filled);
      clobber(&tmp);
    };
    print_row(measure("copy-ctor", impl_tag, N, 1, reps_for(1), setup, kernel));
  }
}

struct queue_trait_u64 {
  using queue_t = micron::queue<u64>;
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = true;

  static void
  push(queue_t &q, u64 v)
  {
    q.push(micron::move(v));
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = q.front();
    q.pop();
    return v;
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
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = true;

  static void
  push(queue_t &q, u64 v)
  {
    q.push(v);
  }

  static u64
  pop(queue_t &q)
  {
    u64 v = q.front();
    q.pop();
    return v;
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
  static constexpr bool has_batch = true;
  static constexpr bool has_iter = false;

  static void
  push(queue_t &q, u64 v)
  {
    q.push(v);
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
  static constexpr bool has_batch = true;
  static constexpr bool has_iter = false;

  static queue_t
  make_empty()
  {
    return queue_t{};
  }

  static void
  push(queue_t &q, u64 v)
  {
    q.publish(v);
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

template<usize Cap> struct crossbeam_trait_u64 {
  using queue_t = micron::crossbeam<u64, Cap>;
  static constexpr bool has_batch = false;
  static constexpr bool has_iter = false;

  static void
  push(queue_t &q, u64 v)
  {
    q.push(v);
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

}      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  micron::io::println("=== micron queues benchmark ===");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell (median)");

  constexpr u64 SIZES[] = { 64, 256, 1024 };

  print_header("queue<u64>");
  for ( u64 N : SIZES ) sweep_mutable_queue<typename queue_trait_u64::queue_t, queue_trait_u64>("queue", N);

  print_header("conqueue<u64>");
  for ( u64 N : SIZES ) sweep_mutable_queue<typename conqueue_trait_u64::queue_t, conqueue_trait_u64>("conqueue", N);

  print_header("spsc_queue<u64, 2048>");
  using spsc_2k = spsc_trait_u64<2048>;
  for ( u64 N : SIZES ) sweep_mutable_queue<typename spsc_2k::queue_t, spsc_2k>("spsc-2k", N);

  print_header("disruptor<u64, 2048>");
  using dis_2k = disruptor_trait_u64<2048>;
  for ( u64 N : SIZES ) sweep_mutable_queue<typename dis_2k::queue_t, dis_2k>("disruptor", N);

  print_header("crossbeam<u64, 2048>");
  using xb_2k = crossbeam_trait_u64<2048>;
  for ( u64 N : SIZES ) sweep_mutable_queue<typename xb_2k::queue_t, xb_2k>("crossbeam", N);

  print_header("immutable_queue<u64>");
  for ( u64 N : SIZES ) sweep_immutable_queue<typename iqueue_trait_u64::queue_t, iqueue_trait_u64>("iqueue", N);

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
