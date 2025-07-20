//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "allocation/chunks.hpp"
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
  requires std::is_move_constructible_v<T>
class slice : private Alloc, public immutable_memory<T>
{
public:
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
    if ( immutable_memory<T>::memory == nullptr )
      return;
    if ( immutable_memory<T>::length == 0 )
      return;     // this is only viewing memory
    this->destroy(to_chunk(immutable_memory<T>::memory, immutable_memory<T>::capacity));
  }
  slice() : immutable_memory<T>(this->create(Alloc::auto_size()))
  {
    immutable_memory<T>::length = immutable_memory<T>::capacity;
  };
  template <typename F = T>
  slice(F *a, F *b)
      : immutable_memory<T>(this->create((static_cast<size_t>(b - a) * sizeof(F)) >= Alloc::auto_size()
                                             ? (static_cast<size_t>(b - a) * sizeof(F))
                                             : Alloc::auto_size()))
  {
    micron::memcpy(&immutable_memory<T>::memory[0], a, b - a);
    immutable_memory<T>::length = b - a;
  };
  slice(const size_t n) : immutable_memory<T>(this->create(n * sizeof(T)))
  {
    immutable_memory<T>::length = immutable_memory<T>::capacity;
  };


  // take a view of memory of the underlying container
  // template <is_micron_structure Y>
  // slice(const Y &c) : immutable_memory<T>(*c)
  //{
  //}
  slice(const slice &) = delete;
  slice(slice &&o) : immutable_memory<T>(micron::move(o)) {}
  slice &operator=(const slice &) = delete;
  slice &
  operator=(slice &&o)
  {
    immutable_memory<T>::memory = o.memory;
    immutable_memory<T>::length = o.length;
    immutable_memory<T>::capacity = o.capacity;
    o.memory = 0;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }
  slice &
  set(const T n)
  {
    for ( size_t i = 0; i < immutable_memory<T>::length; i++ )
      immutable_memory<T>::memory[i] = n;
    return *this;
  }
  slice &
  operator=(const byte n)
  {
    micron::memset(&immutable_memory<T>::memory[0], n, immutable_memory<T>::length);
    return *this;
  }
  chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(immutable_memory<T>::memory), immutable_memory<T>::capacity };
  }
  // overload this to always point to mem
  byte *
  operator&() volatile
  {
    return reinterpret_cast<byte *>(immutable_memory<T>::memory);
  }
  T &
  operator[](const size_t n)
  {
    return immutable_memory<T>::memory[n];
  }
  const T &
  operator[](const size_t n) const
  {
    return immutable_memory<T>::memory[n];
  }
  slice
  operator[](const size_t n, const size_t m) const
  {     // from-to
    return slice<T, Alloc>(&immutable_memory<T>::memory[n], &immutable_memory<T>::memory[m]);
  }
  slice
  operator[](void) const
  {     // from-to
    return slice<T, Alloc>(&immutable_memory<T>::memory[0], &immutable_memory<T>::memory[immutable_memory<T>::length]);
  }
  iterator
  begin() const
  {
    return &immutable_memory<T>::memory[0];
  }

  iterator
  end() const
  {
    return &immutable_memory<T>::memory[immutable_memory<T>::length - 1];
  }
  const_iterator
  cbegin() const
  {
    return &immutable_memory<T>::memory[0];
  }

  const_iterator
  cend() const
  {
    return &immutable_memory<T>::memory[immutable_memory<T>::length - 1];
  }
  size_t
  size() const
  {
    return immutable_memory<T>::length;
  }
  void
  reset()
  {
    micron::zero(immutable_memory<T>::memory, immutable_memory<T>::length);
    immutable_memory<T>::length = immutable_memory<T>::capacity;
  }
};
};     // namespace micron
