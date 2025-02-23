#pragma once

// compiler provided

#if defined(__STDCPP_FLOAT16_T__) && defined(_GLIBCXX_FLOAT_IS_IEEE_BINARY32)
constexpr _Float16
acos(_Float16 __x)
{
  return _Float16(__builtin_acosf(__x));
}

constexpr _Float16
asin(_Float16 __x)
{
  return _Float16(__builtin_asinf(__x));
}

constexpr _Float16
atan(_Float16 __x)
{
  return _Float16(__builtin_atanf(__x));
}

constexpr _Float16
atan2(_Float16 __y, _Float16 __x)
{
  return _Float16(__builtin_atan2f(__y, __x));
}

constexpr _Float16
ceil(_Float16 __x)
{
  return _Float16(__builtin_ceilf(__x));
}

constexpr _Float16
cos(_Float16 __x)
{
  return _Float16(__builtin_cosf(__x));
}

constexpr _Float16
cosh(_Float16 __x)
{
  return _Float16(__builtin_coshf(__x));
}

constexpr _Float16
exp(_Float16 __x)
{
  return _Float16(__builtin_expf(__x));
}

constexpr _Float16
fabs(_Float16 __x)
{
  return _Float16(__builtin_fabsf(__x));
}

constexpr _Float16
floor(_Float16 __x)
{
  return _Float16(__builtin_floorf(__x));
}

constexpr _Float16
fmod(_Float16 __x, _Float16 __y)
{
  return _Float16(__builtin_fmodf(__x, __y));
}

inline _Float16
frexp(_Float16 __x, int *__exp)
{
  return _Float16(__builtin_frexpf(__x, __exp));
}

constexpr _Float16
ldexp(_Float16 __x, int __exp)
{
  return _Float16(__builtin_ldexpf(__x, __exp));
}

constexpr _Float16
log(_Float16 __x)
{
  return _Float16(__builtin_logf(__x));
}

constexpr _Float16
log10(_Float16 __x)
{
  return _Float16(__builtin_log10f(__x));
}

inline _Float16
modf(_Float16 __x, _Float16 *__iptr)
{
  float __i, __ret = __builtin_modff(__x, &__i);
  *__iptr = _Float16(__i);
  return _Float16(__ret);
}

constexpr _Float16
pow(_Float16 __x, _Float16 __y)
{
  return _Float16(__builtin_powf(__x, __y));
}

constexpr _Float16
sin(_Float16 __x)
{
  return _Float16(__builtin_sinf(__x));
}

constexpr _Float16
sinh(_Float16 __x)
{
  return _Float16(__builtin_sinhf(__x));
}

constexpr _Float16
sqrt(_Float16 __x)
{
  return _Float16(__builtin_sqrtf(__x));
}

constexpr _Float16
tan(_Float16 __x)
{
  return _Float16(__builtin_tanf(__x));
}

constexpr _Float16
tanh(_Float16 __x)
{
  return _Float16(__builtin_tanhf(__x));
}
#endif

#if defined(__STDCPP_FLOAT32_T__) && defined(_GLIBCXX_FLOAT_IS_IEEE_BINARY32)
constexpr _Float32
acos(_Float32 __x)
{
  return __builtin_acosf(__x);
}

constexpr _Float32
asin(_Float32 __x)
{
  return __builtin_asinf(__x);
}

constexpr _Float32
atan(_Float32 __x)
{
  return __builtin_atanf(__x);
}

constexpr _Float32
atan2(_Float32 __y, _Float32 __x)
{
  return __builtin_atan2f(__y, __x);
}

constexpr _Float32
ceil(_Float32 __x)
{
  return __builtin_ceilf(__x);
}

constexpr _Float32
cos(_Float32 __x)
{
  return __builtin_cosf(__x);
}

constexpr _Float32
cosh(_Float32 __x)
{
  return __builtin_coshf(__x);
}

constexpr _Float32
exp(_Float32 __x)
{
  return __builtin_expf(__x);
}

constexpr _Float32
fabs(_Float32 __x)
{
  return __builtin_fabsf(__x);
}

constexpr _Float32
floor(_Float32 __x)
{
  return __builtin_floorf(__x);
}

constexpr _Float32
fmod(_Float32 __x, _Float32 __y)
{
  return __builtin_fmodf(__x, __y);
}

inline _Float32
frexp(_Float32 __x, int *__exp)
{
  return __builtin_frexpf(__x, __exp);
}

