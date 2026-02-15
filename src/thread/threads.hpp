//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

#include "../linux/__includes.hpp"
#include "../linux/sys/__threads.hpp"
#include "../linux/sys/resource.hpp"
#include "../linux/sys/system.hpp"
#include "../memory/stack_constants.hpp"

#include "../atomic/atomic.hpp"
#include "../control.hpp"
#include "../memory/cmemory.hpp"
#include "../memory/mman.hpp"
#include "../pointer.hpp"
#include "../sync/until.hpp"
#include "../sync/yield.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"

#include "signal.hpp"

#include "thread_types/auto_thread.hpp"
#include "thread_types/group_thread.hpp"
#include "thread_types/reg_thread.hpp"
