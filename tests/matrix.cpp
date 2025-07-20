//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/matrix/int8x8.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"

int
main(void)
{
  mc::int_matrix_base<int32_t, 8, 8> mat(5);
  mc::console(mat[2, 4]);
  return 0;
}
