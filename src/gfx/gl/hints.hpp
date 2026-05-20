//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

class context;

struct context_hints {
  u8 major = 4;
  u8 minor = 6;
  u8 depth_bits = 24;
  u8 stencil_bits = 8;
  u8 color_bits = 32;      // total RGBA bits (8+8+8+8)
  u8 ms_samples = 0;       // 0 = MSAA off
  bool srgb = false;
  bool double_buffer = true;
  bool debug = false;
  bool forward_compat = true;
  bool core_profile = true;
  const context *share_with = nullptr;
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
