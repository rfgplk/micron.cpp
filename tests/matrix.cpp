//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt


#include "../src/matrix/matrices.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

int
main(void)
{
  mc::console("Internal size of 16x16: ", mc::int16x16_t::__size);
  mc::console("Internal size of 8x8: ", mc::int8x8_t::__size);
  mc::console("Internal size of 64x1: ", mc::int64x1_t::__size);
  mc::console("Internal size of 4x4: ", mc::int4x4_t::__size);
  mc::console("Internal size of 32x2: ", mc::int32x2_t::__size);
  mc::console("Size of 8x8 is: ", sizeof(mc::int8x8_t));
  mc::int8x8_t mat(5);
  mc::console(mat[2, 4]);
  mat += 2;
  mc::console(mat[2, 4]);
  return 0;
}
