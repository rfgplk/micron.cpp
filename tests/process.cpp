//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/linux/process/process.hpp"
#include "../src/control.hpp"
#include "../src/std.h"
#include "../src/thread/signal.hpp"

#include "../src/io/console.hpp"
#include "../src/io/filesystem.hpp"

void
test(void)
{
  while ( true ) {
    mc::console("Test callback");

    micron::sleep(2500);

  }
}

void
test2(void)
{
    micron::sleep(1000);
  while ( true ) {
    mc::console("Second test!");

    micron::sleep(2500);
  }
}
void
forever(int x)
{
  volatile int y = 0;
  for ( ;; ) {
    micron::sleep(x + y);
    y++;
  }
}

int
main(void)
{
  if ( false ) {
    micron::signal s(micron::signals::abort);
    s.mask();
    // s.on_signal(micron::signals::abort, test);
    s();
    mc::console("Mask");
  }
  // micron::stop();
  // micron::daemon(forever, 1000);
  micron::add_callback(test);
  micron::add_callback(test2);
  mc::console("Added callbacks, forking!");
  micron::wfork();
  mc::console("Forked!");
  // auto f = micron::create_processes("/bin/ls", "/bin/top", "/bin/ps");
  // micron::run_processes(f);
  // micron::process("/bin/whoami");
}
