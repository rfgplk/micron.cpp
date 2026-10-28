//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "basic_wayland_source.hpp"

#include "../keymap.hpp"

namespace micron::uxin
{
using first_party_wayland_source = basic_wayland_source<keymap::state>;
};
