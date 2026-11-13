//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/runtime.hpp"
static unsigned calls = 0;

static int
guarded()
{
  static int value = [] {
    ++calls;
#if defined(__micron_eh)
    if ( calls == 1 ) throw 7;
#endif
    return 17;
  }();
  return value;
}

int
main()
{
#if defined(__micron_eh)
  try {
    (void)guarded();
    return 6;
  } catch ( int n ) {
    if ( n != 7 ) return 6;
  }
  if ( guarded() != 17 || calls != 2 || guarded() != 17 || calls != 2 ) return 6;
#else
  if ( guarded() != 17 || calls != 1 ) return 6;
#endif
  return 1;
}
