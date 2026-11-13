//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../../src/std.hpp"
extern int from_a();
extern int from_b();

int
main()
{
  return from_a() == 29 && from_b() == 31 ? 1 : 6;
}
