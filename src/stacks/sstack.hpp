
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/resources.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"

namespace micron
{
// stack allocated fixed-capacity LIFO
template<typename T, usize N = micron::alloc_auto_sz>
  requires micron::is_move_constructible_v<T>
class sstack
{
  // anonymous union; no implicit lifetime
  union {
    T stack[N];
  };

  usize length = 0;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = T *;
  using const_iterator = const T *;

  ~sstack() { clear(); }

  sstack() noexcept : length(0) { }

  sstack(usize n) : length(0)
  {
    if ( n > N ) exc<except::library_error>("sstack size exceeds capacity");
    for ( usize i = 0; i < n; ++i ) push();
  }

  sstack(const std::initializer_list<T> &lst) : length(0)
  {
    if ( lst.size() > N ) exc<except::library_error>("initializer_list size exceeds capacity");
    for ( const T &v : lst ) push(v);
  }

  sstack(const sstack &o) : length(0)
  {
    for ( usize i = 0; i < o.length; ++i ) push(o.stack[i]);
  }

  sstack(sstack &&o) noexcept : length(0)
  {
    for ( usize i = 0; i < o.length; ++i ) push(micron::move(o.stack[i]));
    o.clear();
  }

  sstack &
  operator=(const sstack &o)
  {
    if ( this == micron::addr(o) ) return *this;
    clear();
    for ( usize i = 0; i < o.length; ++i ) push(o.stack[i]);
    return *this;
  }

  sstack &
  operator=(sstack &&o) noexcept
  {
    if ( this == micron::addr(o) ) return *this;
    clear();
    for ( usize i = 0; i < o.length; ++i ) push(micron::move(o.stack[i]));
    o.clear();
    return *this;
  }

  reference
  operator[](usize n)
  {
    if ( n >= length ) exc<except::library_error>("sstack index out of bounds");
    return stack[length - n - 1];
  }

  const_reference
  operator[](usize n) const
  {
    if ( n >= length ) exc<except::library_error>("sstack index out of bounds");
    return stack[length - n - 1];
  }

  reference
  top()
  {
    if ( empty() ) exc<except::library_error>("sstack top() called on empty stack");
    return stack[length - 1];
  }

  const_reference
  top() const
  {
    if ( empty() ) exc<except::library_error>("sstack top() called on empty stack");
    return stack[length - 1];
  }

  T
  operator()()
  {
    T v = micron::move(top());
    pop();
    return v;
  }

  template<typename... Args>
  void
  emplace(Args &&...args)
  {
    if ( length >= N ) exc<except::library_error>("sstack overflow on emplace");
    new (micron::addr(stack[length])) T(micron::forward<Args>(args)...);
    ++length;
  }

  void
  push()
  {
    if ( length >= N ) exc<except::library_error>("sstack overflow on push");
    new (micron::addr(stack[length])) T();
    ++length;
  }

  void
  push(const T &v)
  {
    if ( length >= N ) exc<except::library_error>("sstack overflow on push");
    new (micron::addr(stack[length])) T(v);
    ++length;
  }

  void
  push(T &&v)
  {
    if ( length >= N ) exc<except::library_error>("sstack overflow on push");
    new (micron::addr(stack[length])) T(micron::move(v));
    ++length;
  }

  void
  move(T &&v)
  {
    push(micron::move(v));
  }

  inline void
  pop()
  {
    if ( empty() ) exc<except::library_error>("sstack pop() called on empty stack");
    --length;
    if constexpr ( micron::is_class<T>::value ) stack[length].~T();
  }

  void
  clear()
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( usize i = 0; i < length; ++i ) stack[i].~T();
    }
    length = 0;
  }

  void
  swap(sstack &o) noexcept
  {
    usize common = micron::min(length, o.length);
    for ( usize i = 0; i < common; ++i ) {
      T tmp(micron::move(stack[i]));
      stack[i] = micron::move(o.stack[i]);
      o.stack[i] = micron::move(tmp);
    }
    if ( length > o.length ) {
      for ( usize i = common; i < length; ++i ) {
        new (micron::addr(o.stack[i])) T(micron::move(stack[i]));
        if constexpr ( micron::is_class<T>::value ) stack[i].~T();
      }
    } else if ( o.length > length ) {
      for ( usize i = common; i < o.length; ++i ) {
        new (micron::addr(stack[i])) T(micron::move(o.stack[i]));
        if constexpr ( micron::is_class<T>::value ) o.stack[i].~T();
      }
    }
    micron::swap(length, o.length);
  }

  bool
  empty() const
  {
    return !length;
  }

  inline bool
  full() const
  {
    return (length == N);
  }

  inline bool
  overflowed() const
  {
    return (length > N);
  }

  inline bool
  full_or_overflowed() const
  {
    return (length >= N);
  }

  usize
  size() const
  {
    return length;
  }

  constexpr usize
  max_size() const
  {
    return N;
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<T>;
  }
};

