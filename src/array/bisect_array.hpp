//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
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
template <typename T, size_t N> class bisect_array
{
  alignas(64) T stack[N];
  size_t length = 0;
  inline void
  __impl_zero(T *src)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(T());
    } else {
      micron::cmemset<N>(src, 0x0);
    }
  }
  void
  __impl_set(T *__restrict src, const T &val)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = val;
    } else {
      micron::cmemset<N>(src, val);
    }
  }
  void
  __impl_copy(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = dest[i];
    } else {
      micron::copy<N>(src, dest);
    }
  }
  void
  __impl_move(T *__restrict src, T *__restrict dest)
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i] = micron::move(dest[i]);
    } else {
      micron::copy<N>(src, dest);
      micron::czero<N>(src);
    }
  }

  size_t
  __bisect_left(const T &val) const noexcept
  {
    size_t low = 0, high = length;
    while ( low < high ) {
      size_t mid = low + (high - low) / 2;
      if ( stack[mid] < val )
        low = mid + 1;
      else
        high = mid;
    }
    return low;
  }

  size_t
  __bisect_right(const T &val) const noexcept
  {
    size_t low = 0, high = length;
    while ( low < high ) {
      size_t mid = low + (high - low) / 2;
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
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  ~bisect_array()
  {
    // explicit
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < N; i++ )
        stack[i].~T();
    } else {
      micron::czero<N>(micron::addr(stack[0]));
    }
  }
  bisect_array() { __impl_zero(micron::addr(stack[0])); }
  template <is_container A>
    requires(!micron::is_same_v<A, bisect_array>)
  bisect_array(const A &o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::bisect_array bisect_array(const&) invalid size");
    __impl_copy(micron::addr(o[0]), micron::addr(stack[0]));
  }
  template <is_container A>
    requires(!micron::is_same_v<A, bisect_array>)
  bisect_array(A &&o)
  {
    if ( o.size() < N )
      throw except::runtime_error("micron::bisect_array bisect_array(&&) invalid size");
    __impl_move(micron::addr(o[0]), micron::addr(stack[0]));
  }
  bisect_array(const bisect_array &o)
  {
    __impl_copy(micron::addr(o.stack[0]), micron::addr(stack[0]));
  }     // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]); }
  bisect_array(bisect_array &&o)
  {
    __impl_move(micron::addr(o.stack[0]), micron::addr(stack[0]));
    // micron::copy<N>(micron::addr(o.stack[0], micron::addr(stack[0]);
    // micron::cmemset<N>(micron::addr(stack[0], 0x0);
  }
  size_t
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
  operator[](size_t idx) const
  {
    if ( idx >= length )
      throw except::library_error("micron::bisect_array::erase(): out of range");
    return stack[idx];
  }

  void
  insert(const T &val)
  {
    if ( full() )
      throw except::library_error("micron::bisect_array::erase(): max capacity reached");
    size_t idx = __bisect_right(val);
    for ( size_t i = length; i > idx; --i )
      stack[i] = stack[i - 1];
    stack[idx] = val;
    ++length;
  }

  void
  erase(size_t idx)
  {
    if ( idx >= length )
      throw except::library_error("micron::bisect_array::erase(): out of range");
    for ( size_t i = idx; i + 1 < length; ++i )
      stack[i] = stack[i + 1];
    --length;
  }
  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};

};
