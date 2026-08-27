//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocator_serial
//
// use as the general purpose container allocator;
// default policy starts at one page, rounds capacity to pages, grows by 3x,
// and provides 64-byte default alignment; supply another policy when those sizing rules do not fit the workload

namespace micron
{

template<is_policy P = serial_allocation_policy> class allocator_serial: public __abc_policy_allocator<P, 64>
{
};

};      // namespace micron
