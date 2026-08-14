//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../bits/__cpuid.hpp"
#include "../types.hpp"

#include "clock.hpp"
#include "units.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cycles counter (spooky?!)
//
// NOTE: a TSC delta doesn't represent core cycles

namespace micron
{
namespace chrono
{

// NOTE: the first three are semantic levels, not instruction names; we use amd64 spelling for convenience, other arches use their own
// respective insts
// ..none           no barrier at all
// ..lfence         serialise the instruction stream    x86: lfence          arm: isb
// ..mfence_lfence  drain memory, then serialise        x86: mfence+lfence   arm: dsb sy + isb
//
enum class serial : u8 {
  none,               // bare counter read
  lfence,             // serialise around it
  mfence_lfence,      // drain the store buffer first, then serialise
  cpuid,              // x86: fully serialising, EXPENSIVE and VM-exits
  rdtscp,             // x86: waits for prior instructions to retire; also yields the cpu id
  serialize,          // x86: serialize instruction, (Sapphire Rapids and later)
  isb,                // arm: instruction-synchronisation barrier
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// what this build actually has

#if defined(__micron_arch_x86_any)
inline constexpr bool counter_is_tsc = true;
inline constexpr bool counter_is_native = true;
inline constexpr serial default_serial = serial::lfence;
#elif defined(__micron_arch_arm64)
inline constexpr bool counter_is_tsc = false;
inline constexpr bool counter_is_native = true;
inline constexpr serial default_serial = serial::isb;
#elif defined(__micron_arch_arm32) && defined(MICRON_CHRONO_ARM32_CNTVCT)
inline constexpr bool counter_is_tsc = false;
inline constexpr bool counter_is_native = true;
inline constexpr serial default_serial = serial::isb;
#else
inline constexpr bool counter_is_tsc = false;
inline constexpr bool counter_is_native = false;
inline constexpr serial default_serial = serial::none;
#endif

template<serial S>
inline constexpr bool serial_supported = (S == serial::none) || (S == serial::lfence) || (S == serial::mfence_lfence)
#if defined(__micron_arch_x86_any)
                                         || (S == serial::cpuid) || (S == serial::rdtscp)
#if defined(__micron_x86_serialize)
                                         || (S == serial::serialize)
#endif
#elif defined(__micron_arch_arm_any)
                                         || (S == serial::isb)
#endif
    ;

// %%%%%%%%%%%%%%%%%%%%%%%
// fences

[[gnu::always_inline]] inline void
fence_compiler() noexcept
{
  __asm__ __volatile__("" ::: "memory");
}

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline void
fence_load() noexcept
{
  __asm__ __volatile__("lfence" ::: "memory");
}

[[gnu::always_inline]] inline void
fence_mem() noexcept
{
  __asm__ __volatile__("mfence" ::: "memory");
}

[[gnu::always_inline]] inline void
fence_store() noexcept
{
  __asm__ __volatile__("sfence" ::: "memory");
}

// WARNING: cpuid clobbers eax/ebx/ecx/edx and costs 100+ cycles with wide variance
[[gnu::always_inline]] inline void
fence_serialising() noexcept
{
#if defined(__micron_x86_serialize)
  __asm__ __volatile__("serialize" ::: "memory");
#else
  (void)micron::__cpuid_read(0u);
  __asm__ __volatile__("" ::: "memory");
#endif
}
#elif defined(__micron_arch_arm_any)
[[gnu::always_inline]] inline void
fence_load() noexcept
{
  __asm__ __volatile__("isb" ::: "memory");
}

[[gnu::always_inline]] inline void
fence_mem() noexcept
{
  __asm__ __volatile__("dsb sy" ::: "memory");
}

[[gnu::always_inline]] inline void
fence_store() noexcept
{
  __asm__ __volatile__("dsb st" ::: "memory");
}

[[gnu::always_inline]] inline void
fence_serialising() noexcept
{
  __asm__ __volatile__("dsb sy\n\tisb" ::: "memory");
}
#else
[[gnu::always_inline]] inline void
fence_load() noexcept
{
  fence_compiler();
}

[[gnu::always_inline]] inline void
fence_mem() noexcept
{
  fence_compiler();
}

[[gnu::always_inline]] inline void
fence_store() noexcept
{
  fence_compiler();
}

[[gnu::always_inline]] inline void
fence_serialising() noexcept
{
  fence_compiler();
}
#endif

namespace __impl
{

[[gnu::always_inline]] inline u64
__tick_bare() noexcept
{
#if defined(__micron_arch_x86_any)
  u32 lo = 0, hi = 0;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
#elif defined(__micron_arch_arm64)
  u64 v = 0;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#elif defined(__micron_arch_arm32) && defined(MICRON_CHRONO_ARM32_CNTVCT)
  u32 lo = 0, hi = 0;
  __asm__ __volatile__("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
  return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
#else
  // the counter is nanoseconds here, and tick_hz() says so
  const i64 v = clock_ns(clock_monotonic_raw);
  return v < 0 ? 0ull : static_cast<u64>(v);
#endif
}

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline u64
__tick_p(u32 &aux) noexcept
{
  u32 lo = 0, hi = 0, a = 0;
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(a));
  aux = a;
  return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
}
#endif

template<serial S>
[[gnu::always_inline]] inline void
__fence_before() noexcept
{
  if constexpr ( S == serial::none ) {
    fence_compiler();
  } else if constexpr ( S == serial::mfence_lfence ) {
    fence_mem();
    fence_load();
  } else if constexpr ( S == serial::cpuid || S == serial::serialize ) {
    fence_serialising();
  } else if constexpr ( S == serial::rdtscp ) {
    fence_compiler();
  } else {
    fence_load();
  }
}

};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// public reads
//
// tick()        fence, read          cheapest
// tick_start()  fence, read, fence   region cannot begin before the read retires
// tick_end()    fence, read, fence   region cannot leak past the read

template<serial S = default_serial>
[[gnu::always_inline]] inline u64
tick() noexcept
{
  static_assert(serial_supported<S>, "micron::chrono: that serialisation policy is not available on this target");
#if defined(__micron_arch_x86_any)
  if constexpr ( S == serial::rdtscp ) {
    u32 aux = 0;
    __impl::__fence_before<S>();
    return __impl::__tick_p(aux);
  } else
#endif
  {
    __impl::__fence_before<S>();
    return __impl::__tick_bare();
  }
}

template<serial S = default_serial>
[[gnu::always_inline]] inline u64
tick_start() noexcept
{
  const u64 v = tick<S>();
  fence_load();
  return v;
}

template<serial S = default_serial>
[[gnu::always_inline]] inline u64
tick_end() noexcept
{
  fence_load();
  const u64 v = tick<S>();
  fence_load();
  return v;
}

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline u64
tick_aux(u32 &cpu_node) noexcept
{
  fence_load();
  return __impl::__tick_p(cpu_node);
}

// IA32_TSC_AUX is loaded by Linux with (numa_node << 12) | cpu
inline constexpr u32
aux_cpu(u32 aux) noexcept
{
  return aux & 0xFFFu;
}

inline constexpr u32
aux_node(u32 aux) noexcept
{
  return aux >> 12;
}
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rdpmc
// (instruction only)
//
// WARNING: this faults unless /sys/bus/event_source/devices/cpu/rdpmc allows it && the counter is actually scheduled
// never call it speculatively

#if defined(__micron_arch_x86_any)
[[gnu::always_inline]] inline u64
rdpmc(u32 counter) noexcept
{
  u32 lo = 0, hi = 0;
  __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(counter));
  return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
}

inline constexpr u64
pmc_delta(u64 now, u64 then, u32 width) noexcept
{
  if ( width == 0 || width >= 64 ) return now - then;
  const u64 mask = (1ull << width) - 1ull;
  return (now - then) & mask;
}
#endif

};      // namespace chrono
};      // namespace micron
