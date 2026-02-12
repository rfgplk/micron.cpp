//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocation/chunks.hpp"
#include "../allocator.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"

namespace micron
{

template <typename T, class Alloc = micron::allocator_serial<>>
class binary_heap : public __immutable_memory_resource<T>
{
  using __mem = __immutable_memory_resource<T, Alloc>;
  inline void
  make_heap(size_t j)
  {
    auto k = (j - 1) / 2;
    if ( j > 0 and __mem::memory[k] < __mem::memory[j] ) {
      micron::swap(__mem::memory[k], __mem::memory[j]);
      make_heap(k);
    }
  }

  inline void
  reduce_heap(size_t j)
  {
    int left = 2 * j + 1;
    int right = 2 * j + 2;
    int max = j;

    if ( left < __mem::length && __mem::memory[left] > __mem::memory[max] ) {
      max = left;
    }
    if ( right < __mem::length && __mem::memory[right] > __mem::memory[max] ) {
      max = right;
    }
    if ( max != j ) {
      micron::swap(__mem::memory[j], __mem::memory[max]);
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
    if ( __mem::memory == nullptr )
      return;
    clear();
  }
  binary_heap(void)
      : __mem((Alloc::auto_size() >= sizeof(T) ? Alloc::auto_size() : sizeof(T)))
  {
  }
  template <T... Args> binary_heap(Args &&...args) : __mem((sizeof...(args) * sizeof(T)))
  {
    (insert(args), ...);
  }
  binary_heap(const size_t n) : __mem(n * sizeof(T)) {}
  binary_heap(const binary_heap &o) = delete;
  binary_heap(binary_heap &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
  }
  binary_heap &operator=(const binary_heap &o) = delete;
  binary_heap &
  operator=(binary_heap &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }
  binary_heap &
  insert(T &&v)
  {
    if ( __mem::length == __mem::capacity )
      return;
    if constexpr ( micron::is_class_v<T> )
      __mem::memory[__mem::length] = micron::move(v);
    else
      __mem::memory[__mem::length] = v;
    make_heap(__mem::length++);
  }
  T
  get()
  {
    if ( !__mem::length )
      exc<except::library_error>("micron::binary_heap::get() is empty");
    if constexpr ( micron::is_class_v<T> ) {
      T v = micron::move(__mem::memory[0]);
      __mem::memory[0] = micron::move(__mem::memory[__mem::length - 1]);
      __mem::length--;
      reduce_heap(0);
      return v;
    } else {
      T v = __mem::memory[0];
      __mem::memory[0] = (__mem::memory[__mem::length - 1]);
      __mem::length--;
      reduce_heap(0);
      return v;
    }
  }
  size_t
  size() const
  {
    return __mem::length;
  }
  size_t
  max_size() const
  {
    return __mem::capacity;
  }
  T
  max() const
  {
    if ( __mem::memory == nullptr )
      exc<except::library_error>("micron::binary_heap::max() is empty.");
    return __mem::memory[0];
  }
};
};
