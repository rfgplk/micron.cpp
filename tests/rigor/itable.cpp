// immutable_table_tests.cpp
// Rigorous snowball test suite for micron::immutable_table<K, V>

#include "../../src/maps/itable.hpp"
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

using itab = micron::immutable_table<int, int>;
using utab = micron::immutable_table<unsigned, unsigned>;

// build a table with keys [0, n) mapped to k*10
itab
make_seq(int n)
{
  itab t;
  for ( int i = 0; i < n; ++i )
    t = t.insert(i, i * 10);
  return t;
}

// verify table contains exactly [0, n) with values k*10 and size == n
bool
verify_seq(const itab &t, int n)
{
  if ( (int)t.size() != n )
    return false;
  for ( int i = 0; i < n; ++i ) {
    const int *v = t.find(i);
    if ( !v || *v != i * 10 )
      return false;
  }
  return true;
}

// verify keys [0, n) exist with correct values (may have additional keys)
bool
contains_seq(const itab &t, int n)
{
  for ( int i = 0; i < n; ++i ) {
    const int *v = t.find(i);
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
  sb::print("=== IMMUTABLE_TABLE TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – empty table");
  {
    itab t;
    require(t.size(), usize(0));
    require_true(t.empty());
    require_true(t.identity() == nullptr);
  }
  end_test_case();

  // ================================================================ //
  //  Insert / set                                                     //
  // ================================================================ //
  test_case("insert single element");
  {
    itab t;
    auto t1 = t.insert(42, 100);
    require(t1.size(), usize(1));
    require_false(t1.empty());

    const int *v = t1.find(42);
    require_true(v != nullptr);
    require(*v, 100);

    // original unchanged
    require(t.size(), usize(0));
    require_true(t.find(42) == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert two elements");
  {
    itab t;
    auto t1 = t.insert(1, 10);
    auto t2 = t1.insert(2, 20);

    require(t2.size(), usize(2));
    require(*t2.find(1), 10);
    require(*t2.find(2), 20);

    require(t.size(), usize(0));
    require(t1.size(), usize(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert multiple elements");
  {
    itab t;
    auto t1 = t.insert(1, 10);
    auto t2 = t1.insert(2, 20);
    auto t3 = t2.insert(3, 30);

    require(t3.size(), usize(3));
    require(*t3.find(1), 10);
    require(*t3.find(2), 20);
    require(*t3.find(3), 30);

    // earlier versions intact
    require(t.size(), usize(0));
    require(t1.size(), usize(1));
    require(t2.size(), usize(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert duplicate key updates value, size unchanged");
  {
    auto t = make_seq(5);
    require(t.size(), usize(5));

    auto t2 = t.insert(2, 999);
    require(t2.size(), usize(5));
    require(*t2.find(2), 999);

    // original preserved
    require(*t.find(2), 20);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set() is alias for insert()");
  {
    itab t;
    auto t1 = t.set(10, 100);
    auto t2 = t1.set(20, 200);
    require(t2.size(), usize(2));
    require(*t2.find(10), 100);
    require(*t2.find(20), 200);

    auto t3 = t2.set(10, 999);
    require(*t3.find(10), 999);
    require(t3.size(), usize(2));
  }
  end_test_case();

  // ================================================================ //
  //  Erase                                                            //
  // ================================================================ //
  test_case("erase existing key – original preserved");
  {
    auto t = make_seq(5);
    auto t2 = t.erase(2);

    require(t2.size(), usize(4));
    require_true(t2.find(2) == nullptr);

    require(*t2.find(0), 0);
    require(*t2.find(1), 10);
    require(*t2.find(3), 30);
    require(*t2.find(4), 40);

    // original intact
    require(t.size(), usize(5));
    require(*t.find(0), 0);
    require(*t.find(1), 10);
    require(*t.find(2), 20);
    require(*t.find(3), 30);
    require(*t.find(4), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase non-existing key – no change");
  {
    auto t = make_seq(5);
    auto t2 = t.erase(99);
    require(t2.size(), usize(5));
    require_true(verify_seq(t2, 5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase from empty table – no change");
  {
    itab t;
    auto t2 = t.erase(42);
    require(t2.size(), usize(0));
    require_true(t2.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase single element leaves empty table");
  {
    auto t = itab().insert(1, 10);
    auto t2 = t.erase(1);
    require(t2.size(), usize(0));
    require_true(t2.empty());
    require_true(t2.find(1) == nullptr);

    // original still has the element
    require(t.size(), usize(1));
    require(*t.find(1), 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase all elements one by one");
  {
    auto t = make_seq(5);
    auto t1 = t.erase(0);
    auto t2 = t1.erase(1);
    auto t3 = t2.erase(2);
    auto t4 = t3.erase(3);
    auto t5 = t4.erase(4);

    require(t5.size(), usize(0));
    require_true(t5.empty());

    // original intact
    require(t.size(), usize(5));
    require_true(verify_seq(t, 5));

    // intermediate versions correct
    require(t1.size(), usize(4));
    require(t2.size(), usize(3));
    require(t3.size(), usize(2));
    require(t4.size(), usize(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase then re-insert same key");
  {
    auto t = make_seq(5);
    auto t2 = t.erase(2);
    require_true(t2.find(2) == nullptr);

    auto t3 = t2.insert(2, 999);
    require(t3.size(), usize(5));
    require(*t3.find(2), 999);

    require(*t.find(2), 20);
  }
  end_test_case();

  // ================================================================ //
  //  Find / contains / at / operator[]                                //
  // ================================================================ //
  test_case("find returns nullptr for missing key");
  {
    auto t = make_seq(5);
    require_true(t.find(99) == nullptr);
    require_true(t.find(-1) == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("contains returns correct bool");
  {
    auto t = make_seq(5);
    for ( int i = 0; i < 5; ++i )
      require_true(t.contains(i));
    require_false(t.contains(5));
    require_false(t.contains(-1));
    require_false(t.contains(999));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns correct value");
  {
    auto t = make_seq(5);
    for ( int i = 0; i < 5; ++i )
      require(t.at(i), i * 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on missing key");
  {
    auto t = make_seq(3);
    require_throw([&]() { (void)t.at(99); });
    require_throw([&]() { (void)t.at(-1); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() on empty table throws");
  {
    itab t;
    require_throw([&]() { (void)t.at(0); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] returns value for existing key");
  {
    auto t = make_seq(5);
    require(t[0], 0);
    require(t[3], 30);
    require(t[4], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] returns default for missing key");
  {
    auto t = make_seq(5);
    require(t[99], 0);
    require(t[-1], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] on empty table returns default");
  {
    itab t;
    require(t[42], 0);
  }
  end_test_case();

  // ================================================================ //
  //  Size / empty / capacity / load_factor                            //
  // ================================================================ //
  test_case("size tracks inserts and erases");
  {
    itab t;
    require(t.size(), usize(0));

    auto t1 = t.insert(1, 10);
    require(t1.size(), usize(1));

    auto t2 = t1.insert(2, 20);
    require(t2.size(), usize(2));

    auto t3 = t2.erase(1);
    require(t3.size(), usize(1));

    auto t4 = t3.erase(2);
    require(t4.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("empty() tracks state correctly");
  {
    itab t;
    require_true(t.empty());

    auto t1 = t.insert(1, 10);
    require_false(t1.empty());

    auto t2 = t1.erase(1);
    require_true(t2.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("capacity equals size, load_factor correct");
  {
    itab t;
    require(t.capacity(), usize(0));
    require(t.load_factor(), 0.0f);

    auto t1 = make_seq(10);
    require(t1.capacity(), usize(10));
    require_greater(t1.load_factor(), 0.0f);
  }
  end_test_case();

  // ================================================================ //
  //  Copy / move — O(1)                                               //
  // ================================================================ //
  test_case("copy shares root identity");
  {
    auto t = make_seq(100);
    auto t2 = t;
    require_true(t.identity() == t2.identity());
    require(t.size(), t2.size());
    require_true(t == t2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment shares root");
  {
    auto t = make_seq(50);
    itab t2;
    t2 = t;
    require_true(t.identity() == t2.identity());
    require_true(t == t2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction steals root, source empty");
  {
    auto t = make_seq(10);
    const void *id = t.identity();

    itab t2(micron::move(t));
    require_true(t2.identity() == id);
    require(t2.size(), usize(10));

    require(t.size(), usize(0));
    require_true(t.empty());
    require_true(t.identity() == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment steals root, source empty");
  {
    auto t = make_seq(10);
    const void *id = t.identity();

    itab t2;
    t2 = micron::move(t);
    require_true(t2.identity() == id);
    require(t.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self copy assignment is safe");
  {
    auto t = make_seq(10);
    t = t;
    require(t.size(), usize(10));
    require_true(verify_seq(t, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self move assignment is safe");
  {
    auto t = make_seq(10);
    t = micron::move(t);
    require(t.size(), usize(10));
    require_true(verify_seq(t, 10));
  }
  end_test_case();

  // ================================================================ //
  //  Persistence — multiple versions alive                            //
  // ================================================================ //
  test_case("persistence: versions are independent");
  {
    itab v0;
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
  test_case("persistence: fork from base creates independent branches");
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
  test_case("persistence: identity diverges on mutation");
  {
    auto t = make_seq(10);
    auto t2 = t;
    require_true(t.identity() == t2.identity());

    auto t3 = t2.insert(99, 990);
    require_false(t.identity() == t3.identity());
    require_true(t.identity() == t2.identity());
  }
  end_test_case();

  // ================================================================ //
  //  Equality                                                         //
  // ================================================================ //
  test_case("equality: same content tables are equal");
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
    auto t = make_seq(10);
    auto t2 = t;
    require_true(t == t2);
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
  test_case("equality: empty tables are equal");
  {
    itab a, b;
    require_true(a == b);
  }
  end_test_case();

  // ================================================================ //
  //  Iterator — sorted order                                          //
  //  radix trie with __to_ord gives ascending key order               //
  // ================================================================ //
  test_case("iterator traverses in sorted key order");
  {
    itab t;
    t = t.insert(5, 50);
    t = t.insert(2, 20);
    t = t.insert(8, 80);
    t = t.insert(1, 10);
    t = t.insert(9, 90);
    t = t.insert(3, 30);
    t = t.insert(7, 70);

    int prev = -999;
    int count = 0;
    for ( auto it = t.begin(); it != t.end(); ++it ) {
      require_greater(it.key(), prev);
      require(it.value(), it.key() * 10);
      prev = it.key();
      ++count;
    }
    require(count, 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator on empty table: begin == end");
  {
    itab t;
    require_true(t.begin() == t.end());
    require_true(t.cbegin() == t.cend());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator on single element");
  {
    auto t = itab().insert(42, 420);
    auto it = t.begin();
    require_false(it == t.end());
    require(it.key(), 42);
    require(it.value(), 420);
    ++it;
    require_true(it == t.end());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend match begin/end");
  {
    auto t = make_seq(10);
    auto a = t.begin();
    auto b = t.cbegin();
    int count = 0;
    while ( a != t.end() ) {
      require(a.key(), b.key());
      require(a.value(), b.value());
      ++a;
      ++b;
      ++count;
    }
    require_true(b == t.cend());
    require(count, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("post-increment iterator");
  {
    auto t = make_seq(3);
    auto it = t.begin();
    auto prev = it++;
    require(prev.key(), 0);
    require(it.key(), 1);
  }
  end_test_case();

  // ================================================================ //
  //  Signed key ordering                                              //
  //  the XOR sign-bit trick must produce correct ascending order      //
  // ================================================================ //
  test_case("signed keys: negative before positive in iteration");
  {
    itab t;
    t = t.insert(5, 50);
    t = t.insert(-5, -50);
    t = t.insert(0, 0);
    t = t.insert(-10, -100);
    t = t.insert(10, 100);

    int prev = -9999;
    int count = 0;
    for ( auto it = t.begin(); it != t.end(); ++it ) {
      require_greater(it.key(), prev);
      prev = it.key();
      ++count;
    }
    require(count, 5);

    // first key should be -10
    require(t.begin().key(), -10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("signed keys: full range ordering");
  {
    itab t;
    // insert in scrambled order including extremes
    int keys[] = { 0, -1, 1, -100, 100, -2, 2, -50, 50 };
    for ( int k : keys )
      t = t.insert(k, k * 10);

    int prev = -9999;
    t.for_each([&](int k, int) {
      require_greater(k, prev);
      prev = k;
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("signed keys: find/erase work across sign boundary");
  {
    itab t;
    t = t.insert(-3, -30);
    t = t.insert(-1, -10);
    t = t.insert(0, 0);
    t = t.insert(1, 10);
    t = t.insert(3, 30);

    require(*t.find(-3), -30);
    require(*t.find(0), 0);
    require(*t.find(3), 30);
    require(t.size(), usize(5));

    auto t2 = t.erase(-1);
    require(t2.size(), usize(4));
    require_true(t2.find(-1) == nullptr);
    require(*t2.find(-3), -30);
    require(*t2.find(0), 0);

    // original intact
    require(*t.find(-1), -10);
  }
  end_test_case();

  // ================================================================ //
  //  for_each                                                         //
  // ================================================================ //
  test_case("for_each visits all elements in order");
  {
    auto t = make_seq(10);
    int count = 0;
    int prev_key = -1;
    t.for_each([&](int k, int v) {
      require_greater(k, prev_key);
      require(v, k * 10);
      prev_key = k;
      ++count;
    });
    require(count, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("for_each on empty table visits nothing");
  {
    itab t;
    int count = 0;
    t.for_each([&](int, int) { ++count; });
    require(count, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("for_each on single element");
  {
    auto t = itab().insert(42, 420);
    int count = 0;
    t.for_each([&](int k, int v) {
      require(k, 42);
      require(v, 420);
      ++count;
    });
    require(count, 1);
  }
  end_test_case();

  // ================================================================ //
  //  update / update_or                                               //
  // ================================================================ //
  test_case("update applies function to existing key");
  {
    auto t = make_seq(5);
    auto t2 = t.update(2, [](int v) { return v + 1; });
    require(*t2.find(2), 21);
    require(*t.find(2), 20);
    require(t2.size(), usize(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update on missing key returns same table");
  {
    auto t = make_seq(5);
    auto t2 = t.update(99, [](int v) { return v + 1; });
    require_true(t.identity() == t2.identity());
    require(t2.size(), usize(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update_or applies function to existing key");
  {
    auto t = make_seq(5);
    auto t2 = t.update_or(2, 0, [](int v) { return v * 2; });
    require(*t2.find(2), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update_or uses default for missing key");
  {
    auto t = make_seq(5);
    auto t2 = t.update_or(99, 7, [](int v) { return v * 3; });
    require(t2.size(), usize(6));
    require(*t2.find(99), 21);
  }
  end_test_case();

  // ================================================================ //
  //  emplace                                                          //
  // ================================================================ //
  test_case("emplace constructs value in-place");
  {
    itab t;
    auto t1 = t.emplace(1, 42);
    require(t1.size(), usize(1));
    require(*t1.find(1), 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace updates existing key");
  {
    auto t = make_seq(5);
    auto t2 = t.emplace(2, 999);
    require(t2.size(), usize(5));
    require(*t2.find(2), 999);
    require(*t.find(2), 20);
  }
  end_test_case();

  // ================================================================ //
  //  clear                                                            //
  // ================================================================ //
  test_case("clear returns empty table, original intact");
  {
    auto t = make_seq(10);
    auto t2 = t.clear();
    require(t2.size(), usize(0));
    require_true(t2.empty());
    require(t.size(), usize(10));
    require_true(verify_seq(t, 10));
  }
  end_test_case();

  // ================================================================ //
  //  Different integral types                                         //
  // ================================================================ //
  test_case("unsigned key/value type");
  {
    utab t;
    auto t1 = t.insert(10u, 100u);
    auto t2 = t1.insert(20u, 200u);
    auto t3 = t2.insert(5u, 50u);

    require(t3.size(), usize(3));
    require(*t3.find(10u), 100u);
    require(*t3.find(20u), 200u);
    require(*t3.find(5u), 50u);

    // sorted order
    unsigned prev = 0;
    bool first = true;
    t3.for_each([&](unsigned k, unsigned) {
      if ( !first )
        require_greater(k, prev);
      prev = k;
      first = false;
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("i64 key/value type");
  {
    micron::immutable_table<i64, i64> t;
    auto t1 = t.insert((i64)-1000000000LL, (i64)1);
    auto t2 = t1.insert((i64)1000000000LL, (i64)2);
    auto t3 = t2.insert((i64)0, (i64)0);

    require(t3.size(), usize(3));
    require(*t3.find((i64)0), (i64)0);
    require(*t3.find((i64)-1000000000LL), (i64)1);
    require(*t3.find((i64)1000000000LL), (i64)2);

    // sorted: -1B, 0, +1B
    require(t3.begin().key(), (i64)-1000000000LL);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("u8 key/value type");
  {
    micron::immutable_table<u8, u8> t;
    for ( u8 i = 0; i < 50; ++i )
      t = t.insert(i, (u8)(i * 2));

    require(t.size(), usize(50));
    for ( u8 i = 0; i < 50; ++i )
      require(*t.find(i), (u8)(i * 2));
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases                                                       //
  // ================================================================ //
  test_case("insert and erase same key repeatedly");
  {
    itab t;
    for ( int i = 0; i < 50; ++i ) {
      t = t.insert(42, i);
      require(t.size(), usize(1));
      require(*t.find(42), i);

      t = t.erase(42);
      require(t.size(), usize(0));
      require_true(t.find(42) == nullptr);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("large key range – sparse keys");
  {
    itab t;
    t = t.insert(0, 0);
    t = t.insert(1000000, 1);
    t = t.insert(-1000000, -1);
    require(t.size(), usize(3));
    require(*t.find(0), 0);
    require(*t.find(1000000), 1);
    require(*t.find(-1000000), -1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("keys at integer extremes");
  {
    itab t;
    // INT_MIN and INT_MAX as keys
    constexpr int imin = -2147483647 - 1;
    constexpr int imax = 2147483647;
    t = t.insert(imin, -1);
    t = t.insert(imax, 1);
    t = t.insert(0, 0);

    require(t.size(), usize(3));
    require(*t.find(imin), -1);
    require(*t.find(imax), 1);
    require(*t.find(0), 0);

    // sorted: INT_MIN, 0, INT_MAX
    auto it = t.begin();
    require(it.key(), imin);
    ++it;
    require(it.key(), 0);
    ++it;
    require(it.key(), imax);
    ++it;
    require_true(it == t.end());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("two elements with adjacent keys");
  {
    itab t;
    t = t.insert(0, 0);
    t = t.insert(1, 10);
    require(t.size(), usize(2));
    require(*t.find(0), 0);
    require(*t.find(1), 10);

    auto t2 = t.erase(0);
    require(t2.size(), usize(1));
    require(*t2.find(1), 10);
    require_true(t2.find(0) == nullptr);
  }
  end_test_case();

  // ================================================================ //
  //  Stress tests                                                     //
  // ================================================================ //
  test_case("stress: insert 1000 sequential keys");
  {
    auto t = make_seq(1000);
    require(t.size(), usize(1000));
    require_true(verify_seq(t, 1000));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert 1000 then erase all");
  {
    auto t = make_seq(1000);
    for ( int i = 0; i < 1000; ++i )
      t = t.erase(i);
    require(t.size(), usize(0));
    require_true(t.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert 1000 in reverse order");
  {
    itab t;
    for ( int i = 999; i >= 0; --i )
      t = t.insert(i, i * 10);
    require(t.size(), usize(1000));
    require_true(verify_seq(t, 1000));

    int prev = -1;
    t.for_each([&](int k, int) {
      require_greater(k, prev);
      prev = k;
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: alternating insert/erase maintains consistency");
  {
    itab t;
    for ( int i = 0; i < 500; ++i ) {
      t = t.insert(i, i);
      if ( i > 0 && i % 3 == 0 )
        t = t.erase(i / 3);
    }
    t.for_each([&](int k, int v) {
      require(v, k);
      require_true(t.contains(k));
    });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: 100 versions alive simultaneously");
  {
    itab versions[100];
    versions[0] = itab();
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
    itab branches[50];
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
  test_case("stress: rapid update same key");
  {
    auto t = itab().insert(1, 0);
    for ( int i = 1; i <= 500; ++i )
      t = t.insert(1, i);
    require(t.size(), usize(1));
    require(*t.find(1), 500);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase evens then odds");
  {
    auto t = make_seq(100);
    for ( int i = 0; i < 100; i += 2 )
      t = t.erase(i);
    require(t.size(), usize(50));
    for ( int i = 0; i < 100; i += 2 )
      require_false(t.contains(i));
    for ( int i = 1; i < 100; i += 2 )
      require_true(t.contains(i));

    for ( int i = 1; i < 100; i += 2 )
      t = t.erase(i);
    require(t.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase with persistent base verification");
  {
    auto base = make_seq(50);
    for ( int i = 0; i < 50; ++i ) {
      auto derived = base.erase(i);
      require(derived.size(), usize(49));
      require_true(derived.find(i) == nullptr);
      require(base.size(), usize(50));
      require(*base.find(i), i * 10);
    }
    require_true(verify_seq(base, 50));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: negative key range");
  {
    itab t;
    for ( int i = -500; i < 500; ++i )
      t = t.insert(i, i);
    require(t.size(), usize(1000));

    // verify sorted
    int prev = -9999;
    int count = 0;
    t.for_each([&](int k, int v) {
      require_greater(k, prev);
      require(v, k);
      prev = k;
      ++count;
    });
    require(count, 1000);

    // erase negatives
    for ( int i = -500; i < 0; ++i )
      t = t.erase(i);
    require(t.size(), usize(500));
    require_false(t.contains(-1));
    require_true(t.contains(0));
    require_true(t.contains(499));
  }
  end_test_case();

  // ================================================================ //
  //  Iterator stress                                                  //
  // ================================================================ //
  test_case("stress: iterator over 1000 elements in sorted order");
  {
    auto t = make_seq(1000);
    int prev = -1;
    int count = 0;
    for ( auto it = t.begin(); it != t.end(); ++it ) {
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
    auto t = make_seq(1000);
    auto it = t.begin();
    int fe_count = 0;
    t.for_each([&](int k, int v) {
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
    itab t;
    for ( int i = 0; i < 200; ++i ) {
      t = t.insert(i, i * 7);
      const int *v = t.find(i);
      require_true(v != nullptr);
      require(*v, i * 7);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase then find always returns nullptr");
  {
    auto t = make_seq(200);
    for ( int i = 0; i < 200; ++i ) {
      t = t.erase(i);
      require_true(t.find(i) == nullptr);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: insert preserves existing keys");
  {
    auto t = make_seq(50);
    auto t2 = t.insert(999, 9990);
    require(t2.size(), usize(51));
    require_true(contains_seq(t2, 50));
    require(*t2.find(999), 9990);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase preserves other keys");
  {
    auto t = make_seq(50);
    auto t2 = t.erase(25);
    for ( int i = 0; i < 50; ++i ) {
      if ( i == 25 )
        require_true(t2.find(i) == nullptr);
      else
        require(*t2.find(i), i * 10);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: iterator count matches size()");
  {
    for ( int n : { 0, 1, 5, 10, 50, 100 } ) {
      auto t = make_seq(n);
      int count = 0;
      for ( auto it = t.begin(); it != t.end(); ++it )
        ++count;
      require(count, n);
      require(t.size(), usize(n));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: for_each count matches size()");
  {
    for ( int n : { 0, 1, 5, 10, 50, 100 } ) {
      auto t = make_seq(n);
      int count = 0;
      t.for_each([&](int, int) { ++count; });
      require(count, n);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: duplicate insert doesn't change size");
  {
    auto t = make_seq(20);
    for ( int i = 0; i < 20; ++i ) {
      auto t2 = t.insert(i, 999);
      require(t2.size(), t.size());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase non-existing doesn't change size");
  {
    auto t = make_seq(20);
    for ( int i = 100; i < 120; ++i ) {
      auto t2 = t.erase(i);
      require(t2.size(), t.size());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: insert + erase same key is identity on size");
  {
    auto t = make_seq(10);
    for ( int i = 100; i < 150; ++i ) {
      auto t2 = t.insert(i, i);
      auto t3 = t2.erase(i);
      require(t3.size(), t.size());
    }
  }
  end_test_case();

  sb::print("=== ALL IMMUTABLE_TABLE TESTS PASSED ===");
  return 1;
}
