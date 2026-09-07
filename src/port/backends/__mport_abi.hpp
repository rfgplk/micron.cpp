/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 */
#pragma once

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * BARE-METAL PORT ABI
 */

typedef __SIZE_TYPE__ mc_mport_usize;
typedef __UINTPTR_TYPE__ mc_mport_addr;
typedef __INT64_TYPE__ mc_mport_i64;
typedef __INT32_TYPE__ mc_mport_i32;
typedef __UINT32_TYPE__ mc_mport_u32;

#if defined(__cplusplus)
extern "C" {
#endif

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * the console
 */

void mc_mport_write_diag(const char *__s, mc_mport_usize __n);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * halt
 */

void mc_mport_halt(int __code) __attribute__((noreturn));

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * RAM
 */

void mc_mport_heap(void **__base, mc_mport_usize *__len);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * the clock
 */

mc_mport_i64 mc_mport_mono_ns(void);
mc_mport_i64 mc_mport_real_ns(void);

mc_mport_i32 mc_mport_cpu_id(void);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * interrupt masking
 */

mc_mport_usize mc_mport_irq_save(void);
void mc_mport_irq_restore(mc_mport_usize __state);

#if defined(__cplusplus)
}
#endif
