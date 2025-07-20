//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/mutex/spinlock.hpp"
#include "../../src/std.h"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/contract.hpp"
#include "../../src/thread/thread.hpp"

#include "../../src/attributes.hpp"
#include "../../src/string/strings.h"

int global = 4;

mc::spin_lock sl;
int
fn(int a, int b, int c, int d, int e)
{
  global = 100;
  mc::console("A should be: ", a, b, c, d, e);
  mc::console("I am: ", ::gettid());
  mc::ssleep(2);
  mc::console("Global is: ", global);
  size_t x = 0;
  for ( size_t i = 0; i < 1e10; i++ )
    x += i;
  mc::console("Done", x);
  // sl();
  return 0;
}

int
fn_void(void)
{
  mc::console("Done");
  return 0;
}
#include "../../src/sync/when.hpp"

int
main(void)
{
  if constexpr ( false ) {
    int t = 6;
    // mc::when(t, 6);
    // mc::as_thread(fn, t);
    mc::thread th(fn, 5, 6, 7, 8, 9);
    mc::ssleep(1);
    mc::console("Parent is: ", ::gettid());
    mc::ssleep(5);
    mc::console("Will unlock now!");
    sl.unlock();
    mc::sleep(1000);
    mc::console("Starting new thread");
    th[fn, 1, 2, 3, 4, 5];
    sl.unlock();
    th.try_join();
    return 0;
  }
  if constexpr ( false ) {
    mc::yield();
    auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5, 6, 7, 8, 9);
    mc::ssleep(1);
    // mc::console("Global is ", global);
    mc::solo::yield(t);
    mc::ssleep(2);
    // t->sleep();
    // mc::console("Thread Asleep!");
    // mc::ssleep(2);
    // t->awaken();
    mc::solo::join(t);
  }
  if constexpr ( false ) {
    mc::auto_thread th(fn, 5, 6, 7, 8, 9);
    auto f = th.try_join();
    mc::console("trying to join, should be -1: ", f, ".");
  }

  if constexpr ( true ) {
    mc::yield();
    auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5, 6, 7, 8, 9);
    mc::ssleep(1);
    // mc::console("Global is ", global);
    mc::solo::yield(t);
    //mc::ssleep(2);
    t->sleep();
    mc::console("Thread Asleep!");
    mc::ssleep(2);
    t->awaken();
    mc::solo::join(t);
  }
  if constexpr ( false ) {
    auto t = mc::solo::spawn<mc::auto_thread<>>(fn_void);
    mc::solo::join(t);
  }
  return 1;
}
