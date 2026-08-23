//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../external/bbench/bench.hpp"

#include <micron/atomic/atomic.hpp>
#include <micron/io/console.hpp>
#include <micron/io/stdout.hpp>
#include <micron/linux/sys/sched.hpp>
#include <micron/maps/conmap.hpp>
#include <micron/thread/thread.hpp>

namespace
{

using events_t = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 threads = 4;
constexpr u32 measurements = 5;
constexpr u32 keys_per_thread = 8192;
constexpr u32 update_keys = 64;
constexpr u32 update_ops = 1u << 17;

struct counters {
  u64 cycles;
  u64 instructions;
  u64 branches;
  u64 branch_misses;
};

struct row {
  const char *name;
  u64 cycles_x100;
  u64 ipc_x100;
  u64 bmiss_x100;
};

static micron::atomic_token<u64> sink{ 0 };

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

[[gnu::always_inline]] inline usize
stripe_id(micron::hash64_t hash) noexcept
{
  u64 x = static_cast<u64>(hash);
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return static_cast<usize>(x ^ (x >> 31)) & (micron::conmap<u64, u64>::stripe_count() - 1u);
}

u64
median(u64 *values) noexcept
{
  for ( u32 i = 1; i < measurements; ++i ) {
    const u64 value = values[i];
    u32 j = i;
    while ( j && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  return values[measurements / 2];
}

template<typename Worker>
counters
parallel_counters(Worker worker)
{
  micron::atomic_token<u32> ready{ 0 };
  micron::atomic_token<u32> start{ 0 };
  counters per_thread[threads]{};
  micron::__thread_pointer<micron::auto_thread<>> workers[threads];

  for ( u32 t = 0; t < threads; ++t ) {
    workers[t] = micron::solo::spawn([&, t]() {
      micron::posix::cpu_set_t set;
      set.cpu_zero();
      set.cpu_set(t);
      micron::posix::sched_setaffinity(0, sizeof(set), set);

      events_t events{ bbench::quiet{} };
      events.open();
      ready.fetch_add(1, micron::memory_order_release);
      while ( start.get(micron::memory_order_acquire) == 0 ) __cpu_pause();

      events.begin();
      worker(t);
      events.end();

      per_thread[t].cycles = static_cast<u64>(events.get<bbench::hardware_cycles>().retrieve());
      per_thread[t].instructions = static_cast<u64>(events.get<bbench::hardware_instructions>().retrieve());
      per_thread[t].branches = static_cast<u64>(events.get<bbench::branches>().retrieve());
      per_thread[t].branch_misses = static_cast<u64>(events.get<bbench::branch_misses>().retrieve());
    });
  }

  while ( ready.get(micron::memory_order_acquire) != threads ) __cpu_pause();
  start.store(1, micron::memory_order_release);
  for ( u32 t = 0; t < threads; ++t ) micron::solo::join(workers[t]);

  counters total{};
  for ( const counters &c : per_thread ) {
    total.cycles += c.cycles;
    total.instructions += c.instructions;
    total.branches += c.branches;
    total.branch_misses += c.branch_misses;
  }
  return total;
}

template<typename Setup, typename Worker>
row
measure(const char *name, u64 operation_count, Setup setup, Worker worker)
{
  u64 cycles[measurements];
  u64 ipc[measurements];
  u64 bmiss[measurements];

  setup();
  parallel_counters(worker);

  for ( u32 i = 0; i < measurements; ++i ) {
    setup();
    const counters c = parallel_counters(worker);
    cycles[i] = operation_count ? (c.cycles * 100u) / operation_count : 0;
    ipc[i] = c.cycles ? (c.instructions * 100u) / c.cycles : 0;
    bmiss[i] = c.branches ? (c.branch_misses * 10000u) / c.branches : 0;
  }
  return { name, median(cycles), median(ipc), median(bmiss) };
}

void
print_fixed(u64 value) noexcept
{
  micron::io::print(value / 100u, ".");
  const u64 fraction = value % 100u;
  if ( fraction < 10 ) micron::io::print("0");
  micron::io::print(fraction);
}

void
print_row(const row &r)
{
  micron::io::print(r.name, ": cyc/op=");
  print_fixed(r.cycles_x100);
  micron::io::print(" IPC=");
  print_fixed(r.ipc_x100);
  micron::io::print(" bmiss%=");
  print_fixed(r.bmiss_x100);
  micron::io::println("");
}

};      // namespace

int
main()
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  alignas(64) u64 keys[threads][keys_per_thread];
  for ( u32 t = 0; t < threads; ++t )
    for ( u32 i = 0; i < keys_per_thread; ++i ) keys[t][i] = splitmix64(static_cast<u64>(t) * keys_per_thread + i);

  micron::conmap<u64, u64> map(threads * keys_per_thread * 2u);
  auto clear = [&] { map.clear(); };
  auto insert = [&](u32 t) {
    for ( u32 i = 0; i < keys_per_thread; ++i ) map.insert(keys[t][i], u64{ i });
  };
  print_row(measure("parallel insert", threads * keys_per_thread, clear, insert));

  clear();
  for ( u32 t = 0; t < threads; ++t ) insert(t);
  auto find_setup = [] { };
  auto find = [&](u32 t) {
    u64 total = 0;
    for ( u32 repeat = 0; repeat < 16; ++repeat ) {
      for ( u32 i = 0; i < keys_per_thread; ++i ) {
        u64 value = 0;
        if ( map.find(keys[t][i], value) ) total += value;
      }
    }
    sink.fetch_add(total, micron::memory_order_relaxed);
  };
  print_row(measure("parallel find", threads * keys_per_thread * 16u, find_setup, find));

  u64 stripe_keys[threads][update_keys];
  u32 found[threads]{};
  for ( u64 candidate = 1;; ++candidate ) {
    const u64 key = splitmix64(candidate);
    const usize sid = stripe_id(micron::hash<micron::hash64_t>(key));
    if ( sid < threads && found[sid] < update_keys ) stripe_keys[sid][found[sid]++] = key;
    bool complete = true;
    for ( u32 t = 0; t < threads; ++t ) complete = complete && found[t] == update_keys;
    if ( complete ) break;
  }

  auto disjoint_setup = [&] {
    map.clear();
    for ( u32 t = 0; t < threads; ++t )
      for ( u32 i = 0; i < update_keys; ++i ) map.insert(stripe_keys[t][i], u64{ 0 });
  };
  auto disjoint_update = [&](u32 t) {
    for ( u32 i = 0; i < update_ops; ++i ) map.update(stripe_keys[t][i & (update_keys - 1u)], [](u64 &value) { ++value; });
  };
  print_row(measure("disjoint-stripe update", threads * update_ops, disjoint_setup, disjoint_update));

  auto shared_setup = [&] {
    map.clear();
    for ( u32 i = 0; i < update_keys; ++i ) map.insert(stripe_keys[0][i], u64{ 0 });
  };
  auto shared_update = [&](u32) {
    for ( u32 i = 0; i < update_ops; ++i ) map.update(stripe_keys[0][i & (update_keys - 1u)], [](u64 &value) { ++value; });
  };
  print_row(measure("one-stripe update", threads * update_ops, shared_setup, shared_update));

  micron::io::println("anti-DCE sink: ", sink.get(micron::memory_order_relaxed));
  return 0;
}
