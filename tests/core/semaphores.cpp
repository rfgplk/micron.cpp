//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/sync/semaphore.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

void func(void){
  mc::console("Function.");
}

int
main(void)
{
  mc::semaphore<func> sem;
  sem.run();
  sem.permit_ahead(5);
  sem.run();
  sem.run();
  sem.run();
  sem.run();
  return 0;
}
