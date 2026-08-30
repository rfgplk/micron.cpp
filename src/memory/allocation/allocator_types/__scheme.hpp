//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/atomic.hpp"
#include "../../../except.hpp"
#include "../../../memory/cmemory.hpp"
#include "../../../memory/placement_new.hpp"
#include "../../../numerics.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "../kmemory.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator protocol and policy machinery
//
// internal support for allocator implementations;
// include the allocator umbrella instead of using this header directly

namespace micron
{

inline constexpr usize __allocation_max = micron::numeric_limits<usize>::max();

[[nodiscard]] constexpr bool
allocation_is_power_of_two(usize n) noexcept
{
  return n != 0 && (n & (n - 1)) == 0;
}

[[nodiscard]] constexpr bool
allocation_checked_add(usize a, usize b, usize &result) noexcept
{
  if ( b > __allocation_max - a ) return false;
  result = a + b;
  return true;
}

[[nodiscard]] constexpr bool
allocation_checked_multiply(usize a, usize b, usize &result) noexcept
{
  if ( a != 0 && b > __allocation_max / a ) return false;
  result = a * b;
  return true;
}

[[nodiscard]] constexpr bool
allocation_checked_round_up(usize value, usize granularity, usize &result) noexcept
{
  if ( granularity == 0 ) return false;
  const usize remainder = value % granularity;
  if ( remainder == 0 ) {
    result = value;
    return true;
  }
  return allocation_checked_add(value, granularity - remainder, result);
}

[[nodiscard]] constexpr bool
allocation_checked_growth(usize current, usize minimum, usize numerator, usize denominator, usize &result) noexcept
{
  if ( denominator == 0 || numerator < denominator ) return false;
  const usize whole = numerator / denominator;
  const usize fractional = numerator % denominator;
  usize grown;
  if ( !allocation_checked_multiply(current, whole, grown) ) return false;

  if ( fractional != 0 ) {
    const usize quotient = current / denominator;
    const usize remainder = current % denominator;
    usize fractional_whole;
    usize fractional_tail;
    if ( !allocation_checked_multiply(quotient, fractional, fractional_whole)
         || !allocation_checked_multiply(remainder, fractional, fractional_tail) )
      return false;
    usize fractional_result;
    if ( !allocation_checked_add(fractional_whole, fractional_tail / denominator, fractional_result) ) return false;
    if ( fractional_tail % denominator != 0 ) {
      if ( fractional_result == __allocation_max ) return false;
      ++fractional_result;
    }
    if ( !allocation_checked_add(grown, fractional_result, grown) ) return false;
  }
  result = grown < minimum ? minimum : grown;
  return true;
}

[[nodiscard]] inline usize
allocation_add_or_throw(usize a, usize b)
{
  usize result;
  if ( !allocation_checked_add(a, b, result) ) exc<except::length_error>("allocator: size addition overflow");
  return result;
}

[[nodiscard]] inline usize
allocation_multiply_or_throw(usize a, usize b)
{
  usize result;
  if ( !allocation_checked_multiply(a, b, result) ) exc<except::length_error>("allocator: element count overflow");
  return result;
}

[[nodiscard]] inline usize
allocation_round_up_or_throw(usize value, usize granularity)
{
  usize result;
  if ( !allocation_checked_round_up(value, granularity, result) )
    exc<except::length_error>("allocator: invalid granularity or rounded size overflow");
  return result;
}

inline void
allocation_validate_alignment(usize alignment)
{
  if ( !allocation_is_power_of_two(alignment) ) exc<except::invalid_argument>("allocator: alignment must be a non-zero power of two");
}

template<is_policy P>
[[nodiscard]] inline usize
__allocation_policy_capacity(usize bytes)
{
  if ( bytes == 0 ) return 0;
  const usize requested = bytes < P::minimum_bytes ? P::minimum_bytes : bytes;
  return allocation_round_up_or_throw(requested, P::granularity);
}

template<is_policy P>
[[nodiscard]] constexpr usize
__allocation_policy_recommend(usize current, usize minimum) noexcept
{
  usize grown;
  if ( !allocation_checked_growth(current, minimum, P::growth_numerator, P::growth_denominator, grown) ) return __allocation_max;
  if ( grown < P::minimum_bytes ) grown = P::minimum_bytes;
  usize rounded;
  return allocation_checked_round_up(grown, P::granularity, rounded) ? rounded : __allocation_max;
}

struct allocator_stats_snapshot {
  bool enabled;
  u64 allocations;
  u64 deallocations;
  u64 resizes;
  u64 rewinds;
  u64 resets;
  u64 releases;
  u64 bytes_requested;
  u64 bytes_granted;
  u64 bytes_deallocated;
  u64 bytes_copied;
  u64 current_bytes;
  u64 peak_bytes;
  u64 blocks;
  u64 peak_blocks;
};

template<class Tag> class allocator_telemetry
{
#if defined(MICRON_ALLOCATOR_STATS)
  inline static atomic_token<usize> __allocations{ 0 };
  inline static atomic_token<usize> __deallocations{ 0 };
  inline static atomic_token<usize> __resizes{ 0 };
  inline static atomic_token<usize> __rewinds{ 0 };
  inline static atomic_token<usize> __resets{ 0 };
  inline static atomic_token<usize> __releases{ 0 };
  inline static atomic_token<usize> __bytes_requested{ 0 };
  inline static atomic_token<usize> __bytes_granted{ 0 };
  inline static atomic_token<usize> __bytes_deallocated{ 0 };
  inline static atomic_token<usize> __bytes_copied{ 0 };
  inline static atomic_token<usize> __current_bytes{ 0 };
  inline static atomic_token<usize> __peak_bytes{ 0 };
  inline static atomic_token<usize> __blocks{ 0 };
  inline static atomic_token<usize> __peak_blocks{ 0 };

  static void
  __update_peak(atomic_token<usize> &peak, usize value) noexcept
  {
    usize observed = peak.get(memory_order::relaxed);
    while ( observed < value && !peak.compare_exchange_weak(observed, value, memory_order::relaxed, memory_order::relaxed) ) {
    }
  }

  static void
  __subtract_current(usize bytes) noexcept
  {
    usize observed = __current_bytes.get(memory_order::relaxed);
    for ( ;; ) {
      const usize next = bytes > observed ? 0 : observed - bytes;
      if ( __current_bytes.compare_exchange_weak(observed, next, memory_order::relaxed, memory_order::relaxed) ) return;
    }
  }
#endif

protected:
  static void
  __telemetry_allocate(usize requested, usize granted) noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __allocations.fetch_add(1, memory_order::relaxed);
    __bytes_requested.fetch_add(requested, memory_order::relaxed);
    __bytes_granted.fetch_add(granted, memory_order::relaxed);
    const usize current = __current_bytes.add_fetch(granted, memory_order::relaxed);
    __update_peak(__peak_bytes, current);
#else
    (void)requested;
    (void)granted;
#endif
  }