constexpr _Float32
ldexp(_Float32 __x, int __exp)
{
  return __builtin_ldexpf(__x, __exp);
}

constexpr _Float32
log(_Float32 __x)
{
  return __builtin_logf(__x);
}

constexpr _Float32
log10(_Float32 __x)
{
  return __builtin_log10f(__x);
}

inline _Float32
modf(_Float32 __x, _Float32 *__iptr)
{
  float __i, __ret = __builtin_modff(__x, &__i);
  *__iptr = __i;
  return __ret;
}

constexpr _Float32
pow(_Float32 __x, _Float32 __y)
{
  return __builtin_powf(__x, __y);
}

constexpr _Float32
sin(_Float32 __x)
{
  return __builtin_sinf(__x);
}

constexpr _Float32
sinh(_Float32 __x)
{
  return __builtin_sinhf(__x);
}

constexpr _Float32
sqrt(_Float32 __x)
{
  return __builtin_sqrtf(__x);
}

constexpr _Float32
tan(_Float32 __x)
{
  return __builtin_tanf(__x);
}

constexpr _Float32
tanh(_Float32 __x)
{
  return __builtin_tanhf(__x);
}
#endif

#if defined(__STDCPP_FLOAT64_T__) && defined(_GLIBCXX_DOUBLE_IS_IEEE_BINARY64)
constexpr _Float64
acos(_Float64 __x)
{
  return __builtin_acos(__x);
}

constexpr _Float64
asin(_Float64 __x)
{
  return __builtin_asin(__x);
}

constexpr _Float64
atan(_Float64 __x)
{
  return __builtin_atan(__x);
}

constexpr _Float64
atan2(_Float64 __y, _Float64 __x)
{
  return __builtin_atan2(__y, __x);
}

constexpr _Float64
ceil(_Float64 __x)
{
  return __builtin_ceil(__x);
}

constexpr _Float64
cos(_Float64 __x)
{
  return __builtin_cos(__x);
}

constexpr _Float64
cosh(_Float64 __x)
{
  return __builtin_cosh(__x);
}

constexpr _Float64
exp(_Float64 __x)
{
  return __builtin_exp(__x);
}

constexpr _Float64
fabs(_Float64 __x)
{
  return __builtin_fabs(__x);
}

constexpr _Float64
floor(_Float64 __x)
{
  return __builtin_floor(__x);
}

constexpr _Float64
fmod(_Float64 __x, _Float64 __y)
{
  return __builtin_fmod(__x, __y);
}

inline _Float64
frexp(_Float64 __x, int *__exp)
{
  return __builtin_frexp(__x, __exp);
}

constexpr _Float64
ldexp(_Float64 __x, int __exp)
{
  return __builtin_ldexp(__x, __exp);
}

constexpr _Float64
log(_Float64 __x)
{
  return __builtin_log(__x);
}

constexpr _Float64
log10(_Float64 __x)
{
  return __builtin_log10(__x);
}

inline _Float64
modf(_Float64 __x, _Float64 *__iptr)
{
  double __i, __ret = __builtin_modf(__x, &__i);
  *__iptr = __i;
  return __ret;
}

constexpr _Float64
pow(_Float64 __x, _Float64 __y)
{
  return __builtin_pow(__x, __y);
}

constexpr _Float64
sin(_Float64 __x)
{
  return __builtin_sin(__x);
}

constexpr _Float64
sinh(_Float64 __x)
{
  return __builtin_sinh(__x);
}

constexpr _Float64
sqrt(_Float64 __x)
{
  return __builtin_sqrt(__x);
}

constexpr _Float64
tan(_Float64 __x)
{
  return __builtin_tan(__x);
}

constexpr _Float64
tanh(_Float64 __x)
{
  return __builtin_tanh(__x);
}
#endif

#if defined(__STDCPP_FLOAT128_T__) && defined(_GLIBCXX_LDOUBLE_IS_IEEE_BINARY128)
constexpr _Float128
acos(_Float128 __x)
{
  return __builtin_acosl(__x);
}

constexpr _Float128
asin(_Float128 __x)
{
  return __builtin_asinl(__x);
}

constexpr _Float128
atan(_Float128 __x)
{
  return __builtin_atanl(__x);
}

constexpr _Float128
atan2(_Float128 __y, _Float128 __x)
{
  return __builtin_atan2l(__y, __x);
}

constexpr _Float128
ceil(_Float128 __x)
{
  return __builtin_ceill(__x);
}

constexpr _Float128
cos(_Float128 __x)
{
  return __builtin_cosl(__x);
}

