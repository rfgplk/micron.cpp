//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/io/sys.hpp"
#include "../../linux/sys/fcntl.hpp"
#include "../../memory/cstring.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../../types.hpp"

// Runtime backend selection for the gfx module — used by both the GL and VK
// API layers built on top.
//
// micron has no env-vector primitive yet, so we read /proc/self/environ
// directly (a NUL-separated KEY=VALUE blob). Cached on first access; the
// underlying mapping stays for the lifetime of the process. A future
// linux/env.hpp can absorb this — for now it lives here because the gfx
// platform layer is its only caller.

namespace micron
{
namespace gfx
{
namespace platform
{

enum class backend_tag_t : u8 {
  none = 0,
  x11 = 1,
  wayland = 2,
};

namespace __env
{

inline char *__buf = nullptr;
inline usize __sz = 0;
inline bool __loaded = false;

inline void
__load() noexcept
{
  if ( __loaded ) return;
  __loaded = true;

  // /proc/self/environ reports size 0 in stat; read in chunks until EOF.
  i32 fd = posix::open("/proc/self/environ", posix::o_rdonly);
  if ( fd < 0 ) return;
  constexpr usize cap = 65536;
  char *buf = reinterpret_cast<char *>(micron::mmap(nullptr, cap, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(buf) ) {
    posix::close(fd);
    return;
  }
  usize total = 0;
  while ( total + 1 < cap ) {
    max_t n = posix::read(fd, buf + total, cap - 1 - total);
    if ( n <= 0 ) break;
    total += static_cast<usize>(n);
  }
  buf[total] = 0;
  posix::close(fd);
  __buf = buf;
  __sz = total;
}

// Look up `key`. Returns nullptr if absent; otherwise a pointer to the value
// substring inside __buf (NUL-terminated by the kernel's environ layout).
inline const char *
__get(const char *key) noexcept
{
  __load();
  if ( !__buf || !key ) return nullptr;
  const usize klen = micron::strlen(key);
  usize i = 0;
  while ( i < __sz ) {
    // Match "KEY=" — entry must be klen+1 bytes inside the blob and end with '='.
    if ( i + klen < __sz && __buf[i + klen] == '=' && micron::strncmp(__buf + i, key, klen) == 0 ) {
      return __buf + i + klen + 1;
    }
    // Skip past the rest of this NUL-separated entry.
    while ( i < __sz && __buf[i] != 0 ) ++i;
    ++i;
  }
  return nullptr;
}

inline bool
__nonempty(const char *s) noexcept
{
  return s && s[0] != 0;
}

};      // namespace __env

inline const char *
env(const char *key) noexcept
{
  return __env::__get(key);
}

// Pick a backend by examining standard XDG / X / Wayland environment vars.
// Returns `none` if neither X11 nor Wayland are reachable.
inline backend_tag_t
select_backend() noexcept
{
  const char *session = env("XDG_SESSION_TYPE");
  const char *wl = env("WAYLAND_DISPLAY");
  const char *xdpy = env("DISPLAY");

  if ( session && micron::strcmp(session, "wayland") == 0 && __env::__nonempty(wl) ) return backend_tag_t::wayland;
  if ( session && micron::strcmp(session, "x11") == 0 && __env::__nonempty(xdpy) ) return backend_tag_t::x11;
  // No session hint — fall back to whatever transport is present.
  if ( __env::__nonempty(wl) ) return backend_tag_t::wayland;
  if ( __env::__nonempty(xdpy) ) return backend_tag_t::x11;
  return backend_tag_t::none;
}

inline const char *
backend_name(backend_tag_t t) noexcept
{
  switch ( t ) {
  case backend_tag_t::x11:
    return "x11";
  case backend_tag_t::wayland:
    return "wayland";
  case backend_tag_t::none:
  default:
    return "none";
  }
}

};      // namespace platform
};      // namespace gfx
};      // namespace micron
