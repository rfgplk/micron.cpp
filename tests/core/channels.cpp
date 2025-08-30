//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"
#include "../../src/sync/channel.hpp"
#include "../../src/thread/thread.hpp"

constexpr int loops = 50000;

void
fn(mc::channel<int> &c, int x)
{
  for ( int i = 0; i < loops; i++ )
    c >> (x * i);
  return;
}

int
main(void)
{
  int a = 10;
  int b = 20;
  int c = 30;
  int d = 40;
  int e = 50;
  mc::channel<int> ch;
  auto t1 = mc::solo::spawn<mc::auto_thread<>>(fn, ch, c);
  auto t2 = mc::solo::spawn<mc::auto_thread<>>(fn, ch, d);
  auto t3 = mc::solo::spawn<mc::auto_thread<>>(fn, ch, e);
  auto t4 = mc::solo::spawn<mc::auto_thread<>>(fn, ch, a);
  auto t5 = mc::solo::spawn<mc::auto_thread<>>(fn, ch, b);
  int result = 0;
  while ( !!ch ) {
    ch << result;
    mc::console("Channel get: ", result);
  }
  return 0;
}
