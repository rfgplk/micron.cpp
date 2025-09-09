//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "allocation/resources.hpp"
#include "allocator.hpp"

#include "algorithm/mem.hpp"
#include "memory/addr.hpp"
#include "tags.hpp"
#include "types.hpp"
#include "view.hpp"

// a slice is a view of contiguous memory
// can be instantiated on it's own (allocating new memory) or can be used to
// access memory of other containers (view-like)
// in case of container access, no new memory is allocated
// always moves, never copies

// this is intended to be used in 9/10 of cases where you need 'memory' (in the broadest sense). ((instead of resorting
// to c-style arrs, or std::array or std::vector)) .begin() .end() [] () bitwise >> << (memory streaming) <=>

namespace micron
{

template <typename T, class Alloc = micron::allocator_serial<>>
  requires micron::is_move_constructible_v<T>
struct slice : public __immutable_memory_resource<T, Alloc>
{
  using __mem = __immutable_memory_resource<T, Alloc>;
  using category_type = slice_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using safety_type = unsafe_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  ~slice()
  {
    if ( __mem::memory == nullptr )
      return;
    if ( __mem::length == 0 )
      return;     // this is only viewing memory
    __mem::free();
    //this->destroy(to_chunk(__mem::memory, __mem::capacity));
  }
  slice() : __mem(Alloc::auto_size())
  {
    __mem::length = __mem::capacity();
  };
  slice(T *a, T *b)
      : __mem(((static_cast<size_t>(b - a) / sizeof(T))))
  {
    micron::memcpy(&__mem::memory[0], a, b - a);
    __mem::length = b - a;
  };
  slice(const size_t n) : __mem(n)
  {
    __mem::length = __mem::capacity();
  };


  // take a view of memory of the underlying container
  // template <is_micron_structure Y>
  // slice(const Y &c) : __mem(*c)
  //{
  //}
  slice(const slice &) = delete;
  slice(slice &&o) : __mem(micron::move(o)) {}
  slice &operator=(const slice &) = delete;
  slice &
  operator=(slice &&o)
  {
    __mem::memory = o.memory;
    __mem::length = o.length;
    o.memory = 0;
    o.length = 0;
    return *this;
  }
  slice &
  set(const T n)
  {
    for ( size_t i = 0; i < __mem::length; i++ )
      __mem::memory[i] = n;
    return *this;
  }
  slice &
  operator=(const byte n)
  {
    micron::memset(&__mem::memory[0], n, __mem::length);
    return *this;
  }
  chunk<byte>
  operator*()
  {
    return __mem::data();
  }
  // overload this to always point to mem
  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(__mem::memory);
  }
  T &
  operator[](const size_t n)
  {
    return __mem::memory[n];
  }
  const T &
  operator[](const size_t n) const
  {
    return __mem::memory[n];
  }
  slice
  operator[](const size_t n, const size_t m) const
  {     // from-to
    return slice<T, Alloc>(&__mem::memory[n], &__mem::memory[m]);
  }
  slice
  operator[](void) const
  {     // from-to
    return slice<T, Alloc>(&__mem::memory[0], &__mem::memory[__mem::length]);
  }
  iterator
  begin() const
  {
    return &__mem::memory[0];
  }

  iterator
  end() const
  {
    return &__mem::memory[__mem::length - 1];
  }
  const_iterator
  cbegin() const
  {
    return &__mem::memory[0];
  }

  const_iterator
  cend() const
  {
    return &__mem::memory[__mem::length - 1];
  }
  size_t
  size() const
  {
    return __mem::length;
  }
  void
  reset()
  {
    micron::zero(__mem::memory, __mem::length);
    __mem::length = __mem::capacity();
  }
};
};     // namespace micron
