//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "concepts.hpp"
#include "type_traits.hpp"
#include "types.hpp"

// a slice is a view of contiguous memory
// can be instantiated on it's own (allocating new memory) or can be used to
// access memory of other containers (view-like)
// in case of container access, no new memory is allocated
// always moves, never copies

// this is intended to be used in 9/10 of cases where you need 'memory' (in the broadest sense). ((instead of resorting
// to c-style arrs, or std::array or std::vector)) .begin() .end() [] () bitwise >> << (memory streaming) <=>

namespace micron
{

template <is_movable_object T, class C> struct slice;
};
