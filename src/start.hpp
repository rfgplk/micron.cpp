//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "bits/__arch.hpp"
#if defined(__micron_arch_wasm32)
#include "bits/wasm/runtime.hpp"
#else

// call up main()
___attribute__((noreturn)) inline __attribute__((always_inline)) __call_main(int (*main)(int, char **, char **), int argc, char **argv)
{
  //_Exit(main(argc, argv));
}

static inline __attribute__((always_inline)) int
_start()
{
  __call_main();
}

#endif      // __micron_arch_wasm32
