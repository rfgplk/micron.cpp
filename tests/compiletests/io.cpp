//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron io/console porcelain compiles on every arch/opt.
// Not run (would emit to stdout); this only checks the template surface builds.

#include "../../src/io.hpp"
#include "../../src/io/console.hpp"

int
main()
{
  micron::io::print("compiletest\n");
  micron::io::println("line");
  micron::console("value: ", 42);
  micron::console(true);
  return 0;
}
