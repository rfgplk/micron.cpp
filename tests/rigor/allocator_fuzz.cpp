//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ALLOCATOR_CHECKS 1

#include "../../src/allocator.hpp"
#include "../../src/memory/allocation/resources.hpp"

#include "../snowball/snowball_fuzz.hpp"

namespace
{
struct prng {
  sbf::rng state;

  explicit prng(u64 seed) noexcept : state(sbf::rng::from_seed(seed)) { }

  u64
  next() noexcept
  {
    return state.next();
  }

  usize
  below(usize limit) noexcept
  {
    return static_cast<usize>(sbf::__below(state, limit));
  }
};

[[noreturn]] void
fuzz_failure(u64 seed, usize iteration, const char *message)
{
  micron::io::print("allocator fuzz failure: seed=", seed, " iteration=", iteration, " invariant=", message, '\n');
  sb::error(message);
}

void
insist(bool condition, u64 seed, usize iteration, const char *message)
{
  if ( !condition ) fuzz_failure(seed, iteration, message);
}

byte
pattern(u64 key, usize index) noexcept
{
  u64 value = key ^ (static_cast<u64>(index) + 0x9e3779b97f4a7c15ULL);
  value ^= value >> 30;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27;
  value *= 0x94d049bb133111ebULL;
  value ^= value >> 31;
  return static_cast<byte>(value);
}

void
fill(micron::chunk<byte> memory, u64 key) noexcept
{
  for ( usize i = 0; i < memory.len; ++i ) memory.ptr[i] = pattern(key, i);
}

void
verify_prefix(micron::chunk<byte> memory, usize count, u64 key, u64 seed, usize iteration)
{
  insist(count <= memory.len, seed, iteration, "prefix exceeds allocation");
  for ( usize i = 0; i < count; ++i )
    if ( memory.ptr[i] != pattern(key, i) ) fuzz_failure(seed, iteration, "preserved byte changed");
}

usize
boundary_request(prng &random, usize alignment, usize maximum) noexcept
{
  switch ( random.below(16) ) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return alignment - 1;
  case 3:
    return alignment;
  case 4:
    return alignment + 1;
  case 5:
    return micron::page_size - 1;
  case 6:
    return micron::page_size;
  case 7:
    return micron::page_size + 1;
  case 8:
    return micron::page_size * 2 - 1;
  case 9:
    return micron::page_size * 2;
  case 10:
    return micron::page_size * 2 + 1;
  default:
    return random.below(maximum + 1);
  }
}

struct raw_slot {
  micron::chunk<byte> memory{};
  usize alignment = 1;
  u64 key = 0;
};

void
verify_slot(const raw_slot &slot, u64 seed, usize iteration)
{
  if ( slot.memory.ptr == nullptr ) {
    insist(slot.memory.len == 0, seed, iteration, "null allocation has non-zero length");
    return;
  }
  insist(slot.memory.len != 0, seed, iteration, "live allocation has zero length");
  insist((reinterpret_cast<uintptr_t>(slot.memory.ptr) & (slot.alignment - 1)) == 0, seed, iteration, "allocation is under-aligned");
  verify_prefix(slot.memory, slot.memory.len, slot.key, seed, iteration);
}

template<usize SlotCount>
void
verify_slots(const raw_slot (&slots)[SlotCount], u64 seed, usize iteration)
{
  for ( usize i = 0; i < SlotCount; ++i ) verify_slot(slots[i], seed, iteration);
  for ( usize i = 0; i < SlotCount; ++i ) {
    if ( slots[i].memory.ptr == nullptr ) continue;
    const uintptr_t first_begin = reinterpret_cast<uintptr_t>(slots[i].memory.ptr);
    const uintptr_t first_end = first_begin + slots[i].memory.len;
    for ( usize j = i + 1; j < SlotCount; ++j ) {
      if ( slots[j].memory.ptr == nullptr ) continue;
      const uintptr_t second_begin = reinterpret_cast<uintptr_t>(slots[j].memory.ptr);
      const uintptr_t second_end = second_begin + slots[j].memory.len;
      insist(first_end <= second_begin || second_end <= first_begin, seed, iteration, "live allocations overlap");
    }
  }
}

