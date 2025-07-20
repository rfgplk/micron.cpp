//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <type_traits>

namespace micron {
namespace sync {
template <typename F, typename... Args>
requires std::is_function_v<F>
void until(F f, typename... args)

};
};
