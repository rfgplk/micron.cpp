//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <iostream>

#include "../src/io/console.hpp"
#include "../src/range.hpp"
#include "../src/stack.hpp"
#include "../src/std.hpp"

#include <stack>

int
main()
{
  if constexpr ( false ) {
    mc::istack<int> stck{ 2, 6, 7, 22, 55 };
    mc::console(stck.max_size());
    for ( int i = 0; i < 100; i++ )
      stck.push(i);
    mc::console(stck.size());
    mc::console(stck.top());
  }
  if constexpr ( false ) {
    mc::stack<int> stck{ 2, 6, 7, 22, 55 };
    using rng = mc::int_range<0, 1e7>;
    rng::perform(stck, &mc::stack<int>::push);
    mc::console(stck.size());
    mc::console(stck.top());
  }
  if constexpr ( false ) {
    mc::stack<int> stck{ 2, 6, 7, 22, 55 };
    mc::istack<int> istck(stck);
    mc::console(istck.size());
    mc::console(istck.top());
  }
  if constexpr ( false ) {
    mc::sstack<int> stck{ 2, 6, 7, 22, 55 };
    mc::console(stck.size());
    mc::console(stck.top());
  }
  return 1;
}
