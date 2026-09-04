//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/trees/art.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstdio>

[[gnu::always_inline]] static inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return micron::hstring<char>(static_cast<const char *>(buf));
}

int
main(void)
{
  sb::print("=== ART TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::art<int, int> a;
    sb::require(a.empty());
    sb::require(a.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("insert single");
  {
    micron::art<int, int> a;
    sb::require(a.insert(1, 10));
    sb::require(a.size() == 1ULL);
    sb::require(*a.find(1) == 10);
  }
  sb::end_test_case();

  sb::test_case("insert duplicate updates value");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    a.insert(1, 99);      // duplicate - updates
    sb::require(a.size() == 1ULL);
    sb::require(*a.find(1) == 99);
  }
  sb::end_test_case();

  sb::test_case("find - miss returns nullptr");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.find(99) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("contains");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.contains(1));
    sb::require(!a.contains(2));
  }
  sb::end_test_case();

  sb::test_case("erase - returns true on hit");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.erase(1));
    sb::require(!a.contains(1));
    sb::require(a.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - returns false on miss");
  {
    micron::art<int, int> a;
    sb::require(!a.erase(99));
  }
  sb::end_test_case();

  sb::test_case("at - throws on miss");
  {
    micron::art<int, int> a;
    bool threw = false;
    try {
      a.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("clear");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 100; ++i ) a.insert(i, i);
    a.clear();
    sb::require(a.empty());
  }
  sb::end_test_case();

  // ── bulk ─────────────────────────────────────────────────────────────────

  sb::test_case("bulk - 1000 inserts");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 1000; ++i ) a.insert(i, i * 2);
    sb::require(a.size() == 1000ULL);
    for ( int i = 0; i < 1000; ++i ) {
      int *v = a.find(i);
      sb::require(v != nullptr);
      sb::require(*v == i * 2);
    }
  }
  sb::end_test_case();

  sb::test_case("bulk - growth through node types");
  {
    micron::art<int, int> a;
    // 20 keys with the same low byte will fit in n4/n16; 1000 will force n48/n256.
    for ( int i = 0; i < 5000; ++i ) a.insert(i, i + 1);
    sb::require(a.size() == 5000ULL);
    for ( int i = 0; i < 5000; ++i ) sb::require(*a.find(i) == i + 1);
  }
  sb::end_test_case();

  // ── string keys ──────────────────────────────────────────────────────────

  sb::test_case("string keys - insert/find/erase");
  {
    micron::art<micron::hstring<char>, int> a;
    for ( int i = 0; i < 200; ++i ) a.insert(make_key(i), i);
    sb::require(a.size() == 200ULL);
    for ( int i = 0; i < 200; ++i ) {
      int *v = a.find(make_key(i));
      sb::require(v != nullptr);
      sb::require(*v == i);
    }
    for ( int i = 0; i < 100; ++i ) sb::require(a.erase(make_key(i)));
    sb::require(a.size() == 100ULL);
  }
  sb::end_test_case();

  // ── for_each ─────────────────────────────────────────────────────────────

  sb::test_case("for_each");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 100; ++i ) a.insert(i, i * 3);
    int sum = 0;
    int cnt = 0;
    a.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 100);
    int expected = 0;
    for ( int i = 0; i < 100; ++i ) expected += i * 3;
    sb::require(sum == expected);
  }
  sb::end_test_case();

  // ── move ─────────────────────────────────────────────────────────────────

  sb::test_case("move constructor");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 10; ++i ) a.insert(i, i);
    micron::art<int, int> b(micron::move(a));
    sb::require(b.size() == 10ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find(5) == 5);
  }
  sb::end_test_case();

  sb::test_case("nodes SHRINK and COLLAPSE - churn must not ratchet memory upward");
  {
    // __remove_child collapsed an emptied __n4 and nothing else: an emptied n16/n48/n256 stayed
    // wired into its parent forever, and there was no __shrink at all, so a trie that grew to a
    // wide fan-out and then drained kept every 2056-byte __n256 it had ever promoted. Nothing
    // leaked -- the destructor still found them -- so no correctness test could see it. The
    // assertion is on the block count.
    micron::art<u64, u64> t;
    constexpr u64 N = 200000;

    for ( u64 i = 0; i < N; ++i ) t.insert(splitmix64(i), i);
    const usize peak = t.blocks_allocated();
    sb::require(peak > 0ULL);
    sb::require(t.size() == N);

    for ( u64 i = 0; i < N; ++i ) sb::require(t.erase(splitmix64(i)));
    sb::require(t.size() == 0ULL);

    // The property is that churn does not RATCHET, not that the first rebuild needs no new block.
    // Those are different, and the difference is real: five independent pools plus a
    // demote/promote sequence can legitimately leave one pool one block short while another has
    // spare. Measured on arm32, peak=44 and the first rebuild=45 -- one block, and not a leak.
    // What a reclaim failure looks like is the count climbing every cycle, so that is what is
    // asserted: settle after one rebuild, then require steady state.
    for ( u64 i = 0; i < N; ++i ) t.insert(splitmix64(i), i);
    const usize settled = t.blocks_allocated();
    sb::require(settled <= peak + 8);
    for ( u32 round = 0; round < 4; ++round ) {
      for ( u64 i = 0; i < N; ++i ) t.erase(splitmix64(i));
      for ( u64 i = 0; i < N; ++i ) t.insert(splitmix64(i), i);
      sb::require(t.size() == N);
      sb::require(t.blocks_allocated() <= settled);
    }

    // and the trie still answers correctly after all that promotion and demotion
    for ( u64 i = 0; i < N; i += 997 ) {
      const u64 *p = t.find(splitmix64(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
    sb::require(t.find(splitmix64(N + 12345)) == nullptr);

    // NEGATIVE CONTROL: a populated trie must report blocks, so the bound above is measuring
    // reuse and not a counter stuck at zero
    sb::require(t.blocks_allocated() > 0ULL);

    t.clear();
    sb::require(t.blocks_allocated() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("shrink DEMOTES wide nodes - a drained trie costs what a fresh one costs");
  {
    // The block counter above cannot see __shrink: a freed __n256 returns to the __n256 pool
    // whether or not the tree demoted it, so the block count is identical either way. What shrink
    // changes is which KIND of node is live -- an interior node with four children holding a
    // 2056-byte __n256 instead of a 40-byte __n4. node_bytes_live() measures exactly that, and
    // the assertion is self-calibrating: a trie drained down to a sparse key set must cost about
    // what a trie built directly from that sparse set costs.
    constexpr u64 WIDE = 200000;
    constexpr u64 KEEP = 64;

    micron::art<u64, u64> fresh;
    for ( u64 i = 0; i < KEEP; ++i ) fresh.insert(splitmix64(i), i);
    const usize bytes_fresh = fresh.node_bytes_live();
    sb::require(bytes_fresh > 0ULL);

    micron::art<u64, u64> drained;
    for ( u64 i = 0; i < WIDE; ++i ) drained.insert(splitmix64(i), i);
    const usize bytes_wide = drained.node_bytes_live();
    for ( u64 i = KEEP; i < WIDE; ++i ) sb::require(drained.erase(splitmix64(i)));
    sb::require(drained.size() == KEEP);
    const usize bytes_drained = drained.node_bytes_live();

    // it did get much smaller than it was at full width
    sb::require(bytes_drained < bytes_wide / 8);
    // and it is within a small factor of purpose-built -- this is the assertion that fails when
    // wide nodes are never demoted
    sb::require(bytes_drained <= bytes_fresh * 4);

    // and it still answers correctly
    for ( u64 i = 0; i < KEEP; ++i ) {
      const u64 *p = drained.find(splitmix64(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
    for ( u64 i = KEEP; i < KEEP + 32; ++i ) sb::require(drained.find(splitmix64(i)) == nullptr);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
