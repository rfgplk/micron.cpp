//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/sys/event_codes.hpp"
#include "../../linux/sys/input.hpp"
#include "../../slice.hpp"
#include "../../stack.hpp"

#include "devices.hpp"

namespace micron
{
namespace uxin
{

using button_t = i32;

struct input_packet_t {
  u16 mask_type;
  slice<button_t> button_mask;
  void (*callback)(const micron::timeval_t&, u16, i32);
  void (*actuate_callback)(const micron::timeval_t&, u16, i32);
  void (*release_callback)(const micron::timeval_t&, u16, i32);
};

struct input_t {
  device_t device;
  micron::stack<input_event> history;
};

};
};
