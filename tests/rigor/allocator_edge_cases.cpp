//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ALLOCATOR_CHECKS 1

#include "../../src/allocator.hpp"
#include "../../src/memory/allocation/resources.hpp"

#include "../snowball/snowball.hpp"

namespace
{
struct prng {
  u64 state;

  explicit prng(u64 seed) noexcept : state(seed) { }

  u64
  next() noexcept
  {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
};

byte
pattern(u64 key, usize index) noexcept
{
  u64 value = key + static_cast<u64>(index) * 0x9e3779b97f4a7c15ULL;
  value ^= value >> 29;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 31;
  return static_cast<byte>(value);
}

void
fill(micron::chunk<byte> memory, u64 key) noexcept
{
  for ( usize i = 0; i < memory.len; ++i ) memory.ptr[i] = pattern(key, i);
}

void
require_pattern(micron::chunk<byte> memory, usize count, u64 key)
{
  sb::require_true(count <= memory.len);
  for ( usize i = 0; i < count; ++i ) sb::require(memory.ptr[i], pattern(key, i));
}

template<typename Alloc, typename Policy>
void
capacity_alignment_sweep()
{
  constexpr usize sizes[] = { 0, 1, 2, 15, 16, 17, 31, 63, 64, 65, 255, 256, 257, 4095, 4096, 4097 };
  for ( usize alignment = 1; alignment <= 256; alignment <<= 1 ) {
    for ( usize bytes : sizes ) {
      micron::chunk<byte> memory = Alloc::create(bytes, alignment);
      const usize expected = micron::__allocation_policy_capacity<Policy>(bytes);
      sb::require(memory.len, expected);
      sb::require(micron::allocator_traits<Alloc>::allocation_extent(bytes, alignment), memory.len);
      if ( bytes == 0 ) {
        sb::require(memory.ptr, static_cast<byte *>(nullptr));
      } else {
        sb::require_true(memory.ptr != nullptr);
        sb::require(reinterpret_cast<uintptr_t>(memory.ptr) & (alignment - 1), uintptr_t{ 0 });
        fill(memory, static_cast<u64>(bytes) ^ static_cast<u64>(alignment));
        require_pattern(memory, memory.len, static_cast<u64>(bytes) ^ static_cast<u64>(alignment));
      }
      Alloc::destroy(memory, alignment);
    }
  }
}

template<typename Alloc, typename Policy, bool PageRounded = false>
void
resize_matrix()
{
  constexpr usize sources[] = { 1, 15, 256, 4095, 4097 };
  constexpr usize targets[] = { 0, 1, 16, 255, 4096, 8193 };
  for ( usize source : sources ) {
    for ( usize target : targets ) {
      for ( usize preserve_case = 0; preserve_case < 4; ++preserve_case ) {
        const usize alignment = ((source + target + preserve_case) & 1u) ? 16u : 256u;
        micron::chunk<byte> memory = Alloc::create(source, alignment);
        const u64 key = 0xd6e8feb86659fd93ULL ^ source ^ (static_cast<u64>(target) << 17) ^ preserve_case;
        fill(memory, key);
        usize preserve = 0;
        if ( preserve_case == 1 ) preserve = memory.len / 2;
        if ( preserve_case == 2 ) preserve = memory.len;
        if ( preserve_case == 3 ) preserve = memory.len + 97;
        const usize old_len = memory.len;
        micron::chunk<byte> next = Alloc::resize(memory, target, preserve, alignment);
        usize expected = micron::__allocation_policy_capacity<Policy>(target);
        if constexpr ( PageRounded ) expected = micron::allocation_round_up_or_throw(expected, micron::page_size);
        sb::require(next.len, expected);
        if ( target == 0 ) {
          sb::require(next.ptr, static_cast<byte *>(nullptr));
        } else {
          sb::require_true(next.ptr != nullptr);
          sb::require(reinterpret_cast<uintptr_t>(next.ptr) & (alignment - 1), uintptr_t{ 0 });
          require_pattern(next, micron::min(preserve, old_len, next.len), key);
        }
        Alloc::destroy(next, alignment);
      }
    }
  }
}

struct alignas(64) probe_storage {
  byte data[512];
};

struct null_probe {
  static inline usize destroys = 0;

  static micron::chunk<byte>
  create(usize, usize)
  {
    return { nullptr, 0 };
  }

