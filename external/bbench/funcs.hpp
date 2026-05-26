
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <micron/string/string.hpp>

#include "clock.hpp"
#include "perf.hpp"

namespace bbench
{

struct benchmark_t {
  micron::string name;
  double time;
  long long cycles;
  long long instructions;
  long long cache_misses;
  long long total_branches;
  long long branch_misses;
  long long total_cycles;
  long long cpu_time;
  long long context_switches;
  long long migrations;

  long long l1_cache;
  long long l1t_cache;
  long long ll_cache;
  long long access;
  long long bpu;

  long long page_faults;
  long long minor_faults;
  long long major_faults;
  long long bus_cycles;
  long long stalled_front;
  long long stalled_back;
  long long alignment_faults;
  long long emulation_faults;
  long long dtlb_access;
  long long dtlb_miss;
  long long itlb_access;
  long long itlb_miss;
  long long l1d_miss;
  long long l1t_miss;
  long long llcache_miss;
  long long l1d_prefetch;
  long long l1d_prefetch_miss;

  unsigned long long time_enabled_ns;
  unsigned long long time_running_ns;
};

auto
per_op(double x, long long a)
{
  return x / static_cast<double>(a);
}

auto
per_cycle(const benchmark_t &b)
{
  return static_cast<double>((double)b.cycles / static_cast<double>(b.time));
}

auto
per_instruction(const benchmark_t &b)
{
  return static_cast<double>((double)b.instructions / static_cast<double>(b.time));
}

auto
miss_percent(const benchmark_t &b)
{
  return static_cast<double>((double)b.branch_misses / (double)b.total_branches);
}

auto
cycles_per_instruction(const benchmark_t &b)
{
  return static_cast<double>((double)b.cycles / (double)b.instructions);
}

template<typename T>
void
perf_init_leader(T &&)
{
  hardware_cycles a;
  a.start_as_leader();
}

};      // namespace bbench
