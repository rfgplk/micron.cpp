//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// THE CELL THAT LINKS, WHICH IS THE ONLY REASON IT EXISTS
//
// Every one of the 58 `compile` cells in verify_compile_barebones.duck is `--raw-obj`. Not one of
// them links, and neither does anything in tests/compiletests/. That blind spot hid a hard link
// failure in a top-level public umbrella:
//
//   ./src/memory/allocation/__seam.hpp:37: warning: inline function
//       'bool micron::__heap_owns(const void*)' used but never defined
//   undefined reference to `micron::__heap_owns(void const*)'
//
// __seam.hpp declared __heap_owns `inline` and left the definition to allocation/__internal.hpp,
// "later in the same TU" -- which holds for std.hpp, vector.hpp, maps.hpp and print.hpp, and does
// NOT hold for array.hpp, memory/cmemory.hpp or memory/cstring.hpp. Those three are exactly the
// umbrellas the seam exists to keep cheap, so the failure lived where nobody looked.
//
// It is in tests/build/ and not tests/compiletests/ on purpose: both verify_compile_{gcc,clang}.duck
// sweep that directory BY DIRECTORY with --raw-obj, so a file whose whole assertion is "this links"
// would have its assertion compiled away in ~115 cells. Same reason poison_include_path.cpp lives
// here.
//
// Uses the umbrella that actually broke, and calls a function that odr-uses the seam:
// micron::smemset reaches __is_at_heap -> micron::__heap_owns.

#include <micron/array.hpp>

static unsigned char __buf[64];

int
main()
{
  micron::smemset(__buf, 0, sizeof(__buf));
  // and the seam's own answer, so the call cannot be optimised out before the symbol is needed
  return micron::__heap_owns(__buf) ? 1 : 1;
}
