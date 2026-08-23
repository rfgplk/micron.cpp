//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/maps/b_map.hpp"
#include "../../src/maps/conmap.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/maps/hopscotch.hpp"
#include "../../src/maps/immutable.hpp"
#include "../../src/maps/itable.hpp"
#include "../../src/maps/pmap.hpp"
#include "../../src/maps/rb_map.hpp"
#include "../../src/maps/robin.hpp"
#include "../../src/maps/swiss.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

namespace
{

struct alignas(64) aligned_key {
  u64 value;

  bool
  operator==(const aligned_key &o) const
  {
    return value == o.value;
  }

  bool
  operator!=(const aligned_key &o) const
  {
    return value != o.value;
  }

  bool
  operator<(const aligned_key &o) const
  {
    return value < o.value;
  }

  bool
  operator>(const aligned_key &o) const
  {
    return value > o.value;
  }

  bool
  operator<=(const aligned_key &o) const
  {
    return value <= o.value;
  }

  bool
  operator>=(const aligned_key &o) const
  {
    return value >= o.value;
  }
};

struct alignas(64) aligned_value {
  u64 value;

  aligned_value() : value(0) { }

  explicit aligned_value(u64 v) : value(v) { }
};

template<usize N>
micron::pair<u64, u64>
same_home(usize home)
{
  u64 first = 0;
  for ( u64 key = 1; key < 100000; ++key ) {
    if ( static_cast<usize>(micron::hash<micron::hash64_t>(key)) % N != home ) continue;
    if ( first ) return { first, key };
    first = key;
  }
  return { 0, 0 };
}

u64
hop_mix(u64 h)
{
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

};      // namespace

int
main()
{
  using tracked_t = mtest::Tracked<71>;
  tracked_t::reset();

  sb::test_case("stack_swiss destroys values on erase, clear, and destruction");
  {
    tracked_t seed(7);
    {
      micron::stack_swiss_map<u64, tracked_t, 32> map;
      map.insert(1, seed);
      sb::require(tracked_t::live() == 2);
      sb::require(map.erase(1));
      sb::require(tracked_t::live() == 1);

      map.insert(2, seed);
      map.clear();
      sb::require(tracked_t::live() == 1);

      map.emplace(3, 9);
      sb::require(tracked_t::live() == 2);
    }
    sb::require(tracked_t::live() == 1);
  }
  sb::require(tracked_t::live() == 0);
  sb::end_test_case();

  sb::test_case("heap-backed maps honor over-aligned values");
  {
    micron::robin_map<u64, aligned_value> robin(64);
    robin.insert(1, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(robin.find(1)) & 63u) == 0u);

    micron::heap_swiss_map<u64, aligned_value> heap_swiss(64);
    heap_swiss.insert(1, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(heap_swiss.find(1)) & 63u) == 0u);