  static void
  __telemetry_deallocate(usize bytes) noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __deallocations.fetch_add(1, memory_order::relaxed);
    __bytes_deallocated.fetch_add(bytes, memory_order::relaxed);
    if ( bytes ) __subtract_current(bytes);
#else
    (void)bytes;
#endif
  }

  static void
  __telemetry_resize(usize requested, usize old_bytes, usize new_bytes, usize copied) noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __resizes.fetch_add(1, memory_order::relaxed);
    __bytes_requested.fetch_add(requested, memory_order::relaxed);
    __bytes_copied.fetch_add(copied, memory_order::relaxed);
    if ( new_bytes >= old_bytes ) {
      const usize current = __current_bytes.add_fetch(new_bytes - old_bytes, memory_order::relaxed);
      __update_peak(__peak_bytes, current);
    } else {
      __subtract_current(old_bytes - new_bytes);
    }
#else
    (void)requested;
    (void)old_bytes;
    (void)new_bytes;
    (void)copied;
#endif
  }

  static void
  __telemetry_rewind() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __rewinds.fetch_add(1, memory_order::relaxed);
#endif
  }

  static void
  __telemetry_reset() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __resets.fetch_add(1, memory_order::relaxed);
    __current_bytes.store(0, memory_order::relaxed);
#endif
  }

  static void
  __telemetry_release() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __releases.fetch_add(1, memory_order::relaxed);
    __current_bytes.store(0, memory_order::relaxed);
    __blocks.store(0, memory_order::relaxed);
