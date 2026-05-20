//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../platform/display.hpp"

#include "platform/egl.hpp"
#include "platform/glx.hpp"
#include "platform/wayland_egl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

inline glx_lib_t &
glx_lib() noexcept(false)
{
  static glx_lib_t lib;
  return lib;
}

inline egl_lib_t &
egl_lib() noexcept(false)
{
  static egl_lib_t lib;
  return lib;
}

inline wayland_egl_lib_t &
wayland_egl_lib() noexcept(false)
{
  static wayland_egl_lib_t lib;
  return lib;
}

};      // namespace gl
};      // namespace gfx
};      // namespace micron
