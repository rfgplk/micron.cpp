//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../bits/__container.hpp"
#include "../type_traits.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../tags.hpp"
#include "../types.hpp"

namespace micron
{

// c++ version of python's bisect array, which maintains sorted order
template <is_regular_object T, usize N>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))     // avoid weird stuff with N = 0
class bisect_array
{
  alignas(64) T stack[N];
  usize length = 0;

  usize
  __bisect_left(const T &val) const noexcept
  {
    usize low = 0, high = length;
    while ( low < high ) {
      usize mid = low + (high - low) / 2;
      if ( stack[mid] < val )
        low = mid + 1;
      else
        high = mid;
    }
    return low;
  }

  usize
  __bisect_right(const T &val) const noexcept
  {
    usize low = 0, high = length;
    while ( low < high ) {
      usize mid = low + (high - low) / 2;
      if ( val < stack[mid] )
        high = mid;
      else
        low = mid + 1;
    }
    return low;
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~bisect_array() { __impl_container::destroy<N>(micron::addr(stack[0])); }

  bisect_array() { __impl_container::destroy<N>(micron::addr(stack[0])); }

  template <is_container A>
    requires(!micron::is_same_v<A, bisect_array>)
  bisect_array(const A &o)
  {
    if ( o.size() < N )
      exc<except::runtime_error>("micron::bisect_array bisect_array(const&) invalid size");
    __impl_container::copy<N>(micron::addr(o[0]), micron::addr(stack[0]));
  }

  template <is_container A>
    requires(!micron::is_same_v<A, bisect_array>)
  bisect_array(A &&o)
  {
    if ( o.size() < N )
      exc<except::runtime_error>("micron::bisect_array bisect_array(&&) invalid size");
    __impl_container::move<N>(micron::addr(o[0]), micron::addr(stack[0]));
  }

  bisect_array(const bisect_array &o) { __impl_container::copy<N>(micron::addr(o.stack[0]), micron::addr(stack[0])); }

  bisect_array(bisect_array &&o) { __impl_container::move<N>(micron::addr(o.stack[0]), micron::addr(stack[0])); }

  usize
  size() const noexcept
  {
    return length;
  }

  bool
  full() const noexcept
  {
    return length == N;
  }

  bool
  empty() const noexcept
  {
    return length == 0;
  }

  iterator
  begin() noexcept
  {
    return stack;
  }

  iterator
  end() noexcept
  {
    return &stack[length];
  }

  const_iterator
  cbegin() const noexcept
  {
    return stack;
  }

  const_iterator
  cend() const noexcept
  {
    return &stack[length];
  }

  inline iterator
  get(const size_type n)
  {
    if ( n >= N )
      exc<except::library_error>("micron::array get() out of range");
    return micron::addr(stack + n);
  }

  inline const_iterator
  get(const size_type n) const
  {
    if ( n >= N )
      exc<except::library_error>("micron::array get() out of range");
    return micron::addr(stack + n);
  }

  inline const_iterator
  cget(const size_type n) const
  {
    if ( n >= N )
      exc<except::library_error>("micron::array get() out of range");
    return micron::addr(stack + n);
  }

  auto *
  addr()
  {
    return this;
  }

  const auto *
  addr() const
  {
    return this;
  }

  const_iterator
  data() const
  {
    return stack;
  }

  iterator
  data()
  {
    return stack;
  }

  const T &
  operator[](usize idx) const
  {
    if ( idx >= length )
      exc<except::library_error>("micron::bisect_array::erase(): out of range");
    return stack[idx];
  }

  void
  insert(const T &val)
  {
    if ( full() )
      exc<except::library_error>("micron::bisect_array::erase(): max capacity reached");
    usize idx = __bisect_right(val);
    for ( usize i = length; i > idx; --i )
      stack[i] = stack[i - 1];
    stack[idx] = val;
    ++length;
  }

  void
  erase(usize idx)
  {
    if ( idx >= length )
      exc<except::library_error>("micron::bisect_array::erase(): out of range");
    for ( usize i = idx; i + 1 < length; ++i )
      stack[i] = stack[i + 1];
    --length;
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};

};     // namespace micron
