
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <linux/perf_event.h>

#include <micron/linux/io.hpp>
#include <micron/linux/sys/ioctl.hpp>
#include <micron/memory/cmemory.hpp>
#include <micron/types.hpp>

#include "options.hpp"
#include "perf.hpp"
#include "process.hpp"

namespace bbench::topdown
{

inline constexpr u64 TD_SLOTS = 0x0400u;
inline constexpr u64 TD_RETIRING = 0x8000u;
inline constexpr u64 TD_BAD_SPEC = 0x8100u;
inline constexpr u64 TD_FE_BOUND = 0x8200u;
inline constexpr u64 TD_BE_BOUND = 0x8300u;

struct topdown_t {
  double frontend = 0.0;
  double backend = 0.0;
  double bad_spec = 0.0;
  double retiring = 0.0;
  bool supported = false;
};

struct __td_event {
  int e_fd = -1;
  int last_errno = 0;
  perf_event_attr attr{};

  __td_event() = default;
  __td_event(const __td_event &) = delete;

  __td_event(__td_event &&o) noexcept : e_fd(o.e_fd), last_errno(o.last_errno), attr(o.attr) { o.e_fd = -1; }

  ~__td_event()
  {
    if ( e_fd != -1 ) micron::close(e_fd);
  }

  void
  configure(u64 config, bool excl_kernel = true)
  {
    micron::memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.type = PERF_TYPE_RAW;
    attr.config = config;
    attr.disabled = 1;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    attr.exclude_kernel = excl_kernel ? 1 : 0;
    attr.exclude_hv = 1;
  }

  int
  open_pid(pid_t pid)
  {
    e_fd = static_cast<int>(perf_event_pid(attr, pid));
    if ( e_fd == -1 ) {
      last_errno = errno;
      return -1;
    }
    last_errno = 0;
    return e_fd;
  }

  void
  begin(void)
  {
    if ( e_fd == -1 ) return;
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_RESET, 0);
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_ENABLE, 0);
  }

  void
  end(void)
  {
    if ( e_fd == -1 ) return;
    micron::posix::ioctl(e_fd, PERF_EVENT_IOC_DISABLE, 0);
  }

  long long
  retrieve_scaled(void)
  {
    if ( e_fd == -1 ) return 0;

    struct {
      u64 value, te, tr;
    } r{ 0, 0, 0 };

    micron::posix::read(e_fd, &r, sizeof(r));
    if ( r.tr == 0 ) return 0;
    if ( r.tr == r.te ) return static_cast<long long>(r.value);
    const double scale = static_cast<double>(r.te) / static_cast<double>(r.tr);
    return static_cast<long long>(static_cast<double>(r.value) * scale);
  }
};

inline bool
supported(void)
{
  static int cached = -1;
  if ( cached != -1 ) return cached == 1;
  __td_event probe;
  probe.configure(TD_SLOTS);
  if ( probe.open_pid(0) != -1 ) {
    cached = 1;
    return true;
  }
  cached = 0;
  return false;
}

inline topdown_t
measure_bin(const char *path, const benchmark_opts &opts)
{
  topdown_t out{};
  if ( !supported() ) return out;

  __td_event slots, retiring, bad_spec, fe_bound, be_bound;
  slots.configure(TD_SLOTS, opts.excl_kernel);
  retiring.configure(TD_RETIRING, opts.excl_kernel);
  bad_spec.configure(TD_BAD_SPEC, opts.excl_kernel);
  fe_bound.configure(TD_FE_BOUND, opts.excl_kernel);
  be_bound.configure(TD_BE_BOUND, opts.excl_kernel);

  slots.attr.enable_on_exec = 1;
  retiring.attr.enable_on_exec = 1;
  bad_spec.attr.enable_on_exec = 1;
  fe_bound.attr.enable_on_exec = 1;
  be_bound.attr.enable_on_exec = 1;

  if ( opts.pre ) process<true>(opts.pre);
  int pid = process_attach(path, [&](int child_pid) {
    slots.open_pid(child_pid);
    retiring.open_pid(child_pid);
    bad_spec.open_pid(child_pid);
    fe_bound.open_pid(child_pid);
    be_bound.open_pid(child_pid);
  });

  micron::waitpid(pid, nullptr, 0);

  slots.end();
  retiring.end();
  bad_spec.end();
  fe_bound.end();
  be_bound.end();
  (void)opts;

  if ( opts.post ) process<true>(opts.post);

  long long s = slots.retrieve_scaled();
  if ( s == 0 ) return out;
  out.retiring = static_cast<double>(retiring.retrieve_scaled()) / static_cast<double>(s);
  out.bad_spec = static_cast<double>(bad_spec.retrieve_scaled()) / static_cast<double>(s);
  out.frontend = static_cast<double>(fe_bound.retrieve_scaled()) / static_cast<double>(s);
  out.backend = static_cast<double>(be_bound.retrieve_scaled()) / static_cast<double>(s);
  out.supported = true;
  return out;
}

};      // namespace bbench::topdown
