//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../atomic/atomic.hpp"
#include "../queue/chase_lev.hpp"
#include "../sync/futex.hpp"
#include "../sync/yield.hpp"
#include "../types.hpp"

#include "frame.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scheduler core
//
// outside task.hpp so the task promise final_suspend can call __child_complete without an include cycle

namespace micron
{
namespace fiber
{
struct fiber;      // fwd
};

namespace coro
{

inline constexpr usize __cl_deque_cap = 1024;

struct alignas(64) worker {      // alignas: keep one worker's fields off its neighbors' cachelines

#if defined(MICRON_CORO_FIXED_DEQUE)
  micron::chase_lev<__frame_base *, __cl_deque_cap> deque;
#else
  micron::chase_lev_grow<__frame_base *, __cl_deque_cap> deque;      // stealable continuations (growable)
#endif
  micron::atomic_token<u32> has_work{ 0 };
  micron::atomic_token<u32> active{ 0 };      // this worker may be holding/running a continuation (quiescence scan)
  micron::fiber::fiber *hot = nullptr;        // resident dispatch fiber (cl_sched __run/__cl_hot_entry)
  u32 id = 0;
  u32 tick = 0;      // __find cadence counter (periodic inbox poll for root fairness)
};

inline thread_local worker *__cur_worker = nullptr;

[[gnu::always_inline]] inline worker *
current_worker() noexcept
{
  return __cur_worker;
}

alignas(64) inline micron::atomic_token<u32> __cl_searchers{ 0 };      // workers in the pre-park search spin

#if defined(MICRON_CORO_GLOBAL_SIGNAL)
inline constexpr u32 __cl_max_searchers = 2;
// separate cachelines: every __notify_work read of sleepers must not contend with signal RMWs
alignas(64) inline micron::atomic_token<u32> __cl_signal{ 0 };
alignas(64) inline micron::atomic_token<u32> __cl_sleepers{ 0 };

[[gnu::always_inline]] inline void
__notify_work() noexcept
{
  if ( __cl_sleepers.get(micron::memory_order_relaxed) != 0 ) {
    __cl_signal.fetch_add(1, micron::memory_order_release);
    micron::wake_futex(__cl_signal.ptr(), 1);
  }
}
#else
struct alignas(64) __cl_parkslot {
  micron::atomic_token<u32> epoch{ 0 };
};

inline __cl_parkslot __cl_park[32];
alignas(64) inline micron::atomic_token<u32> __cl_sleeper_mask{ 0 };      // bit i: worker i parked (or committing to park)

// claim-and-wake one parked worker
template<bool Strong>
[[gnu::always_inline]] inline void
__cl_wake_one() noexcept
{
  u32 __m;
  if constexpr ( Strong )
    __m = __cl_sleeper_mask.fetch_or(0, micron::memory_order_seq_cst);
  else
    __m = __cl_sleeper_mask.get(micron::memory_order_relaxed);
  while ( __m != 0 ) {
    const u32 __i = static_cast<u32>(__builtin_ctz(__m));
    const u32 __bit = 1u << __i;
    const u32 __old = __cl_sleeper_mask.fetch_and(~__bit, micron::memory_order_acq_rel);
    if ( __old & __bit ) {      // claimed an actual sleeper
      __cl_park[__i].epoch.fetch_add(1, micron::memory_order_release);
      micron::wake_futex(__cl_park[__i].epoch.ptr(), 1);
      return;
    }
    __m = __old & ~__bit;      // raced the sleepers own clear
  }
}

[[gnu::always_inline]] inline void
__notify_work() noexcept
{
  if ( __cl_sleeper_mask.get(micron::memory_order_relaxed) != 0 ) __cl_wake_one<false>();
}
#endif

[[gnu::always_inline]] inline void
__count_take(__frame_base *fb) noexcept
{
  if ( fb->__pushed_kind ) fb->__steals.fetch_add(1, micron::memory_order_acq_rel);
}

[[gnu::always_inline]] inline std::coroutine_handle<>
__child_complete(__frame_base *child) noexcept
{
  __frame_base *parent = child->__parent;
  worker *w = __cur_worker;
  __frame_base *cont = (w != nullptr) ? w->deque.pop_bottom() : nullptr;
  // balanced pop
  if ( cont == parent && cont != nullptr && cont->__pushed_kind == __frame_base::__kind_fork ) return parent->__self;
  // detached
  const u32 r = parent->__joins.add_fetch(1, micron::memory_order_acq_rel);
  const bool ready = (r & __frame_base::__joiner_bit)
                     && (r & __frame_base::__credit_mask) == ((r >> __frame_base::__expected_shift) & __frame_base::__expected_mask);
  if ( cont != nullptr ) {
    if ( cont == parent ) return parent->__self;
    __count_take(cont);
    if ( ready ) {
      cont->__pushed_kind = __frame_base::__kind_plain;
      while ( !w->deque.push_bottom(cont) ) micron::yield();
      __notify_work();
      return parent->__self;
    }
    return cont->__self;
  }
  return ready ? parent->__self : std::noop_coroutine();
}

[[gnu::always_inline]] inline std::coroutine_handle<>
__join_suspend(__frame_base *self) noexcept
{
  const u32 s = self->__steals.get(micron::memory_order_acquire);
  if ( s == 0 ) return self->__self;
  if ( s > __frame_base::__expected_mask ) [[unlikely]]
    __builtin_trap();      // > 32767 outstanding steals in one join epoch exceeds the packed word
  // TODO: error out properly
  const u32 r = self->__joins.add_fetch(__frame_base::__joiner_bit | (s << __frame_base::__expected_shift), micron::memory_order_acq_rel);
  if ( (r & __frame_base::__credit_mask) == s ) {
    self->__joins.store(0, micron::memory_order_relaxed);
    self->__steals.store(0, micron::memory_order_relaxed);
    return self->__self;
  }
  return std::noop_coroutine();
}

[[gnu::always_inline]] inline void
__join_resume(__frame_base *self) noexcept
{
  self->__joins.store(0, micron::memory_order_relaxed);
  self->__steals.store(0, micron::memory_order_relaxed);
  __frame_base *__c = self->__child_head;
  self->__child_head = nullptr;
  while ( __c != nullptr ) {
    __frame_base *__next = __c->__sibling;
    if ( __c->__self ) __c->__self.destroy();
    __c = __next;
  }
}

};      // namespace coro
};      // namespace micron
