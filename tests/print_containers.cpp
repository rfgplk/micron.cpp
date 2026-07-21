//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/echo.hpp"

#include "../src/array/array.hpp"
#include "../src/array/mdarray.hpp"
#include "../src/bitfield.hpp"
#include "../src/doublelist.hpp"
#include "../src/heap/binary_heap.hpp"
#include "../src/heap/heapq.hpp"
#include "../src/list.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/maps/hopscotch.hpp"
#include "../src/maps/robin.hpp"
#include "../src/math/quants/quat.hpp"
#include "../src/math/quants/vec.hpp"
#include "../src/math/sparse/csc.hpp"
#include "../src/math/sparse/csr.hpp"
#include "../src/math/sparse/sparse_vec.hpp"
#include "../src/memory/pointers/unique.hpp"
#include "../src/queue/spsc_queue.hpp"
#include "../src/sets/hopscotch_set.hpp"
#include "../src/sets/robin_set.hpp"
#include "../src/sort/sorts.hpp"
#include "../src/span.hpp"
#include "../src/stacks/stack.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/sum.hpp"
#include "../src/trees/b.hpp"
#include "../src/tuple.hpp"
#include "../src/vector/svector.hpp"
#include "../src/vector/vector.hpp"

namespace mc = micron;
namespace mm = micron::math;
namespace ms = micron::math::sparse;
using namespace micron;

static int fails = 0;

static void
report(const char *what, const mc::string &got, const char *want)
{
  ++fails;
  io::error("FAIL ", what, ": got \"", got, "\" want \"", want, "\"\n");
}

template<typename Fn>
static mc::string
capture(Fn &&fn)
{
  i32 fds[2]{};
  posix::pipe2(fds, 0);
  fd_t w{ fds[1] };
  fn(w);
  posix::close(fds[1]);
  mc::string out;
  char buf[8192];
  for ( ;; ) {
    max_t r = posix::read(fds[0], buf, sizeof(buf));
    if ( r <= 0 ) break;
    for ( max_t i = 0; i < r; ++i ) out.push_back(buf[i]);
  }
  posix::close(fds[0]);
  return out;
}

template<typename T>
static void
shows(const char *what, const T &v, const char *want)
{
  auto got = capture([&](fd_t w) { io::echo(w, v); });
  mc::string expect(want);
  expect.push_back('\n');
  if ( !(got == expect) ) report(what, got, want);
}

static_assert(!micron::sort::__for_each_harvestable<mc::hopscotch_set<i32>>,
              "hopscotch_set must not be sort-harvestable: it can only ever yield hashes");

static_assert(!micron::is_extractable_heap<mc::heapq<i32>>, "heapq must not be drain_sorted-able: its size()/pop() loop is not atomic");

static_assert(micron::__print::printable<mc::heapq<i32>>, "heapq still prints");
static_assert(micron::__print::printable<mc::hopscotch_set<i32>>, "hopscotch_set still prints");

