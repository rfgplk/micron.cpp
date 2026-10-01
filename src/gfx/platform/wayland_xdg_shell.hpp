//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__bits/__wayland_types.hpp"

namespace micron
{
namespace gfx
{
namespace platform
{

extern const wl_interface __wl_callback_interface;
extern const wl_interface __wl_compositor_interface;
extern const wl_interface __wl_region_interface;
extern const wl_interface __wl_registry_interface;
extern const wl_interface __wl_surface_interface;
extern const wl_interface __xdg_popup_interface;
extern const wl_interface __xdg_positioner_interface;
extern const wl_interface __xdg_surface_interface;
extern const wl_interface __xdg_toplevel_interface;
extern const wl_interface __xdg_wm_base_interface;

inline const wl_message __wl_callback_events[] = {
  { "done", "u", __wl_untyped_arguments },
};
inline constexpr wl_interface __wl_callback_interface = {
  "wl_callback", 1, 0, nullptr, 1, __wl_callback_events,
};

inline const wl_message __wl_region_methods[] = {
  { "destroy", "", nullptr },
  { "add", "iiii", __wl_untyped_arguments },
  { "subtract", "iiii", __wl_untyped_arguments },
};
inline constexpr wl_interface __wl_region_interface = {
  "wl_region", 1, 3, __wl_region_methods, 0, nullptr,
};

inline const wl_interface *__wl_surface_request_types[] = {

  &__wl_callback_interface,
};

inline const wl_interface *__wl_surface_types_buffer[] = { nullptr, nullptr, nullptr };
inline const wl_interface *__wl_surface_types_callback[] = { &__wl_callback_interface };
inline const wl_interface *__wl_surface_types_region[] = { &__wl_region_interface };

inline const wl_message __wl_surface_methods[] = {
  { "destroy", "", nullptr },
  { "attach", "?oii", __wl_surface_types_buffer },
  { "damage", "iiii", __wl_untyped_arguments },
  { "frame", "n", __wl_surface_types_callback },
  { "set_opaque_region", "?o", __wl_surface_types_region },
  { "set_input_region", "?o", __wl_surface_types_region },
  { "commit", "", nullptr },
  { "set_buffer_transform", "2i", __wl_untyped_arguments },
  { "set_buffer_scale", "3i", __wl_untyped_arguments },
  { "damage_buffer", "4iiii", __wl_untyped_arguments },
  { "offset", "5ii", __wl_untyped_arguments },
};

inline const wl_message __wl_surface_events[] = {
  { "enter", "o", __wl_untyped_arguments },
  { "leave", "o", __wl_untyped_arguments },
  { "preferred_buffer_scale", "6i", __wl_untyped_arguments },
  { "preferred_buffer_transform", "6u", __wl_untyped_arguments },
};

inline constexpr wl_interface __wl_surface_interface = {
  "wl_surface",
  6,
  sizeof(__wl_surface_methods) / sizeof(__wl_surface_methods[0]),
  __wl_surface_methods,
  sizeof(__wl_surface_events) / sizeof(__wl_surface_events[0]),
  __wl_surface_events,
};

inline const wl_interface *__wl_compositor_types_surface[] = { &__wl_surface_interface };
inline const wl_interface *__wl_compositor_types_region[] = { &__wl_region_interface };

inline const wl_message __wl_compositor_methods[] = {
  { "create_surface", "n", __wl_compositor_types_surface },
  { "create_region", "n", __wl_compositor_types_region },
};
inline constexpr wl_interface __wl_compositor_interface = {
  "wl_compositor", 6, 2, __wl_compositor_methods, 0, nullptr,
};

inline const wl_message __wl_registry_methods[] = {
  { "bind", "usun", __wl_untyped_arguments },
};
inline const wl_message __wl_registry_events[] = {
  { "global", "usu", __wl_untyped_arguments },
  { "global_remove", "u", __wl_untyped_arguments },
};
inline constexpr wl_interface __wl_registry_interface = {
  "wl_registry", 1, 1, __wl_registry_methods, 2, __wl_registry_events,
};

inline const wl_message __xdg_positioner_methods[] = {
  { "destroy", "", nullptr },
  { "set_size", "ii", __wl_untyped_arguments },
  { "set_anchor_rect", "iiii", __wl_untyped_arguments },
  { "set_anchor", "u", __wl_untyped_arguments },
  { "set_gravity", "u", __wl_untyped_arguments },
  { "set_constraint_adjustment", "u", __wl_untyped_arguments },
  { "set_offset", "ii", __wl_untyped_arguments },
  { "set_reactive", "3", __wl_untyped_arguments },
  { "set_parent_size", "3ii", __wl_untyped_arguments },
  { "set_parent_configure", "3u", __wl_untyped_arguments },
};
inline constexpr wl_interface __xdg_positioner_interface = {
  "xdg_positioner", 6, 10, __xdg_positioner_methods, 0, nullptr,
};

inline const wl_interface *__xdg_popup_types_grab[] = { nullptr /*wl_seat*/, nullptr };
inline const wl_interface *__xdg_popup_types_reposition[] = { &__xdg_positioner_interface, nullptr };
inline const wl_message __xdg_popup_methods[] = {
  { "destroy", "", nullptr },
  { "grab", "ou", __xdg_popup_types_grab },
  { "reposition", "3ou", __xdg_popup_types_reposition },
};
inline const wl_message __xdg_popup_events[] = {
  { "configure", "iiii", __wl_untyped_arguments },
  { "popup_done", "", nullptr },
  { "repositioned", "3u", __wl_untyped_arguments },
};
inline constexpr wl_interface __xdg_popup_interface = {
  "xdg_popup", 6, 3, __xdg_popup_methods, 3, __xdg_popup_events,
};

inline const wl_interface *__xdg_toplevel_types_set_parent[] = { &__xdg_toplevel_interface };
inline const wl_interface *__xdg_toplevel_types_show_window_menu[] = { nullptr /*wl_seat*/, nullptr, nullptr, nullptr };
inline const wl_interface *__xdg_toplevel_types_move[] = { nullptr /*wl_seat*/, nullptr };
inline const wl_interface *__xdg_toplevel_types_resize[] = { nullptr /*wl_seat*/, nullptr, nullptr };
inline const wl_interface *__xdg_toplevel_types_set_fullscreen[] = { nullptr /*wl_output*/ };

inline const wl_message __xdg_toplevel_methods[] = {
  { "destroy", "", nullptr },
  { "set_parent", "?o", __xdg_toplevel_types_set_parent },
  { "set_title", "s", __wl_untyped_arguments },
  { "set_app_id", "s", __wl_untyped_arguments },
  { "show_window_menu", "ouii", __xdg_toplevel_types_show_window_menu },
  { "move", "ou", __xdg_toplevel_types_move },
  { "resize", "ouu", __xdg_toplevel_types_resize },
  { "set_max_size", "ii", __wl_untyped_arguments },
  { "set_min_size", "ii", __wl_untyped_arguments },
  { "set_maximized", "", nullptr },
  { "unset_maximized", "", nullptr },
  { "set_fullscreen", "?o", __xdg_toplevel_types_set_fullscreen },
  { "unset_fullscreen", "", nullptr },
  { "set_minimized", "", nullptr },
};

inline const wl_message __xdg_toplevel_events[] = {
  { "configure", "iia", __wl_untyped_arguments },
  { "close", "", nullptr },
  { "configure_bounds", "4ii", __wl_untyped_arguments },
  { "wm_capabilities", "5a", __wl_untyped_arguments },
};

inline constexpr wl_interface __xdg_toplevel_interface = {
  "xdg_toplevel", 6, 14, __xdg_toplevel_methods, 4, __xdg_toplevel_events,
};

inline const wl_interface *__xdg_surface_types_get_toplevel[] = { &__xdg_toplevel_interface };
inline const wl_interface *__xdg_surface_types_get_popup[]
    = { &__xdg_popup_interface, &__xdg_surface_interface, &__xdg_positioner_interface };

inline const wl_message __xdg_surface_methods[] = {
  { "destroy", "", nullptr },
  { "get_toplevel", "n", __xdg_surface_types_get_toplevel },
  { "get_popup", "n?oo", __xdg_surface_types_get_popup },
  { "set_window_geometry", "iiii", __wl_untyped_arguments },
  { "ack_configure", "u", __wl_untyped_arguments },
};
inline const wl_message __xdg_surface_events[] = {
  { "configure", "u", __wl_untyped_arguments },
};
inline constexpr wl_interface __xdg_surface_interface = {
  "xdg_surface", 6, 5, __xdg_surface_methods, 1, __xdg_surface_events,
};

inline const wl_interface *__xdg_wm_base_types_create_positioner[] = { &__xdg_positioner_interface };
inline const wl_interface *__xdg_wm_base_types_get_xdg_surface[] = { &__xdg_surface_interface, &__wl_surface_interface };

inline const wl_message __xdg_wm_base_methods[] = {
  { "destroy", "", nullptr },
  { "create_positioner", "n", __xdg_wm_base_types_create_positioner },
  { "get_xdg_surface", "no", __xdg_wm_base_types_get_xdg_surface },
  { "pong", "u", __wl_untyped_arguments },
};
inline const wl_message __xdg_wm_base_events[] = {
  { "ping", "u", __wl_untyped_arguments },
};
inline constexpr wl_interface __xdg_wm_base_interface = {
  "xdg_wm_base", 6, 4, __xdg_wm_base_methods, 1, __xdg_wm_base_events,
};

namespace xdg_shell_ops
{
inline constexpr u32 xdg_wm_base_destroy = 0;
inline constexpr u32 xdg_wm_base_create_positioner = 1;
inline constexpr u32 xdg_wm_base_get_xdg_surface = 2;
inline constexpr u32 xdg_wm_base_pong = 3;

inline constexpr u32 xdg_surface_destroy = 0;
inline constexpr u32 xdg_surface_get_toplevel = 1;
inline constexpr u32 xdg_surface_get_popup = 2;
inline constexpr u32 xdg_surface_set_window_geometry = 3;
inline constexpr u32 xdg_surface_ack_configure = 4;

inline constexpr u32 xdg_toplevel_destroy = 0;
inline constexpr u32 xdg_toplevel_set_title = 2;
inline constexpr u32 xdg_toplevel_set_app_id = 3;

inline constexpr u32 wl_compositor_create_surface = 0;
inline constexpr u32 wl_surface_destroy = 0;
inline constexpr u32 wl_surface_commit = 6;
inline constexpr u32 wl_registry_bind = 0;
};      // namespace xdg_shell_ops

struct wl_registry_listener_t {
  void (*global)(void *data, wl_registry *registry, u32 name, const char *interface, u32 version);
  void (*global_remove)(void *data, wl_registry *registry, u32 name);
};

struct xdg_wm_base_listener_t {
  void (*ping)(void *data, xdg_wm_base *base, u32 serial);
};

struct xdg_surface_listener_t {
  void (*configure)(void *data, xdg_surface *surface, u32 serial);
};

struct xdg_toplevel_listener_t {
  void (*configure)(void *data, xdg_toplevel *toplevel, i32 width, i32 height, void *states_array);
  void (*close)(void *data, xdg_toplevel *toplevel);
  void (*configure_bounds)(void *data, xdg_toplevel *toplevel, i32 width, i32 height);
  void (*wm_capabilities)(void *data, xdg_toplevel *toplevel, void *caps_array);
};

};      // namespace platform
};      // namespace gfx
};      // namespace micron
