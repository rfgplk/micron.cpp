//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../math/ratios.hpp"
#include "../../memory/cmemory.hpp"
#include "../sys/time.hpp"

namespace micron
{

template <system_clocks C = system_clocks::monotonic> class timerfd_t
{
  static constexpr timespec_t
  __to_timespec(fduration_t value, unit u) noexcept
  {
    timespec_t ts;
    switch ( u ) {
    case unit::nanoseconds :
      ts.tv_sec = static_cast<time_t>(value / 1'000'000'000LL);
      ts.tv_nsec = static_cast<long>(value) % 1'000'000'000L;
      break;
    case unit::microseconds :
      ts.tv_sec = static_cast<time_t>(value / 1'000'000LL);
      ts.tv_nsec = static_cast<long>(value % 1'000'000LL) * 1'000L;
      break;
    case unit::milliseconds :
      ts.tv_sec = static_cast<time_t>(value / 1'000LL);
      ts.tv_nsec = static_cast<long>(value % 1'000LL) * 1'000'000L;
      break;
    case unit::seconds :
      ts.tv_sec = static_cast<time_t>(value);
      ts.tv_nsec = 0;
      break;
    case unit::minutes :
      ts.tv_sec = static_cast<time_t>(value * __dur_sec_per_min);
      ts.tv_nsec = 0;
      break;
    case unit::hours :
      ts.tv_sec = static_cast<time_t>(value * __dur_sec_per_hr);
      ts.tv_nsec = 0;
      break;
    case unit::days :
      ts.tv_sec = static_cast<time_t>(value * __dur_sec_per_day);
      ts.tv_nsec = 0;
      break;
    }
    return ts;
  }

public:
  int fd;
  itimerspec_t current;

  ~timerfd_t()
  {
    if ( fd >= 0 )
      micron::syscall(SYS_close, fd);
  }

  timerfd_t()
  {
    micron::memset(&current, 0x0, sizeof(itimerspec_t));

    fd = micron::timerfd_create(static_cast<clockid_t>(C), tfd_cloexec);
    if ( fd < 0 )
      exc<except::runtime_error>("micron::timerfd_t failed to create timerfd");
  }

  static timerfd_t
  nonblocking()
  {
    timerfd_t t;
    micron::syscall(SYS_close, t.fd);
    t.fd = micron::timerfd_create(static_cast<clockid_t>(C), tfd_cloexec | tfd_nonblock);
    if ( t.fd < 0 )
      exc<except::runtime_error>("micron::timerfd_t::nonblocking failed to create timerfd");
    return t;
  }

  timerfd_t(const timerfd_t &) = delete;
  timerfd_t &operator=(const timerfd_t &) = delete;

  timerfd_t(timerfd_t &&o) noexcept : fd(o.fd), current(o.current)
  {
    o.fd = -1;
    micron::memset(&o.current, 0x0, sizeof(itimerspec_t));
  }

  timerfd_t &
  operator=(timerfd_t &&o) noexcept
  {
    if ( fd >= 0 )
      micron::syscall(SYS_close, fd);
    fd = o.fd;
    current = o.current;
    o.fd = -1;
    micron::memset(&o.current, 0x0, sizeof(itimerspec_t));
    return *this;
  }

  inline __attribute__((always_inline)) void
  arm(fduration_t interval, unit u = unit::milliseconds)
  {
    timespec_t ts = __to_timespec(interval, u);
    current.it_interval = ts;
    current.it_value = ts; /* first expiry == interval */
    if ( micron::timerfd_settime(fd, 0, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::arm failed");
  }

  inline __attribute__((always_inline)) void
  arm(fduration_t initial, unit iu, fduration_t interval, unit u)
  {
    current.it_value = __to_timespec(initial, iu);
    current.it_interval = __to_timespec(interval, u);
    if ( micron::timerfd_settime(fd, 0, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::arm failed");
  }

  inline __attribute__((always_inline)) void
  arm(const itimerspec_t &spec, int flags = 0)
  {
    current = spec;
    if ( micron::timerfd_settime(fd, flags, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::arm failed");
  }

  inline __attribute__((always_inline)) void
  arm_once(fduration_t delay, unit u = unit::milliseconds)
  {
    micron::memset(&current, 0x0, sizeof(itimerspec_t));
    current.it_value = __to_timespec(delay, u);
    if ( micron::timerfd_settime(fd, 0, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::arm_oneshot failed");
  }

  inline __attribute__((always_inline)) void
  arm_abs(const timespec_t &abs_expiry, const timespec_t &interval = { 0, 0 })
  {
    current.it_value = abs_expiry;
    current.it_interval = interval;
    if ( micron::timerfd_settime(fd, tfd_timer_abstime, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::arm_abs failed");
  }

  inline __attribute__((always_inline)) void
  disarm()
  {
    micron::memset(&current, 0x0, sizeof(itimerspec_t));
    if ( micron::timerfd_settime(fd, 0, current) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::disarm failed");
  }

  inline __attribute__((always_inline)) unsigned long long
  wait() const
  {
    unsigned long long count = 0;
    long r = micron::syscall(SYS_read, fd, &count, sizeof(count));
    if ( r < 0 )
      exc<except::runtime_error>("micron::timerfd_t::wait read failed");
    return count;
  }

  inline __attribute__((always_inline)) unsigned long long
  try_wait(bool &ok) const noexcept
  {
    unsigned long long count = 0;
    long r = micron::syscall(SYS_read, fd, &count, sizeof(count));
    ok = (r == static_cast<long>(sizeof(count)));
    return ok ? count : 0ULL;
  }

  inline itimerspec_t
  gettime() const
  {
    itimerspec_t spec;
    if ( micron::timerfd_gettime(fd, spec) != 0 )
      exc<except::runtime_error>("micron::timerfd_t::gettime failed");
    return spec;
  }

  template <unit U = unit::milliseconds>
  inline fduration_t
  remaining() const
  {
    itimerspec_t spec = gettime();
    return __impl::delta_to_unit<U>(spec.it_value.tv_sec, spec.it_value.tv_nsec);
  }

  inline bool
  armed() const noexcept
  {
    itimerspec_t spec;
    if ( micron::timerfd_gettime(fd, spec) != 0 )
      return false;
    return spec.it_value.tv_sec != 0 || spec.it_value.tv_nsec != 0;
  }

  inline
  operator int() const noexcept
  {
    return fd;
  }
};

using realtime_timerfd_t = timerfd_t<system_clocks::realtime>;
using monotonic_timerfd_t = timerfd_t<system_clocks::monotonic>;
using boottime_timerfd_t = timerfd_t<system_clocks::since_boot>;

};     // namespace micron
