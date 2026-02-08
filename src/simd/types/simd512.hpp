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
template <is_simd_512_type T, is_flag_type F> class v512
{
  T value;

  inline void
  __impl_zero_init(void)
  {
    if constexpr ( stdlib::same_as<T, f512> ) {
      value = _mm512_setzero_ps();
    }
    if constexpr ( stdlib::same_as<T, d512> ) {
      value = _mm512_setzero_pd();
    }
    if constexpr ( stdlib::same_as<T, i512> ) {
      value = _mm512_setzero_si512();
    }
  }

  inline void
  __impl_one_init(void)
  {
    if constexpr ( stdlib::same_as<T, f512> ) {
      value = _mm512_castsi512_pd(_mm512_set1_epi32(-1));
    }
    if constexpr ( stdlib::same_as<T, d512> ) {
      value = _mm512_castsi512_ps(_mm512_set1_epi64(-1));
    }
    if constexpr ( stdlib::same_as<T, i512> ) {
      value = _mm512_set1_epi32(-1);
    }
  }

public:
  ~v512() = default;
  v512(void) { __impl_zero_init(); }

  inline v512 &
  operator=(std::initializer_list<double> lst)
  {
    if ( lst.size() != 4 )
      return *this;
    double a, b, c, d, e, f, g, h;
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
      if ( _f == 3 )
        e = *itr;
      if ( _f == 3 )
        f = *itr;
      if ( _f == 3 )
        g = *itr;
      if ( _f == 3 )
        h = *itr;
      _f++;
    }
    value = _mm512_set_pd(h, g, f, e, d, c, b, a);
    return *this;
  }
  inline v512 &
  operator=(std::initializer_list<float> lst)
  {
    if ( lst.size() != 16 )
      return *this;

    float __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm512_set_ps(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8], __arr[7],
                          __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  // start of ints
  inline v512 &
  operator=(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 8 )
      return *this;

    i64 __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_epi64(__arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  inline v512 &
  operator=(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 16 )
      return *this;
    i32 __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm512_set_epi32(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  inline v512 &
  operator=(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 32 )
      return *this;
    i16 __arr[32];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm512_set_epi16(__arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                             __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                             __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  inline v512 &
  operator=(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 64 )
      return *this;
    i8 __arr[64];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_epi8(__arr[63], __arr[62], __arr[61], __arr[60], __arr[59], __arr[58], __arr[57], __arr[56],
                            __arr[55], __arr[54], __arr[53], __arr[52], __arr[51], __arr[50], __arr[49], __arr[48],
                            __arr[47], __arr[46], __arr[45], __arr[44], __arr[43], __arr[42], __arr[41], __arr[40],
                            __arr[39], __arr[38], __arr[37], __arr[36], __arr[35], __arr[34], __arr[33], __arr[32],
                            __arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                            __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                            __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                            __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
    return *this;
  }
  // end of ints
  v512(i8 _e0)
    requires stdlib::is_same_v<T, i512>
  {
    value = _mm512_set1_epi8(_e0);
  }
  v512(i16 _e0)
    requires stdlib::is_same_v<T, i512>
  {
    value = _mm512_set1_epi16(_e0);
  }
  v512(i32 a)
    requires stdlib::is_same_v<T, i512>
  {
    value = _mm512_set1_epi32(a);
  }
  v512(i64 a)
    requires stdlib::is_same_v<T, i512>
  {
    value = _mm512_set1_epi64(a);
  }
  v512(float a)
    requires stdlib::is_same_v<T, f512>
  {
    value = _mm512_set1_ps(a);
  }
  v512(double a)
    requires stdlib::is_same_v<T, d512>
  {
    value = _mm512_set1_pd(a);
  }
  v512(std::initializer_list<double> lst)
  {
    if ( lst.size() != 8 )
      return;
    double a, b, c, d, e, f, g, h;
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
    value = _mm512_set_pd(h, g, f, e, d, c, b, a);
  }
  v512(std::initializer_list<float> lst)
  {
    if ( lst.size() != 16 )
      return;
    float __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_ps(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8], __arr[7],
                          __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  // start of ints
  v512(std::initializer_list<i64> lst)
  {
    if ( lst.size() != 8 )
      return;
    float __arr[8];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_epi64(__arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  v512(std::initializer_list<i32> lst)
  {
    if ( lst.size() != 16 )
      return;
    float __arr[16];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_epi32(__arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  v512(std::initializer_list<i16> lst)
  {
    if ( lst.size() != 32 )
      return;
    float __arr[32];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;

    value = _mm512_set_epi16(__arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                             __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                             __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                             __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  v512(std::initializer_list<i8> lst)
  {
    if ( lst.size() != 64 )
      return;
    i8 __arr[64];

    int __i = 0;
    for ( auto itr = lst.begin(); itr != lst.end(); ++itr )
      __arr[__i++] = *itr;
    value = _mm512_set_epi8(__arr[63], __arr[62], __arr[61], __arr[60], __arr[59], __arr[58], __arr[57], __arr[56],
                            __arr[55], __arr[54], __arr[53], __arr[52], __arr[51], __arr[50], __arr[49], __arr[48],
                            __arr[47], __arr[46], __arr[45], __arr[44], __arr[43], __arr[42], __arr[41], __arr[40],
                            __arr[39], __arr[38], __arr[37], __arr[36], __arr[35], __arr[34], __arr[33], __arr[32],
                            __arr[31], __arr[30], __arr[29], __arr[28], __arr[27], __arr[26], __arr[25], __arr[24],
                            __arr[23], __arr[22], __arr[21], __arr[20], __arr[19], __arr[18], __arr[17], __arr[16],
                            __arr[15], __arr[14], __arr[13], __arr[12], __arr[11], __arr[10], __arr[9], __arr[8],
                            __arr[7], __arr[6], __arr[5], __arr[4], __arr[3], __arr[2], __arr[1], __arr[0]);
  }
  // end of ints

  v512(const v512 &o) : value(o.value) {}
  v512(v512 &&o) : value(o.value)
  {
    if constexpr ( stdlib::same_as<T, f512> ) {
      o.value = _mm512_setzero_ps();
    }
    if constexpr ( stdlib::same_as<T, d512> ) {
      o.value = _mm512_setzero_pd();
    }
    if constexpr ( stdlib::same_as<T, i512> ) {
      o.value = _mm512_setzero_si512();
    }
  }
  inline v512 &
  operator=(const v512 &o)
  {
    value = o.value;
    return *this;
  }
  inline v512 &
  operator=(v512 &&o)
  {
    value = o.value;
    return *this;
  }
  // broadcast /set() analog.
  inline v512 &
  operator=(float f)
  {
    value = _mm512_set1_ps(f);
    return *this;
  }
  inline v512 &
  operator=(double d)
  {
    value = _mm512_set1_pd(d);
    return *this;
  }
  inline v512 &
  operator=(i8 c)
  {
    value = _mm512_set1_epi8(c);
    return *this;
  }
  inline v512 &
  operator=(i16 s)
  {
    value = _mm512_set1_epi16(s);
    return *this;
  }
  inline v512 &
  operator=(i32 i)
  {
    value = _mm512_set1_epi32(i);
    return *this;
  }
  inline v512 &
  operator=(i64 l)
  {
    value = _mm512_set1_epi64(l);
    return *this;
  }
  inline v512 &
  operator=(F *addr)
  {
    return uload<F>(addr);
  }
  constexpr v512 &operator[](const int a) = delete;
  constexpr int
  operator==(const v512 &o) const
  {
    {
      if constexpr ( stdlib::is_same_v<T, f512> ) {
        T _r = _mm512_cmpeq_ps(value, o.value);
        return _mm512_movemask_ps(_r);
      } else if constexpr ( stdlib::is_same_v<T, d512> ) {
        T _r = _mm512_cmpeq_pd(value, o.value);
        return _mm512_movemask_pd(_r);
      } else if constexpr ( stdlib::is_same_v<T, i512> ) {
        T _r;
        if constexpr ( stdlib::is_same_v<F, __v8> ) {
          _r = _mm512_cmpeq_epi8(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v16> ) {
          _r = _mm512_cmpeq_epi16(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v32> ) {
          _r = _mm512_cmpeq_epi32(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v64> ) {
          _r = _mm512_cmpeq_epi64(value, o.value);
        }
        return _mm512_movemask_ps(_mm512_castsi512_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator>=(const v512 &o) const
  {
    {
      if constexpr ( stdlib::is_same_v<T, f512> ) {
        T _r = _mm512_cmpge_ps(value, o.value);
        return _mm512_movemask_ps(_r);
      } else if constexpr ( stdlib::is_same_v<T, d512> ) {
        T _r = _mm512_cmpge_pd(value, o.value);
        return _mm512_movemask_pd(_r);
      } else if constexpr ( stdlib::is_same_v<T, i512> ) {
        T _r;
        if constexpr ( stdlib::is_same_v<F, __v8> ) {
          _r = _mm512_cmpge_epi8(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v16> ) {
          _r = _mm512_cmpge_epi16(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v32> ) {
          _r = _mm512_cmpge_epi32(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v64> ) {
          _r = _mm512_cmpge_epi64(value, o.value);
        }
        return _mm512_movemask_ps(_mm512_castsi512_ps(_r));
      }
    }
    return 0x0;
  }

  constexpr int
  operator>(const v512 &o) const
  {
    {
      if constexpr ( stdlib::is_same_v<T, f512> ) {
        T _r = _mm512_cmpgt_ps(value, o.value);
        return _mm512_movemask_ps(_r);
      } else if constexpr ( stdlib::is_same_v<T, d512> ) {
        T _r = _mm512_cmpgt_pd(value, o.value);
        return _mm512_movemask_pd(_r);
      } else if constexpr ( stdlib::is_same_v<T, i512> ) {
        T _r;
        if constexpr ( stdlib::is_same_v<F, __v8> ) {
          _r = _mm512_cmpgt_epi8(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v16> ) {
          _r = _mm512_cmpgt_epi16(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v32> ) {
          _r = _mm512_cmpgt_epi32(value, o.value);
        }
        if constexpr ( stdlib::is_same_v<F, __v64> ) {
          _r = _mm512_cmpgt_epi64(value, o.value);
        }
        return _mm512_movemask_ps(_mm512_castsi512_ps(_r));
      }
    }
    return 0x0;
  }
  constexpr int
  operator<(const v512 &o) const
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      T _r = _mm512_cmplt_ps(value, o.value);
      return _mm512_movemask_ps(_r);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      T _r = _mm512_cmplt_pd(value, o.value);
      return _mm512_movemask_pd(_r);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      T _r;
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_cmplt_epi8(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_cmplt_epi16(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_cmplt_epi32(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
        _r = _mm512_cmplt_epi64(value, o.value);
      }
      return _mm512_movemask_ps(_mm512_castsi512_ps(_r));
    }
    return 0x0;
  }
  constexpr int
  operator<=(const v512 &o) const
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      T _r = _mm512_cmple_ps(value, o.value);
      return _mm512_movemask_ps(_r);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      T _r = _mm512_cmple_pd(value, o.value);
      return _mm512_movemask_pd(_r);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      T _r;
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_cmple_epi8(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_cmple_epi16(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_cmple_epi32(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
        _r = _mm512_cmple_epi64(value, o.value);
      }
      return _mm512_movemask_ps(_mm512_castsi512_ps(_r));
    }
    return 0x0;
  }
  // scalar const. adds
  constexpr inline v512 &
  operator+=(double x)
  {
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      d512 _r = _mm512_set1_pd(x);
      value = _mm512_add_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v512 &
  operator+=(float x)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      value = _mm512_add_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v512 &
  operator+=(A x)
    requires is_int_flag_type<A>
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<A, __v8> ) {
        i512 _r = _mm512_set1_epi8(x);
        value = _mm512_add_epi8(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v16> ) {
        i512 _r = _mm512_set1_epi16(x);
        value = _mm512_add_epi16(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v32> ) {
        i512 _r = _mm512_set1_epi32(x);
        value = _mm512_add_epi32(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v64> ) {
        i512 _r = _mm512_set1_epi64(x);
        value = _mm512_add_epi64(value, _r);
      }
    }
    return *this;
  }
  // end
  // scalar const subs

  constexpr inline v512 &
  operator-=(double x)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      d512 _r = _mm512_set1_pd(x);
      value = _mm512_sub_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v512 &
  operator-=(float x)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      value = _mm512_sub_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v512 &
  operator-=(A x)
    requires is_int_flag_type<A>
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<A, __v8> ) {
        i512 _r = _mm512_set1_epi8(x);
        value = _mm512_sub_epi8(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v16> ) {
        i512 _r = _mm512_set1_epi16(x);
        value = _mm512_sub_epi16(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v32> ) {
        i512 _r = _mm512_set1_epi32(x);
        value = _mm512_sub_epi32(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v64> ) {
        i512 _r = _mm512_set1_epi64(x);
        value = _mm512_sub_epi64(value, _r);
      }
    }
    return *this;
  }
  // end
  constexpr inline v512 &
  operator-=(const v512 &o)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      value = _mm512_sub_ps(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      value = _mm512_sub_pd(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        value = _mm512_sub_epi8(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        value = _mm512_sub_epi16(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        value = _mm512_sub_epi32(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
        value = _mm512_sub_epi64(value, o.value);
      }
    }
    return *this;
  }

  constexpr inline v512 &
  operator+(const v512 &o) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      _r = _mm512_add_ps(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      _r = _mm512_add_pd(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_add_epi8(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_add_epi16(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_add_epi32(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
        _r = _mm512_add_epi64(value, o.value);
      }
    }
    return _r;
  }
  constexpr inline v512 &
  operator-(const v512 &o) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      _r = _mm512_sub_ps(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      _r = _mm512_sub_pd(value, o.value);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_sub_epi8(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_sub_epi16(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_sub_epi32(value, o.value);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
        _r = _mm512_sub_epi64(value, o.value);
      }
    }
    return _r;
  }
  constexpr bool
  all_zeroes() const
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      return (_mm512_movemask_ps(value) == 0x0);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      return (_mm512_movemask_pd(value) == 0x0);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      return (_mm512_test_all_zeros(value, value) != 0);
    }
  }
  constexpr bool
  all_ones() const
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      return (_mm512_movemask_ps(value) == 0b1111);
    } else if constexpr ( stdlib::is_same_v<T, d512> ) {
      return (_mm512_movemask_pd(value) == 0b11);
    } else if constexpr ( stdlib::is_same_v<T, i512> ) {
      return (_mm512_test_all_ones(value) != 0);
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
  inline v512 &
  set(float f)
  {
    value = _mm512_set1_ps(f);
    return *this;
  }
  inline v512 &
  set(double d)
  {
    value = _mm512_set1_pd(d);
    return *this;
  }
  inline v512 &
  set(i8 c)
  {
    value = _mm512_set1_epi8(c);
    return *this;
  }
  inline v512 &
  set(i16 s)
  {
    value = _mm512_set1_epi16(s);
    return *this;
  }
  inline v512 &
  set(i32 i)
  {
    value = _mm512_set1_epi32(i);
    return *this;
  }
  inline v512 &
  set(i64 l)
  {
    value = _mm512_set1_epi64(l);
    return *this;
  }
  inline v512 &
  set(float const *f)
  {
    f128 t = _mm_load_ss(f);
    value = _mm512_broadcastss_ps(t);     // why :c
    return *this;
  }
  inline v512 &
  broadcast(float const *f)
  {
    return set(f);
  }

  template <typename B>
  inline v512 &
  load(B *mem)
  {
    if ( !is_aligned<256>(mem) )
      return *this;     // silent fail
    value = load<T>(mem);
    return *this;
  }

  template <typename B>
  inline v512 &
  uload(B *mem)
  {
    value = loadu<T, F>(reinterpret_cast<F *>(mem));
    return *this;
  }
  // BOOLEANS start
  inline v512 &
  operator|=(const v512 &o)
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      value = _mm512_or_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      value = _mm512_or_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      value = _mm512_or_pd(value, o.value);
    }
    return *this;
  }
  inline v512 &
  operator&=(const v512 &o)
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      value = _mm512_and_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      value = _mm512_and_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      value = _mm512_and_pd(value, o.value);
    }
    return *this;
  }
  inline v512 &
  operator^=(const v512 &o)
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      value = _mm512_xor_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      value = _mm512_xor_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      value = _mm512_xor_pd(value, o.value);
    }
    return *this;
  }
  inline T
  operator|(const v512 &o) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      _r = _mm512_or_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      _r = _mm512_or_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      _r = _mm512_or_pd(value, o.value);
    }
    return _r;
  }
  inline T
  operator&(const v512 &o) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      _r = _mm512_and_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      _r = _mm512_and_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      _r = _mm512_and_pd(value, o.value);
    }
    return _r;
  }
  inline T
  operator^(const v512 &o) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      _r = _mm512_xor_si512(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      _r = _mm512_xor_ps(value, o.value);
    }
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      _r = _mm512_xor_pd(value, o.value);
    }
    return _r;
  }

  inline T
  operator<<(int i) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_srai_epi8(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_srai_epi16(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_srai_epi32(value, i);
      }
    }
    return _r;
  }

  inline v512 &
  operator<<=(int i)
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        value = _mm512_srai_epi8(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        value = _mm512_srai_epi16(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        value = _mm512_srai_epi32(value, i);
      }
    }
    return *this;
  }

  inline T
  operator>>(int i) const
  {
    T _r;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        _r = _mm512_slai_epi8(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        _r = _mm512_slai_epi16(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        _r = _mm512_slai_epi32(value, i);
      }
    }
    return _r;
  }

  inline v512 &
  operator>>=(int i)
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        value = _mm512_slai_epi8(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        value = _mm512_slai_epi16(value, i);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        value = _mm512_slai_epi32(value, i);
      }
    }
    return *this;
  }
  // BOOLEANS end
  // divs (f only) and muls

  constexpr inline v512 &
  operator/=(double x)
  {
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      d512 _r = _mm512_set1_pd(x);
      value = _mm512_div_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v512 &
  operator/=(float x)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      value = _mm512_div_ps(value, _r);
    }
    return *this;
  }

  constexpr inline T
  operator/(double x) const
  {
    T _d;
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      d512 _r = _mm512_set1_pd(x);
      _d = _mm512_div_pd(value, _r);
    }
    return _d;
  }
  constexpr inline T
  operator/(float x) const
  {
    T _d;
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      _d = _mm512_div_ps(value, _r);
    }
    return _d;
  }
  constexpr inline v512 &
  operator*=(double x)
  {
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      d512 _r = _mm512_set1_pd(x);
      value = _mm512_mul_pd(value, _r);
    }
    return *this;
  }
  constexpr inline v512 &
  operator*=(float x)
  {
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      value = _mm512_mul_ps(value, _r);
    }
    return *this;
  }
  template <typename A>
  constexpr inline v512 &
  operator*=(A x)
    requires is_int_flag_type<A>
  {
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<A, __v8> ) {
        i512 _r = _mm512_set1_epi8(x);
        value = _mm512_mullo_epi8(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v16> ) {
        i512 _r = _mm512_set1_epi16(x);
        value = _mm512_mullo_epi16(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v32> ) {
        i512 _r = _mm512_set1_epi32(x);
        value = _mm512_mullo_epi32(value, _r);
      }
      if constexpr ( stdlib::is_same_v<A, __v64> ) {
      }
    }
    return *this;
  }

  constexpr inline T
  operator*(double x) const
  {
    T _d;
    if constexpr ( stdlib::is_same_v<T, d512> ) {
      d512 _r = _mm512_set1_pd(x);
      _d = _mm512_mul_pd(value, _r);
    }
    return _d;
  }
  constexpr inline T
  operator*(float x) const
  {
    T _d;
    if constexpr ( stdlib::is_same_v<T, f512> ) {
      f512 _r = _mm512_set1_ps(x);
      _d = _mm512_mul_ps(value, _r);
    }
    return _d;
  }
  constexpr inline T
  operator*(F x)
  {
    T _d;
    if constexpr ( stdlib::is_same_v<T, i512> ) {
      if constexpr ( stdlib::is_same_v<F, __v8> ) {
        i512 _r = _mm512_set1_epi8(x);
        _d = _mm512_mullo_epi8(value, _r);
      }
      if constexpr ( stdlib::is_same_v<F, __v16> ) {
        i512 _r = _mm512_set1_epi16(x);
        _d = _mm512_mullo_epi16(value, _r);
      }
      if constexpr ( stdlib::is_same_v<F, __v32> ) {
        i512 _r = _mm512_set1_epi32(x);
        _d = _mm512_mullo_epi32(value, _r);
      }
      if constexpr ( stdlib::is_same_v<F, __v64> ) {
      }
    }
    return _d;
  }
  // end divs and muls

  inline void
  get(F *arr) const
  {
    if constexpr ( stdlib::is_same_v<F, __vf> ) {
      _mm512_storeu_ps(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( stdlib::is_same_v<F, __vd> ) {
      _mm512_storeu_pd(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( stdlib::is_same_v<F, __v8> or stdlib::is_same_v<F, __v16> or stdlib::is_same_v<F, __v32>
                   or stdlib::is_same_v<F, __v64> ) {
      _mm512_storeu_si512(reinterpret_cast<T *>(arr), value);
    }
  }
  inline void
  get_aligned(F *arr) const
  {
    if constexpr ( stdlib::is_same_v<F, __vf> ) {
      _mm512_store_ps(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( stdlib::is_same_v<F, __vd> ) {
      _mm512_store_pd(reinterpret_cast<T *>(arr), value);
    }
    if constexpr ( stdlib::is_same_v<F, __v8> or stdlib::is_same_v<F, __v16> or stdlib::is_same_v<F, __v32>
                   or stdlib::is_same_v<F, __v64> ) {
      _mm512_store_si512(reinterpret_cast<T *>(arr), value);
    }
  }
};

};
};
