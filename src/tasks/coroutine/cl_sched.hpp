//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__abc_mt.hpp"      // autofires MICRON_ABC_MT; must precede abcmalloc

#include "../../__special/coroutine"

#include "../../atomic/atomic.hpp"
#include "../../queue/crossbeam.hpp"
#include "../../sync/futex.hpp"
#include "../../sync/futex_future.hpp"
#include "../../sync/yield.hpp"
#include "../../tasks/coro_core.hpp"
#include "../../tasks/task.hpp"
#include "../../thread/cpu.hpp"
#include "../../thread/thread.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/sys/time.hpp"
#include "../cancellation.hpp"
#include "fiber.hpp"
#include "reactor.hpp"

#if defined(MICRON_CORO_URING) && defined(MICRON_CORO_GLOBAL_SIGNAL)
#error "MICRON_CORO_URING requires the per-worker park words (incompatible with MICRON_CORO_GLOBAL_SIGNAL)"
#endif

// TODO: this is only here temporarily move it out to */coroutine (somewhere else?)

// WARNING: GCC ICE caught (report is below)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// continuation stealing scheduler
namespace micron
{
namespace coro
{

inline constexpr u32 __cl_max_workers = 32;
#if !defined(MICRON_CORO_GLOBAL_SIGNAL)
static_assert(__cl_max_workers <= 32, "__cl_sleeper_mask is a u32 bitmap; __cl_park is sized to 32");
#endif

inline void
__cl_hot_entry(micron::fiber::fiber *self) noexcept
{
  for ( ;; ) {
    void *a = self->arg;
    if ( a == nullptr ) return;
    static_cast<__frame_base *>(a)->__self.resume();
    self->arg = nullptr;
    micron::ar::__micron_swap_context(&self->ctx, self->link);
  }
}

struct engine {
  worker *workers = nullptr;
  micron::crossbeam<__frame_base *, 256> inbox;      // externally submitted roots
  micron::atomic_token<u32> stopping{ 0 };
  micron::atomic_token<u32> pending_timers{ 0 };      // num of frames in the timer
  u32 n = 0;
  micron::__thread_pointer<micron::auto_thread<>> threads[__cl_max_workers];
#if defined(MICRON_CORO_URING)
  // unbounded spillover
  __frame_base *__io_ovf = nullptr;
  micron::atomic_token<u32> __io_ovf_lk{ 0 };
  micron::atomic_token<u32> __io_ovf_n{ 0 };      // lock-free empty probe for the hot __find path
#endif

  ~engine() { delete[] workers; }

  engine() noexcept = default;

  // retire a worker's hot fiber
  static void
  __retire_hot(worker *w) noexcept
  {
    micron::fiber::fiber *f = w->hot;
    if ( f == nullptr ) return;
    w->hot = nullptr;
    micron::fiber::resume(f);
    micron::fiber::destroy_fiber(f);
  }

  [[gnu::always_inline]] void
  __run(worker *w, __frame_base *cont) noexcept
  {
    // steal accounting happens at the take sites
    micron::fiber::fiber *f = w->hot;
    if ( f == nullptr ) [[unlikely]] {
      f = micron::fiber::create_fiber(&__cl_hot_entry, nullptr, static_cast<usize>(small_stack_size));
      while ( f == nullptr ) [[unlikely]] {
        // stack/VA reservation is exhausted
        if ( stopping.get(micron::memory_order_acquire) ) return;
        micron::yield();
        f = micron::fiber::create_fiber(&__cl_hot_entry, nullptr, static_cast<usize>(small_stack_size));
      }
      w->hot = f;
    }
    f->arg = cont;
    micron::fiber::resume(f);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    // hard trap out if an exception escaped the coroutine
    if ( f->escaped ) [[unlikely]]
      __builtin_trap();
#endif
    if ( f->refs.get(micron::memory_order_acquire) != 1 ) [[unlikely]]
      __retire_hot(w);
    else
      f->frame_sp = f->region.frame_base;
  }

  static constexpr u32 __cl_steal_retries = 2;

  __frame_base *
  __steal(worker *w, u32 &seed) noexcept
  {
    if ( n <= 1 ) return nullptr;
    for ( u32 sweep = 0; sweep < 2u; ++sweep ) {
      seed ^= seed << 13;      // one xorshift per sweep; random start avoids thief convoys
      seed ^= seed >> 17;
      seed ^= seed << 5;
      // Lemire reduction instead of seed % n (integer division), then a linear probe (faster)
      u32 v = static_cast<u32>((static_cast<u64>(seed) * n) >> 32);
      for ( u32 i = 0; i < n; ++i ) {
        if ( v != w->id ) {
#if defined(MICRON_CORO_STEAL_MIN_DEPTH)
          // experiment: first sweep skips depth-1 victims
          if ( sweep == 0 && workers[v].deque.size() <= 1 ) {
            if ( ++v == n ) v = 0;
            continue;
          }
#endif
          for ( u32 r = 0; r < __cl_steal_retries; ++r ) {
            const micron::steal_result<__frame_base *> s = workers[v].deque.try_steal();
            if ( s.__st == micron::steal_status::got ) {
              if ( s.__more ) __notify_work();      // wake propagation
              return s.__v;
            }
            if ( s.__st == micron::steal_status::empty ) break;
            __cpu_pause();      // lost
          }
        }
        if ( ++v == n ) v = 0;
      }
    }
    return nullptr;
  }

  __frame_base *
  __search(worker *w, u32 &seed) noexcept
  {
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
    if ( __cl_searchers.get(micron::memory_order_relaxed) >= __cl_max_searchers ) return nullptr;
#else
    const u32 __awake = n - static_cast<u32>(__builtin_popcount(__cl_sleeper_mask.get(micron::memory_order_relaxed)));
    const u32 __s = __cl_searchers.get(micron::memory_order_relaxed);
    if ( __s != 0 && 2u * __s >= __awake ) return nullptr;
#endif
    __cl_searchers.fetch_add(1, micron::memory_order_acq_rel);
    __frame_base *cont = nullptr;
    u32 backoff = 16;
    for ( u32 round = 0; round < 6; ++round ) {      // 16+32+..+512 pauses ~= 1-2 us total
      for ( u32 p = 0; p < backoff; ++p ) __cpu_pause();
      if ( stopping.get(micron::memory_order_acquire) ) break;
      cont = __find(w, seed);
      if ( cont != nullptr ) break;
      if ( backoff < 512 ) backoff <<= 1;
    }
    __cl_searchers.sub_fetch(1, micron::memory_order_acq_rel);
    return cont;
  }

