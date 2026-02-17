//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

// NOTE: these matrix types take a lot of inspiration from ARMs NEON SIMD extensions
namespace micron
{
using int4x4_t = int_matrix_base<i32, 4, 4>;
using uint4x4_t = int_matrix_base<u32, 4, 4>;
};
