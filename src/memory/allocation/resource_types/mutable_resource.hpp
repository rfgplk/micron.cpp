//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__allocators.hpp"
#include "../core_resource.hpp"

namespace micron
{

template<typename T, typename Alloc> struct __owned_memory_resource: public __core_memory_resource<T> {
  using __core = __core_memory_resource<T>;
  using typename __core::size_type;

  size_type length;

private:
  [[nodiscard]] static chunk<byte>
  __allocation(T *memory, usize capacity) noexcept
  {
    if ( memory == nullptr ) return { nullptr, 0 };
    const usize usable_bytes = capacity * sizeof(T);
    return { reinterpret_cast<byte *>(memory), __allocator_extent<Alloc, alignof(T)>(usable_bytes) };
  }

  [[nodiscard]] static bool
  __extent_matches(chunk<byte> memory, usize required_bytes = 0) noexcept
  {
    if ( memory.ptr == nullptr ) return memory.len == 0 && required_bytes == 0;
    if ( memory.len < required_bytes || memory.len == 0 ) return false;
    if ( (reinterpret_cast<uintptr_t>(memory.ptr) & (alignof(T) - 1)) != 0 ) return false;
    const usize usable_bytes = (memory.len / sizeof(T)) * sizeof(T);
    return __allocator_extent<Alloc, alignof(T)>(usable_bytes) == memory.len;
  }

  [[nodiscard]] static chunk<byte>
  __create_checked(usize bytes)
  {
    chunk<byte> memory = __allocator_create<Alloc, alignof(T)>(bytes);
    if ( !__extent_matches(memory, bytes) ) {
      __allocator_destroy<Alloc, alignof(T)>(memory);
      exc<except::memory_error>("allocator: allocation extent cannot be reconstructed from element capacity");
    }
    return memory;
  }

  [[nodiscard]] static chunk<byte> &&
  __adopt_checked(chunk<byte> &memory)
  {
    if ( !__extent_matches(memory) )
      exc<except::memory_error>("allocator: adopted allocation extent cannot be reconstructed from element capacity");
    return micron::move(memory);
  }

  static void
  __destroy_range(T *memory, usize count) noexcept
  {
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = 0; i < count; ++i ) memory[i].~T();
    }
  }

  void
  __release_raw() noexcept
  {
    if ( __core::memory ) __allocator_destroy<Alloc, alignof(T)>(__allocation(__core::memory, __core::capacity));
    __core::memory = nullptr;
    __core::capacity = 0;
  }

  void
  __relocate_to(usize elements)
  {
    const usize bytes = allocation_multiply_or_throw(elements, sizeof(T));
    const usize retained = length < elements ? length : elements;

    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      chunk<byte> next = __allocator_resize_bytes<Alloc, alignof(T)>(__allocation(__core::memory, __core::capacity), bytes,
                                                                     allocation_multiply_or_throw(retained, sizeof(T)));
      __core::memory = reinterpret_cast<T *>(next.ptr);
      __core::capacity = next.len / sizeof(T);
      length = retained;
      if ( !__extent_matches(next, bytes) ) {
        __allocator_destroy<Alloc, alignof(T)>(next);
        __core::memory = nullptr;
        __core::capacity = 0;
        length = 0;
        exc<except::memory_error>("allocator: resized extent cannot be reconstructed from element capacity");
      }
      return;
    }

    chunk<byte> next = __create_checked(bytes);
    T *destination = reinterpret_cast<T *>(next.ptr);
    usize constructed = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( ; constructed < retained; ++constructed ) {
        if constexpr ( micron::is_nothrow_move_constructible_v<T> || !micron::is_copy_constructible_v<T> )
          micron::construct_at(destination + constructed, micron::move(__core::memory[constructed]));
        else
          micron::construct_at(destination + constructed, __core::memory[constructed]);
      }
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __destroy_range(destination, constructed);
      __allocator_destroy<Alloc, alignof(T)>(next);
      throw;
    }
#endif

    __destroy_range(__core::memory, length);
    __release_raw();
    __core::memory = destination;
    __core::capacity = next.len / sizeof(T);
    length = retained;
  }

