//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/sync/channel.hpp"
#include "../../src/thread/thread.hpp"

int
main(void)
{
  mc::atomic<char> a;
  a = 10;
  mc::console(*a.get());
  mc::atomic_ptr<int *> ptr;
  ptr = 0x50;
  mc::console(ptr.get());
  return 0;
}
