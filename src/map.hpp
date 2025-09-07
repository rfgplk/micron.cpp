//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../src/maps/hopscotch.hpp"
#include "../src/maps/robin.hpp"
#include "../src/maps/swiss.hpp"

namespace micron
{
template <typename T> using fmap = micron::hopscotch_map<micron::hash64_t, T>;
template <typename K, typename V> using map = micron::robin_map<K, V>;
};
