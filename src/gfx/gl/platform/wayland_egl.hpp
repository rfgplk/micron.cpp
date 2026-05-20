//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "../../../linux/dynamic.hpp"
#include "../../../types.hpp"

#include "../../platform/__bits/__wayland_types.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

struct wayland_egl_lib_t {
  host_dso host;

  platform::PFN_wl_egl_window_create wl_egl_window_create = nullptr;
  platform::PFN_wl_egl_window_destroy wl_egl_window_destroy = nullptr;
  platform::PFN_wl_egl_window_resize wl_egl_window_resize = nullptr;

  wayland_egl_lib_t() : host("libwayland-egl.so.1")
  {
    wl_egl_window_create = host.sym_as<platform::PFN_wl_egl_window_create>("wl_egl_window_create");
    wl_egl_window_destroy = host.sym_as<platform::PFN_wl_egl_window_destroy>("wl_egl_window_destroy");
    wl_egl_window_resize = host.sym_as<platform::PFN_wl_egl_window_resize>("wl_egl_window_resize");
    if ( !wl_egl_window_create || !wl_egl_window_destroy ) {
      throw except::library_error("wayland-egl: required entry points missing in libwayland-egl.so.1");
    }
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
