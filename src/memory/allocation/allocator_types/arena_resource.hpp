//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/flag.hpp"
#include "../../../memory/actions.hpp"
#include "../../../memory/placement_new.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// instance-owned rewindable arena
//
// use arena_resource when many allocations share a phase or transaction lifetime;
// enable thread_confined for no locking or shared for a spin lock;
// external arenas can fail on exhaustion or spill upstream

namespace micron
{

enum class arena_sync : u8 { thread_confined, shared };
enum class arena_overflow : u8 { fail, upstream };

template<arena_sync> class __arena_lock;

template<> class __arena_lock<arena_sync::thread_confined>
{
protected:
  void
  __lock() const noexcept
  {
  }

  void
  __unlock() const noexcept
  {
  }
};

template<> class __arena_lock<arena_sync::shared>
{
  mutable atomic_flag __flag{};

protected:
  void
  __lock() const noexcept
  {
    __flag.ttas(memory_order::acquire, memory_order::relaxed);
  }

  void
  __unlock() const noexcept
  {
    __flag.clear(memory_order::release);
  }
};

template<bool> class __arena_stats;

template<> class __arena_stats<false>
{
protected:
  void
  __stat_allocate(usize, usize) noexcept
  {
  }

  void
  __stat_deallocate(usize) noexcept
  {
  }

  void
  __stat_resize(usize) noexcept
  {
  }

  void
  __stat_copy(usize) noexcept
  {
  }

  void
  __stat_block_add(usize) noexcept
  {
  }

  void
  __stat_blocks(usize) noexcept
  {
  }

  void
  __stat_used(usize) noexcept
  {
  }

  void
  __stat_rewind() noexcept
  {
  }

  void
  __stat_reset() noexcept
  {
  }

  void
  __stat_release() noexcept
  {
  }

  [[nodiscard]] static constexpr allocator_stats_snapshot
  __stat_snapshot() noexcept
  {
    return { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  }

  void
  __stat_clear() noexcept
  {
  }
};

template<> class __arena_stats<true>
{
  allocator_stats_snapshot __value{ true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

protected:
  void
  __stat_allocate(usize requested, usize granted) noexcept
  {
    ++__value.allocations;
    __value.bytes_requested += requested;
    __value.bytes_granted += granted;
    __value.current_bytes += granted;
    if ( __value.current_bytes > __value.peak_bytes ) __value.peak_bytes = __value.current_bytes;
  }

  void
  __stat_deallocate(usize bytes) noexcept
  {
    ++__value.deallocations;
    __value.bytes_deallocated += bytes;
  }

  void
  __stat_resize(usize requested) noexcept
  {
    ++__value.resizes;
    __value.bytes_requested += requested;
  }

  void
  __stat_copy(usize bytes) noexcept
  {
    __value.bytes_copied += bytes;
  }

  void
  __stat_block_add(usize) noexcept
  {
    ++__value.blocks;
    if ( __value.blocks > __value.peak_blocks ) __value.peak_blocks = __value.blocks;
  }

  void
  __stat_blocks(usize blocks) noexcept
  {
    __value.blocks = blocks;
  }

  void
  __stat_used(usize bytes) noexcept
  {
    __value.current_bytes = bytes;
  }

  void
  __stat_rewind() noexcept
  {
    ++__value.rewinds;
  }

  void
  __stat_reset() noexcept
  {
    ++__value.resets;
  }

  void
  __stat_release() noexcept
  {
    ++__value.releases;
  }

  [[nodiscard]] allocator_stats_snapshot
  __stat_snapshot() const noexcept
  {
    return __value;
  }

  void
  __stat_clear() noexcept
  {
    __value = { true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  }
};

#if defined(MICRON_ALLOCATOR_STATS)
inline constexpr bool __allocator_stats_enabled = true;
#else
inline constexpr bool __allocator_stats_enabled = false;
#endif

template<class Upstream = allocator_serial<>, arena_sync Sync = arena_sync::thread_confined>
class arena_resource: private __arena_lock<Sync>, private __arena_stats<__allocator_stats_enabled>
{
  using __lock_base = __arena_lock<Sync>;
  using __stats_base = __arena_stats<__allocator_stats_enabled>;

  struct __block {
    chunk<byte> storage;
    __block *next;
    byte *begin;
    byte *end;
    byte *cursor;
    usize upstream_alignment;
    bool external;
    bool embedded;
  };

  struct __invalid_marker_range {
    u64 first;
    u64 last;
  };

public:
  struct marker {
    const void *resource;
    void *block;
    usize cursor;
    u64 generation;
    u64 serial;
  };

private:
  class __guard
  {
    const arena_resource *__resource;

  public:
    explicit __guard(const arena_resource *resource) noexcept : __resource(resource) { __resource->__lock_base::__lock(); }

    ~__guard() { __resource->__lock_base::__unlock(); }

    __guard(const __guard &) = delete;
    __guard &operator=(const __guard &) = delete;
  };

  __block __first{};
  __block *__head = nullptr;
  __block *__tail = nullptr;
  __block *__current = nullptr;
  byte *__last_ptr = nullptr;
  usize __block_bytes = page_size;
  arena_overflow __overflow = arena_overflow::upstream;
  bool __external_origin = false;
  u64 __generation = 1;
  u64 __marker_next = 0;
  __invalid_marker_range __invalid_markers[8]{};
  u8 __invalid_count = 0;

  [[nodiscard]] static usize
  __max(usize a, usize b) noexcept
  {
    return a > b ? a : b;
  }

  void
  __init_external(chunk<byte> storage)
  {
    if ( storage.ptr == nullptr || storage.len == 0 ) exc<except::invalid_argument>("arena_resource: external span must be non-empty");
    __first = { storage, nullptr, storage.ptr, storage.ptr + storage.len, storage.ptr, 1, true, false };
    __head = __tail = __current = &__first;
    __external_origin = true;
    this->__stat_block_add(storage.len);
  }

  [[nodiscard, gnu::always_inline]] static inline chunk<byte>
  __try_allocate(__block *block, usize bytes, usize alignment) noexcept
  {
    const uintptr_t current = reinterpret_cast<uintptr_t>(block->cursor);
    const uintptr_t end = reinterpret_cast<uintptr_t>(block->end);
    if ( current > end ) return { nullptr, 0 };
    if ( current > micron::numeric_limits<uintptr_t>::max() - (alignment - 1) ) return { nullptr, 0 };
    const uintptr_t aligned = (current + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    if ( aligned > end || bytes > end - aligned ) return { nullptr, 0 };
    block->cursor = reinterpret_cast<byte *>(aligned + bytes);
    return { reinterpret_cast<byte *>(aligned), bytes };
  }

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] static inline bool
  __try_allocate(__block *block, usize bytes, byte *&result) noexcept
  {
    const uintptr_t current = reinterpret_cast<uintptr_t>(block->cursor);
    const uintptr_t end = reinterpret_cast<uintptr_t>(block->end);
    if constexpr ( Alignment > page_size ) {
      chunk<byte> memory = __try_allocate(block, bytes, Alignment);
      result = memory.ptr;
      return result != nullptr;
    }
    const usize padding = static_cast<usize>(-current) & (Alignment - 1);
    const uintptr_t aligned = current + padding;
    if ( aligned > end || bytes > end - aligned ) return false;
    result = reinterpret_cast<byte *>(aligned);
    block->cursor = result + bytes;
    return true;
  }

  [[gnu::always_inline]] inline void
  __record_allocation(chunk<byte> result, usize bytes) noexcept
  {
    __last_ptr = result.ptr;
    this->__stat_allocate(bytes, result.len);
  }

  [[nodiscard]] __block *
  __append_owned(usize bytes, usize alignment)
  {
    const usize usable = __max(__block_bytes, bytes);
    if ( __head == nullptr && !__external_origin ) {
      const usize upstream_alignment = __max(alignment, alignof(__block));
      chunk<byte> storage = __allocator_create<Upstream>(usable, upstream_alignment);
      __first = { storage, nullptr, storage.ptr, storage.ptr + storage.len, storage.ptr, upstream_alignment, false, false };
      __head = __tail = __current = &__first;
      this->__stat_block_add(storage.len);
      return &__first;
    }

    const usize overhead = allocation_add_or_throw(sizeof(__block), alignment - 1);
    const usize total = allocation_add_or_throw(usable, overhead);
    const usize upstream_alignment = __max(alignment, alignof(__block));
    chunk<byte> storage = __allocator_create<Upstream>(total, upstream_alignment);
    auto *block = new (static_cast<void *>(storage.ptr)) __block{};
    const uintptr_t first = reinterpret_cast<uintptr_t>(storage.ptr) + sizeof(__block);
    const uintptr_t aligned = (first + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    block->storage = storage;
    block->next = nullptr;
    block->begin = reinterpret_cast<byte *>(aligned);
    block->end = storage.ptr + storage.len;
    block->cursor = block->begin;
    block->upstream_alignment = upstream_alignment;
    block->external = false;
    block->embedded = true;
    if ( __tail )
      __tail->next = block;
    else
      __head = block;
    __tail = __current = block;
    this->__stat_block_add(static_cast<usize>(block->end - block->begin));
    return block;
  }

  [[nodiscard, gnu::noinline]] chunk<byte>
  __allocate_slow(usize bytes, usize alignment)
  {
    __block *block = __current ? __current->next : __head;
    for ( ; block; block = block->next ) {
      chunk<byte> result = __try_allocate(block, bytes, alignment);
      if ( result.ptr ) {
        __current = block;
        __record_allocation(result, bytes);
        return result;
      }
    }

    if ( __external_origin && __overflow == arena_overflow::fail ) exc<except::memory_error>("arena_resource: external span exhausted");
    block = __append_owned(bytes, alignment);
    chunk<byte> result = __try_allocate(block, bytes, alignment);
    if ( result.ptr == nullptr ) exc<except::memory_error>("arena_resource: upstream block cannot satisfy allocation");
    __current = block;
    __record_allocation(result, bytes);
    return result;
  }

  [[nodiscard, gnu::always_inline]] inline chunk<byte>
  __allocate_locked(usize bytes, usize alignment)
  {
    if ( bytes == 0 ) return { nullptr, 0 };
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
  [[nodiscard, gnu::always_inline]] inline chunk<byte>
  __allocate_locked(usize bytes)
  {
    if ( bytes == 0 ) return { nullptr, 0 };
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

  [[nodiscard]] bool
  __owns_locked(const byte *address) const noexcept
  {
    if ( address == nullptr ) return false;
    const uintptr_t target = reinterpret_cast<uintptr_t>(address);
    for ( const __block *block = __head; block; block = block->next ) {
      const uintptr_t begin = reinterpret_cast<uintptr_t>(block->begin);
      if ( target >= begin && target < reinterpret_cast<uintptr_t>(block->end) ) return true;
    }
    return false;
  }

  [[nodiscard]] usize
  __used_locked() const noexcept
  {
    usize total = 0;
    for ( const __block *block = __head; block; block = block->next ) total += static_cast<usize>(block->cursor - block->begin);
    return total;
  }

  [[nodiscard]] usize
  __capacity_locked() const noexcept
  {
    usize total = 0;
    for ( const __block *block = __head; block; block = block->next ) total += static_cast<usize>(block->end - block->begin);
    return total;
  }

  [[nodiscard]] usize
  __blocks_locked() const noexcept
  {
    usize total = 0;
    for ( const __block *block = __head; block; block = block->next ) ++total;
    return total;
  }

  void
  __invalidate_markers(u64 first, u64 last) noexcept
  {
    if ( first > last ) return;
    for ( u8 i = 0; i < __invalid_count; ++i ) {
      __invalid_marker_range &range = __invalid_markers[i];
      if ( last + (last != micron::numeric_limits<u64>::max()) < range.first
           || range.last + (range.last != micron::numeric_limits<u64>::max()) < first )
        continue;
      if ( first < range.first ) range.first = first;
      if ( last > range.last ) range.last = last;
      return;
    }
    if ( __invalid_count < 8 ) {
      __invalid_markers[__invalid_count++] = { first, last };
      return;
    }
    ++__generation;
    __marker_next = 0;
    __invalid_count = 0;
  }

  [[nodiscard]] bool
  __marker_valid(const marker &value) const noexcept
  {
    if ( value.resource != this || value.generation != __generation || value.serial == 0 || value.serial > __marker_next ) return false;
    for ( u8 i = 0; i < __invalid_count; ++i )
      if ( value.serial >= __invalid_markers[i].first && value.serial <= __invalid_markers[i].last ) return false;
    return true;
  }

  void
  __clear_last() noexcept
  {
    __last_ptr = nullptr;
  }

  void
  __new_generation() noexcept
  {
    ++__generation;
    if ( __generation == 0 ) ++__generation;
    __marker_next = 0;
    __invalid_count = 0;
  }

  void
  __release_locked(bool count_release) noexcept
  {
    __block *block = __head;
    __block *kept_external = nullptr;
    while ( block ) {
      __block *next = block->next;
      if ( block->external ) {
        block->cursor = block->begin;
        block->next = nullptr;
        kept_external = block;
      } else {
        const chunk<byte> storage = block->storage;
        const usize alignment = block->upstream_alignment;
        if ( block->embedded ) block->~__block();
        __allocator_destroy<Upstream>(storage, alignment);
      }
      block = next;
    }
    __head = __tail = __current = kept_external;
    __clear_last();
    __new_generation();
    this->__stat_used(0);
    this->__stat_blocks(kept_external ? 1 : 0);
    if ( count_release ) this->__stat_release();
  }

public:
  arena_resource() noexcept = default;

  explicit arena_resource(usize block_bytes) : __block_bytes(block_bytes)
  {
    if ( block_bytes == 0 ) exc<except::invalid_argument>("arena_resource: owned block size must be non-zero");
  }

  explicit arena_resource(chunk<byte> external, arena_overflow overflow = arena_overflow::fail, usize block_bytes = page_size)
      : __block_bytes(block_bytes), __overflow(overflow)
  {
    if ( block_bytes == 0 ) exc<except::invalid_argument>("arena_resource: owned block size must be non-zero");
    __init_external(external);
  }

  arena_resource(byte *external, usize bytes, arena_overflow overflow = arena_overflow::fail, usize block_bytes = page_size)
      : arena_resource(chunk<byte>{ external, bytes }, overflow, block_bytes)
  {
  }

  ~arena_resource()
  {
    __guard guard(this);
    __release_locked(false);
  }

  arena_resource(const arena_resource &) = delete;
  arena_resource(arena_resource &&) = delete;
  arena_resource &operator=(const arena_resource &) = delete;
  arena_resource &operator=(arena_resource &&) = delete;

  template<usize Alignment>
  [[nodiscard, gnu::always_inline]] inline chunk<byte>
  allocate(usize bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "arena_resource: alignment must be a non-zero power of two");
    __guard guard(this);
    return __allocate_locked<Alignment>(bytes);
  }

  [[nodiscard]] chunk<byte>
  allocate(usize bytes, usize alignment = alignof(max_align_t))
  {
    allocation_validate_alignment(alignment);
    __guard guard(this);
    return __allocate_locked(bytes, alignment);
  }

  template<typename T>
  [[nodiscard]] T *
  allocate_objects(usize count = 1)
  {
    return reinterpret_cast<T *>(allocate<alignof(T)>(allocation_multiply_or_throw(count, sizeof(T))).ptr);
  }

  template<usize Alignment>
  [[nodiscard]] chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    static_assert(allocation_is_power_of_two(Alignment), "arena_resource: alignment must be a non-zero power of two");
    return resize(old, bytes, preserve_bytes, Alignment);
  }

  [[nodiscard]] chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    __guard guard(this);
    if ( old.ptr == nullptr ) return __allocate_locked(bytes, alignment);
    if ( !__owns_locked(old.ptr) ) exc<except::invalid_argument>("arena_resource: resize pointer is not owned by this arena");

    __block *last_block = __current;
    if ( old.ptr == __last_ptr && last_block && (reinterpret_cast<uintptr_t>(old.ptr) & (alignment - 1)) == 0 ) {
      const usize last_offset = static_cast<usize>(reinterpret_cast<uintptr_t>(old.ptr) - reinterpret_cast<uintptr_t>(last_block->begin));
      const usize last_capacity = static_cast<usize>(last_block->end - last_block->begin);
      if ( last_offset <= last_capacity && old.len <= last_capacity - last_offset && last_block->cursor == old.ptr + old.len
           && bytes <= last_capacity - last_offset ) {
        last_block->cursor = old.ptr + bytes;
        this->__stat_resize(bytes);
        this->__stat_used(__used_locked());
        if ( bytes == 0 ) {
          __clear_last();
          return { nullptr, 0 };
        }
        return { old.ptr, bytes };
      }
    }

    chunk<byte> next = __allocate_locked(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied ) micron::memcpy(next.ptr, old.ptr, copied);
    this->__stat_resize(bytes);
    this->__stat_copy(copied);
    return next;
  }

  void
  deallocate(chunk<byte> memory) noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __guard guard(this);
#endif
    this->__stat_deallocate(memory.len);
  }

  void
  deallocate(byte *, usize bytes = 0) noexcept
  {
#if defined(MICRON_ALLOCATOR_STATS)
    __guard guard(this);
#endif
    this->__stat_deallocate(bytes);
  }

  [[nodiscard]] marker
  mark() noexcept
  {
    __guard guard(this);
    ++__marker_next;
    if ( __marker_next == 0 ) {
      __new_generation();
      ++__marker_next;
    }
    return { this, __current, __current ? static_cast<usize>(__current->cursor - __current->begin) : 0, __generation, __marker_next };
  }

  [[nodiscard]] bool
  rewind(const marker &value) noexcept
  {
    __guard guard(this);
    if ( !__marker_valid(value) ) return false;
    __block *target = reinterpret_cast<__block *>(value.block);
    if ( target ) {
      bool found = false;
      for ( __block *block = __head; block; block = block->next ) {
        if ( block == target ) {
          found = true;
          if ( value.cursor > static_cast<usize>(block->end - block->begin) ) return false;
          block->cursor = block->begin + value.cursor;
          for ( block = block->next; block; block = block->next ) block->cursor = block->begin;
          break;
        }
      }
      if ( !found ) return false;
      __current = target;
    } else {
      for ( __block *block = __head; block; block = block->next ) block->cursor = block->begin;
      __current = __head;
    }
    __invalidate_markers(value.serial + 1, __marker_next);
    __clear_last();
    this->__stat_used(__used_locked());
    this->__stat_rewind();
    return true;
  }

  void
  reset() noexcept
  {
    __guard guard(this);
    for ( __block *block = __head; block; block = block->next ) block->cursor = block->begin;
    __current = __head;
    __clear_last();
    __new_generation();
    this->__stat_used(0);
    this->__stat_reset();
  }

  void
  release() noexcept
  {
    __guard guard(this);
    __release_locked(true);
  }

  [[nodiscard]] bool
  owns(const void *address) const noexcept
  {
    __guard guard(this);
    return __owns_locked(reinterpret_cast<const byte *>(address));
  }

  [[nodiscard]] usize
  capacity() const noexcept
  {
    __guard guard(this);
    return __capacity_locked();
  }

  [[nodiscard]] usize
  used() const noexcept
  {
    __guard guard(this);
    return __used_locked();
  }

  [[nodiscard]] usize
  usage() const noexcept
  {
    return used();
  }

  [[nodiscard]] usize
  available() const noexcept
  {
    __guard guard(this);
    const usize total = __capacity_locked();
    const usize consumed = __used_locked();
    return total - consumed;
  }

  [[nodiscard]] usize
  block_count() const noexcept
  {
    __guard guard(this);
    return __blocks_locked();
  }

  [[nodiscard]] usize
  block_size() const noexcept
  {
    return __block_bytes;
  }

  [[nodiscard]] allocator_stats_snapshot
  stats() const noexcept
  {
    __guard guard(this);
    return this->__stats_base::__stat_snapshot();
  }

  void
  reset_stats() noexcept
  {
    __guard guard(this);
    this->__stats_base::__stat_clear();
    this->__stats_base::__stat_blocks(__blocks_locked());
    this->__stats_base::__stat_used(__used_locked());
  }
};

template<auto &Resource> class arena_allocator
{
  using resource_type = remove_reference_t<decltype(Resource)>;
  static_assert(requires(usize bytes, usize alignment, chunk<byte> old) {
    Resource.allocate(bytes, alignment);
    Resource.resize(old, bytes, bytes, alignment);
  });

public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return 16;
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return Resource.template allocate<Alignment>(bytes);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    return Resource.allocate(bytes, alignment);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create<16>(bytes);
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  allocate(usize bytes)
  {
    return create<Alignment>(bytes);
  }

  [[nodiscard]] static chunk<byte>
  allocate(usize bytes, usize alignment)
  {
    return create(bytes, alignment);
  }

  template<usize Alignment>
  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes)
  {
    return Resource.template resize<Alignment>(old, bytes, preserve_bytes);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    return Resource.resize(old, bytes, preserve_bytes, alignment);
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("arena_allocator: growth overflow");
    return resize<16>(old, target, old.len);
  }

  template<usize>
  static void
  destroy(chunk<byte> memory) noexcept
  {
    Resource.deallocate(memory);
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    Resource.deallocate(memory);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    Resource.deallocate(memory);
  }

  template<usize>
  static void
  destroy(byte *memory) noexcept
  {
    Resource.deallocate(memory);
  }

  static void
  destroy(byte *memory, usize) noexcept
  {
    Resource.deallocate(memory);
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

  [[nodiscard]] static allocator_stats_snapshot
  stats() noexcept
  {
    return Resource.stats();
  }

  static void
  reset_stats() noexcept
  {
    Resource.reset_stats();
  }
};

};      // namespace micron
