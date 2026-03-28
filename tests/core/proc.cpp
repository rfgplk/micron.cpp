//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/control.hpp"
#include "../src/linux/process/exec.hpp"
#include "../src/linux/process/fork.hpp"
#include "../src/linux/process/process.hpp"
#include "../src/linux/process/signals.hpp"
#include "../src/std.hpp"

#include "../src/io/console.hpp"
#include "../src/io/filesystem.hpp"

int
main(void)
{
  auto proc = micron::this_process();
  mc::console(proc.pids.pid);
  mc::console(proc.pids.uid);
  mc::console(proc.pids.gid);
  mc::console(proc.path);
  mc::console(proc.argv);
  auto proc_info = micron::this_proc_info();
  mc::console(proc_info.pid);
  mc::console(proc_info.stat.utime);
  mc::console(proc_info.stat.cutime);
  mc::console(proc_info.stat.priority);
  mc::console(proc_info.status.vm_peak);
  mc::console(proc_info.status.vm_data);
  mc::console(proc_info.status.sig_pnd);
  mc::console(proc_info.ppid);
  mc::console(proc_info.runtime.stack);
  return 1;
};
