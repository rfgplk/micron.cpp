/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 */

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * ONLY TRANSLATION UNIT IN MICRON THAT INCLUDES <linux/...>
 *
 * Compiled as C by kbuild. It includes mc_kport.h and the real kernel headers
 */

#include <linux/delay.h>
#include <linux/irqflags.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <micron/port/backends/__kport_abi.hpp>

/* micron's clock ids, from src/bits/__posix_time_types.hpp. They are UAPI numbers, identical for
 * userspace and for a module. */
#define MC_CLOCK_REALTIME 0
#define MC_CLOCK_MONOTONIC 1
#define MC_CLOCK_PROCESS_CPUTIME 2
#define MC_CLOCK_THREAD_CPUTIME 3
#define MC_CLOCK_MONOTONIC_RAW 4
#define MC_CLOCK_REALTIME_COARSE 5
#define MC_CLOCK_MONOTONIC_COARSE 6
#define MC_CLOCK_BOOTTIME 7

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * panic
 */

#define MC_DIAG_LINE 512

static DEFINE_SPINLOCK(mc_diag_lock);
static char mc_diag_buf[MC_DIAG_LINE];
static unsigned int mc_diag_fill;

#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME "micron"
#endif

static void
mc_diag_emit_locked(void)
{
  if ( mc_diag_fill == 0 ) return;
  printk(KERN_INFO KBUILD_MODNAME ": %.*s\n", (int)mc_diag_fill, mc_diag_buf);
  mc_diag_fill = 0;
}

void
mc_kport_write_diag(const char *s, mc_kport_usize n)
{
  unsigned long flags;
  mc_kport_usize i;

  if ( s == NULL || n == 0 ) return;

  spin_lock_irqsave(&mc_diag_lock, flags);
  for ( i = 0; i < n; i++ ) {
    char c = s[i];

    if ( c == '\n' ) {
      mc_diag_emit_locked();
      continue;
    }
    if ( mc_diag_fill == MC_DIAG_LINE ) mc_diag_emit_locked();
    mc_diag_buf[mc_diag_fill++] = c;
  }
  spin_unlock_irqrestore(&mc_diag_lock, flags);
}

void
mc_kport_write_diag_flush(void)
{
  unsigned long flags;

  spin_lock_irqsave(&mc_diag_lock, flags);
  mc_diag_emit_locked();
  spin_unlock_irqrestore(&mc_diag_lock, flags);
}

