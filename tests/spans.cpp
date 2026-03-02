//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/hash/murmur.hpp"
#include "../src/io/console.hpp"
#include "../src/span.hpp"
#include "../src/std.hpp"
int
main(void)
{
  mc::span<int, 16> sp(16);
  sp.set(10);
  mc::console(sp);
  return 1;
}
