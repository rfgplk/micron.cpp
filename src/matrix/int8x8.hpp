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
using int8x8_t = int_matrix_base<i32, 8, 8>;
using uint8x8_t = int_matrix_base<u32, 8, 8>;
};
