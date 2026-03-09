//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../namespace.hpp"

namespace micron
{
namespace simd
{

template <is_simd_128_type T, is_flag_type F> class v128
{
  using bit_width = T;
  using lane_width = F;
  T value;

  inline void
  __impl_zero_init(void)
  {
    if constexpr ( micron::same_as<T, f128> )
      value = vdupq_n_f32(0.0f);
    if constexpr ( micron::same_as<T, d128> )
      value = vdupq_n_f64(0.0);
    if constexpr ( micron::same_as<T, i128> )
      value = vdupq_n_s32(0);
  }

  inline void
  __impl_one_init(void)
  {
    if constexpr ( micron::same_as<T, f128> )
      value = vreinterpretq_f32_s32(vdupq_n_s32(-1));
    if constexpr ( micron::same_as<T, d128> )
      value = vreinterpretq_f64_s64(vdupq_n_s64(-1LL));
    if constexpr ( micron::same_as<T, i128> )
      value = vdupq_n_s32(-1);
  }

  static inline i128
  __all_ones_si128(void) noexcept
  {
    return vdupq_n_s32(-1);
  }

public:
  ~v128() = default;

  v128(void) { __impl_zero_init(); }

  inline v128 &
  operator=(std::initializer_list<double> lst)
  {
    if ( lst.size() != 2 )
      return *this;
    double a = 0, b = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    const double arr[2] = { a, b };
    value = vld1q_f64(arr);
    return *this;
  }

  inline v128 &
  operator=(std::initializer_list<float> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    float a = 0, b = 0, c = 0, d = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      if ( _f == 2 )
        c = *itr;
      if ( _f == 3 )
        d = *itr;
      _f++;
    }
    const float arr[4] = { a, b, c, d };
    value = vld1q_f32(arr);
    return *this;
  }

  inline v128 &
  operator=(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 2 )
      return *this;
    i64 a = 0, b = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    const int64_t arr[2] = { static_cast<int64_t>(a), static_cast<int64_t>(b) };
    value = vreinterpretq_s32_s64(vld1q_s64(arr));
    return *this;
  }

  inline v128 &
  operator=(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    i32 __arr[4];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vld1q_s32(__arr);
    return *this;
  }

