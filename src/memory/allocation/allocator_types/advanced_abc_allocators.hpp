//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// temporal, retiring, and persistent ABC allocation
//
// use allocator_temporal for explicitly aliased scratch storage;
// repeated launder calls return the same live address and must be paired
// with retire;
// use allocator_retiring when freed addresses must be tombstoned instead of immediately recycled;
// resize always allocates and retires the old block

namespace micron
{

class allocator_temporal
{
public:
  template<usize Alignment = 16>
  [[nodiscard]] static chunk<byte>
  launder(usize bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator_temporal: alignment must be a non-zero power of two");
    chunk<byte> memory = abc::aligned_launder(Alignment, bytes);
    if ( bytes != 0 && memory.ptr == nullptr ) exc<except::memory_error>("allocator_temporal: laundering failed");
    return memory;
  }

  [[nodiscard]] static chunk<byte>
  launder(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    chunk<byte> memory = abc::aligned_launder(alignment, bytes);
    if ( bytes != 0 && memory.ptr == nullptr ) exc<except::memory_error>("allocator_temporal: laundering failed");
    return memory;
  }

  template<usize Alignment = 16>
  static void
  retire(chunk<byte> memory)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator_temporal: alignment must be a non-zero power of two");
    abc::aligned_retire(memory, Alignment);
  }

  static void
  retire(chunk<byte> memory, usize alignment)
  {
    allocation_validate_alignment(alignment);
    abc::aligned_retire(memory, alignment);
  }
};

template<is_policy P = serial_allocation_policy> class allocator_retiring: public __abc_policy_allocator<P, 16>
{
  using base = __abc_policy_allocator<P, 16>;

  template<usize Alignment>
  static void
  __retire(byte *memory) noexcept
  {
    if ( memory == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    base::template destroy<Alignment>(memory);
#else
    abc::aligned_retire(memory, Alignment);
#endif
  }

  static void
  __retire(byte *memory, usize alignment) noexcept
  {
    if ( memory == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    base::destroy(memory, alignment);
#else
    abc::aligned_retire(memory, alignment);
#endif
  }

public:
  static constexpr bool allocator_trusted = true;

  using base::allocate;
  using base::auto_size;
  using base::create;
  using base::get_grow;
  using base::recommend;

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    chunk<byte> next = base::template create<Alignment>(bytes);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied ) micron::memcpy(next.ptr, old.ptr, copied);
    if ( old.ptr ) base::__telemetry_deallocate(old.len);
    __retire<Alignment>(old.ptr);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    chunk<byte> next = base::create(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied ) micron::memcpy(next.ptr, old.ptr, copied);
    if ( old.ptr ) base::__telemetry_deallocate(old.len);
    __retire(old.ptr, alignment);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator_retiring: growth overflow");
    return resize(old, target, old.len, 16);
  }

  template<usize Alignment>
  static void
  destroy(chunk<byte> memory) noexcept
  {
    if ( memory.ptr ) base::__telemetry_deallocate(memory.len);
    __retire<Alignment>(memory.ptr);
  }

  static void
  destroy(chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) base::__telemetry_deallocate(memory.len);
    __retire(memory.ptr, alignment);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    if ( memory.ptr ) base::__telemetry_deallocate(memory.len);
    __retire<16>(memory.ptr);
  }

  template<usize Alignment>
  static void
  destroy(byte *memory) noexcept
  {
    if ( memory ) base::__telemetry_deallocate(0);
    __retire<Alignment>(memory);
  }

  static void
  destroy(byte *memory, usize alignment) noexcept
  {
    if ( memory ) base::__telemetry_deallocate(0);
    __retire(memory, alignment);
  }

  template<usize Alignment>
  static void
  deallocate(chunk<byte> memory) noexcept
  {
    destroy<Alignment>(memory);
  }

  static void
  deallocate(chunk<byte> memory, usize alignment) noexcept
  {
    destroy(memory, alignment);
  }

  template<usize Alignment>
  static void
  deallocate(byte *memory) noexcept
  {
    destroy<Alignment>(memory);
  }

  static void
  deallocate(byte *memory, usize alignment) noexcept
  {
    destroy(memory, alignment);
  }
};

#if defined(MICRON_ABC_PERSISTENT)
template<is_policy P = serial_allocation_policy> class allocator_persistent: public allocator_retiring<P>
{
  static_assert(abc::__default_persistent_mode, "allocator_persistent requires a persistent abcmalloc configuration");
};
#endif

};      // namespace micron
