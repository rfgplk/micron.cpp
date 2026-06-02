//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/list.hpp"
#include "../src/doublelist.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

struct tracked {
  static inline long live = 0;
  static inline long ctor = 0;
  static inline long dtor = 0;

  int *p;

  tracked() : p(new int(0))
  {
    ++live;
    ++ctor;
  }

  tracked(int x) : p(new int(x))
  {
    ++live;
    ++ctor;
  }

  tracked(const tracked &o) : p(new int(*o.p))
  {
    ++live;
    ++ctor;
  }

  tracked(tracked &&o) noexcept : p(o.p)
  {
    o.p = nullptr;
    ++live;
    ++ctor;
  }

  tracked &
  operator=(const tracked &o)
  {
    if ( this != &o ) *p = *o.p;
    return *this;
  }

  ~tracked()
  {
    delete p;
    ++dtor;
    --live;
  }

  bool
  operator==(const tracked &o) const
  {
    return (p && o.p) ? (*p == *o.p) : (p == o.p);
  }

  static void
  reset()
  {
    live = 0;
    ctor = 0;
    dtor = 0;
  }

  static bool
  balanced()
  {
    return live == 0 && ctor == dtor;
  }
};

template<class L>
static bool
matches(const L &l, const int *exp, size_t n)
{
  if ( l.size() != n ) return false;
  auto p = l.ibegin();
  for ( size_t i = 0; i < n; ++i ) {
    if ( p == nullptr || p->data != exp[i] ) return false;
    p = p->next;
  }
  return p == nullptr;
}

template<class L>
static bool
matches_rev(const L &l, const int *exp, size_t n)
{
  if ( l.size() != n ) return false;
  auto p = l.iend();
  for ( size_t i = 0; i < n; ++i ) {
    if ( p == nullptr || p->data != exp[n - 1 - i] ) return false;
    p = p->prev;
  }
  return p == nullptr;
}

template<class L>
static auto
node_at(const L &l, size_t i)
{
  auto p = l.ibegin();
  while ( i-- && p ) p = p->next;
  return p;
}

