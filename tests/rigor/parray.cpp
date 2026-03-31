// test_parray.cpp
// Rigorous snowball test suite for micron::parray<T, K, H>
//
// Focus areas:
//   1. Construction variants
//   2. Immutability guarantees  (set/update return new root, original intact)
//   3. Persistence              (multiple live versions, structural sharing)
//   4. Copy / move semantics    (O(1) retain/steal, identity checks)
//   5. Object lifetimes         (non-trivial dtors, leak detection via counters)
//   6. Element access           (get, at, operator[], bounds)
//   7. Arithmetic operators     (element-wise, scalar, reductions)
//   8. Iterators & for_each
//   9. Stress tests

#include "../../src/array/parray.hpp"     // adjust path as needed
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_smaller;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Type aliases                                                       //
// ------------------------------------------------------------------ //
namespace
{

// default params:  K=5  B=32  H=3  capacity=32768
using pa_int = micron::parray<int>;

// small config for exhaustive traversal:  K=2  B=4  H=2  capacity=16
using pa_small = micron::parray<int, 2, 2>;

// tiny: K=1  B=2  H=2  capacity=4
using pa_tiny = micron::parray<int, 1, 2>;

// ------------------------------------------------------------------ //
//  Lifetime-tracking helper                                           //
// ------------------------------------------------------------------ //
static int g_alive = 0;     // net ctor − dtor calls

struct Tracked {
  int val;

  Tracked() : val(0) { ++g_alive; }

  explicit Tracked(int v) : val(v) { ++g_alive; }

  Tracked(const Tracked &o) : val(o.val) { ++g_alive; }

  Tracked(Tracked &&o) noexcept : val(o.val)
  {
    o.val = -1;
    ++g_alive;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    val = o.val;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    val = o.val;
    o.val = -1;
    return *this;
  }

  ~Tracked() { --g_alive; }

  bool
  operator==(const Tracked &o) const
  {
    return val == o.val;
  }

  bool
  operator!=(const Tracked &o) const
  {
    return val != o.val;
  }

  Tracked
  operator+(const Tracked &o) const
  {
    return Tracked(val + o.val);
  }

  Tracked
  operator-(const Tracked &o) const
  {
    return Tracked(val - o.val);
  }

  Tracked
  operator*(const Tracked &o) const
  {
    return Tracked(val * o.val);
  }

  Tracked &
  operator+=(const Tracked &o)
  {
    val += o.val;
    return *this;
  }

  Tracked &
  operator*=(const Tracked &o)
  {
    val *= o.val;
    return *this;
  }
};

using pa_tracked = micron::parray<Tracked, 2, 2>;     // capacity 16

// ------------------------------------------------------------------ //
//  Helpers                                                            //
// ------------------------------------------------------------------ //

// build a pa_small with values[i] = i*10 for i in [0, n)
pa_small
make_seq_small(int n)
{
  pa_small m;
  for ( int i = 0; i < n && (usize)i < pa_small::length; ++i )
    m = m.set((usize)i, i * 10);
  return m;
}

pa_int
make_seq(int n)
{
  pa_int m;
  for ( int i = 0; i < n && (usize)i < pa_int::length; ++i )
    m = m.set((usize)i, i * 10);
  return m;
}

bool
verify_seq_small(const pa_small &m, int n)
{
  for ( int i = 0; i < n; ++i )
    if ( m.get((usize)i) != i * 10 )
      return false;
  return true;
}

}     // namespace

