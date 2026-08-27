//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/allocator.hpp"

namespace
{
struct static_tag;
struct monotonic_tag;

inline micron::arena_resource<micron::allocator_exact<>> compile_arena{ 4096 };

template<class A>
concept canonical_allocator = requires(micron::chunk<byte> old, usize bytes) {
  { A::create(bytes, usize{ 64 }) } -> micron::same_as<micron::chunk<byte>>;
  { A::resize(old, bytes, bytes, usize{ 64 }) } -> micron::same_as<micron::chunk<byte>>;
  A::destroy(old, usize{ 64 });
  { A::recommend(bytes, bytes) } -> micron::same_as<usize>;
  { A::create(bytes) } -> micron::same_as<micron::chunk<byte>>;
  { A::grow(old, bytes) } -> micron::same_as<micron::chunk<byte>>;
  A::destroy(old);
  { A::auto_size() } -> micron::same_as<usize>;
  { A::get_grow() };
};

using serial_t = micron::allocator_serial<>;
using small_t = micron::allocator_small<>;
using constrained_t = micron::allocator_constrained<>;
using exact_t = micron::allocator_exact<>;
using map_t = micron::map_allocator<>;
using huge_t = micron::allocator_huge<micron::map_huge_2mb>;
using secure_t = micron::allocator_secure<>;
using guarded_t = micron::allocator_guarded<>;
using immutable_t = micron::allocator_immutable;
using retiring_t = micron::allocator_retiring<>;
using static_t = micron::allocator_static<static_tag, 16384, 64>;
using monotonic_t = micron::allocator_monotonic<monotonic_tag, 4096, exact_t>;
using arena_t = micron::arena_allocator<compile_arena>;

static_assert(canonical_allocator<serial_t>);
static_assert(canonical_allocator<small_t>);
static_assert(canonical_allocator<constrained_t>);
static_assert(canonical_allocator<exact_t>);
static_assert(canonical_allocator<map_t>);
static_assert(canonical_allocator<huge_t>);
static_assert(canonical_allocator<secure_t>);
static_assert(canonical_allocator<guarded_t>);
static_assert(canonical_allocator<immutable_t>);
static_assert(canonical_allocator<retiring_t>);
static_assert(canonical_allocator<static_t>);
static_assert(canonical_allocator<monotonic_t>);
static_assert(canonical_allocator<arena_t>);
static_assert(!canonical_allocator<micron::allocator_temporal>);
static_assert(!canonical_allocator<micron::fixed_map_allocator>);
#if defined(MICRON_ABC_PERSISTENT)
static_assert(canonical_allocator<micron::allocator_persistent<>>);
#endif

static_assert(sizeof(serial_t) == 1);
static_assert(sizeof(map_t) == 1);
static_assert(sizeof(static_t) == 1);
static_assert(sizeof(monotonic_t) == 1);

template<class A>
usize
exercise()
{
  micron::chunk<byte> memory = A::create(73, 64);
  memory = A::resize(memory, 193, 73, 64);
  const usize capacity = memory.len;
  A::destroy(memory, 64);
  A::destroy({ nullptr, 0 }, 64);
  return capacity + A::recommend(73, 74);
}
}      // namespace

int
main()
{
  usize value = 0;
  value += exercise<serial_t>();
  value += exercise<small_t>();
  value += exercise<constrained_t>();
  value += exercise<exact_t>();
  value += exercise<map_t>();
  value += exercise<huge_t>();
  value += exercise<secure_t>();
  value += exercise<guarded_t>();
  value += exercise<immutable_t>();
  value += exercise<retiring_t>();
#if defined(MICRON_ABC_PERSISTENT)
  value += exercise<micron::allocator_persistent<>>();
#endif
  value += exercise<static_t>();
  value += exercise<monotonic_t>();
  value += exercise<arena_t>();

  byte *legacy = micron::abc_allocator<byte>::brk_allocate(8);
  micron::abc_allocator<byte>::brk_deallocate(legacy, 8);
  static_t::reset();
  monotonic_t::reset();
  monotonic_t::release();
  compile_arena.release();
  return static_cast<int>(value & 0x7f);
}
