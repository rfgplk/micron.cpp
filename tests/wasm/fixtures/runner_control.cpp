// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
#include "../../../src/bits/wasm/host.hpp"
#ifndef MICRON_RUNNER_CONTROL
#define MICRON_RUNNER_CONTROL 1
#endif
int
main()
{
#if MICRON_RUNNER_CONTROL == 6
  micron::wasm::panic(6);
#elif MICRON_RUNNER_CONTROL == 70
  __builtin_trap();
#elif MICRON_RUNNER_CONTROL == 124
  for ( ;; ) __asm__ volatile("" ::: "memory");
#else
  return MICRON_RUNNER_CONTROL;
#endif
}
