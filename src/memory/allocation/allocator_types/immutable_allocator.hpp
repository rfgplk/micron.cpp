//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_immutable
//
// use for data built once and read thereafter, such as tables or decoded metadata;
// storage is a private, page-rounded mapping returned with requested logical length;
// seal verifies allocator provenance and changes the entire mapping to read-only

namespace micron
{

class allocator_immutable
{
  static usize
  __mapping_len(usize bytes)
  {
    return allocation_round_up_or_throw(bytes, page_size);
  }

public:
  static constexpr bool allocator_trusted = true;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return page_size;
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
    chunk<byte> mapping = __map_create_aligned(__mapping_len(bytes), alignment);
    if ( !abc::register_external(mapping.ptr, mapping.len, abc::external_provenance::immutable) ) {
      micron::munmap(micron::ptr_cast<addr_t *>(mapping.ptr), mapping.len);
      exc<except::memory_error>("allocator_immutable: provenance registration failed");
    }
    return { mapping.ptr, bytes };
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
    if ( copied ) micron::memcpy(next.ptr, old.ptr, copied);
    destroy(old, alignment);
    return next;
  }

  [[nodiscard]] static chunk<byte>
  grow(chunk<byte> old, usize minimum)
  {
    const usize target = recommend(old.len, minimum);
    if ( target == __allocation_max ) exc<except::length_error>("allocator_immutable: growth overflow");
    return resize(old, target, old.len, 16);
  }

  [[nodiscard]] static bool
  seal(chunk<byte> memory) noexcept
  {
    if ( memory.ptr == nullptr || memory.len == 0 ) return false;
    usize mapping_len;
    if ( !allocation_checked_round_up(memory.len, page_size, mapping_len) ) return false;
    if ( abc::external_query_provenance(memory.ptr) != abc::external_provenance::immutable ) return false;
    if ( abc::external_query_size(memory.ptr) != mapping_len ) return false;
    return micron::mprotect(micron::ptr_cast<addr_t *>(memory.ptr), mapping_len, prot_read) == 0;
  }

  [[nodiscard]] static bool
  seal(byte *memory, usize bytes) noexcept
  {
    return seal({ memory, bytes });
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    if ( memory.ptr == nullptr || memory.len == 0 ) return;
    usize mapping_len;
    if ( !allocation_checked_round_up(memory.len, page_size, mapping_len) ) return;
    (void)abc::unregister_external(memory.ptr, mapping_len);
    micron::munmap(micron::ptr_cast<addr_t *>(memory.ptr), mapping_len);
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy(memory, 16);
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
};

};      // namespace micron
