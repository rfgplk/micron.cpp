//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/sync/invoke.hpp"
#include "../src/std.hpp"

void fn(int x)
{
  mc::console(x);
}

int
main(void)
{
  mc::invoke(fn, 2);
  return 0;
}