static void
test_list()
{
  sb::print("=== micron::list ===");

  sb::test_case("push_front / push_back order, size, front, back");
  {
    micron::list<int> l;
    sb::require(l.empty());
    sb::require(l.size() == 0ULL);
    l.push_front(2);
    l.push_front(1);
    l.push_back(3);
    l.push_back(4);
    const int e[] = { 1, 2, 3, 4 };
    sb::require(matches(l, e, 4));
    sb::require(!l.empty());
    sb::require(l.front() == 1);
    sb::require(l.back() == 4);
    sb::require(*l.begin() == 1);
    sb::require(*l.end() == 4);
  }
  sb::end_test_case();

  sb::test_case("count ctors create exactly cnt nodes");
  {
    micron::list<int> a(0);
    sb::require(a.empty() && a.size() == 0ULL);
    micron::list<int> b(3);
    sb::require(b.size() == 3ULL);
    micron::list<int> c(3, 7);
    const int e[] = { 7, 7, 7 };
    sb::require(matches(c, e, 3));
  }
  sb::end_test_case();

  sb::test_case("deep copy ctor is independent");
  {
    micron::list<int> a;
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    micron::list<int> b(a);
    sb::require(a == b);
    a.push_back(99);
    sb::require(a != b);
    const int eb[] = { 1, 2, 3 };
    sb::require(matches(b, eb, 3));
  }
  sb::end_test_case();

  sb::test_case("copy assign frees old + deep copies");
  {
    micron::list<int> a;
    a.push_back(5);
    a.push_back(6);
    micron::list<int> b;
    b.push_back(100);
    b.push_back(101);
    b.push_back(102);
    b = a;
    sb::require(a == b);
    const int e[] = { 5, 6 };
    sb::require(matches(b, e, 2));
    b = b;
    sb::require(matches(b, e, 2));
  }
  sb::end_test_case();

  sb::test_case("move ctor / move assign steal");
  {
    micron::list<int> a;
    a.push_back(1);
    a.push_back(2);
    micron::list<int> b(micron::move(a));
    sb::require(a.empty());
    const int e[] = { 1, 2 };
    sb::require(matches(b, e, 2));
    micron::list<int> c;
    c.push_back(9);
    c = micron::move(b);
    sb::require(b.empty());
    sb::require(matches(c, e, 2));
  }
  sb::end_test_case();

  sb::test_case("find returns element / nullptr on miss");
  {
    micron::list<int> l;
    l.push_back(10);
    l.push_back(20);
    l.push_back(30);
    auto *hit = l.find(20);
    sb::require(hit != nullptr && *hit == 20);
    sb::require(l.find(999) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("insert after node");
  {
    micron::list<int> l;
    l.push_back(1);
    l.push_back(3);
    l.insert(l.ibegin(), 2);
    const int e[] = { 1, 2, 3 };
    sb::require(matches(l, e, 3));
  }
  sb::end_test_case();

  sb::test_case("erase head / middle / tail / single");
  {
    micron::list<int> l;
    for ( int i = 1; i <= 4; ++i ) l.push_back(i);
    l.erase(l.ibegin());
    const int e1[] = { 2, 3, 4 };
    sb::require(matches(l, e1, 3));
    l.erase(node_at(l, 1));
    const int e2[] = { 2, 4 };
    sb::require(matches(l, e2, 2));
    l.erase(node_at(l, 1));
    const int e3[] = { 2 };
    sb::require(matches(l, e3, 1));
    l.erase(l.ibegin());
    sb::require(l.empty());
  }
  sb::end_test_case();

  sb::test_case("merge appends, empties source");
  {
    micron::list<int> a;
    a.push_back(1);
    a.push_back(2);
    micron::list<int> b;
    b.push_back(3);
    b.push_back(4);
    a.merge(b);
    sb::require(b.empty());
    const int e[] = { 1, 2, 3, 4 };
    sb::require(matches(a, e, 4));
    micron::list<int> empty;
    micron::list<int> c;
    c.push_back(7);
    empty.merge(c);
    const int e2[] = { 7 };
    sb::require(matches(empty, e2, 1));
  }
  sb::end_test_case();

  sb::test_case("splice inserts chain after pos");
  {
    micron::list<int> a;
    a.push_back(1);
    a.push_back(4);
    micron::list<int> b;
    b.push_back(2);
    b.push_back(3);
    a.splice(a.ibegin(), b);
    sb::require(b.empty());
    const int e[] = { 1, 2, 3, 4 };
    sb::require(matches(a, e, 4));
  }
  sb::end_test_case();

  sb::test_case("clear empties");
  {
    micron::list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.clear();
    sb::require(l.empty() && l.size() == 0ULL);
    l.push_back(5);
    sb::require(l.front() == 5);
  }
  sb::end_test_case();

  sb::test_case("memory-safety across all ops (no double-free / no leak)");
  {
    tracked::reset();
    {
      micron::list<tracked> l;
      l.push_front(tracked{ 1 });
      l.push_back(tracked{ 2 });
      l.push_back(tracked{ 3 });
      l.insert(l.ibegin(), tracked{ 9 });
      micron::list<tracked> cp(l);
      micron::list<tracked> mv(micron::move(cp));
      l.erase(l.ibegin());
      micron::list<tracked> other;
      other.push_back(tracked{ 7 });
      l.merge(other);
      l.clear();
      l.push_back(tracked{ 42 });
    }
    sb::require(tracked::balanced());
  }
  sb::end_test_case();
}

static void
test_double_list()
{
  sb::print("=== micron::double_list ===");

  sb::test_case("forward + backward links consistent");
  {
    micron::double_list<int> l;
    l.push_front(2);
    l.push_front(1);
    l.push_back(3);
    l.push_back(4);
    const int e[] = { 1, 2, 3, 4 };
    sb::require(matches(l, e, 4));
    sb::require(matches_rev(l, e, 4));
    sb::require(l.front() == 1 && l.back() == 4);
  }
  sb::end_test_case();

  sb::test_case("count ctors create exactly cnt nodes (+ prev links)");
  {
    micron::double_list<int> a(0);
    sb::require(a.empty());
    micron::double_list<int> c(3, 7);
    const int e[] = { 7, 7, 7 };
    sb::require(matches(c, e, 3) && matches_rev(c, e, 3));
  }
  sb::end_test_case();

  sb::test_case("prev() / next() navigation");
  {
    micron::double_list<int> l;
    for ( int i = 0; i < 5; ++i ) l.push_back(i);
    auto first = l.ibegin();
    auto third = l.next(first, 1);
    sb::require(third->data == 2);
    auto back2 = l.prev(third, 0);
    sb::require(back2->data == 1);
    sb::require(l.next(l.iend())->data == 4);
    sb::require(l.prev(l.ibegin())->data == 0);
  }
  sb::end_test_case();

  sb::test_case("deep copy / copy assign / move");
  {
    micron::double_list<int> a;
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    micron::double_list<int> b(a);
    sb::require(a == b);
    const int eb[] = { 1, 2, 3 };
    sb::require(matches(b, eb, 3) && matches_rev(b, eb, 3));
    a.push_back(9);
    sb::require(a != b);
    micron::double_list<int> c;
    c.push_back(100);
    c = a;
    const int ec[] = { 1, 2, 3, 9 };
    sb::require(c == a && matches_rev(c, ec, 4));
    micron::double_list<int> d(micron::move(c));
    sb::require(c.empty() && d == a && matches_rev(d, ec, 4));
  }
  sb::end_test_case();

  sb::test_case("insert after node fixes both links");
  {
    micron::double_list<int> l;
    l.push_back(1);
    l.push_back(3);
    l.insert(l.ibegin(), 2);
    const int e[] = { 1, 2, 3 };
    sb::require(matches(l, e, 3) && matches_rev(l, e, 3));
  }
  sb::end_test_case();

  sb::test_case("erase head / middle / tail repairs prev links");
  {
    micron::double_list<int> l;
    for ( int i = 1; i <= 4; ++i ) l.push_back(i);
    l.erase(node_at(l, 1));
    const int e1[] = { 1, 3, 4 };
    sb::require(matches(l, e1, 3) && matches_rev(l, e1, 3));
    l.erase(l.ibegin());
    const int e2[] = { 3, 4 };
    sb::require(matches(l, e2, 2) && matches_rev(l, e2, 2));
    l.erase(l.iend());
    const int e3[] = { 3 };
    sb::require(matches(l, e3, 1) && matches_rev(l, e3, 1));
  }
  sb::end_test_case();

  sb::test_case("merge / splice keep links consistent");
  {
    micron::double_list<int> a;
    a.push_back(1);
    a.push_back(2);
    micron::double_list<int> b;
    b.push_back(3);
    b.push_back(4);
    a.merge(b);
    sb::require(b.empty());
    const int e[] = { 1, 2, 3, 4 };
    sb::require(matches(a, e, 4) && matches_rev(a, e, 4));

    micron::double_list<int> x;
    x.push_back(10);
    x.push_back(40);
    micron::double_list<int> y;
    y.push_back(20);
    y.push_back(30);
    x.splice(x.ibegin(), y);
    sb::require(y.empty());
    const int e2[] = { 10, 20, 30, 40 };
    sb::require(matches(x, e2, 4) && matches_rev(x, e2, 4));
  }
  sb::end_test_case();

  sb::test_case("memory-safety across all ops (no double-free / no leak)");
  {
    tracked::reset();
    {
      micron::double_list<tracked> l;
      l.push_front(tracked{ 1 });
      l.push_back(tracked{ 2 });
      l.push_back(tracked{ 3 });
      l.insert(l.ibegin(), tracked{ 9 });
      micron::double_list<tracked> cp(l);
      micron::double_list<tracked> mv(micron::move(cp));
      l.erase(node_at(l, 1));
      micron::double_list<tracked> other;
      other.push_back(tracked{ 7 });
      l.merge(other);
      l.clear();
      l.push_back(tracked{ 42 });
    }
    sb::require(tracked::balanced());
  }
  sb::end_test_case();
}

int
main(void)
{
  sb::print("=== LIST / DOUBLE_LIST TESTS ===");
  test_list();
  test_double_list();
  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