#endif
  }

  static void
  __telemetry_block_add() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    const usize blocks = __blocks.add_fetch(1, memory_order::relaxed);
    __update_peak(__peak_blocks, blocks);
#endif
  }

public:
  [[nodiscard]] static allocator_stats_snapshot
  stats() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    return { true,
             __allocations.get(memory_order::relaxed),
             __deallocations.get(memory_order::relaxed),
             __resizes.get(memory_order::relaxed),
             __rewinds.get(memory_order::relaxed),
             __resets.get(memory_order::relaxed),
             __releases.get(memory_order::relaxed),
             __bytes_requested.get(memory_order::relaxed),
             __bytes_granted.get(memory_order::relaxed),
             __bytes_deallocated.get(memory_order::relaxed),
             __bytes_copied.get(memory_order::relaxed),
             __current_bytes.get(memory_order::relaxed),
             __peak_bytes.get(memory_order::relaxed),
             __blocks.get(memory_order::relaxed),
             __peak_blocks.get(memory_order::relaxed) };
#else
    return { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
  }

  static void
  reset_stats() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __allocations.store(0, memory_order::relaxed);
    __deallocations.store(0, memory_order::relaxed);
    __resizes.store(0, memory_order::relaxed);
    __rewinds.store(0, memory_order::relaxed);
    __resets.store(0, memory_order::relaxed);
    __releases.store(0, memory_order::relaxed);
    __bytes_requested.store(0, memory_order::relaxed);
    __bytes_granted.store(0, memory_order::relaxed);
    __bytes_deallocated.store(0, memory_order::relaxed);
    __bytes_copied.store(0, memory_order::relaxed);
    __current_bytes.store(0, memory_order::relaxed);
    __peak_bytes.store(0, memory_order::relaxed);
    __blocks.store(0, memory_order::relaxed);
    __peak_blocks.store(0, memory_order::relaxed);
#endif
  }
};

#if defined(MICRON_ALLOCATOR_CHECKS)
inline constexpr bool allocator_checks_enabled = true;
#else
inline constexpr bool allocator_checks_enabled = false;
#endif

template<class Alloc>
consteval bool
__allocator_is_trusted() noexcept
{
  if constexpr ( requires { Alloc::allocator_trusted; } ) return static_cast<bool>(Alloc::allocator_trusted);
  return false;
}

