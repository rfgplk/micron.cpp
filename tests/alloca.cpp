//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/std.hpp"

#include "../src/memory/actions.hpp"

int
main()
{
  sb::verify_debug();
  enable_scope()
  {
    sb::test_case("mc::balloca()");
    int *arr = alloca(250, int);
    return 1;
    for ( usize i = 0; i < 250; ++i )
      arr[i] = i;
    mc::console(arr[5]);
    mc::console(arr[50]);
    mc::console(arr[200]);
  };
  return 1;
};
