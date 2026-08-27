//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_exact
//
// use when the granted allocation should track the requested byte count instead of reserving geometric slack;
// default policy has no minimum, one-byte granularity, 1x growth, and 16-byte default alignment

namespace micron
{

template<is_policy P = exact_allocation_policy> class allocator_exact: public __abc_policy_allocator<P, 16>
{
};

};      // namespace micron
