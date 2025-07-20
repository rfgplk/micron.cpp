//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/control.hpp"
#include "../src/io/print.hpp"
#include "../src/std.h"

int
main(void)
{
  mc::io::println("Will sleep for 3 seconds");
  mc::sleep(3000);
  mc::io::println("Awakened.");

  mc::io::println("Will pause until SIG_CONT");
  mc::pause();
  mc::io::println("Finishing");
  return 1;
}
