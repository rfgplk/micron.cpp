//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/pointer.hpp"
#include "../../src/vector/vector.hpp"
#include "../../src/std.h"

#include "../../src/string/strings.h"

int
main(void)
{
  {
    mc::cptr<mc::vector<int>> cnst(10, 9);
    mc::console((*cnst)[2]);
  }
  {
    mc::string *test = new mc::string("Reset");
    mc::ptr<mc::string> wptr("Testing weak pointers");
    mc::console(*wptr);
    wptr = test;
    mc::console(*wptr);
  }
  {
    mc::shared<mc::string> sptr("Testing shared pointers");
    mc::shared<mc::string> second(sptr);
    mc::console(*sptr);
    (*sptr)[2] = 'd';
    mc::console(*second);
    mc::const_pointer<mc::wstr> cptr("你好嗎？");
    mc::console(*cptr);
  }
  return 0;
}
