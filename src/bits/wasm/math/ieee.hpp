// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "real.hpp"

namespace micron::wasm::math
{
struct ieee128_traits {
  using uint_type = wide;
  static constexpr int bits = 128, mant_bits = 112, exp_bits = 15, exp_bias = 16383;
  static constexpr wide sign_mask = wide(1) << 127;
  static constexpr wide exp_mask = wide(0x7fff) << 112;
  static constexpr wide mant_mask = (wide(1) << 112) - 1;
  static constexpr wide implicit_one = wide(1) << 112;
  static constexpr wide quiet_nan_bit = wide(1) << 111;
};
}      // namespace micron::wasm::math
