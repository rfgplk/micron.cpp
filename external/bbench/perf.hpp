
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <linux/perf_event.h>

#include <micron/bits/__exceptions.hpp>
#include <micron/chrono.hpp>
#include <micron/concepts.hpp>
#include <micron/errno.hpp>
#include <micron/except.hpp>
#include <micron/linux/io.hpp>
#include <micron/linux/sys/ioctl.hpp>
#include <micron/memory/actions.hpp>
#include <micron/memory/cmemory.hpp>
#include <micron/syscall.hpp>
#include <micron/type_traits.hpp>
#include <micron/types.hpp>

static inline long
__pe_call(long r)
{
  if ( r < 0 ) {
    errno = static_cast<i32>(-r);
    return -1;
  }
  return r;
}

static long
perf_event(struct perf_event_attr &event, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
  return __pe_call(micron::syscall(SYS_perf_event_open, &event, pid, cpu, group_fd, flags));
};

static long
perf_event_this(struct perf_event_attr &event)
{
  return __pe_call(micron::syscall(SYS_perf_event_open, &event, 0, -1, -1, 0));
};

static long
perf_event_attach(int fd, struct perf_event_attr &event)
{
  return __pe_call(micron::syscall(SYS_perf_event_open, &event, 0, -1, fd, 0));
};

static long
perf_event_pid(struct perf_event_attr &event, pid_t pid)
{
  return __pe_call(micron::syscall(SYS_perf_event_open, &event, pid, -1, -1, 0));
};

namespace bbench
{

enum class kernel_clock_types : u64 {
  hardware = PERF_TYPE_HARDWARE,
  software = PERF_TYPE_SOFTWARE,
  tracepoint = PERF_TYPE_TRACEPOINT,
  cache = PERF_TYPE_HW_CACHE
};

namespace options
{

enum class hardware : u64 {
  cpu_cycles = PERF_COUNT_HW_CPU_CYCLES,
  total_inst = PERF_COUNT_HW_INSTRUCTIONS,
  cache_misses = PERF_COUNT_HW_CACHE_MISSES,
  branches = PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
  branch_misses = PERF_COUNT_HW_BRANCH_MISSES,
  bus_cycles = PERF_COUNT_HW_BUS_CYCLES,
  stalled_init = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND,
  stalled_retirement = PERF_COUNT_HW_STALLED_CYCLES_BACKEND,
  total_cycles = PERF_COUNT_HW_REF_CPU_CYCLES,
  none
};

enum class software : u64 {
  cpu_clock = PERF_COUNT_SW_CPU_CLOCK,
  tasks = PERF_COUNT_SW_TASK_CLOCK,
  page_faults = PERF_COUNT_SW_PAGE_FAULTS,
  cntx_swtch = PERF_COUNT_SW_CONTEXT_SWITCHES,
  cpu_migrations = PERF_COUNT_SW_CPU_MIGRATIONS,
  minor_page_flt = PERF_COUNT_SW_PAGE_FAULTS_MIN,
  major_page_flt = PERF_COUNT_SW_PAGE_FAULTS_MAJ,
  alignment_faults = PERF_COUNT_SW_ALIGNMENT_FAULTS,
  emulation_faults = PERF_COUNT_SW_EMULATION_FAULTS,
  nothing = PERF_COUNT_SW_DUMMY,
  sample = PERF_COUNT_SW_BPF_OUTPUT,
  cgroup_swtch = PERF_COUNT_SW_CGROUP_SWITCHES,
  none
};

enum class cache : u64 {
  level1d = PERF_COUNT_HW_CACHE_L1D,
  level1t = PERF_COUNT_HW_CACHE_L1I,
  last_level = PERF_COUNT_HW_CACHE_LL,
  data_tlb = PERF_COUNT_HW_CACHE_DTLB,
  instr_tlb = PERF_COUNT_HW_CACHE_ITLB,
  branch = PERF_COUNT_HW_CACHE_BPU,
  local_access = PERF_COUNT_HW_CACHE_NODE,

