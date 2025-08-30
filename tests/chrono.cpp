//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/chrono.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"

int
main(void)
{
  mc::system_clock clock;
  clock.start();
  clock.stop();
  mc::console(clock.now());
  return 0;
}
