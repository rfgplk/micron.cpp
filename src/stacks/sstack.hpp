
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__container.hpp"

#include "algorithm/memory.hpp"
#include "allocation/resources.hpp"
#include "allocator.hpp"
#include "pointer.hpp"
#include "tags.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "__special/initializer_list"

namespace micron
{
template <typename T, size_t N = micron::alloc_auto_sz>
  requires micron::is_move_constructible_v<T>
class sstack
{
  T stack[N]{};
  size_t length = 0;

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

  constexpr sstack() = default;

  sstack(size_t n) : length(n)
  {
    if ( n > N )
      throw except::library_error("sstack size exceeds capacity");
    for ( size_t i = 0; i < n; ++i )
      push();
  }

  sstack(const std::initializer_list<T> &lst) : length(lst.size())
  {
    if ( lst.size() > N )
      throw except::library_error("initializer_list size exceeds capacity");
    size_t i = 0;
    for ( const T &v : lst )
      push(v);
  }

  sstack(const sstack &o) : length(o.length)
  {
    size_t n = micron::min(N, o.length);
    for ( size_t i = 0; i < n; ++i )
      push(o.stack[i]);
  }

  sstack(sstack &&o) noexcept : length(o.length)
  {
    size_t n = micron::min(N, o.length);
    for ( size_t i = 0; i < n; ++i )
      push(micron::move(o.stack[i]));
    o.clear();
  }

  sstack &
  operator=(const sstack &o)
  {
    clear();
    size_t n = micron::min(N, o.length);
    for ( size_t i = 0; i < n; ++i )
      push(o.stack[i]);
    return *this;
  }

  sstack &
  operator=(sstack &&o) noexcept
  {
    clear();
    size_t n = micron::min(N, o.length);
    for ( size_t i = 0; i < n; ++i )
      push(micron::move(o.stack[i]));
    o.clear();
    return *this;
  }

  reference
  operator[](size_t n)
  {
    if ( n >= length )
      throw except::library_error("sstack index out of bounds");
    return stack[length - n - 1];
  }

  const_reference
  operator[](size_t n) const
  {
    if ( n >= length )
      throw except::library_error("sstack index out of bounds");
    return stack[length - n - 1];
  }

  reference
  top()
  {
    if ( empty() )
      throw except::library_error("sstack top() called on empty stack");
    return stack[length - 1];
  }

  const_reference
  top() const
  {
    if ( empty() )
      throw except::library_error("sstack top() called on empty stack");
    return stack[length - 1];
  }

  T
  operator()()
  {
    T v = top();
    pop();
    return v;
  }

  template <typename... Args>
  void
  emplace(Args &&...args)
  {
    if ( length >= N )
      throw except::library_error("sstack overflow on emplace");
    new (&stack[length++]) T(micron::forward<Args>(args)...);
  }

  void
  push()
  {
    if ( length >= N )
      throw except::library_error("sstack overflow on push");
    new (&stack[length++]) T();
  }

  void
  push(const T &v)
  {
    if ( length >= N )
      throw except::library_error("sstack overflow on push");
    new (&stack[length++]) T(v);
  }

  void
  push(T &&v)
  {
    if ( length >= N )
      throw except::library_error("sstack overflow on push");
    new (&stack[length++]) T(micron::move(v));
  }

  void
  pop()
  {
    if ( empty() )
      throw except::library_error("pop() called on empty sstack");
    --length;
    if constexpr ( micron::is_class<T>::value )
      stack[length].~T();
  }

  void
  clear()
  {
    if constexpr ( micron::is_class<T>::value ) {
      for ( size_t i = 0; i < length; ++i )
        stack[i].~T();
    }
    length = 0;
  }

