//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/pointer.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../../src/string/strings.hpp"

int
main(void)
{
  {
    mc::thread_pointer<int> tptr(1);
    mc::console((*tptr));
  }
  {
    mc::cptr<mc::vector<int>> cnst(10, 9);
    mc::console((*cnst)[2]);
  }
  {
    mc::string *test = new mc::string("Reset");
    mc::ptr<mc::string> uptr("Testing unique pointers");
    mc::console(*uptr);
    uptr = test;
    mc::console("Through unique pointer: ", *uptr);
    mc::wptr<mc::string> p(uptr);
    mc::console(p == uptr);
    mc::console("Through weak pointer: ", *p);
  }
  {
    mc::shared<mc::string> shptr("Testing shared pointers");
    mc::shared<mc::string> second(shptr);
    mc::console(*shptr);
    (*shptr)[2] = 'd';
    mc::console(*second);
    mc::const_pointer<mc::wstr> cnptr("你好嗎？");
    mc::console(*cnptr);
  }
  return 0;
}
