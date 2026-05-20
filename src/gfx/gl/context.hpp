//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"

#include "display.hpp"
#include "errors.hpp"
#include "hints.hpp"
#include "loader.hpp"
#include "opengl.hpp"
#include "window.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{
using namespace ::micron::gfx::platform;

class context
{
private:
  window *__win = nullptr;
  context_hints __hints{};
  glx_context_t __glx;
  egl_context_t __egl;
  bool __loaded = false;
  bool __has_swap_tear = false;

  void
  __build_x11()
  {
    glx_context_request_t cr;
    cr.major = __hints.major;
    cr.minor = __hints.minor;
    cr.core_profile = __hints.core_profile;
    cr.forward_compat = __hints.forward_compat;
    cr.debug = __hints.debug;
    if ( __hints.share_with ) cr.share = __hints.share_with->__glx.ctx();

    __glx.bind(glx_lib(), *__win->dpy().as_x11(), __win->glx_fbconfig());
    __glx.create(cr);

    __has_swap_tear = glx_lib().has_extension(*__win->dpy().as_x11(), "GLX_EXT_swap_control_tear");
  }

  void
  __build_wayland()
  {
    auto *wd = __win->dpy().as_wayland();
    auto *wwin = __win->as_wayland();
    if ( !wd || !wwin ) throw except::logic_error("gl::context: wayland window/display missing");

    egl_config_request_t cr_cfg;
    cr_cfg.depth_size = __hints.depth_bits;
    cr_cfg.stencil_size = __hints.stencil_bits;
    cr_cfg.samples = __hints.ms_samples;
    egl_context_request_t cr_ctx;
    cr_ctx.major = __hints.major;
    cr_ctx.minor = __hints.minor;
    cr_ctx.core_profile = __hints.core_profile;
    cr_ctx.forward_compat = __hints.forward_compat;
    cr_ctx.debug = __hints.debug;
    if ( __hints.share_with ) cr_ctx.share = __hints.share_with->__egl.ctx();

    __egl.open(egl_lib(), EGL_PLATFORM_WAYLAND_EXT, wd->display());
    __egl.choose_config(cr_cfg);
    __egl.create_context(cr_ctx);

    __egl.create_window_surface(__win->wl_native());

    __has_swap_tear = egl_lib().has_display_extension(__egl.display(), "EGL_EXT_swap_control_tear");
  }

  static context *__current_for_thunk;

  static void *
  __get_proc_thunk(const char *name) noexcept
  {
    return __current_for_thunk ? __current_for_thunk->proc_address(name) : nullptr;
  }

  static void
  __on_window_resize(void *data, i32 w, i32 h) noexcept
  {
    auto *self = static_cast<context *>(data);
    if ( self->__loaded ) {

      (void)self->make_current();
    }
    if ( glViewport ) glViewport(0, 0, w, h);
  }

public:
  ~context()
  {

    if ( __win ) __win->set_internal_resize_cb(nullptr, nullptr);
  }

  context(window &w, const context_hints &h = {}) : __win(&w), __hints(h)
  {
    switch ( w.dpy().backend() ) {
    case backend_tag_t::x11:
      __build_x11();
      break;
    case backend_tag_t::wayland:
      __build_wayland();
      break;
    case backend_tag_t::none:
    default:
      throw except::logic_error("gl::context: window's display has no backend");
    }

    __win->set_internal_resize_cb(&__on_window_resize, this);
  }

  context(window &w, bool make_current_now, const context_hints &h = {}) : context(w, h)
  {
    if ( make_current_now && !make_current() ) throw except::logic_error("gl::context: make_current() failed");
  }

  context(const context &) = delete;
  context(context &&) = delete;

  context &operator=(const context &) = delete;
  context &operator=(context &&) = delete;

  bool
  has_swap_control_tear() const noexcept
  {
    return __has_swap_tear;
  }

  const glx_context_t &
  glx() const noexcept
  {
    return __glx;
  }

  const egl_context_t &
  egl() const noexcept
  {
    return __egl;
  }

  bool
  make_current()
  {
    bool ok = false;
    switch ( __win->dpy().backend() ) {
    case backend_tag_t::x11:
      ok = __glx.make_current(static_cast<GLXDrawable>(__win->as_x11()->window()));
      break;
    case backend_tag_t::wayland:
      ok = __egl.make_current();
      break;
    default:
      ok = false;
      break;
    }
    if ( ok && !__loaded ) {

      __current_for_thunk = this;
      __load_core_4_6(__get_proc_thunk);
      __loaded = true;
      if ( __hints.debug ) install_default_debug_callback();
    }
    return ok;
  }

  void
  release() noexcept
  {
    if ( __win->dpy().backend() == backend_tag_t::x11 )
      __glx.release();
    else if ( __win->dpy().backend() == backend_tag_t::wayland )
      __egl.release();
  }

  void
  swap() noexcept
  {
    switch ( __win->dpy().backend() ) {
    case backend_tag_t::x11:
      if ( auto *xw = __win->as_x11() ) __glx.swap(static_cast<GLXDrawable>(xw->window()));
      break;
    case backend_tag_t::wayland:
      __egl.swap();

      if ( auto *wd = __win->dpy().as_wayland() ) wd->flush();
      break;
    default:
      break;
    }
  }

  void
  set_swap_interval(int interval) noexcept
  {
    if ( interval < 0 && !__has_swap_tear ) interval = -interval;
    if ( __win->dpy().backend() == backend_tag_t::x11 ) {
      if ( auto *xw = __win->as_x11() ) __glx.set_swap_interval(static_cast<GLXDrawable>(xw->window()), interval);
    } else if ( __win->dpy().backend() == backend_tag_t::wayland ) {
      __egl.set_swap_interval(interval);
    }
  }

  void *
  proc_address(const char *name) noexcept
  {
    switch ( __win->dpy().backend() ) {
    case backend_tag_t::x11:
      return __glx.proc_address(name);
    case backend_tag_t::wayland:
      return __egl.proc_address(name);
    default:
      return nullptr;
    }
  }
};

inline context *context::__current_for_thunk = nullptr;

};      // namespace gl
};      // namespace gfx
};      // namespace micron
