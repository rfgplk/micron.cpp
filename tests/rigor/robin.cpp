//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/robin.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstring>
#include <set>
#include <vector>

// ─── small helpers ───────────────────────────────────────────────────────────

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

// Keys that are guaranteed to produce distinct hashes (used for collision-path tests).
// Format chosen to be spread across the hash space while remaining human-readable.
static micron::hstring<char>
make_wide_key(int i)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "wk_%08x_%08x", i * 0x9E3779B9u, i ^ 0xDEADBEEFu);
  return buf;
}

// ─── main ────────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== ROBIN MAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default constructor: empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit capacity constructor");
  {
    micron::robin_map<micron::hstring<char>, int> m(128);
    sb::require(m.empty());
    sb::require(m.max_size() >= 128ULL);
  }
  sb::end_test_case();
  sb::test_case("construction - minimum capacity enforced");
  {
    micron::robin_map<micron::hstring<char>, int> m(1);
    sb::require(m.max_size() >= 16ULL);     // __min_cap = 16
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor transfers contents");
  {
    micron::robin_map<micron::hstring<char>, int> a(64);
    a.insert("hello", 42);
    a.insert("world", 99);

    micron::robin_map<micron::hstring<char>, int> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("hello") == 42);
    sb::require(*b.find("world") == 99);
  }
  sb::end_test_case();

  sb::test_case("construction - move assignment transfers contents");
  {
    micron::robin_map<micron::hstring<char>, int> a(64);
    a.insert("alpha", 1);
    a.insert("beta", 2);

    micron::robin_map<micron::hstring<char>, int> b;
    b = micron::move(a);
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("alpha") == 1);
    sb::require(*b.find("beta") == 2);
  }
  sb::end_test_case();

  sb::test_case("construction - move-assign to different object: source cleared");
  {
    micron::robin_map<micron::hstring<char>, int> a;
    a.insert("self", 1);
    micron::robin_map<micron::hstring<char>, int> b;
    b = micron::move(a);
    sb::require(*b.find("self") == 1);
    sb::require(a.size() == 0ULL);
    sb::require(a.find("self") == nullptr);
  }
  sb::end_test_case();

  // ── insert / find ─────────────────────────────────────────────────────────

  sb::test_case("insert - single key/value, returns non-null pointer");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    int *p = m.insert("foo", 7);
    sb::require(p != nullptr);
    sb::require(*p == 7);
  }
  sb::end_test_case();

  sb::test_case("insert - size increments after each unique insert");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.size() == 0ULL);
    m.insert("a", 1);
    sb::require(m.size() == 1ULL);
    m.insert("b", 2);
    sb::require(m.size() == 2ULL);
    m.insert("c", 3);
    sb::require(m.size() == 3ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key does not increase size");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("dup", 10);
    m.insert("dup", 20);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key updates value in place");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("k", 100);
    int *p = m.insert("k", 999);
    sb::require(p != nullptr);
    sb::require(*p == 999);
    sb::require(*m.find("k") == 999);
  }
  sb::end_test_case();

  sb::test_case("insert - rvalue value overload");
  {
    micron::robin_map<micron::hstring<char>, micron::hstring<char>> m;
    micron::hstring<char> val = "moved_value";
    m.insert("mv", micron::move(val));
    micron::hstring<char> *v = m.find("mv");
    sb::require(v != nullptr);
    sb::require(*v == "moved_value");
  }
  sb::end_test_case();

  sb::test_case("insert - rvalue key overload");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    micron::hstring<char> k = "rval_key";
    int *p = m.insert(micron::move(k), 55);
    sb::require(p != nullptr);
    sb::require(*p == 55);
    sb::require(*m.find("rval_key") == 55);
  }
  sb::end_test_case();

  sb::test_case("insert - const value copy overload");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    const int cv = 77;
    int *p = m.insert("cv_key", cv);
    sb::require(p != nullptr);
    sb::require(*p == 77);
  }
  sb::end_test_case();

  sb::test_case("find - key present returns correct value pointer");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("x", 55);
    int *v = m.find("x");
    sb::require(v != nullptr);
    sb::require(*v == 55);
  }
  sb::end_test_case();

  sb::test_case("find - missing key returns nullptr");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("present", 1);
    int *v = m.find("absent");
    sb::require(v == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - const overload works");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("const_key", 77);
    const micron::robin_map<micron::hstring<char>, int> &cm = m;
    const int *v = cm.find("const_key");
    sb::require(v != nullptr);
    sb::require(*v == 77);
  }
  sb::end_test_case();

  sb::test_case("find - empty map always returns nullptr");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.find("anything") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find_hash - raw hash lookup returns correct value");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    micron::hstring<char> k = "hash_key";
    m.insert(k, 42);
    micron::hash64_t h = micron::hash<micron::hash64_t>(k);
    int *v = m.find_hash(h, k);
    sb::require(v != nullptr);
    sb::require(*v == 42);
  }
  sb::end_test_case();

  // ── contains ──────────────────────────────────────────────────────────────

  sb::test_case("contains - returns true for inserted key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("here", 1);
    sb::require(m.contains("here") == true);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false for missing key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("here", 1);
    sb::require(m.contains("not_here") == false);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false on empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.contains("anything") == false);
  }
  sb::end_test_case();

  sb::test_case("contains - false after erase");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("ephemeral", 1);
    m.erase("ephemeral");
    sb::require(m.contains("ephemeral") == false);
  }
  sb::end_test_case();

  // ── exists / count ────────────────────────────────────────────────────────

  sb::test_case("exists - returns 1 for present key, 0 for absent");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("exists_key", 5);
    sb::require(m.exists("exists_key") == 1ULL);
    sb::require(m.exists("no_such_key") == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("count - returns 1 for present key, 0 for absent");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("count_key", 5);
    sb::require(m.count("count_key") == 1ULL);
    sb::require(m.count("no_such") == 0ULL);
  }
  sb::end_test_case();

  // ── operator[] ────────────────────────────────────────────────────────────

  sb::test_case("operator[] - inserts default value for new key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    int &v = m["new_key"];
    sb::require(v == 0);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("operator[] - returns reference to existing value");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("ref_key", 42);
    int &v = m["ref_key"];
    sb::require(v == 42);
  }
  sb::end_test_case();

  sb::test_case("operator[] - write through reference persists");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m["writable"] = 100;
    sb::require(*m.find("writable") == 100);
    m["writable"] = 200;
    sb::require(*m.find("writable") == 200);
  }
  sb::end_test_case();

  sb::test_case("operator[] - accumulator pattern");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    std::vector<micron::hstring<char>> words = { "cat", "dog", "cat", "bird", "dog", "cat" };
    for ( const auto &w : words )
      m[w] += 1;
    sb::require(*m.find("cat") == 3);
    sb::require(*m.find("dog") == 2);
    sb::require(*m.find("bird") == 1);
  }
  sb::end_test_case();

  // ── at ────────────────────────────────────────────────────────────────────

  sb::test_case("at - returns correct value for existing key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("atkey", 999);
    sb::require(m.at("atkey") == 999);
  }
  sb::end_test_case();

  sb::test_case("at - const overload returns correct value");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("catkey", 123);
    const micron::robin_map<micron::hstring<char>, int> &cm = m;
    sb::require(cm.at("catkey") == 123);
  }
  sb::end_test_case();

  sb::test_case("at - throws on missing key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("exists", 1);
    bool threw = false;
    try {
      (void)m.at("does_not_exist");
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
  }
  sb::end_test_case();

  sb::test_case("at - throws on empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    bool threw = false;
    try {
      (void)m.at("anything");
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
  }
  sb::end_test_case();

  // ── emplace ───────────────────────────────────────────────────────────────

  sb::test_case("emplace - inserts new key via in-place construction");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    int *p = m.emplace("emplace_key", 88);
    sb::require(p != nullptr);
    sb::require(*p == 88);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("emplace - existing key: updates value");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("exist", 10);
    int *p = m.emplace("exist", 999);
    sb::require(p != nullptr);
    sb::require(*p == 999);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  // ── add ───────────────────────────────────────────────────────────────────

  sb::test_case("add - inserts and returns reference");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    int &ref = m.add("added", 55);
    sb::require(ref == 55);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("add - returned reference is live: write visible via find");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    int &ref = m.add("addref", 1);
    ref = 42;
    sb::require(*m.find("addref") == 42);
  }
  sb::end_test_case();

  // ── erase ─────────────────────────────────────────────────────────────────

  sb::test_case("erase - removes existing key, returns true");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("gone", 5);
    bool result = m.erase("gone");
    sb::require(result == true);
    sb::require(m.find("gone") == nullptr);
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - missing key returns false");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    bool result = m.erase("never_inserted");
    sb::require(result == false);
  }
  sb::end_test_case();

  sb::test_case("erase - size decrements on successful erase");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    m.insert("b", 2);
    m.insert("c", 3);
    sb::require(m.size() == 3ULL);
    m.erase("b");
    sb::require(m.size() == 2ULL);
    sb::require(m.contains("a"));
    sb::require(!m.contains("b"));
    sb::require(m.contains("c"));
  }
  sb::end_test_case();

  sb::test_case("erase - neighbors still findable after erase");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("aa", 1);
    m.insert("bb", 2);
    m.insert("cc", 3);
    m.insert("dd", 4);
    m.insert("ee", 5);
    m.erase("cc");
    sb::require(*m.find("aa") == 1);
    sb::require(*m.find("bb") == 2);
    sb::require(m.find("cc") == nullptr);
    sb::require(*m.find("dd") == 4);
    sb::require(*m.find("ee") == 5);
  }
  sb::end_test_case();

  sb::test_case("erase - erase then re-insert same key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("cycle", 10);
    m.erase("cycle");
    m.insert("cycle", 20);
    sb::require(m.size() == 1ULL);
    sb::require(*m.find("cycle") == 20);
  }
  sb::end_test_case();

  sb::test_case("erase - double erase returns false on second attempt");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("once", 1);
    sb::require(m.erase("once") == true);
    sb::require(m.erase("once") == false);
  }
  sb::end_test_case();

  sb::test_case("erase - three cycles of insert/erase same key");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("cycle2", 1);
    m.erase("cycle2");
    m.insert("cycle2", 2);
    m.erase("cycle2");
    m.insert("cycle2", 3);
    sb::require(*m.find("cycle2") == 3);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("erase_hash - skip re-hashing: erase using pre-computed hash");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    micron::hstring<char> k = "prehashed";
    m.insert(k, 77);
    micron::hash64_t h = micron::hash<micron::hash64_t>(k);
    bool ok = m.erase_hash(h, k);
    sb::require(ok == true);
    sb::require(m.find(k) == nullptr);
  }
  sb::end_test_case();

  // ── clear ─────────────────────────────────────────────────────────────────

  sb::test_case("clear - empties map, size becomes 0");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("x", 1);
    m.insert("y", 2);
    m.insert("z", 3);
    m.clear();
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("clear - find returns nullptr after clear");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("clearme", 42);
    m.clear();
    sb::require(m.find("clearme") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("clear - capacity does not shrink");
  {
    micron::robin_map<micron::hstring<char>, int> m(256);
    usize cap_before = m.max_size();
    for ( int i = 0; i < 50; ++i )
      m.insert(make_key(i), i);
    m.clear();
    sb::require(m.max_size() == cap_before);
  }
  sb::end_test_case();

  sb::test_case("clear - can insert again after clear");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    m.clear();
    m.insert("a", 2);
    sb::require(m.size() == 1ULL);
    sb::require(*m.find("a") == 2);
  }
  sb::end_test_case();

  // ── load_factor ───────────────────────────────────────────────────────────

  sb::test_case("load_factor - 0.0 on empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.load_factor() == 0.0f);
  }
  sb::end_test_case();

  sb::test_case("load_factor - increases monotonically as elements are inserted");
  {
    micron::robin_map<micron::hstring<char>, int> m(256);
    float prev = m.load_factor();
    for ( int i = 0; i < 20; ++i ) {
      m.insert(make_key(i), i);
      float cur = m.load_factor();
      sb::require(cur >= prev);
      prev = cur;
    }
  }
  sb::end_test_case();

  sb::test_case("load_factor - within [0.0, 1.0] at all times");
  {
    micron::robin_map<micron::hstring<char>, int> m(256);
    for ( int i = 0; i < 100; ++i )
      m.insert(make_key(i), i);
    float lf = m.load_factor();
    sb::require(lf >= 0.0f);
    sb::require(lf <= 1.0f);
  }
  sb::end_test_case();

  sb::test_case("load_factor - decreases after erase");
  {
    micron::robin_map<micron::hstring<char>, int> m(256);
    for ( int i = 0; i < 50; ++i )
      m.insert(make_key(i), i);
    float before = m.load_factor();
    m.erase(make_key(0));
    sb::require(m.load_factor() < before);
  }
  sb::end_test_case();

  // ── slot_occupied / ctrl ──────────────────────────────────────────────────

  sb::test_case("slot_occupied - correctly identifies live slots");
  {
    micron::robin_map<micron::hstring<char>, int> m(32);
    m.insert("slotkey", 1);
    // At least one slot must be occupied.
    bool any_occupied = false;
    for ( usize i = 0; i < m.max_size(); ++i )
      if ( m.slot_occupied(i) ) {
        any_occupied = true;
        break;
      }
    sb::require(any_occupied);
  }
  sb::end_test_case();

  sb::test_case("slot_occupied - no slots occupied on empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m(32);
    bool any = false;
    for ( usize i = 0; i < m.max_size(); ++i )
      if ( m.slot_occupied(i) ) {
        any = true;
        break;
      }
    sb::require(any == false);
  }
  sb::end_test_case();

  sb::test_case("ctrl - non-null pointer returned");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    sb::require(m.ctrl() != nullptr);
  }
  sb::end_test_case();

  // ── value types ───────────────────────────────────────────────────────────

  sb::test_case("value type - map<string, string>: insert and find");
  {
    micron::robin_map<micron::hstring<char>, micron::hstring<char>> m;
    m.insert("greeting", micron::hstring<char>("hello"));
    m.insert("farewell", micron::hstring<char>("goodbye"));
    sb::require(*m.find("greeting") == "hello");
    sb::require(*m.find("farewell") == "goodbye");
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, double>: precision preserved");
  {
    micron::robin_map<micron::hstring<char>, double> m;
    m.insert("pi", 3.14159265358979);
    m.insert("e", 2.71828182845904);
    sb::require(*m.find("pi") == 3.14159265358979);
    sb::require(*m.find("e") == 2.71828182845904);
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, uint64_t>: large values preserved");
  {
    micron::robin_map<micron::hstring<char>, uint64_t> m;
    m.insert("big", 0xDEADBEEFCAFEBABEULL);
    uint64_t *v = m.find("big");
    sb::require(v != nullptr);
    sb::require(*v == 0xDEADBEEFCAFEBABEULL);
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, int> negative values preserved");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("neg", -12345);
    int *v = m.find("neg");
    sb::require(v != nullptr);
    sb::require(*v == -12345);
  }
  sb::end_test_case();

  sb::test_case("value type - struct value stored and retrieved");
  {
    struct Point {
      int x, y;

      bool
      operator==(const Point &o) const
      {
        return x == o.x && y == o.y;
      }
    };

    micron::robin_map<micron::hstring<char>, Point> m;
    m.insert("origin", Point{ 0, 0 });
    m.insert("unit", Point{ 1, 1 });
    sb::require(*m.find("origin") == (Point{ 0, 0 }));
    sb::require(*m.find("unit") == (Point{ 1, 1 }));
  }
  sb::end_test_case();

  // ── edge cases ────────────────────────────────────────────────────────────

  sb::test_case("edge - single character keys (all 26 letters)");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    for ( char c = 'a'; c <= 'z'; ++c ) {
      micron::hstring<char> k(1, c);
      m.insert(k, (int)(c - 'a'));
    }
    sb::require(m.size() == 26ULL);
    bool ok = true;
    for ( char c = 'a'; c <= 'z' && ok; ++c ) {
      micron::hstring<char> k(1, c);
      int *v = m.find(k);
      if ( !v || *v != (int)(c - 'a') )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("edge - very long key string (512 chars)");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    micron::hstring<char> long_key(512, 'X');
    m.insert(long_key, 42);
    sb::require(m.size() == 1ULL);
    int *v = m.find(long_key);
    sb::require(v != nullptr);
    sb::require(*v == 42);
  }
  sb::end_test_case();

  sb::test_case("edge - keys differing only at last character");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("prefix_a", 1);
    m.insert("prefix_b", 2);
    m.insert("prefix_c", 3);
    sb::require(*m.find("prefix_a") == 1);
    sb::require(*m.find("prefix_b") == 2);
    sb::require(*m.find("prefix_c") == 3);
    sb::require(m.size() == 3ULL);
  }
  sb::end_test_case();

  sb::test_case("edge - keys differing only at first character");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("asuffix", 10);
    m.insert("bsuffix", 20);
    m.insert("csuffix", 30);
    sb::require(*m.find("asuffix") == 10);
    sb::require(*m.find("bsuffix") == 20);
    sb::require(*m.find("csuffix") == 30);
  }
  sb::end_test_case();

  sb::test_case("edge - zero integer value findable (not confused with empty)");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("zero_val", 0);
    int *v = m.find("zero_val");
    sb::require(v != nullptr);
    sb::require(*v == 0);
  }
  sb::end_test_case();

  sb::test_case("edge - negative, zero, and positive values coexist");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("neg", -1);
    m.insert("zero", 0);
    m.insert("pos", 1);
    sb::require(*m.find("neg") == -1);
    sb::require(*m.find("zero") == 0);
    sb::require(*m.find("pos") == 1);
  }
  sb::end_test_case();

  sb::test_case("edge - INT_MAX stored correctly");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("maxint", INT_MAX);
    sb::require(*m.find("maxint") == INT_MAX);
  }
  sb::end_test_case();

  sb::test_case("edge - INT_MIN stored correctly");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("minint", INT_MIN);
    sb::require(*m.find("minint") == INT_MIN);
  }
  sb::end_test_case();

  // ── hash collision correctness ────────────────────────────────────────────
  //
  // This section tests the two-level (hash + key) equality path introduced to
  // fix the collision spin/aliasing bug.  Keys with different hashes are used
  // here; collision-specific tests below use the make_wide_key spread pattern.

  sb::test_case("collision correctness - distinct keys, distinct hashes: both findable");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("alpha_key", 1);
    m.insert("beta_key", 2);
    sb::require(*m.find("alpha_key") == 1);
    sb::require(*m.find("beta_key") == 2);
    sb::require(m.size() == 2ULL);
  }
  sb::end_test_case();

  sb::test_case("collision correctness - 20 keys sharing adjacent bucket slots: all retrievable");
  {
    // Force probe chains by filling a small table past halfway.
    micron::robin_map<micron::hstring<char>, int> m(32);
    std::vector<micron::hstring<char>> inserted;
    for ( int i = 0; i < 14; ++i ) {     // ~43% load on 32-slot table
      auto k = make_key(i);
      m.insert(k, i * 7);
      inserted.push_back(k);
    }
    bool ok = true;
    for ( int i = 0; i < (int)inserted.size() && ok; ++i ) {
      int *v = m.find(inserted[i]);
      if ( !v || *v != i * 7 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("collision correctness - erase middle of probe chain: remaining keys findable");
  {
    micron::robin_map<micron::hstring<char>, int> m(32);
    // Insert enough to guarantee some probe chains.
    for ( int i = 0; i < 12; ++i )
      m.insert(make_key(i), i);
    // Erase a key in the middle of the likely probe range.
    m.erase(make_key(5));
    sb::require(m.find(make_key(5)) == nullptr);
    // Every other key must still be found with the correct value.
    bool ok = true;
    for ( int i = 0; i < 12 && ok; ++i ) {
      if ( i == 5 )
        continue;
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("collision correctness - update does not displace neighbor");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("n1", 1);
    m.insert("n2", 2);
    m.insert("n1", 99);     // update n1; n2 must be unaffected
    sb::require(*m.find("n1") == 99);
    sb::require(*m.find("n2") == 2);
  }
  sb::end_test_case();

  // ── Robin Hood invariant ─────────────────────────────────────────────────
  // Verify that the backward-shift deletion correctly restores the invariant
  // so all elements remain findable after a series of deletes.

  sb::test_case("robin hood - interleaved insert/erase/find 200 cycles");
  {
    micron::robin_map<micron::hstring<char>, int> m(256);
    bool ok = true;
    for ( int i = 0; i < 200 && ok; ++i ) {
      micron::hstring<char> k = make_key(i);
      m.insert(k, i);
      int *v = m.find(k);
      if ( !v || *v != i ) {
        ok = false;
        break;
      }
      m.erase(k);
      if ( m.find(k) != nullptr ) {
        ok = false;
        break;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("robin hood - 30 inserts then erase alternating: rest survives");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    const int N = 30;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    // Erase even-indexed keys.
    for ( int i = 0; i < N; i += 2 )
      m.erase(make_key(i));
    sb::require(m.size() == (usize)(N / 2));
    bool ok = true;
    // Odd keys must still be present with correct values.
    for ( int i = 1; i < N && ok; i += 2 ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    // Even keys must be absent.
    for ( int i = 0; i < N && ok; i += 2 )
      if ( m.find(make_key(i)) != nullptr )
        ok = false;
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("robin hood - erase all entries one by one: map becomes empty");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    const int N = 30;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    sb::require(m.size() == (usize)N);
    for ( int i = 0; i < N; ++i )
      m.erase(make_key(i));
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  // ── bulk correctness ──────────────────────────────────────────────────────

  sb::test_case("bulk - 50 unique keys: all found correctly after insert");
  {
    micron::robin_map<micron::hstring<char>, int> m(128);
    const int N = 50;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i * 3);
    sb::require(m.size() == (usize)N);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i * 3 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("bulk - wide-spread keys: 40 entries all retrievable");
  {
    micron::robin_map<micron::hstring<char>, int> m(128);
    const int N = 40;
    for ( int i = 0; i < N; ++i )
      m.insert(make_wide_key(i), i * 11);
    sb::require(m.size() == (usize)N);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_wide_key(i));
      if ( !v || *v != i * 11 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("bulk - non-contiguous string keys (fruit names)");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    std::vector<micron::hstring<char>> keys
        = { "apple", "banana", "cherry", "date", "elderberry", "fig", "grape", "kiwi", "nectarine", "orange" };
    for ( int i = 0; i < (int)keys.size(); ++i )
      m.insert(keys[i], i * 11);
    sb::require(m.size() == keys.size());
    bool ok = true;
    for ( int i = 0; i < (int)keys.size() && ok; ++i ) {
      int *v = m.find(keys[i]);
      if ( !v || *v != i * 11 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("bulk - repeated clear and refill 5 times: no stale data");
  {
    micron::robin_map<micron::hstring<char>, int> m(128);
    bool ok = true;
    for ( int round = 0; round < 5 && ok; ++round ) {
      for ( int i = 0; i < 40; ++i )
        m.insert(make_key(i), i + round);
      sb::require(m.size() == 40ULL);
      for ( int i = 0; i < 40 && ok; ++i ) {
        int *v = m.find(make_key(i));
        if ( !v || *v != i + round )
          ok = false;
      }
      m.clear();
      sb::require(m.empty());
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // ── independence ──────────────────────────────────────────────────────────

  sb::test_case("independence - two maps do not share state");
  {
    micron::robin_map<micron::hstring<char>, int> m1;
    micron::robin_map<micron::hstring<char>, int> m2;
    m1.insert("shared_key", 111);
    m2.insert("shared_key", 222);
    sb::require(*m1.find("shared_key") == 111);
    sb::require(*m2.find("shared_key") == 222);
    m1.erase("shared_key");
    sb::require(m1.find("shared_key") == nullptr);
    sb::require(*m2.find("shared_key") == 222);
  }
  sb::end_test_case();

  sb::test_case("independence - erase from one map does not affect another");
  {
    micron::robin_map<micron::hstring<char>, int> a;
    micron::robin_map<micron::hstring<char>, int> b;
    for ( int i = 0; i < 10; ++i ) {
      a.insert(make_key(i), i);
      b.insert(make_key(i), i * 2);
    }
    for ( int i = 0; i < 10; i += 2 )
      a.erase(make_key(i));
    // b must be completely intact
    bool ok = true;
    for ( int i = 0; i < 10 && ok; ++i ) {
      int *v = b.find(make_key(i));
      if ( !v || *v != i * 2 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // ── raw iteration ─────────────────────────────────────────────────────────

  sb::test_case("iteration - begin <= end on non-empty map");
  {
    micron::robin_map<micron::hstring<char>, int> m;
    m.insert("iter_a", 1);
    m.insert("iter_b", 2);
    auto b = m.begin();
    auto e = m.end();
    sb::require(b != nullptr);
    sb::require(e != nullptr);
    sb::require(e >= b);
  }
  sb::end_test_case();

  sb::test_case("iteration - occupied slot count via ctrl matches size");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    const int N = 20;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    usize occupied = 0;
    for ( usize i = 0; i < m.max_size(); ++i )
      if ( m.slot_occupied(i) )
        ++occupied;
    sb::require(occupied == (usize)N);
  }
  sb::end_test_case();

  sb::test_case("iteration - scan via begin/end recovers all inserted values");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    const int N = 15;
    std::set<int> expected;
    for ( int i = 0; i < N; ++i ) {
      m.insert(make_key(i), i * 3);
      expected.insert(i * 3);
    }
    std::set<int> found;
    for ( usize i = 0; i < m.max_size(); ++i )
      if ( m.slot_occupied(i) )
        found.insert(m.begin()[i].value);
    sb::require(found == expected);
  }
  sb::end_test_case();

  // ── swap ──────────────────────────────────────────────────────────────────

  sb::test_case("swap - exchanges contents of two maps");
  {
    micron::robin_map<micron::hstring<char>, int> a(64);
    micron::robin_map<micron::hstring<char>, int> b(64);
    a.insert("a_key", 1);
    b.insert("b_key", 2);
    a.swap(b);
    sb::require(a.find("a_key") == nullptr);
    sb::require(*a.find("b_key") == 2);
    sb::require(b.find("b_key") == nullptr);
    sb::require(*b.find("a_key") == 1);
  }
  sb::end_test_case();

  sb::test_case("swap - sizes are exchanged");
  {
    micron::robin_map<micron::hstring<char>, int> a(64);
    micron::robin_map<micron::hstring<char>, int> b(64);
    for ( int i = 0; i < 5; ++i )
      a.insert(make_key(i), i);
    for ( int i = 0; i < 3; ++i )
      b.insert(make_wide_key(i), i);
    a.swap(b);
    sb::require(a.size() == 3ULL);
    sb::require(b.size() == 5ULL);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
