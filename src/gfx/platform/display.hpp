//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "platform.hpp"
#include "wayland.hpp"
#include "x11.hpp"

namespace micron
{
namespace gfx
{
namespace platform
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lib accessors for the X11 / Wayland clients
//
// (libGL.so.1 via glx_lib(), libEGL.so.1 via egl_lib(), libwayland-egl.so.1 via wayland_egl_lib(), libvulkan.so.1 viavk_lib()) live in
// their respective gfx/gl/ and gfx/vk/ headers

inline x11_lib_t &
x11_lib() noexcept(false)
{
  static x11_lib_t lib;
  return lib;
}

inline wayland_lib_t &
wayland_lib() noexcept(false)
{
  static wayland_lib_t lib;
  return lib;
}

class display
{
private:
  backend_tag_t __backend = backend_tag_t::none;
  x11_display_t __x11;
  wayland_display_t __wl;

  static inline char __x11_last_err[256] = "";
  static inline char __wl_last_err[256] = "";

  static void
  __copy_msg(char *dst, const char *src) noexcept
  {
    if ( !src ) {
      dst[0] = 0;
      return;
    }
    usize i = 0;
    while ( i + 1 < 256 && src[i] ) {
      dst[i] = src[i];
      ++i;
    }
    dst[i] = 0;
  }

  bool
  __try_open(backend_tag_t tag) noexcept
  {
    if ( tag == backend_tag_t::none ) return false;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      if ( tag == backend_tag_t::x11 ) {
        __x11.open(x11_lib());
      } else {
        __wl.open(wayland_lib());
      }
      __backend = tag;
      return true;
    } catch ( const except::__base_exception &e ) {
      __copy_msg(tag == backend_tag_t::x11 ? __x11_last_err : __wl_last_err, e.what());
      return false;
    } catch ( ... ) {
      __copy_msg(tag == backend_tag_t::x11 ? __x11_last_err : __wl_last_err, "unknown");
      return false;
    }
#else

    if ( tag == backend_tag_t::x11 ) {
      __x11.open(x11_lib());
    } else {
      __wl.open(wayland_lib());
    }
    __backend = tag;
    return true;
#endif
  }

  [[noreturn]] static void
  __throw_open_failed()
  {

    static char buf[768];
    const char *p1 = "gfx::display: no display server reachable. x11: ";
    const char *p2 = "; wayland: ";
    usize n = 0;
    auto write = [&](const char *s) {
      while ( s && *s && n + 1 < sizeof(buf) ) buf[n++] = *s++;
    };
    write(p1);
    write(__x11_last_err[0] ? __x11_last_err : "(not attempted)");
    write(p2);
    write(__wl_last_err[0] ? __wl_last_err : "(not attempted)");
    buf[n] = 0;
    throw except::network_error(buf);
  }

public:
  ~display() = default;

  display() : display(select_backend()) { }

  explicit display(backend_tag_t requested)
  {
    backend_tag_t alt = backend_tag_t::none;
    if ( requested == backend_tag_t::x11 )
      alt = backend_tag_t::wayland;
    else if ( requested == backend_tag_t::wayland )
      alt = backend_tag_t::x11;

    if ( __try_open(requested) ) return;
    if ( __try_open(alt) ) return;
    __throw_open_failed();
  }

  display(const display &) = delete;
  display(display &&) = delete;

  display &operator=(const display &) = delete;
  display &operator=(display &&) = delete;

  backend_tag_t
  backend() const noexcept
  {
    return __backend;
  }

  i32
  raw_fd() const noexcept
  {
    switch ( __backend ) {
    case backend_tag_t::x11:
      return __x11.raw_fd();
    case backend_tag_t::wayland:
      return __wl.raw_fd();
    default:
      return -1;
    }
  }

  x11_display_t *
  as_x11() noexcept
  {
    return __backend == backend_tag_t::x11 ? &__x11 : nullptr;
  }

  const x11_display_t *
  as_x11() const noexcept
  {
    return __backend == backend_tag_t::x11 ? &__x11 : nullptr;
  }

  wayland_display_t *
  as_wayland() noexcept
  {
    return __backend == backend_tag_t::wayland ? &__wl : nullptr;
  }

  const wayland_display_t *
  as_wayland() const noexcept
  {
    return __backend == backend_tag_t::wayland ? &__wl : nullptr;
  }
};

};      // namespace platform
};      // namespace gfx
};      // namespace micron
