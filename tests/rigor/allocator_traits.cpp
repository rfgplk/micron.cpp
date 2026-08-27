//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ALLOCATOR_CHECKS 1

#include "../../src/allocator.hpp"

#include "../snowball/snowball.hpp"

namespace
{
struct canonical_probe {
  static inline usize allocations = 0;
  static inline usize deallocations = 0;

  static micron::chunk<byte>
  allocate(usize bytes, usize alignment)
  {
    ++allocations;
    return micron::allocator_exact<>::create(bytes, alignment);
  }

  static void
  deallocate(micron::chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) ++deallocations;
    micron::allocator_exact<>::destroy(memory, alignment);
  }

  static constexpr usize
  auto_size() noexcept
  {
    return 16;
  }
};

struct legacy_probe {
  static inline usize creates = 0;
  static inline usize grows = 0;
  static inline usize destroys = 0;

  static micron::chunk<byte>
  create(usize bytes, usize alignment)
  {
    ++creates;
    return micron::allocator_exact<>::create(bytes, alignment);
  }

  static micron::chunk<byte>
  grow(micron::chunk<byte> old, usize bytes)
  {
    ++grows;
    micron::chunk<byte> next = create(bytes, 64);
    micron::memcpy(next.ptr, old.ptr, micron::min(old.len, next.len));
    destroy(old, 64);
    return next;
  }

  static void
  destroy(micron::chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) ++destroys;
    micron::allocator_exact<>::destroy(memory, alignment);
  }

  static constexpr usize
  auto_size() noexcept
  {
    return 16;
  }
};

struct trusted_short_probe {
  static constexpr bool allocator_trusted = true;
  static inline usize destroys = 0;

  static micron::chunk<byte>
  create(usize bytes, usize alignment)
  {
    micron::chunk<byte> memory = micron::allocator_exact<>::create(bytes, alignment);
    return { memory.ptr, bytes ? bytes - 1 : 0 };
  }

  static void
  destroy(micron::chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) ++destroys;
    micron::allocator_exact<>::destroy(memory, alignment);
  }
};

struct sized_pointer_probe {
  static inline usize chunk_destroys = 0;
  static inline usize pointer_destroys = 0;

  static micron::chunk<byte>
  create(usize bytes, usize alignment)
  {
    return micron::map_allocator<micron::exact_allocation_policy>::create(bytes, alignment);
  }

  static void
  destroy(micron::chunk<byte> memory, usize alignment) noexcept
  {
    ++chunk_destroys;
    micron::map_allocator<micron::exact_allocation_policy>::destroy(memory, alignment);
  }

  static void
  destroy(byte *memory, usize bytes) noexcept
  {
    ++pointer_destroys;
    micron::munmap(reinterpret_cast<addr_t *>(memory), bytes);
  }
};

static_assert(!micron::allocator_traits<sized_pointer_probe>::has_unsized_deallocate<alignof(u64)>);
}      // namespace

int
main()
{
  sb::print("=== ALLOCATOR TRAITS RIGOR ===");

  sb::test_case("canonical allocate/deallocate normalizes through allocator_traits");
  {
    micron::chunk<byte> memory = micron::allocator_traits<canonical_probe>::allocate<64>(37);
    sb::require(reinterpret_cast<uintptr_t>(memory.ptr) & 63u, uintptr_t{ 0 });
    micron::allocator_traits<canonical_probe>::deallocate<64>(memory);
    sb::require(canonical_probe::allocations, usize{ 1 });
    sb::require(canonical_probe::deallocations, usize{ 1 });
  }
  sb::end_test_case();

  sb::test_case("legacy create/grow/destroy remains source compatible");
  {
    micron::chunk<byte> memory = micron::allocator_traits<legacy_probe>::allocate<64>(32);
    for ( usize i = 0; i < memory.len; ++i ) memory.ptr[i] = static_cast<byte>(i + 3);
    memory = micron::allocator_traits<legacy_probe>::resize<64>(memory, 96, 32);
    for ( usize i = 0; i < 32; ++i ) sb::require(memory.ptr[i], static_cast<byte>(i + 3));
    micron::allocator_traits<legacy_probe>::deallocate<64>(memory);
    sb::require(legacy_probe::grows, usize{ 1 });
    sb::require(legacy_probe::creates, usize{ 2 });
    sb::require(legacy_probe::destroys, usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("MICRON_ALLOCATOR_CHECKS validates even trusted built-ins");
  {
    bool rejected = false;
    try {
      (void)micron::allocator_traits<trusted_short_probe>::allocate<16>(32);
    } catch ( const micron::except::memory_error & ) {
      rejected = true;
    }
    sb::require_true(rejected);
    sb::require(trusted_short_probe::destroys, usize{ 1 });
  }
  sb::end_test_case();

  sb::test_case("a legacy pointer-plus-size destroy is not mistaken for unsized destruction");
  {
    sized_pointer_probe::chunk_destroys = 0;
    sized_pointer_probe::pointer_destroys = 0;
    u64 *memory = micron::__allocator_allocate_array<sized_pointer_probe, u64>(17);
    memory[16] = 0xfeed'faceu;
    sb::require(memory[16], u64{ 0xfeed'faceu });
    micron::__allocator_deallocate_array<sized_pointer_probe>(memory);
    sb::require(sized_pointer_probe::chunk_destroys, usize{ 1 });
    sb::require(sized_pointer_probe::pointer_destroys, usize{ 0 });
  }
  sb::end_test_case();

  sb::print("=== ALL ALLOCATOR TRAITS RIGOR PASSED ===");
  return 1;
}
