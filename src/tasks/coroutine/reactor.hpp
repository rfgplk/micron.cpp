//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define __micron_coro_reactor_seen

#if defined(MICRON_CORO_URING)

#include "../../atomic/atomic.hpp"
#include "../../kernel.hpp"
#include "../../linux/sys/uring.hpp"
#include "../../memory/mman.hpp"
#include "../../sync/yield.hpp"
#include "../../types.hpp"

#include "../coro_core.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// io_uring reactor state (-DMICRON_CORO_URING) - per-worker rings
//
// NOTE: NOT libjkr
//
// enabled via -D/--def MICRON_CORO_URING
//
// (1.9.1) each engine worker owns one ring;
// initialized on the worker thread (NUMA type first touch)
// submission to the rings is lockfree

namespace micron
{
namespace coro
{

#ifndef MICRON_CORO_URING_ENTRIES
#define MICRON_CORO_URING_ENTRIES 256u
#endif

inline constexpr u32 __io_sq_entries = MICRON_CORO_URING_ENTRIES;
inline constexpr u32 __io_fb_entries = 64u;

// [63..56] tag, [55..0] payload (pointer or worker id)
inline constexpr u64 __io_ud_tag_shift = 56;
inline constexpr u8 __io_ud_op = 0x00;           // payload = __io_op*
inline constexpr u8 __io_ud_park = 0x01;         // payload = worker id (own-ring futex_wait park sqe)
inline constexpr u8 __io_ud_cancel = 0x02;       // async_cancel companion cqe; dropped
inline constexpr u8 __io_ud_ltimer = 0x03;       // link_timeout companion cqe; dropped
inline constexpr u8 __io_ud_mop = 0x04;          // payload = __io_mop* (multishot recv; many cqes per op)
inline constexpr u8 __io_ud_reclaim = 0xfe;      // always ignored

[[gnu::always_inline]] inline constexpr u64
__io_ud_payload(u64 __ud) noexcept
{
  return __ud & ((1ull << __io_ud_tag_shift) - 1ull);
}

[[gnu::always_inline]] inline constexpr u64
__io_ud_make(u8 __tag, u64 __payload) noexcept
{
  return (static_cast<u64>(__tag) << __io_ud_tag_shift) | __payload;
}

[[gnu::always_inline]] inline constexpr u8
__io_ud_tag(u64 __ud) noexcept
{
  return static_cast<u8>(__ud >> __io_ud_tag_shift);
}

// op completion handshake states
inline constexpr u32 __io_st_submitted = 0;
inline constexpr u32 __io_st_suspended = 1;
inline constexpr u32 __io_st_done_early = 2;

struct __io_op {
  __frame_base *__f = nullptr;
  micron::atomic_token<u32> __st{ __io_st_submitted };
  i32 __res = 0;
};

#if defined(MICRON_CORO_STATS)
struct __io_ring_stats {
  micron::atomic_token<u64> submits{ 0 };
  micron::atomic_token<u64> enters{ 0 };
  micron::atomic_token<u64> inline_completions{ 0 };
  micron::atomic_token<u64> sqe_full_flushes{ 0 };
  micron::atomic_token<u64> parks{ 0 };
  micron::atomic_token<u64> wakes{ 0 };
  micron::atomic_token<u64> cancels{ 0 };
};
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multishot recv
//
// per worker ring buffer pool;
// user allocator, initialized lazily by the rings owner thread on first touch
// recycle() called by consumer coros

#ifndef MICRON_CORO_PBUF_ENTRIES
#define MICRON_CORO_PBUF_ENTRIES 256u
#endif
#ifndef MICRON_CORO_PBUF_SZ
#define MICRON_CORO_PBUF_SZ 8192u
#endif
inline constexpr u32 __io_pb_entries = MICRON_CORO_PBUF_ENTRIES;
inline constexpr u32 __io_pb_sz = MICRON_CORO_PBUF_SZ;
inline constexpr u16 __io_pb_bgid = 7;
static_assert((__io_pb_entries & (__io_pb_entries - 1)) == 0, "MICRON_CORO_PBUF_ENTRIES must be a power of two");

struct __io_pbuf {
  micron::uring::buf_ring *__br = nullptr;
  byte *__arena = nullptr;
  u32 __tail_shadow = 0;
  micron::atomic_token<u32> __lk{ 0 };
  micron::atomic_token<u32> __state{ 0 };      // 0 untried / 1 live / 2 refused
};

inline constexpr u8 __io_mev_buf = 1u << 0;       // a provided buffer is attached (bid valid)
inline constexpr u8 __io_mev_more = 1u << 1;      // the op is still armed after this event

struct __io_mev {
  i32 __res = 0;      // >0 bytes in the buffer; 0 peer EOF; <0 -errno
  u16 __bid = 0;
  u8 __ring = 0xff;      // worker ring the buffer belongs to
  u8 __fl = 0;
};

inline constexpr u32 __io_mop_qcap = 2u * __io_pb_entries;

inline constexpr u32 __io_mop_idle = 0;
// consumer is inside await_suspend; owns the frame
inline constexpr u32 __io_mop_parking = 1;
inline constexpr u32 __io_mop_parked = 2;
// completed while the consumer was parking
inline constexpr u32 __io_mop_pending = 3;

struct __io_mop {
  __frame_base *__f = nullptr;
  micron::atomic_token<u32> __st{ __io_mop_idle };
  micron::atomic_token<u32> __live{ 0 };      // armed and the kernel still promises more
  micron::atomic_token<u32> __qh{ 0 };
  micron::atomic_token<u32> __qt{ 0 };
  u8 __ring = 0xff;      // worker ring the op is armed on
  __io_mev __q[__io_mop_qcap];

