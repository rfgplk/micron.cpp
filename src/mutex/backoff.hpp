//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__backoff.hpp"
#include "../bits/__pause.hpp"

#include "../atomic/atomic.hpp"
#include "../sync/yield.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// acquisition counters for the lock family
//
// the escalation policy itself -- spin_policy, spin_only/spin_yield/spin_park, spin_step,
// __lock_backoff, default_backoff, park_backoff -- lives in bits/__backoff.hpp, which this header
// re-exports so every existing includer keeps compiling. it had to move down: atomic<T>'s internal
// spinlock wants the yield tier too, and atomic/atomic.hpp is BELOW this header. what stays here is
// the half that genuinely needs atomic_token.

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// opt-in acquisition counters (MICRON_CORO_STATS style)

class __lock_stats
{
#if defined(MICRON_LOCK_STATS)
  atomic_token<u64> __acq{ 0 };
  atomic_token<u64> __spin{ 0 };
  atomic_token<u64> __yield{ 0 };
  atomic_token<u64> __park{ 0 };
#endif

public:
  constexpr __lock_stats() noexcept = default;

  void
  note([[maybe_unused]] spin_step s) noexcept
  {
#if defined(MICRON_LOCK_STATS)
    switch ( s ) {
    case spin_step::spun:
      __spin.fetch_add(1, memory_order::relaxed);
      break;
    case spin_step::yielded:
      __yield.fetch_add(1, memory_order::relaxed);
      break;
    case spin_step::park:
      __park.fetch_add(1, memory_order::relaxed);
      break;
    }
#endif
  }

  void
  note_acquire() noexcept
  {
#if defined(MICRON_LOCK_STATS)
    __acq.fetch_add(1, memory_order::relaxed);
#endif
  }

#if defined(MICRON_LOCK_STATS)
#define __micron_lock_stat(name, member)                                                                                                   \
  [[nodiscard]] u64 name() const noexcept { return member.get(memory_order::relaxed); }
#else
#define __micron_lock_stat(name, member)                                                                                                   \
  [[nodiscard]] constexpr u64 name() const noexcept { return 0; }
#endif

  __micron_lock_stat(acquires, __acq) __micron_lock_stat(spins, __spin) __micron_lock_stat(yields, __yield)
      __micron_lock_stat(parks, __park)

#undef __micron_lock_stat
};

// a backoff bound to a stats sink
template<spin_policy P> class __counted_backoff
{
  __lock_backoff<P> __bo;
  __lock_stats *__st;

public:
  explicit constexpr __counted_backoff(__lock_stats &s) noexcept : __bo(), __st(&s) { }

  spin_step
  next() noexcept
  {
    const spin_step s = __bo.next();
    __st->note(s);
    return s;
  }

  void
  relax() noexcept
  {
    if ( next() == spin_step::park ) micron::__sched_yield();
  }

  void
  reset() noexcept
  {
    __bo.reset();
  }

  [[nodiscard]] u32
  rounds() const noexcept
  {
    return __bo.rounds();
  }
};

};      // namespace micron
