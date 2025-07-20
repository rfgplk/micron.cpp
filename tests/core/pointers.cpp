//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/pointer.hpp"
#include "../../src/std.h"

#include "../../src/string/strings.h"

int
main(void)
{
  mc::const_pointer<mc::wstr> cptr("你好嗎？");
  mc::console(*cptr);
  return 0;
}