template<typename t, usize N = micron::alloc_auto_sz>
  requires micron::is_move_constructible_v<t>
class fsstack
{
  union {
    t stack[N];
  };

  usize length = 0;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef t value_type;
  typedef t &reference;
  typedef t &ref;
  typedef const t &const_reference;
  typedef const t &const_ref;
  typedef t *pointer;
  typedef const t *const_pointer;
  typedef t *iterator;
  typedef const t *const_iterator;

  ~fsstack() { clear(); }

  fsstack() noexcept : length(0) { }

  fsstack(const umax_t n) : length(0)
  {
    if ( n > N ) exc<except::library_error>("micron::fsstack() count exceeds capacity");
    for ( umax_t i = 0; i < n; i++ ) push();
  }

  fsstack(const std::initializer_list<t> &lst) : length(0)
  {
    if ( lst.size() > N ) exc<except::library_error>("micron::fsstack() initializer_list out of bounds");
    for ( const t &v : lst ) push(v);
  }

  fsstack(const fsstack &o) : length(0)
  {
    for ( usize i = 0; i < o.length; ++i ) push(o.stack[i]);
  }

  fsstack(fsstack &&o) noexcept : length(0)
  {
    for ( usize i = 0; i < o.length; ++i ) push(micron::move(o.stack[i]));
    o.clear();
  }

  fsstack &
  operator=(const fsstack &o)
  {
    if ( this == micron::addr(o) ) return *this;
    clear();
    for ( usize i = 0; i < o.length; ++i ) push(o.stack[i]);
    return *this;
  }

  fsstack &
  operator=(fsstack &&o) noexcept
  {
    if ( this == micron::addr(o) ) return *this;
    clear();
    for ( usize i = 0; i < o.length; ++i ) push(micron::move(o.stack[i]));
    o.clear();
    return *this;
  }

  inline t &
  operator[](const umax_t n) noexcept
  {
    return stack[length - n - 1];
  }

  inline const t &
  operator[](const umax_t n) const noexcept
  {
    return stack[length - n - 1];
  }

  inline t &
  top(void) noexcept
  {
    return stack[length - 1];
  }

  inline const t &
  top(void) const noexcept
  {
    return stack[length - 1];
  }

  // calling operator() is equivalent to top() then pop()
  inline t
  operator()(void)
  {
    t n = micron::move(stack[length - 1]);
    pop();
    return n;
  }

  template<typename... Args>
  inline void
  emplace(Args &&...args)
  {
    new (micron::addr(stack[length])) t(micron::forward<Args>(args)...);
    ++length;
  }

  inline void
  push(void)
  {
    new (micron::addr(stack[length])) t();
    ++length;
  }

  inline void
  push(const t &v)
  {
    new (micron::addr(stack[length])) t(v);
    ++length;
  }

  inline void
  push(t &&v)
  {
    new (micron::addr(stack[length])) t(micron::move(v));
    ++length;
  }

  inline void
  move(t &&v)
  {
    push(micron::move(v));
  }

  inline void
  pop()
  {
    --length;
    if constexpr ( micron::is_class<t>::value ) stack[length].~t();
  }

  inline void
  clear()
  {
    if constexpr ( micron::is_class<t>::value ) {
      for ( usize i = 0; i < length; ++i ) stack[i].~t();
    }
    length = 0;
  }

  inline void reserve(const usize) = delete;
  inline void swap(fsstack &o) = delete;

  inline bool
  empty() const
  {
    return !length;
  }

  inline bool
  full() const
  {
    return (length == N);
  }

  inline bool
  overflowed() const
  {
    return (length > N);
  }

  inline bool
  full_or_overflowed() const
  {
    return (length >= N);
  }

  inline usize
  size() const
  {
    return length;
  }

  inline usize
  max_size() const
  {
    return N;
  }

  static constexpr bool
  is_pod()
  {
    return micron::is_pod_v<t>;
  }
};

};      // namespace micron
