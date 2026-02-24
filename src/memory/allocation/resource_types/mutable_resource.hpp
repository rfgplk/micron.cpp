//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../core_resource.hpp"

#include "../__allocators.hpp"

// NOTE: these functions should handle all of the allocating/deallocating logic

namespace micron
{
// universal mutable memory object, allows copying & moving.
template <typename T, typename Alloc = allocator_serial<>>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T>
struct __mutable_memory_resource : public __core_memory_resource<T> {
  typename __core_memory_resource<T>::size_type length;

  ~__mutable_memory_resource()
  {
    if ( __core_memory_resource<T>::alive() ) {
      Alloc::destroy(__core_memory_resource<T>::operator*());
      length = 0;
      __core_memory_resource<T>::capacity = 0;
      __core_memory_resource<T>::memory = nullptr;
    }
  }

  __mutable_memory_resource(void)
      : __core_memory_resource<T>(Alloc::create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T)))), length(0)
  {
  }

  __mutable_memory_resource(size_t n_elements)
      : __core_memory_resource<T>(Alloc::create(n_elements * (sizeof(T) / sizeof(byte)))), length(0) {};

  __mutable_memory_resource(const __mutable_memory_resource &o) : __core_memory_resource<T>(o), length(o.length) {}

  __mutable_memory_resource(__mutable_memory_resource &&o) : __core_memory_resource<T>(micron::move(o)), length(o.length) { o.length = 0; }

  __mutable_memory_resource(__chunk<byte> &&o) : __core_memory_resource<T>(micron::move(o)), length(0) { o = nullptr; }

  __mutable_memory_resource &
  operator=(const __mutable_memory_resource &o)
  {
    __core_memory_resource<T>::operator=(o);
    length = o.length;
    return *this;
  }

  __mutable_memory_resource &
  operator=(__mutable_memory_resource &&o)
  {
    __core_memory_resource<T>::operator=(micron::move(o));
    length = o.length;
    o.length = 0;
    return *this;
  }

  __mutable_memory_resource &
  swap(__mutable_memory_resource &o) noexcept
  {
    micron::swap(__core_memory_resource<T>::capacity, o.capacity);
    micron::swap(__core_memory_resource<T>::memory, o.memory);
    micron::swap(length, o.length);
    return *this;
  }

  inline bool
  is_zero() const
  {
    return !__core_memory_resource<T>::alive();
  }

  // start funcs
  inline bool
  has_space(const size_t n_el) const
  {
    return (((n_el * (sizeof(T) / sizeof(byte))) + length) <= (__core_memory_resource<T>::capacity));
  }

  inline chunk<byte>
  data() const
  {
    return __core_memory_resource<T>::operator*();
  }

  void
  free(void)
  {
    if ( __core_memory_resource<T>::alive() ) {
      Alloc::destroy(__core_memory_resource<T>::operator*());
      __core_memory_resource<T>::memory = nullptr;
      __core_memory_resource<T>::capacity = 0;
      length = 0;
    }
  }

  // deletes and reallocs, number of elements
  void
  realloc(size_t len)
  {
    if ( len == 0 ) [[unlikely]]
      return;
    if ( __core_memory_resource<T>::alive() )
      Alloc::destroy(__core_memory_resource<T>::operator*());
    __core_memory_resource<T>::accept(Alloc::create(len * (sizeof(T) / sizeof(byte))));
  }

  // expands memory, size_t is number of elements
  void
  expand(size_t len)
  {
    if ( len == 0 ) [[unlikely]]
      return;
    // NOTE: grow destroys memory
    __core_memory_resource<T>::accept(Alloc::grow(__core_memory_resource<T>::operator*(), len * (sizeof(T) / sizeof(byte))));
  }
};
};     // namespace micron
