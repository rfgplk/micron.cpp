
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/function.hpp"
#include "../../src/sum.hpp"
#include "../../src/tuple.hpp"
#include "../../src/vector.hpp"

#include "../../src/algorithm/fp.hpp"

#include "../../src/io/console.hpp"

#include "../../src/std.hpp"

void
f(int a, int b)
{
  mc::console("First arg: ", a);
  mc::console("Second arg: ", b);
}

int
g(int a, int b, int c, int d)
{
  mc::console("First arg: ", a);
  mc::console("Second arg: ", b);
  mc::console("Third arg: ", c);
  mc::console("Fourth arg: ", d);
  return a + b + c + d;
}

void
n(int b)
{
  mc::console(b >> 1);     // 50
}


int
main(void)
{
  f(1, 2);
  mc::flip(f)(1, 2);
  mc::partial(g, 1)(2, 3, 4);
  mc::compose_ltr(g, n)(10, 20, 30, 40);

  // mc::vector<mc::tuple<int, float>> vec;
  // mc::vector<mc::option<int, float>> vec_opt;
  // mc::vector<mc::any<int, float, mc::vector<int>>> vec_any;

  int c = 0;
  mc::vector<int> vec(50, [&c](int *x) { return c++; });
  mc::vector<int> vec2(50, [&c](int *x) { return c+=2; });
  mc::vector<int> res = (mc::fp::zip_with(vec, vec2, [](int x, int y)->int{ return x+y; }));
  mc::console(res);
  return 0;
}
