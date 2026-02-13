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

auto
prepare_generic_mouse_sensor_abs(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                                 void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                                 void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input = { ev_rel, sizeof(button_t) * 9, fn_cb, fn_acb, fn_rcb };

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

auto
prepare_generic_mouse_sensor_abs(void) -> input_packet_t
{
  input_packet_t __input = { ev_rel, sizeof(button_t) * 9, nullptr, nullptr, nullptr };

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

auto
prepare_generic_mouse_sensor(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                             void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                             void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input = { ev_rel, sizeof(button_t) * 9, fn_cb, fn_acb, fn_rcb };

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

auto
prepare_generic_mouse_sensor(void) -> input_packet_t
{
  input_packet_t __input = { ev_rel, sizeof(button_t) * 9, nullptr, nullptr, nullptr };

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

auto
prepare_generic_mouse(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                      void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                      void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  input_packet_t __input = { ev_key, sizeof(button_t) * 8, fn_cb, fn_acb, fn_rcb };

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

auto
prepare_generic_keyboard_us(void) -> input_packet_t
{
  const byte __first = key_esc;
  const byte __end = key_kpdot;
  input_packet_t __input = { ev_key, sizeof(button_t) * (__end - __first), nullptr, nullptr, nullptr };

  for ( byte i = 0; i < (__end - __first); ++i )
    __input.button_mask[i] = __first + i;

  return __input;
};

auto
prepare_generic_keyboard_us(void (*fn_cb)(const micron::timeval_t &, u16, i32),
                            void (*fn_acb)(const micron::timeval_t &, u16, i32) = nullptr,
                            void (*fn_rcb)(const micron::timeval_t &, u16, i32) = nullptr) -> input_packet_t
{
  const byte __first = key_esc;
  const byte __end = key_kpdot;
  input_packet_t __input = { ev_key, sizeof(button_t) * (__end - __first), fn_cb, fn_acb, fn_rcb };

  for ( byte i = 0; i < (__end - __first); ++i )
    __input.button_mask[i] = __first + i;
  return __input;
};

bool
read_raw(const device_t &dev, input_event &event)
{
  if ( dev.bound_fd.has_error() )
    return false;

  i64 read_bytes = posix::read(dev.bound_fd.fd, &event, sizeof(input_event));
  return (read_bytes == sizeof(input_event));
};
};
};
