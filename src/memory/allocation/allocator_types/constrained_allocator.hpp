//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_constrained
//
// use where retained capacity matters more than minimizing growth events
// default policy grants in 256b units, grows by 3/2;
// sets its shareable policy property;
// uses 16-byte default alignment; all sizing remains replaceable through P

namespace micron
{

template<is_policy P = constrained_allocation_policy> class allocator_constrained: public __abc_policy_allocator<P, 16>
{
};

};      // namespace micron
