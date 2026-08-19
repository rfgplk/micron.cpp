//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "display.hpp"
#include "wayland.hpp"
#include "x11.hpp"

namespace micron
{
namespace gfx
{
namespace platform
{

struct x11_visual_choice {
  XVisualInfo *visual = nullptr;
  void *extra = nullptr;
};

struct window_hooks {
  x11_visual_choice (*x11_pick_visual)(x11_display_t &, x11_lib_t &, void *user) = nullptr;
  void *x11_user = nullptr;

  void *(*wl_create_native)(wl_surface *, i32 w, i32 h, void *user) = nullptr;
  void (*wl_resize_native)(void *native, i32 w, i32 h) = nullptr;
  void (*wl_destroy_native)(void *native) = nullptr;
  void *wl_user = nullptr;
};

class window
{
protected:
  display *__dpy = nullptr;
  x11_window_t __x11;
  wayland_window_t __wl;
  XVisualInfo *__x11_visual = nullptr;
  XVisualInfo __x11_visual_storage{};
  void *__x11_extra = nullptr;
  void *__wl_native = nullptr;
  window_hooks __hooks{};

  void (*__user_resize)(void *, i32, i32) noexcept = nullptr;
  void (*__user_close)(void *) noexcept = nullptr;
  void (*__user_focus)(void *, bool) noexcept = nullptr;
  void (*__user_expose)(void *) noexcept = nullptr;
  void (*__user_visibility)(void *, bool) noexcept = nullptr;
  void (*__user_move)(void *, i32, i32) noexcept = nullptr;

  void (*__internal_resize)(void *, i32, i32) noexcept = nullptr;
  void *__internal_resize_data = nullptr;

  static void
  __platform_resize_thunk(void *self_v, i32 w, i32 h) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__wl_native && self.__hooks.wl_resize_native ) self.__hooks.wl_resize_native(self.__wl_native, w, h);
    if ( self.__internal_resize ) self.__internal_resize(self.__internal_resize_data, w, h);
    if ( self.__user_resize ) self.__user_resize(self_v, w, h);
  }

  static void
  __platform_close_thunk(void *self_v) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__user_close ) self.__user_close(self_v);
    (void)self;
  }

  static void
  __platform_focus_thunk(void *self_v, bool f) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__user_focus ) self.__user_focus(self_v, f);
    (void)self;
  }

  static void
  __platform_expose_thunk(void *self_v) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__user_expose ) self.__user_expose(self_v);
    (void)self;
  }

  static void
  __platform_visibility_thunk(void *self_v, bool v) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__user_visibility ) self.__user_visibility(self_v, v);
    (void)self;
  }

  static void
  __platform_move_thunk(void *self_v, i32 x, i32 y) noexcept
  {
    auto &self = *static_cast<window *>(self_v);
    if ( self.__user_move ) self.__user_move(self_v, x, y);
    (void)self;
  }

  void
  __build_x11(i32 width, i32 height, const char *title)
  {
    auto *xd = __dpy->as_x11();
    if ( !xd ) throw except::logic_error("gfx::window: display is not x11 but backend says x11");

    if ( __hooks.x11_pick_visual ) {
      auto choice = __hooks.x11_pick_visual(*xd, x11_lib(), __hooks.x11_user);
      __x11_visual = choice.visual;
      __x11_extra = choice.extra;
    } else {

      auto &lib = x11_lib();
      if ( !lib.XMatchVisualInfo ) throw except::library_error("gfx::window: x11_pick_visual hook absent and XMatchVisualInfo unavailable");
      const int xscreen = xd->screen();

      if ( !lib.XMatchVisualInfo(xd->display(), xscreen, 24, TrueColor, &__x11_visual_storage)
           && !lib.XMatchVisualInfo(xd->display(), xscreen, 32, TrueColor, &__x11_visual_storage) ) {
        throw except::library_error("gfx::window: XMatchVisualInfo found no TrueColor visual");
      }
      __x11_visual = &__x11_visual_storage;
    }
    if ( !__x11_visual ) throw except::library_error("gfx::window: no X11 visual picked");

    __x11.create(*xd, __x11_visual, width, height, title);
  }

  void
  __build_wayland(i32 width, i32 height, const char *title)
  {
    auto *wd = __dpy->as_wayland();
    if ( !wd ) throw except::logic_error("gfx::window: display is not wayland but backend says wayland");
    __wl.create(*wd, width, height, title);

    if ( __hooks.wl_create_native ) __wl_native = __hooks.wl_create_native(__wl.surface(), width, height, __hooks.wl_user);
  }

  void
  __install_platform_thunks() noexcept
  {
    __x11.set_owner(this);
    __wl.set_owner(this);
    __x11.set_resize_cb(&__platform_resize_thunk);
    __wl.set_resize_cb(&__platform_resize_thunk);
    __x11.set_close_cb(&__platform_close_thunk);
    __wl.set_close_cb(&__platform_close_thunk);
    __x11.set_focus_cb(&__platform_focus_thunk);
    __wl.set_focus_cb(&__platform_focus_thunk);
    __x11.set_expose_cb(&__platform_expose_thunk);
    __wl.set_expose_cb(&__platform_expose_thunk);
    __x11.set_visibility_cb(&__platform_visibility_thunk);
    __wl.set_visibility_cb(&__platform_visibility_thunk);
    __x11.set_move_cb(&__platform_move_thunk);
    __wl.set_move_cb(&__platform_move_thunk);
  }

