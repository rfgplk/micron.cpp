//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// WEAK DEFAULTS

#include <micron/port/backends/__mport_abi.hpp>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// RAM, from the linker script
extern "C" char __heap_start[];
extern "C" char __heap_end[];

extern "C" {

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// console

__attribute__((weak)) void
mc_mport_write_diag(const char *__s, mc_mport_usize __n)
{
  (void)__s;
  (void)__n;
}

__attribute__((weak, noreturn)) void
mc_mport_halt(int __code)
{
  (void)__code;
  for ( ;; ) __asm__ __volatile__("" ::: "memory");
}

__attribute__((weak)) void
mc_mport_heap(void **__base, mc_mport_usize *__len)
{
  if ( __base == nullptr || __len == nullptr ) return;
  *__base = static_cast<void *>(__heap_start);
  *__len = static_cast<mc_mport_usize>(__heap_end - __heap_start);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// clock

static mc_mport_i64 __mport_tick = 0;

__attribute__((weak)) mc_mport_i64
mc_mport_mono_ns(void)
{
  mc_mport_usize __st = mc_mport_irq_save();
  mc_mport_i64 __v = ++__mport_tick;
  mc_mport_irq_restore(__st);
  return __v;
}

__attribute__((weak)) mc_mport_i64
mc_mport_real_ns(void)
{
  return mc_mport_mono_ns() + 1420070400000000000LL;      // 2015-01-01T00:00:00Z, in ns
}

__attribute__((weak)) mc_mport_i32
mc_mport_cpu_id(void)
{
  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// interrupt masking

__attribute__((weak)) mc_mport_usize
mc_mport_irq_save(void)
{
  return 0;
}

__attribute__((weak)) void
mc_mport_irq_restore(mc_mport_usize __state)
{
  (void)__state;
}

};      // extern "C"
