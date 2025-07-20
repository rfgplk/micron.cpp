//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/ftw.hpp"
#include "../src/std.h"

int
main(void)
{
  auto r = mc::io::ftw_all(".");
  for(auto& n : r)
    mc::console(n);
  return 0;
}
