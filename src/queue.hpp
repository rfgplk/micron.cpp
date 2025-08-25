//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "algorithm/mem.hpp"
#include "allocation/chunks.hpp"
#include "allocator.hpp"
#include "types.hpp"
#include "type_traits.hpp"

namespace micron
{

template <typename T, size_t N = micron::alloc_auto_sz, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
class queue : private Alloc, public contiguous_memory_no_copy<T>
{
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
  queue() : contiguous_memory_no_copy<T>(this->create(N)) {}
  queue(const umax_t n) : contiguous_memory_no_copy<T>(this->create(n * sizeof(T)))
  {
    for ( umax_t i = 0; i < n; i++ )
      push();
  }
  queue(const queue &) = delete;
  queue(queue &&o) : contiguous_memory_no_copy<T>(micron::move(o)) {}
  queue &operator=(const queue &) = delete;
  ~queue()
  {
    if ( contiguous_memory_no_copy<T>::memory == nullptr )
      return;
    clear();
    this->destroy(to_chunk(contiguous_memory_no_copy<T>::memory, contiguous_memory_no_copy<T>::capacity));
  }
  inline void
  clear()
  {
    if ( !contiguous_memory_no_copy<T>::length )
      return;
    if constexpr ( micron::is_class_v<T> ) {
      for ( size_t i = 0; i < contiguous_memory_no_copy<T>::length; i++ )
        (contiguous_memory_no_copy<T>::memory)[i].~T();
    }
    micron::zero((byte *)micron::voidify(&(contiguous_memory_no_copy<T>::memory)[0]),
                 contiguous_memory_no_copy<T>::capacity * (sizeof(T) / sizeof(byte)));
    contiguous_memory_no_copy<T>::length = 0;
  }
  // grow container
  inline void
  reserve(const size_t n)
  {
    contiguous_memory_no_copy<T>::accept_new_memory(
        this->grow(reinterpret_cast<byte *>(contiguous_memory_no_copy<T>::memory),
                   contiguous_memory_no_copy<T>::capacity * sizeof(T), sizeof(T) * n));
  }
  inline void
  swap(queue &o)
  {
    micron::swap(contiguous_memory_no_copy<T>::memory);
    micron::swap(contiguous_memory_no_copy<T>::length);
    micron::swap(contiguous_memory_no_copy<T>::capacity);
  }
  inline bool
  empty() const
  {
    return contiguous_memory_no_copy<T>::length;
  }
  inline size_t
  size() const
  {
    return contiguous_memory_no_copy<T>::length;
  }
};
};     // namespace micron