  [[nodiscard]] bool
  __pop(__io_mev &__out) noexcept      // single consumer
  {
    const u32 __h = __qh.get(micron::memory_order_relaxed);
    if ( __qt.get(micron::memory_order_acquire) == __h ) return false;
    __out = __q[__h & (__io_mop_qcap - 1u)];
    __qh.store(__h + 1u, micron::memory_order_release);
    return true;
  }

  void
  __push(const __io_mev &__e) noexcept      // dispatcher only; serialized by the ring's __cq_lk
  {
    const u32 __t = __qt.get(micron::memory_order_relaxed);
    if ( __t - __qh.get(micron::memory_order_acquire) >= __io_mop_qcap ) [[unlikely]]
      __builtin_trap();      // see the capacity note above
    __q[__t & (__io_mop_qcap - 1u)] = __e;
    // WARNING: must be seq_cst, not release; will cause cross read deadlocks otherwise
    __qt.store(__t + 1u, micron::memory_order_seq_cst);
  }
};

struct alignas(64) __wring {
  micron::uring::ring __r;
  micron::atomic_token<u32> __sq_lk{ 0 };           // fallback ring only
  micron::atomic_token<u32> __cq_lk{ 0 };           // one reaper at a time
  micron::atomic_token<u32> __pending{ 0 };         // in-flight ops
  micron::atomic_token<u32> __park_fired{ 0 };      // this ring's park sqe cqe observed
  micron::atomic_token<u32> __live{ 0 };            // init succeeded
  micron::atomic_token<u32> __bufs_reg{ 0 };        // fixed-buffer slab registered into this ring (0 no / 1 yes / 2 refused)
  u8 __defer = 0;                                   // ring came up with setup_defer_taskrun (n==1 mode)
  u32 __staged = 0;
  __io_pbuf __pb;
#if defined(MICRON_CORO_STATS)
  __io_ring_stats __stat;
#endif
};

inline __wring __io_rings[32];
inline __wring __io_fb;

struct __io_state {
  micron::atomic_token<u32> any_live{ 0 };      // >=1 ring
  micron::atomic_token<u32> futex_ok{ 0 };      // (>=6.7)
  micron::atomic_token<i32> watcher{ -1 };
};

inline __io_state __io;

[[gnu::always_inline]] inline void
__io_lock(micron::atomic_token<u32> &__l) noexcept
{
  u32 __e = 0;
  while ( !__l.compare_exchange_weak(__e, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __e = 0;
    micron::cpu_pause<1>();
  }
}

[[gnu::always_inline]] inline bool
__io_trylock(micron::atomic_token<u32> &__l) noexcept
{
  u32 __e = 0;
  return __l.compare_exchange_strong(__e, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire);
}

[[gnu::always_inline]] inline void
__io_unlock(micron::atomic_token<u32> &__l) noexcept
{
  __l.store(0u, micron::memory_order_release);
}

[[gnu::always_inline]] inline bool
__io_cq_acquire(__wring &__wr) noexcept
{
  if ( __wr.__live.get(micron::memory_order_acquire) == 0 ) return false;
  if ( !__io_trylock(__wr.__cq_lk) ) return false;
  if ( __wr.__live.get(micron::memory_order_acquire) == 0 ) [[unlikely]] {      // died between the test and the lock
    __io_unlock(__wr.__cq_lk);
    return false;
  }
  return true;
}

[[gnu::always_inline]] inline __wring *
__io_own_ring() noexcept
{
  worker *__w = current_worker();
  if ( __w == nullptr ) return nullptr;
  __wring &__wr = __io_rings[__w->id];
  return __wr.__live.get(micron::memory_order_acquire) != 0 ? &__wr : nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// staged submissions (-DMICRON_CORO_STAGED_SUBMIT)

#if defined(MICRON_CORO_STAGED_SUBMIT)
inline constexpr u32 __io_stage_max = 16;
#endif

[[gnu::always_inline]] inline void
__io_flush_staged(__wring &__wr) noexcept
{
#if defined(MICRON_CORO_STAGED_SUBMIT)
  if ( __wr.__staged != 0 ) {
    __wr.__staged = 0;
    (void)__wr.__r.enter(0);
#if defined(MICRON_CORO_STATS)
    __wr.__stat.enters.fetch_add(1, micron::memory_order_relaxed);
#endif
  }
#else
  (void)__wr;
#endif
}

[[gnu::always_inline]] inline void
__io_flush_own_staged() noexcept
{
#if defined(MICRON_CORO_STAGED_SUBMIT)
  __wring *__r = __io_own_ring();
  if ( __r != nullptr ) __io_flush_staged(*__r);
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// provided-buffer pool

inline i32
__io_pb_init(__wring &__wr) noexcept
{
  const u32 __st = __wr.__pb.__state.get(micron::memory_order_acquire);
  if ( __st == 1u ) return 0;
  if ( __st == 2u ) return -95;      // EOPNOTSUPP: refused earlier
  const usize __rb = static_cast<usize>(__io_pb_entries) * sizeof(micron::uring::buf);
  auto *__br = reinterpret_cast<micron::uring::buf_ring *>(
      micron::mmap(nullptr, __rb, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0));
  if ( micron::mmap_failed(reinterpret_cast<addr_t *>(__br)) ) {
    __wr.__pb.__state.store(2u, micron::memory_order_release);
    return -95;
  }
  byte *__ar
      = reinterpret_cast<byte *>(micron::mmap(nullptr, static_cast<usize>(__io_pb_entries) * __io_pb_sz,
                                              micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0));
  if ( micron::mmap_failed(reinterpret_cast<addr_t *>(__ar)) ) {
    micron::munmap(reinterpret_cast<addr_t *>(__br), __rb);
    __wr.__pb.__state.store(2u, micron::memory_order_release);
    return -95;
  }
  micron::uring::buf_reg __reg{};
  __reg.ring_addr = reinterpret_cast<u64>(__br);
  __reg.ring_entries = __io_pb_entries;
  __reg.bgid = __io_pb_bgid;
  if ( __wr.__r.register_pbuf_ring(__reg) < 0 ) {
    micron::munmap(reinterpret_cast<addr_t *>(__br), __rb);
    micron::munmap(reinterpret_cast<addr_t *>(__ar), static_cast<usize>(__io_pb_entries) * __io_pb_sz);
    __wr.__pb.__state.store(2u, micron::memory_order_release);
    return -95;
  }
  for ( u32 __i = 0; __i < __io_pb_entries; ++__i ) {
    micron::uring::buf *__b = &__br->bufs()[__i];
    __b->addr = reinterpret_cast<u64>(__ar + static_cast<usize>(__i) * __io_pb_sz);
    __b->len = __io_pb_sz;
    __b->bid = static_cast<u16>(__i);
  }
  __wr.__pb.__br = __br;
  __wr.__pb.__arena = __ar;
  __wr.__pb.__tail_shadow = __io_pb_entries;
  micron::atom::store(&__br->tail, static_cast<u16>(__io_pb_entries), __ATOMIC_RELEASE);
  __wr.__pb.__state.store(1u, micron::memory_order_release);      // publishes __br/__arena
  return 0;
}

inline constexpr u8 __io_ring_cap = static_cast<u8>(sizeof(__io_rings) / sizeof(__io_rings[0]));

// __ring is 0xff on every synthesized event
[[gnu::always_inline]] inline byte *
__io_pb_data(u8 __ring, u16 __bid) noexcept
{
  if ( __ring >= __io_ring_cap || __bid >= __io_pb_entries ) [[unlikely]] return nullptr;
  byte *__a = __io_rings[__ring].__pb.__arena;
  return __a == nullptr ? nullptr : __a + static_cast<usize>(__bid) * __io_pb_sz;
}

inline void
__io_pb_recycle(u8 __ring, u16 __bid) noexcept
{
  if ( __ring >= __io_ring_cap || __bid >= __io_pb_entries ) [[unlikely]] return;
  __wring &__wr = __io_rings[__ring];
  if ( __wr.__pb.__state.get(micron::memory_order_acquire) != 1u ) return;
  __io_lock(__wr.__pb.__lk);
  if ( __wr.__pb.__state.get(micron::memory_order_acquire) != 1u ) [[unlikely]] {      // died between the test and the lock
    __io_unlock(__wr.__pb.__lk);
    return;
  }
  micron::uring::buf *__b = &__wr.__pb.__br->bufs()[__wr.__pb.__tail_shadow & (__io_pb_entries - 1u)];
  __b->addr = reinterpret_cast<u64>(__wr.__pb.__arena + static_cast<usize>(__bid) * __io_pb_sz);
  __b->len = __io_pb_sz;
  __b->bid = __bid;
  ++__wr.__pb.__tail_shadow;
  micron::atom::store(&__wr.__pb.__br->tail, static_cast<u16>(__wr.__pb.__tail_shadow), __ATOMIC_RELEASE);
  __io_unlock(__wr.__pb.__lk);
}

inline void
__io_pb_shutdown(__wring &__wr) noexcept      // owner thread, before the ring fd closes
{
  if ( __wr.__pb.__state.get(micron::memory_order_acquire) != 1u ) return;
  __wr.__pb.__state.store(0u, micron::memory_order_release);
  __io_lock(__wr.__pb.__lk);
  __io_unlock(__wr.__pb.__lk);
  (void)__wr.__r.unregister_pbuf_ring(__io_pb_bgid);
  micron::munmap(reinterpret_cast<addr_t *>(__wr.__pb.__br), static_cast<usize>(__io_pb_entries) * sizeof(micron::uring::buf));
  micron::munmap(reinterpret_cast<addr_t *>(__wr.__pb.__arena), static_cast<usize>(__io_pb_entries) * __io_pb_sz);
  __wr.__pb.__br = nullptr;
  __wr.__pb.__arena = nullptr;
  __wr.__pb.__tail_shadow = 0;
}

inline u32
__io_ring_flags(u32 __nworkers) noexcept
{
  // NOTE: we don't do setup_single_issuer/defer_taskrun
  // kernel rejects io_uring_register from any other task on such rings;
  // would break crossthread SYNC_CANCELLing
  // submission must stay single producer
  (void)__nworkers;
  return micron::uring::setup_no_sqarray;
}

inline void
__io_worker_ring_init(u32 __id, u32 __nworkers) noexcept
{
  __wring &__wr = __io_rings[__id];
  if ( __wr.__r.init_best(__io_sq_entries, __io_ring_flags(__nworkers)) != 0 ) return;
  __wr.__defer = (__wr.__r.setup_flags & micron::uring::setup_defer_taskrun) != 0 ? 1 : 0;
  (void)__wr.__r.register_ring_fd();
  __wr.__pending.store(0, micron::memory_order_relaxed);
  __wr.__park_fired.store(0, micron::memory_order_relaxed);
  __wr.__live.store(1, micron::memory_order_release);
  __io.any_live.store(1, micron::memory_order_release);
}

inline void
__io_worker_ring_shutdown(u32 __id) noexcept
{
  __wring &__wr = __io_rings[__id];
  if ( __wr.__live.get(micron::memory_order_acquire) == 0 ) return;
  __io_flush_staged(__wr);
  __io_pb_shutdown(__wr);      // unregister before the ring fd closes

  __wr.__live.store(0, micron::memory_order_release);
  __io_lock(__wr.__cq_lk);
  __wr.__r.shutdown();
  __wr.__defer = 0;
  __wr.__bufs_reg.store(0, micron::memory_order_relaxed);      // the slab is reregistered into the next ring on first tocuh
  __io_unlock(__wr.__cq_lk);
}

inline void
__io_fb_init() noexcept
{
  if ( __io_fb.__r.init_best(__io_fb_entries, 0) != 0 ) return;
  __io_fb.__pending.store(0, micron::memory_order_relaxed);
  __io_fb.__live.store(1, micron::memory_order_release);
  __io.any_live.store(1, micron::memory_order_release);
}

inline void
__io_fb_shutdown() noexcept
{
  if ( __io_fb.__live.get(micron::memory_order_acquire) == 0 ) return;
  __io_fb.__live.store(0, micron::memory_order_release);
  __io_lock(__io_fb.__sq_lk);
  __io_lock(__io_fb.__cq_lk);
  __io_fb.__r.shutdown();
  __io_fb.__defer = 0;
  __io_fb.__bufs_reg.store(0, micron::memory_order_relaxed);
  __io_unlock(__io_fb.__cq_lk);
  __io_unlock(__io_fb.__sq_lk);
}

inline bool
__io_submit_own(__wring &__wr, const micron::uring::sqe &__q, u64 __ud) noexcept
{
  micron::uring::sqe *__s = __wr.__r.get_sqe();
  if ( __s == nullptr ) [[unlikely]] {
#if defined(MICRON_CORO_STATS)
    __wr.__stat.sqe_full_flushes.fetch_add(1, micron::memory_order_relaxed);
#endif
    __wr.__staged = 0;
    (void)__wr.__r.enter(0);
    __s = __wr.__r.get_sqe();
    if ( __s == nullptr ) return false;
  }
  *__s = __q;
  __s->user_data = __ud;
  __wr.__r.advance_sq();
#if defined(MICRON_CORO_STAGED_SUBMIT)
  if ( ++__wr.__staged < __io_stage_max ) {
#if defined(MICRON_CORO_STATS)
    __wr.__stat.submits.fetch_add(1, micron::memory_order_relaxed);
#endif
    return true;
  }
  __wr.__staged = 0;
#endif
  (void)__wr.__r.enter(0);
#if defined(MICRON_CORO_STATS)
  __wr.__stat.submits.fetch_add(1, micron::memory_order_relaxed);
  __wr.__stat.enters.fetch_add(1, micron::memory_order_relaxed);
#endif
  return true;
}

inline bool
__io_submit_fb(const micron::uring::sqe &__q, u64 __ud) noexcept
{
  // spinlocked
  __io_lock(__io_fb.__sq_lk);
  if ( __io_fb.__live.get(micron::memory_order_acquire) == 0 ) [[unlikely]] {      // torn down under us
    __io_unlock(__io_fb.__sq_lk);
    return false;
  }
  micron::uring::sqe *__s = __io_fb.__r.get_sqe();
  if ( __s == nullptr ) [[unlikely]] {
#if defined(MICRON_CORO_STATS)
    __io_fb.__stat.sqe_full_flushes.fetch_add(1, micron::memory_order_relaxed);
#endif
    (void)__io_fb.__r.enter(0);
    __s = __io_fb.__r.get_sqe();
    if ( __s == nullptr ) {
      __io_unlock(__io_fb.__sq_lk);
      return false;
    }
  }
  *__s = __q;
  __s->user_data = __ud;
  __io_fb.__r.advance_sq();
  (void)__io_fb.__r.enter(0);
#if defined(MICRON_CORO_STATS)
  __io_fb.__stat.submits.fetch_add(1, micron::memory_order_relaxed);
  __io_fb.__stat.enters.fetch_add(1, micron::memory_order_relaxed);
#endif
  __io_unlock(__io_fb.__sq_lk);
  return true;
}

inline bool
__io_submit_own2(__wring &__wr, const micron::uring::sqe &__a, u64 __ud_a, const micron::uring::sqe &__b, u64 __ud_b) noexcept
{
  micron::uring::sqe *__s0 = __wr.__r.peek_sqe(0);
  micron::uring::sqe *__s1 = __wr.__r.peek_sqe(1);
  if ( __s0 == nullptr || __s1 == nullptr ) [[unlikely]] {
#if defined(MICRON_CORO_STATS)
    __wr.__stat.sqe_full_flushes.fetch_add(1, micron::memory_order_relaxed);
#endif
    __wr.__staged = 0;
    (void)__wr.__r.enter(0);
    __s0 = __wr.__r.peek_sqe(0);
    __s1 = __wr.__r.peek_sqe(1);
    if ( __s0 == nullptr || __s1 == nullptr ) return false;      // never silently submit untimed
  }
  *__s0 = __a;
  __s0->user_data = __ud_a;
  *__s1 = __b;
  __s1->user_data = __ud_b;
  __wr.__r.advance_sq(2);
#if defined(MICRON_CORO_STAGED_SUBMIT)
  __wr.__staged += 2;
  if ( __wr.__staged < __io_stage_max ) {
#if defined(MICRON_CORO_STATS)
    __wr.__stat.submits.fetch_add(2, micron::memory_order_relaxed);
#endif
    return true;
  }
  __wr.__staged = 0;
#endif
  (void)__wr.__r.enter(0);
#if defined(MICRON_CORO_STATS)
  __wr.__stat.submits.fetch_add(2, micron::memory_order_relaxed);
  __wr.__stat.enters.fetch_add(1, micron::memory_order_relaxed);
#endif
  return true;
}

inline bool
__io_submit_fb2(const micron::uring::sqe &__a, u64 __ud_a, const micron::uring::sqe &__b, u64 __ud_b) noexcept
{
  __io_lock(__io_fb.__sq_lk);
  if ( __io_fb.__live.get(micron::memory_order_acquire) == 0 ) [[unlikely]] {
    __io_unlock(__io_fb.__sq_lk);
    return false;
  }
  micron::uring::sqe *__s0 = __io_fb.__r.peek_sqe(0);
  micron::uring::sqe *__s1 = __io_fb.__r.peek_sqe(1);
  if ( __s0 == nullptr || __s1 == nullptr ) [[unlikely]] {
    (void)__io_fb.__r.enter(0);
    __s0 = __io_fb.__r.peek_sqe(0);
    __s1 = __io_fb.__r.peek_sqe(1);
    if ( __s0 == nullptr || __s1 == nullptr ) {
      __io_unlock(__io_fb.__sq_lk);
      return false;
    }
  }
  *__s0 = __a;
  __s0->user_data = __ud_a;
  *__s1 = __b;
  __s1->user_data = __ud_b;
  __io_fb.__r.advance_sq(2);
  (void)__io_fb.__r.enter(0);
  __io_unlock(__io_fb.__sq_lk);
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cancellation registry

struct __io_cxl_node {
  __io_cxl_node *__next = nullptr;
  const micron::atomic_token<u32> *__flag = nullptr;
  u64 __ud = 0;
  i32 __ring_fd = -1;
};

inline __io_cxl_node *__io_cxl_head = nullptr;
inline micron::atomic_token<u32> __io_cxl_lk{ 0 };

inline void
__io_cxl_push(__io_cxl_node *__n) noexcept
{
  __io_lock(__io_cxl_lk);
  __n->__next = __io_cxl_head;
  __io_cxl_head = __n;
  __io_unlock(__io_cxl_lk);
}

inline void
__io_cxl_unlink(__io_cxl_node *__n) noexcept
{
  __io_lock(__io_cxl_lk);
  __io_cxl_node **__pp = &__io_cxl_head;
  while ( *__pp != nullptr ) {
    if ( *__pp == __n ) {
      *__pp = __n->__next;
      break;
    }
    __pp = &(*__pp)->__next;
  }
  __io_unlock(__io_cxl_lk);
}

inline void
__io_sync_cancel_ud(i32 __ring_fd, u64 __ud) noexcept
{
  micron::uring::sync_cancel_reg __sc{};
  __sc.addr = __ud;
  __sc.fd = -1;
  __sc.flags = 0;
  (void)micron::uring::__io_uring_register(__ring_fd, micron::uring::reg_register_sync_cancel, &__sc, 1);
}

inline void
__io_cancel_walker(const micron::atomic_token<u32> *__flag) noexcept
{
  __io_lock(__io_cxl_lk);
  for ( __io_cxl_node *__n = __io_cxl_head; __n != nullptr; __n = __n->__next )
    if ( __n->__flag == __flag && __n->__ring_fd >= 0 ) {
      __io_sync_cancel_ud(__n->__ring_fd, __n->__ud);
#if defined(MICRON_CORO_STATS)
      __io_fb.__stat.cancels.fetch_add(1, micron::memory_order_relaxed);
#endif
    }
  __io_unlock(__io_cxl_lk);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// buffer pool
// fixed
// one global slab
// lazily registered into each ring
//
// TODO: add a clone_buffers (6.12+) path

inline constexpr u32 __io_fixed_slots = 8;
inline constexpr usize __io_fixed_sz = 256 * 1024;

struct __io_fixed_pool {
  byte *__base = nullptr;
  micron::atomic_token<u32> __map{ 0 };      // bit i set = slot busy
  micron::atomic_token<u32> __lk{ 0 };
  micron::atomic_token<u32> __ready{ 0 };
};

inline __io_fixed_pool __io_fixed;

inline bool
__io_fixed_ensure() noexcept
{
  if ( __io_fixed.__ready.get(micron::memory_order_acquire) != 0 ) return true;
  __io_lock(__io_fixed.__lk);
  if ( __io_fixed.__ready.get(micron::memory_order_acquire) != 0 ) {
    __io_unlock(__io_fixed.__lk);
    return true;
  }
  void *__p = micron::mmap(nullptr, __io_fixed_slots * __io_fixed_sz, prot_read | prot_write, map_private | map_anonymous, -1, 0);
  if ( reinterpret_cast<usize>(__p) >= static_cast<usize>(-4095) ) {
    __io_unlock(__io_fixed.__lk);
    return false;
  }
  __io_fixed.__base = reinterpret_cast<byte *>(__p);
  __io_fixed.__ready.store(1, micron::memory_order_release);
  __io_unlock(__io_fixed.__lk);
  return true;
}

inline bool
__io_fixed_reg(__wring &__wr) noexcept
{
  const u32 __r0 = __wr.__bufs_reg.get(micron::memory_order_acquire);
  if ( __r0 == 1 ) return true;
  if ( __r0 == 2 ) return false;
  __io_lock(__wr.__sq_lk);
  const u32 __r1 = __wr.__bufs_reg.get(micron::memory_order_acquire);
  if ( __r1 != 0 ) [[unlikely]] {      // another thread finished while we waited
    __io_unlock(__wr.__sq_lk);
    return __r1 == 1;
  }
  if ( !__io_fixed_ensure() ) {
    __wr.__bufs_reg.store(2, micron::memory_order_release);
    __io_unlock(__wr.__sq_lk);
    return false;
  }
  micron::uring::iovec __iov[__io_fixed_slots];
  for ( u32 __i = 0; __i < __io_fixed_slots; ++__i ) {
    __iov[__i].iov_base = __io_fixed.__base + static_cast<usize>(__i) * __io_fixed_sz;
    __iov[__i].iov_len = __io_fixed_sz;
  }
  // RLIMIT_MEMLOCK or old kernel
  const u32 __v = __wr.__r.register_buffers(__iov, __io_fixed_slots) != 0 ? 2u : 1u;
  __wr.__bufs_reg.store(__v, micron::memory_order_release);
  __io_unlock(__wr.__sq_lk);
  return __v == 1;
}

[[nodiscard]] inline i32
acquire_fixed() noexcept
{
  if ( !__io_fixed_ensure() ) return -1;
  for ( ;; ) {
    u32 __m = __io_fixed.__map.get(micron::memory_order_acquire);
    if ( __m == (1u << __io_fixed_slots) - 1u ) return -1;      // exhausted
    const u32 __i = static_cast<u32>(__builtin_ctz(~__m));
    if ( __io_fixed.__map.compare_exchange_weak(__m, __m | (1u << __i), micron::memory_order_acq_rel, micron::memory_order_acquire) )
      return static_cast<i32>(__i);
  }
}

inline void
release_fixed(i32 __slot) noexcept
{
  if ( __slot < 0 || static_cast<u32>(__slot) >= __io_fixed_slots ) return;
  __io_fixed.__map.fetch_and(~(1u << static_cast<u32>(__slot)), micron::memory_order_acq_rel);
}

[[nodiscard]] inline byte *
fixed_ptr(i32 __slot) noexcept
{
  if ( __slot < 0 || static_cast<u32>(__slot) >= __io_fixed_slots || __io_fixed.__base == nullptr ) return nullptr;
  return __io_fixed.__base + static_cast<usize>(__slot) * __io_fixed_sz;
}

[[nodiscard]] inline constexpr usize
fixed_size() noexcept
{
  return __io_fixed_sz;
}

inline void
__io_fixed_shutdown() noexcept
{
  if ( __io_fixed.__ready.get(micron::memory_order_acquire) == 0 ) return;
  __io_fixed.__ready.store(0, micron::memory_order_release);
  micron::munmap(reinterpret_cast<addr_t *>(__io_fixed.__base), __io_fixed_slots * __io_fixed_sz);
  __io_fixed.__base = nullptr;
  __io_fixed.__map.store(0, micron::memory_order_relaxed);
}

inline u64
__io_pending_total() noexcept
{
  u64 __t = __io_fb.__pending.get(micron::memory_order_acquire);
  for ( u32 __i = 0; __i < 32u; ++__i ) __t += __io_rings[__i].__pending.get(micron::memory_order_acquire);
  return __t;
}

[[nodiscard]] inline u64
io_pending() noexcept
{
  return __io_pending_total();
}

struct io_stats_t {
  u64 submits = 0;
  u64 enters = 0;
  u64 inline_completions = 0;
  u64 sqe_full_flushes = 0;
  u64 parks = 0;
  u64 wakes = 0;
  u64 cancels = 0;
};

[[nodiscard]] inline io_stats_t
io_stats() noexcept
{
  io_stats_t __o{};
#if defined(MICRON_CORO_STATS)
  auto __add = [&__o](const __wring &__wr) {
    __o.submits += __wr.__stat.submits.get(micron::memory_order_relaxed);
    __o.enters += __wr.__stat.enters.get(micron::memory_order_relaxed);
    __o.inline_completions += __wr.__stat.inline_completions.get(micron::memory_order_relaxed);
    __o.sqe_full_flushes += __wr.__stat.sqe_full_flushes.get(micron::memory_order_relaxed);
    __o.parks += __wr.__stat.parks.get(micron::memory_order_relaxed);
    __o.wakes += __wr.__stat.wakes.get(micron::memory_order_relaxed);
    __o.cancels += __wr.__stat.cancels.get(micron::memory_order_relaxed);
  };
  for ( u32 __i = 0; __i < 32u; ++__i ) __add(__io_rings[__i]);
  __add(__io_fb);
#endif
  return __o;
}

#if defined(MICRON_CORO_STATS)
inline void
__io_stats_reset() noexcept
{
  auto __z = [](__wring &__wr) {
    __wr.__stat.submits.store(0, micron::memory_order_relaxed);
    __wr.__stat.enters.store(0, micron::memory_order_relaxed);
    __wr.__stat.inline_completions.store(0, micron::memory_order_relaxed);
    __wr.__stat.sqe_full_flushes.store(0, micron::memory_order_relaxed);
    __wr.__stat.parks.store(0, micron::memory_order_relaxed);
    __wr.__stat.wakes.store(0, micron::memory_order_relaxed);
    __wr.__stat.cancels.store(0, micron::memory_order_relaxed);
  };
  for ( u32 __i = 0; __i < 32u; ++__i ) __z(__io_rings[__i]);
  __z(__io_fb);
}
#endif

};      // namespace coro
};      // namespace micron

#endif
