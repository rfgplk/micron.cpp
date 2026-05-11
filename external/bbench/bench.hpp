//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../src/concepts.hpp"
#include "../../src/memory/actions.hpp"
#include "../../src/proc.hpp"
#include "../../src/string/string.hpp"
#include "../../src/type_traits.hpp"
#include "../../src/types.hpp"
#include "../../src/vector.hpp"

#include "clock.hpp"
#include "events.hpp"
#include "funcs.hpp"
#include "options.hpp"
#include "process.hpp"

namespace bbench
{

// d1: cycles, instructions, branch + branch-miss, cache, software, L1d/L1i/LLC, BPU, node
using event_group_d1 = event_group<hardware_cycles, hardware_instructions, cache_misses, branches, branch_misses, total_cycles, cpu_time,
                                   context_switches, proc_migrations, level1d, level1t, llcache, cache_node, bpu>;

// d2: d1 + dTLB/iTLB access + miss + page-faults + stalled-cycles + bus-cycles + L1d_miss + LLC_miss
using event_group_d2 = event_group<hardware_cycles, hardware_instructions, cache_misses, branches, branch_misses, total_cycles, cpu_time,
                                   context_switches, proc_migrations, level1d, level1t, llcache, cache_node, bpu, page_faults, bus_cycles_e,
                                   stalled_front, stalled_back, dtlb_access, dtlb_miss, itlb_access, itlb_miss, level1d_miss, llcache_miss>;

// d3: d2 + L1d prefetch + L1i miss + minor/major/alignment/emulation faults
using event_group_d3
    = event_group<hardware_cycles, hardware_instructions, cache_misses, branches, branch_misses, total_cycles, cpu_time, context_switches,
                  proc_migrations, level1d, level1t, llcache, cache_node, bpu, page_faults, bus_cycles_e, stalled_front, stalled_back,
                  dtlb_access, dtlb_miss, itlb_access, itlb_miss, level1d_miss, llcache_miss, level1t_miss, l1d_prefetch, l1d_prefetch_miss,
                  minor_faults, major_faults, alignment_faults, emulation_faults>;

// alias for the old bbench code
using full_event_group = event_group_d1;

namespace __impl
{

template <typename G, typename E> struct group_has : micron::false_type {
};

template <typename E, typename... Cs> struct group_has<event_group<Cs...>, E> : micron::bool_constant<(micron::is_same_v<E, Cs> || ...)> {
};

template <typename G, typename E> inline constexpr bool group_has_v = group_has<G, E>::value;

template <time_resolution R, class G>
inline benchmark_t
collect(const micron::string &name, time_clock &cl, G &gr)
{
  benchmark_t b{};
  b.name = name;
  b.time = cl.template elapsed<R>();

  if constexpr ( group_has_v<G, hardware_cycles> ) b.cycles = (long long)gr.template get<hardware_cycles>().retrieve();
  if constexpr ( group_has_v<G, hardware_instructions> ) b.instructions = (long long)gr.template get<hardware_instructions>().retrieve();
  if constexpr ( group_has_v<G, cache_misses> ) b.cache_misses = (long long)gr.template get<cache_misses>().retrieve();
  if constexpr ( group_has_v<G, branches> ) b.total_branches = (long long)gr.template get<branches>().retrieve();
  if constexpr ( group_has_v<G, branch_misses> ) b.branch_misses = (long long)gr.template get<branch_misses>().retrieve();
  if constexpr ( group_has_v<G, total_cycles> ) b.total_cycles = (long long)gr.template get<total_cycles>().retrieve();
  if constexpr ( group_has_v<G, cpu_time> ) b.cpu_time = (long long)gr.template get<cpu_time>().retrieve();
  if constexpr ( group_has_v<G, context_switches> ) b.context_switches = (long long)gr.template get<context_switches>().retrieve();
  if constexpr ( group_has_v<G, proc_migrations> ) b.migrations = (long long)gr.template get<proc_migrations>().retrieve();
  if constexpr ( group_has_v<G, level1d> ) b.l1_cache = (long long)gr.template get<level1d>().retrieve();
  if constexpr ( group_has_v<G, level1t> ) b.l1t_cache = (long long)gr.template get<level1t>().retrieve();
  if constexpr ( group_has_v<G, llcache> ) b.ll_cache = (long long)gr.template get<llcache>().retrieve();
  if constexpr ( group_has_v<G, cache_node> ) b.access = (long long)gr.template get<cache_node>().retrieve();
  if constexpr ( group_has_v<G, bpu> ) b.bpu = (long long)gr.template get<bpu>().retrieve();

  // d2+ events
  if constexpr ( group_has_v<G, page_faults> ) b.page_faults = (long long)gr.template get<page_faults>().retrieve();
  if constexpr ( group_has_v<G, bus_cycles_e> ) b.bus_cycles = (long long)gr.template get<bus_cycles_e>().retrieve();
  if constexpr ( group_has_v<G, stalled_front> ) b.stalled_front = (long long)gr.template get<stalled_front>().retrieve();
  if constexpr ( group_has_v<G, stalled_back> ) b.stalled_back = (long long)gr.template get<stalled_back>().retrieve();
  if constexpr ( group_has_v<G, dtlb_access> ) b.dtlb_access = (long long)gr.template get<dtlb_access>().retrieve();
  if constexpr ( group_has_v<G, dtlb_miss> ) b.dtlb_miss = (long long)gr.template get<dtlb_miss>().retrieve();
  if constexpr ( group_has_v<G, itlb_access> ) b.itlb_access = (long long)gr.template get<itlb_access>().retrieve();
  if constexpr ( group_has_v<G, itlb_miss> ) b.itlb_miss = (long long)gr.template get<itlb_miss>().retrieve();
  if constexpr ( group_has_v<G, level1d_miss> ) b.l1d_miss = (long long)gr.template get<level1d_miss>().retrieve();
  if constexpr ( group_has_v<G, level1t_miss> ) b.l1t_miss = (long long)gr.template get<level1t_miss>().retrieve();
  if constexpr ( group_has_v<G, llcache_miss> ) b.llcache_miss = (long long)gr.template get<llcache_miss>().retrieve();

  // d3+ events
  if constexpr ( group_has_v<G, l1d_prefetch> ) b.l1d_prefetch = (long long)gr.template get<l1d_prefetch>().retrieve();
  if constexpr ( group_has_v<G, l1d_prefetch_miss> ) b.l1d_prefetch_miss = (long long)gr.template get<l1d_prefetch_miss>().retrieve();
  if constexpr ( group_has_v<G, minor_faults> ) b.minor_faults = (long long)gr.template get<minor_faults>().retrieve();
  if constexpr ( group_has_v<G, major_faults> ) b.major_faults = (long long)gr.template get<major_faults>().retrieve();
  if constexpr ( group_has_v<G, alignment_faults> ) b.alignment_faults = (long long)gr.template get<alignment_faults>().retrieve();
  if constexpr ( group_has_v<G, emulation_faults> ) b.emulation_faults = (long long)gr.template get<emulation_faults>().retrieve();

  return b;
}
};     // namespace __impl

template <time_resolution R = time_resolution::us, class G = event_group_d1, typename F, typename... Args>
inline benchmark_t
benchmark(F func, Args &&...args)
{
  time_clock cl;
  G gr{ quiet{} };
  gr.open();
  cl.begin();
  gr.begin();
  func(micron::forward<Args>(args)...);
  cl.end();
  gr.end();
  return __impl::collect<R>(micron::string{}, cl, gr);
}

template <time_resolution R = time_resolution::us, class G = event_group_d1, typename F, typename... Args>
inline benchmark_t
benchmark(const micron::string &_name, F func, Args &&...args)
{
  time_clock cl;
  G gr{ quiet{} };
  gr.open();
  cl.begin();
  gr.begin();
  func(micron::forward<Args>(args)...);
  cl.end();
  gr.end();
  return __impl::collect<R>(_name, cl, gr);
}

// retained for source-compat
template <time_resolution R = time_resolution::us, class G = event_group_d1, typename F, typename... Args>
inline benchmark_t
benchmark_batch(F func, Args &&...args)
{
  return benchmark<R, G>(micron::forward<F>(func), micron::forward<Args>(args)...);
}

template <class G = event_group_d1, typename... A>
inline benchmark_t
benchmark_bin(const char *s, A... args)
{
  time_clock cl;
  G gr{ quiet{} };
  int pid = process<false>(s, args...);
  gr.reopen(pid);
  cl.begin();
  gr.begin();
  micron::waitpid(pid, nullptr, 0);
  cl.end();
  gr.end();
  return __impl::collect<time_resolution::us>(micron::string{ s }, cl, gr);
}

template <class G = event_group_d1, micron::is_string T, micron::is_string... A>
inline benchmark_t
benchmark_bin(const T &s, A... args)
{
  time_clock cl;
  G gr{ quiet{} };
  int pid = process<false>(s, args...);
  gr.reopen(pid);
  cl.begin();
  gr.begin();
  micron::waitpid(pid, nullptr, 0);
  cl.end();
  gr.end();
  return __impl::collect<time_resolution::us>(micron::string{ s.c_str() }, cl, gr);
}

namespace __impl
{

inline void
__sleep_ms(u32 ms)
{
  if ( ms == 0 ) return;
  micron::timespec_t req{};
  req.tv_sec = ms / 1000;
  req.tv_nsec = static_cast<long>((ms % 1000) * 1'000'000);
  micron::nanosleep(req);
}

inline u64
__now_ms(void)
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return static_cast<u64>(t.tv_sec) * 1000ull + static_cast<u64>(t.tv_nsec) / 1'000'000ull;
}

inline void
__wait_with_timeout(int pid, u32 timeout_ms)
{
  if ( timeout_ms == 0 ) {
    micron::waitpid(pid, nullptr, 0);
    return;
  }
  u64 deadline = __now_ms() + timeout_ms;
  int status = 0;
  for ( ;; ) {
    int r = micron::waitpid(pid, &status, micron::wnohang);
    if ( r == pid ) return;     // exited
    if ( __now_ms() >= deadline ) {
      micron::posix::kill(pid, static_cast<int>(micron::signal::terminate));
      u64 hard = __now_ms() + 50;
      while ( __now_ms() < hard ) {
        if ( micron::waitpid(pid, &status, micron::wnohang) == pid ) return;
        __sleep_ms(2);
      }
      micron::posix::kill(pid, static_cast<int>(micron::signal::kill9));
      micron::waitpid(pid, &status, 0);
      return;
    }
    __sleep_ms(1);
  }
}

template <class G>
inline benchmark_t
__bench_bin_with_opts(const char *s, const benchmark_opts &opts)
{
  time_clock cl;
  G gr{ quiet{} };
  gr.set_inherit(opts.inherit);
  gr.set_pinned(opts.pinned);
  gr.set_enable_on_exec(true);

  if ( opts.pre ) process<true>(opts.pre);
  int pid = process_attach(s, [&](int child_pid) { gr.reopen(child_pid); });
  if ( opts.delay_ms > 0 ) __sleep_ms(opts.delay_ms);
  cl.begin();
  __wait_with_timeout(pid, opts.timeout_ms);
  cl.end();
  gr.end();
  if ( opts.post ) process<true>(opts.post);
  return __impl::collect<time_resolution::us>(micron::string{ s }, cl, gr);
}
};     // namespace __impl

