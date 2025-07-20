//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/slice.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"
#include "../src/hash/murmur.hpp"
int
main(void)
{
  mc::slice<float> sl;
  auto b = sl[1000, 2000];
  auto c = sl[];
  mc::console(sl[0]);
  return 1;
}
