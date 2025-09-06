//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"
#include <initializer_list>

namespace micron
{
namespace simd
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
template <is_simd_128_type T> class v128
{
  T value;

  inline void
  __impl_zero_init(void)
  {
    if constexpr ( micron::same_as<T, f128> ) {
      value = _mm_setzero_ps();
    }
    if constexpr ( micron::same_as<T, d128> ) {
      value = _mm_setzero_pd();
    }
    if constexpr ( micron::same_as<T, i128> ) {
      value = _mm_setzero_si128();
    }
  }

public:
  ~v128() = default;
  v128(void) { __impl_zero_init(); }

  v128 &
  operator=(std::initializer_list<double> lst)
  {
    if ( lst.size() != 2 )
      return *this;
    float a, b;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    value = _mm_set_pd(a, b);
    return *this;
  }
  v128 &
  operator=(std::initializer_list<float> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    float a, b, c, d;
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
    value = _mm_set_ps(a, b, c, d);
    return *this;
  }
  v128 &
  operator=(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    int a, b, c, d;
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
    value = _mm_set_epi32(a, b, c, d);
    return *this;
  }

  v128(std::initializer_list<double> lst)
  {
    if ( lst.size() != 2 )
      return;
    float a, b;
    int _f = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr ) {
      if ( _f == 0 )
        a = *itr;
      if ( _f == 1 )
        b = *itr;
      _f++;
    }
    value = _mm_set_pd(a, b);
  }
  v128(std::initializer_list<float> lst)
  {
    if ( lst.size() != 4 )
      return;
    float a, b, c, d;
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
    value = _mm_set_ps(a, b, c, d);
  }
  v128(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 4 )
      return;
    int a, b, c, d;
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
    value = _mm_set_epi32(a, b, c, d);
  }
  v128(i32 a, i32 b, i32 c, i32 d)
    requires micron::is_same_v<T, i128>
  {
    value = _mm_set_epi32(a, b, c, d);
  }
  v128(float a, float b, float c, float d)
    requires micron::is_same_v<T, f128>
  {
    value = _mm_set_ps(a, b, c, d);
  }
  v128(double a, double b)
    requires micron::is_same_v<T, d128>
  {
    value = _mm_set_pd(a, b);
  }
  v128(T a, T b)
  {
    if constexpr ( micron::same_as<T, d128> ) {
      value = _mm_set_pd(a, b);
    }
  }
  v128(const v128 &o) : value(o.value) {}
  v128(v128 &&o) : value(o.value)
  {
    if constexpr ( micron::same_as<T, f128> ) {
      o.value = _mm_setzero_ps();
    }
    if constexpr ( micron::same_as<T, d128> ) {
      o.value = _mm_setzero_pd();
    }
    if constexpr ( micron::same_as<T, i128> ) {
      o.value = _mm_setzero_si128();
    }
  }
  v128 &
  operator=(const v128 &o)
  {
    value = o.value;
    return *this;
  }
  v128 &
  operator=(v128 &&o)
  {
    value = o.value;
    return *this;
  }

  constexpr auto
  operator[](const int a)
  {
    // guess the formatter doesn't like this :(
    if constexpr ( micron::is_same_v<T, f128> ) {
      float _f = _mm_cvtss_f32(_mm_shuffle_ps(value, value, _MM_SHUFFLE(a, a, a, a)));
      return _f;
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      double _d;
      if ( value == 0 )
        _d = _mm_cvtsd_f64(value);
      else
        _d = _mm_cvtsd_f64(_mm_unpackhi_pd(value, value));
      return _d;
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      i32 _d = 0;
      switch ( a ) {
      case 0:
        _d = _mm_extract_epi32(value, 0);
        break;
      case 1:
        _d = _mm_extract_epi32(value, 1);
        break;
      case 2:
        _d = _mm_extract_epi32(value, 2);
        break;
      case 3:
        _d = _mm_extract_epi32(value, 3);
        break;
      }
      return _d;
    }
  }
  constexpr int
  operator==(const v128 &o) const
  {
    {
      // guess the formatter doesn't like this :(
      if constexpr ( micron::is_same_v<T, f128> ) {
        T _r = _mm_cmpeq_ps(value, o.value);
        return _mm_movemask_ps(_r);
      } else if constexpr ( micron::is_same_v<T, d128> ) {
        T _r = _mm_cmpeq_pd(value, o.value);
        return _mm_movemask_pd(_r);
      } else if constexpr ( micron::is_same_v<T, i128> ) {
        T _r = _mm_cmpeq_epi32(value, o.value);
        return _mm_movemask_ps(_mm_castsi128_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator>(const v128 &o) const
  {
    {
      // guess the formatter doesn't like this :(
      if constexpr ( micron::is_same_v<T, f128> ) {
        T _r = _mm_cmpgt_ps(value, o.value);
        return _mm_movemask_ps(_r);
      } else if constexpr ( micron::is_same_v<T, d128> ) {
        T _r = _mm_cmpgt_pd(value, o.value);
        return _mm_movemask_pd(_r);
      } else if constexpr ( micron::is_same_v<T, i128> ) {
        T _r = _mm_cmpgt_epi32(value, o.value);
        return _mm_movemask_ps(_mm_castsi128_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator<(const v128 &o) const
  {
    // guess the formatter doesn't like this :(
    if constexpr ( micron::is_same_v<T, f128> ) {
      T _r = _mm_cmplt_ps(value, o.value);
      return _mm_movemask_ps(_r);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      T _r = _mm_cmplt_pd(value, o.value);
      return _mm_movemask_pd(_r);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      T _r = _mm_cmplt_epi32(value, o.value);
      return _mm_movemask_ps(_mm_castsi128_ps(_r));
    }
    return 0x0;
  }
  constexpr bool
  all_zeroes() const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return (_mm_movemask_ps(value) == 0x0);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return (_mm_movemask_pd(value) == 0x0);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      return (_mm_test_all_zeros(value, value) != 0);
    }
  }
  constexpr bool
  all_ones() const
  {
    if constexpr ( micron::is_same_v<T, f128> ) {
      return (_mm_movemask_ps(value) == 0b1111);
    } else if constexpr ( micron::is_same_v<T, d128> ) {
      return (_mm_movemask_pd(value) == 0b11);
    } else if constexpr ( micron::is_same_v<T, i128> ) {
      return (_mm_test_all_ones(value) != 0);
    }
  }
  constexpr void
  to_zero(void)
  {
    __impl_zero_init();
  }
};
#pragma GCC diagnostic pop

};

};
