//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/string/strings.h"
#include "../src/io/console.hpp"
#include "../src/std.h"

#include <iostream>

int
main(void)
{
  mc::ustr8 str = "Another message";
  for(size_t i = 0; i < (size_t)1e7; i++)
    mc::console(str);
}
