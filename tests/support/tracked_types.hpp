//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Instrumented value types for container/allocator tests. Evolves the inline
// Tracked struct from tests/rigor/vector.cpp:22-65. Each Tracked<N> has a
// disjoint set of static counters so two tests in the same TU can use
// Tracked<0> and Tracked<1> without interfering.

#include "../../src/except.hpp"
#include "../../src/types.hpp"

namespace mtest
{

template<int Tag = 0> struct Tracked {
  static inline usize ctor = 0;
  static inline usize copy_ctor = 0;
  static inline usize move_ctor = 0;
  static inline usize dtor = 0;
  static inline usize copy_assign = 0;
  static inline usize move_assign = 0;

  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++copy_ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++move_ctor;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    ++copy_assign;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    ++move_assign;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const Tracked &o) const
  {
    return v != o.v;
  }

  static usize
  live(void) noexcept
  {
    return (ctor + copy_ctor + move_ctor) - dtor;
  }

  static void
  reset(void) noexcept
  {
    ctor = 0;
    copy_ctor = 0;
    move_ctor = 0;
    dtor = 0;
    copy_assign = 0;
    move_assign = 0;
  }
};

// Trait selectors for Throwing<>
enum class throw_on : int { default_ctor, copy_ctor, move_ctor, dtor, copy_assign, move_assign };

// Configurable throwing type. Trip = how many of the selected op succeed
// before the next one throws. Trip = 0 means: throw on the very first call.
template<throw_on Trait, int Tag = 0> struct Throwing {
  static inline int trip = -1;      // -1 = never throw
  static inline int count = 0;
  static inline usize ctor = 0;
  static inline usize dtor = 0;
  int v;

  static void
  arm(int trip_count) noexcept
  {
    trip = trip_count;
    count = 0;
  }

  static void
  disarm(void) noexcept
  {
    trip = -1;
    count = 0;
  }

  static void
  reset(void) noexcept
  {
    trip = -1;
    count = 0;
    ctor = 0;
    dtor = 0;
  }

  Throwing() : v(0)
  {
    if constexpr ( Trait == throw_on::default_ctor ) {
      if ( trip >= 0 and count++ == trip ) throw micron::runtime{ "Throwing: configured default_ctor throw" };
    }
    ++ctor;
  }

  explicit Throwing(int x) : v(x) { ++ctor; }

  Throwing(const Throwing &o) : v(o.v)
  {
    if constexpr ( Trait == throw_on::copy_ctor ) {
      if ( trip >= 0 and count++ == trip ) throw micron::runtime{ "Throwing: configured copy_ctor throw" };
    }
    ++ctor;
  }

  Throwing(Throwing &&o) : v(o.v)
  {
    if constexpr ( Trait == throw_on::move_ctor ) {
      if ( trip >= 0 and count++ == trip ) throw micron::runtime{ "Throwing: configured move_ctor throw" };
    }
    o.v = -1;
    ++ctor;
  }

  Throwing &
  operator=(const Throwing &o)
  {
    if constexpr ( Trait == throw_on::copy_assign ) {
      if ( trip >= 0 and count++ == trip ) throw micron::runtime{ "Throwing: configured copy_assign throw" };
    }
    v = o.v;
    return *this;
  }

  Throwing &
  operator=(Throwing &&o)
  {
    if constexpr ( Trait == throw_on::move_assign ) {
      if ( trip >= 0 and count++ == trip ) throw micron::runtime{ "Throwing: configured move_assign throw" };
    }
    v = o.v;
    o.v = -1;
    return *this;
  }

  ~Throwing() noexcept(Trait != throw_on::dtor) { ++dtor; }

  bool
  operator==(const Throwing &o) const
  {
    return v == o.v;
  }
};

};      // namespace mtest
