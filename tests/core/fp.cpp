
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/function.hpp"

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

  return 0;
}
