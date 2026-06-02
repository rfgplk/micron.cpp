//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "__subdiv_tree.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::octree
// a 3D point-region (PR / bucket) octree over a fixed universe box

template<typename Value, typename F = float, usize Capacity = 8, usize MaxDepth = 12>
using octree = __subdiv::pr_tree<Value, F, 3, Capacity, MaxDepth>;

};      // namespace micron
