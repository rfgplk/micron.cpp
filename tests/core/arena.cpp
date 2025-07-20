//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/thread/arena.hpp"
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/mutex/spinlock.hpp"
#include "../../src/std.h"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/contract.hpp"
#include "../../src/thread/thread.hpp"

#include "../../src/attributes.hpp"
#include "../../src/string/strings.h"

#include "../src/range.hpp"

int
fn(void)
{
  mc::console("Created");
  return 0;
}

int
main(void)
{
  int x = 0;
  mc::console("Maximum threads = ", mc::maximum_threads);
  mc::standard_arena arena;
redo: {
  for ( size_t i = 0; i < 10; i++ ) {
    arena.create_at(0, fn);
  }
  if ( x++ < 50 )
    goto redo;
  arena.clean();
}
  return 1;
}
