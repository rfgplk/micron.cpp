#pragma once

#include "math/ratios.hpp"
#include "except.hpp"
#include "memory/cmemory.hpp"

namespace micron {

// standard duration
using duration_t = __time_t;
using duration_d = double;
constexpr duration_d duration_ratio = 1000;     // the standard SI metric prefix (power of 1000)

template <typename S = base_ratio>
inline constexpr duration_d
seconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = milli>
inline constexpr duration_d
milliseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = micro>
inline constexpr duration_d
microseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = nano>
inline constexpr duration_d
nanoseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}


enum class system_clocks : clockid_t {
  realtime_set = CLOCK_REALTIME,
  realtime = CLOCK_REALTIME_ALARM,
  realtime_coarse = CLOCK_REALTIME_COARSE,
  taitime = CLOCK_TAI,
  monotonic = CLOCK_MONOTONIC,
  monotonic_coarse = CLOCK_MONOTONIC_COARSE,
  monotonic_raw = CLOCK_MONOTONIC_RAW,
  since_boot = CLOCK_BOOTTIME,
  cputime = CLOCK_PROCESS_CPUTIME_ID,
  cputime_this = CLOCK_THREAD_CPUTIME_ID
};

// clock meant to get general time
template <system_clocks C = system_clocks::realtime> struct system_clock {
  struct timespec time_begin;
  struct timespec time_end;
  system_clock(options::hardware)
  {
    std::memset(&time_begin, 0x0, sizeof(timespec));
    std::memset(&time_end, 0x0, sizeof(timespec));
    if ( ::clock_gettime((clockid_t)C, &time_begin) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
  }
  system_clock()
  {
    std::memset(&time_begin, 0x0, sizeof(timespec));
    std::memset(&time_end, 0x0, sizeof(timespec));
    if ( ::clock_gettime((clockid_t)C, &time_begin) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
  }
  inline __attribute__((always_inline)) void
  start(void)
  {
    if ( ::clock_gettime((clockid_t)C, &time_begin) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
  }
  inline __attribute__((always_inline)) auto
  start_get(void) -> timespec
  {
    if ( ::clock_gettime((clockid_t)C, &time_begin) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
    return time_begin;
  }
  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( ::clock_gettime((clockid_t)C, &time_end) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
  }
  inline __attribute__((always_inline)) auto
  stop_get(void) -> timespec
  {
    if ( ::clock_gettime((clockid_t)C, &time_end) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
    return time_end;
  }
  inline __attribute__((always_inline)) static auto
  now(void) -> double
  {
    struct timespec t;
    if ( ::clock_gettime((clockid_t)C, &t) == -1 )
      throw std::runtime_error("bbench clock failed to get time");
    auto sec = t.tv_sec;
    auto msec = (t.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read(const timespec &t) -> double
  {
    auto sec = t.tv_sec - time_begin.tv_sec;
    auto msec = (t.tv_nsec - time_begin.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read_ms(const timespec &t) -> double
  {
    auto sec = t.tv_sec - time_begin.tv_sec;
    auto msec = (t.tv_nsec - time_begin.tv_nsec) / 1000000;
    return chrono::milliseconds(static_cast<double>(sec)) + static_cast<double>(msec);
  }
  auto
  read(void) -> double
  {
    auto sec = time_end.tv_sec - time_begin.tv_sec;
    auto msec = (time_end.tv_nsec - time_begin.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read_ms(void) -> double
  {
    auto sec = time_end.tv_sec - time_begin.tv_sec;
    auto msec = (time_end.tv_nsec - time_begin.tv_nsec) / 1000000;
    return chrono::milliseconds(static_cast<double>(sec)) + static_cast<double>(msec);
  }
};
};