  __frame_base *
  __find(worker *w, u32 &seed) noexcept
  {
    __frame_base *cont = nullptr;
    if ( (++w->tick & 63u) == 0u ) {      // periodic inbox-first so roots aren't starved by deep local work
      if ( inbox.pop(cont) && cont != nullptr ) return cont;
    }
    cont = w->deque.pop_bottom();
    if ( cont != nullptr ) {
      __count_take(cont);      // a locally-popped fork continuation was detached from its child (it suspended)
      return cont;
    }
    if ( w->ovf != nullptr ) [[unlikely]] {      // deque-overflow spill
      cont = w->ovf;
      w->ovf = cont->__ovf_next;
      cont->__ovf_next = nullptr;
      return cont;
    }
    if ( inbox.pop(cont) && cont != nullptr ) return cont;
#if defined(MICRON_CORO_URING)
    cont = __io_ovf_pop();      // completions the inbox had no room for
    if ( cont != nullptr ) return cont;
    if ( __io.any_live.get(micron::memory_order_acquire) != 0 ) {
      if ( __drain_io(w, seed) ) {
        if ( inbox.pop(cont) && cont != nullptr ) return cont;
        cont = __io_ovf_pop();
        if ( cont != nullptr ) return cont;
      }
    }
#endif
    cont = __steal(w, seed);
    if ( cont != nullptr ) __count_take(cont);
    return cont;
  }

  static constexpr u32 __cl_prewarm_segments = 2;      // segments carved into the TLS freelist at worker start (0 disables)

  void
  worker_main(u32 id) noexcept
  {
    worker *w = &workers[id];
    __cur_worker = w;
    u32 seed = id * 2654435761u + 1u;
#if defined(MICRON_CORO_URING)
    __io_worker_ring_init(id, n);
#endif
    for ( u32 __i = 0; __i < __cl_prewarm_segments; ++__i ) {
      micron::fiber::fiber *__pf = micron::fiber::create_fiber(&__cl_hot_entry, nullptr, static_cast<usize>(small_stack_size));
      if ( __pf == nullptr ) break;
      micron::fiber::destroy_fiber(__pf);
    }
    // active covers the interval where a continuation left the inbox/deque but is still running here
    for ( ;; ) {
      if ( stopping.get(micron::memory_order_acquire) ) break;
      w->active.store(1, micron::memory_order_release);
      __frame_base *cont = __find(w, seed);
      if ( cont == nullptr ) cont = __search(w, seed);      // bounded pre-park spin (capped searchers)
      if ( cont != nullptr ) {
        __run(w, cont);
        continue;
      }
      w->active.store(0, micron::memory_order_release);
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
      __cl_sleepers.fetch_add(1, micron::memory_order_seq_cst);      // announce parked (Dekker: see submit)
      const u32 sig = __cl_signal.get(micron::memory_order_seq_cst);
#else
      const u32 __bit = 1u << w->id;
      // NOTE: a waker can only bump the epoch after claiming our bit, so any wake between the announce and the futex_wait leaves epoch !=
      // __ep -> EAGAIN
      const u32 __ep = __cl_park[w->id].epoch.get(micron::memory_order_relaxed);
      __cl_sleeper_mask.fetch_or(__bit, micron::memory_order_seq_cst);      // announce parked (Dekker: see __cl_wake_one<true>)
#endif
      w->active.store(1, micron::memory_order_release);
      cont = __find(w, seed);
      if ( cont != nullptr ) {
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
        __cl_sleepers.sub_fetch(1, micron::memory_order_acq_rel);
#else
        __cl_sleeper_mask.fetch_and(~__bit, micron::memory_order_acq_rel);
#endif
        if ( !stopping.get(micron::memory_order_acquire) ) __run(w, cont);
        continue;
      }
      w->active.store(0, micron::memory_order_release);
      if ( !stopping.get(micron::memory_order_acquire) ) {
#if defined(MICRON_CORO_URING)
        // (>=6.7)
        if ( __io.futex_ok.get(micron::memory_order_acquire) != 0 ) {
          __wring &__own = __io_rings[w->id];
          if ( __own.__live.get(micron::memory_order_acquire) != 0 && __own.__pending.get(micron::memory_order_relaxed) != 0 ) {
            __ring_park(w, __ep);
            __cl_sleeper_mask.fetch_and(~__bit, micron::memory_order_acq_rel);
            continue;
          }
        }
        if ( __io.any_live.get(micron::memory_order_acquire) != 0 && __io_pending_total() != 0 ) {
          i32 __exp = -1;
          if ( __io.watcher.compare_exchange_strong(__exp, static_cast<i32>(w->id), micron::memory_order_acq_rel,
                                                    micron::memory_order_acquire) ) {
            timespec_t __wts{ 0, 1000000 };
            micron::__futex(__cl_park[w->id].epoch.ptr(), futex_wait | futex_private_flag, __ep, &__wts, nullptr, 0);
            __io.watcher.store(-1, micron::memory_order_release);
            __drain_all();
            __cl_sleeper_mask.fetch_and(~__bit, micron::memory_order_acq_rel);
            continue;
          }
        }
#endif
        timespec_t __ts{ 0, 100000000 };
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
        micron::__futex(__cl_signal.ptr(), futex_wait | futex_private_flag, sig, &__ts, nullptr, 0);
#else
        micron::__futex(__cl_park[w->id].epoch.ptr(), futex_wait | futex_private_flag, __ep, &__ts, nullptr, 0);
#endif
      }
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
      __cl_sleepers.sub_fetch(1, micron::memory_order_acq_rel);
#else
      __cl_sleeper_mask.fetch_and(~__bit, micron::memory_order_acq_rel);
#endif
    }
    w->active.store(0, micron::memory_order_release);
    __retire_hot(w);      // before drain: the retired segment recycles into the freelist being drained
    micron::fiber::drain_freelist();
#if defined(MICRON_CORO_URING)
    __io_worker_ring_shutdown(id);      // own thread: last touch of the single-issuer ring
#endif
  }

  void
  submit(__frame_base *fb) noexcept
  {
    while ( !inbox.push(fb) ) micron::yield();      // inbox full
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
    __cl_signal.fetch_add(1, micron::memory_order_seq_cst);
    if ( __cl_sleepers.get(micron::memory_order_seq_cst) != 0 ) micron::wake_futex(__cl_signal.ptr(), 1);
#else
    __cl_wake_one<true>();      // Strong: the seq_cst RMW mask read orders after the inbox push
#endif
  }

#if defined(MICRON_CORO_URING)
  // non blocking submit for completions
  void
  __submit_io(__frame_base *fb) noexcept
  {
    if ( !inbox.push(fb) ) [[unlikely]] {
      __io_lock(__io_ovf_lk);
      fb->__io_next = __io_ovf;
      __io_ovf = fb;
      __io_ovf_n.fetch_add(1, micron::memory_order_acq_rel);
      __io_unlock(__io_ovf_lk);
    }
    if ( __cl_sleeper_mask.get(micron::memory_order_relaxed) != 0 ) __cl_wake_one<true>();
  }