template<class Alloc> struct allocator_traits {
  static constexpr bool trusted = __allocator_is_trusted<Alloc>();

private:
  static void
  __deallocate_chunk(chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr == nullptr ) return;
    if constexpr ( requires { Alloc::deallocate(memory, alignment); } )
      Alloc::deallocate(memory, alignment);
    else if constexpr ( requires { Alloc::destroy(memory, alignment); } )
      Alloc::destroy(memory, alignment);
    else if constexpr ( requires { Alloc::deallocate(memory); } )
      Alloc::deallocate(memory);
    else
      Alloc::destroy(memory);
  }

  template<usize Alignment>
  static void
  __deallocate_chunk(chunk<byte> memory) noexcept
  {
    if ( memory.ptr == nullptr ) return;
    if constexpr ( requires { Alloc::template deallocate<Alignment>(memory); } )
      Alloc::template deallocate<Alignment>(memory);
    else if constexpr ( requires { Alloc::template destroy<Alignment>(memory); } )
      Alloc::template destroy<Alignment>(memory);
    else
      __deallocate_chunk(memory, Alignment);
  }

  static void
  __validate_result(chunk<byte> result, usize bytes, usize alignment)
  {
    if ( bytes != 0 && result.ptr == nullptr ) exc<except::memory_error>("allocator: returned null storage for a non-zero request");
    if ( result.ptr && (reinterpret_cast<uintptr_t>(result.ptr) & (alignment - 1)) != 0 ) {
      __deallocate_chunk(result, alignment);
      exc<except::memory_error_core_unaligned>("allocator: returned storage is under-aligned");
    }
    if ( result.len < bytes ) {
      __deallocate_chunk(result, alignment);
      exc<except::memory_error>("allocator: returned capacity is smaller than requested");
    }
  }

  template<usize Alignment>
  static void
  __validate_result(chunk<byte> result, usize bytes)
  {
    if ( bytes != 0 && result.ptr == nullptr ) exc<except::memory_error>("allocator: returned null storage for a non-zero request");
    if ( result.ptr && (reinterpret_cast<uintptr_t>(result.ptr) & (Alignment - 1)) != 0 ) {
      __deallocate_chunk<Alignment>(result);
      exc<except::memory_error_core_unaligned>("allocator: returned storage is under-aligned");
    }
    if ( result.len < bytes ) {
      __deallocate_chunk<Alignment>(result);
      exc<except::memory_error>("allocator: returned capacity is smaller than requested");
    }
  }

