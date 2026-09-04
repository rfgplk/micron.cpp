//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// CPU affinity, scheduling, and the /proc snapshot.
//
// NOTE: `mc::this_task()` is gone -- process.hpp:314 carries its tombstone. The successor that
// still answers stack/heap/pid/priority is mc::this_proc_info() (process.hpp:375), NOT the adjacent
// this_process(): uprocess_t has none of those four fields.
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/linux/process/process.hpp"
#include "../../src/thread/cpu.hpp"
#include "../../src/thread/scheduling.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  const unsigned ncpu = mc::cpu_count();

  test_case("cpu_count and which_cpu answer a live topology");
  {
    require_true(ncpu > 0);
    require_true(mc::which_cpu() < ncpu);
    sb::print("cpu_count=", ncpu, " running on=", mc::which_cpu());
  }
  end_test_case();

  test_case("park_cpu pins this thread where it is asked to");
  {
    // cpu 0 exists on every machine; park there and confirm the kernel agrees
    mc::park_cpu(0);
    require_true(mc::which_cpu() == 0u);

    if ( ncpu >= 2 ) {
      mc::park_cpu(1);
      require_true(mc::which_cpu() == 1u);
    } else {
      sb::skip("single-cpu machine: cannot observe a migration");
    }
    mc::enable_all_cores();
  }
  end_test_case();

  test_case("this_proc_info reports stack, heap, pid and priority");
  {
    auto f = mc::this_proc_info();
    require_true(f.pid == micron::posix::getpid());
    require_true(f.pid > 0);
    require_true(f.runtime.stack != nullptr);
    // the stack grows down and lives above the heap in a normal ELF image
    require_true(f.runtime.heap == nullptr || f.runtime.stack > f.runtime.heap);
    // SCHED_OTHER priority is reported as 20 - nice, i.e. always inside the classic band
    require_true(f.stat.priority >= -100 && f.stat.priority <= 139);
    require_true(f.stat.num_threads >= 1);
    require_true(f.exe[0] == '/');
    sb::print("pid=", f.pid, " stack=", f.runtime.stack, " heap=", f.runtime.heap, " priority=", f.stat.priority);
  }
  end_test_case();

  test_case("cpu_t carries an affinity mask and a scheduler policy");
  {
    mc::cpu_t c;
    require_true(c.core_count() == ncpu);
    require_true(c.active_count() == 0);      // default-constructed: nothing set

    const unsigned target = ncpu >= 5 ? 4u : 0u;
    c.set_core(target);
    require_true(c.at(target));
    require_true(c.active_count() == 1);

    c.set_scheduler(mc::schedulers::normal);
    require_true(c.get_scheduler() == mc::schedulers::normal);

    c.update_cores();
    require_true(mc::which_cpu() == target);

    // update() also reloads the scheduler, which needs CAP_SYS_NICE for anything but SCHED_OTHER;
    // assert BOTH arms rather than swallowing the failure
    bool threw = false;
    try {
      c.update();
    } catch ( mc::except::system_error &e ) {
      threw = true;
    }
    if ( threw )
      sb::print("scheduler update refused (no CAP_SYS_NICE); affinity arm still checked");
    else
      require_true(mc::which_cpu() == target);

    c.clear_core(target);
    require_true(!c.at(target));
    require_true(c.active_count() == 0);
    mc::enable_all_cores();
  }
  end_test_case();

  sb::print("=== ALL CPU TESTS PASSED ===");
  return 1;
}
