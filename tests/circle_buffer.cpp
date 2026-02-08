//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/circle_buffer.hpp"

int
main()
{
  sb::verify_debug();
  enable_scope()
  {
    sb::test_case("mc::circle_buffer(), testing insertions and erasures");
    mc::circular<int, 512> buf;
    for ( int i = 0; i < 10; i++ )
      buf.emplace_back(i);
  };
}
