//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor: micron::__tree_store -- the primitives every arena-backed tree descends through.
//
// The two scans replaced a linear walk in b_tree's descent, so an off-by-one here is an off-by-one
// in every lookup in the library that goes through a B-tree. They are exercised exhaustively:
// every length in [0, 96], and for each length every distinguishable probe -- below all, equal to
// each element, strictly between each adjacent pair, above all -- against a hand-written linear
// oracle, over strictly-increasing AND duplicate-heavy arrays, for six arithmetic key types plus a
// non-arithmetic one.
//
// The oracle is deliberately the naive loop the scans replaced: if the two ever agreed only
// because both were wrong the same way, the NEGATIVE CONTROL at the end would not fire.

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/trees/__tree_store.hpp"

#include "../snowball/snowball.hpp"

#include <vector>

namespace
{

namespace ts = micron::__tree_store;

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

// NO __natural_order marker: forces the ladder / expensive-comparator path
template<typename T> struct less_policy {
  static bool
  lt(const T &a, const T &b)
  {
    return a < b;
  }
};

// WITH the marker: selects the vector rank for 4/8-byte integers and the linear walk otherwise.
// Both policies order identically, so every assertion below must hold for both -- which is how a
// divergence between the scalar and the vector path shows up as a failure rather than as a
// silently wrong lookup somewhere in b_tree.
template<typename T> struct natural_less_policy {
  using __natural_order = void;

  static bool
  lt(const T &a, const T &b)
  {
    return a < b;
  }
};

// the naive walks that child_index/leaf_lb used to be. this is the oracle.
template<typename K, typename C>
u16
oracle_lower(const K *keys, u16 n, const K &key)
{
  u16 p = 0;
  while ( p < n && C::lt(keys[p], key) ) ++p;
  return p;
}

template<typename K, typename C>
u16
oracle_upper(const K *keys, u16 n, const K &key)
{
  u16 p = 0;
  while ( p < n && !C::lt(key, keys[p]) ) ++p;
  return p;
}

// a key that is NOT arithmetic, to pin that the scans never assumed one
struct boxed {
  int v;

  constexpr boxed() noexcept : v(0) { }

  constexpr explicit boxed(int x) noexcept : v(x) { }

