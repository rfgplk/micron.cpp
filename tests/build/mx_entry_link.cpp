//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// THE --mx ENTRY, LINKED. Nothing in the tree did this, and that is how it broke.
//
// `duck --mx` swaps start.cpp's entry body for __micron_mxc, which hands the user entry a
// micron_mx_entry_args * instead of (argc, argv, envp). The descriptor lives in start/mx_entry.hpp.
// It used to live in src/attach/mx_entry.hpp, reached as <micron/attach/mx_entry.hpp> -- and when
// the Phase 4 prune deleted src/attach/, that include did not fail. On a machine carrying an
// installed snapshot it resolved to /usr/include/micron/attach/mx_entry.hpp, dragged a second copy
// of types.hpp in behind it, and produced ~790 "redefinition of micron::integral_constant" errors
// that named nothing about mx. The whole hosted freestanding link went with it: `-k`, `-ke`,
// `--direct` and `--mx` alike.
//
// So this file's assertion is not that mx works. It is that the CRT STILL LINKS -- and it is in
// tests/build/ rather than tests/compiletests/ because both verify_compile_{gcc,clang}.duck sweep
// that directory with --raw-obj, which would compile the assertion away.
//
// It also pins the descriptor's ABI, which is the part a loader on the other side depends on:
// 128 bytes, abi == 1, and size == what the CRT was compiled with. mxf reads those three to decide
// whether it can call this image at all.
//
//   duck build tests/build/mx_entry_link.cpp --mx -k --start ./start -i . -i ./src -o bin/mx
//
// NOTE THE SYMBOL NAME. MICRON_MX_ENTRY defaults to "entry" (start.cpp:35), not "mx_main" -- the
// __asm__ label is what the CRT calls, so a mismatch is an undefined reference to `entry' and not
// anything that mentions this file.

#include "../../start/mx_entry.hpp"

#include <micron/print.hpp>

extern "C" int
entry(const micron_mx_entry_args *__a)
{
  if ( !micron::__mx_entry_args_valid(__a) ) return 6;

  // the three fields the ABI contract is made of
  if ( __a->abi != micron_mx_entry_abi ) return 6;
  if ( __a->size != sizeof(micron_mx_entry_args) ) return 6;
  if ( sizeof(micron_mx_entry_args) != 128 ) return 6;

  // argv/envp/auxv are non-null or __mx_entry_args_valid would have refused; the stack flag is set
  // by __micron_mxc whenever __stack_init found the main stack, which on linux it always does
  if ( (__a->flags & micron_mx_entry_f_have_stack) == 0 ) return 6;
  if ( __a->stack_hi <= __a->stack_lo ) return 6;

  micron::println("mx: abi=", __a->abi, " size=", __a->size, " argc=", __a->argc, " flags=", __a->flags);
  return 1;      // micron's PASS sentinel
}