  static void
  destroy(micron::chunk<byte>, usize) noexcept
  {
    ++destroys;
  }
};

struct short_probe {
  alignas(64) inline static probe_storage storage{};
  static inline usize destroys = 0;

  static micron::chunk<byte>
  create(usize bytes, usize)
  {
    return { storage.data, bytes ? bytes - 1 : 0 };
  }

  static void
  destroy(micron::chunk<byte>, usize) noexcept
  {
    ++destroys;
  }
};

struct unaligned_probe {
  alignas(64) inline static probe_storage storage{};
  static inline usize destroys = 0;

  static micron::chunk<byte>
  create(usize bytes, usize)
  {
    return { storage.data + 1, bytes };
  }

  static void
  destroy(micron::chunk<byte>, usize) noexcept
  {
    ++destroys;
  }
};

struct fallback_probe {
  static inline usize allocations = 0;
  static inline usize deallocations = 0;
  static inline bool fail = false;

  static micron::chunk<byte>
  allocate(usize bytes, usize alignment)
  {
    if ( fail ) micron::exc<micron::except::memory_error>("fallback probe");
    micron::chunk<byte> memory = micron::map_allocator<micron::exact_allocation_policy>::create(bytes, alignment);
    if ( memory.ptr ) ++allocations;
    return memory;
  }

  static void
  deallocate(micron::chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) ++deallocations;
    micron::map_allocator<micron::exact_allocation_policy>::destroy(memory, alignment);
  }
};

struct unstable_extent_probe {
  static inline bool misreport = false;
  static inline usize deallocations = 0;

  static constexpr usize
  auto_size() noexcept
  {
    return 64;
  }

  static usize
  allocation_extent(usize bytes, usize) noexcept
  {
    return misreport && bytes ? bytes + 1 : bytes;
  }

  static micron::chunk<byte>
  create(usize bytes, usize alignment)
  {
    return micron::allocator_exact<>::create(bytes, alignment);
  }

  static micron::chunk<byte>
  resize(micron::chunk<byte> memory, usize bytes, usize preserve, usize alignment)
  {
    return micron::allocator_exact<>::resize(memory, bytes, preserve, alignment);
  }

  static void
  destroy(micron::chunk<byte> memory, usize alignment) noexcept
  {
    if ( memory.ptr ) ++deallocations;
    micron::allocator_exact<>::destroy(memory, alignment);
  }
};

struct odd_record {
  byte bytes[37];
};
}      // namespace

