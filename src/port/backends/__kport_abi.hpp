/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 */
#pragma once

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * KERNEL PORT ABI
 */

typedef __SIZE_TYPE__ mc_kport_usize;
typedef __UINTPTR_TYPE__ mc_kport_addr;
typedef __INT64_TYPE__ mc_kport_i64;
typedef __INT32_TYPE__ mc_kport_i32;
typedef __UINT32_TYPE__ mc_kport_u32;

#if defined(__cplusplus)
extern "C" {
#endif

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * lifecycle
 */

int mc_kport_init(void);
void mc_kport_fini(void);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * panic: printk and BUG()
 */

void mc_kport_write_diag(const char *__s, mc_kport_usize __n);

void mc_kport_write_diag_flush(void);
void mc_kport_halt(int __code) __attribute__((noreturn));

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * pages
 */

void *mc_kport_page_alloc(mc_kport_usize __n, int __huge);
void mc_kport_page_free(void *__p, mc_kport_usize __n);

int mc_kport_page_protect(void *__p, mc_kport_usize __n, int __prot);
void mc_kport_page_discard(void *__p, mc_kport_usize __n);

void mc_kport_heap_extent(mc_kport_usize *__total, mc_kport_usize *__free);

int mc_kport_addr_readable(const void *__p);

mc_kport_usize mc_kport_page_size(void);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * yield
 */

void mc_kport_yield(void);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * clock: nanoseconds
 */

mc_kport_i64 mc_kport_mono_ns(void);
mc_kport_i64 mc_kport_real_ns(void);
mc_kport_i64 mc_kport_cpu_clock_ns(void);
mc_kport_i64 mc_kport_wall_seconds(void);

int mc_kport_clock_gettime(mc_kport_i32 __clk, mc_kport_i64 *__sec, mc_kport_i64 *__nsec);
int mc_kport_clock_getres(mc_kport_i32 __clk, mc_kport_i64 *__sec, mc_kport_i64 *__nsec);

void mc_kport_sleep_ns(mc_kport_i64 __ns);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * ident
 */

mc_kport_i32 mc_kport_exec_id(void);    /* current->pid  */
mc_kport_i32 mc_kport_process_id(void); /* current->tgid */

mc_kport_i32 mc_kport_cpu_id(void);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * interrupt masking
 * CURRENT CPU ONLY
 */

mc_kport_usize mc_kport_irq_save(void);
void mc_kport_irq_restore(mc_kport_usize __state);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * wait
 */

mc_kport_i64 mc_kport_wait(mc_kport_u32 *__addr, mc_kport_u32 __expected, mc_kport_i64 __timeout_ns);
mc_kport_i64 mc_kport_wake(mc_kport_u32 *__addr, mc_kport_i32 __n);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * rawmap
 */

void *mc_kport_raw_map(mc_kport_usize __n);
void mc_kport_raw_unmap(void *__p, mc_kport_usize __n);

#if defined(__cplusplus)
}
#endif
