//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../allocator.hpp"
#include "../memory/mman.hpp"
#include "../memory/placement_new.hpp"
#include "../type_traits.hpp"

namespace micron
{

// reserved_vector
// table contiguous prefix of a bounded virtual address reservation
// growth commits pages and starts object lifetimes without moving existing objects
template<typename T> class reserved_vector
{
  T *__data = nullptr;
  usize __limit = 0, __mapped = 0, __committed = 0, __size = 0;

public:
  reserved_vector() noexcept = default;

  reserved_vector(const reserved_vector &other)
  {
    if ( !other.__data ) return;
    if ( !initialize(other.__limit) || !try_copy(other) ) {
      release();
      exc<except::memory_error>("reserved_vector: copy allocation failed");
    }
  }

  reserved_vector &
  operator=(const reserved_vector &other)
  {
    if ( !try_copy(other) ) exc<except::memory_error>("reserved_vector: copy allocation failed");
    return *this;
  }

  reserved_vector(reserved_vector &&other) noexcept
      : __data(other.__data), __limit(other.__limit), __mapped(other.__mapped), __committed(other.__committed), __size(other.__size)
  {
    other.__data = nullptr;
    other.__limit = other.__mapped = other.__committed = other.__size = 0;
  }

  reserved_vector &
  operator=(reserved_vector &&other) noexcept
  {
    if ( this == &other ) return *this;
    release();
    __data = other.__data;
    __limit = other.__limit;
    __mapped = other.__mapped;
    __committed = other.__committed;
    __size = other.__size;
    other.__data = nullptr;
    other.__limit = other.__mapped = other.__committed = other.__size = 0;
    return *this;
  }

  ~reserved_vector() { release(); }

  [[nodiscard]] bool
  initialize(usize maximum) noexcept
  {
    if ( __data ) return maximum == __limit;
    if ( !maximum || maximum > (~usize{ 0 } - (page_size - 1)) / sizeof(T) || alignof(T) > page_size ) return false;
    const usize bytes = (maximum * sizeof(T) + page_size - 1) & ~(page_size - 1);
    auto *memory = micron::mmap(nullptr, bytes, prot_none, map_private | map_anonymous, -1, 0);
    if ( micron::mmap_failed(memory) ) return false;
    __data = reinterpret_cast<T *>(memory);
    __limit = maximum;
    __mapped = bytes;
    return true;
  }

  [[nodiscard]] bool
  try_reserve(usize count) noexcept
  {
    if ( count > __limit ) return false;
    const usize bytes = (count * sizeof(T) + page_size - 1) & ~(page_size - 1);
    if ( bytes <= __committed ) return true;
    usize target = __committed ? __committed : page_size;
    while ( target < bytes ) target = target > __mapped / 2 ? __mapped : target * 2;
    auto *at = reinterpret_cast<addr_t *>(reinterpret_cast<byte *>(__data) + __committed);
    auto *memory = micron::mmap(at, target - __committed, prot_read | prot_write, map_private | map_anonymous | map_fixed, -1, 0);
    if ( micron::mmap_failed(memory) || memory != at ) return false;
    __committed = target;
    return true;
  }

  [[nodiscard]] bool
  try_resize(usize count) noexcept
  {
    if ( !try_reserve(count) ) return false;
    usize built = __size;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( ; built < count; ++built ) new (__data + built) T{};
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      if constexpr ( !is_trivially_destructible_v<T> )
        while ( built > __size ) __data[--built].~T();
      return false;
    }
#endif
    if constexpr ( !is_trivially_destructible_v<T> )
      for ( usize i = count; i < __size; ++i ) __data[i].~T();
    __size = count;
    return true;
  }

  // A failed element assignment leaves a valid, partially assigned prefix.
  [[nodiscard]] bool
  try_copy(const reserved_vector &other) noexcept
  {
    if ( this == &other ) return true;
    if ( !other.__data ) {
      clear();
      return true;
    }
    if ( !__data && !initialize(other.__limit) ) return false;
    if ( !try_resize(other.__size) ) return false;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( usize i = 0; i < __size; ++i ) __data[i] = other.__data[i];
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      return false;
    }
#endif
    return true;
  }

  void
  clear() noexcept
  {
    if constexpr ( !is_trivially_destructible_v<T> )
      for ( usize i = 0; i < __size; ++i ) __data[i].~T();
    __size = 0;
  }

  void
  release() noexcept
  {
    clear();
    if ( __data ) micron::munmap(reinterpret_cast<addr_t *>(__data), __mapped);
    __data = nullptr;
    __limit = __mapped = __committed = __size = 0;
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __size;
  }

  [[nodiscard]] usize
  capacity() const noexcept
  {
    return micron::min(__committed / sizeof(T), __limit);
  }

  [[nodiscard]] usize
  max_size() const noexcept
  {
    return __limit;
  }

  [[nodiscard]] T *
  data() noexcept
  {
    return __data;
  }

  [[nodiscard]] const T *
  data() const noexcept
  {
    return __data;
  }

  [[nodiscard]] T &
  operator[](usize i) noexcept
  {
    return __data[i];
  }

  [[nodiscard]] const T &
  operator[](usize i) const noexcept
  {
    return __data[i];
  }
};

};      // namespace micron
