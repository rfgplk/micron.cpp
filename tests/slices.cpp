//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/hash/murmur.hpp"
#include "../src/io/console.hpp"
#include "../src/slice.hpp"
#include "../src/std.hpp"
int
main(void)
{
  mc::slice<int> sl;
  sl.set(10);
  auto b = sl[1000, 2000];
  auto c = sl[];
  mc::console(sl[0]);
  return 1;
}
