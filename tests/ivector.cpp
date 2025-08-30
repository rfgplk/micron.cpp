//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/vector/ivector.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"
int
main(void)
{
  mc::ivector<float> f(500, 1.0f);
  for(auto i = 0; i < 50; i++)
    mc::console(f[i]);
}
