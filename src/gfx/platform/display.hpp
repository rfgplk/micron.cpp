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

// Process-singleton lib accessors for the X11 / Wayland clients. Each is
// constructed on first call and throws library_error if the lib can't be
// loaded — subsequent calls retry per the C++ standard's "static local init
// throws → not initialized, retry next time" rule.
//
// API-specific lib singletons (libGL.so.1 via glx_lib(), libEGL.so.1 via
// egl_lib(), libwayland-egl.so.1 via wayland_egl_lib(), libvulkan.so.1 via
// vk_lib()) live in their respective gfx/gl/ and gfx/vk/ headers.

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

// The user-facing display handle. Hides the X11 vs Wayland choice behind
// a runtime tag; only the chosen backend's underlying state is live, the
// other is left default-constructed (its destructor is a no-op when no
// resources are held).
//
// `raw_fd()` is the integration point with io/uxin/ — a separate event
// loop can register this fd in its own epoll instance to drive input
// alongside the gfx module's rendering.
class display
{
private:
  backend_tag_t __backend = backend_tag_t::none;
  x11_display_t __x11;
  wayland_display_t __wl;
  // Static, statically-allocated buffers for the per-backend failure reason.
  // Populated by __try_open so display::__throw_open_failed can include the
  // actual cause (lib not loaded, server unreachable, missing entry points)
  // instead of the misleading "DISPLAY/WAYLAND_DISPLAY both absent".
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
    try {
      if ( tag == backend_tag_t::x11 ) {
        __x11.open(x11_lib());
      } else {      // wayland
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
  }

  [[noreturn]] static void
  __throw_open_failed()
  {
    // Compose a final message that captures the actual per-backend cause —
    // typically "soname not present in host" when the binary didn't link
    // -l:libX11.so.6 / -l:libwayland-client.so.0, or "XOpenDisplay returned
    // null" when the server is unreachable. The old "absent or unbound"
    // wording made it look like an env-var parsing bug; the env reads
    // succeed, the lib lookup is what fails.
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
  ~display() = default;      // member destructors handle cleanup

  display() : display(select_backend()) { }

  // Try `requested` first, then fall back to the other backend if its env
  // var is set. This is how we tolerate environments where both DISPLAY
  // and WAYLAND_DISPLAY leak from the shell but only one is reachable —
  // we open whichever responds and let the caller query `backend()` after.
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

  // Backend escape hatches. Return nullptr if this display isn't of that
  // backend — call sites should check `backend()` first and only use the
  // matching accessor.
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
