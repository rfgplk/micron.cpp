//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// LIBGCC INTEGER SHIM, FOR MODULES
//
// GCC lowers a few integer operations to libgcc calls it cannot inline; 
// on amd64 (__popcountsi2, __popcountdi2, and (given __int128) __udivti3 / __umodti3)

#include <micron/math/__gcc_int_syms.hpp>
