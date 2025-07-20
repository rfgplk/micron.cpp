//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/std.h"

#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/token.hpp"

void
demo_callback(micron::mutex *p, void (micron::mutex::*b)())
{
  static int x = 0;
  mc::console("Token invoked: ", x++);
  (p->*b)();
}

int
main(void)
{
  mc::mutex m;
  if constexpr ( false ) {
    mc::lock_guard l(m);
    mc::console("Locked");
    mc::lock_guard c(m);
    mc::console("Locked again");
  }
  if constexpr ( false ) {
    mc::unique_lock<mc::lock_starts::defer> lck(m);
    mc::console("unique_lock created in deferred mode");
    lck.lock();
    mc::console("unique_lock is now locked");
    lck.lock();
    mc::cerror("Double locked");
  }
  if constexpr ( false ) {
    mc::unique_lock<mc::lock_starts::defer> lck(m);
    mc::console("unique_lock created in deferred mode");
    lck.lock();
    mc::console("unique_lock is now locked");
    auto *rls = lck.release();
    mc::console("Lock retrieved ", rls);
    mc::unique_lock<mc::lock_starts::locked> l2(rls);
    mc::cerror("Shouldn't be able to relock a locked lock");
  }
  if constexpr ( false ) {
    mc::mutex m1, m2, m3, m4;
    mc::lock(m1, m2, m3, m4);
    mc::console("Mutexes now locked");
    mc::console(!m1);
    mc::lock(m1, m2, m3, m4);
    mc::cerror("Shouldn't be able to relock");
  }
  if constexpr ( true ) {
    mc::token t(demo_callback);
    mc::console(t());     // true not set
    mc::console(t());     // false set + callback
    mc::console(t());     // true again
    mc::console(t());     // false again
  }
  return 0;
}
