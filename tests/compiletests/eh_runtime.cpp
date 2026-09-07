//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE EXCEPTION RUNTIME, COMPILED.
//
// Built, never run. Before Phase 4 NOTHING in tests/compiletests/ reached bits/__eh.hpp -- the EH
// runtime was only ever compiled through <crt>/eh_runtime.cpp at link time, so every header under
// bits/eh/ was outside the compile matrix entirely. That is how __MICRON_EH_DEBUG came to have zero
// cells in verify_compile_gcc.duck and zero in verify_compile_clang.duck.
//
// This file exists so the -ke path and its debug arm are actually compiled. It is the target of the
// ehdebug_* cells in verify_compile_barebones.duck; a broken bits/eh/*.hpp fails HERE rather than in
// a link that only runs in a handful of cells.
//
// Requires -ke (or -D__micron_eh): every header below is inside #if defined(__micron_eh).

#include "../../src/bits/__eh.hpp"

#if defined(__micron_eh)
#include "../../src/bits/eh/cxa_throw.hpp"
#include "../../src/bits/eh/eh_debug.hpp"
#include "../../src/bits/eh/terminate.hpp"
#endif

namespace
{

#if defined(__micron_eh)
// pin the debug sinks: under __MICRON_EH_DEBUG these are the port::write_diag calls Phase 4 routed,
// and under a normal -ke build they must still compile to nothing.
[[gnu::used]] void
debug_sinks()
{
  micron::eh::__dbg_s("eh compiletest");
  micron::eh::__dbg_h(0xDEADBEEFu);
}

// pin the emergency pool, which Phase 4 moved off a raw mmap onto port::raw_map
[[gnu::used]] void *
pool(usize n)
{
  void *p = micron::eh::__eh_raw_map(n);
  if ( p != nullptr ) micron::eh::__eh_raw_unmap(p, n);
  return p;
}
#endif

}      // namespace

int
main()
{
  return 1;
}
