//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/mem.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"

namespace micron
{

template <typename T, class Alloc = micron::allocator_serial<>>
class binary_heap : private Alloc, public immutable_memory<T>
{
  inline void
  make_heap(size_t j)
  {
    auto k = (j - 1) / 2;
    if ( j > 0 and immutable_memory<T>::memory[k] < immutable_memory<T>::memory[j] ) {
      micron::swap(immutable_memory<T>::memory[k], immutable_memory<T>::memory[j]);
      make_heap(k);
    }
  }

  inline void
  reduce_heap(size_t j)
  {
    int left = 2 * j + 1;
    int right = 2 * j + 2;
    int max = j;

    if ( left < immutable_memory<T>::length && immutable_memory<T>::memory[left] > immutable_memory<T>::memory[max] ) {
      max = left;
    }
    if ( right < immutable_memory<T>::length && immutable_memory<T>::memory[right] > immutable_memory<T>::memory[max] ) {
      max = right;
    }
    if ( max != j ) {
      micron::swap(immutable_memory<T>::memory[j], immutable_memory<T>::memory[max]);
      reduce_heap(max);
    }
  }

public:
  using category_type = theap_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef size_t size_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  ~binary_heap()
  {
    if ( contiguous_memory_no_copy<T>::memory == nullptr )
      return;
    clear();
    this->destroy(to_chunk(immutable_memory<T>::memory, immutable_memory<T>::capacity));
  }
  binary_heap(void)
      : immutable_memory<T>(this->create((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T))))
  {
  }
  template <T... Args> binary_heap(Args &&...args) : immutable_memory<T>(this->create(sizeof...(args) * sizeof(T)))
  {
    (insert(args), ...);
  }
  binary_heap(const size_t n) : immutable_memory<T>(this->create(n * sizeof(T))) {}
  binary_heap(const binary_heap &o) = delete;
  binary_heap(binary_heap &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
  }
  binary_heap &operator=(const binary_heap &o) = delete;
  binary_heap &
  operator=(binary_heap &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }
  binary_heap &
  insert(T &&v)
  {
    if ( immutable_memory<T>::length == immutable_memory<T>::capacity )
      return;
    if constexpr ( std::is_class_v<T> )
      immutable_memory<T>::memory[immutable_memory<T>::length] = micron::move(v);
    else
      immutable_memory<T>::memory[immutable_memory<T>::length] = v;
    make_heap(immutable_memory<T>::length++);
  }
  T
  get()
  {
    if ( !immutable_memory<T>::length )
      throw except::library_error("micron::binary_heap::get() is empty");
    if constexpr ( std::is_class_v<T> ) {
      T v = micron::move(immutable_memory<T>::memory[0]);
      immutable_memory<T>::memory[0] = micron::move(immutable_memory<T>::memory[immutable_memory<T>::length - 1]);
      immutable_memory<T>::length--;
      reduce_heap(0);
      return v;
    } else {
      T v = immutable_memory<T>::memory[0];
      immutable_memory<T>::memory[0] = (immutable_memory<T>::memory[immutable_memory<T>::length - 1]);
      immutable_memory<T>::length--;
      reduce_heap(0);
      return v;
    }
  }
  size_t
  size() const
  {
    return immutable_memory<T>::length;
  }
  size_t
  max_size() const
  {
    return immutable_memory<T>::capacity;
  }
  T
  max() const
  {
    if ( immutable_memory<T>::memory == nullptr )
      throw except::library_error("micron::binary_heap::max() is empty.");
    return immutable_memory<T>::memory[0];
  }
};
};