  level1d_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_L1D) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  level1t_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_L1I) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  last_level_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_LL) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  data_tlb_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_DTLB) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  instr_tlb_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_ITLB) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  branch_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_BPU) | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),
  level1d_prefetch = static_cast<u64>(PERF_COUNT_HW_CACHE_L1D) | (static_cast<u64>(PERF_COUNT_HW_CACHE_OP_PREFETCH) << 8),
  level1d_prefetch_miss = static_cast<u64>(PERF_COUNT_HW_CACHE_L1D) | (static_cast<u64>(PERF_COUNT_HW_CACHE_OP_PREFETCH) << 8)
                          | (static_cast<u64>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16),

  none
};
};      // namespace options

struct time_userland {
};

struct time_kernelland {
};

struct time_vmland {
};

struct time_everyland {
};

template<class T, kernel_clock_types R> struct kernel_clock {
  int e_fd;
  int last_errno = 0;
  struct perf_event_attr event;

  ~kernel_clock(void)
  {
    if ( e_fd != -1 ) micron::close(e_fd);
  }

  kernel_clock(const kernel_clock &) = delete;

  kernel_clock(kernel_clock &&o) : e_fd(o.e_fd), last_errno(o.last_errno), event(micron::move(o.event)) { o.e_fd = -1; }

  auto
  __get_event(void) const
  {
    return event;
  }

  void
  set_inherit(bool on) noexcept
  {
    event.inherit = on ? 1 : 0;
  }

  void
  set_pinned(bool on) noexcept
  {
    event.pinned = on ? 1 : 0;
  }

  void
  set_enable_on_exec(bool on) noexcept
  {
    event.enable_on_exec = on ? 1 : 0;
  }

  int
  open_error(void) const noexcept
  {
    return last_errno;
  }

  template<class P>
    requires(micron::same_as<P, options::hardware> or micron::same_as<P, options::software> or micron::same_as<P, options::cache>)
  kernel_clock(P e_type) : e_fd(-1)
  {
    micron::memset(&event, 0, sizeof(event));

    event.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    if constexpr ( micron::same_as<T, time_everyland> ) {
      event.size = sizeof(event);
      if constexpr ( R == kernel_clock_types::hardware ) {
        event.type = PERF_TYPE_HARDWARE;
        event.disabled = 1;
        event.pinned = 1;

        if constexpr ( micron::is_same_v<P, options::hardware> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::software ) {
        event.type = PERF_TYPE_SOFTWARE;
        event.disabled = 1;
        event.pinned = 1;

        event.exclude_hv = 1;

        if constexpr ( micron::is_same_v<P, options::software> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::tracepoint ) {
        event.type = PERF_TYPE_TRACEPOINT;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_hv = 1;
      }
      if constexpr ( R == kernel_clock_types::cache ) {
        event.type = PERF_TYPE_HW_CACHE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_hv = 1;
        if constexpr ( micron::is_same_v<P, options::cache> ) event.config = static_cast<__u64>(e_type);
      }
    }

    if constexpr ( micron::same_as<T, time_kernelland> ) {
      event.size = sizeof(event);
      if constexpr ( R == kernel_clock_types::hardware ) {
        event.type = PERF_TYPE_HARDWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_hv = 1;

        if constexpr ( micron::is_same_v<P, options::hardware> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::software ) {
        event.type = PERF_TYPE_SOFTWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_hv = 1;
        if constexpr ( micron::is_same_v<P, options::software> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::tracepoint ) {
        event.type = PERF_TYPE_TRACEPOINT;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_hv = 1;
      }
      if constexpr ( R == kernel_clock_types::cache ) {
        event.type = PERF_TYPE_HW_CACHE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_hv = 1;
        if constexpr ( micron::is_same_v<P, options::cache> ) event.config = static_cast<__u64>(e_type);
      }
    }

    if constexpr ( micron::same_as<T, time_userland> ) {
      event.size = sizeof(event);
      if constexpr ( R == kernel_clock_types::hardware ) {
        event.type = PERF_TYPE_HARDWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_kernel = 1;
        event.exclude_hv = 1;

        if constexpr ( micron::is_same_v<P, options::hardware> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::software ) {
        event.type = PERF_TYPE_SOFTWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_kernel = 1;
        event.exclude_hv = 1;
        if constexpr ( micron::is_same_v<P, options::software> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::tracepoint ) {
        event.type = PERF_TYPE_TRACEPOINT;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_kernel = 1;
        event.exclude_hv = 1;
      }
      if constexpr ( R == kernel_clock_types::cache ) {
        event.type = PERF_TYPE_HW_CACHE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_kernel = 1;
        event.exclude_hv = 1;
        if constexpr ( micron::is_same_v<P, options::cache> ) event.config = static_cast<__u64>(e_type);
      }
    }

    if constexpr ( micron::same_as<T, time_vmland> ) {
      event.size = sizeof(event);
      if constexpr ( R == kernel_clock_types::hardware ) {
        event.type = PERF_TYPE_HARDWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_kernel = 1;

        if constexpr ( micron::is_same_v<P, options::hardware> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::software ) {
        event.type = PERF_TYPE_SOFTWARE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_kernel = 1;
        if constexpr ( micron::is_same_v<P, options::software> ) event.config = static_cast<__u64>(e_type);
      }
      if constexpr ( R == kernel_clock_types::tracepoint ) {
        event.type = PERF_TYPE_TRACEPOINT;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_kernel = 1;
      }
      if constexpr ( R == kernel_clock_types::cache ) {
        event.type = PERF_TYPE_HW_CACHE;
        event.disabled = 1;
        event.pinned = 1;
        event.exclude_user = 1;
        event.exclude_kernel = 1;
        if constexpr ( micron::is_same_v<P, options::cache> ) event.config = static_cast<__u64>(e_type);
      }
    }
  }

  int
  reopen(pid_t pid)
  {
    if ( e_fd != -1 ) micron::close(e_fd);
    e_fd = static_cast<int>(perf_event_pid(event, pid));
    if ( e_fd == -1 ) {
      last_errno = errno;
      return -1;
    }
    last_errno = 0;
    return e_fd;
  }

  int
  reopen(void)
  {
    if ( e_fd != -1 ) micron::close(e_fd);
    e_fd = static_cast<int>(perf_event_this(event));
    if ( e_fd == -1 ) {
      last_errno = errno;
      return -1;
    }
    last_errno = 0;
    return e_fd;
  }

  int
  open(pid_t pid)
  {
    e_fd = static_cast<int>(perf_event_pid(event, pid));
    if ( e_fd == -1 ) {
      last_errno = errno;
      return -1;
    }
    last_errno = 0;
    return e_fd;
  }

  int
  open(void)
  {
    e_fd = static_cast<int>(perf_event_this(event));
    if ( e_fd == -1 ) {
      last_errno = errno;
      return -1;
    }
    last_errno = 0;
    return e_fd;
  }

  template<typename X = T>
  inline __attribute__((always_inline)) void
  start(void)
  {
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_RESET, 0);
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_ENABLE, 0);
  }

  template<typename X = T>
  inline __attribute__((always_inline)) void
  start_as_leader(void)
  {
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }

  template<typename X = T>
  inline __attribute__((always_inline)) void
  stop(void)
  {
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_DISABLE, 0);
  }

  template<typename X = T>
  inline __attribute__((always_inline)) void
  stop_as_leader(void)
  {
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
  }

  struct read_result {
    u64 value;
    u64 time_enabled;
    u64 time_running;
  };

  inline read_result
  __read_triplet(void)
  {
    read_result r{ 0, 0, 0 };
    if ( e_fd == -1 ) return r;
    micron::posix::read(e_fd, &r, sizeof(r));
    return r;
  }

  long long
  read_raw(void)
  {
    return static_cast<long long>(__read_triplet().value);
  }

  long long
  read(void)
  {
    auto t = __read_triplet();
    if ( t.time_running == 0 ) return 0;
    if ( t.time_running == t.time_enabled ) return static_cast<long long>(t.value);

    const double scale = static_cast<double>(t.time_enabled) / static_cast<double>(t.time_running);
    return static_cast<long long>(static_cast<double>(t.value) * scale);
  }
};

enum class system_clocks : clockid_t {
  realtime_set = micron::clock_realtime,
  realtime = micron::clock_realtime_alarm,
  realtime_coarse = micron::clock_realtime_coarse,
  taitime = micron::clock_tai,
  monotonic = micron::clock_monotonic,
  monotonic_coarse = micron::clock_monotonic_coarse,
  monotonic_raw = micron::clock_monotonic_raw,
  since_boot = micron::clock_boottime,
  cputime = micron::clock_process_cputime_id,
  cputime_this = micron::clock_thread_cputime_id
};

template<system_clocks C = system_clocks::realtime> struct system_clock {
  micron::timespec_t time_begin;
  micron::timespec_t time_end;

  system_clock(options::hardware)
  {
    micron::memset(&time_begin, 0x0, sizeof(micron::timespec_t));
    micron::memset(&time_end, 0x0, sizeof(micron::timespec_t));
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
  }

  system_clock()
  {
    micron::memset(&time_begin, 0x0, sizeof(micron::timespec_t));
    micron::memset(&time_end, 0x0, sizeof(micron::timespec_t));
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
  }

  inline __attribute__((always_inline)) void
  start(void)
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  start_get(void) -> micron::timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
    return time_begin;
  }

  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  stop_get(void) -> micron::timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
    return time_end;
  }

  inline __attribute__((always_inline)) static auto
  now(void) -> double
  {
    micron::timespec_t t;
    if ( micron::clock_gettime(static_cast<clockid_t>(C), t) == -1 )
      micron::exc<micron::except::runtime_error>("bbench clock failed to get time");
    auto sec = static_cast<double>(t.tv_sec);
    auto ns = static_cast<double>(t.tv_nsec);
    return sec + ns / 1e9;
  }

  auto
  read(const micron::timespec_t &t) -> double
  {
    auto sec = static_cast<f64>(t.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(t.tv_nsec - time_begin.tv_nsec);
    return sec + ns / 1e9;
  }

  auto
  read_ms(const micron::timespec_t &t) -> double
  {
    auto sec = static_cast<f64>(t.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(t.tv_nsec - time_begin.tv_nsec);
    return micron::milliseconds(sec) + ns / 1e6;
  }

  auto
  read(const micron::timespec_t &t, const micron::timespec_t &ts) -> double
  {
    auto sec = static_cast<f64>(t.tv_sec - ts.tv_sec);
    auto ns = static_cast<f64>(t.tv_nsec - ts.tv_nsec);
    return sec + ns / 1e9;
  }

  auto
  read_ms(const micron::timespec_t &t, const micron::timespec_t &ts) -> double
  {
    auto sec = static_cast<f64>(t.tv_sec - ts.tv_sec);
    auto ns = static_cast<f64>(t.tv_nsec - ts.tv_nsec);
    return micron::milliseconds(sec) + ns / 1e6;
  }

  auto
  read(void) -> double
  {
    auto sec = static_cast<f64>(time_end.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(time_end.tv_nsec - time_begin.tv_nsec);
    return sec + ns / 1e9;
  }

  auto
  read_ds(void) -> double
  {
    return read() * 10.0;
  }

  auto
  read_ms(void) -> double
  {
    auto sec = static_cast<f64>(time_end.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(time_end.tv_nsec - time_begin.tv_nsec);
    return micron::milliseconds(sec) + ns / 1e6;
  }

  auto
  read_us(void) -> double
  {
    auto sec = static_cast<f64>(time_end.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(time_end.tv_nsec - time_begin.tv_nsec);
    return micron::microseconds(sec) + ns / 1e3;
  }

  auto
  read_ns(void) -> double
  {
    auto sec = static_cast<f64>(time_end.tv_sec - time_begin.tv_sec);
    auto ns = static_cast<f64>(time_end.tv_nsec - time_begin.tv_nsec);
    return micron::nanoseconds(sec) + ns;
  }
};

};      // namespace bbench