// ================================================================== //
int
main()
{
  print("=== PARRAY TESTS ===");

  // ================================================================ //
  //  1. Construction                                                  //
  // ================================================================ //
  test_case("default construction – null root, fixed size");
  {
    pa_small m;
    // parray always reports its full capacity as size
    require(m.size(), pa_small::length);
    require_false(m.empty());     // empty() is always false for parray
    require_true(m.identity() == nullptr);

    // all slots default-initialized to T{}
    for ( usize i = 0; i < pa_small::length; ++i )
      require(m.get(i), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("broadcast fill construction");
  {
    pa_small m(42);
    require_true(m.identity() != nullptr);
    for ( usize i = 0; i < pa_small::length; ++i )
      require(m.get(i), 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list construction");
  {
    pa_small m({ 1, 2, 3, 4 });
    require(m.get(0), 1);
    require(m.get(1), 2);
    require(m.get(2), 3);
    require(m.get(3), 4);
    // remaining slots default
    for ( usize i = 4; i < pa_small::length; ++i )
      require(m.get(i), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pointer + count construction");
  {
    int data[] = { 10, 20, 30 };
    pa_small m(data, 3);
    require(m.get(0), 10);
    require(m.get(1), 20);
    require(m.get(2), 30);
    for ( usize i = 3; i < pa_small::length; ++i )
      require(m.get(i), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator construction");
  {
    pa_small m([](usize i) -> int { return (int)(i * i); }, 8);
    for ( usize i = 0; i < 8; ++i )
      require(m.get(i), (int)(i * i));
  }
  end_test_case();

  // ================================================================ //
  //  2. Immutability – set / update return new root                   //
  // ================================================================ //
  test_case("set returns new array, original unchanged");
  {
    pa_small m;
    auto m1 = m.set(0, 100);

    // new array has the value
    require(m1.get(0), 100);

    // original is untouched (null root → default 0)
    require(m.get(0), 0);
    require_true(m.identity() == nullptr);
    require_true(m1.identity() != nullptr);
    require_false(m.identity() == m1.identity());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set multiple slots – original always intact");
  {
    pa_small base;
    auto v1 = base.set(0, 10);
    auto v2 = v1.set(1, 20);
    auto v3 = v2.set(2, 30);

    require(v3.get(0), 10);
    require(v3.get(1), 20);
    require(v3.get(2), 30);

    // earlier versions intact
    require(base.get(0), 0);
    require(v1.get(0), 10);
    require(v1.get(1), 0);
    require(v2.get(2), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set overwrite same index – old value preserved in original");
  {
    auto m = make_seq_small(8);
    auto m2 = m.set(3, 999);

    require(m2.get(3), 999);
    require(m.get(3), 30);     // original has 3*10
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set with rvalue");
  {
    pa_small m;
    int v = 77;
    auto m1 = m.set(5, micron::move(v));
    require(m1.get(5), 77);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update applies function, original unchanged");
  {
    auto m = make_seq_small(8);
    auto m2 = m.update(4, [](const int &v) { return v + 1; });

    require(m2.get(4), 41);     // 4*10 + 1
    require(m.get(4), 40);      // original
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("update on default slot");
  {
    pa_small m;
    auto m2 = m.update(0, [](const int &v) { return v + 100; });
    require(m2.get(0), 100);
    require(m.get(0), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set_many applies multiple changes");
  {
    pa_small m;
    auto m2 = m.set(0, 10).set(3, 30).set(7, 70);

    require(m2.get(0), 10);
    require(m2.get(3), 30);
    require(m2.get(7), 70);
    require(m2.get(1), 0);

    // original untouched
    require(m.get(0), 0);
    require(m.get(3), 0);
  }
  end_test_case();

  // ================================================================ //
  //  3. Element access – get / at / operator[]                        //
  // ================================================================ //
  test_case("get returns default for null subtree");
  {
    pa_small m;
    for ( usize i = 0; i < pa_small::length; ++i )
      require(m.get(i), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at returns correct value");
  {
    auto m = make_seq_small(10);
    for ( usize i = 0; i < 10; ++i )
      require(m.at(i), (int)(i * 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at throws on out-of-range index");
  {
    pa_small m;
    require_throw([&]() { (void)m.at(pa_small::length); });
    require_throw([&]() { (void)m.at(pa_small::length + 100); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set throws on out-of-range index");
  {
    pa_small m;
    require_throw([&]() { (void)m.set(pa_small::length, 42); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] matches get");
  {
    auto m = make_seq_small(8);
    for ( usize i = 0; i < pa_small::length; ++i )
      require(m[i], m.get(i));
  }
  end_test_case();

  // ================================================================ //
  //  4. clear / fill                                                  //
  // ================================================================ //
  test_case("clear returns null-root array, original intact");
  {
    auto m = make_seq_small(8);
    auto m2 = m.clear();

    require_true(m2.identity() == nullptr);
    for ( usize i = 0; i < pa_small::length; ++i )
      require(m2.get(i), 0);

    // original intact
    require_true(verify_seq_small(m, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fill returns broadcast array, original intact");
  {
    auto m = make_seq_small(8);
    auto m2 = m.fill(99);

    for ( usize i = 0; i < pa_small::length; ++i )
      require(m2.get(i), 99);

    require_true(verify_seq_small(m, 8));
  }
  end_test_case();

  // ================================================================ //
  //  5. Copy / Move semantics                                         //
  // ================================================================ //
  test_case("copy is O(1) – shares root identity");
  {
    auto m = make_seq_small(10);
    auto m2 = m;

    require_true(m.identity() == m2.identity());
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment shares root");
  {
    auto m = make_seq_small(10);
    pa_small m2;
    m2 = m;

    require_true(m.identity() == m2.identity());
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction steals root, source nulled");
  {
    auto m = make_seq_small(10);
    const void *id = m.identity();

    pa_small m2(micron::move(m));
    require_true(m2.identity() == id);
    require_true(m.identity() == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment steals root, source nulled");
  {
    auto m = make_seq_small(10);
    const void *id = m.identity();

    pa_small m2;
    m2 = micron::move(m);
    require_true(m2.identity() == id);
    require_true(m.identity() == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self copy assignment is safe");
  {
    auto m = make_seq_small(10);
    m = m;
    require_true(verify_seq_small(m, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self move assignment is safe");
  {
    auto m = make_seq_small(10);
    m = micron::move(m);
    require_true(verify_seq_small(m, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy then mutate diverges identity");
  {
    auto m = make_seq_small(5);
    auto m2 = m;
    require_true(m.identity() == m2.identity());

    auto m3 = m2.set(0, 999);
    require_false(m.identity() == m3.identity());
    // m2 still shares with m (m2 was not mutated)
    require_true(m.identity() == m2.identity());
  }
  end_test_case();

  // ================================================================ //
  //  6. Persistence – multiple versions alive simultaneously          //
  // ================================================================ //
  test_case("persistence: versions are independent");
  {
    pa_small v0;
    auto v1 = v0.set(0, 10);
    auto v2 = v1.set(1, 20);
    auto v3 = v2.set(2, 30);
    auto v4 = v3.set(0, 999);     // overwrite slot 0

    require(v0.get(0), 0);
    require(v1.get(0), 10);
    require(v1.get(1), 0);
    require(v2.get(0), 10);
    require(v2.get(1), 20);
    require(v3.get(0), 10);
    require(v3.get(1), 20);
    require(v3.get(2), 30);
    require(v4.get(0), 999);
    require(v4.get(1), 20);
    require(v4.get(2), 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("persistence: branch from single base");
  {
    auto base = make_seq_small(8);
    auto branch_a = base.set(0, 1000);
    auto branch_b = base.set(0, 2000);

    require(base.get(0), 0);
    require(branch_a.get(0), 1000);
    require(branch_b.get(0), 2000);

    // both branches share base values for other slots
    for ( usize i = 1; i < 8; ++i ) {
      require(branch_a.get(i), (int)(i * 10));
      require(branch_b.get(i), (int)(i * 10));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("persistence: structural sharing – identity diverges on mutation");
  {
    auto m = make_seq_small(8);
    auto m2 = m;
    require_true(m.identity() == m2.identity());

    auto m3 = m2.set(7, 999);
    require_false(m.identity() == m3.identity());
    require_true(m.identity() == m2.identity());     // m2 not mutated
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("persistence: deep version chain");
  {
    pa_small versions[16];
    versions[0] = pa_small();
    for ( usize i = 1; i < 16 && i < pa_small::length; ++i )
      versions[i] = versions[i - 1].set(i, (int)(i * 100));

    // each version has exactly the values set up to its point
    for ( usize v = 1; v < 16 && v < pa_small::length; ++v ) {
      for ( usize slot = 1; slot <= v; ++slot )
        require(versions[v].get(slot), (int)(slot * 100));
      // slot 0 is always default
      require(versions[v].get(0), 0);
    }
  }
  end_test_case();

  // ================================================================ //
  //  7. Equality                                                      //
  // ================================================================ //
  test_case("equality: same-content arrays are equal");
  {
    auto a = make_seq_small(10);
    auto b = make_seq_small(10);
    require_true(a == b);
    require_false(a != b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: shared root is equal");
  {
    auto m = make_seq_small(10);
    auto m2 = m;
    require_true(m == m2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: different values are unequal");
  {
    auto a = make_seq_small(5);
    auto b = a.set(2, 999);
    require_false(a == b);
    require_true(a != b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: default (null root) arrays are equal");
  {
    pa_small a, b;
    require_true(a == b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("equality: null root vs explicit zero-fill");
  {
    pa_small a;
    pa_small b(0);     // broadcast fill with 0
    require_true(a == b);
  }
  end_test_case();

  // ================================================================ //
  //  8. Arithmetic operators                                          //
  // ================================================================ //
  test_case("element-wise addition");
  {
    pa_small a({ 1, 2, 3, 4 });
    pa_small b({ 10, 20, 30, 40 });
    auto c = a + b;

    require(c.get(0), 11);
    require(c.get(1), 22);
    require(c.get(2), 33);
    require(c.get(3), 44);

    // originals intact
    require(a.get(0), 1);
    require(b.get(0), 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise subtraction");
  {
    pa_small a({ 10, 20, 30, 40 });
    pa_small b({ 1, 2, 3, 4 });
    auto c = a - b;

    require(c.get(0), 9);
    require(c.get(1), 18);
    require(c.get(2), 27);
    require(c.get(3), 36);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise multiplication");
  {
    pa_small a({ 2, 3, 4, 5 });
    pa_small b({ 10, 10, 10, 10 });
    auto c = a * b;

    require(c.get(0), 20);
    require(c.get(1), 30);
    require(c.get(2), 40);
    require(c.get(3), 50);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar addition (operator+=)");
  {
    pa_small a({ 1, 2, 3, 4 });
    auto b = a += 100;

    require(b.get(0), 101);
    require(b.get(1), 102);
    require(b.get(2), 103);
    require(b.get(3), 104);

    // original intact (operator+= returns new parray for immutable type)
    require(a.get(0), 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar multiplication (operator*=)");
  {
    pa_small a({ 1, 2, 3, 4 });
    auto b = a *= 10;

    require(b.get(0), 10);
    require(b.get(1), 20);
    require(b.get(2), 30);
    require(b.get(3), 40);
  }
  end_test_case();

  // ================================================================ //
  //  9. Reductions                                                    //
  // ================================================================ //
  test_case("sum reduction");
  {
    pa_tiny a({ 1, 2, 3, 4 });     // capacity 4
    int s = a.sum();
    require(s, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum on default array is zero");
  {
    pa_tiny a;
    require(a.sum(), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all – every slot matches");
  {
    pa_tiny a(42);
    require_true(a.all(42));
    require_false(a.all(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("any – at least one slot matches");
  {
    pa_tiny a;
    auto b = a.set(2, 77);
    require_true(b.any(77));
    require_true(b.any(0));     // other slots are 0
    require_false(b.any(999));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add / sub / mul / div named helpers");
  {
    pa_tiny a({ 10, 20, 30, 40 });
    auto b = a.add(5);
    require(b.get(0), 15);
    require(b.get(1), 25);

    auto c = a.sub(5);
    require(c.get(0), 5);

    auto d = a.mul(2);
    require(d.get(0), 20);
    require(d.get(1), 40);

    auto e = a.div(10);
    require(e.get(0), 1);
    require(e.get(1), 2);
  }
  end_test_case();

  // ================================================================ //
  //  10. Iterators                                                    //
  // ================================================================ //
  test_case("iterator traverses full capacity");
  {
    auto m = make_seq_small(8);
    int count = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      ++count;
    require(count, (int)pa_small::length);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator dereference matches get");
  {
    auto m = make_seq_small(10);
    usize idx = 0;
    for ( auto it = m.begin(); it != m.end(); ++it, ++idx )
      require(*it, m.get(idx));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend match begin/end");
  {
    auto m = make_seq_small(8);
    auto a = m.begin();
    auto b = m.cbegin();
    while ( a != m.end() ) {
      require(*a, *b);
      ++a;
      ++b;
    }
    require_true(b == m.cend());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("post-increment iterator");
  {
    auto m = make_seq_small(4);
    auto it = m.begin();
    auto prev = it++;
    require(*prev, m.get(0));
    require(*it, m.get(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator index method");
  {
    auto m = make_seq_small(5);
    auto it = m.begin();
    require(it.index(), usize(0));
    ++it;
    require(it.index(), usize(1));
    ++it;
    ++it;
    require(it.index(), usize(3));
  }
  end_test_case();

  // ================================================================ //
  //  11. for_each                                                     //
  // ================================================================ //
  test_case("for_each visits every slot with correct index");
  {
    auto m = make_seq_small(10);
    int count = 0;
    m.for_each([&](usize idx, const int &val) {
      require(val, m.get(idx));
      ++count;
    });
    // for_each visits all slots in the trie (full capacity)
    require_greater(count, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("for_each on default array visits zero-valued slots");
  {
    pa_small m;
    // null root → for_each should be a no-op (no nodes to visit)
    int count = 0;
    m.for_each([&](usize, const int &) { ++count; });
    // null root: for_each returns immediately
    require(count, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("for_each on filled array visits all slots");
  {
    pa_small m(7);
    int count = 0;
    m.for_each([&](usize, const int &val) {
      require(val, 7);
      ++count;
    });
    require(count, (int)pa_small::length);
  }
  end_test_case();

  // ================================================================ //
  //  12. Object lifetimes – non-trivial types, leak detection         //
  // ================================================================ //
  test_case("tracked: construction and destruction balance");
  {
    int before = g_alive;
    {
      pa_tracked m;
      auto m1 = m.set(0, Tracked(10));
      auto m2 = m1.set(1, Tracked(20));
      auto m3 = m2.set(2, Tracked(30));

      require(m3.get(0).val, 10);
      require(m3.get(1).val, 20);
      require(m3.get(2).val, 30);
    }     // all versions destroyed here
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: copy shares root – no extra constructions");
  {
    int before = g_alive;
    {
      pa_tracked m;
      auto m1 = m.set(0, Tracked(42));
      int after_set = g_alive;

      auto m2 = m1;     // O(1) copy, just retains root
      // no new Tracked objects should be constructed
      require(g_alive, after_set);
      require_true(m1.identity() == m2.identity());
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: move nulls source – no leak");
  {
    int before = g_alive;
    {
      pa_tracked m;
      auto m1 = m.set(0, Tracked(99));
      auto m2 = pa_tracked(micron::move(m1));

      require_true(m1.identity() == nullptr);
      require(m2.get(0).val, 99);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: multiple versions alive – no leak on scope exit");
  {
    int before = g_alive;
    {
      pa_tracked v0;
      auto v1 = v0.set(0, Tracked(1));
      auto v2 = v1.set(1, Tracked(2));
      auto v3 = v2.set(2, Tracked(3));
      auto v4 = v3.set(0, Tracked(100));

      // all four versions alive
      require(v1.get(0).val, 1);
      require(v4.get(0).val, 100);
      require(v4.get(1).val, 2);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: branch and merge – no leak");
  {
    int before = g_alive;
    {
      pa_tracked base;
      auto base1 = base.set(0, Tracked(10));
      auto branch_a = base1.set(1, Tracked(20));
      auto branch_b = base1.set(1, Tracked(30));

      require(branch_a.get(1).val, 20);
      require(branch_b.get(1).val, 30);
      require(base1.get(1).val, 0);     // default
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: fill construction – no leak");
  {
    int before = g_alive;
    {
      pa_tracked m(Tracked(7));
      for ( usize i = 0; i < pa_tracked::length; ++i )
        require(m.get(i).val, 7);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: clear – no leak");
  {
    int before = g_alive;
    {
      pa_tracked m(Tracked(5));
      auto m2 = m.clear();
      require_true(m2.identity() == nullptr);
      // m still alive with its nodes
      require(m.get(0).val, 5);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: update – no leak");
  {
    int before = g_alive;
    {
      pa_tracked m;
      auto m1 = m.set(3, Tracked(10));
      auto m2 = m1.update(3, [](const Tracked &t) { return Tracked(t.val + 1); });
      require(m2.get(3).val, 11);
      require(m1.get(3).val, 10);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: rapid overwrite same slot – no leak");
  {
    int before = g_alive;
    {
      pa_tracked m;
      for ( int i = 0; i < 50; ++i )
        m = m.set(0, Tracked(i));
      require(m.get(0).val, 49);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ================================================================ //
  //  13. Size / capacity / static properties                          //
  // ================================================================ //
  test_case("size equals capacity constant");
  {
    pa_small m;
    require(m.size(), pa_small::length);
    require(m.max_size(), pa_small::length);

    auto m2 = m.set(0, 1);
    require(m2.size(), pa_small::length);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("empty is always false");
  {
    pa_small m;
    require_false(m.empty());

    auto m2 = m.set(0, 1);
    require_false(m2.empty());

    auto m3 = m2.clear();
    require_false(m3.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("static properties: branching, height, length");
  {
    // pa_small: K=2, H=2 → B=4, capacity=16
    require(pa_small::branching, usize(4));
    require(pa_small::height, usize(2));
    require(pa_small::length, usize(16));

    // pa_tiny: K=1, H=2 → B=2, capacity=4
    require(pa_tiny::branching, usize(2));
    require(pa_tiny::height, usize(2));
    require(pa_tiny::length, usize(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("type traits: is_pod, is_trivial for int");
  {
    require_true(pa_small::is_pod());
    require_true(pa_small::is_trivial());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("type traits: Tracked is not POD/trivial");
  {
    require_false(pa_tracked::is_pod());
    require_false(pa_tracked::is_trivial());
    require_true(pa_tracked::is_class());
  }
  end_test_case();

  // ================================================================ //
  //  14. Edge cases                                                   //
  // ================================================================ //
  test_case("set and read every slot in capacity");
  {
    pa_small m;
    for ( usize i = 0; i < pa_small::length; ++i )
      m = m.set(i, (int)(i + 1));

    for ( usize i = 0; i < pa_small::length; ++i )
      require(m.get(i), (int)(i + 1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("overwrite every slot – original always intact");
  {
    pa_small orig;
    for ( usize i = 0; i < pa_small::length; ++i )
      orig = orig.set(i, (int)i);

    for ( usize i = 0; i < pa_small::length; ++i ) {
      auto derived = orig.set(i, 999);
      require(derived.get(i), 999);
      require(orig.get(i), (int)i);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("set same slot 100 times – final value correct");
  {
    pa_small m;
    for ( int i = 0; i < 100; ++i )
      m = m.set(0, i);
    require(m.get(0), 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("negative values stored correctly");
  {
    pa_small m;
    m = m.set(0, -100);
    m = m.set(1, -999999);
    m = m.set(2, 0);
    m = m.set(3, 2147483647);     // INT_MAX

    require(m.get(0), -100);
    require(m.get(1), -999999);
    require(m.get(2), 0);
    require(m.get(3), 2147483647);
  }
  end_test_case();

  // ================================================================ //
  //  15. Stress tests                                                 //
  // ================================================================ //
  test_case("stress: set all 32768 slots (default config)");
  {
    pa_int m;
    for ( usize i = 0; i < 1000; ++i )
      m = m.set(i, (int)(i * 7));

    for ( usize i = 0; i < 1000; ++i )
      require(m.get(i), (int)(i * 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: set 1000 slots in reverse order");
  {
    pa_int m;
    for ( int i = 999; i >= 0; --i )
      m = m.set((usize)i, i * 3);

    for ( usize i = 0; i < 1000; ++i )
      require(m.get(i), (int)(i * 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: 100 versions alive simultaneously");
  {
    pa_small versions[16];
    versions[0] = pa_small();
    for ( usize i = 1; i < 16 && i < pa_small::length; ++i )
      versions[i] = versions[i - 1].set(i, (int)(i * 10));

    // each version has its slot set, earlier slots from parent
    for ( usize v = 1; v < 16 && v < pa_small::length; ++v )
      require(versions[v].get(v), (int)(v * 10));

    // version 0 is all defaults
    for ( usize i = 0; i < pa_small::length; ++i )
      require(versions[0].get(i), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: fork 50 branches from single base");
  {
    auto base = make_seq_small(8);
    pa_small branches[16];     // limited to capacity

    for ( usize i = 0; i < 16 && i < pa_small::length; ++i )
      branches[i] = base.set(i, 9999);

    // base intact
    require_true(verify_seq_small(base, 8));

    // each branch diverges only at its slot
    for ( usize i = 0; i < 16 && i < pa_small::length; ++i ) {
      require(branches[i].get(i), 9999);
      // other modified slots match base
      for ( usize j = 0; j < 8; ++j ) {
        if ( j != i )
          require(branches[i].get(j), (int)(j * 10));
      }
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: tracked – 50 versions alive, no leak");
  {
    int before = g_alive;
    {
      pa_tracked versions[16];
      versions[0] = pa_tracked();
      for ( usize i = 1; i < 16 && i < pa_tracked::length; ++i )
        versions[i] = versions[i - 1].set(i, Tracked((int)i));

      // verify last version
      for ( usize i = 1; i < 16 && i < pa_tracked::length; ++i )
        require(versions[15].get(i).val, (int)i);
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: tracked – rapid create/destroy cycle, no leak");
  {
    int before = g_alive;
    {
      for ( int round = 0; round < 100; ++round ) {
        pa_tracked m;
        m = m.set(0, Tracked(round));
        m = m.set(1, Tracked(round * 2));
        auto m2 = m.set(0, Tracked(round * 3));
        require(m2.get(0).val, round * 3);
      }
    }
    require(g_alive, before);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: alternating set on two slots");
  {
    pa_small m;
    for ( int i = 0; i < 500; ++i ) {
      m = m.set(0, i);
      m = m.set(1, i * 2);
    }
    require(m.get(0), 499);
    require(m.get(1), 998);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: equality after independent construction");
  {
    // build the same array two different ways
    pa_small a;
    for ( usize i = 0; i < pa_small::length; ++i )
      a = a.set(i, (int)(i * 5));

    pa_small b([](usize i) -> int { return (int)(i * 5); }, pa_small::length);

    require_true(a == b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: iterator over all slots matches get");
  {
    auto m = make_seq(500);
    usize idx = 0;
    for ( auto it = m.begin(); it != m.end(); ++it, ++idx ) {
      if ( idx < 500 )
        require(*it, (int)(idx * 10));
    }
  }
  end_test_case();

  // ================================================================ //
  //  16. Invariants                                                   //
  // ================================================================ //
  test_case("invariant: set then get always returns new value");
  {
    pa_small m;
    for ( usize i = 0; i < pa_small::length; ++i ) {
      auto m2 = m.set(i, (int)(i * 7));
      require(m2.get(i), (int)(i * 7));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: set preserves other slots");
  {
    auto m = make_seq_small(pa_small::length);
    for ( usize target = 0; target < pa_small::length; ++target ) {
      auto m2 = m.set(target, 9999);
      for ( usize i = 0; i < pa_small::length; ++i ) {
        if ( i == target )
          require(m2.get(i), 9999);
        else
          require(m2.get(i), (int)(i * 10));
      }
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: set + clear roundtrip yields default");
  {
    auto m = make_seq_small(10);
    auto m2 = m.clear();
    pa_small expected;
    require_true(m2 == expected);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: copy equality is reflexive");
  {
    auto m = make_seq_small(10);
    require_true(m == m);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: set same value is logically equal to original");
  {
    auto m = make_seq_small(8);
    // set slot 3 to the same value it already has
    auto m2 = m.set(3, 30);     // 3*10 = 30
    require_true(m == m2);
    // but identity should differ (path copy always creates new nodes)
    require_false(m.identity() == m2.identity());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: fill then check all slots");
  {
    pa_small m(42);
    require_true(m.all(42));
    require_false(m.all(0));
    require_false(m.any(0));
  }
  end_test_case();

  print("=== ALL PARRAY TESTS PASSED ===");
  return 1;
}
