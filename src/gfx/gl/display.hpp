//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../platform/display.hpp"
#include "gl_libs.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

using platform::backend_name;
using platform::backend_tag_t;
using platform::display;
using platform::env;
using platform::select_backend;
using platform::wayland_lib;
using platform::x11_lib;

};      // namespace gl
};      // namespace gfx
};      // namespace micron