int
main(void)
{

  {
    mc::vector<i32> v;
    v.push_back(1);
    v.push_back(2);
    shows("sequence", v, "{ 1, 2 }");
  }
  {
    mc::list<i32> l;
    l.push_back(7);
    l.push_back(8);
    shows("node_chain(list)", l, "{ 7, 8 }");
  }
  {
    mc::double_list<i32> l;
    l.push_back(3);
    l.push_back(4);
    shows("node_chain(double_list)", l, "{ 3, 4 }");
  }
  {
    mc::hswiss<u64, u64> m;
    m.insert(1ul, 10ul);
    shows("kv .a/.b map", m, "{ 1: 10 }");
  }
  {
    mc::pair<i32, const char *> p{ 5, "five" };
    shows("pair", p, "[5, five]");
  }
  {
    mc::stack<i32> s;
    s.push(9);
    shows("stack", s, "{ 9 }");
  }
  {

    mc::array<i32, 3> a;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    shows("array is a sequence, not a vec", a, "{ 1, 2, 3 }");
  }
  {
    mc::svector<i32, 8> v;
    v.push_back(1);
    v.push_back(2);
    shows("svector", v, "{ 1, 2 }");
  }
  {
    mc::span<i32, 4> sp;
    shows("span", sp, "{  }");
  }

  {

    mc::robin_map<mc::string, i32> m;
    m.insert(mc::string("a"), 1);
    shows("robin_map<string,i32>", m, "{ a: 1 }");
  }
  {
    mc::robin_map<i32, i32> m;
    m.insert(2, 20);
    shows("robin_map<i32,i32>", m, "{ 2: 20 }");
  }
  {

    mc::b_tree<i32, f32> t;
    t.insert(1, 1.5f);
    auto got = capture([&](fd_t w) { io::echo(w, t); });
    if ( got.size() < 3 || got[0] != '{' ) report("b_tree", got, "{ 1: ... }");
  }
  {
    mc::robin_set<i32> s;
    s.insert(4);
    shows("robin_set", s, "{ 4 }");
  }
  {
    mc::tuple<i32, const char *> t{ 1, "hi" };
    shows("tuple", t, "[1, hi]");
  }
  {
    mc::tuple<> t{};
    shows("empty tuple", t, "[]");
  }

  {

    mc::vector<mc::pair<i32, i32>> v;
    v.push_back(mc::pair<i32, i32>{ 1, 2 });
    shows("vector<pair> is NOT a map", v, "{ [1, 2] }");
  }
  {

    byte b = 65;
    shows("byte renders numerically", b, "65");
    const byte cb = 65;
    shows("const byte renders numerically", cb, "65");
  }
  {

    mc::mdarray<i32, 2> a(2, 3);
    for ( usize i = 0; i < 6; ++i ) a.begin()[i] = static_cast<i32>(i + 1);
    shows("mdarray nests by shape", a, "{ { 1, 2, 3 }, { 4, 5, 6 } }");
  }

  {
    mc::option<i32, u64> o(mc::tag<i32>{}, 42);
    shows("option first", o, "first(42)");
  }
  {
    mc::any<i32, f32> a;
    a.template emplace<f32>(2.5f);
    shows("any", a, "any#1(2.5E0)");
  }
  {
    mm::vec<f32, 4> v{ { 1.0f, 2.0f, 3.0f, 4.0f } };
    shows("vec", v, "(1E0, 2E0, 3E0, 4E0)");
  }
  {
    mm::quat<f32> q{ { 1.0f, 0.0f, 0.0f, 0.0f } };
    shows("quat", q, "(1E0, 0E0, 0E0, 0E0)");
  }
  {
    mc::bitfield<8> b;
    b.set(0);
    b.set(3);
    shows("bitfield", b, "0b10010000");
  }
  {
    mc::binary_heap<i32> h;
    h.insert(5);
    shows("heap", h, "{ 5 }");
  }
  {
    mc::unique_pointer<i32> p(7);
    shows("smart pointer prints the pointee", p, "7");
  }
  {
    mc::spsc_queue<i32, 64> q;
    auto got = capture([&](fd_t w) { io::echo(w, q); });
    if ( !(got == mc::string("{ ~0 }\n")) ) report("lock-free queue summarises", got, "{ ~0 }");
  }
  {

    mc::hopscotch_set<i32> s;
    s.insert(7);
    s.insert(9);
    shows("hopscotch_set summarises", s, "{ ~2 }");
  }
  {
    mc::heapq<i32> h;
    h.push(3);
    h.push(1);
    shows("heapq", h, "{ 1, 3 }");
  }
  {

    ms::csc<f32> c(3, 3);
    c.outer[0] = 0;
    c.outer[1] = 0;
    c.outer[2] = 1;
    c.outer[3] = 1;
    c.inner.push_back(2);
    c.values.push_back(7.0f);
    shows("square csc is not transposed", c, "{ (2, 1): 7E0 }");

    ms::csr<f32> r(3, 3);
    r.outer[0] = 0;
    r.outer[1] = 0;
    r.outer[2] = 1;
    r.outer[3] = 1;
    r.inner.push_back(2);
    r.values.push_back(7.0f);
    shows("square csr", r, "{ (1, 2): 7E0 }");
  }

  {
    mc::vector<mc::vector<i32>> vv;
    mc::vector<i32> a;
    a.push_back(1);
    vv.push_back(mc::move(a));
    shows("nested containers", vv, "{ { 1 } }");
  }
  {
    mc::vector<u32> v;
    v.push_back(10);
    v.push_back(255);
    auto got = capture([&](fd_t w) { io::echof(w, "{:x}", v); });
    if ( !(got == mc::string("{ a, ff }\n")) ) report("echof spec applies per element", got, "{ a, ff }");

    auto s = mc::format::format("v = {}", v);
    if ( !(s == mc::hstring<schar>("v = { 10, 255 }")) ) report("format() container", mc::string(s.c_str()), "v = { 10, 255 }");
  }
  {

    mc::tuple<i32, i32> t{ 1, 2 };
    auto s = mc::format::format("{}", t);
    if ( !(s == mc::hstring<schar>("[1, 2]")) ) report("format() tuple", mc::string(s.c_str()), "[1, 2]");
  }

  if ( fails == 0 ) {
    posix::write(1, "ALL PASS\n", 9);
    return 1;
  }
  return 6;
}
