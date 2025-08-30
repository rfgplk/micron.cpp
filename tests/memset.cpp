//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/print.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"
#include <cstring>

int
main(void)
{
  byte buf[64];
  volatile int x = 0;
  for ( umax_t i = 0; i < 5e9; i++ ) {
    mc::memset_64b(&buf[0], 6);
    x++;
  }
  for ( int i = 0; i < 64; i++ )
    mc::io::print((int)buf[i]);
}
