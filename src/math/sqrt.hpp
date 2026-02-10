//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../simd/intrin.hpp"
#include "../types.hpp"
#include "generic.hpp"

// void rsqrtps_sse(float *in, float *out, unsigned long int len);
// void vrsqrtps_avx(float *in, float *out, unsigned long int len);

namespace micron
{
namespace math
{
// inline void sqrt_sse(float *in, float *out, unsigned long int len) {
//  rsqrtps_sse(in, out, len);
// };
// inline void sqrt_avx(float *in, float *out, unsigned long int len) {
//  vrsqrtps_avx(in, out, len);
//};
inline f32
cbrt(const f32 x)
{
  return math::powerf32(x, 1 / 3.f);
};
inline f64
cbrtd(const f64 x)
{
  return math::powerf(x, 1 / 3.f);
};
inline flong
cbrtdl(const flong x)
{
  return math::powerflong(x, 1 / 3.f);
};
inline float
fsqrt(const float x)
{
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
};
inline float
gsqrt(const float x)
{
  return static_cast<float>(__builtin_sqrt(x));
};

inline float
sd_sqrt(const float x)
{
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
};
inline float
sd_rsqrt(const float x)
{
  return 1.0f / _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
};
inline float
ss_sqrt(const float x)
{
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
};
inline float
ss_rsqrt(const float x)
{
  return 1.0f / _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
};
inline float
sqrt(const float x)
{
  return fsqrt(x);
};
};
};     // namespace micron