public:
  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( bytes == 0 ) return 0;
    if constexpr ( requires { Alloc::allocation_extent(bytes, alignment); } )
      return Alloc::allocation_extent(bytes, alignment);
    else if constexpr ( requires { Alloc::allocation_extent(bytes); } )
      return Alloc::allocation_extent(bytes);
    else
      return bytes;
  }

  template<usize Alignment>
  [[nodiscard]] static usize
  allocation_extent(usize bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator: alignment must be a non-zero power of two");
    return allocation_extent(bytes, Alignment);
  }

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] static inline chunk<byte>
  allocate(usize bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator: alignment must be a non-zero power of two");
    chunk<byte> result;
    if constexpr ( requires { Alloc::template allocate<Alignment>(bytes); } )
      result = Alloc::template allocate<Alignment>(bytes);
    else if constexpr ( requires { Alloc::template create<Alignment>(bytes); } )
      result = Alloc::template create<Alignment>(bytes);
    else if constexpr ( requires { Alloc::allocate(bytes, Alignment); } )
      result = Alloc::allocate(bytes, Alignment);
    else if constexpr ( requires { Alloc::create(bytes, Alignment); } )
      result = Alloc::create(bytes, Alignment);
    else if constexpr ( requires { Alloc::allocate(bytes); } )
      result = Alloc::allocate(bytes);
    else
      result = Alloc::create(bytes);

    if constexpr ( !trusted || allocator_checks_enabled ) __validate_result<Alignment>(result, bytes);
    return result;
  }

  [[nodiscard]] static chunk<byte>
  allocate(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    chunk<byte> result;
    if constexpr ( requires { Alloc::allocate(bytes, alignment); } )
      result = Alloc::allocate(bytes, alignment);
    else if constexpr ( requires { Alloc::create(bytes, alignment); } )
      result = Alloc::create(bytes, alignment);
    else if constexpr ( requires { Alloc::allocate(bytes); } )
      result = Alloc::allocate(bytes);
    else
      result = Alloc::create(bytes);

    if constexpr ( !trusted || allocator_checks_enabled ) __validate_result(result, bytes, alignment);
    return result;
  }

  template<usize Alignment>
  static constexpr bool has_unsized_deallocate
      = requires(byte *ptr) { Alloc::template deallocate<Alignment>(ptr); } || requires(byte *ptr) {
          Alloc::template destroy<Alignment>(ptr);
        } || requires(byte *ptr) { Alloc::deallocate(ptr); } || requires(byte *ptr) { Alloc::destroy(ptr); };

  template<usize Alignment>
  static void
  deallocate(byte *memory) noexcept
  {
    static_assert(has_unsized_deallocate<Alignment>, "allocator does not support unsized deallocation");
    if ( memory == nullptr ) return;
    if constexpr ( requires { Alloc::template deallocate<Alignment>(memory); } )
      Alloc::template deallocate<Alignment>(memory);
    else if constexpr ( requires { Alloc::template destroy<Alignment>(memory); } )
      Alloc::template destroy<Alignment>(memory);
    else if constexpr ( requires { Alloc::deallocate(memory); } )
      Alloc::deallocate(memory);
    else
      Alloc::destroy(memory);
  }

  template<usize Alignment>
  [[gnu::always_inline]] static inline void
  deallocate(chunk<byte> memory) noexcept
  {
    __deallocate_chunk<Alignment>(memory);
  }

  static void
  deallocate(chunk<byte> memory, usize alignment) noexcept
  {
    __deallocate_chunk(memory, alignment);
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator: alignment must be a non-zero power of two");
    chunk<byte> result;
    if constexpr ( requires { Alloc::template resize<Alignment>(old, bytes, preserve_bytes); } ) {
      result = Alloc::template resize<Alignment>(old, bytes, preserve_bytes);
    } else if constexpr ( requires { Alloc::resize(old, bytes, preserve_bytes, Alignment); } ) {
      result = Alloc::resize(old, bytes, preserve_bytes, Alignment);
    } else if constexpr ( requires { Alloc::grow(old, bytes); } ) {
      if ( bytes >= old.len && preserve_bytes >= old.len ) {
        result = Alloc::grow(old, bytes);
      } else {
        result = allocate<Alignment>(bytes);
        const usize copied = micron::min(preserve_bytes, old.len, result.len);
        if ( copied ) micron::memcpy(result.ptr, old.ptr, copied);
        __deallocate_chunk<Alignment>(old);
      }
    } else {
      result = allocate<Alignment>(bytes);
      const usize copied = micron::min(preserve_bytes, old.len, result.len);
      if ( copied ) micron::memcpy(result.ptr, old.ptr, copied);
      __deallocate_chunk<Alignment>(old);
    }
    if constexpr ( !trusted || allocator_checks_enabled ) __validate_result<Alignment>(result, bytes);
    return result;
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    chunk<byte> result;
    if constexpr ( requires { Alloc::resize(old, bytes, preserve_bytes, alignment); } ) {
      result = Alloc::resize(old, bytes, preserve_bytes, alignment);
    } else {
      result = allocate(bytes, alignment);
      const usize copied = micron::min(preserve_bytes, old.len, result.len);
      if ( copied ) micron::memcpy(result.ptr, old.ptr, copied);
      __deallocate_chunk(old, alignment);
    }
    if constexpr ( !trusted || allocator_checks_enabled ) __validate_result(result, bytes, alignment);
    return result;
  }

  [[nodiscard]] static constexpr usize
  recommend(usize current, usize minimum) noexcept
  {
    if constexpr ( requires { Alloc::recommend(current, minimum); } ) {
      return Alloc::recommend(current, minimum);
    } else {
      usize result;
      if ( !allocation_checked_growth(current, minimum, 2, 1, result) ) return __allocation_max;
      if constexpr ( requires { Alloc::auto_size(); } ) return result < Alloc::auto_size() ? Alloc::auto_size() : result;
      return result;
    }
  }
};

template<class Alloc, usize Alignment>
[[nodiscard]] inline chunk<byte>
__allocator_create(usize bytes)
{
  return allocator_traits<Alloc>::template allocate<Alignment>(bytes);
}

template<class Alloc>
[[nodiscard]] inline chunk<byte>
__allocator_create(usize bytes, usize alignment)
{
  return allocator_traits<Alloc>::allocate(bytes, alignment);
}

template<class Alloc, usize Alignment>
inline void
__allocator_destroy(chunk<byte> memory) noexcept
{
  allocator_traits<Alloc>::template deallocate<Alignment>(memory);
}

template<class Alloc>
inline void
__allocator_destroy(chunk<byte> memory, usize alignment) noexcept
{
  allocator_traits<Alloc>::deallocate(memory, alignment);
}