void
mc_kport_halt(int code)
{
  mc_kport_write_diag_flush();
  printk(KERN_ERR KBUILD_MODNAME ": halt(%d)\n", code);
  BUG();
  /* BUG() is noreturn on every arch micron targets, but the compiler cannot see that through
   * the macro on all of them, and this function is declared noreturn. */
  for ( ;; ) cpu_relax();
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * pages
 */

void *
mc_kport_page_alloc(mc_kport_usize n, int huge)
{
  (void)huge; /* MAP_HUGETLB has no module-side analogue; kvmalloc picks its own backing */
  if ( n == 0 ) return NULL;
  return kvmalloc(n, GFP_KERNEL);
}

void
mc_kport_page_free(void *p, mc_kport_usize n)
{
  (void)n; /* kvfree recovers the length itself */
  if ( p != NULL ) kvfree(p);
}

int
mc_kport_page_protect(void *p, mc_kport_usize n, int prot)
{
  (void)p;
  (void)n;
  (void)prot;
  return 0;
}

void
mc_kport_page_discard(void *p, mc_kport_usize n)
{
  (void)p;
  (void)n; /* MADV_DONTNEED has no analogue here; advisory everywhere */
}

void
mc_kport_heap_extent(mc_kport_usize *total, mc_kport_usize *freeb)
{
  struct sysinfo si;

  if ( total == NULL || freeb == NULL ) return;
  si_meminfo(&si);
  *total = (mc_kport_usize)si.totalram * PAGE_SIZE;
  *freeb = (mc_kport_usize)si.freeram * PAGE_SIZE;
}

int
mc_kport_addr_readable(const void *p)
{
  if ( p == NULL ) return 0;
  if ( is_vmalloc_addr(p) ) return 1;
  return virt_addr_valid(p) ? 1 : 0;
}

mc_kport_usize
mc_kport_page_size(void)
{
  return (mc_kport_usize)PAGE_SIZE;
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * yield
 */

void
mc_kport_yield(void)
{
  cond_resched();
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * clock
 */

mc_kport_i64
mc_kport_mono_ns(void)
{
  return (mc_kport_i64)ktime_get_ns();
}

mc_kport_i64
mc_kport_real_ns(void)
{
  return (mc_kport_i64)ktime_get_real_ns();
}

mc_kport_i64
mc_kport_cpu_clock_ns(void)
{
  return (mc_kport_i64)(current->utime + current->stime);
}

mc_kport_i64
mc_kport_wall_seconds(void)
{
  return (mc_kport_i64)ktime_get_real_seconds();
}

int
mc_kport_clock_gettime(mc_kport_i32 clk, mc_kport_i64 *sec, mc_kport_i64 *nsec)
{
  struct timespec64 ts;

  if ( sec == NULL || nsec == NULL ) return -EINVAL;

  switch ( clk ) {
  case MC_CLOCK_REALTIME:
    ktime_get_real_ts64(&ts);
    break;
  case MC_CLOCK_MONOTONIC:
    ktime_get_ts64(&ts);
    break;
  case MC_CLOCK_MONOTONIC_RAW:
    ktime_get_raw_ts64(&ts);
    break;
  case MC_CLOCK_REALTIME_COARSE:
    ktime_get_coarse_real_ts64(&ts);
    break;
  case MC_CLOCK_MONOTONIC_COARSE:
    ktime_get_coarse_ts64(&ts);
    break;
  case MC_CLOCK_BOOTTIME:
    ktime_get_boottime_ts64(&ts);
    break;
  case MC_CLOCK_PROCESS_CPUTIME:
  case MC_CLOCK_THREAD_CPUTIME: {
    u64 t = current->utime + current->stime;

    ts.tv_sec = (time64_t)(t / NSEC_PER_SEC);
    ts.tv_nsec = (long)(t % NSEC_PER_SEC);
    break;
  }
  default:
    return -EINVAL;
  }

  *sec = (mc_kport_i64)ts.tv_sec;
  *nsec = (mc_kport_i64)ts.tv_nsec;
  return 0;
}

int
mc_kport_clock_getres(mc_kport_i32 clk, mc_kport_i64 *sec, mc_kport_i64 *nsec)
{
  if ( sec == NULL || nsec == NULL ) return -EINVAL;

  switch ( clk ) {
  case MC_CLOCK_REALTIME_COARSE:
  case MC_CLOCK_MONOTONIC_COARSE:
    *sec = 0;
    *nsec = (mc_kport_i64)(NSEC_PER_SEC / HZ);
    return 0;
  case MC_CLOCK_REALTIME:
  case MC_CLOCK_MONOTONIC:
  case MC_CLOCK_MONOTONIC_RAW:
  case MC_CLOCK_BOOTTIME:
  case MC_CLOCK_PROCESS_CPUTIME:
  case MC_CLOCK_THREAD_CPUTIME:
    *sec = 0;
    *nsec = 1;
    return 0;
  default:
    return -EINVAL;
  }
}

void
mc_kport_sleep_ns(mc_kport_i64 ns)
{
  if ( ns <= 0 ) return;

  if ( irqs_disabled() ) {
    unsigned long us = (unsigned long)(ns / 1000);

    if ( us == 0 )
      ndelay((unsigned long)ns);
    else
      udelay(us > 10000UL ? 10000UL : us);
    return;
  }

  if ( ns < 1000 ) {
    ndelay((unsigned long)ns);
    return;
  }
  if ( ns < 20LL * 1000LL * 1000LL ) {
    unsigned long us = (unsigned long)(ns / 1000);

    usleep_range(us, us + (us >> 3) + 1);
    return;
  }
  msleep((unsigned int)(ns / 1000000LL));
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * ident
 */

mc_kport_i32
mc_kport_exec_id(void)
{
  return (mc_kport_i32)current->pid;
}

mc_kport_i32
mc_kport_process_id(void)
{
  return (mc_kport_i32)current->tgid;
}

mc_kport_i32
mc_kport_cpu_id(void)
{
  return (mc_kport_i32)raw_smp_processor_id();
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * interrupt masking
 */

mc_kport_usize
mc_kport_irq_save(void)
{
  unsigned long flags;
  local_irq_save(flags);
  return (mc_kport_usize)flags;
}

void
mc_kport_irq_restore(mc_kport_usize state)
{
  local_irq_restore((unsigned long)state);
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * wait
 *
 * hashed on the address, such that two unrelated words can share a queue
 */

#define MC_WAIT_SLOTS 64

static wait_queue_head_t mc_wait_q[MC_WAIT_SLOTS];
static int mc_wait_ready;

static inline unsigned int
mc_wait_slot(const void *addr)
{
  unsigned long v = (unsigned long)addr;

  return (unsigned int)((v >> 2) & (MC_WAIT_SLOTS - 1));
}

mc_kport_i64
mc_kport_wait(mc_kport_u32 *addr, mc_kport_u32 expected, mc_kport_i64 timeout_ns)
{
  wait_queue_head_t *q;
  long rc;

  if ( addr == NULL ) return -EINVAL;
  if ( !READ_ONCE(mc_wait_ready) ) return -ENOSYS;

  q = &mc_wait_q[mc_wait_slot(addr)];

  if ( READ_ONCE(*addr) != expected ) return -EAGAIN;

  if ( timeout_ns < 0 ) {
    rc = wait_event_interruptible(*q, READ_ONCE(*addr) != expected);
    return (rc != 0) ? -EINTR : 0;
  }

  rc = wait_event_interruptible_timeout(*q, READ_ONCE(*addr) != expected, nsecs_to_jiffies((u64)timeout_ns));
  if ( rc == 0 ) return -ETIMEDOUT;
  if ( rc < 0 ) return -EINTR;
  return 0;
}

mc_kport_i64
mc_kport_wake(mc_kport_u32 *addr, mc_kport_i32 n)
{
  wait_queue_head_t *q;

  if ( addr == NULL ) return -EINVAL;
  if ( !READ_ONCE(mc_wait_ready) ) return -ENOSYS;

  q = &mc_wait_q[mc_wait_slot(addr)];
  if ( n < 0 ) {
    wake_up_all(q);
    return n;
  }
  if ( n > 0 ) wake_up_nr(q, (int)n);
  return n;
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * rawmap
 */

void *
mc_kport_raw_map(mc_kport_usize n)
{
  if ( n == 0 ) return NULL;
  return vmalloc(n);
}

void
mc_kport_raw_unmap(void *p, mc_kport_usize n)
{
  (void)n;
  if ( p != NULL ) vfree(p);
}

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * lifecycle
 */

int
mc_kport_init(void)
{
  unsigned int i;

  for ( i = 0; i < MC_WAIT_SLOTS; i++ ) init_waitqueue_head(&mc_wait_q[i]);
  smp_wmb();
  WRITE_ONCE(mc_wait_ready, 1);

  mc_diag_fill = 0;
  return 0;
}

void
mc_kport_fini(void)
{
  WRITE_ONCE(mc_wait_ready, 0);
  mc_kport_write_diag_flush();
}
