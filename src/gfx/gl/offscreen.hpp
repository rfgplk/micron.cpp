//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "../platform/wayland.hpp"
#include "../platform/x11.hpp"
#include "display.hpp"
#include "hints.hpp"
#include "platform/egl.hpp"
#include "platform/glx.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{
using namespace ::micron::gfx::platform;

class offscreen_surface
{
  display *__dpy = nullptr;
  i32 __width = 0;
  i32 __height = 0;
  context_hints __hints{};
  glx_context_t __glx_ctx{};
  GLXPbuffer __glx_pbuf = 0;
  egl_context_t __egl_ctx{};
  EGLSurface __egl_pbuf = EGL_NO_SURFACE;

  void
  __build_x11()
  {
    auto *xd = __dpy->as_x11();
    if ( !xd ) throw except::logic_error("gl::offscreen_surface: display is not x11");
    glx_fbconfig_request_t fbreq;
    fbreq.depth_size = __hints.depth_bits;
    fbreq.stencil_size = __hints.stencil_bits;
    fbreq.samples = __hints.ms_samples;
    fbreq.double_buffer = false;
    __glx_ctx.pick_pbuffer_fbconfig(glx_lib(), *xd, fbreq);
    __glx_pbuf = __glx_ctx.create_pbuffer(__width, __height);
    if ( !__glx_pbuf ) throw except::library_error("offscreen_surface: glXCreatePbuffer returned 0");
  }

  void
  __build_wayland()
  {
    auto *wd = __dpy->as_wayland();
    if ( !wd ) throw except::logic_error("gl::offscreen_surface: display is not wayland");
    egl_config_request_t cr_cfg;
    cr_cfg.depth_size = __hints.depth_bits;
    cr_cfg.stencil_size = __hints.stencil_bits;
    cr_cfg.samples = __hints.ms_samples;
    cr_cfg.surface_type = EGL_PBUFFER_BIT;
    __egl_ctx.open(egl_lib(), EGL_PLATFORM_WAYLAND_EXT, wd->display());
    __egl_ctx.choose_config(cr_cfg);
    __egl_pbuf = __egl_ctx.create_pbuffer_surface(__width, __height);
    if ( __egl_pbuf == EGL_NO_SURFACE ) throw except::library_error("offscreen_surface: eglCreatePbufferSurface failed");
    __egl_ctx.attach_surface(__egl_pbuf);
  }

public:
  ~offscreen_surface()
  {
    if ( __glx_pbuf ) __glx_ctx.destroy_pbuffer(__glx_pbuf);
  }

  offscreen_surface() = default;

  offscreen_surface(display &dpy, i32 width, i32 height, const context_hints &hints = {})
      : __dpy(&dpy), __width(width), __height(height), __hints(hints)
  {
    switch ( dpy.backend() ) {
    case backend_tag_t::x11:
      __build_x11();
      break;
    case backend_tag_t::wayland:
      __build_wayland();
      break;
    case backend_tag_t::none:
    default:
      throw except::logic_error("gl::offscreen_surface: display has no backend");
    }
  }

  offscreen_surface(const offscreen_surface &) = delete;
  offscreen_surface(offscreen_surface &&) = delete;

  offscreen_surface &operator=(const offscreen_surface &) = delete;
  offscreen_surface &operator=(offscreen_surface &&) = delete;

  i32
  width() const noexcept
  {
    return __width;
  }

  i32
  height() const noexcept
  {
    return __height;
  }

  display &
  dpy() noexcept
  {
    return *__dpy;
  }

  const context_hints &
  hints() const noexcept
  {
    return __hints;
  }

  GLXPbuffer
  glx_pbuffer() const noexcept
  {
    return __glx_pbuf;
  }

  GLXFBConfig
  glx_fbconfig() const noexcept
  {
    return __glx_ctx.fbconfig();
  }

  EGLSurface
  egl_surface() const noexcept
  {
    return __egl_pbuf;
  }

  EGLDisplay
  egl_display() const noexcept
  {
    return __egl_ctx.display();
  }

  EGLConfig
  egl_config() const noexcept
  {
    return __egl_ctx.config();
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
