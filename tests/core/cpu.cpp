//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/cpu.hpp"
#include "../../src/thread/scheduling.hpp"

int
main(void)
{
  mc::console("Was running on: ", mc::park_cpu_pid(0));
  mc::park_cpu(1);
  mc::console("Running on: ", mc::which_cpu());
  mc::set_priority(10);
  auto f = mc::this_task();
  mc::console("Stack starts at: ", f.stack);
  mc::console("Heap starts at: ", f.heap);
  mc::console("Pid is: ", f.pid);
  mc::console("Priority is: ", f.priority);
  mc::console("Number of CPUs: ", mc::cpu_count());
  mc::cpu_t c;
  c.set_core(4);
  c.set_scheduler(mc::schedulers::normal);
  c.update_cores();
  mc::console("Should be running on cpu #4: ", mc::which_cpu());
  mc::console("Current scheduler: ", (u64)c.get_scheduler());
  mc::console("Current priority: ", (u64)c.get_priority());
  try {
    c.update();
  } catch ( mc::except::system_error &e ) {
    mc::console("Error, invalid permissions: ", errno);
  }
  mc::console("Should be running on cpu #4: ", mc::which_cpu());
  return 0;
}
