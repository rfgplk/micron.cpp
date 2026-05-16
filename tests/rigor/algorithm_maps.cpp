//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/algorithm/accumulate.hpp"
#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/find.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/rb_map.hpp"
#include "../src/maps/robin.hpp"
#include "../src/maps/swiss.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

template<typename M>
static void
seed(M &m, int n)
{
  for ( int i = 0; i < n; ++i ) m.insert(make_key(i), i + 1);
}

int
main(void)
{
  sb::print("=== ALGORITHM x MAPS TESTS ===");

  sb::test_case("robin: for_each visits every entry exactly once");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 10);
    int visits = 0;
    micron::for_each(m, [&](const micron::hstring<char> &, int &) { ++visits; });
    sb::require(visits == 10);
  }
  sb::end_test_case();

  sb::test_case("robin: const for_each works on const map");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 5);
    const auto &cm = m;
    int sum = 0;
    micron::for_each(cm, [&](const micron::hstring<char> &, const int &v) { sum += v; });
    sb::require(sum == (1 + 2 + 3 + 4 + 5));
  }
  sb::end_test_case();

  sb::test_case("robin: all_of returns true only when predicate holds for every entry");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 8);
    sb::require(micron::all_of(m, [](const auto &, const int &v) { return v > 0; }));
    sb::require(!micron::all_of(m, [](const auto &, const int &v) { return v > 4; }));
  }
  sb::end_test_case();

  sb::test_case("robin: any_of / none_of");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 8);
    sb::require(micron::any_of(m, [](const auto &, const int &v) { return v == 5; }));
    sb::require(!micron::any_of(m, [](const auto &, const int &v) { return v == 999; }));
    sb::require(micron::none_of(m, [](const auto &, const int &v) { return v < 0; }));
    sb::require(!micron::none_of(m, [](const auto &, const int &v) { return v == 1; }));
  }
  sb::end_test_case();

  sb::test_case("robin: count_if / count by value");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 10);
    sb::require(micron::count_if(m, [](const auto &, const int &v) { return v % 2 == 0; }) == 5u);
    sb::require(micron::count(m, 7) == 1u);
    sb::require(micron::count(m, 999) == 0u);
  }
  sb::end_test_case();

  sb::test_case("robin: find by value returns pointer or nullptr");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 6);
    const int *p = micron::find(m, 4);
    sb::require(p != nullptr);
    sb::require(*p == 4);
    sb::require(micron::find(m, 999) == nullptr);
    sb::require(micron::contains(m, 3));
    sb::require(!micron::contains(m, -1));
  }
  sb::end_test_case();

  sb::test_case("robin: find_if returns pointer to first matching value");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 10);
    const int *p = micron::find_if(m, [](const auto &, const int &v) { return v > 7; });
    sb::require(p != nullptr);
    sb::require(*p > 7);
    sb::require(micron::find_if(m, [](const auto &, const int &v) { return v > 1000; }) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("robin: transform mutates values in place");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 5);
    micron::transform(m, [](const auto &, const int &v) { return v * 10; });
    sb::require(*m.find(make_key(0)) == 10);
    sb::require(*m.find(make_key(4)) == 50);
  }
  sb::end_test_case();

  sb::test_case("robin: fold_left");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 5);
    int total = micron::fold_left(m, 0, [](int acc, const auto &, const int &v) { return acc + v; });
    sb::require(total == (1 + 2 + 3 + 4 + 5));
  }
  sb::end_test_case();

  sb::test_case("robin: accumulate sums values");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    seed(m, 5);
    int total = micron::accumulate(m, 0);
    sb::require(total == (1 + 2 + 3 + 4 + 5));
  }
  sb::end_test_case();

  sb::test_case("rb_map: for_each visits every entry exactly once");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 12);
    int visits = 0;
    micron::for_each(m, [&](const micron::hstring<char> &, int &) { ++visits; });
    sb::require(visits == 12);
  }
  sb::end_test_case();

  sb::test_case("rb_map: count_if and count by value");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 10);
    sb::require(micron::count_if(m, [](const auto &, const int &v) { return v <= 5; }) == 5u);
    sb::require(micron::count(m, 9) == 1u);
  }
  sb::end_test_case();

  sb::test_case("rb_map: find / contains scan values");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 6);
    const int *p = micron::find(m, 5);
    sb::require(p != nullptr);
    sb::require(*p == 5);
    sb::require(micron::contains(m, 1));
    sb::require(!micron::contains(m, 99));
  }
  sb::end_test_case();

  sb::test_case("rb_map: transform mutates values in place");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 5);
    micron::transform(m, [](const auto &, const int &v) { return v + 100; });
    int total = micron::accumulate(m, 0);
    sb::require(total == (101 + 102 + 103 + 104 + 105));
  }
  sb::end_test_case();

  sb::test_case("rb_map: fold_left and accumulate sum match");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 7);
    int via_fold = micron::fold_left(m, 0, [](int a, const auto &, const int &v) { return a + v; });
    int via_acc = micron::accumulate(m, 0);
    sb::require(via_fold == via_acc);
    sb::require(via_fold == (1 + 2 + 3 + 4 + 5 + 6 + 7));
  }
  sb::end_test_case();

  sb::test_case("rb_map: all_of / any_of / none_of");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    seed(m, 4);
    sb::require(micron::all_of(m, [](const auto &, const int &v) { return v >= 1; }));
    sb::require(micron::any_of(m, [](const auto &, const int &v) { return v == 3; }));
    sb::require(micron::none_of(m, [](const auto &, const int &v) { return v == 1000; }));
  }
  sb::end_test_case();

  sb::test_case("swiss: for_each visits every entry exactly once");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 8);
    int visits = 0;
    micron::for_each(m, [&](const micron::hstring<char> &, int &) { ++visits; });
    sb::require(visits == 8);
  }
  sb::end_test_case();

  sb::test_case("swiss: all_of / any_of / none_of");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 5);
    sb::require(micron::all_of(m, [](const auto &, const int &v) { return v > 0; }));
    sb::require(micron::any_of(m, [](const auto &, const int &v) { return v == 3; }));
    sb::require(micron::none_of(m, [](const auto &, const int &v) { return v > 9999; }));
  }
  sb::end_test_case();

  sb::test_case("swiss: count_if and count by value");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 10);
    sb::require(micron::count_if(m, [](const auto &, const int &v) { return v > 5; }) == 5u);
    sb::require(micron::count(m, 4) == 1u);
  }
  sb::end_test_case();

  sb::test_case("swiss: find / contains / find_if scan values");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 6);
    const int *p = micron::find(m, 2);
    sb::require(p != nullptr);
    sb::require(*p == 2);
    sb::require(micron::contains(m, 6));
    sb::require(!micron::contains(m, 0));
    const int *q = micron::find_if(m, [](const auto &, const int &v) { return v == 5; });
    sb::require(q != nullptr);
    sb::require(*q == 5);
  }
  sb::end_test_case();

  sb::test_case("swiss: transform mutates values in place");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 5);
    micron::transform(m, [](const auto &, const int &v) { return -v; });
    int total = micron::accumulate(m, 0);
    sb::require(total == -(1 + 2 + 3 + 4 + 5));
  }
  sb::end_test_case();

  sb::test_case("swiss: fold_left accumulates with custom op");
  {
    micron::stack_swiss_map<micron::hstring<char>, int, 64> m;
    seed(m, 4);
    int product = micron::fold_left(m, 1, [](int a, const auto &, const int &v) { return a * v; });
    sb::require(product == (1 * 2 * 3 * 4));
  }
  sb::end_test_case();

  sb::test_case("empty maps: all_of true / any_of false / accumulate identity");
  {
    micron::robin_map<micron::hstring<char>, int> m(16);
    sb::require(micron::all_of(m, [](const auto &, const int &) { return false; }));
    sb::require(!micron::any_of(m, [](const auto &, const int &) { return true; }));
    sb::require(micron::accumulate(m, 42) == 42);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
