//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/mem.hpp"
#include "allocation/chunks.hpp"
#include "allocator.hpp"
#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{

template <typename T, size_t N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
class queue : public __mutable_memory_resource<T, Alloc>
{
  using __mem = __mutable_memory_resource<T, Alloc>;

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
  queue() : __mem(N) {}
  queue(const umax_t n) : __mem(n)
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  queue(const queue &) = delete;
  queue(queue &&o) : __mem(micron::move(o)) {}
  queue &operator=(const queue &) = delete;
  ~queue()
  {
    if ( __mem::memory == nullptr )
      return;
    clear();
    //this->destroy(to_chunk(__mem::memory, __mem::capacity));
  }
  inline void
  clear()
  {
    if ( !__mem::length )
      return;
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < __mem::length; i++ )
        (__mem::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __mem::capacity * (sizeof(T) / sizeof(byte)));
    __mem::length = 0;
  }
  // grow container
  inline void
  reserve(const size_t n)
  {
    __mem::expand(n);
    //__mem::accept_new_memory(
    //    this->grow(reinterpret_cast<byte *>(__mem::memory), __mem::capacity * sizeof(T), sizeof(T) * n));
  }
  inline void
  swap(queue &o)
  {
    micron::swap(__mem::memory);
    micron::swap(__mem::length);
    micron::swap(__mem::capacity);
  }
  inline bool
  empty() const
  {
    return __mem::length;
  }
  inline size_t
  size() const
  {
    return __mem::length;
  }
};
};     // namespace micron
