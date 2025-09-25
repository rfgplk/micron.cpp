//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/queue.hpp"
#include "../src/std.hpp"

int
main()
{
  sb::verify_debug();
  disable_scope()
  {
    micron::queue<int> qu;
    for ( int i = 0; i < 10; i++ )
      qu.push(i);
    while ( !qu.empty() ) {
      mc::console("Front: ", qu.front());
      mc::console("Back: ", qu.last());
      qu.pop();
    }
  };
  enable_scope()
  {
    micron::queue<int> qu;
    for ( int i = 0; i < 50; i++ )
      qu.push(i);
    qu.reserve(65536);
    for ( int i = 50; i < 60; i++ )
      qu.push(i);
    while ( !qu.empty() ) {
      mc::console("Front: ", qu.front());
      mc::console("Back: ", qu.last());
      qu.pop();
    }
  };
}