template<typename Alloc, usize SlotCount = 16>
void
fuzz_allocator(u64 seed, usize iterations, usize maximum_request)
{
  raw_slot slots[SlotCount]{};
  prng random(seed);

  for ( usize iteration = 0; iteration < iterations; ++iteration ) {
    const usize index = random.below(SlotCount);
    raw_slot &slot = slots[index];
    verify_slot(slot, seed, iteration);

    if ( slot.memory.ptr == nullptr ) {
      slot.alignment = usize{ 1 } << random.below(10);
      const usize requested = boundary_request(random, slot.alignment, maximum_request);
      slot.memory = micron::allocator_traits<Alloc>::allocate(requested, slot.alignment);
      if ( requested == 0 ) {
        insist(slot.memory.ptr == nullptr && slot.memory.len == 0, seed, iteration, "zero allocation is not canonical");
      } else {
        insist(slot.memory.ptr != nullptr, seed, iteration, "non-zero allocation returned null");
        insist(slot.memory.len >= requested, seed, iteration, "allocation is shorter than requested");
        insist(micron::allocator_traits<Alloc>::allocation_extent(requested, slot.alignment) == slot.memory.len, seed, iteration,
               "allocator extent disagrees with allocation result");
        insist((reinterpret_cast<uintptr_t>(slot.memory.ptr) & (slot.alignment - 1)) == 0, seed, iteration,
               "new allocation is under-aligned");
        slot.key = random.next();
        fill(slot.memory, slot.key);
      }
    } else {
      const usize operation = random.below(100);
      if ( operation < 55 ) {
        const usize requested = boundary_request(random, slot.alignment, maximum_request);
        usize preserve = 0;
        switch ( random.below(6) ) {
        case 0:
          preserve = 0;
          break;
        case 1:
          preserve = 1;
          break;
        case 2:
          preserve = slot.memory.len / 2;
          break;
        case 3:
          preserve = slot.memory.len;
          break;
        case 4:
          preserve = slot.memory.len + 1;
          break;
        default:
          preserve = slot.memory.len + random.below(257);
          break;
        }
        const usize old_length = slot.memory.len;
        const u64 old_key = slot.key;
        micron::chunk<byte> next = micron::allocator_traits<Alloc>::resize(slot.memory, requested, preserve, slot.alignment);
        if ( requested == 0 ) {
          insist(next.ptr == nullptr && next.len == 0, seed, iteration, "zero resize is not canonical");
          slot = {};
        } else {
          insist(next.ptr != nullptr, seed, iteration, "non-zero resize returned null");
          insist(next.len >= requested, seed, iteration, "resized allocation is shorter than requested");
          insist(micron::allocator_traits<Alloc>::allocation_extent(requested, slot.alignment) == next.len, seed, iteration,
                 "allocator extent disagrees with resize result");
          insist((reinterpret_cast<uintptr_t>(next.ptr) & (slot.alignment - 1)) == 0, seed, iteration,
                 "resized allocation is under-aligned");
          verify_prefix(next, micron::min(preserve, old_length, next.len), old_key, seed, iteration);
          slot.memory = next;
          slot.key = random.next();
          fill(slot.memory, slot.key);
        }
      } else if ( operation < 78 ) {
        micron::allocator_traits<Alloc>::deallocate(slot.memory, slot.alignment);
        slot = {};
      }
    }

    if ( (iteration & 255u) == 0 ) verify_slots(slots, seed, iteration);
  }

  verify_slots(slots, seed, iterations);
  for ( raw_slot &slot : slots ) {
    micron::allocator_traits<Alloc>::deallocate(slot.memory, slot.alignment);
    slot = {};
  }
}

struct odd_record {
  byte bytes[37];
};

using resource_type = micron::__mutable_memory_resource<odd_record, micron::allocator_serial<>>;

struct resource_slot {
  resource_type resource;
  u64 key;

  resource_slot() noexcept : resource(nullptr), key(0) { }
};

void
verify_resource(const resource_slot &slot, u64 seed, usize iteration)
{
  const micron::chunk<byte> memory = slot.resource.data();
  const usize expected = micron::allocator_traits<micron::allocator_serial<>>::allocation_extent(
      slot.resource.capacity * sizeof(odd_record), alignof(odd_record));
  insist(memory.len == expected, seed, iteration, "resource failed to reconstruct its allocator byte count");
  insist(slot.resource.capacity == memory.len / sizeof(odd_record), seed, iteration, "resource capacity is not byte-derived");
  insist(slot.resource.length <= slot.resource.capacity, seed, iteration, "resource length exceeds capacity");
  if ( memory.ptr == nullptr ) {
    insist(memory.len == 0 && slot.resource.capacity == 0 && slot.resource.length == 0, seed, iteration, "empty resource is not canonical");
  } else {
    insist(memory.len != 0, seed, iteration, "live resource has zero allocation bytes");
    insist((reinterpret_cast<uintptr_t>(memory.ptr) & (alignof(odd_record) - 1)) == 0, seed, iteration, "resource is under-aligned");
    verify_prefix(memory, memory.len, slot.key, seed, iteration);
  }
}

usize
resource_request(prng &random) noexcept
{
  const usize page_elements = micron::page_size / sizeof(odd_record);
  switch ( random.below(10) ) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return page_elements ? page_elements - 1 : 0;
  case 3:
    return page_elements;
  case 4:
    return page_elements + 1;
  default:
    return random.below(page_elements * 3 + 17);
  }
}

void
refresh(resource_slot &slot, prng &random) noexcept
{
  if ( slot.resource.data().ptr == nullptr ) {
    slot.key = 0;
    return;
  }
  slot.key = random.next();
  fill(slot.resource.data(), slot.key);
}

