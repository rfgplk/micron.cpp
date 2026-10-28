//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../allocator.hpp"
#include "../memory/placement_new.hpp"
#include "vector.hpp"

namespace micron
{

// segmented_vector
// indexed stable objects
template<typename T, usize ChunkSize = 256, class Resource = arena_resource<>>
  requires(ChunkSize != 0 && !(ChunkSize & (ChunkSize - 1)))
class segmented_vector
{
  Resource __arena;
  vector<T *> __chunks;
  usize __size = 0;

public:
  segmented_vector() = default;
  segmented_vector(const segmented_vector &) = delete;
  segmented_vector &operator=(const segmented_vector &) = delete;

  ~segmented_vector() { clear(); }

  [[nodiscard]] bool
  try_resize(usize count) noexcept
  {
    if ( count > ~usize{ 0 } - (ChunkSize - 1) ) return false;
    const usize needed = (count + ChunkSize - 1) / ChunkSize;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      __chunks.reserve(needed);
      while ( __chunks.size() < needed ) {
        T *chunk = __arena.template allocate_objects<T>(ChunkSize);
        __chunks.push_back(chunk);
      }
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      return false;
    }
#endif
    usize built = __size;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( ; built < count; ++built ) new (&(*this)[built]) T{};
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      if constexpr ( !is_trivially_destructible_v<T> )
        while ( built > __size ) (*this)[--built].~T();
      return false;
    }
#endif
    if constexpr ( !is_trivially_destructible_v<T> )
      for ( usize i = count; i < __size; ++i ) (*this)[i].~T();
    __size = count;
    return true;
  }

  void
  clear() noexcept
  {
    if constexpr ( !is_trivially_destructible_v<T> )
      for ( usize i = 0; i < __size; ++i ) (*this)[i].~T();
    __size = 0;
  }

  void
  release() noexcept
  {
    clear();
    __chunks.clear();
    __arena.release();
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __size;
  }

  [[nodiscard]] usize
  capacity() const noexcept
  {
    return __chunks.size() * ChunkSize;
  }

  [[nodiscard]] T &
  operator[](usize i) noexcept
  {
    return __chunks.data()[i / ChunkSize][i & (ChunkSize - 1)];
  }

  [[nodiscard]] const T &
  operator[](usize i) const noexcept
  {
    return __chunks.data()[i / ChunkSize][i & (ChunkSize - 1)];
  }
};

}      // namespace micron
