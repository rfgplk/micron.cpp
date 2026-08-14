//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../linux/sys/cpu.hpp"
#include "../linux/sys/sched.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "calibrate.hpp"
#include "clock.hpp"
#include "cycles.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// measuring
//
//
// must follow this order: PIN, then CALIBRATE, then MEASURE

namespace micron
{
namespace chrono
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// optimisation control
//
// sink(v):      v must be computer; the compiler may still assume it is unchanged across the call
// modify(v):    v must be computed && is assumed mutated afterwards
// overwrite(v): v is unknown afterwards, and is not computed
// sink_ptr(p):  full memory clobber

template<typename T>
[[gnu::always_inline]] inline void
sink(T v) noexcept
{
  static_assert(micron::is_arithmetic_v<T> || micron::is_pointer_v<T>, "sink() takes a scalar");
  __asm__ __volatile__("" ::"r"(v));
}

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline void
sink(f64 v) noexcept
{
  __asm__ __volatile__("" ::"x"(v));
}

[[gnu::always_inline]] inline void
sink(f32 v) noexcept
{
  __asm__ __volatile__("" ::"x"(v));
}
#elif defined(__micron_arch_arm_any)
[[gnu::always_inline]] inline void
sink(f64 v) noexcept
{
  __asm__ __volatile__("" ::"w"(v));
}

[[gnu::always_inline]] inline void
sink(f32 v) noexcept
{
  __asm__ __volatile__("" ::"w"(v));
}
#endif

template<typename T>
[[gnu::always_inline]] inline void
modify(T &v) noexcept
{
  static_assert(micron::is_arithmetic_v<T> || micron::is_pointer_v<T>, "modify() takes a scalar");
  __asm__ __volatile__("" : "+r"(v));
}

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline void
modify(f64 &v) noexcept
{
  __asm__ __volatile__("" : "+x"(v));
}

[[gnu::always_inline]] inline void
modify(f32 &v) noexcept
{
  __asm__ __volatile__("" : "+x"(v));
}
#elif defined(__micron_arch_arm_any)
[[gnu::always_inline]] inline void
modify(f64 &v) noexcept
{
  __asm__ __volatile__("" : "+w"(v));
}

[[gnu::always_inline]] inline void
modify(f32 &v) noexcept
{
  __asm__ __volatile__("" : "+w"(v));
}
#endif

template<typename T>
[[gnu::always_inline]] inline void
overwrite(T &v) noexcept
{
  static_assert(micron::is_arithmetic_v<T> || micron::is_pointer_v<T>, "overwrite() takes a scalar");
  __asm__ __volatile__("" : "=r"(v));
}

[[gnu::always_inline]] inline void
sink_ptr(const void *p) noexcept
{
  __asm__ __volatile__("" ::"r"(p) : "memory");
}

// everything in memory is assumed written
[[gnu::always_inline]] inline void
clobber_memory() noexcept
{
  __asm__ __volatile__("" ::: "memory");
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pinning

// the cpu this thread is on right now, or ~0u
inline u32
current_cpu(void) noexcept
{
  u32 cpu = 0;
  u32 node = 0;
  if ( micron::posix::getcpu(&cpu, &node) < 0 ) return ~0u;
  return cpu;
}

inline u32
first_available_cpu(void) noexcept
{
  micron::posix::cpu_set_t set{};
  if ( micron::posix::sched_getaffinity(0, sizeof(set), set) < 0 ) return 0;
  for ( usize c = 0; c < static_cast<usize>(micron::posix::cpu_setsize); ++c ) {
    if ( set.cpu_isset(c) ) return static_cast<u32>(c);
  }
  return 0;
}

// 0 on success, -errno otherwise
inline i32
pin_to_cpu(u32 cpu) noexcept
{
  micron::posix::cpu_set_t set{};
  set.cpu_zero();
  set.cpu_set(static_cast<usize>(cpu));
  return static_cast<i32>(micron::posix::sched_setaffinity(0, sizeof(set), set));
}

inline i32
pin_here(void) noexcept
{
  return pin_to_cpu(first_available_cpu());
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// warmup
//
// a core at its idle P-state ramps over milliseconds

[[gnu::noinline]] inline void
warmup_ns(u64 ns) noexcept
{
  if ( ns == 0 ) return;
  const i64 t0 = clock_ns(clock_monotonic);
  if ( t0 < 0 ) return;
  u64 acc = 1;
  for ( ;; ) {
    for ( u32 i = 0; i < 4096; ++i ) {
      acc = acc * 6364136223846793005ull + 1442695040888963407ull;
      modify(acc);
    }
    const i64 t = clock_ns(clock_monotonic);
    if ( t < 0 || static_cast<u64>(t - t0) >= ns ) break;
  }
  sink(acc);
}

// pin, warm the core, then calibrate on it
inline i32
prepare(u32 cpu, u64 warm_ns = 50'000'000ull) noexcept
{
  const i32 r = pin_to_cpu(cpu);
  warmup_ns(warm_ns);
  reset_core_hz();
  (void)core_hz();
  (void)tick_hz();
  return r;
}

inline i32
prepare_here(u64 warm_ns = 50'000'000ull) noexcept
{
  return prepare(first_available_cpu(), warm_ns);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a scoped tick counter
//
// NOTE: always_inline on the reads and noinline on samples

template<serial S = default_serial> struct tick_timer {
  u64 begin;
  u64 *out;

  [[gnu::always_inline]] explicit tick_timer(u64 *result = nullptr) noexcept : begin(tick_start<S>()), out(result) { }

  [[gnu::always_inline]] ~tick_timer()
  {
    if ( !out ) return;
    const u64 e = tick_end<S>();
    *out = e > begin ? e - begin : 0;
  }

  tick_timer(const tick_timer &) = delete;
  tick_timer &operator=(const tick_timer &) = delete;

  [[gnu::always_inline]] u64
  elapsed_ticks() const noexcept
  {
    const u64 e = tick_end<S>();
    return e > begin ? e - begin : 0;
  }

  u64
  elapsed_ns() const noexcept
  {
    return cycles_to_ns(elapsed_ticks(), tick_hz());
  }
};

};      // namespace chrono
};      // namespace micron
