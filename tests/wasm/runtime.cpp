//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/runtime.hpp"
#include "../../src/bits/wasm/time.hpp"
#include "../../src/exit.hpp"
static unsigned constructors = 0, finalized = 0;

struct initialization {
  initialization() { ++constructors; }

  ~initialization() { ++finalized; }
};

static initialization object;

static int
guarded()
{
  static int value = [] { return 73; }();
  return value;
}

extern "C" __attribute__((export_name("runtime_finalized"))) unsigned
runtime_finalized()
{
  return finalized;
}

int
main()
{
  if ( constructors != 1 || guarded() != 73 || guarded() != 73 ) return 6;
  if ( !micron::wasm::stack_size() ) return 6;
  micron::timespec_t before{}, after{};
  if ( micron::clock_gettime(micron::clock_monotonic, before) || micron::clock_gettime(micron::clock_monotonic, after) ) return 6;
  if ( after.tv_sec < before.tv_sec || (after.tv_sec == before.tv_sec && after.tv_nsec < before.tv_nsec) ) return 6;
  if ( micron::clock_gettime(micron::clock_process_cputime_id, after) != -95 ) return 6;
  unsigned word = 0;
  if ( micron::wasm::wait32(&word, 1, 0) != -11 ) return 6;
#if !defined(__micron_wasm_threads)
  if ( micron::wasm::wait32(&word, 0, 0) != -95 ) return 6;
#else
  if ( micron::wasm::wait32(&word, 0, 0) != -110 ) return 6;
#endif
  return 1;
}
