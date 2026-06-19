//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/sys/time.hpp"

#include "../../type_traits.hpp"

#include "bits.hpp"
#include "devices.hpp"

namespace micron
{
namespace uxin
{

// generic absolute-axis sensor (joysticks, gamepads): EV_ABS
inline auto
prepare_generic_mouse_sensor_abs(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                                 void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                                 void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(9)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_z;
  __input.button_mask[3] = abs_rx;
  __input.button_mask[4] = abs_ry;
  __input.button_mask[5] = abs_rz;
  __input.button_mask[6] = abs_throttle;
  __input.button_mask[7] = abs_rudder;
  __input.button_mask[8] = abs_wheel;

  return __input;
};

inline auto
prepare_generic_mouse_sensor_abs(void) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(9)), nullptr, nullptr, nullptr };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_z;
  __input.button_mask[3] = abs_rx;
  __input.button_mask[4] = abs_ry;
  __input.button_mask[5] = abs_rz;
  __input.button_mask[6] = abs_throttle;
  __input.button_mask[7] = abs_rudder;
  __input.button_mask[8] = abs_wheel;

  return __input;
};

inline auto
prepare_generic_mouse_sensor(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                             void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                             void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_rel, slice<button_t>(static_cast<size_t>(9)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = rel_x;
  __input.button_mask[1] = rel_y;
  __input.button_mask[2] = rel_z;
  __input.button_mask[3] = rel_rx;
  __input.button_mask[4] = rel_ry;
  __input.button_mask[5] = rel_rz;
  __input.button_mask[6] = rel_hwheel;
  __input.button_mask[7] = rel_dial;
  __input.button_mask[8] = rel_wheel;

  return __input;
};

inline auto
prepare_generic_mouse_sensor(void) -> input_packet_t
{
  input_packet_t __input{ ev_rel, slice<button_t>(static_cast<size_t>(9)), nullptr, nullptr, nullptr };

  __input.button_mask[0] = rel_x;
  __input.button_mask[1] = rel_y;
  __input.button_mask[2] = rel_z;
  __input.button_mask[3] = rel_rx;
  __input.button_mask[4] = rel_ry;
  __input.button_mask[5] = rel_rz;
  __input.button_mask[6] = rel_hwheel;
  __input.button_mask[7] = rel_dial;
  __input.button_mask[8] = rel_wheel;
  return __input;
};

inline auto
prepare_generic_mouse(void (*fn_cb)(const micron::timeval_t &, u16, i32), void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                      void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_key, slice<button_t>(static_cast<size_t>(8)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = btn_left;
  __input.button_mask[1] = btn_right;
  __input.button_mask[2] = btn_middle;
  __input.button_mask[3] = btn_side;
  __input.button_mask[4] = btn_extra;
  __input.button_mask[5] = btn_forward;
  __input.button_mask[6] = btn_back;
  __input.button_mask[7] = btn_task;

  return __input;
};

// touchpad / touchscreen motion: absolute position + multitouch axes (EV_ABS)
inline auto
prepare_generic_touchpad_sensor(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                                void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                                void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(7)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_pressure;
  __input.button_mask[3] = abs_mt_slot;
  __input.button_mask[4] = abs_mt_position_x;
  __input.button_mask[5] = abs_mt_position_y;
  __input.button_mask[6] = abs_mt_tracking_id;

  return __input;
};

inline auto
prepare_generic_touchpad_sensor(void) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(7)), nullptr, nullptr, nullptr };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_pressure;
  __input.button_mask[3] = abs_mt_slot;
  __input.button_mask[4] = abs_mt_position_x;
  __input.button_mask[5] = abs_mt_position_y;
  __input.button_mask[6] = abs_mt_tracking_id;

  return __input;
};

