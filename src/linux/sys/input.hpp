//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"
#include "time.hpp"

constexpr u32 ev_version = 0x010001;

struct input_event {
  struct timeval time;
  u16 type;
  u16 code;
  i32 value;
};