    micron::hopscotch_map<u64, aligned_value> hop(4096);
    hop.insert(1, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(hop.find(1)) & 63u) == 0u);

    micron::btree_map<u64, aligned_value> btree(16);
    btree.insert(1, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(btree.find(1)) & 63u) == 0u);

    micron::rb_map<u64, aligned_value> rb;
    rb.insert(u64{ 1 }, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(rb.find(1)) & 63u) == 0u);

    micron::stack_swiss_map<u64, aligned_value, 32> stack_swiss;
    stack_swiss.insert(1, aligned_value{ 1 });
    sb::require((reinterpret_cast<uintptr_t>(stack_swiss.find(1)) & 63u) == 0u);

    micron::conmap<u64, aligned_value> concurrent(4096);
    concurrent.insert(1, aligned_value{ 1 });
    bool aligned = false;
    concurrent.update(1, [&](aligned_value &v) { aligned = (reinterpret_cast<uintptr_t>(micron::addressof(v)) & 63u) == 0u; });
    sb::require(aligned);
  }
  sb::end_test_case();

  sb::test_case("immutable_map balances tracked values across shared versions");
  {
    using immutable_tracked_t = mtest::Tracked<76>;
    immutable_tracked_t::reset();
    {
      immutable_tracked_t seed(8);
      micron::immutable_map<int, immutable_tracked_t> empty;
      auto one = empty.insert(1, seed);
      auto two = one.insert(2, seed);
      auto shared = two;
      auto erased = two.erase(1);
      sb::require(one.find(1) != nullptr);
      sb::require(two.find(1) != nullptr && two.find(2) != nullptr);
      sb::require(shared.find(1) != nullptr && shared.find(2) != nullptr);
      sb::require(erased.find(1) == nullptr && erased.find(2) != nullptr);
    }
    sb::require(immutable_tracked_t::live() == 0);
  }
  sb::end_test_case();

  sb::test_case("immutable_map keeps source intact after a throwing node copy");
  {
    using throwing_t = mtest::Throwing<mtest::throw_on::copy_ctor, 77>;
    throwing_t::reset();
    throwing_t value(4);
    micron::immutable_map<int, throwing_t> source;
    throwing_t::arm(0);
    bool threw = false;
    try {
      auto unused = source.insert(1, value);
      (void)unused;
    } catch ( ... ) {
      threw = true;
    }
    throwing_t::disarm();
    sb::require(threw);
    sb::require(source.empty());
  }
  sb::end_test_case();

  sb::test_case("immutable_map honors over-aligned node members");
  {
    micron::immutable_map<aligned_key, u64> map;
    auto next = map.insert(aligned_key{ 3 }, 9);
    auto it = next.begin();
    sb::require((reinterpret_cast<uintptr_t>(micron::addressof(it.key())) & 63u) == 0u);
    sb::require(*next.find(aligned_key{ 3 }) == 9);
  }
  sb::end_test_case();

  sb::test_case("immutable_map traversal callback cannot leave threaded links");
  {
    micron::immutable_map<int, int> map;
    for ( int i = 0; i < 32; ++i ) map = map.insert(i, i * 2);
    bool threw = false;
    try {
      map.for_each_morris([](const int &key, const int &) {
        if ( key == 7 ) throw micron::runtime{ "stop" };
      });
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
    for ( int i = 0; i < 32; ++i ) sb::require(*map.find(i) == i * 2);
  }
  sb::end_test_case();

  sb::test_case("pmap balances tracked values across shared versions");
  {
    using pmap_tracked_t = mtest::Tracked<78>;
    pmap_tracked_t::reset();
    {
      pmap_tracked_t seed(5);
      micron::pmap<int, pmap_tracked_t> empty;
      auto one = empty.insert(1, seed);
      auto two = one.insert(2, seed);
      auto shared = two;
      auto erased = two.erase(1);
      sb::require(one.find(1) != nullptr);
      sb::require(two.find(1) != nullptr && two.find(2) != nullptr);
      sb::require(shared.find(1) != nullptr && shared.find(2) != nullptr);
      sb::require(erased.find(1) == nullptr && erased.find(2) != nullptr);
    }
    sb::require(pmap_tracked_t::live() == 0);
  }
  sb::end_test_case();

  sb::test_case("stack_swiss publishes control only after value construction");
  {
    using throwing_t = mtest::Throwing<mtest::throw_on::copy_ctor, 72>;
    throwing_t::reset();
    throwing_t value(11);
    micron::stack_swiss_map<u64, throwing_t, 32> map;
    throwing_t::arm(0);
    bool threw = false;
    try {
      map.insert(1, value);
    } catch ( ... ) {
      threw = true;
    }
    throwing_t::disarm();
    sb::require(threw);
    sb::require(map.empty());
    sb::require(map.find(1) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("stack_swiss copy preserves tombstones in the probe chain");
  {
    auto keys = same_home<64>(0);
    sb::require(keys.a != 0 && keys.b != 0);
    micron::stack_swiss_map<u64, u64, 64> source;
    source.insert(keys.a, 10);
    source.insert(keys.b, 20);
    sb::require(source.erase(keys.a));

    micron::stack_swiss_map<u64, u64, 64> copy(source);
    sb::require(copy.find(keys.a) == nullptr);
    sb::require(copy.find(keys.b) != nullptr);
    sb::require(*copy.find(keys.b) == 20);
  }
  sb::end_test_case();

  sb::test_case("stack_swiss clips SIMD groups to NH");
  {
    auto keys = same_home<32>(0);
    sb::require(keys.a != 0 && keys.b != 0);
    micron::stack_swiss_map<u64, u64, 32, 1> map;
    auto first = map.insert(keys.a, 1);
    auto second = map.insert(keys.b, 2);
    sb::require(first.a && first.b != nullptr);
    sb::require(!second.a && second.b == nullptr);
    sb::require(map.size() == 1);
  }
  sb::end_test_case();

  sb::test_case("heap_swiss balances values through erase, clear, and rehash");
  {
    using heap_tracked_t = mtest::Tracked<73>;
    heap_tracked_t::reset();
    {
      heap_tracked_t seed(3);
      micron::heap_swiss_map<u64, heap_tracked_t> map;
      for ( u64 i = 0; i < 14; ++i ) map.insert(i, seed);
      sb::require(heap_tracked_t::live() == 15);
      map.reserve(64);
      sb::require(heap_tracked_t::live() == 15);
      map.erase(0);
      sb::require(heap_tracked_t::live() == 14);
      map.clear();
      sb::require(heap_tracked_t::live() == 1);
    }
    sb::require(heap_tracked_t::live() == 0);
  }
  sb::end_test_case();

  sb::test_case("heap_swiss copy preserves tombstones in the probe chain");
  {
    auto keys = same_home<64>(0);
    micron::heap_swiss_map<u64, u64> source(64);
    source.insert(keys.a, 10);
    source.insert(keys.b, 20);
    sb::require(source.erase(keys.a));

    micron::heap_swiss_map<u64, u64> copy(source);
    sb::require(copy.find(keys.a) == nullptr);
    sb::require(copy.find(keys.b) != nullptr);
    sb::require(*copy.find(keys.b) == 20);
  }
  sb::end_test_case();

  sb::test_case("heap_swiss rehash is transactional when value copy throws");
  {
    using throwing_t = mtest::Throwing<mtest::throw_on::copy_ctor, 74>;
    throwing_t::reset();
    micron::heap_swiss_map<u64, throwing_t> map;
    for ( u64 i = 0; i < 12; ++i ) {
      throwing_t value(static_cast<int>(i));
      map.insert(i, value);
    }
    throwing_t::arm(2);
    bool threw = false;
    try {
      map.reserve(64);
    } catch ( ... ) {
      threw = true;
    }
    throwing_t::disarm();
    sb::require(threw);
    sb::require(map.size() == 12);
    for ( u64 i = 0; i < 12; ++i ) {
      const throwing_t *value = map.find(i);
      sb::require(value != nullptr);
      sb::require(value->v == static_cast<int>(i));
    }
  }
  sb::end_test_case();

  sb::test_case("hopscotch resize leaves the source intact when value copy throws");
  {
    using throwing_t = mtest::Throwing<mtest::throw_on::copy_ctor, 75>;
    throwing_t::reset();
    micron::hopscotch_map<u64, throwing_t> map(1024);
    const usize slots = map.__slot_count();
    sb::require(slots <= 4096);
    const usize threshold = slots * 3u / 4u;
    bool homes[4096]{};
    u64 hashes[4096]{};
    u64 candidate = 1;
    for ( usize i = 0; i < threshold; ++i ) {
      while ( homes[hop_mix(candidate) & (slots - 1u)] ) ++candidate;
      hashes[i] = candidate++;
      homes[hop_mix(hashes[i]) & (slots - 1u)] = true;
      throwing_t value(static_cast<int>(i));
      map.insert_asis(hashes[i], value);
    }

    throwing_t extra(99);
    throwing_t::arm(2);
    bool threw = false;
    try {
      map.insert_asis(candidate, extra);
    } catch ( ... ) {
      threw = true;
    }
    throwing_t::disarm();
    sb::require(threw);
    sb::require(map.size() == threshold);
    for ( usize i = 0; i < threshold; ++i ) {
      const throwing_t *value = map.find_hash(hashes[i]);
      sb::require(value != nullptr);
      sb::require(value->v == static_cast<int>(i));
    }
  }
  sb::end_test_case();

  return 1;
}
