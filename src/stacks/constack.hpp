//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__container.hpp"
#include "../memory/new.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/resources.hpp"
#include "../pointer.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../__special/initializer_list"
#include "../concepts.hpp"

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

// stack
template <is_regular_object T, usize N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
class stack : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;
  micron::mutex __mtx;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~stack()
  {
    if ( __mem::is_zero() )
      return;
    clear();
  }

  stack() : __mem(N) {}

  explicit stack(const umax_t n) : __mem(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }

  stack(const std::initializer_list<T> &lst) : __mem(lst.size())
  {
    if constexpr ( micron::is_class_v<T> ) {
      usize i = 0;
      for ( const T &value : lst )
        new (&__mem::memory[i++]) T(micron::move(const_cast<T &>(value)));
      __mem::length = lst.size();
    } else {
      usize i = 0;
      for ( T value : lst )
        __mem::memory[i++] = value;
      __mem::length = lst.size();
    }
  }

  stack(const stack &o) : __mem(o.length)
  {
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
  }

  stack(stack &&o) : __mem(micron::move(o)) {}

  stack &
  operator=(const stack &o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( o.length >= __mem::capacity )
      reserve(o.length);
    __impl_container::copy(__mem::memory, o.memory, o.length);
    __mem::length = o.length;
    return *this;
  }

  stack &
  operator=(stack &&o)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::memory )
      __mem::free();
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }

  inline T &
  operator[](const umax_t n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    umax_t c = __mem::length - n - 1;
    if ( c >= __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::stack operator[] out of range");
    return __mem::memory[c];
  }

  inline const T &
  operator[](const umax_t n) const
  {
    umax_t c = __mem::length - n - 1;
    if ( c >= __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::stack operator[] out of range");
    return __mem::memory[c];
  }

  T &
  top()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::stack top() called on empty stack");
    return __mem::memory[__mem::length - 1];
  }

  const T &
  top() const
  {
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::stack top() called on empty stack");
    return __mem::memory[__mem::length - 1];
  }

  inline T
  operator()()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::stack operator()() called on empty stack");
    T n = micron::move(__mem::memory[__mem::length - 1]);
    pop();
    return n;
  }

  inline void
  push()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) T{};
  }

  inline void
  push(const T &v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    if constexpr ( micron::is_class_v<T> || !micron::is_trivially_constructible_v<T> )
      new (&__mem::memory[__mem::length++]) T(v);
    else
      __mem::memory[__mem::length++] = v;
  }

  inline void
  push(T &&v)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) T(micron::move(v));
  }

  inline void
  move(T &&v)
  {
    push(micron::move(v));
  }

  template <typename... Args>
  inline void
  push_range(Args &&...args)
  {
    (push(micron::forward<Args>(args)), ...);
  }

  template <typename... Args>
    requires(micron::is_lvalue_reference_v<Args> && ...)
  inline void
  emplace(Args &&...args)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length >= __mem::capacity )
      reserve(__mem::capacity * 2);
    new (&__mem::memory[__mem::length++]) T(micron::forward<Args>(args)...);
  }

  inline T
  pop()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( __mem::length == 0 ) [[unlikely]]
      exc<except::library_error>("micron::stack pop() called on empty stack");
    T val = micron::move(__mem::memory[__mem::length - 1]);
    if constexpr ( micron::is_class_v<T> )
      __mem::memory[__mem::length - 1].~T();
    czero<sizeof(T) / sizeof(byte)>((byte *)micron::voidify(&__mem::memory[__mem::length-- - 1]));
    return val;
  }

  template <typename... Args>
  inline void
  pop_range(Args &...args)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    static_assert((micron::is_same_v<micron::remove_cvref_t<Args>, T> && ...),
                  "micron::stack pop_range(): all output types must match value_type");
    if ( sizeof...(Args) > __mem::length ) [[unlikely]]
      exc<except::library_error>("micron::stack pop_range(): not enough elements");
    ((args = pop()), ...);
  }

  inline void
  reserve(const usize n)
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( n < __mem::capacity )
      return;
    __mem::expand(n);
  }

  inline void
  clear()
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    if ( !__mem::length )
      return;
    __impl_container::destroy(micron::addr(__mem::memory[0]), __mem::length);
    __mem::length = 0;
  }

  inline void
  swap(stack &o) noexcept
  {
    micron::unique_lock<micron::lock_starts::locked> __lock(__mtx);
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  [[nodiscard]] inline bool
  empty() const noexcept
  {
    return __mem::length == 0;
  }

  [[nodiscard]] inline usize
  size() const noexcept
  {
    return __mem::length;
  }

  [[nodiscard]] inline usize
  max_size() const noexcept
  {
    return __mem::capacity;
  }

  inline pointer
  data() noexcept
  {
    return __mem::memory;
  }

  inline const_pointer
  data() const noexcept
  {
    return __mem::memory;
  }

  byte *
  operator&() noexcept
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }

  const byte *
  operator&() const noexcept
  {
    return reinterpret_cast<const byte *>(__mem::memory);
  }

  auto *
  addr() noexcept
  {
    return this;
  }

  const auto *
  addr() const noexcept
  {
    return this;
  }

  static constexpr bool
  is_pod() noexcept
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class_type() noexcept
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }

  inline iterator
  begin() noexcept
  {
    return __mem::memory;
  }

  inline const_iterator
  begin() const noexcept
  {
    return __mem::memory;
  }

  inline const_iterator
  cbegin() const noexcept
  {
    return __mem::memory;
  }

  inline iterator
  end() noexcept
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  end() const noexcept
  {
    return __mem::memory + __mem::length;
  }

  inline const_iterator
  cend() const noexcept
  {
    return __mem::memory + __mem::length;
  }

  bool
  operator==(const stack &o) const noexcept
  {
    if ( __mem::length != o.length )
      return false;
    for ( usize i = 0; i < __mem::length; ++i )
      if ( __mem::memory[i] != o.memory[i] )
        return false;
    return true;
  }

  bool
  operator!=(const stack &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator<(const stack &o) const noexcept
  {
    usize n = __mem::length < o.length ? __mem::length : o.length;
    for ( usize i = 0; i < n; ++i ) {
      if ( __mem::memory[i] < o.memory[i] )
        return true;
      if ( o.memory[i] < __mem::memory[i] )
        return false;
    }
    return __mem::length < o.length;
  }

  bool
  operator>(const stack &o) const noexcept
  {
    return o < *this;
  }

  bool
  operator<=(const stack &o) const noexcept
  {
    return !(o < *this);
  }

  bool
  operator>=(const stack &o) const noexcept
  {
    return !(*this < o);
  }
};

}     // namespace micron
