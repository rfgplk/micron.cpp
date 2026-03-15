//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// floats
// (uses ryu method)

template <typename T = char>
inline micron::hstring<T>
float_to_string(f32 val)
{
  char buf[32];
  usize n = __impl::__ryu::d2s_buffered(static_cast<f64>(val), buf);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
double_to_string(f64 val)
{
  char buf[32];
  usize n = __impl::__ryu::d2s_buffered(val, buf);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
float_to_string(f32 val, u32 prec)
{
  char buf[48];
  usize n = __impl::__ryu::d2f_buffered(static_cast<f64>(val), buf, 48, prec);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
double_to_string(f64 val, u32 prec)
{
  char buf[64];
  usize n = __impl::__ryu::d2f_buffered(val, buf, 64, prec);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
to_fixed(f64 val, u32 precision = 6)
{
  char buf[64];
  usize n = __impl::__ryu::d2f_buffered(val, buf, 64, precision);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
to_scientific(f64 val, u32 precision = 6)
{
  char buf[80];
  usize n = __impl::__ryu::d2e_buffered(val, buf, 80, precision);
  return micron::hstring<T>(buf, buf + n);
}

template <typename T = char>
inline micron::hstring<T>
to_general(f64 val, u32 precision = 6)
{
  f64 abs_val = val < 0.0 ? -val : val;
  if ( abs_val != 0.0 ) {
    i32 exp = 0;
    f64 tmp = abs_val;
    while ( tmp >= 10.0 ) {
      tmp /= 10.0;
      ++exp;
    }
    while ( tmp < 1.0 ) {
      tmp *= 10.0;
      --exp;
    }
    if ( exp < -4 || exp >= static_cast<i32>(precision) )
      return to_scientific<T>(val, precision > 0 ? precision - 1 : 0);
  }
  return to_fixed<T>(val, precision);
}

};     // namespace micron
