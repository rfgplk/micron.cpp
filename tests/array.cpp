//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/std.hpp"
#include "../src/io/console.hpp"
#include "../src/array.hpp"


int
main()
{
  mc::array<int, 20> arr(5);
  for(auto& n : arr)
    mc::console(n);
  return 0;
};
