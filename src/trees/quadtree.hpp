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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::quadtree
// a 2D point-region (PR / bucket) quadtree over a fixed universe box

template<typename Value, typename F = float, usize Capacity = 8, usize MaxDepth = 18>
using quadtree = __subdiv::pr_tree<Value, F, 2, Capacity, MaxDepth>;

};      // namespace micron
