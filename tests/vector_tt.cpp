#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "snowball/snowball.hpp"

#include <cassert>

#include "../src/vector/vector.hpp"

struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  static inline size_t copy = 0;
  static inline size_t move = 0;

  int v;

  Tracked(int x = 0) : v(x) { ++ctor; }
  Tracked(const Tracked &o) : v(o.v) { ++copy; }
  Tracked(Tracked &&o) noexcept : v(o.v) { ++move; }
  ~Tracked() { ++dtor; }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    ++copy;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    ++move;
    return *this;
  }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
  bool
  operator<(const Tracked &o) const
  {
    return v < o.v;
  }

  static void
  reset()
  {
    ctor = dtor = copy = move = 0;
  }
};

void
test_construction()
{
  {
    micron::vector<int> v;
    assert(v.size() == 0);
    assert(v.empty());
  }

  {
    micron::vector<int> v(10);
    assert(v.size() == 10);
    for ( size_t i = 0; i < v.size(); ++i )
      assert(v[i] == 0);
  }

  {
    micron::vector<int> v(8, 42);
    assert(v.size() == 8);
    for ( auto &x : v )
      assert(x == 42);
  }

  {
    micron::vector<int> v{ 1, 2, 3, 4, 5 };
    assert(v.size() == 5);
    for ( size_t i = 0; i < 5; ++i )
      assert(v[i] == int(i + 1));
  }
}

void
test_copy_move()
{
  Tracked::reset();
  {
    micron::vector<Tracked> a;
    for ( int i = 0; i < 10; ++i )
      a.emplace_back(i);

    micron::vector<Tracked> b(a);
    assert(b.size() == a.size());

    for ( size_t i = 0; i < a.size(); ++i )
      assert(a[i] == b[i]);

    micron::vector<Tracked> c;
    c = a;
    assert(c.size() == a.size());
  }
  // assert(Tracked::ctor >= Tracked::dtor); ?
}

void
test_push_pop()
{
  micron::vector<int> v;

  for ( int i = 0; i < 10000; ++i )
    v.push_back(i);

  assert(v.size() == 10000);

  for ( int i = 9999; i >= 0; --i ) {
    assert(v.back() == i);
    v.pop_back();
  }

  assert(v.empty());
}

void
test_insert_index()
{
  micron::vector<int> v;

  for ( int i = 0; i < 5; ++i )
    v.push_back(i);     // [0 1 2 3 4]

  v.insert(v.begin() + 0, 99);     // [99 0 1 2 3 4]
  v.insert(v.begin() + 3, 77);     // [99 0 1 77 2 3 4]
  v.insert(v.end() - 1, 55);

  assert(v[0] == 99);
  assert(v[3] == 77);
  assert(v[v.size() - 2] == 55);
}

void
test_insert_iterator()
{
  micron::vector<int> v;
  for ( int i = 0; i < 5; ++i )
    v.push_back(i);

  auto it = v.begin() + 2;
  v.insert(it, 42);

  assert(v.size() == 6);
  assert(v[2] == 42);
}

void
test_erase()
{
  micron::vector<int> v;
  for ( int i = 0; i < 10; ++i )
    v.push_back(i);

  v.erase(size_t(0));
  assert(v.front() == 1);

  v.erase(v.begin() + 4);
  assert(v[4] == 6);

  while ( !v.empty() )
    v.erase(v.begin());

  assert(v.size() == 0);
}

void
test_resize_reserve()
{
  micron::vector<int> v;

  v.reserve(100);
  assert(v.size() == 0);

  v.resize(50);
  assert(v.size() == 50);

  v.resize(100, 7);
  assert(v.size() == 100);

  for ( size_t i = 50; i < 100; ++i )
    assert(v[i] == 7);
}

void
test_sort()
{
  micron::vector<int> v{ 5, 4, 3, 2, 1 };
  v.sort();

  for ( size_t i = 1; i < v.size(); ++i )
    assert(v[i - 1] <= v[i]);
}

void
test_insert_sort()
{
  micron::vector<int> v;
  v.push_back(1);
  v.push_back(3);
  v.push_back(5);

  v.insert_sort(4);
  v.insert_sort(10);
  v.insert_sort(5);
  v.insert_sort(8);
  v.insert_sort(0);
  for ( size_t i = 1; i < v.size(); ++i )
    assert(v[i - 1] <= v[i]);
}
void
test_into_bytes()
{
  micron::vector<uint32_t> v;
  for ( uint32_t i = 0; i < 16; ++i )
    v.push_back(i);

  auto b = v.into_bytes();
  assert(b.size() == sizeof(uint32_t) * v.size());
}
void
test_clear_reuse()
{
  micron::vector<int> v;
  for ( int i = 0; i < 100; ++i )
    v.push_back(i);

  v.clear();
  assert(v.size() == 0);

  for ( int i = 0; i < 50; ++i )
    v.push_back(i * 2);

  assert(v.size() == 50);
}

struct Tracked2 {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  static inline size_t copy = 0;
  static inline size_t move = 0;

  int v;

  Tracked2(int x = 0) : v(x) { ++ctor; }
  Tracked2(const Tracked2 &o) : v(o.v) { ++copy; }
  Tracked2(Tracked2 &&o) noexcept : v(o.v) { ++move; }
  ~Tracked2() { ++dtor; }

  Tracked2 &
  operator=(const Tracked2 &o)
  {
    v = o.v;
    ++copy;
    return *this;
  }

  Tracked2 &
  operator=(Tracked2 &&o) noexcept
  {
    v = o.v;
    ++move;
    return *this;
  }

