//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/atomic.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fixed-capacity static bump allocator
//
// use when a compile-time byte budget must serve allocations without heap or mmap traffic

namespace micron
{

template<class Tag, usize Bytes, usize Alignment = 64> class allocator_static
{
  static_assert(Bytes != 0, "allocator_static: Bytes must be non-zero");
  static_assert(allocation_is_power_of_two(Alignment), "allocator_static: Alignment must be a power of two");

  alignas(Alignment) inline static byte __storage[Bytes]{};
  inline static atomic_token<usize> __cursor{ 0 };

public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return Bytes < Alignment ? Bytes : Alignment;
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( alignment > Alignment ) exc<except::invalid_argument>("allocator_static: requested alignment exceeds arena alignment");
    if ( bytes == 0 ) return { nullptr, 0 };

    usize observed = __cursor.get(memory_order::relaxed);
    for ( ;; ) {
      usize begin;
      if ( !allocation_checked_round_up(observed, alignment, begin) || begin > Bytes || bytes > Bytes - begin )
        exc<except::memory_error>("allocator_static: arena exhausted");
      const usize end = begin + bytes;
      usize expected = observed;
      if ( __cursor.compare_exchange_weak(expected, end, memory_order::acq_rel, memory_order::relaxed) )
        return { __storage + begin, bytes };
      observed = expected;
    }
  }

  template<usize RequestedAlignment>
  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    static_assert(allocation_is_power_of_two(RequestedAlignment), "allocator_static: requested alignment must be a non-zero power of two");
    static_assert(RequestedAlignment <= Alignment, "allocator_static: requested alignment exceeds arena alignment");
    return create(bytes, RequestedAlignment);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create(bytes, Alignment);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    chunk<byte> next = create(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied != 0 ) micron::memcpy(next.ptr, old.ptr, copied);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator_static: growth overflow");
    return resize(old, target, old.len, Alignment);
  }

  static void
  destroy(chunk<byte>, usize) noexcept
  {
  }

  static void
  destroy(chunk<byte>) noexcept
  {
  }

  static void
  destroy(byte *, usize) noexcept
  {
  }

  template<usize>
  static void
  destroy(byte *) noexcept
  {
  }

  [[nodiscard]] static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    usize result;
    return allocation_checked_growth(current, minimum, 2, 1, result) ? result : __allocation_max;
  }

  [[nodiscard]] static constexpr f32
  get_grow() noexcept
  {
    return 2.0f;
  }

  static void
  reset() noexcept
  {
    __cursor.store(0, memory_order::release);
  }

  [[nodiscard]] static usize
  used() noexcept
  {
    return __cursor.get(memory_order::acquire);
  }

  byte *share(void) = delete;
};

};      // namespace micron
