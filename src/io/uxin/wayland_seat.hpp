//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../gfx/platform/__bits/__wayland_types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wl_seat / wl_pointer / wl_keyboard / wl_touch
// opcodes from wayland.xml
// (https://chromium.googlesource.com/external/wayland/wayland/+/refs/heads/master/protocol/wayland.xml)

namespace micron
{
namespace uxin
{

extern const gfx::platform::wl_interface __wl_seat_interface;
extern const gfx::platform::wl_interface __wl_pointer_interface;
extern const gfx::platform::wl_interface __wl_keyboard_interface;
extern const gfx::platform::wl_interface __wl_touch_interface;

inline const gfx::platform::wl_interface *__wl_pointer_types_set_cursor[] = {
  nullptr,                     // serial (uint)
  nullptr /*wl_surface*/,      // we don't pass a wl_surface here from uxin
  nullptr,                     // hotspot_x (int)
  nullptr,                     // hotspot_y (int)
};
inline const gfx::platform::wl_message __wl_pointer_methods[] = {
  { "set_cursor", "u?oii", __wl_pointer_types_set_cursor },
  { "release", "3", nullptr },
};
inline const gfx::platform::wl_message __wl_pointer_events[] = {
  { "enter", "uoff", nullptr },
  { "leave", "uo", nullptr },
  { "motion", "uff", nullptr },
  { "button", "uuuu", nullptr },
  { "axis", "uuf", nullptr },
  { "frame", "5", nullptr },
  { "axis_source", "5u", nullptr },
  { "axis_stop", "5uu", nullptr },
  { "axis_discrete", "5ui", nullptr },
  { "axis_value120", "8ui", nullptr },
  { "axis_relative_direction", "9uu", nullptr },
};
inline constexpr gfx::platform::wl_interface __wl_pointer_interface = {
  "wl_pointer",
  9,
  sizeof(__wl_pointer_methods) / sizeof(__wl_pointer_methods[0]),
  __wl_pointer_methods,
  sizeof(__wl_pointer_events) / sizeof(__wl_pointer_events[0]),
  __wl_pointer_events,
};

inline const gfx::platform::wl_message __wl_keyboard_methods[] = {
  { "release", "3", nullptr },
};
inline const gfx::platform::wl_message __wl_keyboard_events[] = {
  { "keymap", "uhu", nullptr }, { "enter", "uoa", nullptr },       { "leave", "uo", nullptr },
  { "key", "uuuu", nullptr },   { "modifiers", "uuuuu", nullptr }, { "repeat_info", "4ii", nullptr },
};
inline constexpr gfx::platform::wl_interface __wl_keyboard_interface = {
  "wl_keyboard",
  9,
  sizeof(__wl_keyboard_methods) / sizeof(__wl_keyboard_methods[0]),
  __wl_keyboard_methods,
  sizeof(__wl_keyboard_events) / sizeof(__wl_keyboard_events[0]),
  __wl_keyboard_events,
};

// TODO: expand
inline const gfx::platform::wl_message __wl_touch_methods[] = {
  { "release", "3", nullptr },
};
inline const gfx::platform::wl_message __wl_touch_events[] = {
  { "down", "uuoiff", nullptr }, { "up", "uui", nullptr },     { "motion", "uiff", nullptr },     { "frame", "", nullptr },
  { "cancel", "", nullptr },     { "shape", "6iff", nullptr }, { "orientation", "6if", nullptr },
};
inline constexpr gfx::platform::wl_interface __wl_touch_interface = {
  "wl_touch", 9, 1, __wl_touch_methods, 7, __wl_touch_events,
};

inline const gfx::platform::wl_interface *__wl_seat_types_get_pointer[] = { &__wl_pointer_interface };
inline const gfx::platform::wl_interface *__wl_seat_types_get_keyboard[] = { &__wl_keyboard_interface };
inline const gfx::platform::wl_interface *__wl_seat_types_get_touch[] = { &__wl_touch_interface };

inline const gfx::platform::wl_message __wl_seat_methods[] = {
  { "get_pointer", "n", __wl_seat_types_get_pointer },
  { "get_keyboard", "n", __wl_seat_types_get_keyboard },
  { "get_touch", "n", __wl_seat_types_get_touch },
  { "release", "5", nullptr },
};
inline const gfx::platform::wl_message __wl_seat_events[] = {
  { "capabilities", "u", nullptr },
  { "name", "2s", nullptr },
};
inline constexpr gfx::platform::wl_interface __wl_seat_interface = {
  "wl_seat",
  9,
  sizeof(__wl_seat_methods) / sizeof(__wl_seat_methods[0]),
  __wl_seat_methods,
  sizeof(__wl_seat_events) / sizeof(__wl_seat_events[0]),
  __wl_seat_events,
};

namespace wl_seat_ops
{
inline constexpr u32 wl_seat_get_pointer = 0;
inline constexpr u32 wl_seat_get_keyboard = 1;
inline constexpr u32 wl_seat_get_touch = 2;
inline constexpr u32 wl_seat_release = 3;

inline constexpr u32 wl_pointer_set_cursor = 0;
inline constexpr u32 wl_pointer_release = 1;

inline constexpr u32 wl_keyboard_release = 0;
};      // namespace wl_seat_ops

inline constexpr u32 wl_seat_capability_pointer = 1;
inline constexpr u32 wl_seat_capability_keyboard = 2;
inline constexpr u32 wl_seat_capability_touch = 4;

inline constexpr u32 wl_pointer_button_state_released = 0;
inline constexpr u32 wl_pointer_button_state_pressed = 1;
inline constexpr u32 wl_keyboard_key_state_released = 0;
inline constexpr u32 wl_keyboard_key_state_pressed = 1;

// callbacks
struct wl_seat_listener_t {
  void (*capabilities)(void *data, gfx::platform::wl_seat *seat, u32 caps);
  void (*name)(void *data, gfx::platform::wl_seat *seat, const char *name);
};

struct wl_pointer_listener_t {
  void (*enter)(void *data, gfx::platform::wl_pointer *p, u32 serial, gfx::platform::wl_surface *s, i32 surface_x_fixed,
                i32 surface_y_fixed);
  void (*leave)(void *data, gfx::platform::wl_pointer *p, u32 serial, gfx::platform::wl_surface *s);
  void (*motion)(void *data, gfx::platform::wl_pointer *p, u32 time, i32 surface_x_fixed, i32 surface_y_fixed);
  void (*button)(void *data, gfx::platform::wl_pointer *p, u32 serial, u32 time, u32 button, u32 state);
  void (*axis)(void *data, gfx::platform::wl_pointer *p, u32 time, u32 axis, i32 value_fixed);
  void (*frame)(void *data, gfx::platform::wl_pointer *p);
  void (*axis_source)(void *data, gfx::platform::wl_pointer *p, u32 axis_source);
  void (*axis_stop)(void *data, gfx::platform::wl_pointer *p, u32 time, u32 axis);
  void (*axis_discrete)(void *data, gfx::platform::wl_pointer *p, u32 axis, i32 discrete);
  void (*axis_value120)(void *data, gfx::platform::wl_pointer *p, u32 axis, i32 value120);
  void (*axis_relative_direction)(void *data, gfx::platform::wl_pointer *p, u32 axis, u32 direction);
};

struct wl_keyboard_listener_t {
  void (*keymap)(void *data, gfx::platform::wl_keyboard *k, u32 format, i32 fd, u32 size);
  void (*enter)(void *data, gfx::platform::wl_keyboard *k, u32 serial, gfx::platform::wl_surface *s, void *keys_array);
  void (*leave)(void *data, gfx::platform::wl_keyboard *k, u32 serial, gfx::platform::wl_surface *s);
  void (*key)(void *data, gfx::platform::wl_keyboard *k, u32 serial, u32 time, u32 key, u32 state);
  void (*modifiers)(void *data, gfx::platform::wl_keyboard *k, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group);
  void (*repeat_info)(void *data, gfx::platform::wl_keyboard *k, i32 rate, i32 delay);
};

};      // namespace uxin
};      // namespace micron
