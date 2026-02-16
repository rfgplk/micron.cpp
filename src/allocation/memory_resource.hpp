//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"
#include "linux/kmemory.hpp"

#include "../memory/actions.hpp"

#include "../except.hpp"

namespace micron
{

// chunk<byte> represents a raw memory chunk as received from an allocator
// convert it into a __core_memory_resouce

enum class memory_flags { read, write, exec, none };

template <typename T> struct __core_memory_flags {
  memory_flags flags;
};

template <typename T>
  requires micron::is_copy_constructible_v<T> and micron::is_move_constructible_v<T>
struct __core_memory_resource {
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using value_type = T;
  using size_type = size_t;

  T *memory;     // capacity implied due to byte / sizeof(T)
  size_t capacity;

  ~__core_memory_resource() = default;
  __core_memory_resource(void) : memory(nullptr), capacity(0) {}
  __core_memory_resource(const __core_memory_resource &o) : memory(o.memory), capacity(o.capacity) {}
  __core_memory_resource(__core_memory_resource &&o) noexcept : memory(o.memory), capacity(o.capacity)
  {
    o.memory = nullptr;
    o.capacity = 0;
  };

  template <typename C> __core_memory_resource(__core_memory_resource<C> &&o) : memory(o.memory), capacity(o.capacity){};
  __core_memory_resource(chunk<byte> &&b)
  {
    auto addr = reinterpret_cast<uintptr_t>(b.ptr);
    if ( addr % alignof(T) != 0 )
      exc<except::memory_error>("__core_memory_resource, address isn't aligned");

    memory = reinterpret_cast<T *>(b.ptr);
    capacity = b.len / sizeof(T);
    b = nullptr;
  }
  __core_memory_resource &
  operator=(const __core_memory_resource &o)
  {
    memory = o.memory;
    capacity = o.capacity;
    return *this;
  }
  __core_memory_resource &
  operator=(__core_memory_resource &&o)
  {
    memory = o.memory;
    capacity = o.capacity;
    o.memory = nullptr;
    o.capacity = 0;
    return *this;
  };
  bool
  alive() const
  {
    return (memory != nullptr);
  }
  // converts a chunk to a memory_resource
  // this is important since a raw chunk is simply a mapping that the allocator returns
  void
  accept(const chunk<byte> &o)
  {
    auto addr = reinterpret_cast<uintptr_t>(o.ptr);
    if ( addr % alignof(T) != 0 )
      exc<except::memory_error>("__core_memory_resource, address isn't aligned");

    memory = reinterpret_cast<T *>(o.ptr);
    capacity = o.len / sizeof(T);
  }
  void
  accept(chunk<byte> &&o)
  {
    auto addr = reinterpret_cast<uintptr_t>(o.ptr);
    if ( addr % alignof(T) != 0 )
      exc<except::memory_error>("__core_memory_resource, address isn't aligned");

    memory = reinterpret_cast<T *>(o.ptr);
    capacity = o.len / sizeof(T);
    o = nullptr;
  }
  inline pointer
  cast()
  {
    return reinterpret_cast<pointer *>(memory);
  }
  inline const_pointer
  cast() const
  {
    return reinterpret_cast<const_pointer *>(memory);
  }
  inline reference
  ref()
  {
    if ( memory == nullptr )
      __builtin_exit(1);
    if ( capacity == 0 )
      __builtin_exit(1);
    return *cast();
  }
  inline const_reference
  ref() const
  {
    if ( memory == nullptr )
      __builtin_exit(1);
    if ( capacity == 0 )
      __builtin_exit(1);
    return *cast();
  }
  // converts a memory resource to a chunk

  inline const chunk<byte>
  operator*() const
  {
    return { reinterpret_cast<byte *>(memory), capacity * sizeof(T) };
  };
  inline chunk<byte>
  operator*()
  {
    return { reinterpret_cast<byte *>(memory), capacity * sizeof(T) };
  };
};
};     // namespace micron
