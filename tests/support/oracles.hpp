//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Reference implementations and generators for property/oracle tests. The
// references are obviously-correct (linear, no SIMD, no allocator games) so
// they can serve as the "ground truth" against which micron's containers are
// diffed. STL is forbidden per CLAUDE.md so these are hand-rolled on top of
// abc_allocator.
//
// For tests that stay below kCap, use the fixed-capacity variants — they
// avoid reallocation entirely and so are simpler to reason about than the
// type under test.

#include "../../src/except.hpp"
#include "../../src/memory/cmemory.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

namespace mtest
{

// fixed-capacity reference vector. T must be trivially copyable for the
// straight memcpy paths used here; for non-trivial T tests should compare
// element-wise via the predicates below.
template<typename T, usize Cap = 1024> class reference_vector
{
  T data_[Cap];
  usize len_ = 0;

public:
  using value_type = T;

  usize
  size(void) const noexcept
  {
    return len_;
  }

  usize
  capacity(void) const noexcept
  {
    return Cap;
  }

  bool
  empty(void) const noexcept
  {
    return len_ == 0;
  }

  T &
  operator[](usize i)
  {
    return data_[i];
  }

  const T &
  operator[](usize i) const
  {
    return data_[i];
  }

  void
  push_back(const T &v)
  {
    if ( len_ >= Cap ) micron::exc<micron::except::runtime_error>("reference_vector: capacity exceeded");
    data_[len_++] = v;
  }

  void
  pop_back(void) noexcept
  {
    if ( len_ > 0 ) --len_;
  }

  void
  erase(usize idx)
  {
    if ( idx >= len_ ) return;
    for ( usize i = idx; i + 1 < len_; ++i ) data_[i] = data_[i + 1];
    --len_;
  }

  void
  insert(usize idx, const T &v)
  {
    if ( len_ >= Cap ) micron::exc<micron::except::runtime_error>("reference_vector: capacity exceeded");
    if ( idx > len_ ) idx = len_;
    for ( usize i = len_; i > idx; --i ) data_[i] = data_[i - 1];
    data_[idx] = v;
    ++len_;
  }

  void
  clear(void) noexcept
  {
    len_ = 0;
  }

  // diff against any container with size() and operator[](usize)
  template<typename Other>
  bool
  matches(const Other &o) const
  {
    if ( o.size() != len_ ) return false;
    for ( usize i = 0; i < len_; ++i )
      if ( o[i] != data_[i] ) return false;
    return true;
  }
};

// fixed-capacity reference map (sorted by key, linear find). Use diff to
// compare against micron's hash maps.
template<typename K, typename V, usize Cap = 1024> class reference_map
{
  K keys_[Cap];
  V vals_[Cap];
  usize len_ = 0;

  usize
  __find_idx(const K &k) const noexcept
  {
    for ( usize i = 0; i < len_; ++i )
      if ( keys_[i] == k ) return i;
    return Cap;
  }

public:
  usize
  size(void) const noexcept
  {
    return len_;
  }

  bool
  empty(void) const noexcept
  {
    return len_ == 0;
  }

  usize
  capacity(void) const noexcept
  {
    return Cap;
  }

  bool
  contains(const K &k) const noexcept
  {
    return __find_idx(k) < Cap;
  }

  V *
  find(const K &k) noexcept
  {
    auto i = __find_idx(k);
    return (i < Cap) ? &vals_[i] : nullptr;
  }

  const V *
  find(const K &k) const noexcept
  {
    auto i = __find_idx(k);
    return (i < Cap) ? &vals_[i] : nullptr;
  }

  // returns true if inserted, false if updated
  bool
  insert(const K &k, const V &v)
  {
    auto i = __find_idx(k);
    if ( i < Cap ) {
      vals_[i] = v;
      return false;
    }
    if ( len_ >= Cap ) micron::exc<micron::except::runtime_error>("reference_map: capacity exceeded");
    keys_[len_] = k;
    vals_[len_] = v;
    ++len_;
    return true;
  }

  bool
  erase(const K &k)
  {
    auto i = __find_idx(k);
    if ( i >= len_ ) return false;
    for ( usize j = i; j + 1 < len_; ++j ) {
      keys_[j] = keys_[j + 1];
      vals_[j] = vals_[j + 1];
    }
    --len_;
    return true;
  }

  void
  clear(void) noexcept
  {
    len_ = 0;
  }
};

// PRNG wrapper around snowball's xorshift. seed() with a chosen value for
// reproducibility, or with cycle counter for randomness.
class prng
{
  u64 state_;

public:
  explicit prng(u64 s = 0xc001cafedeadbeefULL) : state_(s ? s : 0xdeadbeefULL) { }

  void
  seed(u64 s) noexcept
  {
    state_ = s ? s : 0xdeadbeefULL;
  }

  u64
  next(void) noexcept
  {
    return snowball::__impl::__xorshift64(state_);
  }

  u64
  next_in(u64 hi) noexcept
  {
    return hi ? (next() % hi) : 0;
  }

  i32
  int32(void) noexcept
  {
    return static_cast<i32>(next() & 0x7fffffffULL);
  }
};

};      // namespace mtest
