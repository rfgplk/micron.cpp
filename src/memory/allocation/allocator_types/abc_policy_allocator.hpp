//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// abcmalloc backed policy allocator base
//
// base for the public serial, small, constrained, exact, and retiring allocators

namespace micron
{

template<is_policy P, usize DefaultAlignment>
class __abc_policy_allocator: private abc_allocator<byte>, public allocator_telemetry<__abc_policy_allocator<P, DefaultAlignment>>
{
  using __telemetry = allocator_telemetry<__abc_policy_allocator<P, DefaultAlignment>>;
  static_assert(allocation_is_power_of_two(DefaultAlignment), "allocator: default alignment must be a power of two");

  template<usize Alignment>
  [[gnu::always_inline]] static inline chunk<byte>
  __create_capacity(usize capacity)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator: alignment must be a non-zero power of two");
    if ( capacity == 0 ) return { nullptr, 0 };
#if defined(__micron_sanitizer_owns_heap)
    chunk<byte> memory = abc_allocator<byte>::allocate_aligned(capacity, Alignment);
#else
    chunk<byte> memory = Alignment <= abc::native_block_alignment ? abc::balloc(capacity) : abc::aligned_balloc(Alignment, capacity);
    if ( memory.ptr == nullptr ) [[unlikely]]
      exc<except::critical_error>("allocator: abcmalloc failed to satisfy request");
#endif
    return { memory.ptr, capacity };
  }

  static chunk<byte>
  __create_capacity(usize capacity, usize alignment)
  {
    if ( capacity == 0 ) return { nullptr, 0 };
    allocation_validate_alignment(alignment);
#if defined(__micron_sanitizer_owns_heap)
    chunk<byte> memory = abc_allocator<byte>::allocate_aligned(capacity, alignment);
#else
    chunk<byte> memory = alignment <= abc::native_block_alignment ? abc_allocator<byte>::allocate(capacity)
                                                                  : abc_allocator<byte>::allocate_aligned(capacity, alignment);
#endif
    return { memory.ptr, capacity };
  }

