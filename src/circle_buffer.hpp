//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "vector/circle_vector.hpp"

namespace micron
{
template <typename T, size_t N> using circular = circle_vector<T, N>;
template <typename T, size_t N> using circle_buffer = circle_vector<T, N>;

};     // namespace micron
