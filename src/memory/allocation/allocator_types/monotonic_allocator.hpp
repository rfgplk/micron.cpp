//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/flag.hpp"
#include "../../../memory/actions.hpp"
#include "../../../memory/placement_new.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// process-wide monotonic allocator
//
// use when all allocations of a type domain can die together and an allocator type is required

namespace micron
{

template<class Tag, usize BlockBytes = page_size, class Upstream = allocator_serial<>> class allocator_monotonic
{
  static_assert(BlockBytes != 0, "allocator_monotonic: BlockBytes must be non-zero");

  struct __block {
    chunk<byte> storage;
    __block *next;
    byte *begin;
    byte *end;
    byte *cursor;
    usize allocation_alignment;
  };

  inline static atomic_flag __locked{};
  inline static __block *__head = nullptr;
  inline static __block *__tail = nullptr;
  inline static __block *__current = nullptr;
  inline static byte *__last_ptr = nullptr;

#if defined(MICRON_ALLOCATOR_STATS)
  inline static allocator_stats_snapshot __stats{ true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

  [[gnu::always_inline]] static inline void
  __lock() noexcept
  {
    __locked.ttas(memory_order::acquire, memory_order::relaxed);
  }

  [[gnu::always_inline]] static inline void
  __unlock() noexcept
  {
    __locked.clear(memory_order::release);
  }

  struct __guard {
    [[gnu::always_inline]] __guard() noexcept { __lock(); }

    [[gnu::always_inline]] ~__guard() { __unlock(); }

    __guard(const __guard &) = delete;
    __guard &operator=(const __guard &) = delete;
  };

  static __block *
  __new_block(usize bytes, usize alignment)
  {
    const usize usable = bytes < BlockBytes ? BlockBytes : bytes;
    const usize overhead = allocation_add_or_throw(sizeof(__block), alignment - 1);
    const usize total = allocation_add_or_throw(usable, overhead);
    const usize block_alignment = alignment < alignof(__block) ? alignof(__block) : alignment;
    chunk<byte> storage = __allocator_create<Upstream>(total, block_alignment);
    auto *block = new (static_cast<void *>(storage.ptr)) __block{};
    const uintptr_t first = reinterpret_cast<uintptr_t>(storage.ptr) + sizeof(__block);
    const uintptr_t aligned = (first + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    block->storage = storage;
    block->next = nullptr;
    block->begin = reinterpret_cast<byte *>(aligned);
    block->end = storage.ptr + storage.len;
    block->cursor = block->begin;
    block->allocation_alignment = block_alignment;
    if ( __tail )
      __tail->next = block;
    else
      __head = block;
    __tail = block;
    __current = block;
#if defined(MICRON_ALLOCATOR_STATS)
    ++__stats.blocks;
    if ( __stats.blocks > __stats.peak_blocks ) __stats.peak_blocks = __stats.blocks;
#endif
    return block;
  }

  static chunk<byte>
  __try_allocate(__block *block, usize bytes, usize alignment) noexcept
  {
    const uintptr_t current = reinterpret_cast<uintptr_t>(block->cursor);
    const uintptr_t end = reinterpret_cast<uintptr_t>(block->end);
    if ( current > micron::numeric_limits<uintptr_t>::max() - (alignment - 1) ) return { nullptr, 0 };
    const uintptr_t aligned = (current + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    if ( aligned > end || bytes > end - aligned ) return { nullptr, 0 };
    block->cursor = reinterpret_cast<byte *>(aligned + bytes);
    return { reinterpret_cast<byte *>(aligned), bytes };
  }

  template<usize Alignment>
  [[gnu::always_inline]] static inline bool
  __try_allocate(__block *block, usize bytes, byte *&result) noexcept
  {
    if constexpr ( Alignment > page_size ) {
      chunk<byte> memory = __try_allocate(block, bytes, Alignment);
      result = memory.ptr;
      return result != nullptr;
    }
    const uintptr_t current = reinterpret_cast<uintptr_t>(block->cursor);
    const uintptr_t end = reinterpret_cast<uintptr_t>(block->end);
    const usize padding = static_cast<usize>(-current) & (Alignment - 1);
    const uintptr_t aligned = current + padding;
    if ( aligned > end || bytes > end - aligned ) return false;
    result = reinterpret_cast<byte *>(aligned);
    block->cursor = result + bytes;
    return true;
  }

  [[gnu::always_inline]] static inline void
  __record_allocation(chunk<byte> result, usize bytes) noexcept
  {
    __last_ptr = result.ptr;
#if defined(MICRON_ALLOCATOR_STATS)
    ++__stats.allocations;
    __stats.bytes_requested += bytes;
    __stats.bytes_granted += result.len;
    __stats.current_bytes += result.len;
    if ( __stats.current_bytes > __stats.peak_bytes ) __stats.peak_bytes = __stats.current_bytes;
#else
    (void)bytes;
#endif
  }

  [[gnu::noinline]] static chunk<byte>
  __allocate_slow(usize bytes, usize alignment)
  {
    for ( __block *block = __current ? __current->next : __head; block; block = block->next ) {
      chunk<byte> result = __try_allocate(block, bytes, alignment);
      if ( result.ptr ) {
        __current = block;
        __record_allocation(result, bytes);
        return result;
      }
    }
    __block *block = __new_block(bytes, alignment);
    chunk<byte> result = __try_allocate(block, bytes, alignment);
    __record_allocation(result, bytes);
    return result;
  }

public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return 16;
  }

  [[nodiscard]] static constexpr usize
  allocation_extent(usize bytes, usize) noexcept
  {
    return bytes;
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( bytes == 0 ) return { nullptr, 0 };
    __guard guard;
    __block *block = __current;
    if ( block ) [[likely]] {
      chunk<byte> result = __try_allocate(block, bytes, alignment);
      if ( result.ptr ) [[likely]] {
        __record_allocation(result, bytes);
        return result;
      }
    }
    return __allocate_slow(bytes, alignment);
  }

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] static inline chunk<byte>
  create(usize bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "allocator_monotonic: alignment must be a non-zero power of two");
    if ( bytes == 0 ) return { nullptr, 0 };
    __guard guard;
    __block *block = __current;
    if ( block ) [[likely]] {
      byte *ptr;
      if ( __try_allocate<Alignment>(block, bytes, ptr) ) [[likely]] {
        chunk<byte> result{ ptr, bytes };
        __record_allocation(result, bytes);
        return result;
      }
    }
    return __allocate_slow(bytes, Alignment);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create<16>(bytes);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( old.ptr == nullptr ) return create(bytes, alignment);
    {
      __guard guard;
      __block *block = __current;
      if ( old.ptr == __last_ptr && block && (reinterpret_cast<uintptr_t>(old.ptr) & (alignment - 1)) == 0 ) {
        const usize offset = static_cast<usize>(reinterpret_cast<uintptr_t>(old.ptr) - reinterpret_cast<uintptr_t>(block->begin));
        const usize capacity = static_cast<usize>(block->end - block->begin);
        if ( offset <= capacity && old.len <= capacity - offset && block->cursor == old.ptr + old.len && bytes <= capacity - offset ) {
          block->cursor = old.ptr + bytes;
#if defined(MICRON_ALLOCATOR_STATS)
          ++__stats.resizes;
          __stats.bytes_requested += bytes;
          usize used = 0;
          for ( __block *it = __head; it; it = it->next ) used += static_cast<usize>(it->cursor - it->begin);
          __stats.current_bytes = used;
#endif
          if ( bytes == 0 ) {
            __last_ptr = nullptr;
            return { nullptr, 0 };
          }
          return { old.ptr, bytes };
        }
      }
    }
    chunk<byte> next = create(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied != 0 ) micron::memcpy(next.ptr, old.ptr, copied);
#if defined(MICRON_ALLOCATOR_STATS)
    {
      __guard guard;
      ++__stats.resizes;
      __stats.bytes_copied += copied;
    }
#endif
    return next;
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    return resize(old, bytes, preserve_bytes, Alignment);
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator_monotonic: growth overflow");
    return resize(old, target, old.len, 16);
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
    __guard guard;
    for ( __block *block = __head; block; block = block->next ) block->cursor = block->begin;
    __current = __head;
    __last_ptr = nullptr;
#if defined(MICRON_ALLOCATOR_STATS)
    ++__stats.resets;
    __stats.current_bytes = 0;
#endif
  }

  static void
  release() noexcept
  {
    __guard guard;
    while ( __head ) {
      __block *block = __head;
      __head = block->next;
      chunk<byte> storage = block->storage;
      const usize alignment = block->allocation_alignment;
      block->~__block();
      __allocator_destroy<Upstream>(storage, alignment);
    }
    __tail = nullptr;
    __current = nullptr;
    __last_ptr = nullptr;
#if defined(MICRON_ALLOCATOR_STATS)
    ++__stats.releases;
    __stats.blocks = 0;
    __stats.current_bytes = 0;
#endif
  }

  [[nodiscard]] static allocator_stats_snapshot
  stats() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __guard guard;
    return __stats;
#else
    return { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
  }

  static void
  reset_stats() noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __guard guard;
    usize blocks = 0;
    usize used = 0;
    for ( __block *block = __head; block; block = block->next ) {
      ++blocks;
      used += static_cast<usize>(block->cursor - block->begin);
    }
    __stats = { true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, used, used, blocks, blocks };
#endif
  }

  byte *share(void) = delete;
};

};      // namespace micron
