//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/math/quants/vecs.hpp"
#include "../src/std.hpp"

#include "../src/io/console.hpp"

int
main()
{
  mc::vector_2<float> vec = { 2.0f, 3.0f };
  mc::vector_16<float> vec16;
  vec += { 2.0f, 2.0f };
  return 0;
}