  void
  swap(sstack &o) noexcept
  {
    size_t min_len = micron::min(length, o.length);
    for ( size_t i = 0; i < min_len; ++i )
      micron::swap(stack[i], o.stack[i]);
    if ( length > o.length ) {
      for ( size_t i = min_len; i < length; ++i )
        o.push(micron::move(stack[i]));
      length = o.length;
    } else if ( o.length > length ) {
      for ( size_t i = min_len; i < o.length; ++i )
        push(micron::move(o.stack[i]));
      o.length = length;
    }
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
  size_t
  size() const
  {
    return length;
  }
  constexpr size_t
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

template <typename t, size_t N = micron::alloc_auto_sz>
  requires micron::is_move_constructible_v<t>
class fsstack
{
  t stack[N];
  size_t length;

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
  ~fsstack() = default;
  fsstack() : stack{}, length(0) {}
  fsstack(const umax_t n) : stack{}, length(0)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  fsstack(const std::initializer_list<t> &lst) : length(lst.size())
  {
    if ( lst.size() > N )
      throw except::library_error("micron::fsstack() initializer_list out of bounds");
    if constexpr ( micron::is_class_v<t> ) {
      size_t i = 0;
      for ( auto &&value : lst )
        new (&stack[i++]) t(value);
    } else {
      size_t i = 0;
      for ( t value : lst )
        stack[i++] = value;
    }
  }
  fsstack(const fsstack &o)
  {
    __impl_container::copy(&stack[0], &o.stack[0], N);
    length = o.length;
  }
  template <typename C = t, size_t M> fsstack(const fsstack<C, M> &o)
  {
    if constexpr ( N < M ) {
      __impl_container::copy(&stack[0], &o.stack[0], M);
    } else if constexpr ( M >= N ) {
      __impl_container::copy(&stack[0], &o.stack[0], N);
    }
    length = o.length;
  }
  fsstack(fsstack &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    length = o.length;
    o.length = 0;
  }
  template <typename C = t, size_t M> fsstack(fsstack<C, M> &&o)
  {
    if constexpr ( N >= M ) {
      micron::copy<N>(&o.stack[0], &stack[0]);
      length = o.length;
      o.length = 0;
    } else {
      micron::copy<M>(&o.stack[0], &stack[0]);
      length = o.length;
      o.length = 0;
    }
  };
  fsstack &
  operator=(const fsstack &o)
  {
    __impl_container::copy(&stack[0], &o.stack[0], N);
    length = o.length;
    return *this;
  }
  template <typename C = t, size_t M>
  fsstack &
  operator=(const fsstack<C, M> &o)
  {
    if constexpr ( N >= M ) {
      __impl_container::copy(stack, o.stack, N);
    } else {
      __impl_container::copy(stack, o.stack, M);
    }
    length = o.length;
    return *this;
  }
  fsstack &
  operator=(fsstack &&o)
  {
    micron::copy<N>(&o.stack[0], &stack[0]);
    length = o.length;
    o.length = 0;
    return *this;
  };
  inline t &
  operator[](const umax_t n)
  {
    umax_t c = length - n - 1;
    return stack[c];
  }
  inline const t &
  operator[](const umax_t n) const
  {
    umax_t c = length - n - 1;
    return stack[c];
  }
  t &
  top(void)
  {
    return stack[length - 1];
  }
  const t &
  top(void) const
  {
    return stack[length - 1];
  }
  // calling stack() is equivalent to top() and pop()
  inline t
  operator()(void)
  {
    auto n = stack[length - 1];
    pop();
    return n;
  }
  template <typename... Args>
  inline void
  emplace(Args &&...args)
  {
    new (&stack[length++]) t(micron::forward<Args>(args)...);
  }
  inline void
  push(void)
  {
    new (&stack[length++]) t();
  }
  inline void
  push(const t &v)
  {
    new (&stack[length++]) t(v);
  }
  inline void
  push(t &&v)
  {
    new (&stack[length++]) t(micron::move(v));
  }
  inline void
  pop()
  {
    --length;
    if constexpr ( micron::is_class<t>::value )
      stack[length].~t();
  }
  inline void
  clear()
  {
    if ( !length )
      return;
    if constexpr ( micron::is_class<t>::value ) {
      for ( size_t i = 0; i < length; i++ )
        (stack)[i].~t();
    }
    micron::zero((byte *)micron::voidify(&(stack)[0]), N * (sizeof(t) / sizeof(byte)));
    length = 0;
  }
  // grow container
  inline void reserve(const size_t) = delete;
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
  inline size_t
  size() const
  {
    return length;
  }
  inline size_t
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

};     // namespace micron
