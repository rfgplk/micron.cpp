//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// THE SECOND TRANSLATION UNIT. That is the entire test.
//
// micron is single-TU by design and every gate in the tree asks "does this compile", so a header
// that defines an ordinary external-linkage symbol is invisible: errno.hpp had
//
//     i32 __micron_errno = 0;                       // .globl, no comdat
//     i32 *__micron_errno_location(void) { ... }    // .globl, no comdat
//
// on BOTH arms (.tbss hosted, .bss under MICRON_NO_TLS). Two TUs is a multiple-definition link
// error, and A .ko IS INHERENTLY MULTI-TU -- micron_demo-y is three objects and Kbuild passes
// -fno-common. It linked only because exactly one of those objects pulls a container header, a
// constraint mc_libgcc.cpp:33-34 had to document precisely because of this.
//
// Same class BAREBONES.md records for micron::except::__write_n, which was `void` where it needed
// to be `inline void`. Link this against tests/build/seam_link.cpp (or anything else that reaches
// errno.hpp) and the pair must resolve.

#include <micron/array.hpp>

int
__micron_second_tu_probe(void)
{
  return errno;
}
