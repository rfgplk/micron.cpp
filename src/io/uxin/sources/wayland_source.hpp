//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "../../../gl/display.hpp"
#include "../../../gl/platform/wayland.hpp"
#include "../../../linux/io/sys.hpp"
#include "../../../linux/sys/event_codes.hpp"
#include "../../../linux/sys/input.hpp"
#include "../../../memory/mman.hpp"
#include "../../../memory/mmap_bits.hpp"

#include "../wayland_seat.hpp"
#include "../xkbcommon.hpp"

namespace micron
{
namespace uxin
{

// primary wayland input source
class wayland_source
{
  static void
  __on_global(void *data, gl::wl_registry *registry, u32 name, const char *interface, u32 version) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    auto streq = [](const char *a, const char *b) {
      while ( *a && *a == *b ) {
        ++a;
        ++b;
      }
      return *a == 0 && *b == 0;
    };
    if ( streq(interface, "wl_seat") && !self->__seat ) {
      const u32 use_ver = version < 7 ? version : 7u;
      void *p = self->__lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(registry), 0 /*wl_registry.bind*/,
                                                    &__wl_seat_interface, use_ver, 0, name, "wl_seat", use_ver, nullptr);
      self->__seat = reinterpret_cast<gl::wl_seat *>(p);
      self->__seatversion_ = use_ver;

      static wl_seat_listener_t listener{ &__on_seat_capabilities, &__on_seat_name };
      self->__lib->wl_proxy_add_listener(reinterpret_cast<gl::wl_proxy *>(self->__seat), reinterpret_cast<void (**)(void)>(&listener),
                                         self);
    }
  }

  static void
  __on_global_remove(void *, gl::wl_registry *, u32) noexcept
  {
  }

