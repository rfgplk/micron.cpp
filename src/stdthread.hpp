//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// standard header file to include for when you need threading

// for the main thread spawning API
#include "thread/spawn.hpp"

// for the main solo:: API
#include "thread/thread.hpp"

// for async tasks
#include "sync/async.hpp"

// additional includes
#include "sync/futex.hpp"
#include "sync/future.hpp"
#include "sync/until.hpp"
#include "sync/when.hpp"
