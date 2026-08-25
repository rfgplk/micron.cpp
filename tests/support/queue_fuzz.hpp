//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/types.hpp"

namespace mtest::queue_fuzz
{

struct prng {
  u64 state;

  explicit prng(u64 seed) noexcept : state(seed ? seed : 0x9e3779b97f4a7c15ULL) { }

  u64
  next() noexcept
  {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }

  usize
  below(usize limit) noexcept
  {
    return limit ? static_cast<usize>(next() % limit) : 0;
  }
};

template<typename T, usize Max> struct fifo_oracle {
  T values[Max];
  usize first = 0;
  usize length = 0;

  bool
  push(const T &value)
  {
    if ( length == Max ) return false;
    values[(first + length++) % Max] = value;
    return true;
  }

  bool
  pop(T &out)
  {
    if ( length == 0 ) return false;
    out = values[first];
    first = (first + 1) % Max;
    --length;
    return true;
  }

  const T &
  at(usize index) const
  {
    return values[(first + index) % Max];
  }

  const T &
  front() const
  {
    return at(0);
  }

  const T &
  back() const
  {
    return at(length - 1);
  }

  void
  clear() noexcept
  {
    first = 0;
    length = 0;
  }
};

struct owning_value {
  static inline i64 live = 0;
  int *owned;
  const owning_value *self;

  explicit owning_value(int value = 0) : owned(new int(value)), self(this) { ++live; }

  owning_value(const owning_value &o) : owned(new int(*o.owned)), self(this) { ++live; }

  owning_value(owning_value &&o) noexcept : owned(o.owned), self(this)
  {
    o.owned = nullptr;
    ++live;
  }

  owning_value &
  operator=(const owning_value &o)
  {
    if ( !owned ) owned = new int;
    *owned = *o.owned;
    self = this;
    return *this;
  }

  owning_value &
  operator=(owning_value &&o) noexcept
  {
    if ( this == &o ) return *this;
    delete owned;
    owned = o.owned;
    o.owned = nullptr;
    self = this;
    return *this;
  }

  ~owning_value()
  {
    delete owned;
    --live;
  }

  int
  value() const
  {
    if ( self != this || !owned ) throw "queue owning value was byte-relocated or moved-from";
    return *owned;
  }
};

struct no_move_assign {
  static inline i64 live = 0;
  int value;
  const no_move_assign *self;

  explicit no_move_assign(int v = 0) : value(v), self(this) { ++live; }

  no_move_assign(const no_move_assign &o) : value(o.value), self(this) { ++live; }

  no_move_assign(no_move_assign &&o) noexcept : value(o.value), self(this) { ++live; }

  no_move_assign &
  operator=(const no_move_assign &o)
  {
    value = o.value;
    self = this;
    return *this;
  }

  no_move_assign &operator=(no_move_assign &&) = delete;

  ~no_move_assign() { --live; }

  bool
  valid() const noexcept
  {
    return self == this;
  }
};

struct non_default_value {
  static inline i64 live = 0;
  int value;

  non_default_value() = delete;

  explicit non_default_value(int v) : value(v) { ++live; }

  non_default_value(const non_default_value &o) : value(o.value) { ++live; }

  non_default_value(non_default_value &&o) noexcept : value(o.value) { ++live; }

  non_default_value &
  operator=(const non_default_value &o)
  {
    value = o.value;
    return *this;
  }

  non_default_value &
  operator=(non_default_value &&o) noexcept
  {
    value = o.value;
    return *this;
  }

  ~non_default_value() { --live; }
};

struct alignas(128) over_aligned_value {
  u64 value;

  explicit over_aligned_value(u64 v = 0) : value(v) { }

  over_aligned_value(const over_aligned_value &) = default;
  over_aligned_value(over_aligned_value &&) = default;
  over_aligned_value &operator=(const over_aligned_value &) = default;
  over_aligned_value &operator=(over_aligned_value &&) = default;
};

enum class scoped_value : u32 { zero = 0, one = 1, high = 0xfedcba98u };

}      // namespace mtest::queue_fuzz