  static void
  __on_seat_capabilities(void *data, gl::wl_seat *seat, u32 caps) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( (caps & wl_seat_capability_pointer) && !self->pointer_ ) {
      void *p = self->__lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(seat), wl_seat_ops::wl_seat_get_pointer,
                                                    &__wl_pointer_interface, self->__seat_version, 0, nullptr);
      self->pointer_ = reinterpret_cast<gl::wl_pointer *>(p);
      static wl_pointer_listener_t plistener{
        &__on_p_enter,       &__on_p_leave,     &__on_p_motion,        &__on_p_button,    &__on_p_axis,         &__on_p_frame,
        &__on_p_axis_source, &__on_p_axis_stop, &__on_p_axis_discrete, &__on_p_axis_v120, &__on_p_axis_rel_dir,
      };
      self->__lib->wl_proxy_add_listener(reinterpret_cast<gl::wl_proxy *>(self->pointer_), reinterpret_cast<void (**)(void)>(&plistener),
                                         self);
    }
    if ( (caps & wl_seat_capability_keyboard) && !self->keyboard_ ) {
      void *p = self->__lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(seat), wl_seat_ops::wl_seat_get_keyboard,
                                                    &__wl_keyboard_interface, self->__seat_version, 0, nullptr);
      self->keyboard_ = reinterpret_cast<gl::wl_keyboard *>(p);
      static wl_keyboard_listener_t klistener{
        &__on_k_keymap, &__on_k_enter, &__on_k_leave, &__on_k_key, &__on_k_modifiers, &__on_k_repeat_info,
      };
      self->__lib->wl_proxy_add_listener(reinterpret_cast<gl::wl_proxy *>(self->keyboard_), reinterpret_cast<void (**)(void)>(&klistener),
                                         self);
    }
  }

  static void
  __on_seat_name(void *, gl::wl_seat *, const char *) noexcept
  {
  }

  // pointer callbacks
  static void
  __on_p_enter(void *, gl::wl_pointer *, u32, gl::wl_surface *, i32, i32) noexcept
  {
  }

  static void
  __on_p_leave(void *, gl::wl_pointer *, u32, gl::wl_surface *) noexcept
  {
  }

  static void
  __on_p_motion(void *data, gl::wl_pointer *, u32 time, i32 sx_fixed, i32 sy_fixed) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( !self->on_motion ) return;
    input_event ev{};
    ev.time.tv_sec = static_cast<long>(time / 1000);
    ev.time.tv_usec = static_cast<long>((time % 1000) * 1000);
    ev.type = ev_rel;
    ev.code = rel_x;
    ev.value = sx_fixed >> 8;
    self->on_motion(self->user, ev);
    ev.code = rel_y;
    ev.value = sy_fixed >> 8;
    self->on_motion(self->user, ev);
  }

  static void
  __on_p_button(void *data, gl::wl_pointer *, u32, u32 time, u32 button, u32 state) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( !self->on_button ) return;
    input_event ev{};
    ev.time.tv_sec = static_cast<long>(time / 1000);
    ev.time.tv_usec = static_cast<long>((time % 1000) * 1000);
    ev.type = ev_key;
    ev.code = static_cast<u16>(button);      // Wayland button codes match BTN_*
    ev.value = (state == wl_pointer_button_state_pressed) ? 1 : 0;
    self->on_button(self->user, ev);
  }

  static void
  __on_p_axis(void *data, gl::wl_pointer *, u32 time, u32 axis, i32 value_fixed) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( !self->on_axis ) return;
    input_event ev{};
    ev.time.tv_sec = static_cast<long>(time / 1000);
    ev.time.tv_usec = static_cast<long>((time % 1000) * 1000);
    ev.type = ev_rel;
    ev.code = (axis == 0) ? rel_wheel : rel_hwheel;
    ev.value = value_fixed >> 8;
    self->on_axis(self->user, ev);
  }

  static void
  __on_p_frame(void *, gl::wl_pointer *) noexcept
  {
  }

  static void
  __on_p_axis_source(void *, gl::wl_pointer *, u32) noexcept
  {
  }

  static void
  __on_p_axis_stop(void *, gl::wl_pointer *, u32, u32) noexcept
  {
  }

  static void
  __on_p_axis_discrete(void *, gl::wl_pointer *, u32, i32) noexcept
  {
  }

  static void
  __on_p_axis_v120(void *, gl::wl_pointer *, u32, i32) noexcept
  {
  }

  static void
  __on_p_axis_rel_dir(void *, gl::wl_pointer *, u32, u32) noexcept
  {
  }

  // keyboard callbacks
  static void
  __on_k_keymap(void *data, gl::wl_keyboard *, u32 format, i32 fd, u32 size) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( !self->__xkb_available || fd < 0 || size == 0 || format != xkb::XKB_KEYMAP_FORMAT_TEXT_V1 ) {
      if ( fd >= 0 ) posix::close(fd);
      return;
    }
    char *km = reinterpret_cast<char *>(micron::mmap(nullptr, size, prot_read, map_private, fd, 0));
    if ( mmap_failed(km) ) {
      posix::close(fd);
      return;
    }
    try {
      self->__keymap.load(km);
    } catch ( const except::__base_exception & ) {
    }
    micron::munmap(reinterpret_cast<addr_t *>(km), size);
    posix::close(fd);
  }

  static void
  __on_k_enter(void *, gl::wl_keyboard *, u32, gl::wl_surface *, void *) noexcept
  {
  }

  static void
  __on_k_leave(void *, gl::wl_keyboard *, u32, gl::wl_surface *) noexcept
  {
  }

  static void
  __on_k_key(void *data, gl::wl_keyboard *, u32, u32 time, u32 key, u32 state) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    const bool pressed = (state == wl_keyboard_key_state_pressed);

    if ( self->on_key ) {
      input_event ev{};
      ev.time.tv_sec = static_cast<long>(time / 1000);
      ev.time.tv_usec = static_cast<long>((time % 1000) * 1000);
      ev.type = ev_key;
      ev.code = static_cast<u16>(key);
      ev.value = pressed ? 1 : 0;
      self->on_key(self->user, ev);
    }

    if ( self->__keymap.valid() ) {
      self->__keymap.update_key(key, pressed);
      if ( self->on_keysym ) {
        xkb::keysym_t sym = self->__keymap.translate_key(key);
        u32 utf32 = self->__keymap.translate_utf32(key);
        self->on_keysym(self->user, key, sym, utf32, pressed);
      }
    }
  }

  static void
  __on_k_modifiers(void *data, gl::wl_keyboard *, u32, u32 depressed, u32 latched, u32 locked, u32 group) noexcept
  {
    auto *self = static_cast<wayland_source *>(data);
    if ( self->__keymap.valid() ) self->__keymap.update_mods(depressed, latched, locked, group);
  }

  static void
  __on_k_repeat_info(void *, gl::wl_keyboard *, i32, i32) noexcept
  {
  }

  gl::wayland_display_t *__dpy = nullptr;
  gl::wayland_lib_t *__lib = nullptr;
  gl::wl_seat *__seat = nullptr;
  gl::wl_pointer *pointer_ = nullptr;
  gl::wl_keyboard *keyboard_ = nullptr;
  u32 __seat_version = 0;
  bool __xkb_available = false;
  xkb::keymap_state __keymap;

