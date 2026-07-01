//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/vector.hpp"

byte a = 1;

int
main()
{
  byte *b = abc::alloc(1024);
  abc::dealloc(b);
  abc::dealloc(b);
  abc::dealloc(0x0);
  byte *bad_ptr = &a;
  abc::dealloc(bad_ptr);
  return 0;
};
