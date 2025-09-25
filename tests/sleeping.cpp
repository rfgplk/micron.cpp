//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/control.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

int
main(void)
{
  mc::io::stdoutln("Will sleep for 3 seconds");
  mc::sleep(3000);
  mc::io::stdoutln("Awakened.");

  mc::io::stdoutln("Will pause until SIG_CONT");
  mc::pause();
  mc::io::stdoutln("Finishing");
  return 1;
}
