// immutable_map_tests.cpp
// Rigorous snowball test suite for micron::immutable_map<K, V>

#include "../../src/maps/immutable.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_smaller;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Helpers                                                            //
// ------------------------------------------------------------------ //
namespace
{

using imap = micron::immutable_map<int, int>;
using smap = micron::immutable_map<int, const char *>;

// build a map with keys [0, n) mapped to k*10
imap
make_seq(int n)
{
  imap m;
  for ( int i = 0; i < n; ++i )
    m = m.insert(i, i * 10);
  return m;
}

// verify map contains exactly [0, n) with values k*10 and size == n
bool
verify_seq(const imap &m, int n)
{
  if ( (int)m.size() != n )
    return false;
  for ( int i = 0; i < n; ++i ) {
    const int *v = m.find(i);
    if ( !v || *v != i * 10 )
      return false;
  }
  return true;
}

// verify map contains keys [0, n) with values k*10 (may have other keys too)
bool
contains_seq(const imap &m, int n)
{
  for ( int i = 0; i < n; ++i ) {
    const int *v = m.find(i);
    if ( !v || *v != i * 10 )
      return false;
  }
  return true;
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== IMMUTABLE_MAP TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – empty map");
  {
    imap m;
    require(m.size(), usize(0));
    require_true(m.empty());
    require_true(m.identity() == nullptr);
  }
  end_test_case();

  // ================================================================ //
  //  Insert / set                                                     //
  // ================================================================ //
  test_case("insert single element");
  {
    imap m;
    auto m1 = m.insert(42, 100);
    require(m1.size(), usize(1));
    require_false(m1.empty());

    const int *v = m1.find(42);
    require_true(v != nullptr);
    require(*v, 100);

    // original unchanged
    require(m.size(), usize(0));
    require_true(m.find(42) == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert multiple elements");
  {
    imap m;
    auto m1 = m.insert(1, 10);
    auto m2 = m1.insert(2, 20);
    auto m3 = m2.insert(3, 30);

    require(m3.size(), usize(3));
    require(*m3.find(1), 10);
    require(*m3.find(2), 20);
    require(*m3.find(3), 30);

    // earlier versions intact
    require(m.size(), usize(0));
    require(m1.size(), usize(1));
    require(m2.size(), usize(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert duplicate key updates value, size unchanged");
  {
    auto m = make_seq(5);
    require(m.size(), usize(5));

    auto m2 = m.insert(2, 999);
    require(m2.size(), usize(5));
    require(*m2.find(2), 999);

    // original preserved
    require(*m.find(2), 20);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set() is alias for insert()");
  {
    imap m;
    auto m1 = m.set(10, 100);
    auto m2 = m1.set(20, 200);
    require(m2.size(), usize(2));
    require(*m2.find(10), 100);
    require(*m2.find(20), 200);

    auto m3 = m2.set(10, 999);
    require(*m3.find(10), 999);
    require(m3.size(), usize(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert with rvalue value");
  {
    imap m;
    int val = 42;
    auto m1 = m.insert(1, micron::move(val));
    require(*m1.find(1), 42);
  }
  end_test_case();

  // ================================================================ //
  //  Erase — the core persistence test                                //
  //  These test the retain/release fix in erase()                     //
  // ================================================================ //
  test_case("erase existing key – original preserved");
  {
    auto m = make_seq(5);     // {0:0, 1:10, 2:20, 3:30, 4:40}
    auto m2 = m.erase(2);

    require(m2.size(), usize(4));
    require_true(m2.find(2) == nullptr);

    // other keys still present in m2
    require(*m2.find(0), 0);
    require(*m2.find(1), 10);
    require(*m2.find(3), 30);
    require(*m2.find(4), 40);

    // CRITICAL: original map is completely intact
    require(m.size(), usize(5));
    require(*m.find(0), 0);
    require(*m.find(1), 10);
    require(*m.find(2), 20);
    require(*m.find(3), 30);
    require(*m.find(4), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase does not share root with original");
  {
    auto m = make_seq(5);
    auto m2 = m.erase(2);
    // m2 must have a DIFFERENT root — erase must path-copy
    require_false(m.identity() == m2.identity());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase non-existing key – no change");
  {
    auto m = make_seq(5);
    auto m2 = m.erase(99);
    require(m2.size(), usize(5));
    require_true(verify_seq(m2, 5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase from empty map – no change");
  {
    imap m;
    auto m2 = m.erase(42);
    require(m2.size(), usize(0));
    require_true(m2.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase single element leaves empty map");
  {
    auto m = imap().insert(1, 10);
    auto m2 = m.erase(1);
    require(m2.size(), usize(0));
    require_true(m2.empty());
    require_true(m2.find(1) == nullptr);

    // original still has the element
    require(m.size(), usize(1));
    require(*m.find(1), 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase all elements one by one – originals intact");
  {
    auto m = make_seq(5);
    auto m1 = m.erase(0);
    auto m2 = m1.erase(1);
    auto m3 = m2.erase(2);
    auto m4 = m3.erase(3);
    auto m5 = m4.erase(4);

    require(m5.size(), usize(0));
    require_true(m5.empty());

    // every intermediate version still has correct content
    require(m.size(), usize(5));
    require_true(verify_seq(m, 5));
    require(m1.size(), usize(4));
    require(m2.size(), usize(3));
    require(m3.size(), usize(2));
    require(m4.size(), usize(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase first, middle, last keys from same base");
  {
    auto m = make_seq(10);

    auto a = m.erase(0);     // first
    require(a.size(), usize(9));
    require_true(a.find(0) == nullptr);
    require(*a.find(1), 10);

    auto b = m.erase(5);     // middle
    require(b.size(), usize(9));
    require_true(b.find(5) == nullptr);
    require(*b.find(4), 40);
    require(*b.find(6), 60);

    auto c = m.erase(9);     // last
    require(c.size(), usize(9));
    require_true(c.find(9) == nullptr);
    require(*c.find(8), 80);

    // all three derived from same base — base intact
    require(m.size(), usize(10));
    require_true(verify_seq(m, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase then insert same key – round trip");
  {
    auto m = make_seq(5);
    auto m2 = m.erase(2);
    require_true(m2.find(2) == nullptr);

    auto m3 = m2.insert(2, 999);
    require(m3.size(), usize(5));
    require(*m3.find(2), 999);

    // original had different value for key 2
    require(*m.find(2), 20);
  }
  end_test_case();

  // ================================================================ //
  //  Find / contains / at / operator[]                                //
  // ================================================================ //
  test_case("find returns nullptr for missing key");
  {
    auto m = make_seq(5);
    require_true(m.find(99) == nullptr);
    require_true(m.find(-1) == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("contains returns correct bool");
  {
    auto m = make_seq(5);
    for ( int i = 0; i < 5; ++i )
      require_true(m.contains(i));
    require_false(m.contains(5));
    require_false(m.contains(-1));
    require_false(m.contains(999));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns correct reference");
  {
    auto m = make_seq(5);
    for ( int i = 0; i < 5; ++i )
      require(m.at(i), i * 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on missing key");
  {
    auto m = make_seq(3);
    require_throw([&]() { (void)m.at(99); });
    require_throw([&]() { (void)m.at(-1); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() on empty map throws");
  {
    imap m;
    require_throw([&]() { (void)m.at(0); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] returns value for existing key");
  {
    auto m = make_seq(5);
    require(m[0], 0);
    require(m[3], 30);
    require(m[4], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] returns default for missing key");
  {
    auto m = make_seq(5);
    require(m[99], 0);
    require(m[-1], 0);
  }
  end_test_case();

  // ================================================================ //
  //  Size / empty / capacity / load_factor                            //
  // ================================================================ //
  test_case("size tracks inserts and erases");
  {
    imap m;
    require(m.size(), usize(0));

    auto m1 = m.insert(1, 10);
    require(m1.size(), usize(1));

    auto m2 = m1.insert(2, 20);
    require(m2.size(), usize(2));

    auto m3 = m2.erase(1);
    require(m3.size(), usize(1));

    auto m4 = m3.erase(2);
    require(m4.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("empty() tracks state correctly");
  {
    imap m;
    require_true(m.empty());

    auto m1 = m.insert(1, 10);
    require_false(m1.empty());

    auto m2 = m1.erase(1);
    require_true(m2.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("capacity equals size, load_factor correct");
  {
    imap m;
    require(m.capacity(), usize(0));
    require(m.load_factor(), 0.0f);

    auto m1 = make_seq(10);
    require(m1.capacity(), usize(10));
    require_greater(m1.load_factor(), 0.0f);
  }
  end_test_case();

  // ================================================================ //
  //  Copy / move — O(1) structural sharing                            //
  // ================================================================ //
  test_case("copy is O(1) – shares root identity");
  {
    auto m = make_seq(100);
    auto m2 = m;
    require_true(m.identity() == m2.identity());
    require(m.size(), m2.size());
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment shares root");
  {
    auto m = make_seq(50);
    imap m2;
    m2 = m;
    require_true(m.identity() == m2.identity());
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction steals root, source empty");
  {
    auto m = make_seq(10);
    const void *id = m.identity();

    imap m2(micron::move(m));
    require_true(m2.identity() == id);
    require(m2.size(), usize(10));

    require(m.size(), usize(0));
    require_true(m.empty());
    require_true(m.identity() == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment steals root, source empty");
  {
    auto m = make_seq(10);
    const void *id = m.identity();

    imap m2;
    m2 = micron::move(m);
    require_true(m2.identity() == id);
    require(m.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self copy assignment is safe");
  {
    auto m = make_seq(10);
    m = m;
    require(m.size(), usize(10));
    require_true(verify_seq(m, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self move assignment is safe");
  {
    auto m = make_seq(10);
    m = micron::move(m);
    require(m.size(), usize(10));
    require_true(verify_seq(m, 10));
  }
  end_test_case();

  // ================================================================ //
  //  Persistence — multiple versions alive simultaneously             //
  // ================================================================ //
  test_case("persistence: versions are independent");
  {
    imap v0;
    auto v1 = v0.insert(1, 10);
    auto v2 = v1.insert(2, 20);
    auto v3 = v2.insert(3, 30);
    auto v4 = v3.erase(1);
    auto v5 = v4.insert(1, 999);

    require(v0.size(), usize(0));
    require(v1.size(), usize(1));
    require(v2.size(), usize(2));
    require(v3.size(), usize(3));
    require(v4.size(), usize(2));
    require(v5.size(), usize(3));

    require(*v3.find(1), 10);
    require(*v5.find(1), 999);
    require_true(v4.find(1) == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("persistence: insert on old version creates new branch");
  {
    auto base = make_seq(5);
    auto branch_a = base.insert(100, 1000);
    auto branch_b = base.insert(200, 2000);

    require(branch_a.size(), usize(6));
    require(branch_b.size(), usize(6));
    require(base.size(), usize(5));

    require_true(branch_a.contains(100));
    require_false(branch_a.contains(200));
    require_true(branch_b.contains(200));
    require_false(branch_b.contains(100));
    require_false(base.contains(100));
    require_false(base.contains(200));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("persistence: structural sharing – identity diverges on mutation");
  {
    auto m = make_seq(10);
    auto m2 = m;
    require_true(m.identity() == m2.identity());

    auto m3 = m2.insert(99, 990);
    require_false(m.identity() == m3.identity());
    require_true(m.identity() == m2.identity());
  }
  end_test_case();

  // ================================================================ //
  //  Equality                                                         //
  // ================================================================ //
  test_case("equality: same content maps are equal");
  {
    auto a = make_seq(10);
    auto b = make_seq(10);
    require_true(a == b);
    require_false(a != b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: shared root is equal");
  {
    auto m = make_seq(10);
    auto m2 = m;
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: different sizes are unequal");
  {
    auto a = make_seq(5);
    auto b = make_seq(6);
    require_false(a == b);
    require_true(a != b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: same size, different values are unequal");
  {
    auto a = make_seq(5);
    auto b = a.insert(2, 999);
    require_false(a == b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: empty maps are equal");
  {
    imap a, b;
    require_true(a == b);
  }
  end_test_case();

  // ================================================================ //
  //  Iterator — in-order traversal                                    //
  // ================================================================ //
  test_case("iterator traverses in sorted key order");
  {
    imap m;
    m = m.insert(5, 50);
    m = m.insert(2, 20);
    m = m.insert(8, 80);
    m = m.insert(1, 10);
    m = m.insert(9, 90);
    m = m.insert(3, 30);
    m = m.insert(7, 70);

    int prev = -999;
    int count = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) {
      require_greater(it.key(), prev);
      require(it.value(), it.key() * 10);
      prev = it.key();
      ++count;
    }
    require(count, 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator on empty map: begin == end");
  {
    imap m;
    require_true(m.begin() == m.end());
    require_true(m.cbegin() == m.cend());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator on single element");
  {
    auto m = imap().insert(42, 420);
    auto it = m.begin();
    require_false(it == m.end());
    require(it.key(), 42);
    require(it.value(), 420);
    ++it;
    require_true(it == m.end());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend match begin/end");
  {
    auto m = make_seq(10);
    auto a = m.begin();
    auto b = m.cbegin();
    int count = 0;
    while ( a != m.end() ) {
      require(a.key(), b.key());
      require(a.value(), b.value());
      ++a;
      ++b;
      ++count;
    }
    require_true(b == m.cend());
    require(count, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("post-increment iterator");
  {
    auto m = make_seq(3);
    auto it = m.begin();
    auto prev = it++;
    require(prev.key(), 0);
    require(it.key(), 1);
  }
  end_test_case();

  // ================================================================ //
  //  for_each                                                         //
  // ================================================================ //
  test_case("for_each visits all elements in order");
  {
    auto m = make_seq(10);
    int count = 0;
    int prev_key = -1;
    m.for_each([&](const int &k, const int &v) {
      require_greater(k, prev_key);
      require(v, k * 10);
      prev_key = k;
      ++count;
    });
    require(count, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("for_each on empty map visits nothing");
  {
    imap m;
    int count = 0;
    m.for_each([&](const int &, const int &) { ++count; });
    require(count, 0);
  }
  end_test_case();

  // ================================================================ //
  //  update / update_or                                               //
  // ================================================================ //
  test_case("update applies function to existing key");
  {
    auto m = make_seq(5);
    auto m2 = m.update(2, [](const int &v) { return v + 1; });
    require(*m2.find(2), 21);
    require(*m.find(2), 20);
    require(m2.size(), usize(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update on missing key returns same map");
  {
    auto m = make_seq(5);
    auto m2 = m.update(99, [](const int &v) { return v + 1; });
    require_true(m == m2);
    require(m2.size(), usize(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update_or applies function to existing key");
  {
    auto m = make_seq(5);
    auto m2 = m.update_or(2, 0, [](const int &v) { return v * 2; });
    require(*m2.find(2), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update_or uses default for missing key");
  {
    auto m = make_seq(5);
    auto m2 = m.update_or(99, 7, [](const int &v) { return v * 3; });
    require(m2.size(), usize(6));
    require(*m2.find(99), 21);
  }
  end_test_case();

  // ================================================================ //
  //  emplace                                                          //
  // ================================================================ //
  test_case("emplace constructs value in-place");
  {
    imap m;
    auto m1 = m.emplace(1, 42);
    require(m1.size(), usize(1));
    require(*m1.find(1), 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace updates existing key");
  {
    auto m = make_seq(5);
    auto m2 = m.emplace(2, 999);
    require(m2.size(), usize(5));
    require(*m2.find(2), 999);
    require(*m.find(2), 20);
  }
  end_test_case();

  // ================================================================ //
  //  clear                                                            //
  // ================================================================ //
  test_case("clear returns empty map, original intact");
  {
    auto m = make_seq(10);
    auto m2 = m.clear();
    require(m2.size(), usize(0));
    require_true(m2.empty());
    require(m.size(), usize(10));
    require_true(verify_seq(m, 10));
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases                                                       //
  // ================================================================ //
  test_case("insert and erase same key repeatedly");
  {
    imap m;
    for ( int i = 0; i < 50; ++i ) {
      m = m.insert(42, i);
      require(m.size(), usize(1));
      require(*m.find(42), i);

      m = m.erase(42);
      require(m.size(), usize(0));
      require_true(m.find(42) == nullptr);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("negative keys work correctly");
  {
    imap m;
    m = m.insert(-10, 100);
    m = m.insert(-5, 50);
    m = m.insert(0, 0);
    m = m.insert(5, -50);
    m = m.insert(10, -100);

    require(m.size(), usize(5));
    require(*m.find(-10), 100);
    require(*m.find(0), 0);
    require(*m.find(10), -100);

    int prev = -999;
    m.for_each([&](const int &k, const int &) {
      require_greater(k, prev);
      prev = k;
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("large key range – sparse keys");
  {
    imap m;
    m = m.insert(0, 0);
    m = m.insert(1000000, 1);
    m = m.insert(-1000000, -1);
    require(m.size(), usize(3));
    require(*m.find(0), 0);
    require(*m.find(1000000), 1);
    require(*m.find(-1000000), -1);
  }
  end_test_case();

  // ================================================================ //
  //  Stress tests                                                     //
  // ================================================================ //
  test_case("stress: insert 1000 sequential keys");
  {
    auto m = make_seq(1000);
    require(m.size(), usize(1000));
    require_true(verify_seq(m, 1000));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert 1000 then erase all");
  {
    auto m = make_seq(1000);
    for ( int i = 0; i < 1000; ++i )
      m = m.erase(i);
    require(m.size(), usize(0));
    require_true(m.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert 1000 in reverse order");
  {
    imap m;
    for ( int i = 999; i >= 0; --i )
      m = m.insert(i, i * 10);
    require(m.size(), usize(1000));
    require_true(verify_seq(m, 1000));

    int prev = -1;
    m.for_each([&](const int &k, const int &) {
      require_greater(k, prev);
      prev = k;
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: alternating insert/erase maintains consistency");
  {
    imap m;
    for ( int i = 0; i < 500; ++i ) {
      m = m.insert(i, i);
      if ( i > 0 && i % 3 == 0 )
        m = m.erase(i / 3);
    }
    m.for_each([&](const int &k, const int &v) {
      require(v, k);
      require_true(m.contains(k));
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: 100 versions alive simultaneously");
  {
    imap versions[100];
    versions[0] = imap();
    for ( int i = 1; i < 100; ++i )
      versions[i] = versions[i - 1].insert(i, i * 10);

    for ( int i = 0; i < 100; ++i )
      require(versions[i].size(), usize(i));

    for ( int k = 1; k <= 50; ++k )
      require(*versions[50].find(k), k * 10);
    require_true(versions[50].find(51) == nullptr);

    for ( int k = 1; k <= 99; ++k )
      require(*versions[99].find(k), k * 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: fork from single base – 50 branches");
  {
    auto base = make_seq(10);
    imap branches[50];
    for ( int i = 0; i < 50; ++i )
      branches[i] = base.insert(1000 + i, i);

    require(base.size(), usize(10));
    require_true(verify_seq(base, 10));

    for ( int i = 0; i < 50; ++i ) {
      require(branches[i].size(), usize(11));
      require(*branches[i].find(1000 + i), i);
      if ( i > 0 )
        require_false(branches[i].contains(1000 + i - 1));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: rapid update same key – version chain");
  {
    auto m = imap().insert(1, 0);
    for ( int i = 1; i <= 500; ++i )
      m = m.insert(1, i);
    require(m.size(), usize(1));
    require(*m.find(1), 500);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase evens then odds");
  {
    auto m = make_seq(100);
    for ( int i = 0; i < 100; i += 2 )
      m = m.erase(i);
    require(m.size(), usize(50));
    for ( int i = 0; i < 100; i += 2 )
      require_false(m.contains(i));
    for ( int i = 1; i < 100; i += 2 )
      require_true(m.contains(i));

    for ( int i = 1; i < 100; i += 2 )
      m = m.erase(i);
    require(m.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase with persistent base verification");
  {
    // keep base alive, erase from copies, verify base after each
    auto base = make_seq(50);
    for ( int i = 0; i < 50; ++i ) {
      auto derived = base.erase(i);
      require(derived.size(), usize(49));
      require_true(derived.find(i) == nullptr);
      // base must still be fully intact
      require(base.size(), usize(50));
      require(*base.find(i), i * 10);
    }
    require_true(verify_seq(base, 50));
  }
  end_test_case();

  // ================================================================ //
  //  Iterator stress                                                  //
  // ================================================================ //
  test_case("stress: iterator over 1000 elements in sorted order");
  {
    auto m = make_seq(1000);
    int prev = -1;
    int count = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) {
      require_greater(it.key(), prev);
      require(it.value(), it.key() * 10);
      prev = it.key();
      ++count;
    }
    require(count, 1000);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: for_each over 1000 elements matches iterator");
  {
    auto m = make_seq(1000);
    auto it = m.begin();
    int fe_count = 0;
    m.for_each([&](const int &k, const int &v) {
      require(k, it.key());
      require(v, it.value());
      ++it;
      ++fe_count;
    });
    require(fe_count, 1000);
  }
  end_test_case();

  // ================================================================ //
  //  Invariants                                                       //
  // ================================================================ //
  test_case("invariant: insert then find always succeeds");
  {
    imap m;
    for ( int i = 0; i < 200; ++i ) {
      m = m.insert(i, i * 7);
      const int *v = m.find(i);
      require_true(v != nullptr);
      require(*v, i * 7);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase then find always returns nullptr");
  {
    auto m = make_seq(200);
    for ( int i = 0; i < 200; ++i ) {
      m = m.erase(i);
      require_true(m.find(i) == nullptr);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: insert preserves existing keys");
  {
    auto m = make_seq(50);
    auto m2 = m.insert(999, 9990);
    // m2 has 51 elements — use contains_seq (doesn't assert size)
    require(m2.size(), usize(51));
    require_true(contains_seq(m2, 50));
    require(*m2.find(999), 9990);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase preserves other keys");
  {
    auto m = make_seq(50);
    auto m2 = m.erase(25);
    for ( int i = 0; i < 50; ++i ) {
      if ( i == 25 )
        require_true(m2.find(i) == nullptr);
      else
        require(*m2.find(i), i * 10);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: iterator count matches size()");
  {
    for ( int n : { 0, 1, 5, 10, 50, 100 } ) {
      auto m = make_seq(n);
      int count = 0;
      for ( auto it = m.begin(); it != m.end(); ++it )
        ++count;
      require(count, n);
      require(m.size(), usize(n));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: for_each count matches size()");
  {
    for ( int n : { 0, 1, 5, 10, 50, 100 } ) {
      auto m = make_seq(n);
      int count = 0;
      m.for_each([&](const int &, const int &) { ++count; });
      require(count, n);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: duplicate insert doesn't change size");
  {
    auto m = make_seq(20);
    for ( int i = 0; i < 20; ++i ) {
      auto m2 = m.insert(i, 999);
      require(m2.size(), m.size());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase non-existing doesn't change size");
  {
    auto m = make_seq(20);
    for ( int i = 100; i < 120; ++i ) {
      auto m2 = m.erase(i);
      require(m2.size(), m.size());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: insert + erase same key is identity on size");
  {
    auto m = make_seq(10);
    for ( int i = 100; i < 150; ++i ) {
      auto m2 = m.insert(i, i);
      auto m3 = m2.erase(i);
      require(m3.size(), m.size());
    }
  }
  end_test_case();

  sb::print("=== ALL IMMUTABLE_MAP TESTS PASSED ===");
  return 1;
}