int
main()
{
  sb::print("=== ALLOCATOR EDGE CASES ===");

  sb::test_case("checked arithmetic agrees with bounded integer oracles");
  {
    constexpr u64 seed = 0x8f3f73b5cf1c9adeULL;
    prng random(seed);
    for ( usize i = 0; i < 50000; ++i ) {
      const usize a = static_cast<usize>(random.next());
      const usize b = static_cast<usize>(random.next());
      usize result = 0;
      const bool add_ok = b <= micron::__allocation_max - a;
      sb::require(micron::allocation_checked_add(a, b, result), add_ok);
      if ( add_ok ) sb::require(result, static_cast<usize>(a + b));

      const bool multiply_ok = a == 0 || b <= micron::__allocation_max / a;
      sb::require(micron::allocation_checked_multiply(a, b, result), multiply_ok);
      if ( multiply_ok ) sb::require(result, static_cast<usize>(a * b));

      const usize granularity = static_cast<usize>(random.next() & 1023u);
      const usize remainder = granularity ? a % granularity : 0;
      const bool round_ok = granularity != 0 && (remainder == 0 || granularity - remainder <= micron::__allocation_max - a);
      sb::require(micron::allocation_checked_round_up(a, granularity, result), round_ok);
      if ( round_ok ) sb::require(result, remainder ? a + granularity - remainder : a);

      const usize current = static_cast<usize>(random.next() % 1'000'000u);
      const usize minimum = static_cast<usize>(random.next() % 2'000'000u);
      const usize denominator = static_cast<usize>(random.next() % 8u) + 1;
      const usize numerator = denominator + static_cast<usize>(random.next() % (9u - denominator));
      sb::require_true(micron::allocation_checked_growth(current, minimum, numerator, denominator, result));
      const usize product = current * numerator;
      const usize scaled = product / denominator + (product % denominator != 0);
      sb::require(result, scaled < minimum ? minimum : scaled);
    }

    usize result = 0;
    sb::require_true(micron::allocation_checked_add(micron::__allocation_max, 0, result));
    sb::require(result, micron::__allocation_max);
    sb::require_false(micron::allocation_checked_add(micron::__allocation_max, 1, result));
    sb::require_true(micron::allocation_checked_multiply(micron::__allocation_max, 1, result));
    sb::require_false(micron::allocation_checked_multiply(micron::__allocation_max, 2, result));
    sb::require_true(micron::allocation_checked_round_up(micron::__allocation_max, 1, result));
    sb::require_false(micron::allocation_checked_round_up(micron::__allocation_max, 2, result));
    sb::require_true(micron::allocation_checked_growth(micron::__allocation_max, 0, 1, 1, result));
    sb::require_false(micron::allocation_checked_growth(micron::__allocation_max, 0, 3, 2, result));
  }
  sb::end_test_case();

  sb::test_case("policy floors, rounding, recommendations, and overflow boundaries");
  {
    using policy = micron::allocation_policy<32, 16, 3, 2>;
    constexpr usize requests[] = { 0, 1, 15, 16, 31, 32, 33, 47, 48, 49, 63, 64, 65 };
    constexpr usize expected[] = { 0, 32, 32, 32, 32, 32, 48, 48, 48, 64, 64, 64, 80 };
    for ( usize i = 0; i < sizeof(requests) / sizeof(requests[0]); ++i )
      sb::require(micron::__allocation_policy_capacity<policy>(requests[i]), expected[i]);

    for ( usize current = 0; current <= 4096; current += 7 ) {
      for ( usize minimum = 0; minimum <= 4096; minimum += 31 ) {
        usize scaled = current + current / 2 + (current & 1u);
        if ( scaled < minimum ) scaled = minimum;
        if ( scaled < 32 ) scaled = 32;
        const usize rounded = scaled + ((16 - scaled % 16) % 16);
        sb::require(micron::__allocation_policy_recommend<policy>(current, minimum), rounded);
      }
    }

    bool overflowed = false;
    try {
      (void)micron::__allocation_policy_capacity<policy>(micron::__allocation_max);
    } catch ( const micron::except::length_error & ) {
      overflowed = true;
    }
    sb::require_true(overflowed);
    sb::require(micron::__allocation_policy_recommend<policy>(micron::__allocation_max, 0), micron::__allocation_max);
  }
  sb::end_test_case();

  sb::test_case("policy allocators honor every boundary size and alignment tier");
  {
    capacity_alignment_sweep<micron::allocator_serial<>, micron::serial_allocation_policy>();
    capacity_alignment_sweep<micron::allocator_small<>, micron::small_allocation_policy>();
    capacity_alignment_sweep<micron::allocator_constrained<>, micron::constrained_allocation_policy>();
    capacity_alignment_sweep<micron::allocator_exact<>, micron::exact_allocation_policy>();
  }
  sb::end_test_case();

  sb::test_case("resize matrices preserve exactly the promised prefix");
  {
    resize_matrix<micron::allocator_serial<>, micron::serial_allocation_policy>();
    resize_matrix<micron::allocator_small<>, micron::small_allocation_policy>();
    resize_matrix<micron::allocator_constrained<>, micron::constrained_allocation_policy>();
    resize_matrix<micron::allocator_exact<>, micron::exact_allocation_policy>();
    resize_matrix<micron::map_allocator<micron::exact_allocation_policy>, micron::exact_allocation_policy, true>();
  }
  sb::end_test_case();

  sb::test_case("traits reject null, short, and under-aligned allocator results");
  {
    bool rejected_null = false;
    try {
      (void)micron::allocator_traits<null_probe>::allocate(32, 16);
    } catch ( const micron::except::memory_error & ) {
      rejected_null = true;
    }
    sb::require_true(rejected_null);
    sb::require(null_probe::destroys, usize{ 0 });

    bool rejected_short = false;
    try {
      (void)micron::allocator_traits<short_probe>::allocate(32, 16);
    } catch ( const micron::except::memory_error & ) {
      rejected_short = true;
    }
    sb::require_true(rejected_short);
    sb::require(short_probe::destroys, usize{ 1 });

    bool rejected_alignment = false;
    try {
      (void)micron::allocator_traits<unaligned_probe>::allocate(32, 16);
    } catch ( const micron::except::memory_error_core_unaligned & ) {
      rejected_alignment = true;
    }
    sb::require_true(rejected_alignment);
    sb::require(unaligned_probe::destroys, usize{ 1 });
  }
  sb::end_test_case();

  sb::test_case("fallback resize is prefix-safe and allocation failure leaves the source owned");
  {
    fallback_probe::allocations = 0;
    fallback_probe::deallocations = 0;
    fallback_probe::fail = false;
    micron::chunk<byte> memory = micron::allocator_traits<fallback_probe>::allocate(127, 64);
    fill(memory, 0xa0761d6478bd642fULL);
    memory = micron::allocator_traits<fallback_probe>::resize(memory, 513, 79, 64);
    require_pattern(memory, 79, 0xa0761d6478bd642fULL);
    sb::require(fallback_probe::allocations, usize{ 2 });
    sb::require(fallback_probe::deallocations, usize{ 1 });

    fill(memory, 0xe7037ed1a0b428dbULL);
    fallback_probe::fail = true;
    bool failed = false;
    try {
      (void)micron::allocator_traits<fallback_probe>::resize(memory, 1024, memory.len, 64);
    } catch ( const micron::except::memory_error & ) {
      failed = true;
    }
    fallback_probe::fail = false;
    sb::require_true(failed);
    require_pattern(memory, memory.len, 0xe7037ed1a0b428dbULL);
    sb::require(fallback_probe::deallocations, usize{ 1 });
    micron::allocator_traits<fallback_probe>::deallocate(memory, 64);
    sb::require(fallback_probe::deallocations, usize{ 2 });
  }
  sb::end_test_case();

  sb::test_case("owned resources fail closed when an allocator changes its extent contract");
  {
    using resource = micron::__mutable_memory_resource<odd_record, unstable_extent_probe>;
    unstable_extent_probe::misreport = false;
    unstable_extent_probe::deallocations = 0;
    resource memory(2);
    memory.length = 2;

    unstable_extent_probe::misreport = true;
    bool rejected = false;
    try {
      memory.realloc(3);
    } catch ( const micron::except::memory_error & ) {
      rejected = true;
    }
    sb::require_true(rejected);
    sb::require(memory.memory, static_cast<odd_record *>(nullptr));
    sb::require(memory.capacity, usize{ 0 });
    sb::require(memory.length, usize{ 0 });
    sb::require(unstable_extent_probe::deallocations, usize{ 1 });
    unstable_extent_probe::misreport = false;
  }
  sb::require(unstable_extent_probe::deallocations, usize{ 1 });
  sb::end_test_case();

  sb::test_case("sized array headers survive zero, over-alignment, and overflow edges");
  {
    fallback_probe::allocations = 0;
    fallback_probe::deallocations = 0;
    odd_record *zero = micron::__allocator_allocate_array<fallback_probe, odd_record>(0, 256);
    sb::require_true(zero != nullptr);
    sb::require(reinterpret_cast<uintptr_t>(zero) & 255u, uintptr_t{ 0 });
    micron::__allocator_deallocate_array<fallback_probe>(zero);

    odd_record *records = micron::__allocator_allocate_array<fallback_probe, odd_record>(37, 512);
    sb::require(reinterpret_cast<uintptr_t>(records) & 511u, uintptr_t{ 0 });
    for ( usize i = 0; i < 37; ++i )
      for ( usize j = 0; j < sizeof(odd_record); ++j ) records[i].bytes[j] = pattern(i, j);
    for ( usize i = 0; i < 37; ++i )
      for ( usize j = 0; j < sizeof(odd_record); ++j ) sb::require(records[i].bytes[j], pattern(i, j));
    micron::__allocator_deallocate_array<fallback_probe>(records);

    const usize calls = fallback_probe::allocations;
    bool overflowed = false;
    try {
      (void)micron::__allocator_allocate_array<fallback_probe, odd_record>(micron::__allocation_max);
    } catch ( const micron::except::length_error & ) {
      overflowed = true;
    }
    sb::require_true(overflowed);
    sb::require(fallback_probe::allocations, calls);
    sb::require(fallback_probe::allocations, fallback_probe::deallocations);
  }
  sb::end_test_case();

  sb::print("=== ALL ALLOCATOR EDGE CASES PASSED ===");
  return 1;
}
