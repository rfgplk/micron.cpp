//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Shared rigor support for the micron stack family (stack / fstack / sstack /
// fsstack / istack / constack / cactus_stack / fixed_stack).
//
// Provides: a deterministic PRNG, and three element types used to flush out
// lifetime bugs that a plain int can hide:
//   - tracked  : counts live/ctor/dtor (catches leaks & double-destroy)
//   - owner    : owns a heap allocation (ASan catches double-free / UAF / leak)
//   - move_only: non-copyable (forces the move-only code paths to instantiate)
//
// The actual LIFO oracle is just a std::vector used in each suite (STL is allowed
// as an oracle-only reference in tests, exactly as the trees/heap suites do; only
// <thread>/<atomic> are off-limits because of the pthread shim).

#include "../../src/types.hpp"

namespace stest
{

[[gnu::always_inline]] inline u64
splitmix64(u64 &s) noexcept
{
  u64 x = (s += 0x9E3779B97F4A7C15ULL);
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

// lifetime-counting value. `live` must return to 0 and ctor==dtor after any
// balanced sequence; any drift is a leak or a double-destroy.
struct tracked {
  static inline long live = 0;
  static inline long ctor = 0;
  static inline long dtor = 0;
  int v;

  tracked() : v(0)
  {
    ++ctor;
    ++live;
  }

  explicit tracked(int x) : v(x)
  {
    ++ctor;
    ++live;
  }

  tracked(const tracked &o) : v(o.v)
  {
    ++ctor;
    ++live;
  }

  tracked(tracked &&o) noexcept : v(o.v)
  {
    o.v = -1;
    ++ctor;
    ++live;
  }

  tracked &
  operator=(const tracked &o)
  {
    v = o.v;
    return *this;
  }

  tracked &
  operator=(tracked &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    return *this;
  }

  ~tracked()
  {
    ++dtor;
    --live;
  }

  bool
  operator==(const tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const tracked &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const tracked &o) const
  {
    return v < o.v;
  }

  static void
  reset() noexcept
  {
    live = ctor = dtor = 0;
  }

  static bool
  balanced() noexcept
  {
    return live == 0 && ctor == dtor;
  }
};

// owns a heap int via libc new/delete; under ASan a double-destroy is a
// double-free and a dropped-without-destroy is a leak.
struct owner {
  static inline long live = 0;
  int *p;

  owner() : p(new int(0)) { ++live; }

  explicit owner(int x) : p(new int(x)) { ++live; }

  owner(const owner &o) : p(new int(*o.p)) { ++live; }

  owner(owner &&o) noexcept : p(o.p)
  {
    o.p = nullptr;
    ++live;
  }

  owner &
  operator=(const owner &o)
  {
    if ( this != &o ) {
      int *np = new int(*o.p);
      delete p;
      p = np;
    }
    return *this;
  }

  owner &
  operator=(owner &&o) noexcept
  {
    if ( this != &o ) {
      delete p;
      p = o.p;
      o.p = nullptr;
    }
    return *this;
  }

  ~owner()
  {
    delete p;
    --live;
  }

  int
  val() const
  {
    return p ? *p : -0x7fffffff;
  }

  bool
  operator==(const owner &o) const
  {
    return val() == o.val();
  }

  bool
  operator!=(const owner &o) const
  {
    return val() != o.val();
  }

  bool
  operator<(const owner &o) const
  {
    return val() < o.val();
  }

  static void
  reset() noexcept
  {
    live = 0;
  }
};

// move-only: copy ctor / copy assign deleted. forces the rvalue/move paths.
struct move_only {
  int v;

  move_only() : v(0) { }

  explicit move_only(int x) : v(x) { }

  move_only(const move_only &) = delete;
  move_only &operator=(const move_only &) = delete;

  move_only(move_only &&o) noexcept : v(o.v) { o.v = -1; }

  move_only &
  operator=(move_only &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    return *this;
  }

  ~move_only() = default;

  bool
  operator==(const move_only &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const move_only &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const move_only &o) const
  {
    return v < o.v;
  }
};

}      // namespace stest