inline benchmark_t
benchmark_bin(const char *s, const benchmark_opts &opts)
{
  switch ( opts.detail ) {
  case 2 :
    return __impl::__bench_bin_with_opts<event_group_d2>(s, opts);
  case 3 :
    return __impl::__bench_bin_with_opts<event_group_d3>(s, opts);
  default :
    return __impl::__bench_bin_with_opts<event_group_d1>(s, opts);
  }
}

// dynamic (-e)
struct dynamic_result_t {
  micron::string name;
  double time;

  // (event-pretty-name, scaled-value, errno-from-open)
  struct entry {
    const char *name;
    long long value;
    int err;
  };

  micron::vector<entry> rows;
};

inline dynamic_result_t
benchmark_bin_dynamic(const char *s, const micron::vector<event_def> &defs, const benchmark_opts &opts)
{
  dynamic_result_t out;
  out.name = micron::string{ s };

  dynamic_event_group gr(defs, opts.excl_kernel);
  gr.set_inherit(opts.inherit);
  gr.set_pinned(opts.pinned);
  gr.set_enable_on_exec(true);

  time_clock cl;
  if ( opts.pre ) process<true>(opts.pre);
  int pid = process_attach(s, [&](int child_pid) { gr.reopen(child_pid); });
  if ( opts.delay_ms > 0 ) __impl::__sleep_ms(opts.delay_ms);
  cl.begin();
  __impl::__wait_with_timeout(pid, opts.timeout_ms);
  cl.end();
  gr.end();
  if ( opts.post ) process<true>(opts.post);

  out.time = cl.template elapsed<time_resolution::us>();
  out.rows.reserve(defs.size());
  gr.for_each([&](const char *n, long long v, int err) { out.rows.push_back({ n, v, err }); });
  return out;
}

