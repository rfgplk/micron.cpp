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

// bit width - lane width
template <is_simd_256_type T, is_flag_type F> class v256
{
  T value;

  inline void
  __impl_zero_init(void)
  {
    if constexpr ( micron::same_as<T, f256> ) {
      value = _mm256_setzero_ps();
    }
    if constexpr ( micron::same_as<T, d256> ) {
      value = _mm256_setzero_pd();
    }
    if constexpr ( micron::same_as<T, i256> ) {
      value = _mm256_setzero_si256();
    }
  }

  inline void
  __impl_one_init(void)
  {
    if constexpr ( micron::same_as<T, f256> ) {
      value = _mm256_castsi256_pd(_mm256_set1_epi32(-1));
    }
    if constexpr ( micron::same_as<T, d256> ) {
      value = _mm256_castsi256_ps(_mm256_set1_epi64x(-1));
    }
    if constexpr ( micron::same_as<T, i256> ) {
      value = _mm256_set1_epi32(-1);
    }
  }

public:
  ~v256() = default;
  v256(void) { __impl_zero_init(); }

  inline v256 &
  operator=(std::initializer_list<double> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    double a, b, c, d;
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
    value = _mm256_set_pd(d, c, b, a);
    return *this;
  }
  inline v256 &
  operator=(std::initializer_list<float> lst)
  {
    if ( lst.size() != 8 )
      return *this;

    float __arr[8];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_ps(__arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  // start of ints
  inline v256 &
  operator=(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    i64 a, b, c, d;
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
    value = _mm256_set_epi64x(d, c, b, a);
    return *this;
  }
  inline v256 &
  operator=(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 8 )
      return *this;
    i32 __arr[8];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_epi32(__arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  inline v256 &
  operator=(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 16 )
      return *this;
    i16 __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_epi16(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  inline v256 &
  operator=(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 16 )
      return *this;
    i8 __arr[32];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm256_set_epi8(__arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                            __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                            __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                            __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  // end of ints
  v256(i8 _e0)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set1_epi8(_e0);
  }
  v256(i16 _e0)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set1_epi16(_e0);
  }
  v256(i32 a)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set1_epi32(a);
  }
  v256(i64 a)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set1_epi64x(a);
  }
  v256(float a)
    requires micron::is_same_v<T, f256>
  {
    value = _mm256_set1_ps(a);
  }
  v256(double a)
    requires micron::is_same_v<T, d256>
  {
    value = _mm256_set1_pd(a);
  }
  v256(std::initializer_list<double> lst)
  {
    if ( lst.size() != 4 )
      return;
    double a, b, c, d;
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
    value = _mm256_set_pd(d, c, b, a);
  }
  v256(std::initializer_list<float> lst)
  {
    if ( lst.size() != 8 )
      return;
    float a, b, c, d, e, f, g, h;
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
      if ( _f == 4 )
        e = *itr;
      if ( _f == 5 )
        f = *itr;
      if ( _f == 6 )
        g = *itr;
      if ( _f == 7 )
        h = *itr;
      _f++;
    }
    value = _mm256_set_ps(h, g, f, e, d, c, b, a);
  }
  // start of ints
  v256(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 4 )
      return;
    i64 a, b, c, d;
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
    value = _mm256_set_epi64x(d, c, b, a);
  }
  v256(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 8 )
      return;
    i32 __arr[8];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_epi32(__arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  v256(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 16 )
      return;
    i16 __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_epi16(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  v256(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 32 )
      return;
    i8 __arr[32];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm256_set_epi8(__arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                            __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                            __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                            __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  // end of ints

  v256(i8 _e0, i8 _e1, i8 _e2, i8 _e3, i8 _e4, i8 _e5, i8 _e6, i8 _e7, i8 _e8, i8 _e9, i8 _e10, i8 _e11, i8 _e12,
       i8 _e13, i8 _e14, i8 _e15, i8 _e16, i8 _e17, i8 _e18, i8 _e19, i8 _e20, i8 _e21, i8 _e22, i8 _e23, i8 _e24,
       i8 _e25, i8 _e26, i8 _e27, i8 _e28, i8 _e29, i8 _e30, i8 _e31)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set_epi8(_e0, _e1, _e2, _e3, _e4, _e5, _e6, _e7, _e8, _e9, _e10, _e11, _e12, _e13, _e14, _e15, _e16,
                            _e17, _e18, _e19, _e20, _e21, _e22, _e23, _e24, _e25, _e26, _e27, _e28, _e29, _e30, _e31);
  }
  v256(i16 _e0, i16 _e1, i16 _e2, i16 _e3, i16 _e4, i16 _e5, i16 _e6, i16 _e7, i16 _e8, i16 _e9, i16 _e10, i16 _e11,
       i16 _e12, i16 _e13, i16 _e14, i16 _e15)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set_epi16(_e0, _e1, _e2, _e3, _e4, _e5, _e6, _e7, _e8, _e9, _e10, _e11, _e12, _e13, _e14, _e15);
  }
  v256(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set_epi32(h, g, f, e, d, c, b, a);
  }
  v256(i64 a, i64 b, i64 c, i64 d)
    requires micron::is_same_v<T, i256>
  {
    value = _mm256_set_epi64x(d, c, b, a);
  }
  v256(float a, float b, float c, float d, float e, float f, float g, float h)
    requires micron::is_same_v<T, f256>
  {
    value = _mm256_set_ps(h, g, f, e, d, c, b, a);
  }
  v256(double a, double b, double c, double d)
    requires micron::is_same_v<T, d256>
  {
    value = _mm256_set_pd(d, c, b, a);
  }
  v256(const v256 &o) : value(o.value) {}
  v256(v256 &&o) : value(o.value)
  {
    if constexpr ( micron::same_as<T, f256> ) {
      o.value = _mm256_setzero_ps();
    }
    if constexpr ( micron::same_as<T, d256> ) {
      o.value = _mm256_setzero_pd();
    }
    if constexpr ( micron::same_as<T, i256> ) {
      o.value = _mm256_setzero_si256();
    }
  }
  inline v256 &
  operator=(const v256 &o)
  {
    value = o.value;
    return *this;
  }
  inline v256 &
  operator=(v256 &&o)
  {
    value = o.value;
    return *this;
  }
  // broadcast /set() analog.
  inline v256 &
  operator=(float f)
  {
    value = _mm256_set1_ps(f);
    return *this;
  }
  inline v256 &
  operator=(double d)
  {
    value = _mm256_set1_pd(d);
    return *this;
  }
  inline v256 &
  operator=(i8 c)
  {
    value = _mm256_set1_epi8(c);
    return *this;
  }
  inline v256 &
  operator=(i16 s)
  {
    value = _mm256_set1_epi16(s);
    return *this;
  }
  inline v256 &
  operator=(i32 i)
  {
    value = _mm256_set1_epi32(i);
    return *this;
  }
  inline v256 &
  operator=(i64 l)
  {
    value = _mm256_set1_epi64x(l);
    return *this;
  }
  inline v256 &
  operator=(F *addr)
  {
    return uload<F>(addr);
  }
  // for loops
  constexpr size_t
  size(void)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      return 1;
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      return 4;
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_64_wide<F>() ) {
        return 4;
      }
      if constexpr ( __is_32_wide<F>() ) {
        return 8;
      }
      if constexpr ( __is_16_wide<F>() ) {
        return 16;
      }
      if constexpr ( __is_8_wide<F>() ) {
        return 32;
      }
    }
  }

  template <typename R>
    requires(micron::is_integral_v<R>)
  constexpr auto
  operator[](const R a)
  {
    // guess the formatter doesn't like this :(
    if constexpr ( micron::is_same_v<T, f256> ) {
      float _f = 0.0f;
      // thanks docs ;c
      switch ( a ) {
      case 0:
        _f = _mm256_cvtss_f32(_mm256_permute_ps(value, _MM_SHUFFLE(0, 0, 0, 0)));
        break;
      case 1:
        _f = _mm256_cvtss_f32(_mm256_permute_ps(value, _MM_SHUFFLE(1, 1, 1, 1)));
        break;
      case 2:
        _f = _mm256_cvtss_f32(_mm256_permute_ps(value, _MM_SHUFFLE(2, 2, 2, 2)));
        break;
      case 3:
        _f = _mm256_cvtss_f32(_mm256_permute_ps(value, _MM_SHUFFLE(3, 3, 3, 3)));
        break;
      }
      return _f;
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      double _d;
      if ( a == 0 ) {
        d128 __lo = _mm256_castpd256_pd128(value);
        _d = _mm_cvtsd_f64(__lo);
      } else if ( a == 1 ) {
        d128 __lo = _mm256_castpd256_pd128(value);
        _d = _mm_cvtsd_f64(_mm_unpackhi_pd(__lo, __lo));
        ;
      } else if ( a == 2 ) {
        d128 __hi = _mm256_extractf128_pd(value, 1);
        _d = _mm_cvtsd_f64(__hi);
      } else if ( a == 3 ) {
        d128 __hi = _mm256_extractf128_pd(value, 1);
        _d = _mm_cvtsd_f64(_mm_unpackhi_pd(__hi, __hi));
      }
      return _d;
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_64_wide<F>() ) {
        i64 _d = 0;
        switch ( a ) {
        case 0:
          _d = _mm256_extract_epi64(value, 0);
          break;
        case 1:
          _d = _mm256_extract_epi64(value, 1);
          break;
        case 2:
          _d = _mm256_extract_epi64(value, 2);
          break;
        case 3:
          _d = _mm256_extract_epi64(value, 3);
          break;
        }
        return _d;
      }
      if constexpr ( __is_32_wide<F>() ) {
        i32 _d = 0;
        switch ( a ) {
        case 0:
          _d = _mm256_extract_epi32(value, 0);
          break;
        case 1:
          _d = _mm256_extract_epi32(value, 1);
          break;
        case 2:
          _d = _mm256_extract_epi32(value, 2);
          break;
        case 3:
          _d = _mm256_extract_epi32(value, 3);
          break;
        case 4:
          _d = _mm256_extract_epi32(value, 4);
          break;
        case 5:
          _d = _mm256_extract_epi32(value, 5);
          break;
        case 6:
          _d = _mm256_extract_epi32(value, 6);
          break;
        case 7:
          _d = _mm256_extract_epi32(value, 7);
          break;
        }
        return _d;
      }
      if constexpr ( __is_16_wide<F>() ) {
        i16 _d = 0;
        switch ( a ) {
        case 0:
          _d = _mm256_extract_epi16(value, 0);
          break;
        case 1:
          _d = _mm256_extract_epi16(value, 1);
          break;
        case 2:
          _d = _mm256_extract_epi16(value, 2);
          break;
        case 3:
          _d = _mm256_extract_epi16(value, 3);
          break;
        case 4:
          _d = _mm256_extract_epi16(value, 4);
          break;
        case 5:
          _d = _mm256_extract_epi16(value, 5);
          break;
        case 6:
          _d = _mm256_extract_epi16(value, 6);
          break;
        case 7:
          _d = _mm256_extract_epi16(value, 7);
          break;
        case 8:
          _d = _mm256_extract_epi16(value, 8);
          break;
        case 9:
          _d = _mm256_extract_epi16(value, 9);
          break;
        case 10:
          _d = _mm256_extract_epi16(value, 10);
          break;
        case 11:
          _d = _mm256_extract_epi16(value, 11);
          break;
        case 12:
          _d = _mm256_extract_epi16(value, 12);
          break;
        case 13:
          _d = _mm256_extract_epi16(value, 13);
          break;
        case 14:
          _d = _mm256_extract_epi16(value, 14);
          break;
        case 15:
          _d = _mm256_extract_epi16(value, 15);
          break;
        }
        return _d;
      }
      if constexpr ( __is_8_wide<F>() ) {
        i8 _d = 0;
        switch ( a ) {
        case 0:
          _d = _mm256_extract_epi8(value, 0);
          break;
        case 1:
          _d = _mm256_extract_epi8(value, 1);
          break;
        case 2:
          _d = _mm256_extract_epi8(value, 2);
          break;
        case 3:
          _d = _mm256_extract_epi8(value, 3);
          break;
        case 4:
          _d = _mm256_extract_epi8(value, 4);
          break;
        case 5:
          _d = _mm256_extract_epi8(value, 5);
          break;
        case 6:
          _d = _mm256_extract_epi8(value, 6);
          break;
        case 7:
          _d = _mm256_extract_epi8(value, 7);
          break;
        case 8:
          _d = _mm256_extract_epi8(value, 8);
          break;
        case 9:
          _d = _mm256_extract_epi8(value, 9);
          break;
        case 10:
          _d = _mm256_extract_epi8(value, 10);
          break;
        case 11:
          _d = _mm256_extract_epi8(value, 11);
          break;
        case 12:
          _d = _mm256_extract_epi8(value, 12);
          break;
        case 13:
          _d = _mm256_extract_epi8(value, 13);
          break;
        case 14:
          _d = _mm256_extract_epi8(value, 14);
          break;
        case 15:
          _d = _mm256_extract_epi8(value, 15);
          break;
        case 16:
          _d = _mm256_extract_epi8(value, 16);
          break;
        case 17:
          _d = _mm256_extract_epi8(value, 17);
          break;
        case 18:
          _d = _mm256_extract_epi8(value, 18);
          break;
        case 19:
          _d = _mm256_extract_epi8(value, 19);
          break;
        case 20:
          _d = _mm256_extract_epi8(value, 20);
          break;
        case 21:
          _d = _mm256_extract_epi8(value, 21);
          break;
        case 22:
          _d = _mm256_extract_epi8(value, 22);
          break;
        case 23:
          _d = _mm256_extract_epi8(value, 23);
          break;
        case 24:
          _d = _mm256_extract_epi8(value, 24);
          break;
        case 25:
          _d = _mm256_extract_epi8(value, 25);
          break;
        case 26:
          _d = _mm256_extract_epi8(value, 26);
          break;
        case 27:
          _d = _mm256_extract_epi8(value, 27);
          break;
        case 28:
          _d = _mm256_extract_epi8(value, 28);
          break;
        case 29:
          _d = _mm256_extract_epi8(value, 29);
          break;
        case 30:
          _d = _mm256_extract_epi8(value, 30);
          break;
        case 31:
          _d = _mm256_extract_epi8(value, 31);
          break;
        }
        return _d;
      }
    }
  }
  constexpr int
  operator==(const v256 &o) const
  {
    {
      if constexpr ( micron::is_same_v<T, f256> ) {
        T _r = _mm256_cmpeq_ps(value, o.value);
        return _mm256_movemask_ps(_r);
      } else if constexpr ( micron::is_same_v<T, d256> ) {
        T _r = _mm256_cmpeq_pd(value, o.value);
        return _mm256_movemask_pd(_r);
      } else if constexpr ( micron::is_same_v<T, i256> ) {
        T _r;
        if constexpr ( __is_8_wide<F>() ) {
          _r = _mm256_cmpeq_epi8(value, o.value);
        }
        if constexpr ( __is_16_wide<F>() ) {
          _r = _mm256_cmpeq_epi16(value, o.value);
        }
        if constexpr ( __is_32_wide<F>() ) {
          _r = _mm256_cmpeq_epi32(value, o.value);
        }
        if constexpr ( __is_64_wide<F>() ) {
          _r = _mm256_cmpeq_epi64(value, o.value);
        }
        return _mm256_movemask_ps(_mm256_castsi256_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator>=(const v256 &o) const
  {
    {
      if constexpr ( micron::is_same_v<T, f256> ) {
        T _r = _mm256_cmpge_ps(value, o.value);
        return _mm256_movemask_ps(_r);
      } else if constexpr ( micron::is_same_v<T, d256> ) {
        T _r = _mm256_cmpge_pd(value, o.value);
        return _mm256_movemask_pd(_r);
      } else if constexpr ( micron::is_same_v<T, i256> ) {
        T _r;
        if constexpr ( __is_8_wide<F>() ) {
          _r = _mm256_cmpge_epi8(value, o.value);
        }
        if constexpr ( __is_16_wide<F>() ) {
          _r = _mm256_cmpge_epi16(value, o.value);
        }
        if constexpr ( __is_32_wide<F>() ) {
          _r = _mm256_cmpge_epi32(value, o.value);
        }
        if constexpr ( __is_64_wide<F>() ) {
          _r = _mm256_cmpge_epi64(value, o.value);
        }
        return _mm256_movemask_ps(_mm256_castsi256_ps(_r));
      }
    }
    return 0x0;
  }

  constexpr int
  operator>(const v256 &o) const
  {
    {
      if constexpr ( micron::is_same_v<T, f256> ) {
        T _r = _mm256_cmpgt_ps(value, o.value);
        return _mm256_movemask_ps(_r);
      } else if constexpr ( micron::is_same_v<T, d256> ) {
        T _r = _mm256_cmpgt_pd(value, o.value);
        return _mm256_movemask_pd(_r);
      } else if constexpr ( micron::is_same_v<T, i256> ) {
        T _r;
        if constexpr ( __is_8_wide<F>() ) {
          _r = _mm256_cmpgt_epi8(value, o.value);
        }
        if constexpr ( __is_16_wide<F>() ) {
          _r = _mm256_cmpgt_epi16(value, o.value);
        }
        if constexpr ( __is_32_wide<F>() ) {
          _r = _mm256_cmpgt_epi32(value, o.value);
        }
        if constexpr ( __is_64_wide<F>() ) {
          _r = _mm256_cmpgt_epi64(value, o.value);
        }
        return _mm256_movemask_ps(_mm256_castsi256_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator<(const v256 &o) const
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      T _r = _mm256_cmplt_ps(value, o.value);
      return _mm256_movemask_ps(_r);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      T _r = _mm256_cmplt_pd(value, o.value);
      return _mm256_movemask_pd(_r);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      T _r;
      if constexpr ( __is_8_wide<F>() ) {
        _r = _mm256_cmplt_epi8(value, o.value);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_cmplt_epi16(value, o.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_cmplt_epi32(value, o.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        _r = _mm256_cmplt_epi64(value, o.value);
      }
      return _mm256_movemask_ps(_mm256_castsi256_ps(_r));
    }
    return 0x0;
  }
  constexpr int
  operator<=(const v256 &o) const
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      T _r = _mm256_cmple_ps(value, o.value);
      return _mm256_movemask_ps(_r);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      T _r = _mm256_cmple_pd(value, o.value);
      return _mm256_movemask_pd(_r);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      T _r;
      if constexpr ( __is_8_wide<F>() ) {
        _r = _mm256_cmple_epi8(value, o.value);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_cmple_epi16(value, o.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_cmple_epi32(value, o.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        _r = _mm256_cmple_epi64(value, o.value);
      }
      return _mm256_movemask_ps(_mm256_castsi256_ps(_r));
    }
    return 0x0;
  }
  // scalar const. adds
  constexpr inline v256 &
  operator+=(double x)
  {
    if constexpr ( micron::is_same_v<T, d256> ) {
      d256 _r = _mm256_set1_pd(x);
      value = _mm256_add_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v256 &
  operator+=(float x)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      value = _mm256_add_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v256 &
  operator+=(A __x)
    requires is_int_flag_type<A>
  {
    F x = static_cast<F>(__x);
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        i256 _r = _mm256_set1_epi8(x);
        value = _mm256_add_epi8(value, _r);
      }
      if constexpr ( __is_16_wide<F>() ) {
        i256 _r = _mm256_set1_epi16(x);
        value = _mm256_add_epi16(value, _r);
      }
      if constexpr ( __is_32_wide<F>() ) {
        i256 _r = _mm256_set1_epi32(x);
        value = _mm256_add_epi32(value, _r);
      }
      if constexpr ( __is_64_wide<F>() ) {
        i256 _r = _mm256_set1_epi64x(x);
        value = _mm256_add_epi64(value, _r);
      }
    }
    return *this;
  }
  constexpr inline v256 &
  operator+=(const v256 &o)
  {
    T _r;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_add_ps(value, o.value);
      value = _mm256_add_ps(value, _r);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_add_pd(value, o.value);
      value = _mm256_add_pd(value, _r);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        _r = _mm256_add_epi8(value, o.value);
        value = _mm256_add_epi8(value, _r);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_add_epi16(value, o.value);
        value = _mm256_add_epi16(value, _r);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_add_epi32(value, o.value);
        value = _mm256_add_epi32(value, _r);
      }
      if constexpr ( __is_64_wide<F>() ) {
        _r = _mm256_add_epi64(value, o.value);
        value = _mm256_add_epi64(value, _r);
      }
    }
    return *this;
  }
  constexpr inline v256 &
  operator*=(const v256 &o)
  {
    T _r;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_mul_ps(value, o.value);
      value = _r;
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_mul_pd(value, o.value);
      value = _r;
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "AVX2 has no epi8 multiply");
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_mullo_epi16(value, o.value);
        value = _r;
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_mullo_epi32(value, o.value);
        value = _r;
      }
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "AVX2 has no epi64 multiply");
      }
    }
    return *this;
  }
  constexpr inline v256
  operator*(v256 x)
  {
    v256 _d;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _d.value = _mm256_mul_ps(value, x.value);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _d.value = _mm256_mul_pd(value, x.value);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        static_assert(!__is_8_wide<F>(), "AVX2 has no epi8 multiply");
      }
      if constexpr ( __is_16_wide<F>() ) {
        _d.value = _mm256_mullo_epi16(value, x.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _d.value = _mm256_mullo_epi32(value, x.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "AVX2 has no epi64 multiply");
      }
    }
    return _d;
  }
  // end
  // scalar const subs

  constexpr inline v256 &
  operator-=(double x)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      d256 _r = _mm256_set1_pd(x);
      value = _mm256_sub_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v256 &
  operator-=(float x)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      value = _mm256_sub_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v256 &
  operator-=(A x)
    requires is_int_flag_type<A>
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( micron::is_same_v<A, __v8> or micron::is_same_v<A, __uv8> ) {
        i256 _r = _mm256_set1_epi8(x);
        value = _mm256_sub_epi8(value, _r);
      }
      if constexpr ( micron::is_same_v<A, __v16> or micron::is_same_v<A, __uv16> ) {
        i256 _r = _mm256_set1_epi16(x);
        value = _mm256_sub_epi16(value, _r);
      }
      if constexpr ( micron::is_same_v<A, __v32> or micron::is_same_v<A, __uv32> ) {
        i256 _r = _mm256_set1_epi32(x);
        value = _mm256_sub_epi32(value, _r);
      }
      if constexpr ( micron::is_same_v<A, __v64> or micron::is_same_v<A, __uv64> ) {
        i256 _r = _mm256_set1_epi64x(x);
        value = _mm256_sub_epi64(value, _r);
      }
    }
    return *this;
  }
  // end
  constexpr inline v256 &
  operator-=(const v256 &o)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      value = _mm256_sub_ps(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      value = _mm256_sub_pd(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        value = _mm256_sub_epi8(value, o.value);
      }
      if constexpr ( __is_16_wide<F>() ) {
        value = _mm256_sub_epi16(value, o.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        value = _mm256_sub_epi32(value, o.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        value = _mm256_sub_epi64(value, o.value);
      }
    }
    return *this;
  }

  constexpr inline v256
  operator+(const v256 &o) const
  {
    v256 _r;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r.value = _mm256_add_ps(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _r.value = _mm256_add_pd(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        _r.value = _mm256_add_epi8(value, o.value);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r.value = _mm256_add_epi16(value, o.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r.value = _mm256_add_epi32(value, o.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        _r.value = _mm256_add_epi64(value, o.value);
      }
    }
    return _r;
  }
  constexpr inline v256
  operator-(const v256 &o) const
  {
    v256 _r;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r.value = _mm256_sub_ps(value, o.value);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _r.value = _mm256_sub_pd(value, o.value);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        _r.value = _mm256_sub_epi8(value, o.value);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r.value = _mm256_sub_epi16(value, o.value);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r.value = _mm256_sub_epi32(value, o.value);
      }
      if constexpr ( __is_64_wide<F>() ) {
        _r.value = _mm256_sub_epi64(value, o.value);
      }
    }
    return _r;
  }
  constexpr bool
  all_zeroes() const
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      return (_mm256_movemask_ps(value) == 0x0);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      return (_mm256_movemask_pd(value) == 0x0);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      return (_mm256_test_all_zeros(value, value) != 0);
    }
  }
  constexpr bool
  all_ones() const
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      return (_mm256_movemask_ps(value) == 0b1111);
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      return (_mm256_movemask_pd(value) == 0b11);
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      return (_mm256_test_all_ones(value) != 0);
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
  // broadcast /set() analog.
  inline v256 &
  set(float f)
  {
    value = _mm256_set1_ps(f);
    return *this;
  }
  inline v256 &
  set(double d)
  {
    value = _mm256_set1_pd(d);
    return *this;
  }
  inline v256 &
  set(i8 c)
  {
    value = _mm256_set1_epi8(c);
    return *this;
  }
  inline v256 &
  set(i16 s)
  {
    value = _mm256_set1_epi16(s);
    return *this;
  }
  inline v256 &
  set(i32 i)
  {
    value = _mm256_set1_epi32(i);
    return *this;
  }
  inline v256 &
  set(i64 l)
  {
    value = _mm256_set1_epi64x(l);
    return *this;
  }
  inline v256 &
  set(float const *f)
  {
    value = _mm256_broadcast_ss(f);
    return *this;
  }
  inline v256 &
  broadcast(float const *f)
  {
    return set(f);
  }

  template <typename B>
  inline v256 &
  load(B *mem)
  {
    if ( !is_aligned<256>(mem) )
      return *this;     // silent fail
    value = load<T>(mem);
    return *this;
  }

  template <typename B>
  inline v256 &
  uload(B *mem)
  {
    value = loadu<T, F>(reinterpret_cast<F *>(mem));
    return *this;
  }
  // BOOLEANS start
  inline v256 &
  operator|=(const v256 &o)
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      value = _mm256_or_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      value = _mm256_or_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      value = _mm256_or_pd(value, o.value);
    }
    return *this;
  }
  inline v256 &
  operator&=(const v256 &o)
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      value = _mm256_and_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      value = _mm256_and_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      value = _mm256_and_pd(value, o.value);
    }
    return *this;
  }
  inline v256 &
  operator^=(const v256 &o)
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      value = _mm256_xor_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      value = _mm256_xor_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      value = _mm256_xor_pd(value, o.value);
    }
    return *this;
  }
  inline T
  operator|(const v256 &o) const
  {
    T _r;
    if constexpr ( micron::is_same_v<T, i256> ) {
      _r = _mm256_or_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_or_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_or_pd(value, o.value);
    }
    return _r;
  }
  inline T
  operator&(const v256 &o) const
  {
    T _r;
    if constexpr ( micron::is_same_v<T, i256> ) {
      _r = _mm256_and_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_and_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_and_pd(value, o.value);
    }
    return _r;
  }
  inline T
  operator^(const v256 &o) const
  {
    T _r;
    if constexpr ( micron::is_same_v<T, i256> ) {
      _r = _mm256_xor_si256(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_xor_ps(value, o.value);
    }
    if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_xor_pd(value, o.value);
    }
    return _r;
  }

  inline T
  operator<<(int i) const
  {
    T _r;
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        _r = _mm256_srai_epi8(value, i);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_srai_epi16(value, i);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_srai_epi32(value, i);
      }
    }
    return _r;
  }

  inline v256 &
  operator<<=(int i)
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        value = _mm256_srai_epi8(value, i);
      }
      if constexpr ( __is_16_wide<F>() ) {
        value = _mm256_srai_epi16(value, i);
      }
      if constexpr ( __is_32_wide<F>() ) {
        value = _mm256_srai_epi32(value, i);
      }
    }
    return *this;
  }

  inline T
  operator>>(int i) const
  {
    T _r;
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        _r = _mm256_slai_epi8(value, i);
      }
      if constexpr ( __is_16_wide<F>() ) {
        _r = _mm256_slai_epi16(value, i);
      }
      if constexpr ( __is_32_wide<F>() ) {
        _r = _mm256_slai_epi32(value, i);
      }
    }
    return _r;
  }

  inline v256 &
  operator>>=(int i)
  {
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        value = _mm256_slai_epi8(value, i);
      }
      if constexpr ( __is_16_wide<F>() ) {
        value = _mm256_slai_epi16(value, i);
      }
      if constexpr ( __is_32_wide<F>() ) {
        value = _mm256_slai_epi32(value, i);
      }
    }
    return *this;
  }
  // BOOLEANS end
  // divs (f only) and muls

  constexpr inline v256 &
  operator/=(double x)
  {
    if constexpr ( micron::is_same_v<T, d256> ) {
      d256 _r = _mm256_set1_pd(x);
      value = _mm256_div_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v256 &
  operator/=(float x)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      value = _mm256_div_ps(value, _r);
    }
    return *this;
  }

  constexpr inline T
  operator/(double x) const
  {
    T _d;
    if constexpr ( micron::is_same_v<T, d256> ) {
      d256 _r = _mm256_set1_pd(x);
      _d = _mm256_div_pd(value, _r);
    }
    return _d;
  }
  constexpr inline T
  operator/(float x) const
  {
    T _d;
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      _d = _mm256_div_ps(value, _r);
    }
    return _d;
  }
  constexpr inline v256 &
  operator*=(double x)
  {
    if constexpr ( micron::is_same_v<T, d256> ) {
      d256 _r = _mm256_set1_pd(x);
      value = _mm256_mul_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v256 &
  operator*=(float x)
  {
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      value = _mm256_mul_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v256 &
  operator*=(A __x)
    requires is_int_flag_type<A>
  {
    F x = static_cast<F>(__x);
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        i256 _r = _mm256_set1_epi8(x);
        value = _mm256_mullo_epi8(value, _r);
      }
      if constexpr ( __is_16_wide<F>() ) {
        i256 _r = _mm256_set1_epi16(x);
        value = _mm256_mullo_epi16(value, _r);
      }
      if constexpr ( __is_32_wide<F>() ) {
        i256 _r = _mm256_set1_epi32(x);
        value = _mm256_mullo_epi32(value, _r);
      }
      if constexpr ( __is_64_wide<F>() ) {
        i256 _r = _mm256_set1_epi64(x);
        value = _mm256_mullo_epi64(value, _r);
      }
    }
    return *this;
  }

  constexpr inline v256 &
  operator/=(const v256 &o)
  {
    T _r;
    if constexpr ( micron::is_same_v<T, f256> ) {
      _r = _mm256_div_ps(value, o.value);
      value = _r;
    } else if constexpr ( micron::is_same_v<T, d256> ) {
      _r = _mm256_div_pd(value, o.value);
      value = _r;
    } else if constexpr ( micron::is_same_v<T, i256> ) {
      static_assert(!micron::is_same_v<T, i256>, "No SIMD integer division for __m256i (AVX2)");
    }
    return *this;
  }
  constexpr inline v256
  operator*(double x) const
  {
    v256 _d;
    if constexpr ( micron::is_same_v<T, d256> ) {
      d256 _r = _mm256_set1_pd(x);
      _d.value = _mm256_mul_pd(value, _r);
    }
    return _d;
  }
  constexpr inline v256
  operator*(float x) const
  {
    v256 _d;
    if constexpr ( micron::is_same_v<T, f256> ) {
      f256 _r = _mm256_set1_ps(x);
      _d.value = _mm256_mul_ps(value, _r);
    }
    return _d;
  }
  constexpr inline v256
  operator*(F x)
  {
    v256 _d;
    if constexpr ( micron::is_same_v<T, i256> ) {
      if constexpr ( __is_8_wide<F>() ) {
        i256 _r = _mm256_set1_epi8(x);
        _d.value = _mm256_mullo_epi8(value, _r);
      }
      if constexpr ( __is_16_wide<F>() ) {
        i256 _r = _mm256_set1_epi16(x);
        _d.value = _mm256_mullo_epi16(value, _r);
      }
      if constexpr ( __is_32_wide<F>() ) {
        i256 _r = _mm256_set1_epi32(x);
        _d.value = _mm256_mullo_epi32(value, _r);
      }
      if constexpr ( __is_64_wide<F>() ) {
        static_assert(!__is_64_wide<F>(), "AVX2 has no epi64 multiply");
      }
    }
    return _d;
  }
  // end divs and muls

  inline void
  get(F *arr) const
  {
    if constexpr ( micron::is_same_v<F, __vf> ) {
      _mm256_storeu_ps(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( micron::is_same_v<F, __vd> ) {
      _mm256_storeu_pd(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( __is_8_wide<F>() or __is_16_wide<F>() or __is_32_wide<F>() or __is_64_wide<F>() ) {
      _mm256_storeu_si256(reinterpret_cast<T *>(arr), value);
    }
  }
  inline void
  get_aligned(F *arr) const
  {
    if constexpr ( micron::is_same_v<F, __vf> ) {
      _mm256_store_ps(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( micron::is_same_v<F, __vd> ) {
      _mm256_store_pd(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( __is_8_wide<F>() or __is_16_wide<F>() or __is_32_wide<F>() or __is_64_wide<F>() ) {
      _mm256_store_si256(reinterpret_cast<T *>(arr), value);
    }
  }
};

};
};
