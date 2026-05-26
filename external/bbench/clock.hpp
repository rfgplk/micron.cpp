
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "perf.hpp"

#include <micron/memory/actions.hpp>
#include <micron/queue.hpp>
#include <micron/tuple.hpp>
#include <micron/type_traits.hpp>

namespace bbench
{

enum class time_resolution { seconds, sec, deciseconds, ds, milliseconds, ms, microseconds, us, nanoseconds, ns };

template<class C> class clock: public C
{

public:
  ~clock() { }

  clock() : C() { }

  inline __attribute__((always_inline)) auto
  begin()
  {
    return C::start_get();
  }

  inline __attribute__((always_inline)) auto
  end()
  {
    return C::stop_get();
  }

  template<time_resolution R>
  auto
  elapsed() -> double
  {
    if constexpr ( R == time_resolution::seconds or R == time_resolution::sec ) {
      return C::read();
    } else if constexpr ( R == time_resolution::deciseconds or R == time_resolution::ds ) {
      return C::read_ds();
    } else if constexpr ( R == time_resolution::milliseconds or R == time_resolution::ms ) {
      return C::read_ms();
    } else if constexpr ( R == time_resolution::microseconds or R == time_resolution::us ) {
      return C::read_us();
    } else if constexpr ( R == time_resolution::nanoseconds or R == time_resolution::ns ) {
      return C::read_ns();
    }
  }
};

template<typename C = system_clock<system_clocks::monotonic>> class stopwatch: public C
{
  micron::timespec_t start;

  micron::timespec_t last;
  micron::queue<micron::timespec_t> laps;

public:
  ~stopwatch() { }

  stopwatch() : C() { }

  inline void
  begin()
  {
    start = C::start_get();
  }

  inline void
  end()
  {
    last = C::stop_get();
    laps.push(last);
  }

  inline void
  lap()
  {
    last = C::stop_get();
    laps.push(last);
  }

  auto
  elapsed() -> double
  {
    if ( laps.empty() ) return -1.0f;
    return C::read_ms(last, start);
  }

  void operator()(void) = delete;

  template<typename F, typename... Args>
  auto
  operator()(F f, Args &&...args)
  {
    C::start();
    f(micron::forward<Args>(args)...);
    C::stop();
    return C::read();
  }
};

struct quiet {
};

template<class C = kernel_clock<time_userland, kernel_clock_types::hardware>, class Z = options::hardware,
         Z Y = options::hardware::total_inst>
class event: public C
{

public:
  ~event() { }

  event() : C(Y) { C::open(); }

  event(const quiet &) : C(Y) { }

  event(const event &) = delete;
  event(event &&o) = default;

  template<typename T>
  inline __attribute__((always_inline)) void
  attach(T &&t)
  {
    C::attach(micron::forward<T>(t));
  }

  inline __attribute__((always_inline)) void
  reopen(pid_t pid)
  {
    C::reopen(pid);
  }

  inline __attribute__((always_inline)) void
  open(void)
  {
    C::open();
  }

  inline __attribute__((always_inline)) auto
  begin_as_leader()
  {
    return C::start_as_leader();
  }

  inline __attribute__((always_inline)) auto
  end_as_leader()
  {
    return C::stop_as_leader();
  }

  inline __attribute__((always_inline)) void
  begin()
  {
    C::start();
  }

  inline __attribute__((always_inline)) void
  end()
  {
    C::stop();
  }

  inline void
  lap()
  {
    C::stop();
  }

  inline __attribute__((always_inline)) auto
  retrieve() -> long long
  {
    return C::read();
  }

  void operator()(void) = delete;