  __frame_base *
  __io_ovf_pop() noexcept
  {
    if ( __io_ovf_n.get(micron::memory_order_acquire) == 0 ) return nullptr;
    __io_lock(__io_ovf_lk);
    __frame_base *fb = __io_ovf;
    if ( fb != nullptr ) {
      __io_ovf = fb->__io_next;
      fb->__io_next = nullptr;
      __io_ovf_n.sub_fetch(1, micron::memory_order_acq_rel);
    }
    __io_unlock(__io_ovf_lk);
    return fb;
  }

  [[nodiscard]] bool
  __io_ovf_empty() const noexcept
  {
    return __io_ovf_n.get(micron::memory_order_acquire) == 0;
  }

  // cqe popping is serialized per ring
  void
  __dispatch_cqe(__wring &__wr, const micron::uring::cqe &__c) noexcept
  {
    const u8 __tag = __io_ud_tag(__c.user_data);
    if ( __tag == __io_ud_park ) {
      if ( __c.res == -22 || __c.res == -95 ) __io.futex_ok.store(0, micron::memory_order_release);
      __wr.__park_fired.store(1, micron::memory_order_release);
      return;
    }
    if ( __tag == __io_ud_mop ) {
      __io_mop *__m = reinterpret_cast<__io_mop *>(__io_ud_payload(__c.user_data));
      __io_mev __e;
      __e.__res = __c.res;
      __e.__ring = __m->__ring;
      __e.__fl = 0;
      if ( (__c.flags & micron::uring::cqe_f_buffer) != 0 ) {
        __e.__bid = micron::uring::cqe_buffer_id(__c.flags);
        __e.__fl |= __io_mev_buf;
      }
      if ( (__c.flags & micron::uring::cqe_f_more) != 0 )
        __e.__fl |= __io_mev_more;
      else {
        __m->__live.store(0, micron::memory_order_release);
        __wr.__pending.sub_fetch(1, micron::memory_order_acq_rel);
      }
      __m->__push(__e);
      // three state park handoff
      u32 __cur = __m->__st.get(micron::memory_order_seq_cst);
      for ( ;; ) {
        if ( __cur == __io_mop_parked ) {
          if ( !__m->__st.compare_exchange_weak(__cur, __io_mop_idle, micron::memory_order_seq_cst, micron::memory_order_seq_cst) )
            continue;
          __frame_base *__fb = __m->__f;      // lst read of *__m
          __submit_io(__fb);
          break;
        }
        if ( __cur == __io_mop_parking ) {
          if ( !__m->__st.compare_exchange_weak(__cur, __io_mop_pending, micron::memory_order_seq_cst, micron::memory_order_seq_cst) )
            continue;
          break;
        }
        break;
      }
      return;
    }
    if ( __tag == __io_ud_wop ) {
      __io_wop *__wp = reinterpret_cast<__io_wop *>(__io_ud_payload(__c.user_data));
      __wp->__res = __c.res;
      __io_wave *__wv = __wp->__w;
      if ( __wv->__left.sub_fetch(1, micron::memory_order_acq_rel) != 0 ) return;
      u32 __exp = __io_st_submitted;
      if ( __wv->__st.compare_exchange_strong(__exp, __io_st_done_early, micron::memory_order_acq_rel, micron::memory_order_acquire) )
        return;
      __wr.__pending.sub_fetch(1, micron::memory_order_acq_rel);
      u32 __sus = __io_st_suspended;
      if ( !__wv->__st.compare_exchange_strong(__sus, __io_st_resumed, micron::memory_order_acq_rel, micron::memory_order_acquire) )
        return;      // the owner is tearing the wave down and frees it itself
      __submit_io(__wv->__f);
      return;
    }
    if ( __tag != __io_ud_op ) return;      // cancel / ltimer / reclaim
    __io_op *__op = reinterpret_cast<__io_op *>(__c.user_data);
    __op->__res = __c.res;
    u32 __exp = __io_st_submitted;
    if ( __op->__st.compare_exchange_strong(__exp, __io_st_done_early, micron::memory_order_acq_rel, micron::memory_order_acquire) ) return;
    __wr.__pending.sub_fetch(1, micron::memory_order_acq_rel);
    __submit_io(__op->__f);
  }

  bool
  __drain_ring(__wring &__wr) noexcept
  {
    if ( !__io_cq_acquire(__wr) ) return false;
    micron::uring::cqe __c{};
    bool __any = false;
    while ( __wr.__r.peek_cqe(&__c) ) {
      __any = true;
      __dispatch_cqe(__wr, __c);
    }
    __io_unlock(__wr.__cq_lk);
    return __any;
  }

  // own ring -> fallback ring -> one seed-rotated foreign ring
  bool
  __drain_io(worker *__w, u32 &__seed) noexcept
  {
    bool __any = false;
    __wring &__own = __io_rings[__w->id];
    if ( __own.__live.get(micron::memory_order_acquire) != 0 ) {
      __io_flush_staged(__own);
      if ( __own.__r.cq_overflowed() || (__own.__defer != 0 && __own.__r.taskrun_pending()) )
        (void)__own.__r.enter2(0, 0, micron::uring::enter_getevents, nullptr, 0);
      if ( __own.__pending.get(micron::memory_order_relaxed) != 0 ) __any |= __drain_ring(__own);
    }
    if ( __io_fb.__pending.get(micron::memory_order_relaxed) != 0 ) {
      if ( __io_fb.__r.cq_overflowed() ) (void)__io_fb.__r.enter2(0, 0, micron::uring::enter_getevents, nullptr, 0);
      __any |= __drain_ring(__io_fb);
    }
    if ( n > 1 ) {
      __seed ^= __seed << 13;
      __seed ^= __seed >> 17;
      __seed ^= __seed << 5;
      u32 __v = static_cast<u32>((static_cast<u64>(__seed) * n) >> 32);
      if ( __v == __w->id && ++__v == n ) __v = 0;
      if ( __v != __w->id && __io_rings[__v].__pending.get(micron::memory_order_relaxed) != 0 ) __any |= __drain_ring(__io_rings[__v]);
    }
    return __any;
  }

  bool
  __drain_all() noexcept
  {
    bool __any = false;
    for ( u32 __i = 0; __i < n; ++__i ) __any |= __drain_ring(__io_rings[__i]);
    __any |= __drain_ring(__io_fb);
    return __any;
  }

