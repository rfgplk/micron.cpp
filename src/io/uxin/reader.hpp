//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"

#include "devices.hpp"
#include "bits.hpp"

namespace micron
{
namespace uxin
{

auto
prepare_generic_mouse_sensor_abs(void (*fn_cb)(u16, i32), void (*fn_acb)(u16, i32) = nullptr,
                                 void (*fn_rcb)(u16, i32) = nullptr) -> raw_input_t
{
  raw_input_t __input = { ev_rel, sizeof(button_t) * 9, fn_cb, fn_acb, fn_rcb };

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
prepare_generic_mouse_sensor_abs(void) -> raw_input_t
{
  raw_input_t __input = { ev_key, sizeof(button_t) * 9, nullptr, nullptr, nullptr };

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
prepare_generic_mouse_sensor(void (*fn_cb)(u16, i32), void (*fn_acb)(u16, i32) = nullptr,
                             void (*fn_rcb)(u16, i32) = nullptr) -> raw_input_t
{
  raw_input_t __input = { ev_rel, sizeof(button_t) * 9, fn_cb, fn_acb, fn_rcb };

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
prepare_generic_mouse_sensor(void) -> raw_input_t
{
  raw_input_t __input = { ev_key, sizeof(button_t) * 9, nullptr, nullptr, nullptr };

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
prepare_generic_mouse(void (*fn_cb)(u16, i32), void (*fn_acb)(u16, i32) = nullptr, void (*fn_rcb)(u16, i32) = nullptr)
    -> raw_input_t
{
  raw_input_t __input = { ev_key, sizeof(button_t) * 8, fn_cb, fn_acb, fn_rcb };

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
prepare_generic_mouse(void) -> raw_input_t
{
  raw_input_t __input = { ev_key, sizeof(button_t) * 9, nullptr, nullptr, nullptr };

  __input.button_mask[0] = btn_mouse;
  __input.button_mask[1] = btn_left;
  __input.button_mask[2] = btn_right;
  __input.button_mask[3] = btn_middle;
  __input.button_mask[4] = btn_side;
  __input.button_mask[5] = btn_extra;
  __input.button_mask[6] = btn_forward;
  __input.button_mask[7] = btn_back;
  __input.button_mask[8] = btn_task;

  return __input;
};
bool
read_raw(const device_t &dev, input_event &event)
{
  if ( dev.bound_fd == -1 )
    return false;

  i64 read_bytes = posix::read(dev.bound_fd, &event, sizeof(input_event));
  return (read_bytes == sizeof(input_event));
};
};
};