// touchpad / touchscreen buttons & finger-tap tools (EV_KEY)
inline auto
prepare_generic_touchpad(void (*fn_cb)(const micron::timeval_t &, u16, i32), void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                         void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_key, slice<button_t>(static_cast<size_t>(7)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = btn_left;
  __input.button_mask[1] = btn_right;
  __input.button_mask[2] = btn_middle;
  __input.button_mask[3] = btn_touch;
  __input.button_mask[4] = btn_tool_finger;
  __input.button_mask[5] = btn_tool_doubletap;
  __input.button_mask[6] = btn_tool_tripletap;

  return __input;
};

// graphics tablet / stylus motion: absolute position + pressure + tilt (EV_ABS)
inline auto
prepare_generic_tablet_sensor(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                              void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                              void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(6)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_pressure;
  __input.button_mask[3] = abs_distance;
  __input.button_mask[4] = abs_tilt_x;
  __input.button_mask[5] = abs_tilt_y;

  return __input;
};

inline auto
prepare_generic_tablet_sensor(void) -> input_packet_t
{
  input_packet_t __input{ ev_abs, slice<button_t>(static_cast<size_t>(6)), nullptr, nullptr, nullptr };

  __input.button_mask[0] = abs_x;
  __input.button_mask[1] = abs_y;
  __input.button_mask[2] = abs_pressure;
  __input.button_mask[3] = abs_distance;
  __input.button_mask[4] = abs_tilt_x;
  __input.button_mask[5] = abs_tilt_y;

  return __input;
};

// graphics tablet / stylus buttons & tools (EV_KEY)
inline auto
prepare_generic_tablet(void (*fn_cb)(const micron::timeval_t &, u16, i32), void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                       void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_key, slice<button_t>(static_cast<size_t>(5)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = btn_tool_pen;
  __input.button_mask[1] = btn_tool_rubber;
  __input.button_mask[2] = btn_stylus;
  __input.button_mask[3] = btn_stylus2;
  __input.button_mask[4] = btn_touch;

  return __input;
};

// joystick / gamepad buttons (EV_KEY); axes via prepare_generic_mouse_sensor_abs
inline auto
prepare_generic_joystick(void (*fn_cb)(const micron::timeval_t &, u16, i32), void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                         void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input{ ev_key, slice<button_t>(static_cast<size_t>(18)), fn_cb, fn_acb, fn_rcb };

  __input.button_mask[0] = btn_trigger;
  __input.button_mask[1] = btn_thumb;
  __input.button_mask[2] = btn_thumb2;
  __input.button_mask[3] = btn_top;
  __input.button_mask[4] = btn_top2;
  __input.button_mask[5] = btn_pinkie;
  __input.button_mask[6] = btn_base;
  __input.button_mask[7] = btn_gamepad;
  __input.button_mask[8] = btn_east;
  __input.button_mask[9] = btn_north;
  __input.button_mask[10] = btn_west;
  __input.button_mask[11] = btn_tl;
  __input.button_mask[12] = btn_tr;
  __input.button_mask[13] = btn_select;
  __input.button_mask[14] = btn_start;
  __input.button_mask[15] = btn_mode;
  __input.button_mask[16] = btn_thumbl;
  __input.button_mask[17] = btn_thumbr;

  return __input;
};

inline auto
prepare_generic_keyboard_us(void) -> input_packet_t
{
  const byte __first = key_esc;
  const byte __end = key_kpdot;
  const size_t __count = static_cast<size_t>(__end - __first);
  input_packet_t __input{ ev_key, slice<button_t>(__count), nullptr, nullptr, nullptr };

  for ( size_t i = 0; i < __count; ++i ) __input.button_mask[i] = static_cast<button_t>(__first + i);

  return __input;
};

inline auto
prepare_generic_keyboard_us(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                            void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                            void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  const byte __first = key_esc;
  const byte __end = key_kpdot;
  const size_t __count = static_cast<size_t>(__end - __first);
  input_packet_t __input{ ev_key, slice<button_t>(__count), fn_cb, fn_acb, fn_rcb };

  for ( size_t i = 0; i < __count; ++i ) __input.button_mask[i] = static_cast<button_t>(__first + i);
  return __input;
};

inline bool
read_raw(const device_t &dev, input_event &event)
{
  if ( dev.bound_fd.has_error() ) return false;

  i64 read_bytes = posix::read(dev.bound_fd.fd, &event, sizeof(input_event));
  return (read_bytes == sizeof(input_event));
};
};      // namespace uxin
};      // namespace micron
