//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

// Wayland-client opaque type forwards plus the few PFN typedefs we need.
// Layouts are intentionally hidden: libwayland-client owns the actual
// structures, and we only ever hold pointers and call wl_proxy_* on them.
// The hand-written xdg-shell wire-protocol marshalling lives in
// gfx/platform/wayland_xdg_shell.hpp; it uses the wl_proxy primitives below.

namespace micron
{
namespace gfx
{
namespace platform
{

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_surface;
struct wl_proxy;
struct wl_callback;
struct wl_event_queue;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_touch;
struct wl_output;
struct wl_shm;
struct wl_region;
struct wl_egl_window;

// xdg-shell — also opaque (we treat its proxies as wl_proxy under the hood).
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_popup;
struct xdg_positioner;

// Full definition of the wl_interface / wl_message structs from
// /usr/include/wayland-util.h. We replicate the layout 1:1 so that the
// pointer we hand to wl_proxy_marshal_flags is laid out exactly as
// libwayland expects.
struct wl_interface;

struct wl_message {
  const char *name;
  const char *signature;
  const wl_interface **types;
};

struct wl_interface {
  const char *name;
  int version;
  int method_count;
  const wl_message *methods;
  int event_count;
  const wl_message *events;
};

// A wl_argument is the wire-format payload slot used by wl_proxy_marshal*.
// Layout matches /usr/include/wayland-util.h.
union wl_argument {
  i32 i;
  u32 u;
  i32 f;      // wl_fixed_t — 24.8 signed
  const char *s;
  wl_proxy *o;
  u32 n;        // new_id
  void *a;      // wl_array*
  i32 h;        // file descriptor
};

// PFN typedefs for the wl_* entry points we resolve via micron::dso.
using PFN_wl_display_connect = wl_display *(*)(const char *name);
using PFN_wl_display_connect_to_fd = wl_display *(*)(int fd);
using PFN_wl_display_disconnect = void (*)(wl_display *display);
using PFN_wl_display_get_fd = int (*)(wl_display *display);
using PFN_wl_display_dispatch = int (*)(wl_display *display);
using PFN_wl_display_dispatch_pending = int (*)(wl_display *display);
using PFN_wl_display_roundtrip = int (*)(wl_display *display);
using PFN_wl_display_flush = int (*)(wl_display *display);
using PFN_wl_display_prepare_read = int (*)(wl_display *display);
using PFN_wl_display_read_events = int (*)(wl_display *display);
using PFN_wl_display_get_registry = wl_registry *(*)(wl_display * display);

using PFN_wl_proxy_marshal_flags = wl_proxy *(*)(wl_proxy * proxy, u32 opcode, const wl_interface *interface, u32 version, u32 flags, ...);
using PFN_wl_proxy_marshal_array_flags
    = wl_proxy *(*)(wl_proxy * proxy, u32 opcode, const wl_interface *interface, u32 version, u32 flags, wl_argument *args);
using PFN_wl_proxy_add_listener = int (*)(wl_proxy *proxy, void (**implementation)(void), void *data);
using PFN_wl_proxy_destroy = void (*)(wl_proxy *proxy);
using PFN_wl_proxy_get_user_data = void *(*)(wl_proxy * proxy);
using PFN_wl_proxy_set_user_data = void (*)(wl_proxy *proxy, void *user_data);
using PFN_wl_proxy_get_version = u32 (*)(wl_proxy *proxy);
using PFN_wl_proxy_get_id = u32 (*)(wl_proxy *proxy);

// libwayland-egl.so.1 — separate library from libwayland-client.
using PFN_wl_egl_window_create = wl_egl_window *(*)(wl_surface * surface, int width, int height);
using PFN_wl_egl_window_destroy = void (*)(wl_egl_window *egl_window);
using PFN_wl_egl_window_resize = void (*)(wl_egl_window *egl_window, int width, int height, int dx, int dy);

// wl_proxy_marshal_flags flags.
inline constexpr u32 WL_MARSHAL_FLAG_DESTROY = 1u << 0;

};      // namespace platform
};      // namespace gfx
};      // namespace micron
