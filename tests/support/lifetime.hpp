//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/atomic/atomic.hpp"
#include "../../src/bits/__pause.hpp"
#include "../../src/syscall.hpp"
#include "../../src/types.hpp"

#ifndef STRESS_SCALE
#define STRESS_SCALE 1
#endif

namespace ltest
{

inline constexpr u64 stress_scale = STRESS_SCALE;

[[nodiscard]] inline constexpr u64
scaled(u64 base) noexcept
{
  return base * stress_scale;
}

[[nodiscard]] inline i32
this_tid(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

struct live_registry {
  static constexpr usize __bits = 16;
  static constexpr usize __size = usize(1) << __bits;
  static constexpr usize __mask = __size - 1;

  micron::atomic_token<u64> slot[__size];
  micron::atomic_token<u64> collisions;
  micron::atomic_token<u64> tracked_n;

  [[nodiscard]] static usize
  bucket(u64 a) noexcept
  {
    a >>= 4;
    a *= 0x9E3779B97F4A7C15ull;
    return static_cast<usize>(a >> (64 - __bits)) & __mask;
  }

  bool
  note_live(const void *p) noexcept
  {
    const u64 a = reinterpret_cast<u64>(p);
    const usize b = bucket(a);
    u64 expected = 0;
    if ( slot[b].compare_exchange_strong(expected, a, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
      tracked_n.fetch_add(1, micron::memory_order_relaxed);
      return false;
    }
    if ( expected == a ) {
      collisions.fetch_add(1, micron::memory_order_acq_rel);
      return true;
    }
    return false;
  }

  void
  note_dead(const void *p) noexcept
  {
    const u64 a = reinterpret_cast<u64>(p);
    const usize b = bucket(a);
    u64 expected = a;
    if ( slot[b].compare_exchange_strong(expected, 0ull, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      tracked_n.fetch_sub(1, micron::memory_order_relaxed);
  }

  void
  reset(void) noexcept
  {
    for ( usize i = 0; i < __size; ++i ) slot[i].store(0, micron::memory_order_relaxed);
    collisions.store(0, micron::memory_order_release);
    tracked_n.store(0, micron::memory_order_release);
  }
};

inline constexpr u64 __live_magic = 0xC0FFEE5711FEA11Eull;
inline constexpr u64 __dead_magic = 0xDEAD10CCDEAD10CCull;

template<int Tag = 0> struct tracked {
  static inline micron::atomic_token<u64> ctor{ 0 };
  static inline micron::atomic_token<u64> copy_ctor{ 0 };
  static inline micron::atomic_token<u64> move_ctor{ 0 };
  static inline micron::atomic_token<u64> dtor{ 0 };
  static inline micron::atomic_token<u64> copy_assign{ 0 };
  static inline micron::atomic_token<u64> move_assign{ 0 };
  static inline micron::atomic_token<u64> bad_magic{ 0 };
  static inline micron::atomic_token<u64> double_dtor{ 0 };
  static inline micron::atomic_token<u64> foreign_dtor{ 0 };
  static inline live_registry *reg = nullptr;

  u64 magic;
  i32 owner;
  i64 v;

  void
  __born(void) noexcept
  {
    if ( reg != nullptr ) (void)reg->note_live(this);
  }

  tracked(void) noexcept : magic(__live_magic), owner(this_tid()), v(0)
  {
    ctor.fetch_add(1, micron::memory_order_relaxed);
    __born();
  }

  explicit tracked(i64 x) noexcept : magic(__live_magic), owner(this_tid()), v(x)
  {
    ctor.fetch_add(1, micron::memory_order_relaxed);
    __born();
  }

  tracked(const tracked &o) noexcept : magic(__live_magic), owner(this_tid()), v(o.v)
  {
    copy_ctor.fetch_add(1, micron::memory_order_relaxed);
    __born();
  }

  tracked(tracked &&o) noexcept : magic(__live_magic), owner(this_tid()), v(o.v)
  {
    o.v = 0;
    move_ctor.fetch_add(1, micron::memory_order_relaxed);
    __born();
  }

  tracked &
  operator=(const tracked &o) noexcept
  {
    v = o.v;
    copy_assign.fetch_add(1, micron::memory_order_relaxed);
    return *this;
  }

  tracked &
  operator=(tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    move_assign.fetch_add(1, micron::memory_order_relaxed);
    return *this;
  }

  ~tracked(void) noexcept
  {
    if ( magic == __dead_magic ) {
      double_dtor.fetch_add(1, micron::memory_order_relaxed);
      return;
    }
    if ( magic != __live_magic ) {
      bad_magic.fetch_add(1, micron::memory_order_relaxed);
      return;
    }
    if ( owner != this_tid() ) foreign_dtor.fetch_add(1, micron::memory_order_relaxed);
    magic = __dead_magic;
    if ( reg != nullptr ) reg->note_dead(this);
    dtor.fetch_add(1, micron::memory_order_relaxed);
  }

  bool
  operator==(const tracked &o) const noexcept
  {
    return v == o.v;
  }

  bool
  operator!=(const tracked &o) const noexcept
  {
    return v != o.v;
  }

  [[nodiscard]] static u64
  born(void) noexcept
  {
    return ctor.get(micron::memory_order_acquire) + copy_ctor.get(micron::memory_order_acquire)
           + move_ctor.get(micron::memory_order_acquire);
  }

  [[nodiscard]] static i64
  live(void) noexcept
  {
    return static_cast<i64>(born()) - static_cast<i64>(dtor.get(micron::memory_order_acquire));
  }

  [[nodiscard]] static u64
  faults(void) noexcept
  {
    return bad_magic.get(micron::memory_order_acquire) + double_dtor.get(micron::memory_order_acquire);
  }

  static void
  reset(void) noexcept
  {
    ctor.store(0, micron::memory_order_release);
    copy_ctor.store(0, micron::memory_order_release);
    move_ctor.store(0, micron::memory_order_release);
    dtor.store(0, micron::memory_order_release);
    copy_assign.store(0, micron::memory_order_release);
    move_assign.store(0, micron::memory_order_release);
    bad_magic.store(0, micron::memory_order_release);
    double_dtor.store(0, micron::memory_order_release);
    foreign_dtor.store(0, micron::memory_order_release);
  }
};

struct barrier_t {
  micron::atomic_token<u32> count{ 0 };
  micron::atomic_token<u32> sense{ 0 };
  u32 n = 0;
};

inline void
barrier_wait(barrier_t &b, u32 &my_sense) noexcept
{
  my_sense ^= 1u;
  if ( b.count.add_fetch(1u, micron::memory_order_acq_rel) == b.n ) {
    b.count.store(0u, micron::memory_order_release);
    b.sense.store(my_sense, micron::memory_order_release);
  } else {
    while ( b.sense.get(micron::memory_order_acquire) != my_sense ) __cpu_pause();
  }
}

[[nodiscard]] inline i32
fd_watermark(void) noexcept
{
  const long fd = micron::syscall(SYS_openat, -100, "/dev/null", 0 /*O_RDONLY*/, 0);
  if ( fd < 0 ) return -1;
  micron::syscall(SYS_close, fd);
  return static_cast<i32>(fd);
}

[[nodiscard]] inline u64
rss_kb(void) noexcept
{
  const long fd = micron::syscall(SYS_openat, -100, "/proc/self/statm", 0 /*O_RDONLY*/, 0);
  if ( fd < 0 ) return 0;
  char buf[128];
  const long n = micron::syscall(SYS_read, fd, buf, sizeof(buf) - 1);
  micron::syscall(SYS_close, fd);
  if ( n <= 0 ) return 0;
  buf[n] = '\0';

  usize i = 0;
  while ( buf[i] && buf[i] != ' ' ) ++i;
  while ( buf[i] == ' ' ) ++i;
  u64 pages = 0;
  bool any = false;
  while ( buf[i] >= '0' && buf[i] <= '9' ) {
    pages = pages * 10u + static_cast<u64>(buf[i] - '0');
    any = true;
    ++i;
  }
  if ( !any ) return 0;
  return pages * 4u;
}

}      // namespace ltest
