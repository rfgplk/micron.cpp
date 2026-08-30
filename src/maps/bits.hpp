//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/new.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

namespace __maps
{

template<class Alloc, typename T> struct storage_allocator {
  static constexpr usize
  auto_size() noexcept
  {
    return Alloc::auto_size();
  }

  static chunk<byte>
  create(usize bytes)
  {
    return __allocator_create<Alloc, alignof(T)>(bytes);
  }

  static chunk<byte>
  create(usize bytes, usize alignment)
  {
    return __allocator_create<Alloc>(bytes, alignment < alignof(T) ? alignof(T) : alignment);
  }

  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize alignment)
  {
    return allocator_traits<Alloc>::allocation_extent(bytes, alignment < alignof(T) ? alignof(T) : alignment);
  }

  static chunk<byte>
  grow(chunk<byte> memory, usize bytes)
  {
    const usize target = recommend(memory.len, bytes);
    if ( target == __allocation_max ) exc<except::length_error>("maps: storage growth overflow");
    return resize(memory, target, memory.len, alignof(T));
  }

  static chunk<byte>
  resize(chunk<byte> memory, usize bytes, usize preserve_bytes, usize alignment)
  {
    return __allocator_resize_bytes<Alloc>(memory, bytes, preserve_bytes, alignment < alignof(T) ? alignof(T) : alignment);
  }

  static void
  destroy(const chunk<byte> memory) noexcept
  {
    __allocator_destroy<Alloc, alignof(T)>(memory);
  }

  static void
  destroy(const chunk<byte> memory, usize alignment) noexcept
  {
    __allocator_destroy<Alloc>(memory, alignment < alignof(T) ? alignof(T) : alignment);
  }

  static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    return __allocator_recommend<Alloc>(current, minimum);
  }

  static auto
  get_grow()
  {
    return Alloc::get_grow();
  }
};

};      // namespace __maps

};      // namespace micron
