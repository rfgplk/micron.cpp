//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/token.hpp"
#include "../types.hpp"

#include <type_traits>

namespace micron
{
enum class states {
  RED, YELLOW, GREEN
};

enum class priority {
  low, regular, high, realtime
};

struct permit{};

// synchronization mechanism across threads
class semaphore
{
  micron::token tk;
public:

};
};     // namespace micron
