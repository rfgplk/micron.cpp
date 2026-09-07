// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "config.hpp"
#include "malloc.hpp"
#include "metadata.hpp"

#include "../../../except.hpp"
#include "../../../memory/allocation/kmemory.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

namespace bb
{

template <typename T>
  requires(micron::is_integral_v<T>)
struct __bb_allocator {
  static auto
  calloc(usize n) -> micron::__chunk<byte>
  {
    auto mem = bb::zalloc(n);
    if( mem.ptr == nullptr )
      micron::exc<micron::except::critical_error>("bb_allocator::calloc(): pool failed to satisfy request");
    return mem;
  }

  static T *
  alloc(usize n)
  {
    T *ptr = reinterpret_cast<T *>(bb::alloc(n));
    if( ptr == nullptr )
      micron::exc<micron::except::critical_error>("bb_allocator::alloc(): pool failed to satisfy request");
    return ptr;
  }

  static auto
  allocate_aligned(usize n, usize alignment) -> micron::__chunk<byte>
  {
    auto mem = bb::aligned_balloc(alignment, n);
    if( mem.ptr == nullptr )
      micron::exc<micron::except::critical_error>("bb_allocator::allocate_aligned(): pool failed to satisfy request");
    return mem;
  }

  static void
  dealloc(T *mem, usize)
  {
    if( mem == nullptr )
      micron::exc<micron::except::critical_error>("bb_allocator::dealloc(): nullptr was provided");
    bb::dealloc(reinterpret_cast<byte *>(mem));
  }

  static void
  dealloc(T *mem)
  {
    if( mem == nullptr )
      return;
    bb::dealloc(reinterpret_cast<byte *>(mem));
  }

  static void
  dealloc_aligned(T *mem, usize alignment)
  {
    if( mem == nullptr )
      return;
    bb::aligned_dealloc(reinterpret_cast<byte *>(mem), alignment);
  }
};

class bb_allocator
{
public:
  static constexpr bool allocator_trusted = true;
  using chunk_t = micron::__chunk<byte>;

  static constexpr usize
  auto_size() noexcept
  {
    return 64;
  }

  static constexpr usize
  allocation_extent(usize bytes, usize alignment) noexcept
  {
    if( bytes == 0 )
      return 0;
    if( __bits::__usize_max - __rz < bytes )
      return __bits::__usize_max;
    const usize want = bytes + __rz;
    if( alignment <= native_alignment && want <= __class_small ) {
      const usize r = __bits::__round_up_sat(want, 16);
      return (r < 16 ? 16 : r) - __rz;
    }
    usize v = want < __default_min_block ? __default_min_block : want;
    if( v <= 1 )
      return alignment > 1 ? alignment : 1;
    i32 top = 0;
    usize t = v - 1;
    while( t > 0 ) {
      t >>= 1;
      ++top;
    }
    if( top >= static_cast<i32>(sizeof(usize) * 8) )
      return __bits::__usize_max;
    const usize p = static_cast<usize>(1) << top;
    return (p < alignment ? alignment : p) - __rz;
  }

  static constexpr usize
  allocation_extent(usize bytes) noexcept
  {
    return allocation_extent(bytes, native_alignment);
  }

  static chunk_t
  create(usize bytes, usize alignment)
  {
    if( bytes == 0 )
      return { nullptr, 0 };
    chunk_t c = alignment <= native_alignment ? bb::balloc(bytes) : bb::aligned_balloc(alignment, bytes);
    if( c.ptr == nullptr )
      micron::exc<micron::except::memory_error>("bb_allocator::create(): pool exhausted");
    return { c.ptr, allocation_extent(bytes, alignment) };
  }

  template <usize Alignment>
  static chunk_t
  create(usize bytes)
  {
    return create(bytes, Alignment);
  }

  static chunk_t
  create(usize bytes)
  {
    return create(bytes, native_alignment);
  }

  template <usize Alignment>
  static chunk_t
  resize(chunk_t old, usize bytes, usize preserve)
  {
    if( bytes == 0 ) {
      destroy(old, Alignment);
      return { nullptr, 0 };
    }
    chunk_t c = bb::resize(old, bytes, preserve, Alignment);
    if( c.ptr == nullptr )
      micron::exc<micron::except::memory_error>("bb_allocator::resize(): pool exhausted");
    return { c.ptr, allocation_extent(bytes, Alignment) };
  }

  static chunk_t
  resize(chunk_t old, usize bytes, usize preserve, usize alignment)
  {
    if( bytes == 0 ) {
      destroy(old, alignment);
      return { nullptr, 0 };
    }
    chunk_t c = bb::resize(old, bytes, preserve, alignment);
    if( c.ptr == nullptr )
      micron::exc<micron::except::memory_error>("bb_allocator::resize(): pool exhausted");
    return { c.ptr, allocation_extent(bytes, alignment) };
  }

  static void
  destroy(chunk_t memory, usize) noexcept
  {
    if( memory.ptr != nullptr )
      bb::dealloc(static_cast<void *>(memory.ptr));
  }

  static void
  destroy(chunk_t memory) noexcept
  {
    if( memory.ptr != nullptr )
      bb::dealloc(static_cast<void *>(memory.ptr));
  }

  static void
  destroy(byte *ptr, usize) noexcept
  {
    if( ptr != nullptr )
      bb::dealloc(static_cast<void *>(ptr));
  }

  template <usize Alignment>
  static void
  destroy(byte *ptr) noexcept
  {
    if( ptr != nullptr )
      bb::dealloc(static_cast<void *>(ptr));
  }

  static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    usize next = current > __bits::__usize_max / 2 ? __bits::__usize_max : current * 2;
    if( next < minimum )
      next = minimum;
    if( next < auto_size() )
      next = auto_size();
    return next;
  }

  static void
  reset() noexcept
  {
    bb::reset();
  }

  byte *share(void) = delete;
};

};