  void
  __ring_park(worker *__w, u32 __ep) noexcept
  {
    __wring &__wr = __io_rings[__w->id];
    __wr.__park_fired.store(0, micron::memory_order_relaxed);
    micron::uring::sqe __q;
    micron::uring::prep_futex_wait(&__q, __cl_park[__w->id].epoch.ptr(), __ep);
    if ( !__io_submit_own(__wr, __q, __io_ud_make(__io_ud_park, __w->id)) ) {
      timespec_t __ts{ 0, 100000000 };
      micron::__futex(__cl_park[__w->id].epoch.ptr(), futex_wait | futex_private_flag, __ep, &__ts, nullptr, 0);
      return;
    }
#if defined(MICRON_CORO_STATS)
    __wr.__stat.parks.fetch_add(1, micron::memory_order_relaxed);
#endif
    const micron::uring::ktimespec __kt{ 0, 100000000 };
    (void)__wr.__r.submit_and_wait_timeout(1, &__kt);
#if defined(MICRON_CORO_STATS)
    __wr.__stat.wakes.fetch_add(1, micron::memory_order_relaxed);
#endif
    __drain_ring(__wr);
    if ( __wr.__park_fired.get(micron::memory_order_acquire) == 0 ) {
      micron::uring::sqe __cq;
      micron::uring::prep_cancel(&__cq, __io_ud_make(__io_ud_park, __w->id));
      while ( !__io_submit_own(__wr, __cq, __io_ud_make(__io_ud_cancel, 0)) ) {
        if ( __wr.__park_fired.get(micron::memory_order_acquire) != 0 ) break;
        (void)__wr.__r.submit_and_wait_timeout(1, &__kt);
        __drain_ring(__wr);
      }
      while ( __wr.__park_fired.get(micron::memory_order_acquire) == 0 ) {
        (void)__wr.__r.submit_and_wait_timeout(1, &__kt);
        __drain_ring(__wr);
      }
    }
  }
#endif
};

inline engine *__global_engine = nullptr;
// 0 = uninitialized
// 1 = a thread is constructing the engine
// 2 = engine published + ready
inline micron::atomic_token<u32> __engine_state{ 0 };

struct __timer_node {
  timespec_t __deadline;
  __frame_base *__frame = nullptr;
  __timer_node *__next = nullptr;
};

inline __timer_node *__timer_head = nullptr;
inline micron::atomic_token<u32> __timer_lk{ 0 };
inline micron::__thread_pointer<micron::auto_thread<>> __timer_thread;
// 0 = off, 1 = a thread is spawning it, 2 = running
inline micron::atomic_token<u32> __timer_state{ 0 };
alignas(64) inline micron::atomic_token<u32> __timer_epoch{ 0 };

inline void
__timer_lock() noexcept
{
  u32 __e = 0;
  while ( !__timer_lk.compare_exchange_weak(__e, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __e = 0;
    micron::cpu_pause<1>();
  }
}

inline void
__timer_unlock() noexcept
{
  __timer_lk.store(0u, micron::memory_order_release);
}

[[gnu::always_inline]] inline bool
__ts_le(const timespec_t &__a, const timespec_t &__b) noexcept
{
  return __a.tv_sec < __b.tv_sec || (__a.tv_sec == __b.tv_sec && __a.tv_nsec <= __b.tv_nsec);
}

#ifndef MICRON_CORO_IO_DRAIN_GRACE_NS
#define MICRON_CORO_IO_DRAIN_GRACE_NS 2000000000ull
#endif
#if defined(MICRON_CORO_URING)
// length for stop_coroutine_runtime
// cancels tasks in flight with no progress if it exceeds this val
inline constexpr u64 __cl_io_drain_grace_ns = MICRON_CORO_IO_DRAIN_GRACE_NS;
#endif

#ifndef MICRON_CORO_TIMER_DRAIN_GRACE_NS
#define MICRON_CORO_TIMER_DRAIN_GRACE_NS MICRON_CORO_IO_DRAIN_GRACE_NS
#endif
inline constexpr u64 __cl_timer_drain_grace_ns = MICRON_CORO_TIMER_DRAIN_GRACE_NS;

[[gnu::always_inline]] inline void
__ts_add_ns(timespec_t &__t, u64 __ns) noexcept
{
  __t.tv_sec += static_cast<decltype(__t.tv_sec)>(__ns / 1000000000ull);
  __t.tv_nsec += static_cast<decltype(__t.tv_nsec)>(__ns % 1000000000ull);
  if ( __t.tv_nsec >= 1000000000 ) {
    __t.tv_nsec -= 1000000000;
    ++__t.tv_sec;
  }
}

inline void
__timer_main() noexcept
{
  for ( ;; ) {
    engine *__e = __global_engine;
    if ( __e == nullptr || __e->stopping.get(micron::memory_order_acquire) ) break;
    const u32 __ep = __timer_epoch.get(micron::memory_order_acquire);
    timespec_t __now{};
    micron::clock_gettime(micron::clock_monotonic, __now);
    __timer_node *__fired = nullptr;
    bool __have_next = false;
    timespec_t __next{};
    __timer_lock();
    __timer_node **__pp = &__timer_head;
    while ( *__pp != nullptr ) {
      if ( __ts_le((*__pp)->__deadline, __now) ) {
        __timer_node *__n = *__pp;
        *__pp = __n->__next;
        __n->__next = __fired;
        __fired = __n;
      } else {
        if ( !__have_next || __ts_le((*__pp)->__deadline, __next) ) {
          __next = (*__pp)->__deadline;
          __have_next = true;
        }
        __pp = &(*__pp)->__next;
      }
    }
    __timer_unlock();
    while ( __fired != nullptr ) {
      __timer_node *__nx = __fired->__next;      // read before submit (the resumed frame may free the node)
      __frame_base *__f = __fired->__frame;
      __e->submit(__f);
      __e->pending_timers.sub_fetch(1, micron::memory_order_acq_rel);
      __fired = __nx;
    }
    if ( __have_next ) {
      timespec_t __rel{ __next.tv_sec - __now.tv_sec, __next.tv_nsec - __now.tv_nsec };
      if ( __rel.tv_nsec < 0 ) {
        __rel.tv_sec -= 1;
        __rel.tv_nsec += 1000000000l;
      }
      if ( __rel.tv_sec < 0 ) continue;      // already due; rescan
      micron::__futex(__timer_epoch.ptr(), futex_wait | futex_private_flag, __ep, &__rel, nullptr, 0);
    } else {
      micron::__futex(__timer_epoch.ptr(), futex_wait | futex_private_flag, __ep, nullptr, nullptr, 0);
    }
  }
}

inline void
__ensure_timer_thread() noexcept
{
  if ( __timer_state.get(micron::memory_order_acquire) == 2u ) return;
  u32 __e = 0;
  if ( __timer_state.compare_exchange_strong(__e, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __timer_thread = micron::solo::spawn<micron::auto_thread<>>([]() { __timer_main(); });
    __timer_state.store(2u, micron::memory_order_release);
    return;
  }
  while ( __timer_state.get(micron::memory_order_acquire) != 2u ) micron::yield();      // wait for publish
}

inline void stop_coroutine_runtime() noexcept;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// automatic runtime teardown
inline void
__arm_runtime_reaper(void) noexcept
{
  static struct __rt_reaper {
    ~__rt_reaper() noexcept { stop_coroutine_runtime(); }
  } __r{};

  (void)&__r;
}

inline void
start_coroutine_runtime(u32 nworkers = 0) noexcept
{
  if ( __engine_state.get(micron::memory_order_acquire) == 2u ) return;

  u32 __exp = 0u;
  if ( !__engine_state.compare_exchange_strong(__exp, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    while ( __engine_state.get(micron::memory_order_acquire) != 2u ) micron::yield();      // wait for ready
    return;
  }

  __arm_runtime_reaper();

  if ( nworkers == 0 ) nworkers = micron::cpu_count();
  if ( nworkers > __cl_max_workers ) nworkers = __cl_max_workers;
  if ( nworkers < 1 ) nworkers = 1;

  engine *e = new engine();
  e->n = nworkers;
  e->workers = new worker[nworkers];
  for ( u32 i = 0; i < nworkers; ++i ) e->workers[i].id = i;
#if defined(MICRON_CORO_URING)
  __io_fb_init();      // worker rings init on their own threads (single_issuer)
  __io.futex_ok.store(micron::kernel::has(micron::kernel::feature::uring_futex) ? 1u : 0u, micron::memory_order_relaxed);
  __io.watcher.store(-1, micron::memory_order_relaxed);
  __io_cancel_hook = &__io_cancel_walker;      // cancellation_source::cancel() reaches in-flight ops
#endif
  __global_engine = e;
  for ( u32 i = 0; i < nworkers; ++i )
    e->threads[i] = micron::solo::spawn<micron::auto_thread<>>([](engine *eng, u32 id) { eng->worker_main(id); }, e, i);
  // the timer thread spawns lazily on the first sleep/timer
  __engine_state.store(2u, micron::memory_order_release);
}

inline void
stop_coroutine_runtime() noexcept
{
  engine *e = __global_engine;
  if ( e == nullptr ) return;

  {
    u32 __quiet = 0;
    timespec_t __tm_dl{ 0, 0 };      // re-armed whenever a timer fires
    u32 __tm_seen = 0;
    bool __tm_armed = false;
    bool __tm_cut = false;
#if defined(MICRON_CORO_URING)
    timespec_t __io_dl{ 0, 0 };      // grace deadline; re-armed whenever io makes progress
    u64 __seen = 0;                  // in-flight count the deadline was armed against
    bool __armed = false;
#endif
    while ( __quiet < 4 ) {
      // WARNING: a still-armed timer may _NEVER_ stall indefinitely
      const u32 __tm = e->pending_timers.get(micron::memory_order_acquire);
      if ( __tm == 0 ) {
        __tm_armed = false;
      } else if ( !__tm_cut ) {
        timespec_t __now{};
        micron::clock_gettime(micron::clock_monotonic, __now);
        if ( !__tm_armed || __tm != __tm_seen ) {
          __tm_seen = __tm;
          __tm_armed = true;
          __tm_dl = __now;
          __ts_add_ns(__tm_dl, __cl_timer_drain_grace_ns);
        } else if ( !__ts_le(__now, __tm_dl) ) {
          __tm_cut = true;
        }
        if ( !__tm_cut ) {
          timespec_t __nap{ 0, 200000 };
          (void)micron::nanosleep(__nap);
          __quiet = 0;
          continue;
        }
      }
      bool __q = e->inbox.empty() && (__tm == 0 || __tm_cut);
#if defined(MICRON_CORO_URING)
      if ( __q && !e->__io_ovf_empty() ) __q = false;
#endif
      for ( u32 __i = 0; __q && __i < e->n; ++__i )
        if ( e->workers[__i].active.get(micron::memory_order_acquire) != 0 || !e->workers[__i].deque.empty()
             || e->workers[__i].ovf != nullptr )
          __q = false;
#if defined(MICRON_CORO_URING)
      if ( __q ) {
        const u64 __pend = __io_pending_total();
        if ( __pend != 0 ) {
          timespec_t __now{};
          micron::clock_gettime(micron::clock_monotonic, __now);
          if ( !__armed || __pend != __seen ) {
            __seen = __pend;
            __armed = true;
            __io_dl = __now;
            __ts_add_ns(__io_dl, __cl_io_drain_grace_ns);
          } else if ( !__ts_le(__now, __io_dl) ) {      // grace burned with zero progress
            micron::uring::sync_cancel_reg __sc{};
            __sc.fd = -1;
            __sc.flags = micron::uring::async_cancel_any;
            for ( u32 __i = 0; __i < e->n; ++__i ) {
              __wring &__wr = __io_rings[__i];
              if ( __wr.__live.get(micron::memory_order_acquire) != 0 )
                (void)micron::uring::__io_uring_register(__wr.__r.fd, micron::uring::reg_register_sync_cancel, &__sc, 1);
            }
            if ( __io_fb.__live.get(micron::memory_order_acquire) != 0 )
              (void)micron::uring::__io_uring_register(__io_fb.__r.fd, micron::uring::reg_register_sync_cancel, &__sc, 1);
            __armed = false;      // re-arm: a cancel that does not land must not spin the register syscall
          }
          __q = false;
          // the grace window is seconds long: poll it, do not sched_yield a core
          // flat out while the workers are the ones that need the cpu
          timespec_t __nap{ 0, 200000 };
          (void)micron::nanosleep(__nap);
          __quiet = 0;
          continue;
        }
        __armed = false;
      }
#endif
      if ( __q )
        ++__quiet;
      else
        __quiet = 0;
      micron::yield();
    }
  }

  e->stopping.store(1, micron::memory_order_release);
  if ( __timer_state.get(micron::memory_order_acquire) != 0u ) {
    __timer_epoch.fetch_add(1, micron::memory_order_release);      // the timer may be in an indefinite wait
    micron::wake_futex(__timer_epoch.ptr(), 1);
    __timer_thread.reset();
    __timer_state.store(0u, micron::memory_order_release);
  }
  __timer_lock();
  __timer_head = nullptr;
  __timer_unlock();
  e->pending_timers.store(0, micron::memory_order_release);
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
  __cl_signal.fetch_add(1, micron::memory_order_release);
  micron::wake_futex(__cl_signal.ptr(), static_cast<int>(e->n));
#else
  for ( u32 i = 0; i < e->n; ++i ) {
    __cl_park[i].epoch.fetch_add(1, micron::memory_order_release);
    micron::wake_futex(__cl_park[i].epoch.ptr(), 1);
  }
#endif
  for ( u32 i = 0; i < e->n; ++i ) e->threads[i].reset();
  micron::fiber::drain_seg_pool();      // segments parked by crossworker finalize
#if defined(MICRON_CORO_URING)
  __io_cancel_hook = nullptr;
  __io_fb_shutdown();
  __io_fixed_shutdown();
  __io.any_live.store(0, micron::memory_order_release);
  __io.watcher.store(-1, micron::memory_order_relaxed);
#endif
  __global_engine = nullptr;
  delete e;
  __engine_state.store(0u, micron::memory_order_release);
}

struct __sync_bridge {
  struct promise_type: __frame_base {
    micron::atomic_token<u32> *__done = nullptr;

    __sync_bridge
    get_return_object() noexcept
    {
      auto __h = std::coroutine_handle<promise_type>::from_promise(*this);
      this->__self = __h;
      return __sync_bridge{ __h };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    auto
    final_suspend() noexcept
    {
      struct __fa {
        bool
        await_ready() const noexcept
        {
          return false;
        }

        void
        await_suspend(std::coroutine_handle<promise_type> __h) const noexcept
        {
          micron::atomic_token<u32> *__d = __h.promise().__done;
          __d->store(1, micron::memory_order_release);
          micron::wake_futex(__d->ptr());
        }

        void
        await_resume() const noexcept
        {
        }
      };

      return __fa{};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
      __builtin_trap();
    }

    static void *
    operator new(usize __n) noexcept
    {
      return micron::fiber::__frame_alloc(__n);
    }

    static void
    operator delete(void *__p, usize __n) noexcept
    {
      micron::fiber::__frame_free(__p, __n);
    }

    static __sync_bridge get_return_object_on_allocation_failure() noexcept;
  };

  std::coroutine_handle<promise_type> __h{};
  __sync_bridge() noexcept = default;

  explicit __sync_bridge(std::coroutine_handle<promise_type> __hh) noexcept : __h(__hh) { }
};

inline __sync_bridge
__sync_bridge::promise_type::get_return_object_on_allocation_failure() noexcept
{
  return __sync_bridge{};
}

[[gnu::always_inline]] inline void
__sync_spin(const micron::atomic_token<u32> &__done) noexcept
{
  for ( u32 __p = 0; __p < 800; ++__p ) {
    if ( __done.get(micron::memory_order_acquire) != 0 ) return;
    __cpu_pause();
  }
}

template<class T>
decltype(auto)
sync_wait(task<T> &&root)
{
  start_coroutine_runtime();
  micron::atomic_token<u32> done{ 0 };

  if constexpr ( micron::is_void_v<T> ) {
    __sync_bridge bridge = [](task<void> __r) -> __sync_bridge { co_await micron::move(__r); }(micron::move(root));
    bridge.__h.promise().__done = &done;
    __global_engine->submit(&bridge.__h.promise());
    __sync_spin(done);
    // WARNING: exit exclusively via an acquire load seeing done!=0 (without this the bridge destroy below has no hb edge to the worker's
    // frame writes)
    while ( done.get(micron::memory_order_acquire) == 0 ) micron::wait_futex(done.ptr(), 0u);
    bridge.__h.destroy();
    return;
  } else {
    alignas(T) unsigned char storage[sizeof(T)];
    T *slot = micron::ptr_cast<T *>(storage);
    __sync_bridge bridge = [](task<T> __r, T *__s) -> __sync_bridge {
      // NOTE: the co_await is split out of the placement-new to avoid a GCC ICE (co_await inside a new-expr)
      // i investigated this _thoroughly_, the placement-new bug is completely unrelated to our code (all paths have been audited and are
      // fully standard compliant && valid) it's a pure gcc compiler bug
      //  standard compliant repro
      //  #include <coroutine>
      //  #include <new>
      //  struct Aw { bool await_ready() const noexcept {return false;}
      //              void await_suspend(std::coroutine_handle<>) noexcept {}
      //              int  await_resume() noexcept {return 7;} };
      //  struct bridge { struct promise_type {
      //    bridge get_return_object(){return {};}
      //    std::suspend_always initial_suspend() noexcept {return {};}
      //    std::suspend_never  final_suspend() noexcept {return {};}
      //    void return_void(){} void unhandled_exception(){} }; };
      //
      //  bridge f(int* s) { ::new (static_cast<void*>(s)) int(co_await Aw{}); }   // ICE on the spot

      //  during RTL pass: expand
      //  internal compiler error: in expand_expr_real_1, at expr.cc:11648    (GCC 16.1.1)
      //  internal compiler error: in expand_expr_real_1, at expr.cc:11559    (GCC 15.2.1, arm32) [present across cmpl versions and arches
      //  too]

      // main difference between operator new and placement new
      //
      // %%%%%%%%%%%%%%%%%
      //  for operator new
      // _17 = operator new (4);
      // frame_ptr->T001_2_3 = _17;  // result
      // frame_ptr->T002_2_3 = 1;    // guard

      // suspend/resume
      //_24 = frame_ptr->T001_2_3;  // rewritten
      // MEM[(int *)_24] = _25;

      // %%%%%%%%%%%%%%%
      // for placement new
      //  _17 = frame_ptr->s;
      //  frame_ptr->T001_2_3 = _17;  // saved placement ptr
      //  _18 = frame_ptr->T001_2_3;
      //  _19 = operator new (4, _18);
      //  frame_ptr->T002_2_3 = _19;  // result
      //  frame_ptr->T003_2_3 = 1;    // guard
      //  suspend/resume
      //  MEM[(int *)D.12331] = _26;  // __NOT__ rewritten
      //  frame_ptr->T003_2_3 = 0;
      //
      //  according to gdb exact abort fires at pass_expand::execute -> expand_gimple_stmt -> expand_assignment -> expand_expr_real_1 ->
      //  hard abort on reading base of a store coroutine transform promotes temps that live _across a suspend into the frame_ (almost never
      //  occurs in reg code); during gimple emit it rewrites the defs but leaves one redundant use behind important to note that ONLY THE
      //  STANDARD PLACEMENT NEW (in co_await form) materialize an additional saved pointer temporary (three tmps instead of two)
      // fully invariant across std spec exceptions arches and importantly optimization levels
      // the gimple lowering almost certainly misses the spurious third temporary that is entirely unexpected at a later pass (real reg
      // emit?)

      // UPDATE
      // looked at the gcc source
      // gcc_assert fires in expand_expr_real_1
      //
      /* Variables inherited from containing functions should have
         been lowered by this point.  */
      //  tree context = decl_function_context (exp);
      // gcc_assert (SCOPE_FILE_SCOPE_P (context)
      //            || context == current_function_decl
      //            || TREE_STATIC (exp) || DECL_EXTERNAL (exp)
      //            || TREE_CODE (exp) == FUNCTION_DECL);
      // full trigger is in cp/init.cc:3543-3559:
      //  if (call_expr_nargs (alloc_call) == 2
      //      && TYPE_PTR_P (TREE_TYPE (CALL_EXPR_ARG (alloc_call, 1))))
      //    if (placement_first != NULL_TREE && (INTEGRAL_OR_ENUMERATION_TYPE_P (TREE_TYPE (TREE_TYPE (placement)))
      //     || VOID_TYPE_P (TREE_TYPE (TREE_TYPE (placement)))))
      //     placement_expr = get_internal_target_expr (placement_first);  // extra TARGET_EXPR
      // stabilize_call later hoists it, which in turn makes alloc_expr a nested COMPOUND_EXPR rather than a simple TARGET_EXPR
      // it also has NOTHING to do with placement new (placement new is incidental only), only fires for two promoted tmps rather than one,
      // ie operator new(size_t, char) and it will NEVER fire for ::new (nullptr) [do note that nullptr is not a TYPE_PTR_T] OR for
      // placement-new cases where the target pointer is a class
      //
      //
      // final final issue is with cp/coroutines.cc| flatten_await_stmt; by hoisting each TARGET_EXPR into a frame tmp it later must repoint
      // every ref to the old TARGET_EXPR_SLOT; alloc_node <await result> sits at the other end of the enclosing COMPOUND_EXPR, according to
      // gccs own source
      //
      /* ... and any other uses of it or its slot.  */
      /* Compiler-generated temporaries can also have uses in
         following arms of compound expressions, which will be listed
         in 'replace_in' if present.  */
      // thus only the replace_in parameter can patch it but it's never forwarded down the chain
      //  3160: case COMPOUND_EXPR:
      //  3180: flatten_await_stmt (ins, promoted, temps_used, &n->init);
      //  3289: flatten_await_stmt (n,   promoted, temps_used, NULL);   -> replace_in discarded(?!?!)
      // a simple TARGET_EXPR [meaning one tmp only], replace_in = &rest and the store is rewritten, yet nested COMPOUND_EXPRs (two temp
      // forms), the replace_in is discarded, case reenters, the _slot gets rewritten at its definition_ and _read zero times_; VAR_DECL
      // keeps DECL_CONTEXT [original unrewritten raw fn] and the gcc_assert fires
      //
      // CRAZY OBSERVATION is that wrapping the new expression in
      // a comma statement, MAKES THE STORE COME OUT CORRECTLY and the assert no longer fires (?!?!)
      // effectively stopping here, analyzing this further requires effectively recompiling gcc with proper instrumentation attached, and i
      // don't have the time right now, patching replace_in _should_ in principle fix this
      T __v = co_await micron::move(__r);
      ::new (static_cast<void *>(__s)) T(micron::move(__v));
    }(micron::move(root), slot);
    bridge.__h.promise().__done = &done;
    __global_engine->submit(&bridge.__h.promise());
    __sync_spin(done);
    while ( done.get(micron::memory_order_acquire) == 0 ) micron::wait_futex(done.ptr(), 0u);
    bridge.__h.destroy();
    T r = micron::move(*slot);
    slot->~T();
    return r;
  }
}

struct __detached_bridge {
  struct promise_type: __frame_base {
    __detached_bridge
    get_return_object() noexcept
    {
      auto __h = std::coroutine_handle<promise_type>::from_promise(*this);
      this->__self = __h;
      return __detached_bridge{ __h };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    std::suspend_never
    final_suspend() noexcept
    {
      return {};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
      __builtin_trap();
    }

    static void *
    operator new(usize __n) noexcept
    {
      return ::operator new(__n);
    }

    static void
    operator delete(void *__p, usize) noexcept
    {
      ::operator delete(__p);
    }

    static __detached_bridge
    get_return_object_on_allocation_failure() noexcept
    {
      return {};
    }
  };

  std::coroutine_handle<promise_type> __h{};
  __detached_bridge() noexcept = default;

  explicit __detached_bridge(std::coroutine_handle<promise_type> __hh) noexcept : __h(__hh) { }
};

template<class T>
void
detach(task<T> &&__root)
{
  start_coroutine_runtime();
  __detached_bridge __b = [](task<T> __r) -> __detached_bridge { co_await micron::move(__r); }(micron::move(__root));
  if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
}

template<class T>
[[nodiscard]] micron::futex_future<T>
schedule(task<T> &&__root)
{
  start_coroutine_runtime();
  micron::futex_promise<T> __prom;
  micron::futex_future<T> __fut = __prom.get_future();
  if constexpr ( micron::is_void_v<T> ) {
    __detached_bridge __b = [](task<void> __r, micron::futex_promise<void> __p) -> __detached_bridge {
      co_await micron::move(__r);
      __p.set_value();
    }(micron::move(__root), micron::move(__prom));
    if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
  } else {
    __detached_bridge __b = [](task<T> __r, micron::futex_promise<T> __p) -> __detached_bridge {
      T __v = co_await micron::move(__r);      // split co_await out of set_value (GCC ICE on co_await in a call arg)
      __p.set_value(micron::move(__v));
    }(micron::move(__root), micron::move(__prom));
    if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
  }
  return __fut;
}

template<class T> struct __futex_future_awaiter {
  micron::futex_shared_state<T> *__s;

  bool
  await_ready() const noexcept
  {
    return __s == nullptr || __s->is_ready();
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();
    bool __ready = __s->__arm_waiter(
        reinterpret_cast<usize>(__f), +[](usize __t) { __global_engine->submit(reinterpret_cast<__frame_base *>(__t)); });
    return !__ready;
  }

  decltype(auto)
  await_resume()
  {
    if ( __s == nullptr ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: co_await on a future with no shared state");
    return __s->get();
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// LIFO: the frame goes onto the bottom of its own deque and __find pops the bottom
//
// NOTE: that is deliberate (it keeps a hot frame hot) but it is not a fairness primitive nor an io primitive
struct __reschedule_awaitable {
  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();                   // already suspended here; safe to publish (same rule as fork)
    __f->__pushed_kind = __frame_base::__kind_plain;      // a rescheduled frame is never steal-counted
    worker *__w = current_worker();
    if ( __w != nullptr && __w->deque.push_bottom(__f) ) {
      __notify_work();
      return true;      // suspended; a worker (maybe this one) resumes it
    }
    return false;      // off-engine or full deque: no-op yield (resume inline)
  }

  void
  await_resume() const noexcept
  {
  }
};

[[nodiscard]] inline __reschedule_awaitable
reschedule() noexcept
{
  return {};
}

struct __reschedule_fair_awaitable {
  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    worker *__w = current_worker();
#if defined(MICRON_CORO_URING)
    if ( __w != nullptr && __global_engine != nullptr && __io.any_live.get(micron::memory_order_acquire) != 0 ) {
      u32 __seed = __w->id * 2654435761u + 1u;
      (void)__global_engine->__drain_io(__w, __seed);
    }
#endif
    __frame_base *__f = &__h.promise();      // safe to publish
    __f->__pushed_kind = __frame_base::__kind_plain;
    if ( __global_engine != nullptr && __global_engine->inbox.push(__f) ) {
      // same wake pairing as engine::submit
#if defined(MICRON_CORO_GLOBAL_SIGNAL)
      __cl_signal.fetch_add(1, micron::memory_order_seq_cst);
      if ( __cl_sleepers.get(micron::memory_order_seq_cst) != 0 ) micron::wake_futex(__cl_signal.ptr(), 1);
#else
      __cl_wake_one<true>();
#endif
      return true;
    }
    if ( __w != nullptr && __w->deque.push_bottom(__f) ) {      // fall back to the LIFO route
      __notify_work();
      return true;
    }
    return false;
  }

  void
  await_resume() const noexcept
  {
  }
};

[[nodiscard]] inline __reschedule_fair_awaitable
reschedule_fair() noexcept
{
  return {};
}

struct __sel_state {
  micron::atomic_token<i32> __winner{ -1 };
  micron::atomic_token<usize> __refs;
  __frame_base *__parent = nullptr;

  struct __node {
    __sel_state *__st;
    i32 __idx;
  } *__nodes = nullptr;      // heap array sized to n (no fixed cap); freed with the state on the last __rel

  explicit __sel_state(usize __r, usize __n) noexcept : __refs(__r), __nodes(__n ? new __node[__n] : nullptr) { }

  ~__sel_state() noexcept
  {
    if ( __nodes != nullptr ) delete[] __nodes;
  }

  void
  __rel() noexcept
  {
    if ( __refs.sub_fetch(1, micron::memory_order_acq_rel) == 0 ) delete this;      // ~__sel_state frees __nodes
  }
};

inline void
__sel_fire(usize __t) noexcept
{
  __sel_state::__node *__n = reinterpret_cast<__sel_state::__node *>(__t);
  __sel_state *__s = __n->__st;
  i32 __exp = -1;
  if ( __s->__winner.compare_exchange_strong(__exp, __n->__idx, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __s->__rel();
    return;
  }
  if ( __exp == -2 ) {
    i32 __e2 = -2;
    if ( __s->__winner.compare_exchange_strong(__e2, __n->__idx, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      __global_engine->submit(__s->__parent);
  }
  __s->__rel();
}

template<class T> struct [[nodiscard]] __when_any_awaiter {
  micron::futex_future<T> *__futs;
  usize __n;
  __sel_state *__st = nullptr;

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    for ( usize __i = 0; __i < __n; ++__i )
      if ( __futs[__i].__shared() == nullptr ) [[unlikely]]
        exc<except::future_error>("micron::when_any: co_await on a future with no shared state");
    __st = new __sel_state(__n + 1u, __n);
    __st->__parent = &__h.promise();
    for ( usize __i = 0; __i < __n; ++__i ) {
      __st->__nodes[__i] = { __st, static_cast<i32>(__i) };
      bool __ready = __futs[__i].__shared()->__arm_waiter(reinterpret_cast<usize>(&__st->__nodes[__i]), __sel_fire);
      if ( __ready ) __sel_fire(reinterpret_cast<usize>(&__st->__nodes[__i]));      // already ready: fire inline
    }
    i32 __exp = -1;
    if ( __st->__winner.compare_exchange_strong(__exp, -2, micron::memory_order_acq_rel, micron::memory_order_acquire) ) return true;
    return false;
  }

  i32
  await_resume() noexcept
  {
    i32 __w = __st->__winner.get(micron::memory_order_acquire);
    __st->__rel();
    return (__w >= 0) ? __w : -1;
  }
};

// co_await when_any(futs, n) -> index of the first ready future
template<class T>
[[nodiscard]] __when_any_awaiter<T>
when_any(micron::futex_future<T> *__futs, usize __n) noexcept
{
  return __when_any_awaiter<T>{ __futs, __n };
}

// TODO: implement select over arbitrary tasks
struct [[nodiscard]] __sleep_awaiter {
  timespec_t __deadline;
  __timer_node __node{};
#if defined(MICRON_CORO_URING)
  __io_op __op{};
  micron::uring::ktimespec __kts{};
#endif

  bool
  await_ready() noexcept
  {
    timespec_t __now{};
    micron::clock_gettime(micron::clock_monotonic, __now);
    return __ts_le(__deadline, __now);      // already elapsed -> don't suspend
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
#if defined(MICRON_CORO_URING)
    if ( __io.any_live.get(micron::memory_order_acquire) != 0 ) {
      __op.__f = &__h.promise();
      __kts = { static_cast<i64>(__deadline.tv_sec), static_cast<i64>(__deadline.tv_nsec) };
      micron::uring::sqe __q;
      micron::uring::prep_timeout(&__q, &__kts, micron::uring::timeout_abs);
      __wring *__r = __io_own_ring();
      bool __ok = false;
      if ( __r != nullptr ) {
        __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
        __ok = __io_submit_own(*__r, __q, reinterpret_cast<u64>(&__op));
      } else if ( __io_fb.__live.get(micron::memory_order_acquire) != 0 ) {
        __r = &__io_fb;
        __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
        __ok = __io_submit_fb(__q, reinterpret_cast<u64>(&__op));
      }
      if ( __ok ) {
        u32 __exp = __io_st_submitted;
        if ( __op.__st.compare_exchange_strong(__exp, __io_st_suspended, micron::memory_order_acq_rel, micron::memory_order_acquire) )
          return true;
        __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
        return false;
      }
      if ( __r != nullptr ) __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
    }
#endif
    __ensure_timer_thread();
    __node.__deadline = __deadline;
    __node.__frame = &__h.promise();
    __global_engine->pending_timers.fetch_add(1, micron::memory_order_acq_rel);
    __timer_lock();
    __node.__next = __timer_head;
    __timer_head = &__node;
    __timer_unlock();
    __timer_epoch.fetch_add(1, micron::memory_order_release);
    micron::wake_futex(__timer_epoch.ptr(), 1);
    return true;
  }

  void
  await_resume() const noexcept
  {
  }
};

[[nodiscard]] inline __sleep_awaiter
sleep_until(const timespec_t &__deadline_monotonic) noexcept
{
  return __sleep_awaiter{ __deadline_monotonic };
}

[[nodiscard]] inline __sleep_awaiter
sleep_for(u64 __nanos) noexcept
{
  timespec_t __dl{};
  micron::clock_gettime(micron::clock_monotonic, __dl);
  __dl.tv_sec += static_cast<decltype(__dl.tv_sec)>(__nanos / 1000000000ull);
  __dl.tv_nsec += static_cast<decltype(__dl.tv_nsec)>(__nanos % 1000000000ull);
  if ( __dl.tv_nsec >= 1000000000l ) {
    __dl.tv_sec += 1;
    __dl.tv_nsec -= 1000000000l;
  }
  return __sleep_awaiter{ __dl };
}

[[nodiscard]] inline __sleep_awaiter
sleep_for_ms(u64 __ms) noexcept
{
  return sleep_for(__ms * 1000000ull);
}

};      // namespace coro

// ADL operator co_await for a futex_future
template<class T>
inline coro::__futex_future_awaiter<T>
operator co_await(const micron::futex_future<T> &__f) noexcept
{
  return coro::__futex_future_awaiter<T>{ __f.__shared() };
}

};      // namespace micron
