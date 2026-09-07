//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// LIBGCC SHIM, FOR A BOARD

#include <micron/bits/__arch.hpp>

#if defined(__micron_no_fp)
#include <micron/math/__gcc_int_syms.hpp>
#else
#include <micron/math/__gcc_math_syms.hpp>
#endif
