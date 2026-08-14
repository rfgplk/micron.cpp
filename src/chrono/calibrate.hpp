//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../bits/__cpuid.hpp"
#include "../bits/__int128.hpp"
#include "../types.hpp"

#include "clock.hpp"
#include "cycles.hpp"
#include "units.hpp"

#if defined(__micron_arch_amd64)
#include "arch/calib_amd64.hpp"
#elif defined(__micron_arch_x86)
#include "arch/calib_i386.hpp"
#elif defined(__micron_arch_arm64)
#include "arch/calib_arm64.hpp"
#elif defined(__micron_arch_arm32)
#include "arch/calib_arm32.hpp"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tick_hz(): rate at which the counter advances; invariant, fixed at boot
// core_hz(): rate at which the core is executing _currently_
//
// NOTE: anything that reports "cycles per operation" needs core_hz

namespace micron
{
namespace chrono
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exact tick <-> nanosecond conversion
//
// WARNING: never through f64; stops being exact after ~9e15

inline constexpr u64
cycles_to_ns(u64 cycles, u64 hz) noexcept
{
  if ( hz == 0 ) return 0;
#if defined(__micron_has_int128)
  return static_cast<u64>((static_cast<uint128_t>(cycles) * 1'000'000'000ull) / hz);
#else
  // exact and overflow-free without a wide type
  const u64 q = cycles / hz;
  const u64 r = cycles % hz;
  return q * 1'000'000'000ull + (r * 1'000'000'000ull) / hz;
#endif
}

inline constexpr u64
ns_to_cycles(u64 ns, u64 hz) noexcept
{
#if defined(__micron_has_int128)
  return static_cast<u64>((static_cast<uint128_t>(ns) * hz) / 1'000'000'000ull);
#else
  const u64 q = ns / 1'000'000'000ull;
  const u64 r = ns % 1'000'000'000ull;
  return q * hz + (r * hz) / 1'000'000'000ull;
#endif
}

inline constexpr u64
cycles_to_us(u64 cycles, u64 hz) noexcept
{
  return hz == 0 ? 0 : cycles_to_ns(cycles, hz) / 1'000ull;
}

struct tsc_traits {
  bool present = false;
  bool invariant = false;          // CPUID.80000007H:EDX[8]
  bool has_rdtscp = false;         // CPUID.80000001H:EDX[27]
  bool has_serialize = false;      // CPUID.7.0:EDX[14]
  u64 nominal_hz = 0;              // from CPUID leaf 0x15 / 0x16; 0 when the CPU does not say
};

inline tsc_traits
counter_traits(void) noexcept
{
  tsc_traits t{};
#if defined(__micron_arch_x86_any)
  t.present = true;
  if ( micron::__cpuid_has_leaf(0x8000'0007u) ) t.invariant = (micron::__cpuid_read(0x8000'0007u).edx >> 8) & 1u;
  if ( micron::__cpuid_has_leaf(0x8000'0001u) ) t.has_rdtscp = (micron::__cpuid_read(0x8000'0001u).edx >> 27) & 1u;
  if ( micron::__cpuid_has_leaf(7u) ) t.has_serialize = (micron::__cpuid_read(7u, 0u).edx >> 14) & 1u;

  // leaf 0x15: ecx * ebx / eax
  if ( micron::__cpuid_has_leaf(0x15u) ) {
    const cpuid_regs r = micron::__cpuid_read(0x15u, 0u);
    if ( r.eax != 0 && r.ebx != 0 && r.ecx != 0 )
      t.nominal_hz = (static_cast<u64>(r.ecx) * static_cast<u64>(r.ebx)) / static_cast<u64>(r.eax);
  }
  // leaf 0x16: base frequency in MHz
  if ( t.nominal_hz == 0 && micron::__cpuid_has_leaf(0x16u) ) {
    const u32 mhz = micron::__cpuid_read(0x16u).eax & 0xFFFFu;
    if ( mhz != 0 ) t.nominal_hz = static_cast<u64>(mhz) * 1'000'000ull;
  }
#elif defined(__micron_arch_arm64)
  t.present = true;
  t.invariant = true;
  u64 f = 0;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(f));
  t.nominal_hz = f;
#elif defined(__micron_arch_arm32) && defined(MICRON_CHRONO_ARM32_CNTVCT)
  t.present = true;
  t.invariant = true;
  u32 f = 0;
  __asm__ __volatile__("mrc p15, 0, %0, c14, c0, 0" : "=r"(f));
  t.nominal_hz = f;
#endif
  return t;
}

namespace __impl
{
inline void
__sort_u64(u64 *v, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const u64 key = v[i];
    u32 j = i;
    while ( j > 0 && v[j - 1] > key ) {
      v[j] = v[j - 1];
      --j;
    }
    v[j] = key;
  }
}

inline u64
__median_u64(u64 *v, u32 n) noexcept
{
  if ( n == 0 ) return 0;
  __sort_u64(v, n);
  return v[(n - 1) / 2];
}

inline u64
__min_u64(const u64 *v, u32 n) noexcept
{
  if ( n == 0 ) return 0;
  u64 m = v[0];
  for ( u32 i = 1; i < n; ++i )
    if ( v[i] < m ) m = v[i];
  return m;
}
};      // namespace __impl

namespace __impl
{
inline u64 __tick_hz_slot = 0;
inline u32 __tick_hz_state = 0;
};      // namespace __impl

inline u64
calibrate_tick_hz(u64 window_ns = 20'000'000ull) noexcept
{
  const i64 t0 = clock_ns(clock_monotonic);
  if ( t0 < 0 ) return 0;
  const u64 c0 = tick<serial::mfence_lfence>();
  i64 t1 = t0;
  while ( static_cast<u64>(t1 - t0) < window_ns ) {
    t1 = clock_ns(clock_monotonic);
    if ( t1 < 0 ) return 0;
  }
  const u64 c1 = tick<serial::mfence_lfence>();
  const u64 dt = static_cast<u64>(t1 - t0);
  if ( dt == 0 || c1 <= c0 ) return 0;
  return static_cast<u64>((static_cast<u64>(c1 - c0) * 1'000'000'000ull) / dt);
}

inline u64
tick_hz(void) noexcept
{
  if ( __atomic_load_n(&__impl::__tick_hz_state, __ATOMIC_ACQUIRE) != 0u ) [[likely]]
    return __impl::__tick_hz_slot;

  u64 hz = 0;
  if constexpr ( !counter_is_native ) {
    hz = 1'000'000'000ull;
  } else {
    const tsc_traits t = counter_traits();
    hz = t.nominal_hz;
    if ( hz == 0 ) hz = calibrate_tick_hz();
    if ( hz == 0 ) hz = 1'000'000'000ull;
  }
  __impl::__tick_hz_slot = hz;
  __atomic_store_n(&__impl::__tick_hz_state, 1u, __ATOMIC_RELEASE);
  return hz;
}

inline void
set_tick_hz(u64 hz) noexcept
{
  __impl::__tick_hz_slot = hz;
  __atomic_store_n(&__impl::__tick_hz_state, 1u, __ATOMIC_RELEASE);
}

inline u64
ticks_to_ns(u64 ticks) noexcept
{
  return cycles_to_ns(ticks, tick_hz());
}

inline u64
ns_to_ticks(u64 ns) noexcept
{
  return ns_to_cycles(ns, tick_hz());
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// core_hz
#if !defined(MICRON_CHRONO_CALIB_ITERS)
#define MICRON_CHRONO_CALIB_ITERS 1000000
#endif
#if !defined(MICRON_CHRONO_CALIB_TRIES)
#define MICRON_CHRONO_CALIB_TRIES 33
#endif
#if !defined(MICRON_CHRONO_CALIB_WARMUP_NS)
#define MICRON_CHRONO_CALIB_WARMUP_NS 30000000ull
#endif

inline constexpr i64 calib_iters = MICRON_CHRONO_CALIB_ITERS;
inline constexpr u32 calib_tries = MICRON_CHRONO_CALIB_TRIES;
inline constexpr u64 calib_warmup_ns = MICRON_CHRONO_CALIB_WARMUP_NS;

namespace __impl
{
inline u64 __core_hz_slot = 0;
inline u32 __core_hz_state = 0;

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
using arch::calib_kernel;
inline constexpr bool __have_calib_kernel = true;
#else
[[gnu::noinline]] inline void
calib_kernel(i64 iters) noexcept
{
  if ( iters <= 0 ) return;
  iters &= ~static_cast<i64>(3);
  do {
    __asm__ __volatile__("" : "+r"(iters));
    --iters;
    __asm__ __volatile__("" : "+r"(iters));
    --iters;
    __asm__ __volatile__("" : "+r"(iters));
    --iters;
    --iters;
  } while ( iters > 0 );
}

inline constexpr bool __have_calib_kernel = false;
#endif
};      // namespace __impl

inline u64
__calib_sample(void) noexcept
{
  const i64 t0 = clock_ns(clock_monotonic);
  __impl::calib_kernel(calib_iters);
  const i64 t1 = clock_ns(clock_monotonic);
  __impl::calib_kernel(calib_iters * 2);
  const i64 t2 = clock_ns(clock_monotonic);
  if ( t0 < 0 || t1 < 0 || t2 < 0 ) return 0;
  const i64 d = (t2 - t1) - (t1 - t0);
  return d > 0 ? static_cast<u64>(d) : 0;
}

// WARNING: pin the thread before running this
inline u64
calibrate_core_hz(void) noexcept
{
  const i64 w0 = clock_ns(clock_monotonic);
  if ( w0 >= 0 ) {
    for ( ;; ) {
      (void)__calib_sample();
      const i64 wn = clock_ns(clock_monotonic);
      if ( wn < 0 || static_cast<u64>(wn - w0) >= calib_warmup_ns ) break;
    }
  }

  u64 samples[calib_tries];
  u32 got = 0;
  for ( u32 i = 0; i < calib_tries; ++i ) {
    const u64 ns = __calib_sample();
    if ( ns != 0 ) samples[got++] = ns;
  }
  if ( got == 0 ) return 0;

  // WARNING: on a part that boosts, "the highest frequency observed" is the boost clock
  const u64 fastest = __impl::__min_u64(samples, got);
  if ( fastest == 0 ) return 0;
  // calib_iters extra iterations cost calib_iters core cycles
  return (static_cast<u64>(calib_iters) * 1'000'000'000ull) / fastest;
}

struct freq_spread {
  u64 min_hz = 0;
  u64 median_hz = 0;
  u64 max_hz = 0;
  u32 samples = 0;
};

inline freq_spread
core_hz_spread(void) noexcept
{
  freq_spread s{};
  u64 v[calib_tries];
  u32 got = 0;
  for ( u32 i = 0; i < calib_tries; ++i ) {
    const u64 ns = __calib_sample();
    if ( ns != 0 ) v[got++] = (static_cast<u64>(calib_iters) * 1'000'000'000ull) / ns;
  }
  if ( got == 0 ) return s;
  __impl::__sort_u64(v, got);
  s.samples = got;
  s.min_hz = v[0];
  s.median_hz = v[(got - 1) / 2];
  s.max_hz = v[got - 1];
  return s;
}

inline u64
core_hz(void) noexcept
{
  if ( __atomic_load_n(&__impl::__core_hz_state, __ATOMIC_ACQUIRE) != 0u ) [[likely]]
    return __impl::__core_hz_slot;
  u64 hz = calibrate_core_hz();
  if ( hz == 0 ) hz = tick_hz();
  __impl::__core_hz_slot = hz;
  __atomic_store_n(&__impl::__core_hz_state, 1u, __ATOMIC_RELEASE);
  return hz;
}

inline void
set_core_hz(u64 hz) noexcept
{
  __impl::__core_hz_slot = hz;
  __atomic_store_n(&__impl::__core_hz_state, 1u, __ATOMIC_RELEASE);
}

inline void
reset_core_hz(void) noexcept
{
  __atomic_store_n(&__impl::__core_hz_state, 0u, __ATOMIC_RELEASE);
}

struct timer_stats {
  u64 min_ticks = 0;
  u64 median_ticks = 0;
  u64 max_ticks = 0;
  u64 min_ns = 0;
  u64 median_ns = 0;
};

template<serial S = default_serial>
inline timer_stats
timer_overhead(u32 samples = 65) noexcept
{
  constexpr u32 __cap = 257;
  if ( samples > __cap ) samples = __cap;
  if ( samples < 3 ) samples = 3;
  u64 v[__cap];

  for ( u32 i = 0; i < 8; ++i ) {
    const u64 a = tick_start<S>();
    const u64 b = tick_end<S>();
    (void)a;
    (void)b;
  }
  for ( u32 i = 0; i < samples; ++i ) {
    const u64 a = tick_start<S>();
    const u64 b = tick_end<S>();
    v[i] = b > a ? b - a : 0;
  }

  timer_stats s{};
  s.min_ticks = __impl::__min_u64(v, samples);
  __impl::__sort_u64(v, samples);
  s.median_ticks = v[(samples - 1) / 2];
  s.max_ticks = v[samples - 1];
  const u64 hz = tick_hz();
  s.min_ns = cycles_to_ns(s.min_ticks, hz);
  s.median_ns = cycles_to_ns(s.median_ticks, hz);
  return s;
}

template<serial S = default_serial>
inline timer_stats
timer_resolution(u32 samples = 65) noexcept
{
  constexpr u32 __cap = 257;
  if ( samples > __cap ) samples = __cap;
  if ( samples < 3 ) samples = 3;
  u64 v[__cap];
  u32 got = 0;

  for ( u32 i = 0; i < samples * 4 && got < samples; ++i ) {
    const u64 a = tick<S>();
    u64 b = a;
    for ( u32 k = 0; k < 4096 && b == a; ++k ) b = tick<S>();
    if ( b > a ) v[got++] = b - a;
  }
  timer_stats s{};
  if ( got == 0 ) return s;
  s.min_ticks = __impl::__min_u64(v, got);
  __impl::__sort_u64(v, got);
  s.median_ticks = v[(got - 1) / 2];
  s.max_ticks = v[got - 1];
  const u64 hz = tick_hz();
  s.min_ns = cycles_to_ns(s.min_ticks, hz);
  s.median_ns = cycles_to_ns(s.median_ticks, hz);
  return s;
}

inline u64
clock_cost_ns(clockid_t clk, u32 reps = 1000) noexcept
{
  if ( reps == 0 ) reps = 1;
  const i64 t0 = clock_ns(clock_monotonic);
  if ( t0 < 0 ) return 0;
  i64 sink = 0;
  for ( u32 i = 0; i < reps; ++i ) {
    sink += clock_ns(clk);
    __asm__ __volatile__("" : "+r"(sink));
  }
  const i64 t1 = clock_ns(clock_monotonic);
  if ( t1 < 0 || t1 <= t0 ) return 0;
  return static_cast<u64>(t1 - t0) / reps;
}

struct clock_report {
  u64 tick_hz = 0;
  u64 core_hz = 0;
  bool counter_native = false;
  bool counter_invariant = false;
  bool has_rdtscp = false;
  bool has_serialize = false;
  u64 counter_overhead_ticks = 0;
  u64 counter_resolution_ticks = 0;
  u64 realtime_cost_ns = 0;
  u64 monotonic_cost_ns = 0;
  u64 monotonic_coarse_cost_ns = 0;
  u64 monotonic_res_ns = 0;
};

inline clock_report
report(void) noexcept
{
  clock_report r{};
  const tsc_traits t = counter_traits();
  r.tick_hz = tick_hz();
  r.core_hz = core_hz();
  r.counter_native = counter_is_native;
  r.counter_invariant = t.invariant;
  r.has_rdtscp = t.has_rdtscp;
  r.has_serialize = t.has_serialize;
  r.counter_overhead_ticks = timer_overhead<>().min_ticks;
  r.counter_resolution_ticks = timer_resolution<>().min_ticks;
  r.realtime_cost_ns = clock_cost_ns(clock_realtime);
  r.monotonic_cost_ns = clock_cost_ns(clock_monotonic);
  r.monotonic_coarse_cost_ns = clock_cost_ns(clock_monotonic_coarse);
  const i64 res = clock_resolution_ns(clock_monotonic);
  r.monotonic_res_ns = res < 0 ? 0 : static_cast<u64>(res);
  return r;
}

};      // namespace chrono
};      // namespace micron
