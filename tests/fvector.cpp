//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/vector/fvector.hpp"

int
main()
{
  sb::verify_debug();
  enable_scope()
  {
    sb::test_case("mc::fvector(), testing insertions and erasures");
    mc::fvector<int> vec;
    for ( int i = 0; i < 10; i++ )
      vec.push_back(i);
    vec.erase(vec.begin());
    vec.erase(vec.begin() + 3);
    vec.erase(vec.begin() + 7);
    vec.erase(vec.begin() + 9);
    mc::console(vec);
  };
  enable_scope()
  {
    sb::test_case("mc::fvector(), testing move const");
    mc::fvector<int> vec(10, 5);
    mc::console(vec);
    mc::fvector<int> vec_2(mc::move(vec));
    mc::console(vec);
    mc::console(vec_2);
  };
}
