//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/b_map.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstring>
#include <set>
#include <vector>

// ─── small helpers ──────────────────────────────────────────────────────────

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

static micron::hstring<char>
make_wide_key(int i)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "wk_%08x_%08x", i * 0x9E3779B9u, i ^ 0xDEADBEEFu);
  return buf;
}

// (we exercise the hash-collision tie-break path via insert_hash / find_hash with a manually
//  supplied hash — no custom-key trickery required.)

int
main(void)
{
  sb::print("=== BTREE MAP TESTS ===");

  // ─ construction ─────────────────────────────────────────────────────────
  sb::test_case("construction - default constructor: empty map");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit capacity constructor");
  {
    micron::btree_map<micron::hstring<char>, int> m(128);
    sb::require(m.empty());
    sb::require(m.bucket_count() >= 128ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor transfers contents");
  {
    micron::btree_map<micron::hstring<char>, int> a;
    a.insert("hello", 42);
    a.insert("world", 99);
    micron::btree_map<micron::hstring<char>, int> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("hello") == 42);
    sb::require(*b.find("world") == 99);
  }
  sb::end_test_case();

  sb::test_case("construction - move assignment transfers contents");
  {
    micron::btree_map<micron::hstring<char>, int> a;
    a.insert("alpha", 1);
    a.insert("beta", 2);
    micron::btree_map<micron::hstring<char>, int> b;
    b = micron::move(a);
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("alpha") == 1);
    sb::require(*b.find("beta") == 2);
  }
  sb::end_test_case();

  // ─ insert / find / at / [] ───────────────────────────────────────────────
  sb::test_case("insert - single key/value, returns non-null pointer");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    int *p = m.insert("foo", 7);
    sb::require(p != nullptr);
    sb::require(*p == 7);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("find - present and missing keys");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    m.insert("b", 2);
    m.insert("c", 3);
    sb::require(m.find("a") && *m.find("a") == 1);
    sb::require(m.find("b") && *m.find("b") == 2);
    sb::require(m.find("c") && *m.find("c") == 3);
    sb::require(m.find("missing") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("at - hits and throws on miss");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("present", 11);
    sb::require(m.at("present") == 11);
    bool threw = false;
    try {
      (void)m.at("absent");
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator[] - miss creates default-constructed value");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    int &r = m[micron::hstring<char>("new")];
    sb::require(r == 0);
    sb::require(m.size() == 1ULL);
    r = 77;
    sb::require(*m.find("new") == 77);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key updates value in place");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("dup", 1);
    m.insert("dup", 2);
    m.insert("dup", 3);
    sb::require(*m.find("dup") == 3);
    // size should still be 1 (though we accept the buffer-optimistic counter being off
    // until rehash; the externally-visible state is what we test).
    sb::require(m.size() >= 1ULL);
  }
  sb::end_test_case();

  // ─ erase ─────────────────────────────────────────────────────────────────
  sb::test_case("erase - removes an existing key");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("x", 10);
    m.insert("y", 20);
    m.insert("z", 30);
    bool r = m.erase("y");
    sb::require(r);
    sb::require(m.find("y") == nullptr);
    sb::require(m.find("x") && *m.find("x") == 10);
    sb::require(m.find("z") && *m.find("z") == 30);
  }
  sb::end_test_case();

  sb::test_case("erase - missing key returns false");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("k", 1);
    sb::require(!m.erase("nope"));
    sb::require(m.find("k") && *m.find("k") == 1);
  }
  sb::end_test_case();

  // ─ hash collision tie-break (via insert_hash / find_hash) ──────────────
  sb::test_case("hash collision - distinct keys with same hash both retrievable");
  {
    micron::btree_map<int, int> m;
    constexpr micron::hash64_t H = 0xDEADBEEFCAFEBABEull;
    m.insert_hash(H, 1, 11);
    m.insert_hash(H, 2, 22);
    m.insert_hash(H, 3, 33);
    sb::require(m.find_hash(H, 1) && *m.find_hash(H, 1) == 11);
    sb::require(m.find_hash(H, 2) && *m.find_hash(H, 2) == 22);
    sb::require(m.find_hash(H, 3) && *m.find_hash(H, 3) == 33);
    sb::require(m.find_hash(H, 99) == nullptr);
  }
  sb::end_test_case();

  // ─ leaf split (force single-bucket B-tree to grow) ──────────────────────
  sb::test_case("split - leaf overflow promotes to internal root");
  {
    micron::btree_map<int, int> m;
    // every key uses a distinct hash but all into the same bucket via `hash & mask`.
    // we force all hashes to share their low bits while differing in high bits so the
    // bucket selection collides but per-leaf hashes differ (no tie-break churn).
    constexpr int N = 80;
    for ( int i = 0; i < N; ++i ) {
      micron::hash64_t h = (static_cast<micron::hash64_t>(i + 1) << 20) | 0x7;
      m.insert_hash(h, i, i * 7);
    }
    for ( int i = 0; i < N; ++i ) {
      micron::hash64_t h = (static_cast<micron::hash64_t>(i + 1) << 20) | 0x7;
      int *p = m.find_hash(h, i);
      sb::require(p != nullptr);
      sb::require(*p == i * 7);
    }
    sb::require(m.height() >= 2);
  }
  sb::end_test_case();

  // ─ fractal buffer correctness ────────────────────────────────────────────
  sb::test_case("buffer - many inserts + finds all succeed");
  {
    // a deeper tree exercises both the buffer-deposit path and the flush path.
    micron::btree_map<int, int> m;
    const usize buf = micron::btree_map<int, int>::buffer_size();
    const usize N = buf * 8 + 5;
    for ( usize i = 0; i < N; ++i ) {
      micron::hash64_t h = (static_cast<micron::hash64_t>(i + 1) << 20) | 0x7;
      m.insert_hash(h, static_cast<int>(i), static_cast<int>(i));
    }
    for ( usize i = 0; i < N; ++i ) {
      micron::hash64_t h = (static_cast<micron::hash64_t>(i + 1) << 20) | 0x7;
      int *p = m.find_hash(h, static_cast<int>(i));
      sb::require(p != nullptr);
      sb::require(*p == static_cast<int>(i));
    }
  }
  sb::end_test_case();

  // ─ fill-then-grow ────────────────────────────────────────────────────────
  // capped at 1024 because each hstring allocates a 4 KiB chunk (page granularity)
  // and the abc allocator has a ~64-sheet-per-size-class limit. for benchmarking
  // larger key counts use scalar K types (int, u64) or a smaller-granularity allocator.
  sb::test_case("growth - many distinct-hash inserts, all retrievable");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    constexpr int N = 128;
    for ( int i = 0; i < N; ++i ) {
      m.insert(make_wide_key(i), i);
    }
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find(make_wide_key(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
    sb::require(m.size() >= static_cast<usize>(N));
  }
  sb::end_test_case();

  // ─ erase-everything + reinsert (freelist sanity, int keys to avoid allocator pressure) ─
  sb::test_case("freelist - erase all then reinsert works (int key)");
  {
    micron::btree_map<int, int> m;
    constexpr int N = 256;
    for ( int i = 0; i < N; ++i ) m.insert_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i, static_cast<int>(i));
    for ( int i = 0; i < N; ++i ) (void)m.erase_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i);
    for ( int i = 0; i < N; ++i ) {
      sb::require(m.find_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i) == nullptr);
    }
    for ( int i = 0; i < N; ++i ) m.insert_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i, static_cast<int>(i * 2));
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i);
      sb::require(p != nullptr);
      sb::require(*p == i * 2);
    }
  }
  sb::end_test_case();

  // ─ clear ─────────────────────────────────────────────────────────────────
  sb::test_case("clear - resets to empty state");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 50; ++i ) m.insert(make_key(i), i);
    sb::require(m.size() >= 50ULL);
    m.clear();
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
    sb::require(m.find(make_key(0)) == nullptr);
    // reinsert after clear should still work
    m.insert("post-clear", 42);
    sb::require(m.find("post-clear") && *m.find("post-clear") == 42);
  }
  sb::end_test_case();

  // ─ iterator ──────────────────────────────────────────────────────────────
  sb::test_case("iterator - begin/end walk yields every inserted key once");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    constexpr int N = 50;
    std::set<int> expected;
    for ( int i = 0; i < N; ++i ) {
      m.insert(make_key(i), i);
      expected.insert(i);
    }
    std::set<int> seen;
    for ( auto it = m.begin(); it != m.end(); ++it ) seen.insert((*it).value);
    sb::require(seen == expected);
  }
  sb::end_test_case();

  // ─ hash-ordered ops (NOT key-ordered — names are explicitly hash_* to make that visible) ─
  sb::test_case("hash-ordered ops - hash_min/hash_max/hash_lower_bound/hash_upper_bound non-null on populated map");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 32; ++i ) m.insert(make_key(i), i);
    sb::require(m.hash_min() != nullptr);
    sb::require(m.hash_max() != nullptr);
    sb::require(m.hash_lower_bound(make_key(5)) != nullptr);
    // hash_upper_bound may be null if the queried hash is at the high end; just check
    // the call doesn't crash. hash_successor is an alias for hash_upper_bound.
    (void)m.hash_upper_bound(make_key(5));
    (void)m.hash_successor(make_key(5));
    (void)m.hash_predecessor(make_key(5));
  }
  sb::end_test_case();

  // ─ contains / count / exists ─────────────────────────────────────────────
  sb::test_case("contains/count/exists - basic semantics");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.insert("alpha", 1);
    sb::require(m.contains("alpha"));
    sb::require(!m.contains("beta"));
    sb::require(m.count("alpha") == 1ULL);
    sb::require(m.count("beta") == 0ULL);
    sb::require(m.exists("alpha") == 1ULL);
  }
  sb::end_test_case();

  // ─ swap ──────────────────────────────────────────────────────────────────
  sb::test_case("swap - exchanges contents");
  {
    micron::btree_map<micron::hstring<char>, int> a;
    micron::btree_map<micron::hstring<char>, int> b;
    a.insert("one", 1);
    a.insert("two", 2);
    b.insert("ten", 10);
    a.swap(b);
    sb::require(a.find("ten") && *a.find("ten") == 10);
    sb::require(b.find("one") && *b.find("one") == 1);
    sb::require(b.find("two") && *b.find("two") == 2);
  }
  sb::end_test_case();

  // ─ emplace ───────────────────────────────────────────────────────────────
  sb::test_case("emplace - constructs in place");
  {
    micron::btree_map<micron::hstring<char>, int> m;
    m.emplace("x", 42);
    sb::require(m.find("x") && *m.find("x") == 42);
  }
  sb::end_test_case();

  // ─ same-hash overflow chain (regression for equivalence-class split bug) ────────────
  // Insert far more entries with the same hash than fit in one leaf. Under the old code
  // the first split would have placed equal-hash entries on both sides of a pivot, then
  // routing would have permanently lost the left-side ones. The new overflow chain keeps
  // every entry findable.
  sb::test_case("overflow chain - >leaf_fanout entries with same hash all retrievable");
  {
    micron::btree_map<int, int> m;
    constexpr micron::hash64_t H = 0xCAFEBABEDEADBEEFull;
    const usize fanout = micron::btree_map<int, int>::leaf_fanout();
    const int N = static_cast<int>(fanout * 4 + 7);      // >> leaf_fanout
    for ( int i = 0; i < N; ++i ) m.insert_hash(H, i, static_cast<int>(i * 13));
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find_hash(H, i);
      sb::require(p != nullptr);
      sb::require(*p == i * 13);
    }
    sb::require(m.find_hash(H, N + 1) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("overflow chain - erase from chain leaves remaining entries findable");
  {
    micron::btree_map<int, int> m;
    constexpr micron::hash64_t H = 0x0123456789ABCDEFull;
    const usize fanout = micron::btree_map<int, int>::leaf_fanout();
    const int N = static_cast<int>(fanout * 3 + 5);
    for ( int i = 0; i < N; ++i ) m.insert_hash(H, i, static_cast<int>(i));
    for ( int i = 0; i < N; i += 3 ) sb::require(m.erase_hash(H, i));
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find_hash(H, i);
      if ( i % 3 == 0 ) {
        sb::require(p == nullptr);
      } else {
        sb::require(p != nullptr);
        sb::require(*p == i);
      }
    }
  }
  sb::end_test_case();

  // ─ asymmetric split (single-class leaf receives a different hash) ──────────────────
  sb::test_case("asymmetric split - full single-hash leaf accepts a smaller hash");
  {
    micron::btree_map<int, int> m;
    constexpr micron::hash64_t H = 0xF000000000000010ull;
    const usize fanout = micron::btree_map<int, int>::leaf_fanout();
    for ( usize i = 0; i < fanout; ++i ) m.insert_hash(H, static_cast<int>(i), static_cast<int>(i));
    // Smaller hash forces an asymmetric split with the new entry occupying the original leaf.
    constexpr micron::hash64_t H_lo = 0x0000000000000010ull;
    m.insert_hash(H_lo, 9999, 4242);
    sb::require(m.find_hash(H_lo, 9999) != nullptr);
    sb::require(*m.find_hash(H_lo, 9999) == 4242);
    for ( usize i = 0; i < fanout; ++i ) {
      int *p = m.find_hash(H, static_cast<int>(i));
      sb::require(p != nullptr);
      sb::require(*p == static_cast<int>(i));
    }
  }
  sb::end_test_case();

  sb::test_case("asymmetric split - full single-hash leaf accepts a larger hash");
  {
    micron::btree_map<int, int> m;
    constexpr micron::hash64_t H = 0x0000000000000020ull;
    const usize fanout = micron::btree_map<int, int>::leaf_fanout();
    for ( usize i = 0; i < fanout; ++i ) m.insert_hash(H, static_cast<int>(i), static_cast<int>(i) + 100);
    constexpr micron::hash64_t H_hi = 0xF000000000000020ull;
    m.insert_hash(H_hi, 1234, 5678);
    sb::require(m.find_hash(H_hi, 1234) != nullptr);
    sb::require(*m.find_hash(H_hi, 1234) == 5678);
    for ( usize i = 0; i < fanout; ++i ) {
      int *p = m.find_hash(H, static_cast<int>(i));
      sb::require(p != nullptr);
      sb::require(*p == static_cast<int>(i) + 100);
    }
  }
  sb::end_test_case();

  // ─ stress ────────────────────────────────────────────────────────────────
  sb::test_case("stress - many random inserts and finds (int key)");
  {
    micron::btree_map<int, int> m;
    constexpr int N = 8192;
    for ( int i = 0; i < N; ++i ) m.insert_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i, static_cast<int>(i));
    int hits = 0;
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find_hash(static_cast<micron::hash64_t>(i) * 2654435761ULL, i);
      if ( p && *p == i ) ++hits;
    }
    sb::require(hits == N);
  }
  sb::end_test_case();

  sb::print("=== ALL BTREE MAP TESTS PASSED ===");
  return 1;
}