template <class C = hardware_cycles, typename F, typename... Args>
inline long long
cpu_bench(F func, Args &&...args)
{
  C cl;
  cl.begin();
  func(micron::forward<Args>(args)...);
  cl.end();
  return cl.retrieve();
}

template <class C = hardware_cycles, micron::is_string T, micron::is_string... A>
inline long long
cpu_bench_bin(const T &s, A... a)
{
  C cl;
  int pid = process<false>(s, a...);
  cl.reopen(pid);
  cl.begin();
  micron::waitpid(pid, nullptr, 0);
  cl.end();
  return cl.retrieve();
}

template <class C = hardware_cycles, typename... A>
inline long long
cpu_bench_bin(const char *s, A... a)
{
  C cl;
  int pid = process<false>(s, a...);
  cl.reopen(pid);
  cl.begin();
  micron::waitpid(pid, nullptr, 0);
  cl.end();
  return cl.retrieve();
}

template <time_resolution R = time_resolution::milliseconds, typename F, typename... Args>
inline double
bench(F func, Args &&...args)
{
  time_clock cl;
  cl.begin();
  func(micron::forward<Args>(args)...);
  cl.end();
  return cl.template elapsed<R>();
}

template <time_resolution R = time_resolution::milliseconds, micron::is_string T, micron::is_string... A>
inline double
bench_bin(const T &s, A... a)
{
  time_clock_mono cl;
  cl.begin();
  process<true>(s, a...);
  cl.end();
  return cl.template elapsed<R>();
}

template <time_resolution R = time_resolution::milliseconds, typename... A>
inline double
bench_bin(const char *s, A... a)
{
  time_clock_mono cl;
  cl.begin();
  process<true>(s, a...);
  cl.end();
  return cl.template elapsed<R>();
}

template <time_resolution R = time_resolution::milliseconds, typename... F>
inline micron::vector<double>
bench(F... funcs)
{
  micron::vector<double> results;
  results.reserve(sizeof...(F));
  time_clock cl;
  auto call = [&](auto func) {
    cl.begin();
    func();
    cl.end();
    results.push_back(cl.template elapsed<R>());
  };
  (call(funcs), ...);
  return results;
}

// time N functions per func
template <usize N, time_resolution R = time_resolution::milliseconds, typename F, typename... Args>
inline micron::vector<double>
bench_repeat(F func, Args... args)
{
  micron::vector<double> results;
  results.reserve(N);
  time_clock cl;
  auto call = [&]() {
    cl.begin();
    func(args...);
    cl.end();
    results.push_back(cl.template elapsed<R>());
  };
  for ( usize i = 0; i < N; i++ ) call();
  return results;
}

};     // namespace bbench