public:
  ~window()
  {
    if ( __wl_native && __hooks.wl_destroy_native ) __hooks.wl_destroy_native(__wl_native);
    __wl_native = nullptr;
  }

  window(display &dpy, i32 width, i32 height, const char *title, const window_hooks &hooks = {}) : __dpy(&dpy), __hooks(hooks)
  {
    switch ( dpy.backend() ) {
    case backend_tag_t::x11:
      __build_x11(width, height, title);
      break;
    case backend_tag_t::wayland:
      __build_wayland(width, height, title);
      break;
    case backend_tag_t::none:
    default:
      throw except::logic_error("gfx::window: display has no backend");
    }
    __install_platform_thunks();
  }

  window(const window &) = delete;
  window(window &&) = delete;

  window &operator=(const window &) = delete;
  window &operator=(window &&) = delete;

  i32
  width() const noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 ) return __x11.width();
    return __wl.width();
  }

  i32
  height() const noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 ) return __x11.height();
    return __wl.height();
  }

  bool
  should_close() const noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 ) return __x11.should_close();
    return __wl.should_close();
  }

  void
  set_title(const char *t) noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 )
      __x11.set_title(t);
    else if ( __dpy->backend() == backend_tag_t::wayland )
      __wl.set_title(t);
  }

  void
  resize(i32 w, i32 h) noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 )
      __x11.resize(w, h);
    else if ( __dpy->backend() == backend_tag_t::wayland ) {
      __wl.resize(w, h);
      if ( __wl_native && __hooks.wl_resize_native ) __hooks.wl_resize_native(__wl_native, w, h);
    }
    if ( __internal_resize ) __internal_resize(__internal_resize_data, w, h);
  }

  void
  set_internal_resize_cb(void (*f)(void *, i32, i32) noexcept, void *data) noexcept
  {
    __internal_resize = f;
    __internal_resize_data = data;
  }

  void
  poll_events() noexcept
  {
    if ( __dpy->backend() == backend_tag_t::x11 )
      __x11.poll_events();
    else if ( __dpy->backend() == backend_tag_t::wayland )
      __wl.poll_events();
  }

  x11_window_t *
  as_x11() noexcept
  {
    return __dpy->backend() == backend_tag_t::x11 ? &__x11 : nullptr;
  }

  wayland_window_t *
  as_wayland() noexcept
  {
    return __dpy->backend() == backend_tag_t::wayland ? &__wl : nullptr;
  }

  display &
  dpy() noexcept
  {
    return *__dpy;
  }

  const display &
  dpy() const noexcept
  {
    return *__dpy;
  }

  XVisualInfo *
  x11_visual() const noexcept
  {
    return __x11_visual;
  }

  void *
  x11_extra() const noexcept
  {
    return __x11_extra;
  }

  void *
  wl_native() const noexcept
  {
    return __wl_native;
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), i32, i32> || micron::is_invocable_v<decltype(Fn), window &, i32, i32>)
  void
  set_resize_func() noexcept
  {
    __user_resize = +[](void *self_v, i32 w, i32 h) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn), i32, i32> )
        Fn(w, h);
      else
        Fn(self, w, h);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn)> || micron::is_invocable_v<decltype(Fn), window &>)
  void
  set_close_func() noexcept
  {
    __user_close = +[](void *self_v) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn)> )
        Fn();
      else
        Fn(self);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), bool> || micron::is_invocable_v<decltype(Fn), window &, bool>)
  void
  set_focus_func() noexcept
  {
    __user_focus = +[](void *self_v, bool focused) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn), bool> )
        Fn(focused);
      else
        Fn(self, focused);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn)> || micron::is_invocable_v<decltype(Fn), window &>)
  void
  set_expose_func() noexcept
  {
    __user_expose = +[](void *self_v) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn)> )
        Fn();
      else
        Fn(self);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), bool> || micron::is_invocable_v<decltype(Fn), window &, bool>)
  void
  set_visibility_func() noexcept
  {
    __user_visibility = +[](void *self_v, bool visible) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn), bool> )
        Fn(visible);
      else
        Fn(self, visible);
    };
  }

  template<auto Fn>
    requires(micron::is_invocable_v<decltype(Fn), i32, i32> || micron::is_invocable_v<decltype(Fn), window &, i32, i32>)
  void
  set_move_func() noexcept
  {
    __user_move = +[](void *self_v, i32 x, i32 y) noexcept {
      auto &self = *static_cast<window *>(self_v);
      if constexpr ( micron::is_invocable_v<decltype(Fn), i32, i32> )
        Fn(x, y);
      else
        Fn(self, x, y);
    };
  }

  void
  clear_resize_func() noexcept
  {
    __user_resize = nullptr;
  }

  void
  clear_close_func() noexcept
  {
    __user_close = nullptr;
  }

  void
  clear_focus_func() noexcept
  {
    __user_focus = nullptr;
  }

  void
  clear_expose_func() noexcept
  {
    __user_expose = nullptr;
  }

  void
  clear_visibility_func() noexcept
  {
    __user_visibility = nullptr;
  }

  void
  clear_move_func() noexcept
  {
    __user_move = nullptr;
  }
};

};      // namespace platform
};      // namespace gfx
};      // namespace micron
