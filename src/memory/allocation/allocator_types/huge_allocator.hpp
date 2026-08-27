//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_huge
//
// use for large, hot regions that benefit from explicitly reserved hugetlb pages

namespace micron
{

template<i32 HugePageFlag = map_huge_2mb, is_policy P = huge_allocation_policy> class allocator_huge
{
  static constexpr usize
  __huge_size() noexcept
  {
    constexpr u32 shift = static_cast<u32>(HugePageFlag >> map_huge_shift) & static_cast<u32>(map_huge_mask);
    static_assert(shift != 0, "allocator_huge: an explicit MAP_HUGE_* flag is required");
    static_assert(shift < sizeof(usize) * 8, "allocator_huge: huge-page size is not representable on this target");
    return static_cast<usize>(1) << shift;
  }

  static usize
  __capacity(usize bytes)
  {
    usize requested = __allocation_policy_capacity<P>(bytes);
    if ( requested < __huge_size() ) requested = __huge_size();
    return allocation_round_up_or_throw(requested, __huge_size());
  }

public:
  static constexpr bool allocator_trusted = true;
  static constexpr i32 huge_page_flag = HugePageFlag;

  [[nodiscard]] static constexpr usize
  auto_size() noexcept
  {
    return __huge_size();
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes, usize alignment)
  {
    allocation_validate_alignment(alignment);
    if ( alignment > __huge_size() ) exc<except::invalid_argument>("allocator_huge: alignment exceeds the selected huge-page size");
    const usize capacity = __capacity(bytes);
    addr_t *memory
        = micron::mmap(nullptr, capacity, prot_read | prot_write, map_private | map_anonymous | map_hugetlb | HugePageFlag, -1, 0);
    if ( micron::mmap_failed(memory) ) exc<except::memory_error>("allocator_huge: strict huge-page mapping failed");
    if ( !abc::register_external(reinterpret_cast<byte *>(memory), capacity, abc::external_provenance::huge) ) {
      micron::munmap(memory, capacity);
      exc<except::memory_error>("allocator_huge: provenance registration failed");
    }
    return { reinterpret_cast<byte *>(memory), capacity };
  }

  [[nodiscard]] static chunk<byte>
  create(usize bytes)
  {
    return create(bytes, __huge_size());
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
    if ( target == __allocation_max ) exc<except::length_error>("allocator_huge: growth overflow");
    return resize(old, target, old.len, __huge_size());
  }

  static void
  destroy(chunk<byte> memory, usize) noexcept
  {
    if ( memory.ptr ) {
      (void)abc::unregister_external(memory.ptr, memory.len);
      micron::munmap(reinterpret_cast<addr_t *>(memory.ptr), memory.len);
    }
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    destroy(memory, __huge_size());
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
