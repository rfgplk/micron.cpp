//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/control.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/process.hpp"
#include "../../src/linux/std.hpp"
#include "../../src/std.hpp"
#include "../../src/thread/signal.hpp"

#include "../../src/io/console.hpp"
#include "../../src/io/filesystem.hpp"

void
fn(int x, const mc::string &str, const mc::vector<int> &vec)
{
  mc::console("The argument provided was: ", x);
  mc::console("The string provided was: ", str);
  mc::console("The vec provided was: { ", vec[0], vec[1], vec[2], vec[3], vec[4], " }");
}

int
main(void)
{
  int pid = micron::wfork();
  if ( pid == 0 ) {
    mc::console("Return of wfork(): ", pid);
    mc::console("PID of child: ", micron::getpid());
    mc::console("From the forked process!");
    auto f = micron::create_processes("/bin/ls", "/bin/whoami");
    micron::run_processes(f);
    return 2;
  }
  mc::console("PID of parent: ", micron::getpid());
  mc::console("The return code was: ", pid);
  mc::console("Errno was: ", errno);
  mc::console("From the parent process!");

  mc::string str = "Hey!";
  mc::vector<int> vec = { 5, 3, 9, 5, 6 };
  pid = micron::wfork<fn>(5, str, vec);
  mc::console(pid);
  return 0;
}
