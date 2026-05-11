
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../src/memory/cmemory.hpp"
#include "../../src/types.hpp"

#include "funcs.hpp"

// pseudo perf stat -M metrics for benchmark_t
// -> ipc
// -> branch_miss_rate
// -> cache_miss_rate
// -> frontend_stall_rate
// -> backend_stall_rate
// -> ghz
// -> inst_per_ns
// -> dtlb_miss_rate, itlb_miss_rate, l1d_miss_rate, llc_miss_rate

namespace bbench::metric
{

inline double
__safe_div(long long num, long long den)
{
  if ( den == 0 ) return 0.0;
  return static_cast<double>(num) / static_cast<double>(den);
}

inline double
ipc(const benchmark_t &b)
{
  return __safe_div(b.instructions, b.cycles);
}

inline double
cpi(const benchmark_t &b)
{
  return __safe_div(b.cycles, b.instructions);
}

inline double
branch_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.branch_misses, b.total_branches);
}

inline double
cache_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.cache_misses, b.l1_cache);     // cache-misses / L1-loads-as-proxy
}

inline double
frontend_stall_rate(const benchmark_t &b)
{
  return __safe_div(b.stalled_front, b.cycles);
}

inline double
backend_stall_rate(const benchmark_t &b)
{
  return __safe_div(b.stalled_back, b.cycles);
}

inline double
l1d_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.l1d_miss, b.l1_cache);
}

inline double
l1i_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.l1t_miss, b.l1t_cache);
}

inline double
llc_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.llcache_miss, b.ll_cache);
}

inline double
dtlb_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.dtlb_miss, b.dtlb_access);
}

inline double
itlb_miss_rate(const benchmark_t &b)
{
  return __safe_div(b.itlb_miss, b.itlb_access);
}

inline double
ghz(const benchmark_t &b)
{
  // time is in us
  return __safe_div(b.cycles, static_cast<long long>(b.time * 1000.0));
}

struct named_metric {
  const char *name;
  double (*fn)(const benchmark_t &);
};

inline constexpr named_metric known_metrics[] = {
  { "ipc", &ipc },
  { "cpi", &cpi },
  { "branch-miss-rate", &branch_miss_rate },
  { "cache-miss-rate", &cache_miss_rate },
  { "frontend-stall-rate", &frontend_stall_rate },
  { "backend-stall-rate", &backend_stall_rate },
  { "l1d-miss-rate", &l1d_miss_rate },
  { "l1i-miss-rate", &l1i_miss_rate },
  { "llc-miss-rate", &llc_miss_rate },
  { "dtlb-miss-rate", &dtlb_miss_rate },
  { "itlb-miss-rate", &itlb_miss_rate },
  { "ghz", &ghz },
};

inline const named_metric *
lookup_metric(const char *name)
{
  if ( !name ) return nullptr;
  for ( const auto &m : known_metrics ) {
    if ( micron::strcmp(m.name, name) == 0 ) return &m;
  }
  return nullptr;
}

};     // namespace bbench::metric
