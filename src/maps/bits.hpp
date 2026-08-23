//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../memory/new.hpp"
#include "../except.hpp"
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
    if constexpr ( alignof(T) <= 32 ) {
      return Alloc::create(bytes);
    } else {
      byte *ptr = static_cast<byte *>(::operator new(bytes, static_cast<std::align_val_t>(alignof(T))));
      return { ptr, bytes };
    }
  }

  static chunk<byte>
  grow(chunk<byte> memory, usize bytes)
  {
    if constexpr ( alignof(T) <= 32 ) {
      return Alloc::grow(memory, bytes);
    } else {
      chunk<byte> next = create(bytes);
      if constexpr ( micron::is_trivially_copyable_v<T> ) {
        const usize copied = memory.len < next.len ? memory.len : next.len;
        micron::memcpy(next.ptr, memory.ptr, copied);
      } else {
        destroy(next);
        exc<except::library_error>("maps: raw growth of over-aligned non-trivial storage is not supported");
      }
      destroy(memory);
      return next;
    }
  }

  static void
  destroy(const chunk<byte> memory) noexcept
  {
    if constexpr ( alignof(T) <= 32 )
      Alloc::destroy(memory);
    else if ( memory.ptr )
      ::operator delete(memory.ptr, static_cast<std::align_val_t>(alignof(T)));
  }

  static i16
  get_grow()
  {
    return Alloc::get_grow();
  }
};

};      // namespace __maps

};