template<class Alloc, usize Alignment>
[[nodiscard]] inline chunk<byte>
__allocator_resize_bytes(chunk<byte> old, usize bytes, usize preserve_bytes)
{
  return allocator_traits<Alloc>::template resize<Alignment>(old, bytes, preserve_bytes);
}

template<class Alloc>
[[nodiscard]] inline chunk<byte>
__allocator_resize_bytes(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
{
  return allocator_traits<Alloc>::resize(old, bytes, preserve_bytes, alignment);
}

template<class Alloc>
[[nodiscard]] constexpr usize
__allocator_recommend(usize current, usize minimum) noexcept
{
  return allocator_traits<Alloc>::recommend(current, minimum);
}

template<class Alloc, usize Alignment>
[[nodiscard]] inline usize
__allocator_extent(usize bytes)
{
  return allocator_traits<Alloc>::template allocation_extent<Alignment>(bytes);
}

struct __allocator_array_header {
  chunk<byte> storage;
  usize alignment;
};

template<class Alloc, typename T>
[[nodiscard]] inline T *
__allocator_allocate_array(usize count)
{
  const usize bytes = allocation_multiply_or_throw(count, sizeof(T));
  if constexpr ( allocator_traits<Alloc>::template has_unsized_deallocate<alignof(T)> ) {
    return reinterpret_cast<T *>(__allocator_create<Alloc, alignof(T)>(bytes).ptr);
  }

  constexpr usize alignment = alignof(T) < alignof(__allocator_array_header) ? alignof(__allocator_array_header) : alignof(T);
  const usize overhead = allocation_add_or_throw(sizeof(__allocator_array_header), alignment - 1);
  chunk<byte> storage = __allocator_create<Alloc, alignment>(allocation_add_or_throw(bytes, overhead));
  const uintptr_t first = reinterpret_cast<uintptr_t>(storage.ptr) + sizeof(__allocator_array_header);
  const uintptr_t aligned = (first + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
  T *result = reinterpret_cast<T *>(aligned);
  auto *header = reinterpret_cast<__allocator_array_header *>(reinterpret_cast<byte *>(result) - sizeof(__allocator_array_header));
  new (static_cast<void *>(header)) __allocator_array_header{ storage, alignment };
  return result;
}

template<class Alloc, typename T>
[[nodiscard]] inline T *
__allocator_allocate_array(usize count, usize alignment)
{
  allocation_validate_alignment(alignment);
  if ( alignment < alignof(T) ) alignment = alignof(T);
  if ( alignment < alignof(__allocator_array_header) ) alignment = alignof(__allocator_array_header);
  const usize bytes = allocation_multiply_or_throw(count, sizeof(T));
  const usize overhead = allocation_add_or_throw(sizeof(__allocator_array_header), alignment - 1);
  chunk<byte> storage = __allocator_create<Alloc>(allocation_add_or_throw(bytes, overhead), alignment);
  const uintptr_t first = reinterpret_cast<uintptr_t>(storage.ptr) + sizeof(__allocator_array_header);
  const uintptr_t aligned = (first + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
  T *result = reinterpret_cast<T *>(aligned);
  auto *header = reinterpret_cast<__allocator_array_header *>(reinterpret_cast<byte *>(result) - sizeof(__allocator_array_header));
  new (static_cast<void *>(header)) __allocator_array_header{ storage, alignment };
  return result;
}

template<class Alloc, typename T>
inline void
__allocator_deallocate_array(T *memory) noexcept
{
  if ( memory == nullptr ) return;
  if constexpr ( allocator_traits<Alloc>::template has_unsized_deallocate<alignof(T)> ) {
    allocator_traits<Alloc>::template deallocate<alignof(T)>(reinterpret_cast<byte *>(memory));
    return;
  }
  auto *header = reinterpret_cast<__allocator_array_header *>(reinterpret_cast<byte *>(memory) - sizeof(__allocator_array_header));
  __allocator_destroy<Alloc>(header->storage, header->alignment);
}

};      // namespace micron