constexpr _Float128
cosh(_Float128 __x)
{
  return __builtin_coshl(__x);
}

constexpr _Float128
exp(_Float128 __x)
{
  return __builtin_expl(__x);
}

constexpr _Float128
fabs(_Float128 __x)
{
  return __builtin_fabsl(__x);
}

constexpr _Float128
floor(_Float128 __x)
{
  return __builtin_floorl(__x);
}

constexpr _Float128
fmod(_Float128 __x, _Float128 __y)
{
  return __builtin_fmodl(__x, __y);
}

inline _Float128
frexp(_Float128 __x, int *__exp)
{
  return __builtin_frexpl(__x, __exp);
}

constexpr _Float128
ldexp(_Float128 __x, int __exp)
{
  return __builtin_ldexpl(__x, __exp);
}

constexpr _Float128
log(_Float128 __x)
{
  return __builtin_logl(__x);
}

constexpr _Float128
log10(_Float128 __x)
{
  return __builtin_log10l(__x);
}

inline _Float128
modf(_Float128 __x, _Float128 *__iptr)
{
  long double __i, __ret = __builtin_modfl(__x, &__i);
  *__iptr = __i;
  return __ret;
}

constexpr _Float128
pow(_Float128 __x, _Float128 __y)
{
  return __builtin_powl(__x, __y);
}

constexpr _Float128
sin(_Float128 __x)
{
  return __builtin_sinl(__x);
}

constexpr _Float128
sinh(_Float128 __x)
{
  return __builtin_sinhl(__x);
}

constexpr _Float128
sqrt(_Float128 __x)
{
  return __builtin_sqrtl(__x);
}

constexpr _Float128
tan(_Float128 __x)
{
  return __builtin_tanl(__x);
}

constexpr _Float128
tanh(_Float128 __x)
{
  return __builtin_tanhl(__x);
}
#elif defined(__STDCPP_FLOAT128_T__) && defined(_GLIBCXX_HAVE_FLOAT128_MATH)
constexpr _Float128
acos(_Float128 __x)
{
  return __builtin_acosf128(__x);
}

constexpr _Float128
asin(_Float128 __x)
{
  return __builtin_asinf128(__x);
}

constexpr _Float128
atan(_Float128 __x)
{
  return __builtin_atanf128(__x);
}

constexpr _Float128
atan2(_Float128 __y, _Float128 __x)
{
  return __builtin_atan2f128(__y, __x);
}

constexpr _Float128
ceil(_Float128 __x)
{
  return __builtin_ceilf128(__x);
}

constexpr _Float128
cos(_Float128 __x)
{
  return __builtin_cosf128(__x);
}

constexpr _Float128
cosh(_Float128 __x)
{
  return __builtin_coshf128(__x);
}

constexpr _Float128
exp(_Float128 __x)
{
  return __builtin_expf128(__x);
}

constexpr _Float128
fabs(_Float128 __x)
{
  return __builtin_fabsf128(__x);
}

constexpr _Float128
floor(_Float128 __x)
{
  return __builtin_floorf128(__x);
}

constexpr _Float128
fmod(_Float128 __x, _Float128 __y)
{
  return __builtin_fmodf128(__x, __y);
}

inline _Float128
frexp(_Float128 __x, int *__exp)
{
  return __builtin_frexpf128(__x, __exp);
}

constexpr _Float128
ldexp(_Float128 __x, int __exp)
{
  return __builtin_ldexpf128(__x, __exp);
}

constexpr _Float128
log(_Float128 __x)
{
  return __builtin_logf128(__x);
}

constexpr _Float128
log10(_Float128 __x)
{
  return __builtin_log10f128(__x);
}

inline _Float128
modf(_Float128 __x, _Float128 *__iptr)
{
  return __builtin_modff128(__x, __iptr);
}

constexpr _Float128
pow(_Float128 __x, _Float128 __y)
{
  return __builtin_powf128(__x, __y);
}

constexpr _Float128
sin(_Float128 __x)
{
  return __builtin_sinf128(__x);
}

constexpr _Float128
sinh(_Float128 __x)
{
  return __builtin_sinhf128(__x);
}

constexpr _Float128
sqrt(_Float128 __x)
{
  return __builtin_sqrtf128(__x);
}

constexpr _Float128
tan(_Float128 __x)
{
  return __builtin_tanf128(__x);
}

constexpr _Float128
tanh(_Float128 __x)
{
  return __builtin_tanhf128(__x);
}
#endif