  template<typename F, typename... Args>
  auto
  operator()(F f, Args &&...args)
  {
    C::start();
    f(micron::forward<Args>(args)...);
    C::stop();
    return C::read();
  }
};

template<class... C> class event_group
{
  micron::tuple<C...> members;

  template<usize... Ts>
  void
  __impl_begin_all(micron::index_sequence<Ts...>)
  {
    auto call_begin = [](auto &m) { m.begin(); };
    (call_begin(micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_open_all(micron::index_sequence<Ts...>)
  {
    auto call_end = [](auto &m) { m.open(); };
    (call_end(micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_end_all(micron::index_sequence<Ts...>)
  {
    auto call_end = [](auto &m) { m.end(); };
    (call_end(micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_reopen_all(int pid, micron::index_sequence<Ts...>)
  {
    auto call_reopen = [](int _pid, auto &m) { m.reopen(_pid); };
    (call_reopen(pid, micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_set_inherit_all(bool on, micron::index_sequence<Ts...>)
  {
    auto f = [on](auto &m) { m.set_inherit(on); };
    (f(micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_set_pinned_all(bool on, micron::index_sequence<Ts...>)
  {
    auto f = [on](auto &m) { m.set_pinned(on); };
    (f(micron::get<Ts>(members)), ...);
  }

  template<usize... Ts>
  void
  __impl_set_eoe_all(bool on, micron::index_sequence<Ts...>)
  {
    auto f = [on](auto &m) { m.set_enable_on_exec(on); };
    (f(micron::get<Ts>(members)), ...);
  }

public:
  event_group(void) : members(micron::move(C{})...) { }

  event_group(C... args) : members(C(args)...) { }

  template<typename S> event_group(S arg) : members(C(arg)...) { }

  template<typename T>
  auto &
  get()
  {
    return micron::get<T>(members);
  }

  void
  open()
  {
    __impl_open_all(micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  begin()
  {
    __impl_begin_all(micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  end()
  {
    __impl_end_all(micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  reopen(int pid)
  {
    __impl_reopen_all(pid, micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  set_inherit(bool on)
  {
    __impl_set_inherit_all(on, micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  set_pinned(bool on)
  {
    __impl_set_pinned_all(on, micron::make_index_sequence<sizeof...(C)>{});
  }

  void
  set_enable_on_exec(bool on)
  {
    __impl_set_eoe_all(on, micron::make_index_sequence<sizeof...(C)>{});
  }
};

using time_clock = clock<system_clock<system_clocks::realtime>>;
using time_clock_mono = clock<system_clock<system_clocks::monotonic>>;
using boot_time = clock<system_clock<system_clocks::since_boot>>;
using stopwatch_rt = stopwatch<system_clock<system_clocks::realtime>>;

using hardware_cycles = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::cpu_cycles>;
using hardware_instructions
    = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_inst>;
using cache_misses = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::cache_misses>;
using branches = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::branches>;
using branch_misses = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::branch_misses>;
using total_cycles = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_cycles>;
using u_hardware_cycles
    = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::cpu_cycles>;
using u_hardware_instructions
    = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_inst>;
using u_cache_misses = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::cache_misses>;
using u_branches = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::branches>;
using u_branch_misses
    = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::branch_misses>;
using u_total_cycles = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_cycles>;

using k_hardware_cycles
    = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::cpu_cycles>;
using k_hardware_instructions
    = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_inst>;
using k_cache_misses
    = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::cache_misses>;
using k_branches = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::branches>;
using k_branch_misses
    = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::branch_misses>;
using k_total_cycles
    = event<kernel_clock<time_kernelland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_cycles>;

using a_hardware_cycles
    = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::cpu_cycles>;
using a_hardware_instructions
    = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_inst>;
using a_cache_misses
    = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::cache_misses>;
using a_branches = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::branches>;
using a_branch_misses
    = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::branch_misses>;
using a_total_cycles
    = event<kernel_clock<time_everyland, kernel_clock_types::hardware>, options::hardware, options::hardware::total_cycles>;

using cpu_time = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::cpu_clock>;
using context_switches = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::cntx_swtch>;
using proc_migrations
    = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::cpu_migrations>;

using level1d = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1d>;
using level1t = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1t>;
using llcache = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::last_level>;
using cache_node = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::local_access>;
using bpu = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::branch>;

using bus_cycles_e = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::bus_cycles>;
using stalled_front = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::stalled_init>;
using stalled_back
    = event<kernel_clock<time_userland, kernel_clock_types::hardware>, options::hardware, options::hardware::stalled_retirement>;

using page_faults = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::page_faults>;
using minor_faults = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::minor_page_flt>;
using major_faults = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::major_page_flt>;
using alignment_faults
    = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::alignment_faults>;
using emulation_faults
    = event<kernel_clock<time_userland, kernel_clock_types::software>, options::software, options::software::emulation_faults>;

using level1d_miss = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1d_miss>;
using level1t_miss = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1t_miss>;
using llcache_miss = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::last_level_miss>;
using dtlb_access = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::data_tlb>;
using dtlb_miss = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::data_tlb_miss>;
using itlb_access = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::instr_tlb>;
using itlb_miss = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::instr_tlb_miss>;
using l1d_prefetch = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1d_prefetch>;
using l1d_prefetch_miss
    = event<kernel_clock<time_userland, kernel_clock_types::cache>, options::cache, options::cache::level1d_prefetch_miss>;

};      // namespace bbench
