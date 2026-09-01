//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_guarded
//
// use for diagnostics, fuzzing, and hostile boundaries where an out-of-bounds access should fault close to its source;
// every allocation gets inaccessible guard pages on both sides and is right-aligned against the trailing guard, making the first byte past
// capacity fault

namespace micron
{

template<is_policy P = exact_allocation_policy> class allocator_guarded
{
  static usize
  __capacity(usize bytes, usize alignment)
  {
    const usize requested = __allocation_policy_capacity<P>(bytes);
    return allocation_round_up_or_throw(requested, alignment);
  }

public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return page_size;
  }

  [[nodiscard]] static usize
  allocation_extent(usize bytes, usize alignment)
  {
    return __capacity(bytes, alignment);
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( alignment > page_size ) exc<except::invalid_argument>("allocator_guarded: alignment exceeds the system-page floor");
    const usize capacity = __capacity(bytes, alignment);
    if ( capacity == 0 ) return { nullptr, 0 };

    const usize data_len = allocation_round_up_or_throw(capacity, page_size);
    const usize guards = allocation_multiply_or_throw(page_size, 2);
    const usize mapping_len = allocation_add_or_throw(data_len, guards);
    addr_t *base = micron::mmap(nullptr, mapping_len, prot_none, map_private | map_anonymous, -1, 0);
    if ( micron::mmap_failed(base) ) exc<except::memory_error>("allocator_guarded: mmap failed");

    addr_t *data = micron::ptr_cast<addr_t *>(reinterpret_cast<byte *>(base) + page_size);
    addr_t *trailing = micron::ptr_cast<addr_t *>(reinterpret_cast<byte *>(data) + data_len);
    if ( micron::mprotect(data, data_len, prot_read | prot_write) != 0 ) {
      micron::munmap(base, mapping_len);
      exc<except::memory_error>("allocator_guarded: mprotect failed");
    }

    byte *result = reinterpret_cast<byte *>(trailing) - capacity;
    if ( !abc::register_external(result, capacity, abc::external_provenance::guarded) ) {
      micron::munmap(base, mapping_len);
      exc<except::memory_error>("allocator_guarded: provenance registration failed");
    }
    return { result, capacity };
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create(bytes, 16);
  }

  [[nodiscard]] static chunk<byte>
  resize(chunk<byte> old, usize bytes, usize preserve_bytes, usize alignment)
  {
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
    if ( target == __allocation_max ) exc<except::length_error>("allocator_guarded: growth overflow");
    return resize(old, target, old.len, 16);
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    if ( memory.ptr == nullptr ) return;
    const uintptr_t ptr = reinterpret_cast<uintptr_t>(memory.ptr);
    const uintptr_t data_page = ptr & ~(static_cast<uintptr_t>(page_size) - 1);
    const uintptr_t base = data_page - page_size;
    const uintptr_t trailing = ptr + memory.len;
    const usize mapping_len = static_cast<usize>(trailing - base) + page_size;
    (void)abc::unregister_external(memory.ptr, memory.len);
    micron::munmap(reinterpret_cast<addr_t *>(base), mapping_len);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy(memory, 16);
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
