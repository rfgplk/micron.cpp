//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// interrupt masking, on bare metal

#include "__mport_abi.hpp"

#include "../../bits/__arch.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

using irq_state = usize;

#if defined(__micron_arch_x86_any)

[[gnu::always_inline]] inline irq_state
irq_save(void) noexcept
{
  irq_state __s;
#if defined(__micron_arch_width_64)
  __asm__ __volatile__("pushfq\n\tpopq %0\n\tcli" : "=r"(__s) : : "memory");
#else
  __asm__ __volatile__("pushfl\n\tpopl %0\n\tcli" : "=r"(__s) : : "memory");
#endif
  return __s;
}

[[gnu::always_inline]] inline void
irq_restore(irq_state __state) noexcept
{
#if defined(__micron_arch_width_64)
  __asm__ __volatile__("pushq %0\n\tpopfq" : : "r"(__state) : "memory", "cc");
#else
  __asm__ __volatile__("pushl %0\n\tpopfl" : : "r"(__state) : "memory", "cc");
#endif
}

#elif defined(__micron_arch_arm64)

[[gnu::always_inline]] inline irq_state
irq_save(void) noexcept
{
  irq_state __s;
  __asm__ __volatile__("mrs %0, daif\n\tmsr daifset, #2" : "=r"(__s) : : "memory");
  return __s;
}

[[gnu::always_inline]] inline void
irq_restore(irq_state __state) noexcept
{
  __asm__ __volatile__("msr daif, %0" : : "r"(__state) : "memory");
}

#elif defined(__micron_arch_arm32) && defined(__ARM_ARCH_PROFILE) && (__ARM_ARCH_PROFILE == 'M')

[[gnu::always_inline]] inline irq_state
irq_save(void) noexcept
{
  irq_state __s;
  __asm__ __volatile__("mrs %0, primask\n\tcpsid i" : "=r"(__s) : : "memory");
  return __s;
}

[[gnu::always_inline]] inline void
irq_restore(irq_state __state) noexcept
{
  __asm__ __volatile__("msr primask, %0" : : "r"(__state) : "memory");
}

#elif defined(__micron_arch_arm32)

[[gnu::always_inline]] inline irq_state
irq_save(void) noexcept
{
  irq_state __s;
  __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i" : "=r"(__s) : : "memory");
  return __s;
}

[[gnu::always_inline]] inline void
irq_restore(irq_state __state) noexcept
{
  __asm__ __volatile__("msr cpsr_c, %0" : : "r"(__state) : "memory");
}

#else

inline irq_state
irq_save(void) noexcept
{
  return static_cast<irq_state>(::mc_mport_irq_save());
}

inline void
irq_restore(irq_state __state) noexcept
{
  ::mc_mport_irq_restore(static_cast<mc_mport_usize>(__state));
}

#endif

};      // namespace port
};      // namespace micron