public:
  ~wayland_source() { release(); }

  wayland_source() = default;
  wayland_source(const wayland_source &) = delete;
  wayland_source &operator=(const wayland_source &) = delete;

  // NOTE: in terminal sessions that are run by a different user than host, this will fail, without reconfig
  void
  bind(gl::display &dpy)
  {
    if ( dpy.backend() != gl::backend_tag_t::wayland_egl ) {
      throw except::logic_error("wayland_source: gl::display is not Wayland");
    }
    __dpy = dpy.as_wayland();
    if ( !__dpy || !__dpy->lib ) throw except::logic_error("wayland_source: wayland_display_t missing");
    __lib = __dpy->lib;

    static gl::wl_registry_listener_t reg_listener{ &__on_global, &__on_global_remove };
    __lib->wl_proxy_add_listener(reinterpret_cast<gl::wl_proxy *>(__dpy->registry), reinterpret_cast<void (**)(void)>(&reg_listener), this);
    __lib->wl_display_roundtrip(__dpy->display);
    __lib->wl_display_roundtrip(__dpy->display);

    if ( !__seat ) throw except::library_error("wayland_source: wl_seat missing — compositor advertises no input");

    try {
      (void)xkb::xkb_lib();
      __xkb_available = true;
    } catch ( const except::__base_exception & ) {
      __xkb_available = false;
    }
  }

  // raw evdev key event
  void (*on_key)(void *user, const input_event &ev) = nullptr;
  // pointer button event
  void (*on_button)(void *user, const input_event &ev) = nullptr;
  // pointer motion
  void (*on_motion)(void *user, const input_event &ev) = nullptr;
  // scroll wheel
  void (*on_axis)(void *user, const input_event &ev) = nullptr;
  // xkbcommon keysym
  void (*on_keysym)(void *user, u32 evdev_code, xkb::keysym_t keysym, u32 utf32, bool pressed) = nullptr;
  void *user = nullptr;

  bool
  xkb_available() const noexcept
  {
    return __xkb_available && __keymap.valid();
  }

  void
  poll() noexcept
  {
    if ( __lib && __dpy ) __lib->wl_display_dispatch_pending(__dpy->display);
  }

  void
  release() noexcept
  {
    if ( !__lib ) return;
    if ( keyboard_ ) {
      __lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(keyboard_), wl_seat_ops::wl_keyboard_release, nullptr, __seat_version,
                                    1 /*WL_MARSHAL_FLAG_DESTROY*/);
      keyboard_ = nullptr;
    }
    if ( pointer_ ) {
      __lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(pointer_), wl_seat_ops::wl_pointer_release, nullptr, __seat_version,
                                    1);
      pointer_ = nullptr;
    }
    if ( __seat ) {
      __lib->wl_proxy_marshal_flags(reinterpret_cast<gl::wl_proxy *>(__seat), wl_seat_ops::wl_seat_release, nullptr, __seat_version, 1);
      __seat = nullptr;
    }
  }

  gl::wl_seat *
  seat() const noexcept
  {
    return __seat;
  }
};

};      // namespace uxin
};      // namespace micron
