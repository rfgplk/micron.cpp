//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron::string surface compiles on every arch/opt. Not run.

#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

int
main()
{
  micron::string s("hello");
  s += " world";
  micron::string t(5, 'z');
  int acc = static_cast<int>(s.size() + t.size());

  micron::vector<micron::string> vs;
  vs.push_back(s);
  vs.emplace_back("compiletest");
  acc += static_cast<int>(vs.size());

  return acc & 0x7f;
}
