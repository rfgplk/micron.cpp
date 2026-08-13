//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"
#include "__pause.hpp"

#include "../syscall.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lockfree plumbing

namespace micron
{

// exponential CAS backoff; doubles per failed attempt, saturates at 64 pauses
[[gnu::always_inline]] inline unsigned
__spin_backoff(unsigned b) noexcept
{
  for ( unsigned i = 0; i < b; ++i ) __cpu_pause();
  return (b < 64u) ? (b << 1u) : 64u;
}

// syscall free backoff
class __pause_backoff
{
  unsigned __b = 1u;

public:
  constexpr __pause_backoff() noexcept = default;

  void
  relax() noexcept
  {
    __b = __spin_backoff(__b);
  }

  void
  reset() noexcept
  {
    __b = 1u;
  }

  [[nodiscard]] unsigned
  width() const noexcept
  {
    return __b;
  }
};

// portable cache-line filler
template<usize N> struct __cache_pad {
  char __[N];
};

// no unique address should be free
template<> struct __cache_pad<0> {
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// escalating spin policy for the lock family

#ifndef MICRON_LOCK_SPIN_LIMIT
#define MICRON_LOCK_SPIN_LIMIT 64u      // ceiling on the pause burst
#endif

#ifndef MICRON_LOCK_SPIN_ROUNDS
#define MICRON_LOCK_SPIN_ROUNDS 6u      // pause bursts before escalating
#endif

#ifndef MICRON_LOCK_YIELD_ROUNDS
#define MICRON_LOCK_YIELD_ROUNDS 4u      // sched_yield rounds before reporting park
#endif

[[gnu::always_inline]] inline void
__sched_yield() noexcept
{
  (void)micron::syscall(SYS_sched_yield);
}

struct spin_policy {
  u32 pause_cap;
  u32 spin_rounds;
  u32 yield_rounds;
};

// never escalates
inline constexpr spin_policy spin_only{ MICRON_LOCK_SPIN_LIMIT, 0xFFFFFFFFu, 0u };

// pause -> yield -> park
inline constexpr spin_policy spin_yield{ MICRON_LOCK_SPIN_LIMIT, MICRON_LOCK_SPIN_ROUNDS, MICRON_LOCK_YIELD_ROUNDS };

// pause -> park; for locks with a real sleep
inline constexpr spin_policy spin_park{ MICRON_LOCK_SPIN_LIMIT, MICRON_LOCK_SPIN_ROUNDS, 0u };

enum class spin_step : u8 { spun, yielded, park };

template<spin_policy P> class __lock_backoff
{
  u32 __b;      // width of the next pause burst
  u32 __r;      // rounds completed

public:
  constexpr __lock_backoff() noexcept : __b(1u), __r(0u) { }

  spin_step
  next() noexcept
  {
    ++__r;
    if constexpr ( P.pause_cap != 0u ) {
      if ( __r <= P.spin_rounds ) {
        for ( u32 i = 0; i < __b; ++i ) __cpu_pause();
        __b <<= 1u;
        if ( __b > P.pause_cap ) __b = P.pause_cap;
        return spin_step::spun;
      }
    }
    if constexpr ( P.yield_rounds != 0u ) {
      if ( __r <= P.spin_rounds + P.yield_rounds ) {
        micron::__sched_yield();
        return spin_step::yielded;
      }
    }
    return spin_step::park;
  }

  // for locks with no sleep
  void
  relax() noexcept
  {
    if ( next() == spin_step::park ) micron::__sched_yield();
  }

  void
  reset() noexcept
  {
    __b = 1u;
    __r = 0u;
  }

  [[nodiscard]] u32
  rounds() const noexcept
  {
    return __r;
  }

  [[nodiscard]] u32
  width() const noexcept
  {
    return __b;
  }
};

using default_backoff = __lock_backoff<spin_yield>;
using park_backoff = __lock_backoff<spin_park>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lock-misuse guards
//
// (releasing a lock you do not hold is a programming error)

[[noreturn]] [[gnu::cold]] inline void
__lock_misuse_trap(const char *__m) noexcept
{
  usize __n = 0;
  while ( __m[__n] ) ++__n;
  (void)micron::syscall(SYS_write, 2, "micron lock misuse: ", 20);
  (void)micron::syscall(SYS_write, 2, __m, __n);
  (void)micron::syscall(SYS_write, 2, "\n", 1);
  __builtin_trap();
}

};      // namespace micron

#ifndef MICRON_LOCK_DEBUG
#define __micron_lock_misuse(msg) ((void)0)
#else
#define __micron_lock_misuse(msg) (micron::__lock_misuse_trap(msg))
#endif