  template<usize Alignment>
  [[gnu::always_inline]] static inline void
  __destroy_ptr(byte *memory) noexcept
  {
    if ( memory == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    abc_allocator<byte>::dealloc_aligned(memory, Alignment);
#else
    if constexpr ( Alignment <= abc::native_block_alignment )
      abc::dealloc(memory);
    else
      abc::aligned_dealloc(memory, Alignment);
#endif
  }

  static void
  __destroy_ptr(byte *memory, usize alignment) noexcept
  {
    if ( memory == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    abc_allocator<byte>::dealloc_aligned(memory, alignment);
#else
    if ( alignment <= abc::native_block_alignment )
      abc_allocator<byte>::dealloc(memory);
    else
      abc_allocator<byte>::dealloc_aligned(memory, alignment);
#endif
  }

public:
  static constexpr bool allocator_trusted = true;

  ~__abc_policy_allocator() = default;
  __abc_policy_allocator() = default;
  __abc_policy_allocator(const __abc_policy_allocator &) = default;
  __abc_policy_allocator(__abc_policy_allocator &&) = default;
  __abc_policy_allocator &operator=(const __abc_policy_allocator &) = default;
  __abc_policy_allocator &operator=(__abc_policy_allocator &&) = default;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return P::minimum_bytes != 0 ? P::minimum_bytes : P::granularity;
  }

  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize)
  {
    return __allocation_policy_capacity<P>(bytes);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    chunk<byte> memory = __create_capacity(__allocation_policy_capacity<P>(bytes), alignment);
    __telemetry::__telemetry_allocate(bytes, memory.len);
    return memory;
  }

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] static inline chunk<byte>
  create(usize bytes)
  {
    chunk<byte> memory = __create_capacity<Alignment>(__allocation_policy_capacity<P>(bytes));
    __telemetry::__telemetry_allocate(bytes, memory.len);
    return memory;
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create<DefaultAlignment>(bytes);
  }

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] static inline chunk<byte>
  allocate(usize bytes)
  {
    return create<Alignment>(bytes);
  }

  [[nodiscard]] static chunk<byte>
  allocate(usize bytes, usize alignment)
  {
    return create(bytes, alignment);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    const usize capacity = __allocation_policy_capacity<P>(bytes);
    if ( old.ptr == nullptr ) {
      chunk<byte> next = __create_capacity(capacity, alignment);
      __telemetry::__telemetry_resize(bytes, 0, next.len, 0);
      return next;
    }
    if ( capacity == 0 ) {
      __destroy_ptr(old.ptr, alignment);
      __telemetry::__telemetry_resize(bytes, old.len, 0, 0);
      return { nullptr, 0 };
    }
#if !defined(__micron_sanitizer_owns_heap)
    if ( alignment <= abc::native_block_alignment ) {
      chunk<byte> next = abc::resize_chunk(old, capacity, micron::min(preserve_bytes, old.len));
      if ( next.ptr == nullptr ) exc<except::memory_error>("allocator: abcmalloc resize failed");
      __telemetry::__telemetry_resize(bytes, old.len, capacity, next.ptr == old.ptr ? 0 : micron::min(preserve_bytes, old.len, capacity));
      return { next.ptr, capacity };
    }
#endif
    chunk<byte> next = __create_capacity(capacity, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied != 0 ) micron::memcpy(next.ptr, old.ptr, copied);
    __destroy_ptr(old.ptr, alignment);
    __telemetry::__telemetry_resize(bytes, old.len, next.len, copied);
    return next;
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator: alignment must be a non-zero power of two");
    const usize capacity = __allocation_policy_capacity<P>(bytes);
    if ( old.ptr == nullptr ) {
      chunk<byte> next = __create_capacity<Alignment>(capacity);
      __telemetry::__telemetry_resize(bytes, 0, next.len, 0);
      return next;
    }
    if ( capacity == 0 ) {
      __destroy_ptr<Alignment>(old.ptr);
      __telemetry::__telemetry_resize(bytes, old.len, 0, 0);
      return { nullptr, 0 };
    }
#if !defined(__micron_sanitizer_owns_heap)
    if constexpr ( Alignment <= abc::native_block_alignment ) {
      chunk<byte> next = abc::resize_chunk(old, capacity, micron::min(preserve_bytes, old.len));
      if ( next.ptr == nullptr ) exc<except::memory_error>("allocator: abcmalloc resize failed");
      __telemetry::__telemetry_resize(bytes, old.len, capacity, next.ptr == old.ptr ? 0 : micron::min(preserve_bytes, old.len, capacity));
      return { next.ptr, capacity };
    }
#endif
    chunk<byte> next = __create_capacity<Alignment>(capacity);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied ) micron::memcpy(next.ptr, old.ptr, copied);
    __destroy_ptr<Alignment>(old.ptr);
    __telemetry::__telemetry_resize(bytes, old.len, next.len, copied);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator: growth overflow");
    return resize(old, target, old.len, DefaultAlignment);
  }

  static void
  destroy(chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) __telemetry::__telemetry_deallocate(memory.len);
    if ( memory.ptr == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    abc_allocator<byte>::dealloc_aligned(memory.ptr, alignment);
#else
    if ( alignment <= abc::native_block_alignment )
      abc::dealloc(memory.ptr, memory.len);
    else
      abc::aligned_dealloc(memory.ptr, alignment);
#endif
  }

  template<usize Alignment>
  [[gnu::always_inline]] static inline void
  destroy(chunk<byte> memory) noexcept
  {
    if ( memory.ptr ) __telemetry::__telemetry_deallocate(memory.len);
    if ( memory.ptr == nullptr ) return;
#if defined(__micron_sanitizer_owns_heap)
    abc_allocator<byte>::dealloc_aligned(memory.ptr, Alignment);
#else
    if constexpr ( Alignment <= abc::native_block_alignment )
      abc::dealloc(memory.ptr, memory.len);
    else
      abc::aligned_dealloc(memory.ptr, Alignment);
#endif
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy<DefaultAlignment>(memory);
  }

  static void
  destroy(byte *memory, usize alignment) noexcept
  {
    if ( memory ) __telemetry::__telemetry_deallocate(0);
    __destroy_ptr(memory, alignment);
  }

  template<usize Alignment>
  static void
  destroy(byte *memory) noexcept
  {
    if ( memory ) __telemetry::__telemetry_deallocate(0);
    __destroy_ptr<Alignment>(memory);
  }

  static void
  destroy(byte *memory) noexcept
  {
    destroy<DefaultAlignment>(memory);
  }

  static void
  deallocate(chunk<byte> memory, usize alignment) noexcept
  {
    destroy(memory, alignment);
  }

  template<usize Alignment>
  [[gnu::always_inline]] static inline void
  deallocate(chunk<byte> memory) noexcept
  {
    destroy<Alignment>(memory);
  }

  static void
  deallocate(byte *memory, usize alignment) noexcept
  {
    destroy(memory, alignment);
  }

  template<usize Alignment>
  static void
  deallocate(byte *memory) noexcept
  {
    destroy<Alignment>(memory);
  }

  [[nodiscard]] static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    return __allocation_policy_recommend<P>(current, minimum);
  }

  [[nodiscard]] static constexpr f32
  get_grow() noexcept
  {
    return P::on_grow;
  }

  byte *share(void) = delete;
};

};      // namespace micron
