//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"
static unsigned ctor_result = 0, once_calls = 0;

static int
guarded()
{
  static int value = [] {
    __atomic_fetch_add(&once_calls, 1u, __ATOMIC_RELAXED);
    return 81;
  }();
  return value;
}

static int
task(void *)
{
  return guarded();
}

struct boot_test {
  boot_test()
  {
#if defined(__micron_wasm_threads)
    micron::wasm::thread_handle h[4];
    for ( auto &v : h )
      if ( micron::wasm::spawn(task, nullptr, v) ) return;
    for ( auto &v : h ) {
      int r = 0;
      if ( micron::wasm::join(v, &r) || r != 81 ) return;
    }
    ctor_result = 1;
#else
    ctor_result = guarded() == 81;
#endif
  }
};

static boot_test boot;

int
main()
{
  return ctor_result == 1 && once_calls == 1 ? 1 : 6;
}