  inline v128 &
  operator=(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 8 )
      return *this;
    i16 __arr[8];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vreinterpretq_s32_s16(vld1q_s16(__arr));
    return *this;
  }

  inline v128 &
  operator=(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 16 )
      return *this;
    i8 __arr[16];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vreinterpretq_s32_s8(vld1q_s8(__arr));
    return *this;
  }

  v128(std::initializer_list<double> lst)
  {
    if ( lst.size() != 2 ) {
      __impl_zero_init();
      return;
    }
    double a = 0, b = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    const double arr[2] = { a, b };
    value = vld1q_f64(arr);
  }

  v128(std::initializer_list<float> lst)
  {
    if ( lst.size() != 4 ) {
      __impl_zero_init();
      return;
    }
    float a = 0, b = 0, c = 0, d = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      if ( _f == 2 )
        c = *itr;
      if ( _f == 3 )
        d = *itr;
      _f++;
    }
    const float arr[4] = { a, b, c, d };
    value = vld1q_f32(arr);
  }

  v128(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 2 ) {
      __impl_zero_init();
      return;
    }
    i64 a = 0, b = 0;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    const int64_t arr[2] = { static_cast<int64_t>(a), static_cast<int64_t>(b) };
    value = vreinterpretq_s32_s64(vld1q_s64(arr));
  }

  v128(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 4 ) {
      __impl_zero_init();
      return;
    }
    i32 __arr[4];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vld1q_s32(__arr);
  }

  v128(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 8 ) {
      __impl_zero_init();
      return;
    }
    i16 __arr[8];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vreinterpretq_s32_s16(vld1q_s16(__arr));
  }

  v128(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 16 ) {
      __impl_zero_init();
      return;
    }
    i8 __arr[16];
    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = vreinterpretq_s32_s8(vld1q_s8(__arr));
  }

  v128(i8 _e0)
    requires micron::is_same_v<T, i128>
  {
    value = vreinterpretq_s32_s8(vdupq_n_s8(_e0));
  }

  v128(i16 _e0)
    requires micron::is_same_v<T, i128>
  {
    value = vreinterpretq_s32_s16(vdupq_n_s16(_e0));
  }

  v128(i32 a)
    requires micron::is_same_v<T, i128>
  {
    value = vdupq_n_s32(a);
  }

  v128(i64 a)
    requires micron::is_same_v<T, i128>
  {
    value = vreinterpretq_s32_s64(vdupq_n_s64(a));
  }

  v128(float a)
    requires micron::is_same_v<T, f128>
  {
    value = vdupq_n_f32(a);
  }

  v128(double a)
    requires micron::is_same_v<T, d128>
  {
    value = vdupq_n_f64(a);
  }

  v128(i8 _e0, i8 _e1, i8 _e2, i8 _e3, i8 _e4, i8 _e5, i8 _e6, i8 _e7, i8 _e8, i8 _e9, i8 _e10, i8 _e11, i8 _e12, i8 _e13, i8 _e14, i8 _e15)
    requires micron::is_same_v<T, i128>
  {
    const int8_t arr[16] = { _e15, _e14, _e13, _e12, _e11, _e10, _e9, _e8, _e7, _e6, _e5, _e4, _e3, _e2, _e1, _e0 };
    value = vreinterpretq_s32_s8(vld1q_s8(arr));
  }

  v128(i16 _e0, i16 _e1, i16 _e2, i16 _e3, i16 _e4, i16 _e5, i16 _e6, i16 _e7)
    requires micron::is_same_v<T, i128>
  {
    const int16_t arr[8] = { _e7, _e6, _e5, _e4, _e3, _e2, _e1, _e0 };
    value = vreinterpretq_s32_s16(vld1q_s16(arr));
  }

  v128(i32 a, i32 b, i32 c, i32 d)
    requires micron::is_same_v<T, i128>
  {
    const int32_t arr[4] = { a, b, c, d };
    value = vld1q_s32(arr);
  }

  v128(i64 a, i64 b)
    requires micron::is_same_v<T, i128>
  {
    const int64_t arr[2] = { static_cast<int64_t>(a), static_cast<int64_t>(b) };
    value = vreinterpretq_s32_s64(vld1q_s64(arr));
  }

  v128(float a, float b, float c, float d)
    requires micron::is_same_v<T, f128>
  {
    const float arr[4] = { a, b, c, d };
    value = vld1q_f32(arr);
  }

  v128(double a, double b)
    requires micron::is_same_v<T, d128>
  {
    const double arr[2] = { a, b };
    value = vld1q_f64(arr);
  }

  v128(const v128 &o) : value(o.value) {}

  v128(v128 &&o) : value(o.value)
  {
    if constexpr ( micron::same_as<T, f128> )
      o.value = vdupq_n_f32(0.0f);
    if constexpr ( micron::same_as<T, d128> )
      o.value = vdupq_n_f64(0.0);
    if constexpr ( micron::same_as<T, i128> )
      o.value = vdupq_n_s32(0);
  }

  inline v128 &
  operator=(const v128 &o)
  {
    value = o.value;
    return *this;
  }

  inline v128 &
  operator=(v128 &&o)
  {
    value = o.value;
    return *this;
  }

  inline v128 &
  operator=(float f)
  {
    value = vdupq_n_f32(f);
    return *this;
  }

  inline v128 &
  operator=(double d)
  {
    value = vdupq_n_f64(d);
    return *this;
  }

  inline v128 &
  operator=(i8 c)
  {
    value = vreinterpretq_s32_s8(vdupq_n_s8(c));
    return *this;
  }

  inline v128 &
  operator=(i16 s)
  {
    value = vreinterpretq_s32_s16(vdupq_n_s16(s));
    return *this;
  }

  inline v128 &
  operator=(i32 i)
  {
    value = vdupq_n_s32(i);
    return *this;
  }

  inline v128 &
  operator=(i64 l)
  {
    value = vreinterpretq_s32_s64(vdupq_n_s64(l));
    return *this;
  }

  inline v128 &
  operator=(F *addr)
  {
    return uload<F>(addr);
  }

  constexpr usize
  size(void)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      return 4;
    else if constexpr ( micron::is_same_v<T, d128> )
      return 2;
    else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_64_wide<F>() )
        return 2;
      if constexpr ( __is_32_wide<F>() )
        return 4;
      if constexpr ( __is_16_wide<F>() )
        return 8;
      if constexpr ( __is_8_wide<F>() )
        return 16;
    }
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  constexpr auto
  operator[](const R a)
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      float _f = 0.0f;
      switch ( a ) {
      case 0 :
        _f = vgetq_lane_f32(value, 0);
        break;
      case 1 :
        _f = vgetq_lane_f32(value, 1);
        break;
      case 2 :
        _f = vgetq_lane_f32(value, 2);
        break;
      case 3 :
        _f = vgetq_lane_f32(value, 3);
        break;
      }
      return _f;
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      double _d = 0.0;
      if ( a == 0 )
        _d = vgetq_lane_f64(value, 0);
      else
        _d = vgetq_lane_f64(value, 1);
      return _d;
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_64_wide<F>() ) {
        i64 _d = 0;
        switch ( a ) {
        case 0 :
          _d = static_cast<i64>(vgetq_lane_s64(vreinterpretq_s64_s32(value), 0));
          break;
        case 1 :
          _d = static_cast<i64>(vgetq_lane_s64(vreinterpretq_s64_s32(value), 1));
          break;
        }
        return _d;
      }
      if constexpr ( __is_32_wide<F>() ) {
        i32 _d = 0;
        switch ( a ) {
        case 0 :
          _d = vgetq_lane_s32(value, 0);
          break;
        case 1 :
          _d = vgetq_lane_s32(value, 1);
          break;
        case 2 :
          _d = vgetq_lane_s32(value, 2);
          break;
        case 3 :
          _d = vgetq_lane_s32(value, 3);
          break;
        }
        return _d;
      }
      if constexpr ( __is_16_wide<F>() ) {
        i16 _d = 0;
        auto v16 = vreinterpretq_s16_s32(value);
        switch ( a ) {
        case 0 :
          _d = vgetq_lane_s16(v16, 0);
          break;
        case 1 :
          _d = vgetq_lane_s16(v16, 1);
          break;
        case 2 :
          _d = vgetq_lane_s16(v16, 2);
          break;
        case 3 :
          _d = vgetq_lane_s16(v16, 3);
          break;
        case 4 :
          _d = vgetq_lane_s16(v16, 4);
          break;
        case 5 :
          _d = vgetq_lane_s16(v16, 5);
          break;
        case 6 :
          _d = vgetq_lane_s16(v16, 6);
          break;
        case 7 :
          _d = vgetq_lane_s16(v16, 7);
          break;
        }
        return _d;
      }
      if constexpr ( __is_8_wide<F>() ) {
        i8 _d = 0;
        auto v8 = vreinterpretq_s8_s32(value);
        switch ( a ) {
        case 0 :
          _d = vgetq_lane_s8(v8, 0);
          break;
        case 1 :
          _d = vgetq_lane_s8(v8, 1);
          break;
        case 2 :
          _d = vgetq_lane_s8(v8, 2);
          break;
        case 3 :
          _d = vgetq_lane_s8(v8, 3);
          break;
        case 4 :
          _d = vgetq_lane_s8(v8, 4);
          break;
        case 5 :
          _d = vgetq_lane_s8(v8, 5);
          break;
        case 6 :
          _d = vgetq_lane_s8(v8, 6);
          break;
        case 7 :
          _d = vgetq_lane_s8(v8, 7);
          break;
        case 8 :
          _d = vgetq_lane_s8(v8, 8);
          break;
        case 9 :
          _d = vgetq_lane_s8(v8, 9);
          break;
        case 10 :
          _d = vgetq_lane_s8(v8, 10);
          break;
        case 11 :
          _d = vgetq_lane_s8(v8, 11);
          break;
        case 12 :
          _d = vgetq_lane_s8(v8, 12);
          break;
        case 13 :
          _d = vgetq_lane_s8(v8, 13);
          break;
        case 14 :
          _d = vgetq_lane_s8(v8, 14);
          break;
        case 15 :
          _d = vgetq_lane_s8(v8, 15);
          break;
        }
        return _d;
      }
    }
  }

  constexpr int
  operator==(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(vreinterpretq_f32_u32(vceqq_f32(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(vreinterpretq_f64_u64(vceqq_f64(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _r;
      if constexpr ( __is_8_wide<F>() )
        _r = vreinterpretq_s32_u8(vceqq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r = vreinterpretq_s32_u16(vceqq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r = vreinterpretq_s32_u32(vceqq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _r = vreinterpretq_s32_u64(vceqq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr int
  operator!=(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {

      auto _eq = vceqq_f32(value, o.value);
      auto _ne = veorq_u32(_eq, vreinterpretq_u32_s32(neon_all_ones_si128()));
      return neon_movemask_f32(vreinterpretq_f32_u32(_ne));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      auto _eq = vceqq_f64(value, o.value);
      auto _ne = veorq_u64(_eq, vreinterpretq_u64_s32(neon_all_ones_si128()));
      return neon_movemask_f64(vreinterpretq_f64_u64(_ne));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _eq, _r;
      if constexpr ( __is_8_wide<F>() )
        _eq = vreinterpretq_s32_u8(vceqq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _eq = vreinterpretq_s32_u16(vceqq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _eq = vreinterpretq_s32_u32(vceqq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _eq = vreinterpretq_s32_u64(vceqq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      _r = veorq_s32(_eq, neon_all_ones_si128());
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr int
  operator>=(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(vreinterpretq_f32_u32(vcgeq_f32(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(vreinterpretq_f64_u64(vcgeq_f64(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _r;
      if constexpr ( __is_8_wide<F>() )
        _r = vreinterpretq_s32_u8(vcgeq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r = vreinterpretq_s32_u16(vcgeq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r = vreinterpretq_s32_u32(vcgeq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _r = vreinterpretq_s32_u64(vcgeq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr int
  operator>(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(vreinterpretq_f32_u32(vcgtq_f32(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(vreinterpretq_f64_u64(vcgtq_f64(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _r;
      if constexpr ( __is_8_wide<F>() )
        _r = vreinterpretq_s32_u8(vcgtq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r = vreinterpretq_s32_u16(vcgtq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r = vreinterpretq_s32_u32(vcgtq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _r = vreinterpretq_s32_u64(vcgtq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr int
  operator<(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(vreinterpretq_f32_u32(vcltq_f32(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(vreinterpretq_f64_u64(vcltq_f64(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _r;
      if constexpr ( __is_8_wide<F>() )
        _r = vreinterpretq_s32_u8(vcltq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r = vreinterpretq_s32_u16(vcltq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r = vreinterpretq_s32_u32(vcltq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _r = vreinterpretq_s32_u64(vcltq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr int
  operator<=(const v128 &o) const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(vreinterpretq_f32_u32(vcleq_f32(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(vreinterpretq_f64_u64(vcleq_f64(value, o.value)));
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i128 _r;
      if constexpr ( __is_8_wide<F>() )
        _r = vreinterpretq_s32_u8(vcleq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r = vreinterpretq_s32_u16(vcleq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r = vreinterpretq_s32_u32(vcleq_s32(value, o.value));
      if constexpr ( __is_64_wide<F>() )
        _r = vreinterpretq_s32_u64(vcleq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
      return neon_movemask_si128(_r);
    }
    return 0x0;
  }

  constexpr inline v128 &
  operator+=(double x)
  {
    if constexpr ( micron::is_same_v<T, d128> )
      value = vaddq_f64(value, vdupq_n_f64(x));
    return *this;
  }

  constexpr inline v128 &
  operator+=(float x)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      value = vaddq_f32(value, vdupq_n_f32(x));
    return *this;
  }

  template <typename A>
  constexpr inline v128 &
  operator+=(A __x)
    requires is_int_flag_type<A>
  {
    F x = static_cast<F>(__x);
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        value = vreinterpretq_s32_s8(vaddq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(x))));
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vaddq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(x))));
      if constexpr ( __is_32_wide<F>() )
        value = vaddq_s32(value, vdupq_n_s32(static_cast<int32_t>(x)));
      if constexpr ( __is_64_wide<F>() )
        value = vreinterpretq_s32_s64(vaddq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(x))));
    }
    return *this;
  }

  constexpr inline v128 &
  operator+=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      value = vaddq_f32(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      value = vaddq_f64(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        value = vreinterpretq_s32_s8(vaddq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vaddq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        value = vaddq_s32(value, o.value);
      if constexpr ( __is_64_wide<F>() )
        value = vreinterpretq_s32_s64(vaddq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
    }
    return *this;
  }

  constexpr inline v128 &
  operator*=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      value = vmulq_f32(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      value = vmulq_f64(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "No epi8 SIMD multiply");
      }
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        value = vmulq_s32(value, o.value);
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "No epi64 SIMD multiply");
      }
    }
    return *this;
  }

  constexpr inline v128 &
  operator/=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      value = vdivq_f32(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      value = vdivq_f64(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      static_assert(!micron::is_same_v<T, i128>, "No SIMD integer division (NEON, same as SSE)");
    }
    return *this;
  }

  constexpr inline v128
  operator/(const v128 &o) const
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, f128> ) {
      _d.value = vdivq_f32(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      _d.value = vdivq_f64(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      static_assert(!micron::is_same_v<T, i128>, "No SIMD integer division (NEON, same as SSE)");
    }
    return _d;
  }

  constexpr inline v128 &
  operator-=(double x)
  {
    if constexpr ( micron::is_same_v<T, d128> )
      value = vsubq_f64(value, vdupq_n_f64(x));
    return *this;
  }

  constexpr inline v128 &
  operator-=(float x)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      value = vsubq_f32(value, vdupq_n_f32(x));
    return *this;
  }

  template <typename A>
  constexpr inline v128 &
  operator-=(A x)
    requires is_int_flag_type<A>
  {
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( micron::is_same_v<A, __v8> || micron::is_same_v<A, __uv8> )
        value = vreinterpretq_s32_s8(vsubq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(x))));
      if constexpr ( micron::is_same_v<A, __v16> || micron::is_same_v<A, __uv16> )
        value = vreinterpretq_s32_s16(vsubq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(x))));
      if constexpr ( micron::is_same_v<A, __v32> || micron::is_same_v<A, __uv32> )
        value = vsubq_s32(value, vdupq_n_s32(static_cast<int32_t>(x)));
      if constexpr ( micron::is_same_v<A, __v64> || micron::is_same_v<A, __uv64> )
        value = vreinterpretq_s32_s64(vsubq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(x))));
    }
    return *this;
  }

  constexpr inline v128 &
  operator-=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      value = vsubq_f32(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      value = vsubq_f64(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        value = vreinterpretq_s32_s8(vsubq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vsubq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        value = vsubq_s32(value, o.value);
      if constexpr ( __is_64_wide<F>() )
        value = vreinterpretq_s32_s64(vsubq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
    }
    return *this;
  }

  constexpr inline v128
  operator+(const v128 &o) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vaddq_f32(value, o.value);
    else if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vaddq_f64(value, o.value);
    else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        _r.value = vreinterpretq_s32_s8(vaddq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r.value = vreinterpretq_s32_s16(vaddq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r.value = vaddq_s32(value, o.value);
      if constexpr ( __is_64_wide<F>() )
        _r.value = vreinterpretq_s32_s64(vaddq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
    }
    return _r;
  }

  constexpr inline v128
  operator-(const v128 &o) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vsubq_f32(value, o.value);
    else if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vsubq_f64(value, o.value);
    else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        _r.value = vreinterpretq_s32_s8(vsubq_s8(vreinterpretq_s8_s32(value), vreinterpretq_s8_s32(o.value)));
      if constexpr ( __is_16_wide<F>() )
        _r.value = vreinterpretq_s32_s16(vsubq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(o.value)));
      if constexpr ( __is_32_wide<F>() )
        _r.value = vsubq_s32(value, o.value);
      if constexpr ( __is_64_wide<F>() )
        _r.value = vreinterpretq_s32_s64(vsubq_s64(vreinterpretq_s64_s32(value), vreinterpretq_s64_s32(o.value)));
    }
    return _r;
  }

  constexpr bool
  all_zeroes() const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(value) == 0x0;
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(value) == 0x0;
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      const uint64x2_t v64 = vreinterpretq_u64_s32(value);
      return (vgetq_lane_u64(v64, 0) | vgetq_lane_u64(v64, 1)) == 0ULL;
    }
  }

  constexpr bool
  all_ones() const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return neon_movemask_f32(value) == 0xF;
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return neon_movemask_f64(value) == 0x3;
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      const uint64x2_t v64 = vreinterpretq_u64_s32(value);
      return (vgetq_lane_u64(v64, 0) & vgetq_lane_u64(v64, 1)) == UINT64_MAX;
    }
  }

  constexpr void
  to_zero(void)
  {
    __impl_zero_init();
  }

  constexpr void
  to_ones(void)
  {
    __impl_one_init();
  }

  inline v128 &
  set(float f)
  {
    value = vdupq_n_f32(f);
    return *this;
  }

  inline v128 &
  set(double d)
  {
    value = vdupq_n_f64(d);
    return *this;
  }

  inline v128 &
  set(i8 c)
  {
    value = vreinterpretq_s32_s8(vdupq_n_s8(c));
    return *this;
  }

  inline v128 &
  set(i16 s)
  {
    value = vreinterpretq_s32_s16(vdupq_n_s16(s));
    return *this;
  }

  inline v128 &
  set(i32 i)
  {
    value = vdupq_n_s32(i);
    return *this;
  }

  inline v128 &
  set(i64 l)
  {
    value = vreinterpretq_s32_s64(vdupq_n_s64(l));
    return *this;
  }

  inline v128 &
  set(float const *f)
  {
    value = vld1q_dup_f32(f);
    return *this;
  }

  inline v128 &
  broadcast(float const *f)
  {
    return set(f);
  }

  template <typename B>
  inline v128 &
  load(B *mem)
  {
    if ( !is_aligned<128>(mem) )
      return *this;
    value = ::micron::simd::load<T, B>(mem);
    return *this;
  }

  template <typename B>
  inline v128 &
  uload(B *mem)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      value = vld1q_f32(reinterpret_cast<const float *>(mem));
    else if constexpr ( micron::is_same_v<T, d128> )
      value = vld1q_f64(reinterpret_cast<const double *>(mem));
    else if constexpr ( micron::is_same_v<T, i128> )
      value = vreinterpretq_s32_u8(vld1q_u8(reinterpret_cast<const uint8_t *>(mem)));
    return *this;
  }

  inline v128 &
  operator|=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, i128> )
      value = vorrq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      value = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      value = vreinterpretq_f64_u64(vorrq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return *this;
  }

  inline v128 &
  operator&=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, i128> )
      value = vandq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      value = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      value = vreinterpretq_f64_u64(vandq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return *this;
  }

  inline v128 &
  operator^=(const v128 &o)
  {
    if constexpr ( micron::is_same_v<T, i128> )
      value = veorq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      value = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      value = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return *this;
  }

  inline v128
  operator|(const v128 &o) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> )
      _r.value = vorrq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vreinterpretq_f64_u64(vorrq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return _r;
  }

  inline v128
  operator&(const v128 &o) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> )
      _r.value = vandq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vreinterpretq_f64_u64(vandq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return _r;
  }

  inline v128
  operator^(const v128 &o) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> )
      _r.value = veorq_s32(value, o.value);
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_f32(o.value)));
    if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_f64(o.value)));
    return _r;
  }

  inline v128
  operator~() const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> )
      _r.value = veorq_s32(value, neon_all_ones_si128());
    if constexpr ( micron::is_same_v<T, f128> )
      _r.value = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(value), vreinterpretq_u32_s32(neon_all_ones_si128())));
    if constexpr ( micron::is_same_v<T, d128> )
      _r.value = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(value), vreinterpretq_u64_s32(neon_all_ones_si128())));
    return _r;
  }

  inline v128
  operator<<(int i) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        _r.value = vreinterpretq_s32_s8(vshlq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(i))));
      if constexpr ( __is_16_wide<F>() )
        _r.value = vreinterpretq_s32_s16(vshlq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(i))));
      if constexpr ( __is_32_wide<F>() )
        _r.value = vshlq_s32(value, vdupq_n_s32(i));
      if constexpr ( __is_64_wide<F>() )
        _r.value = vreinterpretq_s32_s64(vshlq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(i))));
    }
    return _r;
  }

  inline v128 &
  operator<<=(int i)
  {
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        value = vreinterpretq_s32_s8(vshlq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(i))));
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vshlq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(i))));
      if constexpr ( __is_32_wide<F>() )
        value = vshlq_s32(value, vdupq_n_s32(i));
      if constexpr ( __is_64_wide<F>() )
        value = vreinterpretq_s32_s64(vshlq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(i))));
    }
    return *this;
  }

  inline v128
  operator>>(int i) const
  {
    v128 _r;
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        _r.value = vreinterpretq_s32_s8(vshlq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(-i))));
      if constexpr ( __is_16_wide<F>() )
        _r.value = vreinterpretq_s32_s16(vshlq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(-i))));
      if constexpr ( __is_32_wide<F>() )
        _r.value = vshlq_s32(value, vdupq_n_s32(-i));
      if constexpr ( __is_64_wide<F>() )
        _r.value = vreinterpretq_s32_s64(vshlq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(-i))));
    }
    return _r;
  }

  inline v128 &
  operator>>=(int i)
  {
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() )
        value = vreinterpretq_s32_s8(vshlq_s8(vreinterpretq_s8_s32(value), vdupq_n_s8(static_cast<int8_t>(-i))));
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vshlq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(-i))));
      if constexpr ( __is_32_wide<F>() )
        value = vshlq_s32(value, vdupq_n_s32(-i));
      if constexpr ( __is_64_wide<F>() )
        value = vreinterpretq_s32_s64(vshlq_s64(vreinterpretq_s64_s32(value), vdupq_n_s64(static_cast<int64_t>(-i))));
    }
    return *this;
  }

  constexpr inline v128 &
  operator/=(double x)
  {
    if constexpr ( micron::is_same_v<T, d128> )
      value = vdivq_f64(value, vdupq_n_f64(x));
    return *this;
  }

  constexpr inline v128 &
  operator/=(float x)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      value = vdivq_f32(value, vdupq_n_f32(x));
    return *this;
  }

  constexpr inline v128
  operator/(double x) const
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, d128> )
      _d.value = vdivq_f64(value, vdupq_n_f64(x));
    return _d;
  }

  constexpr inline v128
  operator/(float x) const
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, f128> )
      _d.value = vdivq_f32(value, vdupq_n_f32(x));
    return _d;
  }

  constexpr inline v128 &
  operator*=(double x)
  {
    if constexpr ( micron::is_same_v<T, d128> )
      value = vmulq_f64(value, vdupq_n_f64(x));
    return *this;
  }

  constexpr inline v128 &
  operator*=(float x)
  {
    if constexpr ( micron::is_same_v<T, f128> )
      value = vmulq_f32(value, vdupq_n_f32(x));
    return *this;
  }

  template <typename A>
  constexpr inline v128 &
  operator*=(A __x)
    requires is_int_flag_type<A>
  {
    F x = static_cast<F>(__x);
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "No epi8 SIMD multiply");
      }
      if constexpr ( __is_16_wide<F>() )
        value = vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(x))));
      if constexpr ( __is_32_wide<F>() )
        value = vmulq_s32(value, vdupq_n_s32(static_cast<int32_t>(x)));
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "No epi64 SIMD multiply");
      }
    }
    return *this;
  }

  constexpr inline v128
  operator*(double x) const
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, d128> )
      _d.value = vmulq_f64(value, vdupq_n_f64(x));
    return _d;
  }

  constexpr inline v128
  operator*(float x) const
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, f128> )
      _d.value = vmulq_f32(value, vdupq_n_f32(x));
    return _d;
  }

  constexpr inline v128
  operator*(F x)
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "No epi8 SIMD multiply");
      }
      if constexpr ( __is_16_wide<F>() )
        _d.value = vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(value), vdupq_n_s16(static_cast<int16_t>(x))));
      if constexpr ( __is_32_wide<F>() )
        _d.value = vmulq_s32(value, vdupq_n_s32(static_cast<int32_t>(x)));
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "No epi64 SIMD multiply");
      }
    }
    return _d;
  }

  constexpr inline v128
  operator*(v128 x)
  {
    v128 _d;
    if constexpr ( micron::is_same_v<T, f128> )
      _d.value = vmulq_f32(value, x.value);
    else if constexpr ( micron::is_same_v<T, d128> )
      _d.value = vmulq_f64(value, x.value);
    else if constexpr ( micron::is_same_v<T, i128> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "No epi8 SIMD multiply");
      }
      if constexpr ( __is_16_wide<F>() )
        _d.value = vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(value), vreinterpretq_s16_s32(x.value)));
      if constexpr ( __is_32_wide<F>() )
        _d.value = vmulq_s32(value, x.value);
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "No epi64 SIMD multiply");
      }
    }
    return _d;
  }

  inline void
  get(F *arr) const
  {
    if constexpr ( micron::is_same_v<F, __vf> )
      vst1q_f32(reinterpret_cast<float *>(arr), value);
    if constexpr ( micron::is_same_v<F, __vd> )
      vst1q_f64(reinterpret_cast<double *>(arr), value);
    if constexpr ( __is_8_wide<F>() || __is_16_wide<F>() || __is_32_wide<F>() || __is_64_wide<F>() )
      vst1q_s32(reinterpret_cast<int32_t *>(arr), value);
  }

  inline void
  get_aligned(F *arr) const
  {
    get(arr);
  }
};

};     // namespace simd
};     // namespace micron
