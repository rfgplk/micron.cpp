//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../../atomic/atomic.hpp"
#include "../../linux/sys/uring.hpp"
#include "../../sync/yield.hpp"
#include "../../types.hpp"

#include "../coro_core.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// io_uring reactor state (-DMICRON_CORO_URING)
//
// NOTE: NOT libjkr
//
// enabled via -D/--def MICRON_CORO_URING

namespace micron
{
namespace coro
{

struct __io_op {
  __frame_base *__f = nullptr;
  i32 __res = 0;
};

inline constexpr u64 __io_tag_park = 1;        // the sentinel's futex-wait park sqe
inline constexpr u64 __io_tag_cancel = 2;      // cancel issued against an unfired park sqe

struct __reactor {
  micron::uring::ring ring;
  micron::atomic_token<u32> sq_lk{ 0 };           // guards get_sqe/advance_sq/enter(0)
  micron::atomic_token<u32> cq_lk{ 0 };           // guards peek_cqe (single reaper at a time)
  micron::atomic_token<u32> pending{ 0 };         // in-flight ops incl. timers (stop-drain + park gate)
  micron::atomic_token<i32> sentinel{ -1 };       // worker id parked inside the ring; -1 = none
  micron::atomic_token<u32> park_fired{ 0 };      // the park sqe's cqe was observed (by ANY reaper)
  bool live = false;                              // init succeeded at engine start
};

inline __reactor __io;

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

template<class Fill>
inline bool
__io_submit(u64 __ud, Fill &&__fill) noexcept
{
  __io_lock(__io.sq_lk);
  micron::uring::sqe *__s = __io.ring.get_sqe();
  if ( __s == nullptr ) {
    __io.ring.enter(0);      // flush a full SQ once
    __s = __io.ring.get_sqe();
    if ( __s == nullptr ) {
      __io_unlock(__io.sq_lk);
      return false;
    }
  }
  __fill(__s);
  __s->user_data = __ud;
  __io.ring.advance_sq();
  long __r = __io.ring.enter(0);
  __io_unlock(__io.sq_lk);
  return __r >= 0;
}

};      // namespace coro
};      // namespace micron

#endif
