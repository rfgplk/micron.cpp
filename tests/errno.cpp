//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/errno.hpp"
#include "../src/io/console.hpp"
#include "../src/std.h"

int
main(void)
{
  mc::console(mc::error::const_get_errno<EPERM>());
  for ( int i = 0; i < 39; i++ )
    mc::console(mc::error::get_errno(i));
  return 0;
}
