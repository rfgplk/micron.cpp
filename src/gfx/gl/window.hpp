//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/addr.hpp"
#include "../../type_traits.hpp"
#include "../platform/display.hpp"
#include "../platform/window.hpp"

#include "gl_libs.hpp"
#include "hints.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

namespace __impl
{
class owned_display_base
{
  using __display_t = platform::display;

protected:
  micron::aligned_storage_t<sizeof(__display_t), alignof(__display_t)> __dpy_storage;
  bool __owns_dpy = false;

  struct own_tag {
  };

  owned_display_base() noexcept = default;

  explicit owned_display_base(own_tag)
  {
    new (micron::addr(__dpy_storage)) __display_t();
    __owns_dpy = true;
  }

  owned_display_base(const owned_display_base &) = delete;
  owned_display_base &operator=(const owned_display_base &) = delete;

  ~owned_display_base()
  {
    if ( __owns_dpy ) __owned_ref().~__display_t();
  }

  __display_t &
  __owned_ref() noexcept
  {
    return *reinterpret_cast<__display_t *>(&__dpy_storage);
  }
};
};      // namespace __impl

class window: private __impl::owned_display_base, public platform::window
{
  context_hints __hints;

  static platform::x11_visual_choice
  __x11_pick(platform::x11_display_t &xd, platform::x11_lib_t &, void *user) noexcept
  {
    auto *h = static_cast<context_hints *>(user);
    glx_context_t tmp;
    glx_fbconfig_request_t fbreq;
    fbreq.depth_size = h->depth_bits;
    fbreq.stencil_size = h->stencil_bits;
    fbreq.samples = h->ms_samples;
    fbreq.double_buffer = h->double_buffer;
    tmp.pick_visual(glx_lib(), xd, fbreq);
    platform::x11_visual_choice c{};
    c.visual = tmp.take_visual();
    c.extra = reinterpret_cast<void *>(tmp.fbconfig());
    return c;
  }

  static void *
  __wl_create(platform::wl_surface *s, i32 w, i32 h, void *) noexcept
  {
    return wayland_egl_lib().wl_egl_window_create(s, w, h);
  }

  static void
  __wl_resize(void *native, i32 w, i32 h) noexcept
  {
    wayland_egl_lib().wl_egl_window_resize(static_cast<platform::wl_egl_window *>(native), w, h, 0, 0);
  }

  static void
  __wl_destroy(void *native) noexcept
  {
    wayland_egl_lib().wl_egl_window_destroy(static_cast<platform::wl_egl_window *>(native));
  }

  static platform::window_hooks
  __build_hooks(const context_hints &h) noexcept
  {
    static context_hints staged;
    staged = h;
    platform::window_hooks hooks{};
    hooks.x11_pick_visual = &__x11_pick;
    hooks.x11_user = &staged;
    hooks.wl_create_native = &__wl_create;
    hooks.wl_resize_native = &__wl_resize;
    hooks.wl_destroy_native = &__wl_destroy;
    return hooks;
  }

public:
  window(platform::display &dpy, i32 width, i32 height, const char *title, const context_hints &hints = {})
      : platform::window(dpy, width, height, title, __build_hooks(hints)), __hints(hints)
  {
  }

  window(i32 width, i32 height, const char *title, const context_hints &hints = {})
      : __impl::owned_display_base(__impl::owned_display_base::own_tag{}),
        platform::window(__owned_ref(), width, height, title, __build_hooks(hints)), __hints(hints)
  {
  }

  GLXFBConfig
  glx_fbconfig() const noexcept
  {
    return static_cast<GLXFBConfig>(x11_extra());
  }

  const context_hints &
  hints() const noexcept
  {
    return __hints;
  }

  using platform::window::set_close_func;
  using platform::window::set_expose_func;
  using platform::window::set_focus_func;
  using platform::window::set_move_func;
  using platform::window::set_resize_func;
  using platform::window::set_visibility_func;

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &, i32, i32>)
  void
  set_resize_func() noexcept
  {
    __user_resize = +[](void *self_v, i32 w, i32 h) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self, w, h);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &>)
  void
  set_close_func() noexcept
  {
    __user_close = +[](void *self_v) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &, bool>)
  void
  set_focus_func() noexcept
  {
    __user_focus = +[](void *self_v, bool focused) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self, focused);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &>)
  void
  set_expose_func() noexcept
  {
    __user_expose = +[](void *self_v) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &, bool>)
  void
  set_visibility_func() noexcept
  {
    __user_visibility = +[](void *self_v, bool visible) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self, visible);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), window &, i32, i32>)
  void
  set_move_func() noexcept
  {
    __user_move = +[](void *self_v, i32 x, i32 y) noexcept {
      auto &self = *static_cast<window *>(self_v);
      Fn(self, x, y);
    };
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
