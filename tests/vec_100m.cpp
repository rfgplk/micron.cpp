//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/vector/fvector.hpp"

int
main()
{
  constexpr usize N = 100'000'000;
  mc::fvector<usize> vec;
  vec.reserve(N);
  for ( usize i = 0; i < N; i++ )
    vec.emplace_back(i);
  mc::console(vec[100], vec[1000], vec[1000000]);
  return 0;
}
