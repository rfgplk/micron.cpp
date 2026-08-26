//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"

namespace integrate_bench
{

using timing_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
using cache_events = bbench::event_group<bbench::level1d, bbench::level1d_miss>;

inline constexpr u32 samples = 9;

struct counters {
  f64 cycles_per_unit{};
  f64 instructions_per_unit{};
  f64 ipc{};
  f64 branches_per_unit{};
  f64 branch_miss_percent{};
  f64 l1_loads_per_unit{};
  f64 l1_miss_percent{};
};

[[gnu::always_inline]] inline void
clobber(const void *pointer) noexcept
{
  asm volatile("" : : "r"(pointer) : "memory");
}

template<typename T>
[[gnu::always_inline]] inline void
consume(const T &value) noexcept
{
  clobber(&value);
}

inline f64
median(f64 (&values)[samples]) noexcept
{
  for ( u32 i = 1; i < samples; ++i ) {
    const f64 value = values[i];
    u32 j = i;
    while ( j != 0 && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  return values[samples / 2];
}

template<u64 Repetitions, u64 Units, typename Kernel>
[[nodiscard]] counters
measure(Kernel kernel) noexcept
{
  static_assert(Repetitions > 0 && Units > 0);
  for ( u32 warmup = 0; warmup < 5; ++warmup ) kernel();

  f64 cycles[samples]{}, instructions[samples]{}, ipc[samples]{}, branches[samples]{}, branch_misses[samples]{};
  f64 l1_loads[samples]{}, l1_misses[samples]{};
  constexpr f64 denominator = f64(Repetitions * Units);
  for ( u32 sample = 0; sample < samples; ++sample ) {
    timing_events timing{ bbench::quiet{} };
    timing.open();
    timing.begin();
    for ( u64 repetition = 0; repetition < Repetitions; ++repetition ) kernel();
    timing.end();
    const f64 cycle_count = f64(timing.get<bbench::hardware_cycles>().retrieve());
    const f64 instruction_count = f64(timing.get<bbench::hardware_instructions>().retrieve());
    const f64 branch_count = f64(timing.get<bbench::branches>().retrieve());
    const f64 miss_count = f64(timing.get<bbench::branch_misses>().retrieve());
    cycles[sample] = cycle_count / denominator;
    instructions[sample] = instruction_count / denominator;
    ipc[sample] = cycle_count == 0 ? 0 : instruction_count / cycle_count;
    branches[sample] = branch_count / denominator;
    branch_misses[sample] = branch_count == 0 ? 0 : f64(100) * miss_count / branch_count;

    cache_events cache{ bbench::quiet{} };
    cache.open();
    cache.begin();
    for ( u64 repetition = 0; repetition < Repetitions; ++repetition ) kernel();
    cache.end();
    const f64 load_count = f64(cache.get<bbench::level1d>().retrieve());
    const f64 load_misses = f64(cache.get<bbench::level1d_miss>().retrieve());
    l1_loads[sample] = load_count / denominator;
    l1_misses[sample] = load_count == 0 ? 0 : f64(100) * load_misses / load_count;
  }
  return counters{ median(cycles),        median(instructions), median(ipc),      median(branches),
                   median(branch_misses), median(l1_loads),     median(l1_misses) };
}

inline void
header() noexcept
{
  micron::io::println("suite,case,variant,type,size,cycles_per_unit,instructions_per_unit,ipc,branches_per_unit,branch_miss_pct,l1_loads_"
                      "per_unit,l1_miss_pct,true_error,reported_error,evaluations,accepted,rejected");
}

inline void
print(const char *suite, const char *name, const char *variant, const char *type, usize size, const counters &value, f64 true_error = 0,
      f64 reported_error = 0, usize evaluations = 0, usize accepted = 0, usize rejected = 0) noexcept
{
  micron::io::println(suite, ",", name, ",", variant, ",", type, ",", size, ",", value.cycles_per_unit, ",", value.instructions_per_unit,
                      ",", value.ipc, ",", value.branches_per_unit, ",", value.branch_miss_percent, ",", value.l1_loads_per_unit, ",",
                      value.l1_miss_percent, ",", true_error, ",", reported_error, ",", evaluations, ",", accepted, ",", rejected);
}

inline void
pin_cpu2() noexcept
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(2);
  (void)micron::posix::sched_setaffinity(0, sizeof(set), set);
}

};      // namespace integrate_bench