  bool
  operator==(const Tracked2 &o) const
  {
    return v == o.v;
  }
  bool
  operator<(const Tracked2 &o) const
  {
    return v < o.v;
  }
  bool
  operator>(const Tracked2 &o) const
  {
    return v > o.v;
  }

  static void
  reset()
  {
    ctor = dtor = copy = move = 0;
  }
};

/* ---------------------------------------------------------
 * Capacity / growth torture
 * --------------------------------------------------------- */

void
test_capacity_growth()
{
  micron::vector<int> v;
  size_t last_cap = v.max_size();

  for ( int i = 0; i < 10000; ++i ) {
    v.push_back(i);
    assert(v.size() == size_t(i + 1));
    assert(v.max_size() >= v.size());
    if ( v.max_size() != last_cap ) {
      assert(v.max_size() > last_cap);
      last_cap = v.max_size();
    }
  }
}

/* ---------------------------------------------------------
 * Iterator stability under reserve
 * --------------------------------------------------------- */

void
test_iterator_invalidation()
{
  micron::vector<int> v;
  for ( int i = 0; i < 16; ++i )
    v.push_back(i);

  int *p = &v[5];
  int val = *p;

  v.reserve(1024);

  assert(v[5] == val);
  assert(*(&v[5]) == val);
}

/* ---------------------------------------------------------
 * Self-aliasing insert_sort
 * --------------------------------------------------------- */

void
test_insert_sort_self_alias()
{
  micron::vector<int> v;
  for ( int i = 0; i < 10; ++i )
    v.push_back(i * 2);

  int x = v[4];
  v.insert_sort(x);

  for ( size_t i = 1; i < v.size(); ++i )
    assert(v[i - 1] <= v[i]);
}

/* ---------------------------------------------------------
 * Repeated erase / insert churn
 * --------------------------------------------------------- */

void
test_churn()
{
  micron::vector<int> v;

  for ( int i = 0; i < 1000; ++i )
    v.push_back(i);

  for ( int r = 0; r < 100; ++r ) {
    for ( int i = 0; i < 100; ++i )
      v.erase(v.begin());

    for ( int i = 0; i < 100; ++i )
      v.insert(v.begin(), i);
  }

  assert(v.size() == 1000);
}

/* ---------------------------------------------------------
 * Front / back correctness
 * --------------------------------------------------------- */

void
test_front_back()
{
  micron::vector<int> v;
  v.push_back(1);
  v.push_back(2);
  v.push_back(3);

  assert(v.front() == 1);
  assert(v.back() == 3);

  v.erase(size_t(0));
  assert(v.front() == 2);

  v.pop_back();
  assert(v.back() == 2);
}

/* ---------------------------------------------------------
 * Assign + clear + reuse
 * --------------------------------------------------------- */

void
test_assign_clear_reuse()
{
  micron::vector<int> v;
  v.assign(100, 7);

  assert(v.size() == 100);
  for ( auto &x : v )
    assert(x == 7);

  v.clear();
  assert(v.size() == 0);

  for ( int i = 0; i < 50; ++i )
    v.push_back(i);

  for ( int i = 0; i < 50; ++i )
    assert(v[i] == i);
}

/* ---------------------------------------------------------
 * Non-trivial type lifetime accounting
 * --------------------------------------------------------- */

void
test_tracked_lifetimes()
{
  Tracked2::reset();

  {
    micron::vector<Tracked2> v;
    for ( int i = 0; i < 100; ++i )
      v.emplace_back(i);

    v.insert(v.begin()+10, Tracked2(42));
    v.erase(size_t(20));
    v.resize(150);
    v.resize(50);
    v.clear();
  }

  assert(Tracked2::ctor + Tracked2::copy + Tracked2::move >= Tracked2::dtor);
}

/* ---------------------------------------------------------
 * Insert ranges
 * --------------------------------------------------------- */

void
test_insert_ranges()
{
  micron::vector<int> v;
  for ( int i = 0; i < 10; ++i )
    v.push_back(i);

  v.insert(5, 99, 10);
  assert(v.size() == 20);

  for ( int i = 5; i < 15; ++i )
    assert(v[i] == 99);
}

/* ---------------------------------------------------------
 * Slice consistency
 * --------------------------------------------------------- */

void
test_slice_consistency()
{
  micron::vector<int> v;
  for ( int i = 0; i < 100; ++i )
    v.push_back(i);

  auto s = v[10, 20];
  assert(s.size() == 10);

  for ( size_t i = 0; i < s.size(); ++i )
    assert(s[i] == int(i + 10));
}

/* ---------------------------------------------------------
 * Massive insert_sort stress
 * --------------------------------------------------------- */

void
test_insert_sort_stress()
{
  micron::vector<int> v;

  for ( int i = 1000; i >= 0; --i )
    v.insert_sort(i);

  for ( size_t i = 1; i < v.size(); ++i )
    assert(v[i - 1] <= v[i]);
}

int
main()
{
  test_construction();
  test_copy_move();
  test_push_pop();
  test_insert_index();
  test_insert_iterator();
  test_erase();
  test_resize_reserve();
  test_sort();
  test_insert_sort();
  test_into_bytes();
  test_clear_reuse();

  test_capacity_growth();
  test_iterator_invalidation();
  test_insert_sort_self_alias();
  test_churn();
  test_front_back();
  test_assign_clear_reuse();
  test_tracked_lifetimes();
  test_insert_ranges();
  test_slice_consistency();
  test_insert_sort_stress();
  return 0;
}
