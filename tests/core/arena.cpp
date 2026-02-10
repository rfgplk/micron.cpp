//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/mutex/spinlock.hpp"
#include "../../src/std.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/contract.hpp"
#include "../../src/thread/pool.hpp"
#include "../../src/thread/thread.hpp"

#include "../../src/attributes.hpp"
#include "../../src/string/strings.hpp"

#include "../src/range.hpp"

#include "../snowball/snowball.hpp"

int
fn(void)
{
  mc::console("Created");
  return 0;
}

int
main(void)
{
  disable_scope()
  {
    int x = 0;
    mc::console("Maximum threads = ", mc::maximum_threads);
    mc::standard_arena arena;
  redo_fs: {
    for ( size_t i = 0; i < 10; i++ ) {
      arena.create_at(0, fn);
    }
    if ( x++ < 50 )
      goto redo_fs;
    mc::sleep(250);
    arena.force_clean();
  }
  }
  disable_scope()
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
  }

  disable_scope()
  {
    mc::console("Maximum threads = ", mc::maximum_threads);
    mc::standard_arena arena;
    for ( size_t i = 0; i < 4; i++ ) {
      arena.create_at(i, [&]() -> int {
        for ( ;; ) {
        };
        return 0;
      });     // throws because console isn't ts, fix it
    }
    arena.join_all();
  }
  enable_scope()
  {
    mc::console("Maximum threads = ", mc::maximum_threads);
    mc::standard_arena arena;
    for ( size_t i = 0; i < 12; i++ ) {
      arena.create_burden([&]() -> int {
        mc::ssleep(1);
        // mc::cpu_pause<1000000000>();
        return 0;
      });     // throws because console isn't ts, fix it
    }
    arena.join_all();
  }
  return 1;
}
