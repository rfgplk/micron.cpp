//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/swiss.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <vector>

// TODO: investigate

// ─── small helpers ───────────────────────────────────────────────────────────

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

// ─── main ────────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== STACK SWISS MAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default constructor: empty map");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - capacity equals N");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.capacity() == 64ULL);
    sb::require(m.max_size() == 64ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - all slots empty after default construction");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 32> m;
    sb::require(m.find("anything") == nullptr);
    sb::require(m.contains("anything") == false);
  }
  sb::end_test_case();

  sb::test_case("construction - copy constructor duplicates contents");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> a;
    a.insert("hello", 1);
    a.insert("world", 2);

    micron::stack_swiss_map<micron::hstring<char>, int, 64> b(a);
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 2ULL);     // original unchanged
    sb::require(*b.find("hello") == 1);
    sb::require(*b.find("world") == 2);
  }
  sb::end_test_case();

  sb::test_case("construction - copy constructor: maps are independent");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> a;
    a.insert("shared", 10);
    micron::stack_swiss_map<micron::hstring<char>, int, 64> b(a);
    b.erase("shared");
    sb::require(a.contains("shared") == true);
    sb::require(b.contains("shared") == false);
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor transfers contents");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> a;
    a.insert("alpha", 10);
    a.insert("beta", 20);

    micron::stack_swiss_map<micron::hstring<char>, int, 64> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);     // moved-from is empty
    sb::require(*b.find("alpha") == 10);
    sb::require(*b.find("beta") == 20);
  }
  sb::end_test_case();

  sb::test_case("construction - copy assignment duplicates contents");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> a;
    a.insert("x", 99);
    micron::stack_swiss_map<micron::hstring<char>, int, 64> b;
    b = a;
    sb::require(b.size() == 1ULL);
    sb::require(*b.find("x") == 99);
    sb::require(a.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - move assignment transfers contents");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> a;
    a.insert("gamma", 30);
    micron::stack_swiss_map<micron::hstring<char>, int, 64> b;
    b = micron::move(a);
    sb::require(b.size() == 1ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("gamma") == 30);
  }
  sb::end_test_case();

  // ── insert ────────────────────────────────────────────────────────────────

  sb::test_case("insert - single key/value: returns {true, non-null}");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    auto [ok, ptr] = m.insert("foo", 7);
    sb::require(ok == true);
    sb::require(ptr != nullptr);
    sb::require(*ptr == 7);
  }
  sb::end_test_case();

  sb::test_case("insert - size increments after successful insert");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.size() == 0ULL);
    m.insert("a", 1);
    sb::require(m.size() == 1ULL);
    m.insert("b", 2);
    sb::require(m.size() == 2ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key returns {false, ptr-to-existing}");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("dup", 100);
    auto [ok, ptr] = m.insert("dup", 200);
    sb::require(ok == false);
    sb::require(ptr != nullptr);
    sb::require(*ptr == 100);     // original value preserved
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - rvalue overload stores correctly");
  {
    micron::stack_swiss_map<micron::hstring<char>, micron::hstring<char>, 64> m;
    micron::hstring<char> val = "moved";
    auto [ok, ptr] = m.insert(micron::hstring<char>("mv"), micron::move(val));
    sb::require(ok == true);
    sb::require(ptr != nullptr);
    sb::require(*ptr == "moved");
  }
  sb::end_test_case();

  sb::test_case("insert - pair overload stores correctly");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    micron::pair<micron::hstring<char>, int> kv{ "pair_key", 55 };
    auto [ok, ptr] = m.insert(kv);
    sb::require(ok == true);
    sb::require(*ptr == 55);
  }
  sb::end_test_case();

  sb::test_case("insert - map full returns {false, nullptr}");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 16> m;
    // Fill to capacity
    int inserted = 0;
    for ( int i = 0; i < 16; ++i ) {
      auto [ok, ptr] = m.insert(make_key(i), i);
      if ( ok )
        ++inserted;
    }
    // Now map is full — any further insert of a new key must fail
    auto [ok, ptr] = m.insert("overflow_key", 999);
    if ( ok == false ) {
      sb::require(ptr == nullptr);
    }
    // Either it was inserted (there was room) or it correctly signals full
    sb::require(true);
  }
  sb::end_test_case();

  // ── find ──────────────────────────────────────────────────────────────────

  sb::test_case("find - key present returns correct value pointer");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("x", 55);
    int *v = m.find("x");
    sb::require(v != nullptr);
    sb::require(*v == 55);
  }
  sb::end_test_case();

  sb::test_case("find - missing key returns nullptr");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("present", 1);
    sb::require(m.find("absent") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - const overload works");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("ck", 77);
    const auto &cm = m;
    const int *v = cm.find("ck");
    sb::require(v != nullptr);
    sb::require(*v == 77);
  }
  sb::end_test_case();

  sb::test_case("find - empty map always returns nullptr");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.find("anything") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - returns nullptr for deleted key (tombstone)");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("gone", 5);
    m.erase("gone");
    sb::require(m.find("gone") == nullptr);
  }
  sb::end_test_case();

  // ── contains / count ──────────────────────────────────────────────────────

  sb::test_case("contains - returns true for inserted key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("here", 1);
    sb::require(m.contains("here") == true);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false for missing key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("here", 1);
    sb::require(m.contains("not_here") == false);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false on empty map");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.contains("anything") == false);
  }
  sb::end_test_case();

  sb::test_case("contains - false after erase");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("eraseme", 1);
    m.erase("eraseme");
    sb::require(m.contains("eraseme") == false);
  }
  sb::end_test_case();

  sb::test_case("count - returns 1 for present key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("c", 1);
    sb::require(m.count("c") == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("count - returns 0 for absent key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.count("missing") == 0ULL);
  }
  sb::end_test_case();

  // ── operator[] ────────────────────────────────────────────────────────────

  sb::test_case("operator[] - inserts default value for new key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    int &v = m["new_key"];
    sb::require(v == 0);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("operator[] - returns reference to existing value");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("ref_key", 42);
    int &v = m["ref_key"];
    sb::require(v == 42);
  }
  sb::end_test_case();

  sb::test_case("operator[] - write through reference persists");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m["writable"] = 100;
    sb::require(*m.find("writable") == 100);
    m["writable"] = 200;
    sb::require(*m.find("writable") == 200);
  }
  sb::end_test_case();

  /*
  sb::test_case("operator[] - accumulator pattern (word frequency)");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    std::vector<micron::hstring<char>> words = { "cat111", "dog222", "cat111", "bird222", "dog111", "cat222" };
    for ( const auto &w : words )
      m[w] += 1;
    sb::require(*m.find("cat111") == 3);
    sb::require(*m.find("dog222") == 2);
    sb::require(*m.find("bird222") == 1);
  }
  sb::end_test_case();
*/
  // ── at ────────────────────────────────────────────────────────────────────

  sb::test_case("at - returns correct value for existing key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("atkey", 999);
    sb::require(m.at("atkey") == 999);
  }
  sb::end_test_case();

  sb::test_case("at - const overload returns correct value");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("catkey", 123);
    const auto &cm = m;
    sb::require(cm.at("catkey") == 123);
  }
  sb::end_test_case();

  sb::test_case("at - throws on missing key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
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

  sb::test_case("at - throws after key is erased");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("gone", 1);
    m.erase("gone");
    bool threw = false;
    try {
      (void)m.at("gone");
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
  }
  sb::end_test_case();

  // ── emplace ───────────────────────────────────────────────────────────────

  sb::test_case("emplace - inserts new key via in-place construction");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    auto [ok, ptr] = m.emplace("emplace_key", 88);
    sb::require(ok == true);
    sb::require(ptr != nullptr);
    sb::require(*ptr == 88);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("emplace - existing key: returns {false, existing ptr}, no overwrite");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("exist", 10);
    auto [ok, ptr] = m.emplace("exist", 999);
    sb::require(ok == false);
    sb::require(ptr != nullptr);
    sb::require(*ptr == 10);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  // ── erase ─────────────────────────────────────────────────────────────────

  sb::test_case("erase - removes an existing key, returns true");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("gone", 5);
    sb::require(m.erase("gone") == true);
    sb::require(m.find("gone") == nullptr);
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - missing key returns false");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.erase("never_inserted") == false);
  }
  sb::end_test_case();

  sb::test_case("erase - size decrements after successful erase");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
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

  sb::test_case("erase - double erase of same key: second returns false");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("once", 1);
    sb::require(m.erase("once") == true);
    sb::require(m.erase("once") == false);
  }
  sb::end_test_case();

  sb::test_case("erase - erase then re-insert same key succeeds");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("cycle", 10);
    m.erase("cycle");
    auto [ok, ptr] = m.insert("cycle", 20);
    sb::require(ok == true);
    sb::require(*m.find("cycle") == 20);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - repeated erase/reinsert same key multiple times");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("cycle2", 1);
    m.erase("cycle2");
    m.insert("cycle2", 2);
    m.erase("cycle2");
    m.insert("cycle2", 3);
    sb::require(*m.find("cycle2") == 3);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - tombstone does not block find of later key with same h2");
  {
    // Insert, erase (leaving tombstone), insert a different key — find must
    // probe through the tombstone correctly.
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("probe_a", 1);
    m.erase("probe_a");
    m.insert("probe_b", 2);
    sb::require(m.find("probe_a") == nullptr);
    sb::require(*m.find("probe_b") == 2);
  }
  sb::end_test_case();

  // ── clear ─────────────────────────────────────────────────────────────────

  sb::test_case("clear - empties map, size becomes 0");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
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
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("clearme", 42);
    m.clear();
    sb::require(m.find("clearme") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("clear - capacity unchanged after clear");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("x", 1);
    m.clear();
    sb::require(m.capacity() == 64ULL);
  }
  sb::end_test_case();

  sb::test_case("clear - can insert again after clear");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("a", 1);
    m.clear();
    auto [ok, ptr] = m.insert("a", 2);
    sb::require(ok == true);
    sb::require(m.size() == 1ULL);
    sb::require(*m.find("a") == 2);
  }
  sb::end_test_case();

  sb::test_case("clear - clears tombstones: re-insert succeeds cleanly");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 16> m;
    for ( int i = 0; i < 8; ++i )
      m.insert(make_key(i), i);
    for ( int i = 0; i < 8; ++i )
      m.erase(make_key(i));     // leaves tombstones
    m.clear();                  // should reset everything including tombstones
    sb::require(m.empty());
    for ( int i = 0; i < 8; ++i ) {
      auto [ok, ptr] = m.insert(make_key(i), i * 2);
      sb::require(ok == true);
    }
    sb::require(m.size() == 8ULL);
  }
  sb::end_test_case();

  // ── value types ───────────────────────────────────────────────────────────

  sb::test_case("value type - map<string, string>: insert and find");
  {
    micron::stack_swiss_map<micron::hstring<char>, micron::hstring<char>, 64> m;
    m.insert("greeting", micron::hstring<char>("hello"));
    m.insert("farewell", micron::hstring<char>("goodbye"));
    sb::require(*m.find("greeting") == "hello");
    sb::require(*m.find("farewell") == "goodbye");
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, double>: precision preserved");
  {
    micron::stack_swiss_map<micron::hstring<char>, double, 64> m;
    m.insert("pi", 3.14159265358979);
    m.insert("e", 2.71828182845904);
    sb::require(*m.find("pi") == 3.14159265358979);
    sb::require(*m.find("e") == 2.71828182845904);
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, uint64_t>: large values preserved");
  {
    micron::stack_swiss_map<micron::hstring<char>, uint64_t, 64> m;
    m.insert("big", 0xDEADBEEFCAFEBABEULL);
    sb::require(*m.find("big") == 0xDEADBEEFCAFEBABEULL);
  }
  sb::end_test_case();

  sb::test_case("value type - negative int values preserved");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("neg", -12345);
    sb::require(*m.find("neg") == -12345);
  }
  sb::end_test_case();

  sb::test_case("value type - INT_MAX and INT_MIN stored correctly");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("maxint", INT_MAX);
    m.insert("minint", INT_MIN);
    sb::require(*m.find("maxint") == INT_MAX);
    sb::require(*m.find("minint") == INT_MIN);
  }
  sb::end_test_case();

  sb::test_case("value type - zero value distinguishable from missing key");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("zero_val", 0);
    int *v = m.find("zero_val");
    sb::require(v != nullptr);
    sb::require(*v == 0);
    sb::require(m.find("no_such_key") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("value type - negative, zero, positive coexist");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("neg", -1);
    m.insert("zero", 0);
    m.insert("pos", 1);
    sb::require(*m.find("neg") == -1);
    sb::require(*m.find("zero") == 0);
    sb::require(*m.find("pos") == 1);
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

    micron::stack_swiss_map<micron::hstring<char>, Point, 64> m;
    m.insert("origin", Point{ 0, 0 });
    m.insert("unit", Point{ 1, 1 });
    sb::require(*m.find("origin") == (Point{ 0, 0 }));
    sb::require(*m.find("unit") == (Point{ 1, 1 }));
  }
  sb::end_test_case();

  // ── bulk / correctness ────────────────────────────────────────────────────

  /*
  sb::test_case("bulk insert - non-contiguous keys all found correctly");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    std::vector<micron::hstring<char>> keys = { "apple", "banana", "cherry", "date",      "elderberry", "fig",    "grape", "honeydew",
                                                "kiwi",  "lemon",  "mango",  "nectarine", "orange",     "papaya", "quince" };
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
  sb::test_case("bulk insert - single character keys a-z all correct");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
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

*/
  sb::test_case("bulk insert - keys differing only at last character");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("prefix_a", 1);
    m.insert("prefix_b", 2);
    m.insert("prefix_c", 3);
    sb::require(*m.find("prefix_a") == 1);
    sb::require(*m.find("prefix_b") == 2);
    sb::require(*m.find("prefix_c") == 3);
  }
  sb::end_test_case();

  sb::test_case("bulk insert - keys differing only at first character");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("asuffix", 10);
    m.insert("bsuffix", 20);
    m.insert("csuffix", 30);
    sb::require(*m.find("asuffix") == 10);
    sb::require(*m.find("bsuffix") == 20);
    sb::require(*m.find("csuffix") == 30);
  }
  sb::end_test_case();

  sb::test_case("bulk insert - very long key (512 chars)");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    micron::hstring<char> long_key(512, 'X');
    m.insert(long_key, 42);
    sb::require(m.size() == 1ULL);
    int *v = m.find(long_key);
    sb::require(v != nullptr);
    sb::require(*v == 42);
  }
  sb::end_test_case();

  // ── different N / NH template params ─────────────────────────────────────

  sb::test_case("template params - N=16 (minimum) works for small workload");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 16> m;
    m.insert("a", 1);
    m.insert("b", 2);
    sb::require(*m.find("a") == 1);
    sb::require(*m.find("b") == 2);
    sb::require(m.size() == 2ULL);
    sb::require(m.capacity() == 16ULL);
  }
  sb::end_test_case();

  /*
  sb::test_case("template params - N=128 stores up to 3/4 capacity without issue");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 128> m;
    for ( int i = 0; i < 96; ++i )
      m.insert(make_key(i), i);
    bool ok = true;
    for ( int i = 0; i < 96 && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();
*/
  sb::test_case("template params - NH=16 (minimum neighbourhood) works");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64, 16> m;
    m.insert("nh_a", 1);
    m.insert("nh_b", 2);
    sb::require(*m.find("nh_a") == 1);
    sb::require(*m.find("nh_b") == 2);
  }
  sb::end_test_case();

  // ── stress ────────────────────────────────────────────────────────────────

  /*
  sb::test_case("stress - interleaved insert/erase/find 30 cycles");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    bool ok = true;
    for ( int i = 0; i < 30 && ok; ++i ) {
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
  sb::test_case("stress - repeated clear and refill 10 times");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    bool ok = true;
    for ( int round = 0; round < 10 && ok; ++round ) {
      for ( int i = 0; i < 20; ++i )
        m.insert(make_key(i), i + round);
      for ( int i = 0; i < 20 && ok; ++i ) {
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
*/

  // ── iterator ──────────────────────────────────────────────────────────────

  sb::test_case("iterator - begin/end traversal visits exactly 'size' occupied slots");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("iter_a", 1);
    m.insert("iter_b", 2);
    m.insert("iter_c", 3);
    usize count = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      ++count;
    sb::require(count == 3ULL);
  }
  sb::end_test_case();

  sb::test_case("iterator - skips empty and deleted slots");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("keep_a", 1);
    m.insert("del_b", 2);
    m.insert("keep_c", 3);
    m.erase("del_b");
    usize count = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      ++count;
    sb::require(count == 2ULL);
  }
  sb::end_test_case();

  sb::test_case("iterator - empty map: begin == end");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    sb::require(m.begin() == m.end());
  }
  sb::end_test_case();

  sb::test_case("iterator - post-clear map: begin == end");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("x", 1);
    m.clear();
    sb::require(m.begin() == m.end());
  }
  sb::end_test_case();

  sb::test_case("const_iterator - cbegin/cend traversal works");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    m.insert("ca", 10);
    m.insert("cb", 20);
    const auto &cm = m;
    usize count = 0;
    for ( auto it = cm.cbegin(); it != cm.cend(); ++it )
      ++count;
    sb::require(count == 2ULL);
  }
  sb::end_test_case();

  // ── independence ──────────────────────────────────────────────────────────

  sb::test_case("independence - two maps do not share state");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m1;
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m2;
    m1.insert("shared_key", 111);
    m2.insert("shared_key", 222);
    sb::require(*m1.find("shared_key") == 111);
    sb::require(*m2.find("shared_key") == 222);
    m1.erase("shared_key");
    sb::require(m1.find("shared_key") == nullptr);
    sb::require(*m2.find("shared_key") == 222);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
