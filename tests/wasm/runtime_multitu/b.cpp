//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../../src/bits/wasm/runtime.hpp"
#include "../../../src/memory/new.hpp"
#include "../../../src/std.hpp"
static int initialized = 31;

int
from_b()
{
  auto *p = new int[8];
  p[7] = initialized;
  int n = p[7];
  delete[] p;
#if defined(__micron_eh)
  try {
    throw n;
  } catch ( const int &value ) {
    return value;
  }
#else
  return n;
#endif
}
