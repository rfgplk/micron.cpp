//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/hopscotch.hpp"
#include "../src/string/string.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstring>
#include <set>
#include <vector>

// ─── small helpers ───────────────────────────────────────────────────────────

// Build a key string like "key_0042"
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
  sb::print("=== HOPSCOTCH MAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default constructor: empty map");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - capacity >= min_size after default construction");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.capacity() >= 64ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor transfers contents");
  {
    micron::hopscotch_map<micron::hstring<char>, int> a;
    a.insert("hello", 42);
    a.insert("world", 99);

    micron::hopscotch_map<micron::hstring<char>, int> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);     // moved-from is empty
    sb::require(*b.find("hello") == 42);
    sb::require(*b.find("world") == 99);
  }
  sb::end_test_case();

  sb::test_case("construction - move assignment transfers contents");
  {
    micron::hopscotch_map<micron::hstring<char>, int> a;
    a.insert("alpha", 1);
    a.insert("beta", 2);

    micron::hopscotch_map<micron::hstring<char>, int> b;
    b = micron::move(a);
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("alpha") == 1);
    sb::require(*b.find("beta") == 2);
  }
  sb::end_test_case();

  // ── insert / find ─────────────────────────────────────────────────────────

  sb::test_case("insert - single key/value, returns non-null pointer");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    int *p = m.insert("foo", 7);
    sb::require(p != nullptr);
    sb::require(*p == 7);
  }
  sb::end_test_case();

  sb::test_case("insert - size increments after successful insert");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.size() == 0ULL);
    m.insert("a", 1);
    sb::require(m.size() == 1ULL);
    m.insert("b", 2);
    sb::require(m.size() == 2ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key does not increase size");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("dup", 10);
    m.insert("dup", 20);     // same key
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key returns pointer to existing value");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("k", 100);
    int *p = m.insert("k", 999);
    sb::require(p != nullptr);
    sb::require(*p == 100);     // original value preserved
  }
  sb::end_test_case();

  sb::test_case("find - key present returns correct value pointer");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("x", 55);
    int *v = m.find("x");
    sb::require(v != nullptr);
    sb::require(*v == 55);
  }
  sb::end_test_case();

  sb::test_case("find - missing key returns nullptr");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("present", 1);
    int *v = m.find("absent");
    sb::require(v == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - const overload works");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("const_key", 77);
    const micron::hopscotch_map<micron::hstring<char>, int> &cm = m;
    const int *v = cm.find("const_key");
    sb::require(v != nullptr);
    sb::require(*v == 77);
  }
  sb::end_test_case();

  sb::test_case("find - empty map always returns nullptr");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.find("anything") == nullptr);
  }
  sb::end_test_case();

  // ── contains ──────────────────────────────────────────────────────────────

  sb::test_case("contains - returns true for inserted key");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("here", 1);
    sb::require(m.contains("here") == true);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false for missing key");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("here", 1);
    sb::require(m.contains("not_here") == false);
  }
  sb::end_test_case();

  sb::test_case("contains - returns false on empty map");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.contains("anything") == false);
  }
  sb::end_test_case();

  // ── operator[] ────────────────────────────────────────────────────────────

  sb::test_case("operator[] - inserts default value for new key");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    int &v = m["new_key"];
    sb::require(v == 0);     // default-constructed int
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("operator[] - returns reference to existing value");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("ref_key", 42);
    int &v = m["ref_key"];
    sb::require(v == 42);
  }
  sb::end_test_case();

  sb::test_case("operator[] - write through reference persists");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m["writable"] = 100;
    sb::require(*m.find("writable") == 100);
    m["writable"] = 200;
    sb::require(*m.find("writable") == 200);
  }
  sb::end_test_case();

  // ── at ────────────────────────────────────────────────────────────────────

  sb::test_case("at - returns correct value for existing key");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("atkey", 999);
    sb::require(m.at("atkey") == 999);
  }
  sb::end_test_case();

  sb::test_case("at - const overload returns correct value");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("catkey", 123);
    const micron::hopscotch_map<micron::hstring<char>, int> &cm = m;
    sb::require(cm.at("catkey") == 123);
  }
  sb::end_test_case();

  sb::test_case("at - throws/errors on missing key (must not silently return garbage)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
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

  // ── erase ─────────────────────────────────────────────────────────────────

  sb::test_case("erase - removes an existing key, returns true");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("gone", 5);
    bool result = m.erase("gone");
    sb::require(result == true);
    sb::require(m.find("gone") == nullptr);
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - missing key returns false");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    bool result = m.erase("never_inserted");
    sb::require(result == false);
  }
  sb::end_test_case();

  sb::test_case("erase - size decrements after successful erase");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
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

  sb::test_case("erase - erase then re-insert same key");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("cycle", 10);
    m.erase("cycle");
    m.insert("cycle", 20);
    sb::require(m.size() == 1ULL);
    sb::require(*m.find("cycle") == 20);
  }
  sb::end_test_case();

  /*
  sb::test_case("erase - erase all entries one by one: map becomes empty");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 32;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    sb::require(m.size() == (size_t)N);
    for ( int i = 0; i < N; ++i )
      m.erase(make_key(i));
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();
*/
  sb::test_case("erase - double erase of same key: second returns false");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("once", 1);
    sb::require(m.erase("once") == true);
    sb::require(m.erase("once") == false);
  }
  sb::end_test_case();

  // ── clear ─────────────────────────────────────────────────────────────────

  sb::test_case("clear - empties map, size becomes 0");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
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
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("clearme", 42);
    m.clear();
    sb::require(m.find("clearme") == nullptr);
  }
  sb::end_test_case();

  /*
  sb::test_case("clear - can insert again after clear");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    m.clear();
    m.insert("a", 2);
    sb::require(m.size() == 1ULL);
    sb::require(*m.find("a") == 2);
  }
  sb::end_test_case();
*/
  // ── emplace ───────────────────────────────────────────────────────────────

  sb::test_case("emplace - inserts new key via in-place construction");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    int *p = m.emplace("emplace_key", 88);
    sb::require(p != nullptr);
    sb::require(*p == 88);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("emplace - existing key: returns existing value, no overwrite");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("exist", 10);
    int *p = m.emplace("exist", 999);
    sb::require(p != nullptr);
    sb::require(*p == 10);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  // ── add ───────────────────────────────────────────────────────────────────

  sb::test_case("add - inserts and returns reference");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    int &ref = m.add("added", 55);
    sb::require(ref == 55);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  // ── load_factor ───────────────────────────────────────────────────────────

  sb::test_case("load_factor - 0.0 on empty map");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    sb::require(m.load_factor() == 0.0f);
  }
  sb::end_test_case();

  sb::test_case("load_factor - increases as elements are inserted");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    float prev = m.load_factor();
    for ( int i = 0; i < 10; ++i ) {
      m.insert(make_key(i), i);
      float cur = m.load_factor();
      sb::require(cur >= prev);
      prev = cur;
    }
  }
  sb::end_test_case();

  sb::test_case("load_factor - within [0,1] range");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 50; ++i )
      m.insert(make_key(i), i);
    float lf = m.load_factor();
    sb::require(lf >= 0.0f);
    sb::require(lf <= 1.0f);
  }
  sb::end_test_case();

  // ── bulk insert / correctness ─────────────────────────────────────────────

  /*
  sb::test_case("bulk insert - 100 unique keys all found correctly");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 100;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i * 3);
    sb::require(m.size() == (size_t)N);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i * 3 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();
  sb::test_case("bulk insert - 500 unique keys all found correctly");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 500;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    sb::require(m.size() == (size_t)N);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("bulk insert - 1000 unique keys: none lost after resize");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 1000;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i * 7);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i * 7 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

*/
  sb::test_case("bulk insert - non-contiguous keys (hashes spread out)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
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

  // ── value types ───────────────────────────────────────────────────────────

  sb::test_case("value type - map<string, string>: insert and find");
  {
    micron::hopscotch_map<micron::hstring<char>, micron::hstring<char>> m;
    m.insert("greeting", micron::hstring<char>("hello"));
    m.insert("farewell", micron::hstring<char>("goodbye"));
    sb::require(*m.find("greeting") == "hello");
    sb::require(*m.find("farewell") == "goodbye");
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, double>: precision preserved");
  {
    micron::hopscotch_map<micron::hstring<char>, double> m;
    m.insert("pi", 3.14159265358979);
    m.insert("e", 2.71828182845904);
    double *pi = m.find("pi");
    double *e = m.find("e");
    sb::require(pi != nullptr);
    sb::require(e != nullptr);
    sb::require(*pi == 3.14159265358979);
    sb::require(*e == 2.71828182845904);
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, uint64_t>: large values preserved");
  {
    micron::hopscotch_map<micron::hstring<char>, uint64_t> m;
    m.insert("big", 0xDEADBEEFCAFEBABEULL);
    uint64_t *v = m.find("big");
    sb::require(v != nullptr);
    sb::require(*v == 0xDEADBEEFCAFEBABEULL);
  }
  sb::end_test_case();

  sb::test_case("value type - map<string, int> negative values preserved");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("neg", -12345);
    int *v = m.find("neg");
    sb::require(v != nullptr);
    sb::require(*v == -12345);
  }
  sb::end_test_case();

  // ── rvalue / move insert ──────────────────────────────────────────────────

  sb::test_case("insert rvalue - moved string value stored correctly");
  {
    micron::hopscotch_map<micron::hstring<char>, micron::hstring<char>> m;
    micron::hstring<char> val = "moved_value";
    m.insert("mv", micron::move(val));
    micron::hstring<char> *v = m.find("mv");
    sb::require(v != nullptr);
    sb::require(*v == "moved_value");
  }
  sb::end_test_case();

  // ── insert_asis ───────────────────────────────────────────────────────────

  sb::test_case("insert_asis - raw hash insert, findable by same hash");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    // Derive the hash the same way the map does
    micron::hash64_t hsh = micron::hash<micron::hash64_t>(micron::hstring<char>("raw_key"));
    if ( hsh != 0 ) {
      int *p = m.insert_asis(hsh, 42);
      sb::require(p != nullptr);
      sb::require(*p == 42);
    }
    sb::require(true);     // just survive if hash == 0
  }
  sb::end_test_case();

  // ── stress / resize ───────────────────────────────────────────────────────

  /*
  sb::test_case("resize - insert past 3/4 load triggers resize, all keys survive");
  {
    // Use a small initial capacity to force early resize
    micron::hopscotch_map<micron::hstring<char>, int> m(256);
    const int N = 300;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();
  sb::test_case("resize - multiple resizes: 2000 insertions, all retrievable");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m(64);
    const int N = 2000;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i * 2);
    bool ok = true;
    for ( int i = 0; i < N && ok; ++i ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i * 2 )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

*/
  sb::test_case("stress - interleaved insert/erase/find 500 cycles");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    bool ok = true;
    for ( int i = 0; i < 500 && ok; ++i ) {
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

  /*
  sb::test_case("stress - 200 inserts, erase half, verify remaining");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 200;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    // erase even-indexed keys
    for ( int i = 0; i < N; i += 2 )
      m.erase(make_key(i));
    sb::require(m.size() == (size_t)(N / 2));
    bool ok = true;
    // odd keys must still be present
    for ( int i = 1; i < N && ok; i += 2 ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    // even keys must be gone
    for ( int i = 0; i < N && ok; i += 2 )
      if ( m.find(make_key(i)) != nullptr )
        ok = false;
    sb::require(ok);
  }
  sb::end_test_case();
  sb::test_case("stress - repeated clear and refill 10 times");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    bool ok = true;
    for ( int round = 0; round < 10 && ok; ++round ) {
      for ( int i = 0; i < 100; ++i )
        m.insert(make_key(i), i + round);
      sb::require(m.size() == 100ULL);
      for ( int i = 0; i < 100 && ok; ++i ) {
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
  sb::test_case("stress - operator[] as accumulator (word frequency style)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    std::vector<micron::hstring<char>> words = { "cat", "dog", "cat", "bird", "dog", "cat" };
    for ( const auto &w : words )
      m[w] += 1;
    sb::require(*m.find("cat") == 3);
    sb::require(*m.find("dog") == 2);
    sb::require(*m.find("bird") == 1);
  }
  sb::end_test_case();

  // ── iterator / traversal ──────────────────────────────────────────────────

  sb::test_case("iterator - begin/end span is valid (non-empty map)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("iter_a", 1);
    m.insert("iter_b", 2);
    auto b = m.begin();
    auto e = m.end();
    sb::require(b != nullptr);
    sb::require(e != nullptr);
    sb::require(e >= b);
  }
  sb::end_test_case();
/*
  sb::test_case("iterator - traversal counts exactly 'size' occupied slots");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 20;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    size_t occupied = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      if ( it->key )
        ++occupied;
    sb::require(occupied == (size_t)N);
  }
  sb::end_test_case();
*/
  // ── edge cases ────────────────────────────────────────────────────────────

  sb::test_case("edge - single character keys");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
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
    micron::hopscotch_map<micron::hstring<char>, int> m;
    micron::hstring<char> long_key(512, 'X');
    m.insert(long_key, 42);
    sb::require(m.size() == 1ULL);
    int *v = m.find(long_key);
    sb::require(v != nullptr);
    sb::require(*v == 42);
  }
  sb::end_test_case();

  sb::test_case("edge - keys that differ only at last character");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("prefix_a", 1);
    m.insert("prefix_b", 2);
    m.insert("prefix_c", 3);
    sb::require(*m.find("prefix_a") == 1);
    sb::require(*m.find("prefix_b") == 2);
    sb::require(*m.find("prefix_c") == 3);
    sb::require(m.size() == 3ULL);
  }
  sb::end_test_case();

  sb::test_case("edge - keys that differ only at first character");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("asuffix", 10);
    m.insert("bsuffix", 20);
    m.insert("csuffix", 30);
    sb::require(*m.find("asuffix") == 10);
    sb::require(*m.find("bsuffix") == 20);
    sb::require(*m.find("csuffix") == 30);
  }
  sb::end_test_case();

  sb::test_case("edge - zero-length-value-but-valid-key (insert 0)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("zero_val", 0);
    int *v = m.find("zero_val");
    sb::require(v != nullptr);
    sb::require(*v == 0);
  }
  sb::end_test_case();

  sb::test_case("edge - negative, zero, and positive values coexist");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("neg", -1);
    m.insert("zero", 0);
    m.insert("pos", 1);
    sb::require(*m.find("neg") == -1);
    sb::require(*m.find("zero") == 0);
    sb::require(*m.find("pos") == 1);
  }
  sb::end_test_case();

  sb::test_case("edge - max int value stored correctly");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("maxint", INT_MAX);
    sb::require(*m.find("maxint") == INT_MAX);
  }
  sb::end_test_case();

  sb::test_case("edge - min int value stored correctly");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("minint", INT_MIN);
    sb::require(*m.find("minint") == INT_MIN);
  }
  sb::end_test_case();

  sb::test_case("edge - insert, erase, reinsert with different value");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    m.insert("cycle2", 1);
    m.erase("cycle2");
    m.insert("cycle2", 2);
    m.erase("cycle2");
    m.insert("cycle2", 3);
    sb::require(*m.find("cycle2") == 3);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("edge - capacity never shrinks below min_size after clear");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 500; ++i )
      m.insert(make_key(i), i);
    size_t cap_before_clear = m.capacity();
    m.clear();
    sb::require(m.capacity() >= 64ULL);
    (void)cap_before_clear;
  }
  sb::end_test_case();

  /*
  sb::test_case("edge - find after many erases does not return stale data");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 50; ++i )
      m.insert(make_key(i), i);
    // erase all but key_0025
    for ( int i = 0; i < 50; ++i )
      if ( i != 25 )
        m.erase(make_key(i));
    sb::require(m.size() == 1ULL);
    int *v = m.find(make_key(25));
    sb::require(v != nullptr);
    sb::require(*v == 25);
    for ( int i = 0; i < 50; ++i )
      if ( i != 25 )
        sb::require(m.find(make_key(i)) == nullptr);
  }
  sb::end_test_case();
*/
  sb::test_case("edge - move-assign self: no crash (move assign to different object)");
  {
    micron::hopscotch_map<micron::hstring<char>, int> a;
    a.insert("self", 1);
    micron::hopscotch_map<micron::hstring<char>, int> b;
    b = micron::move(a);
    sb::require(*b.find("self") == 1);
  }
  sb::end_test_case();

  // ── hash collisions / bucket proximity ───────────────────────────────────

  /*
  sb::test_case("collision - many keys mapping to same bucket neighbourhood");
  {
    // Insert a lot of keys to maximise chance of neighbourhood collisions
    micron::hopscotch_map<micron::hstring<char>, int> m;
    const int N = 800;
    for ( int i = 0; i < N; ++i )
      m.insert(make_key(i), i);
    // Spot-check 50 random-ish positions
    bool ok = true;
    for ( int i = 0; i < N && ok; i += 16 ) {
      int *v = m.find(make_key(i));
      if ( !v || *v != i )
        ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();
*/
  // ── pair-type values ──────────────────────────────────────────────────────

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

    micron::hopscotch_map<micron::hstring<char>, Point> m;
    m.insert("origin", Point{ 0, 0 });
    m.insert("unit", Point{ 1, 1 });
    Point *orig = m.find("origin");
    Point *unit = m.find("unit");
    sb::require(orig != nullptr);
    sb::require(unit != nullptr);
    sb::require(*orig == (Point{ 0, 0 }));
    sb::require(*unit == (Point{ 1, 1 }));
  }
  sb::end_test_case();

  // ── multiple maps independent ─────────────────────────────────────────────

  sb::test_case("independence - two maps do not share state");
  {
    micron::hopscotch_map<micron::hstring<char>, int> m1;
    micron::hopscotch_map<micron::hstring<char>, int> m2;
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