public:
  ~__owned_memory_resource() { __release_raw(); }

  __owned_memory_resource(nullptr_t) noexcept : __core(), length(0) { }

  __owned_memory_resource() : __core(), length(0)
  {
    const usize minimum = Alloc::auto_size() < sizeof(T) ? sizeof(T) : Alloc::auto_size();
    const usize bytes = allocation_round_up_or_throw(minimum, sizeof(T));
    chunk<byte> memory = __create_checked(bytes);
    __core::accept(micron::move(memory));
  }

  explicit __owned_memory_resource(usize elements) : __core(__create_checked(allocation_multiply_or_throw(elements, sizeof(T)))), length(0)
  {
  }

  explicit __owned_memory_resource(chunk<byte> &&memory) : __core(__adopt_checked(memory)), length(0) { }

  __owned_memory_resource(const __owned_memory_resource &) = delete;
  __owned_memory_resource &operator=(const __owned_memory_resource &) = delete;

  void accept(const chunk<byte> &) = delete;

  void
  accept(chunk<byte> &&memory)
  {
    __core::accept(__adopt_checked(memory));
  }

  __owned_memory_resource(__owned_memory_resource &&other) noexcept : __core(micron::move(other)), length(other.length)
  {
    other.length = 0;
  }

  __owned_memory_resource &
  operator=(__owned_memory_resource &&other) noexcept
  {
    if ( this == micron::addressof(other) ) return *this;
    __destroy_range(__core::memory, length);
    __release_raw();
    __core::operator=(micron::move(other));
    length = other.length;
    other.length = 0;
    return *this;
  }

  __owned_memory_resource &
  swap(__owned_memory_resource &other) noexcept
  {
    micron::swap(__core::capacity, other.capacity);
    micron::swap(__core::memory, other.memory);
    micron::swap(length, other.length);
    return *this;
  }

  [[nodiscard]] bool
  is_zero() const noexcept
  {
    return !__core::alive();
  }

  [[nodiscard]] bool
  has_space(usize elements) const noexcept
  {
    return length <= __core::capacity && elements <= __core::capacity - length;
  }

  [[nodiscard]] chunk<byte>
  data() const noexcept
  {
    return __allocation(__core::memory, __core::capacity);
  }

  [[nodiscard]] chunk<byte>
  operator*() const noexcept
  {
    return data();
  }

  [[nodiscard]] chunk<byte>
  operator*() noexcept
  {
    return data();
  }

  void
  free() noexcept
  {
    __release_raw();
    length = 0;
  }

  void
  realloc(usize elements)
  {
    if ( elements == __core::capacity ) return;
    __relocate_to(elements);
  }

  void
  expand(usize elements)
  {
    if ( elements <= __core::capacity ) return;
    __relocate_to(elements);
  }

  [[nodiscard]] static usize
  recommended_capacity(usize current, usize minimum)
  {
    const usize current_bytes = allocation_multiply_or_throw(current, sizeof(T));
    const usize minimum_bytes = allocation_multiply_or_throw(minimum, sizeof(T));
    const usize recommended = __allocator_recommend<Alloc>(current_bytes, minimum_bytes);
    if ( recommended == __allocation_max ) exc<except::length_error>("allocator: growth recommendation overflow");
    return recommended / sizeof(T) + (recommended % sizeof(T) != 0);
  }
};

template<typename T, typename Alloc = allocator_serial<>>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T>
struct __mutable_memory_resource: public __owned_memory_resource<T, Alloc> {
  using __base = __owned_memory_resource<T, Alloc>;
  using __base::__base;
  __mutable_memory_resource(__mutable_memory_resource &&) noexcept = default;
  __mutable_memory_resource &operator=(__mutable_memory_resource &&) noexcept = default;
  __mutable_memory_resource(const __mutable_memory_resource &) = delete;
  __mutable_memory_resource &operator=(const __mutable_memory_resource &) = delete;
};

template<typename T, typename Alloc = allocator_serial<>>
  requires micron::is_move_constructible_v<T>
struct __mutable_memory_resource_move_only: public __owned_memory_resource<T, Alloc> {
  using __base = __owned_memory_resource<T, Alloc>;
  using __base::__base;
  __mutable_memory_resource_move_only(__mutable_memory_resource_move_only &&) noexcept = default;
  __mutable_memory_resource_move_only &operator=(__mutable_memory_resource_move_only &&) noexcept = default;
  __mutable_memory_resource_move_only(const __mutable_memory_resource_move_only &) = delete;
  __mutable_memory_resource_move_only &operator=(const __mutable_memory_resource_move_only &) = delete;
};

};      // namespace micron
