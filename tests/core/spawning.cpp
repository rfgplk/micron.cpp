//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"
#include "../../src/string/strings.hpp"

#include "../../src/thread/spawn.hpp"

// #include "../snowball/snowball.hpp"
int
fn(const mc::string &str)
{
  mc::console(str);
  return 0;
}

int
fn_void(void)
{
  mc::console("Void!");
  return 5;
}

int
fn_loop(void)
{
  int i = 0;
  for ( ;i<10000; ) {
   i++;// mc::console(i++);
  }
  return 5;
}
int
main(void)
{
  mc::go(fn_void);
  mc::go(fn, mc::string("Hello World"));
  auto& thread = mc::go(fn_loop);
  mc::sleep(500);
  //thread().cancel();
  return 1;
}
