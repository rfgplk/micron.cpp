//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// time, inside a Linux kernel module

#include "__kport_abi.hpp"

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

inline ssize_t
clock_gettime(micron::clockid_t __clc, micron::timespec_t &__tm)
{
  mc_kport_i64 __s = 0;
  mc_kport_i64 __n = 0;
  const int __r = ::mc_kport_clock_gettime(static_cast<mc_kport_i32>(__clc), &__s, &__n);
  if ( __r != 0 ) return static_cast<ssize_t>(__r);
  __tm.tv_sec = static_cast<decltype(__tm.tv_sec)>(__s);
  __tm.tv_nsec = static_cast<decltype(__tm.tv_nsec)>(__n);
  return 0;
}

inline ssize_t
clock_getres(micron::clockid_t __clc, micron::timespec_t *__res)
{
  if ( __res == nullptr ) return static_cast<ssize_t>(-22);
  mc_kport_i64 __s = 0;
  mc_kport_i64 __n = 0;
  const int __r = ::mc_kport_clock_getres(static_cast<mc_kport_i32>(__clc), &__s, &__n);
  if ( __r != 0 ) return static_cast<ssize_t>(__r);
  __res->tv_sec = static_cast<decltype(__res->tv_sec)>(__s);
  __res->tv_nsec = static_cast<decltype(__res->tv_nsec)>(__n);
  return 0;
}

inline ssize_t
clock_getres(micron::clockid_t __clc, micron::timespec_t &__res)
{
  return clock_getres(__clc, &__res);
}

inline ssize_t
clock_settime(micron::clockid_t __clc, const micron::timespec_t &__tm)
{
  (void)__clc;
  (void)__tm;
  return static_cast<ssize_t>(-1);
}

namespace __bits
{
[[gnu::always_inline]] inline i64
__ts_to_ns(const micron::timespec_t &__ts) noexcept
{
  return static_cast<i64>(__ts.tv_sec) * ticks_per_sec_v + static_cast<i64>(__ts.tv_nsec);
}
};      // namespace __bits

inline i64
mono_ticks(void) noexcept
{
  return static_cast<i64>(::mc_kport_mono_ns());
}

inline i64
real_ticks(void) noexcept
{
  return static_cast<i64>(::mc_kport_real_ns());
}

inline void
sleep_ns(i64 __ns) noexcept
{
  if ( __ns <= 0 ) return;
  ::mc_kport_sleep_ns(static_cast<mc_kport_i64>(__ns));
}

inline ssize_t
nanosleep(const micron::timespec_t &__req, micron::timespec_t &__rem)
{
  sleep_ns(__bits::__ts_to_ns(__req));

  __rem.tv_sec = 0;
  __rem.tv_nsec = 0;
  return static_cast<ssize_t>(0);
}

inline ssize_t
nanosleep(const micron::timespec_t &__req)
{
  sleep_ns(__bits::__ts_to_ns(__req));
  return static_cast<ssize_t>(0);
}

inline ssize_t
clock_nanosleep(micron::clockid_t __clock, i32 __flags, micron::timespec_t &__tm, micron::timespec_t *__rmn)
{
  i64 __ns = __bits::__ts_to_ns(__tm);
  if ( __flags == micron::timer_abstime ) {
    micron::timespec_t __now{};
    if ( clock_gettime(__clock, __now) != 0 ) return static_cast<ssize_t>(-22);
    __ns -= __bits::__ts_to_ns(__now);
  }
  sleep_ns(__ns);
  if ( __rmn != nullptr ) {
    __rmn->tv_sec = 0;
    __rmn->tv_nsec = 0;
  }
  return 0;
}

inline ssize_t
clock_nanosleep(micron::clockid_t __clock, i32 __flags, const micron::timespec_t &__tm)
{
  micron::timespec_t __copy = __tm;
  return clock_nanosleep(__clock, __flags, __copy, nullptr);
}

inline micron::clock_t
cpu_clock(void)
{
  const i64 __ns = static_cast<i64>(::mc_kport_cpu_clock_ns());
  if ( __ns < 0 ) return static_cast<micron::clock_t>(-1);
  return static_cast<micron::clock_t>(__ns / (1000000000 / micron::clocks_per_sec));
}

inline time64_t
wall_seconds(void)
{
  return static_cast<time64_t>(::mc_kport_wall_seconds());
}

};      // namespace port
};      // namespace micron