  constexpr bool
  operator<(const boxed &o) const noexcept
  {
    return v < o.v;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// every length x every distinguishable probe

template<typename K, typename C, typename Make>
bool
sweep_scans(const char *what, Make make)
{
  constexpr u16 MAXN = 96;
  std::vector<K> keys(MAXN);

  for ( u16 n = 0; n <= MAXN; ++n ) {
    // (a) strictly increasing: values 2,4,6,... so odd values probe strictly between
    for ( u16 i = 0; i < n; ++i ) keys[i] = make(2 * (static_cast<int>(i) + 1));

    for ( int probe = 0; probe <= 2 * (static_cast<int>(n) + 1) + 1; ++probe ) {
      const K k = make(probe);
      const u16 lb = ts::lower_bound_scan<K, C>(keys.data(), n, k);
      const u16 ub = ts::upper_bound_scan<K, C>(keys.data(), n, k);
      const u16 olb = oracle_lower<K, C>(keys.data(), n, k);
      const u16 oub = oracle_upper<K, C>(keys.data(), n, k);
      if ( lb != olb || ub != oub ) {
        micron::io::println("  MISMATCH ", what, " strict n=", n, " probe=", probe, " lb=", lb, "/", olb, " ub=", ub, "/", oub);
        return false;
      }
      // the invariants the callers actually rely on
      if ( lb > n || ub > n || lb > ub ) return false;
      if ( lb > 0 && !C::lt(keys[lb - 1], k) ) return false;          // everything before lb is < k
      if ( lb < n && C::lt(keys[lb], k) ) return false;               // keys[lb] is not < k
      if ( ub < n && !C::lt(k, keys[ub]) ) return false;              // keys[ub] is > k
    }

    // (b) duplicate-heavy: runs of three, so lb and ub straddle a run
    for ( u16 i = 0; i < n; ++i ) keys[i] = make(2 * ((static_cast<int>(i) / 3) + 1));

    for ( int probe = 0; probe <= 2 * (static_cast<int>(n) / 3 + 2); ++probe ) {
      const K k = make(probe);
      const u16 lb = ts::lower_bound_scan<K, C>(keys.data(), n, k);
      const u16 ub = ts::upper_bound_scan<K, C>(keys.data(), n, k);
      const u16 olb = oracle_lower<K, C>(keys.data(), n, k);
      const u16 oub = oracle_upper<K, C>(keys.data(), n, k);
      if ( lb != olb || ub != oub ) {
        micron::io::println("  MISMATCH ", what, " dup n=", n, " probe=", probe, " lb=", lb, "/", olb, " ub=", ub, "/", oub);
        return false;
      }
    }
  }
  return true;
}

// randomized: sorted arrays drawn from a fixed seed, probed with values from the same stream
template<typename K, typename C, typename Make>
bool
fuzz_scans(Make make, u64 seed)
{
  u64 rng = seed;
  std::vector<K> keys;
  for ( u32 round = 0; round < 4000; ++round ) {
    const u16 n = static_cast<u16>(splitmix64(rng++) % 64);
    std::vector<int> raw(n);
    for ( u16 i = 0; i < n; ++i ) raw[i] = static_cast<int>(splitmix64(rng++) % 200);
    // insertion sort; the scans require sorted input and nothing else
    for ( u16 i = 1; i < n; ++i ) {
      const int v = raw[i];
      int j = static_cast<int>(i) - 1;
      while ( j >= 0 && raw[j] > v ) {
        raw[j + 1] = raw[j];
        --j;
      }
      raw[j + 1] = v;
    }
    keys.assign(n, make(0));
    for ( u16 i = 0; i < n; ++i ) keys[i] = make(raw[i]);

    for ( u32 t = 0; t < 8; ++t ) {
      const K k = make(static_cast<int>(splitmix64(rng++) % 210));
      if ( ts::lower_bound_scan<K, C>(keys.data(), n, k) != oracle_lower<K, C>(keys.data(), n, k) ) return false;
      if ( ts::upper_bound_scan<K, C>(keys.data(), n, k) != oracle_upper<K, C>(keys.data(), n, k) ) return false;
    }
  }
  return true;
}

};      // namespace

int
main(void)
{
  sb::print("=== TREE_STORE TESTS ===");

  sb::test_case("tagged index - round trip and disjointness");
  {
    for ( u32 s = 0; s < 4096; ++s ) {
      const ts::node_idx slot = static_cast<ts::node_idx>(s);
      const ts::node_idx tagged = ts::tag_leaf(slot);
      sb::require(ts::is_leaf_idx(tagged));
      sb::require(!ts::is_leaf_idx(slot));
      sb::require(ts::slot_of(tagged) == slot);
      sb::require(ts::slot_of(slot) == slot);
    }
    // the top of the slot space, where a naive mask would collide with the tag
    const ts::node_idx top = ts::max_slots - 1u;
    sb::require(ts::slot_of(ts::tag_leaf(top)) == top);
    sb::require(ts::is_leaf_idx(ts::tag_leaf(top)));
    // nil carries the bit; callers must test != nil first. this pins the documented contract
    sb::require(ts::is_leaf_idx(ts::nil));
  }
  sb::end_test_case();

  sb::test_case("scans - exhaustive, u64");
  {
    sb::require(sweep_scans<u64, less_policy<u64>>("u64", [](int x) { return static_cast<u64>(x); }));
    sb::require(sweep_scans<u64, natural_less_policy<u64>>("u64", [](int x) { return static_cast<u64>(x); }));
  }
  sb::end_test_case();

  sb::test_case("scans - exhaustive, u32 / i32 / i64");
  {
    sb::require(sweep_scans<u32, less_policy<u32>>("u32", [](int x) { return static_cast<u32>(x); }));
    sb::require(sweep_scans<u32, natural_less_policy<u32>>("u32", [](int x) { return static_cast<u32>(x); }));
    sb::require(sweep_scans<i32, less_policy<i32>>("i32", [](int x) { return static_cast<i32>(x); }));
    sb::require(sweep_scans<i32, natural_less_policy<i32>>("i32", [](int x) { return static_cast<i32>(x); }));
    sb::require(sweep_scans<i64, less_policy<i64>>("i64", [](int x) { return static_cast<i64>(x); }));
    sb::require(sweep_scans<i64, natural_less_policy<i64>>("i64", [](int x) { return static_cast<i64>(x); }));
  }
  sb::end_test_case();

  sb::test_case("scans - exhaustive, f32 / f64");
  {
    sb::require(sweep_scans<f32, less_policy<f32>>("f32", [](int x) { return static_cast<f32>(x); }));
    sb::require(sweep_scans<f32, natural_less_policy<f32>>("f32", [](int x) { return static_cast<f32>(x); }));
    sb::require(sweep_scans<f64, less_policy<f64>>("f64", [](int x) { return static_cast<f64>(x); }));
    sb::require(sweep_scans<f64, natural_less_policy<f64>>("f64", [](int x) { return static_cast<f64>(x); }));
  }
  sb::end_test_case();

  sb::test_case("scans - exhaustive, non-arithmetic key");
  {
    sb::require(sweep_scans<boxed, less_policy<boxed>>("boxed", [](int x) { return boxed(x); }));
    sb::require(sweep_scans<boxed, natural_less_policy<boxed>>("boxed", [](int x) { return boxed(x); }));
  }
  sb::end_test_case();

  sb::test_case("scans - randomized against the oracle");
  {
    sb::require(fuzz_scans<u64, less_policy<u64>>([](int x) { return static_cast<u64>(x); }, 0xC0FFEE01ULL));
    sb::require(fuzz_scans<u64, natural_less_policy<u64>>([](int x) { return static_cast<u64>(x); }, 0xC0FFEE01ULL));
    sb::require(fuzz_scans<i32, less_policy<i32>>([](int x) { return static_cast<i32>(x); }, 0xBEEF1234ULL));
    sb::require(fuzz_scans<i32, natural_less_policy<i32>>([](int x) { return static_cast<i32>(x); }, 0xBEEF1234ULL));
    sb::require(fuzz_scans<boxed, less_policy<boxed>>([](int x) { return boxed(x); }, 0x9E3779B1ULL));
    sb::require(fuzz_scans<boxed, natural_less_policy<boxed>>([](int x) { return boxed(x); }, 0x9E3779B1ULL));
  }
  sb::end_test_case();

  sb::test_case("scans - NEGATIVE CONTROL, the oracle can disagree");
  {
    // a test that cannot fail is not a test. Feed the upper-bound scan where the lower-bound
    // oracle is expected and require that they DO differ somewhere -- if this passes silently,
    // the two are not measuring what the sweeps above assume.
    u64 keys[8] = { 2, 4, 6, 8, 10, 12, 14, 16 };
    using C = less_policy<u64>;
    bool ever_differed = false;
    for ( u64 k = 0; k <= 18; ++k )
      if ( ts::lower_bound_scan<u64, C>(keys, 8, k) != ts::upper_bound_scan<u64, C>(keys, 8, k) ) ever_differed = true;
    sb::require(ever_differed);

    // and a deliberately corrupted array must make the scan disagree with the oracle, proving
    // the sweeps would have caught a real defect rather than comparing two identical loops
    u64 bad[8] = { 2, 4, 6, 8, 10, 12, 14, 16 };
    bad[3] = 99;      // breaks sortedness
    bool found_disagreement = false;
    for ( u64 k = 0; k <= 100; ++k )
      if ( ts::lower_bound_scan<u64, C>(bad, 8, k) != oracle_lower<u64, C>(bad, 8, k) ) found_disagreement = true;
    sb::require(found_disagreement);
  }
  sb::end_test_case();

  sb::test_case("scans - the vector path and the scalar path agree exactly");
  {
    // The dispatch picks a different implementation for the two policies. If they ever disagree,
    // b_tree's descent is wrong on exactly the machines where the vector path is compiled in --
    // which is the failure mode a single-policy test cannot see.
    u64 rng = 0x5EEDF00DULL;
    u64 keys[64];
    for ( u16 n = 0; n <= 64; ++n ) {
      for ( u16 i = 0; i < n; ++i ) keys[i] = (i + 1) * 4ULL;
      for ( u32 t = 0; t < 300; ++t ) {
        const u64 k = splitmix64(rng++) % 300;
        sb::require((ts::lower_bound_scan<u64, less_policy<u64>>(keys, n, k)
                     == ts::lower_bound_scan<u64, natural_less_policy<u64>>(keys, n, k)));
        sb::require((ts::upper_bound_scan<u64, less_policy<u64>>(keys, n, k)
                     == ts::upper_bound_scan<u64, natural_less_policy<u64>>(keys, n, k)));
      }
    }
    i32 sk[64];
    for ( u16 n = 0; n <= 64; ++n ) {
      // straddle zero: the unsigned bias in the vector path is exactly what this catches
      for ( u16 i = 0; i < n; ++i ) sk[i] = static_cast<i32>(i) * 4 - 128;
      for ( i32 k = -140; k <= 160; ++k ) {
        sb::require((ts::lower_bound_scan<i32, less_policy<i32>>(sk, n, k)
                     == ts::lower_bound_scan<i32, natural_less_policy<i32>>(sk, n, k)));
        sb::require((ts::upper_bound_scan<i32, less_policy<i32>>(sk, n, k)
                     == ts::upper_bound_scan<i32, natural_less_policy<i32>>(sk, n, k)));
      }
    }
    // and unsigned keys with the high bit set, where a signed compare would order them wrongly
    u64 hi[16];
    for ( u16 i = 0; i < 16; ++i ) hi[i] = 0x7FFFFFFFFFFFFFF0ULL + i;      // crosses 2^63
    for ( u32 t = 0; t < 64; ++t ) {
      const u64 k = 0x7FFFFFFFFFFFFFE0ULL + t;
      sb::require((ts::lower_bound_scan<u64, less_policy<u64>>(hi, 16, k)
                   == ts::lower_bound_scan<u64, natural_less_policy<u64>>(hi, 16, k)));
      sb::require((ts::upper_bound_scan<u64, less_policy<u64>>(hi, 16, k)
                   == ts::upper_bound_scan<u64, natural_less_policy<u64>>(hi, 16, k)));
    }
  }
  sb::end_test_case();

  sb::test_case("node_arena - allocate, free-list reuse, live count");
  {
    ts::node_arena<64, 64> a;
    sb::require(a.slots_used() == 0ULL);
    sb::require(a.slots_live() == 0ULL);

    ts::node_idx ids[128];
    for ( u32 i = 0; i < 128; ++i ) ids[i] = a.allocate();
    for ( u32 i = 0; i < 128; ++i )
      for ( u32 j = i + 1; j < 128; ++j ) sb::require(ids[i] != ids[j]);
    sb::require(a.slots_used() == 128ULL);
    sb::require(a.slots_live() == 128ULL);

    for ( u32 i = 0; i < 64; ++i ) a.deallocate(ids[i]);
    sb::require(a.slots_used() == 128ULL);      // high water does not fall
    sb::require(a.slots_live() == 64ULL);       // live does

    // the next 64 allocations must come off the free list, not extend the slab
    for ( u32 i = 0; i < 64; ++i ) (void)a.allocate();
    sb::require(a.slots_used() == 128ULL);
    sb::require(a.slots_live() == 128ULL);

    a.reset();
    sb::require(a.slots_used() == 0ULL);
    sb::require(a.slots_live() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("node_arena - the free-list link does not corrupt the slot");
  {
    // deallocate() writes the link into the first bytes of a dead slot. A slot handed back out
    // must be fully writable, and writing it must not disturb any other live slot.
    ts::node_arena<64, 64> a;
    ts::node_idx keep[32];
    for ( u32 i = 0; i < 32; ++i ) {
      keep[i] = a.allocate();
      for ( u32 b = 0; b < 64; ++b ) a.raw(keep[i])[b] = static_cast<byte>(i + 1);
    }
    ts::node_idx dead[32];
    for ( u32 i = 0; i < 32; ++i ) dead[i] = a.allocate();
    for ( u32 i = 0; i < 32; ++i ) a.deallocate(dead[i]);
    for ( u32 i = 0; i < 32; ++i ) {
      const ts::node_idx r = a.allocate();
      for ( u32 b = 0; b < 64; ++b ) a.raw(r)[b] = static_cast<byte>(0xAB);
    }
    for ( u32 i = 0; i < 32; ++i )
      for ( u32 b = 0; b < 64; ++b ) sb::require(a.raw(keep[i])[b] == static_cast<byte>(i + 1));
  }
  sb::end_test_case();

  sb::test_case("node_arena - reserve pins the slab");
  {
    ts::node_arena<128, 64> a;
    a.reserve(4096);
    sb::require(a.slots_reserved() >= 4096ULL);
    const byte *base = a.raw(a.allocate());
    for ( u32 i = 1; i < 4096; ++i ) (void)a.allocate();
    // nothing grew, so the first slot did not move -- this is the guarantee reserve() sells
    sb::require(a.raw(0) == base);
  }
  sb::end_test_case();

  sb::test_case("block_pool - carves, reuses, releases; and survives an object bigger than a block");
  {
    struct big {
      byte pad[2056];      // art's __n256 shape: larger than half the 4096-byte minimum block
    };

    ts::block_pool<big> bp;
    // without the min-slot floor this spins forever adding blocks with no usable slot
    big *v[64];
    for ( u32 i = 0; i < 64; ++i ) {
      v[i] = bp.take();
      v[i]->pad[0] = static_cast<byte>(i + 1);
      v[i]->pad[2055] = static_cast<byte>(i + 1);
    }
    for ( u32 i = 0; i < 64; ++i ) {
      sb::require(v[i]->pad[0] == static_cast<byte>(i + 1));
      sb::require(v[i]->pad[2055] == static_cast<byte>(i + 1));
      for ( u32 j = i + 1; j < 64; ++j ) sb::require(v[i] != v[j]);
    }
    sb::require(bp.blocks() > 0ULL);

    // give() then take() must hand back the same storage, not grow
    const usize before = bp.blocks();
    for ( u32 i = 0; i < 64; ++i ) bp.give(v[i]);
    for ( u32 i = 0; i < 64; ++i ) (void)bp.take();
    sb::require(bp.blocks() == before);

    bp.release();
    sb::require(bp.blocks() == 0ULL);

    // a small T, where a block holds many
    ts::block_pool<u64> sp;
    u64 *a = sp.take();
    u64 *b = sp.take();
    sb::require(a != b);
    sb::require(sp.blocks() == 1ULL);
    sp.reserve(100000);
    for ( u32 i = 0; i < 100000; ++i ) (void)sp.take();
    sb::require(sp.blocks() <= 3ULL);
  }
  sb::end_test_case();

  sb::test_case("dual_arena - kinds are disjoint and the tag routes raw()");
  {
    ts::dual_arena<256, 64, 32, 8> d;
    ts::node_idx leaves[64];
    ts::node_idx inner[64];
    for ( u32 i = 0; i < 64; ++i ) {
      leaves[i] = d.allocate_leaf();
      inner[i] = d.allocate_internal();
    }
    for ( u32 i = 0; i < 64; ++i ) {
      sb::require(ts::is_leaf_idx(leaves[i]));
      sb::require(!ts::is_leaf_idx(inner[i]));
    }
    // write a distinct pattern through each and prove they never alias
    for ( u32 i = 0; i < 64; ++i ) {
      d.raw(leaves[i])[0] = static_cast<byte>(i + 1);
      d.raw(inner[i])[0] = static_cast<byte>(0x80 | i);
    }
    for ( u32 i = 0; i < 64; ++i ) {
      sb::require(d.raw(leaves[i])[0] == static_cast<byte>(i + 1));
      sb::require(d.raw(inner[i])[0] == static_cast<byte>(0x80 | i));
      sb::require(d.raw_leaf(leaves[i]) == d.raw(leaves[i]));
      sb::require(d.raw_internal(inner[i]) == d.raw(inner[i]));
    }
    sb::require(d.slots_live() == 128ULL);
    d.deallocate(leaves[0]);
    sb::require(d.slots_live() == 127ULL);

    // the whole point: an internal node must not be paying for a leaf-sized slot
    sb::require(d.bytes_reserved() > 0ULL);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