void
fuzz_resources(u64 seed, usize iterations)
{
  constexpr usize slot_count = 8;
  resource_slot slots[slot_count];
  prng random(seed);

  for ( usize iteration = 0; iteration < iterations; ++iteration ) {
    for ( const resource_slot &slot : slots ) verify_resource(slot, seed, iteration);

    const usize index = random.below(slot_count);
    const usize operation = random.below(100);
    resource_slot &slot = slots[index];

    if ( operation < 35 ) {
      const usize elements = resource_request(random);
      const usize old_capacity = slot.resource.capacity;
      const usize old_length = slot.resource.length;
      const u64 old_key = slot.key;
      slot.resource.realloc(elements);
      const usize expected_length = elements == old_capacity ? old_length : micron::min(old_length, elements);
      insist(slot.resource.length == expected_length, seed, iteration, "resource realloc changed logical length incorrectly");
      verify_prefix(slot.resource.data(), expected_length * sizeof(odd_record), old_key, seed, iteration);
      refresh(slot, random);
    } else if ( operation < 55 ) {
      const usize elements = resource_request(random);
      const usize old_capacity = slot.resource.capacity;
      const usize old_length = slot.resource.length;
      const u64 old_key = slot.key;
      slot.resource.expand(elements);
      insist(slot.resource.length == old_length, seed, iteration, "resource expand changed logical length");
      if ( elements > old_capacity ) verify_prefix(slot.resource.data(), old_length * sizeof(odd_record), old_key, seed, iteration);
      refresh(slot, random);
    } else if ( operation < 67 ) {
      slot.resource.free();
      slot.key = 0;
    } else if ( operation < 79 ) {
      if ( slot.resource.capacity ) slot.resource.length = random.below(slot.resource.capacity + 1);
    } else if ( operation < 90 ) {
      const usize other = random.below(slot_count);
      slot.resource.swap(slots[other].resource);
      micron::swap(slot.key, slots[other].key);
    } else {
      const usize other = random.below(slot_count);
      if ( other == index ) {
        slot.resource = micron::move(slot.resource);
      } else {
        slot.resource = micron::move(slots[other].resource);
        slot.key = slots[other].key;
        slots[other].key = 0;
      }
    }
  }

  for ( usize i = 0; i < slot_count; ++i ) {
    verify_resource(slots[i], seed, iterations);
    slots[i].resource.free();
  }
}

struct oversized_record {
  byte symlink[micron::page_size + 1];
};

template<typename Alloc, usize SlotCount = 16>
void
check_allocator_property(const char *name, u64 seed, usize cases, usize operations, usize maximum_request)
{
  sbf::run_config config;
  config.seed = seed;
  config.count = cases;
  sbf::check_property(
      name, [operations, maximum_request](u64 case_seed) { fuzz_allocator<Alloc, SlotCount>(case_seed, operations, maximum_request); },
      config, sbf::range<u64>(1, 0x7fffffffffffffffULL));
}
}      // namespace

int
main()
{
  sb::print("=== ALLOCATOR STATE-MACHINE FUZZ ===");

  check_allocator_property<micron::allocator_serial<>>("serial allocator state machine", 0x6a09e667f3bcc909ULL, 48, 250, 16384);
  check_allocator_property<micron::allocator_small<>>("small allocator state machine", 0xbb67ae8584caa73bULL, 48, 250, 16384);
  check_allocator_property<micron::allocator_constrained<>>("constrained allocator state machine", 0x3c6ef372fe94f82bULL, 48, 250, 16384);
  check_allocator_property<micron::allocator_exact<>>("exact allocator state machine", 0xa54ff53a5f1d36f1ULL, 48, 250, 16384);
  check_allocator_property<micron::map_allocator<micron::exact_allocation_policy>, 8>("map allocator state machine", 0x510e527fade682d1ULL,
                                                                                      20, 125, 32768);
#if !defined(__micron_sanitizer_owns_heap)
  check_allocator_property<micron::allocator_guarded<micron::exact_allocation_policy>, 4>("guarded allocator state machine",
                                                                                          0x9b05688c2b3e6c1fULL, 10, 50, 8192);
#endif

  sbf::run_config resource_config;
  resource_config.seed = 0x1f83d9abfb41bd6bULL;
  resource_config.count = 40;
  sbf::check_property(
      "owned-resource state machine", [](u64 case_seed) { fuzz_resources(case_seed, 500); }, resource_config,
      sbf::range<u64>(1, 0x7fffffffffffffffULL));

  sb::test_case("oversized element retains its complete allocation span");
  {
    resource_type empty(nullptr);
    empty.realloc(0);
    sb::require(empty.data().ptr, static_cast<byte *>(nullptr));
    sb::require(empty.data().len, usize{ 0 });

    micron::__mutable_memory_resource<oversized_record, micron::allocator_serial<>> resource(nullptr);
    resource.realloc(1);
    sb::require_true(resource.memory != nullptr);
    sb::require(resource.capacity, usize{ 1 });
    sb::require_true(resource.data().len >= sizeof(oversized_record));
    sb::require(resource.data().len, micron::allocator_traits<micron::allocator_serial<>>::allocation_extent(sizeof(oversized_record),
                                                                                                             alignof(oversized_record)));
    resource.free();
    sb::require(resource.data().ptr, static_cast<byte *>(nullptr));
    sb::require(resource.data().len, usize{ 0 });
  }
  sb::end_test_case();

  sb::print("=== ALL ALLOCATOR FUZZ CASES PASSED ===");
  return 1;
}
