//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/control.hpp"
#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"

#include "../src/thread/spawn.hpp"

#include "../src/sync/when.hpp"

int
main(void)
{
  bool t = false, f = false, g = false;
  mc::when_any([]() { mc::console("Finally true!"); }, &t, &f, &g);
  mc::ssleep(2);
  t = true;
  f = true;
  mc::ssleep(1);
  g = true;
  mc::ssleep(1);
  return 1;
}
