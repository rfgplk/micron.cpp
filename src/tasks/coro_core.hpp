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
#include "../types.hpp"

#include "frame.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scheduler core
//
// outside task.hpp so the task promise final_suspend can call __child_complete without an include cycle

namespace micron
{
namespace coro
{

inline constexpr usize __cl_deque_cap = 1024;

struct worker {

#if defined(MICRON_CORO_FIXED_DEQUE)
  micron::chase_lev<__frame_base *, __cl_deque_cap> deque;
#else
  micron::chase_lev_grow<__frame_base *, __cl_deque_cap> deque;      // stealable continuations (growable)
#endif
  micron::atomic_token<u32> has_work{ 0 };
  u32 id = 0;
};

inline thread_local worker *__cur_worker = nullptr;

[[gnu::always_inline]] inline worker *
current_worker() noexcept
{
  return __cur_worker;
}

inline micron::atomic_token<u32> __cl_signal{ 0 };
inline micron::atomic_token<u32> __cl_sleepers{ 0 };

[[gnu::always_inline]] inline void
__notify_work() noexcept
{
  if ( __cl_sleepers.get(micron::memory_order_relaxed) != 0 ) {
    __cl_signal.fetch_add(1, micron::memory_order_release);
    micron::wake_futex(__cl_signal.ptr(), 1);
  }
}

[[gnu::always_inline]] inline std::coroutine_handle<>
__child_complete(__frame_base *child) noexcept
{
  __frame_base *parent = child->__parent;
  worker *w = __cur_worker;
  __frame_base *cont = (w != nullptr) ? w->deque.pop_bottom() : nullptr;
  if ( cont != nullptr ) {

    return cont->__self;
  }

  const u32 r = parent->__joins.add_fetch(1, micron::memory_order_acq_rel);
  if ( (r & __frame_base::__joiner_bit) && (r & __frame_base::__count_mask) == parent->__expected ) return parent->__self;
  return std::noop_coroutine();
}

[[gnu::always_inline]] inline std::coroutine_handle<>
__join_suspend(__frame_base *self) noexcept
{
  const u32 s = self->__steals.get(micron::memory_order_acquire);
  if ( s == 0 ) return self->__self;
  self->__expected = s;
  const u32 r = self->__joins.add_fetch(__frame_base::__joiner_bit, micron::memory_order_acq_rel);
  if ( (r & __frame_base::__count_mask) == s ) {

    self->__joins.store(0, micron::memory_order_relaxed);
    self->__steals.store(0, micron::memory_order_relaxed);
    self->__expected = 0;
    return self->__self;
  }
  return std::noop_coroutine();
}

[[gnu::always_inline]] inline void
__join_resume(__frame_base *self) noexcept
{
  self->__joins.store(0, micron::memory_order_relaxed);
  self->__steals.store(0, micron::memory_order_relaxed);
  self->__expected = 0;
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
