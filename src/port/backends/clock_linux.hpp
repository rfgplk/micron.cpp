//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// time, on userspace Linux

#include "__syscall.hpp"

#include "../../types.hpp"

#include "../../bits/__posix_time_types.hpp"

namespace micron
{
namespace port
{

constexpr i64 ticks_per_sec_v = 1'000'000'000;

constexpr i64
ticks_per_sec(void) noexcept
{
  return ticks_per_sec_v;
}

#if defined(__micron_arch_width_32)
constexpr long __sys_clock_gettime = SYS_clock_gettime64;
constexpr long __sys_clock_settime = SYS_clock_settime64;
constexpr long __sys_clock_getres = SYS_clock_getres_time64;
constexpr long __sys_clock_nanosleep = SYS_clock_nanosleep_time64;
#else
constexpr long __sys_clock_gettime = SYS_clock_gettime;
constexpr long __sys_clock_settime = SYS_clock_settime;
constexpr long __sys_clock_getres = SYS_clock_getres;
constexpr long __sys_clock_nanosleep = SYS_clock_nanosleep;
#endif

inline ssize_t
nanosleep(const micron::timespec_t &req, micron::timespec_t &rem)
{
#if defined(__micron_arch_width_32)
  return micron::syscall(SYS_clock_nanosleep_time64, micron::clock_monotonic, 0, &req, &rem);
#else
  return micron::syscall(SYS_nanosleep, &req, &rem);
#endif
}

inline ssize_t
nanosleep(const micron::timespec_t &req)
{
#if defined(__micron_arch_width_32)
  return micron::syscall(SYS_clock_nanosleep_time64, micron::clock_monotonic, 0, &req, nullptr);
#else
  return micron::syscall(SYS_nanosleep, &req, nullptr);
#endif
}

inline ssize_t
clock_gettime(micron::clockid_t clc, micron::timespec_t &tm)
{
  return micron::syscall(__sys_clock_gettime, clc, &tm);
}

inline ssize_t
clock_getres(micron::clockid_t clc, micron::timespec_t *res)
{
  return micron::syscall(__sys_clock_getres, clc, res);
}

inline ssize_t
clock_getres(micron::clockid_t clc, micron::timespec_t &res)
{
  return micron::syscall(__sys_clock_getres, clc, &res);
}

inline ssize_t
clock_settime(micron::clockid_t clc, const micron::timespec_t &tm)
{
  return micron::syscall(__sys_clock_settime, clc, &tm);
}

inline ssize_t
clock_nanosleep(micron::clockid_t clock, i32 flags, micron::timespec_t &tm, micron::timespec_t *rmn)
{
  return micron::syscall(__sys_clock_nanosleep, clock, flags, &tm, rmn);
}

inline ssize_t
clock_nanosleep(micron::clockid_t clock, i32 flags, const micron::timespec_t &tm)
{
  return micron::syscall(__sys_clock_nanosleep, clock, flags, &tm, nullptr);
}

inline micron::clock_t
cpu_clock(void)
{
  micron::timespec_t tm;
  if ( clock_gettime(micron::clock_process_cputime_id, tm) != 0 ) return -1;

  return static_cast<micron::clock_t>(tm.tv_sec * micron::clocks_per_sec + tm.tv_nsec / (1000000000 / micron::clocks_per_sec));
}

inline time64_t
wall_seconds(void)
{
#if defined(__micron_arch_amd64)
  return micron::syscall(SYS_time, nullptr);
#else
  micron::timespec_t __ts{};
  if ( clock_gettime(micron::clock_realtime, __ts) != 0 ) return static_cast<time64_t>(-1);
  return static_cast<time64_t>(__ts.tv_sec);
#endif
}

namespace __bits
{
[[gnu::always_inline]] inline i64
__clock_ns(clockid_t __clk) noexcept
{
  micron::timespec_t __ts{};
  if ( clock_gettime(__clk, __ts) != 0 ) return -1;
  return static_cast<i64>(__ts.tv_sec) * ticks_per_sec_v + static_cast<i64>(__ts.tv_nsec);
}
};      // namespace __bits

inline i64
mono_ticks(void) noexcept
{
  return __bits::__clock_ns(micron::clock_monotonic);
}

inline i64
real_ticks(void) noexcept
{
  return __bits::__clock_ns(micron::clock_realtime);
}

inline void
sleep_ns(i64 __ns) noexcept
{
  if ( __ns <= 0 ) return;
  micron::timespec_t __req{};
  __req.tv_sec = static_cast<decltype(__req.tv_sec)>(__ns / ticks_per_sec_v);
  __req.tv_nsec = static_cast<decltype(__req.tv_nsec)>(__ns % ticks_per_sec_v);
  (void)nanosleep(__req);
}

};      // namespace port
};      // namespace micron
