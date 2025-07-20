//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <iostream>

#include "../src/vector/vector.hpp"

int
main()
{
  for ( size_t j = 0; j < 5e8; j++ ) {
    micron::vector<int> buf = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  }
  return 0;
}
