//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../../src/bits/wasm/runtime.hpp"
#include "../../../src/memory/new.hpp"
#include "../../../src/std.hpp"

int
from_a()
{
  int *p = new int(29);
  const int n = *p;
  delete p;
#if defined(__micron_eh)
  try {
    throw n;
  } catch ( int value ) {
    return value;
  }
#else
  return n;
#endif
}
