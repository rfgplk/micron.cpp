//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// the host allocator itself moved down to __bits/, where __vk_deleters.hpp can include it
#include "__bits/__vk_host_alloc.hpp"

#include "vulkan.hpp"
