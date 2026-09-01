//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// map_allocator
//
// use for large or independently releasable regions that should bypass abcmalloc and map directly through Linux; every allocation is
// page-rounded; over-page alignment is supported by trimming a larger mapping; destroy returns the whole region with munmap

namespace micron
{

inline chunk<byte>
__map_create_aligned(usize capacity, usize alignment, i32 extra_flags = 0)
{
  allocation_validate_alignment(alignment);
  capacity = allocation_round_up_or_throw(capacity, page_size);
  if ( capacity == 0 ) return { nullptr, 0 };

  usize mapping_len = capacity;
  if ( alignment > page_size ) mapping_len = allocation_add_or_throw(capacity, alignment);

  addr_t *mapping = micron::mmap(nullptr, mapping_len, prot_read | prot_write, map_private | map_anonymous | extra_flags, -1, 0);
  if ( micron::mmap_failed(mapping) ) exc<except::memory_error>("allocator: mmap failed");

  if ( alignment <= page_size ) return { reinterpret_cast<byte *>(mapping), capacity };

  const uintptr_t raw = reinterpret_cast<uintptr_t>(mapping);
  const uintptr_t aligned = (raw + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
  const usize prefix = aligned - raw;
  const usize suffix = mapping_len - prefix - capacity;
  if ( prefix != 0 ) micron::munmap(mapping, prefix);
  if ( suffix != 0 ) micron::munmap(reinterpret_cast<addr_t *>(aligned + capacity), suffix);
  return { reinterpret_cast<byte *>(aligned), capacity };
}

template<is_policy P = serial_allocation_policy> class map_allocator
{
  static usize
  __capacity(usize bytes)
  {
    return allocation_round_up_or_throw(__allocation_policy_capacity<P>(bytes), page_size);
  }

public:
  static constexpr bool allocator_trusted = true;

  ~map_allocator() = default;
  map_allocator() = default;
  map_allocator(const map_allocator &) = default;
  map_allocator(map_allocator &&) = default;
  map_allocator &operator=(const map_allocator &) = default;
  map_allocator &operator=(map_allocator &&) = default;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return page_size;
  }

  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize)
  {
    return __capacity(bytes);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    return __map_create_aligned(__capacity(bytes), alignment);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create(bytes, page_size);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
    if ( old.ptr == nullptr ) return create(bytes, alignment);
    allocation_validate_alignment(alignment);
    const usize capacity = __capacity(bytes);
    const usize overlap = micron::min(old.len, capacity);

    if ( alignment <= page_size && preserve_bytes >= overlap ) {
      addr_t *result = micron::mremap(micron::ptr_cast<addr_t *>(old.ptr), old.len, capacity, mremap_maymove);
      if ( !micron::mmap_failed(result) ) return { reinterpret_cast<byte *>(result), capacity };
    }

    chunk<byte> next = create(bytes, alignment);
    const usize copied = micron::min(preserve_bytes, old.len, next.len);
    if ( copied != 0 ) micron::memcpy(next.ptr, old.ptr, copied);
    destroy(old, alignment);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("map_allocator: growth overflow");
    return resize(old, target, old.len, page_size);
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    if ( memory.ptr == nullptr ) return;
    micron::munmap(micron::ptr_cast<addr_t *>(memory.ptr), memory.len);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy(memory, page_size);
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
