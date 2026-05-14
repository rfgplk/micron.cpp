//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../generic.hpp"
#include "../ieee.hpp"

#if defined(__micron_x86_avx2) || defined(__micron_arm_neon)
#include "../../simd/aliases.hpp"
#endif

namespace micron
{
namespace math
{
namespace sparse
{

namespace __impl_sparse_simd
{

// scalar fallbacks
template <ieee754_floating F>
[[gnu::always_inline, gnu::flatten]] inline F
scalar_dot(const F *__restrict__ a, const F *__restrict__ b, usize n) noexcept
{
  F s0 = F(0), s1 = F(0);
  usize i = 0;
  for ( ; i + 2 <= n; i += 2 ) {
    s0 = math::fma<F>(a[i], b[i], s0);
    s1 = math::fma<F>(a[i + 1], b[i + 1], s1);
  }
  F s = s0 + s1;
  for ( ; i < n; ++i ) s = math::fma<F>(a[i], b[i], s);
  return s;
}

template <ieee754_floating F>
[[gnu::always_inline, gnu::flatten]] inline F
scalar_norm_sq(const F *__restrict__ a, usize n) noexcept
{
  F s0 = F(0), s1 = F(0);
  usize i = 0;
  for ( ; i + 2 <= n; i += 2 ) {
    s0 = math::fma<F>(a[i], a[i], s0);
    s1 = math::fma<F>(a[i + 1], a[i + 1], s1);
  }
  F s = s0 + s1;
  for ( ; i < n; ++i ) s = math::fma<F>(a[i], a[i], s);
  return s;
}

template <ieee754_floating F>
[[gnu::always_inline, gnu::flatten]] inline void
scalar_axpy(F alpha, const F *__restrict__ x, F *__restrict__ y, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) y[i] = math::fma<F>(alpha, x[i], y[i]);
}

template <ieee754_floating F>
[[gnu::always_inline, gnu::flatten]] inline void
scalar_scal(F alpha, F *__restrict__ x, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) x[i] *= alpha;
}

template <ieee754_floating F>
[[gnu::always_inline, gnu::flatten]] inline void
scalar_axpy_scatter(F alpha, const F *__restrict__ vals, const u32 *__restrict__ idx, F *__restrict__ y, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) y[idx[i]] = math::fma<F>(alpha, vals[i], y[idx[i]]);
}

#if defined(__micron_x86_avx2)

[[gnu::always_inline]] inline f64
avx2_dot_f64(const f64 *__restrict__ a_in, const f64 *__restrict__ b_in, usize n) noexcept
{
  const double *__restrict__ a = reinterpret_cast<const double *>(a_in);
  const double *__restrict__ b = reinterpret_cast<const double *>(b_in);
  __m256d acc0 = simd::avx::zero_f64();
  __m256d acc1 = simd::avx::zero_f64();
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    __m256d va0 = simd::avx::loadu_f64(a + i);
    __m256d vb0 = simd::avx::loadu_f64(b + i);
    __m256d va1 = simd::avx::loadu_f64(a + i + 4);
    __m256d vb1 = simd::avx::loadu_f64(b + i + 4);
    acc0 = simd::fma::fma_f64(va0, vb0, acc0);
    acc1 = simd::fma::fma_f64(va1, vb1, acc1);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d va = simd::avx::loadu_f64(a + i);
    __m256d vb = simd::avx::loadu_f64(b + i);
    acc0 = simd::fma::fma_f64(va, vb, acc0);
  }
  __m256d acc = simd::avx::add_f64(acc0, acc1);
  alignas(32) double buf[4];
  simd::avx::store_f64(buf, acc);
  double s = buf[0] + buf[1] + buf[2] + buf[3];
  for ( ; i < n; ++i ) s = static_cast<double>(math::fma<f64>(static_cast<f64>(a[i]), static_cast<f64>(b[i]), static_cast<f64>(s)));
  return static_cast<f64>(s);
}

[[gnu::always_inline]] inline f32
avx2_dot_f32(const f32 *__restrict__ a_in, const f32 *__restrict__ b_in, usize n) noexcept
{
  const float *__restrict__ a = reinterpret_cast<const float *>(a_in);
  const float *__restrict__ b = reinterpret_cast<const float *>(b_in);
  __m256 acc0 = simd::avx::zero_f32();
  __m256 acc1 = simd::avx::zero_f32();
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256 va0 = simd::avx::loadu_f32(a + i);
    __m256 vb0 = simd::avx::loadu_f32(b + i);
    __m256 va1 = simd::avx::loadu_f32(a + i + 8);
    __m256 vb1 = simd::avx::loadu_f32(b + i + 8);
    acc0 = simd::fma::fma_f32(va0, vb0, acc0);
    acc1 = simd::fma::fma_f32(va1, vb1, acc1);
  }
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 va = simd::avx::loadu_f32(a + i);
    __m256 vb = simd::avx::loadu_f32(b + i);
    acc0 = simd::fma::fma_f32(va, vb, acc0);
  }
  __m256 acc = simd::avx::add_f32(acc0, acc1);
  alignas(32) float buf[8];
  simd::avx::store_f32(buf, acc);
  float s = (buf[0] + buf[1] + buf[2] + buf[3]) + (buf[4] + buf[5] + buf[6] + buf[7]);
  for ( ; i < n; ++i ) s = static_cast<float>(math::fma<f32>(static_cast<f32>(a[i]), static_cast<f32>(b[i]), static_cast<f32>(s)));
  return static_cast<f32>(s);
}

[[gnu::always_inline]] inline f64
avx2_norm_sq_f64(const f64 *__restrict__ a_in, usize n) noexcept
{
  const double *__restrict__ a = reinterpret_cast<const double *>(a_in);
  __m256d acc0 = simd::avx::zero_f64();
  __m256d acc1 = simd::avx::zero_f64();
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    __m256d v0 = simd::avx::loadu_f64(a + i);
    __m256d v1 = simd::avx::loadu_f64(a + i + 4);
    acc0 = simd::fma::fma_f64(v0, v0, acc0);
    acc1 = simd::fma::fma_f64(v1, v1, acc1);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d v = simd::avx::loadu_f64(a + i);
    acc0 = simd::fma::fma_f64(v, v, acc0);
  }
  __m256d acc = simd::avx::add_f64(acc0, acc1);
  alignas(32) double buf[4];
  simd::avx::store_f64(buf, acc);
  double s = buf[0] + buf[1] + buf[2] + buf[3];
  for ( ; i < n; ++i ) s += a[i] * a[i];
  return static_cast<f64>(s);
}

[[gnu::always_inline]] inline f32
avx2_norm_sq_f32(const f32 *__restrict__ a_in, usize n) noexcept
{
  const float *__restrict__ a = reinterpret_cast<const float *>(a_in);
  __m256 acc0 = simd::avx::zero_f32();
  __m256 acc1 = simd::avx::zero_f32();
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256 v0 = simd::avx::loadu_f32(a + i);
    __m256 v1 = simd::avx::loadu_f32(a + i + 8);
    acc0 = simd::fma::fma_f32(v0, v0, acc0);
    acc1 = simd::fma::fma_f32(v1, v1, acc1);
  }
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 v = simd::avx::loadu_f32(a + i);
    acc0 = simd::fma::fma_f32(v, v, acc0);
  }
  __m256 acc = simd::avx::add_f32(acc0, acc1);
  alignas(32) float buf[8];
  simd::avx::store_f32(buf, acc);
  float s = (buf[0] + buf[1] + buf[2] + buf[3]) + (buf[4] + buf[5] + buf[6] + buf[7]);
  for ( ; i < n; ++i ) s += a[i] * a[i];
  return static_cast<f32>(s);
}

[[gnu::always_inline]] inline void
avx2_axpy_f64(f64 alpha_in, const f64 *__restrict__ x_in, f64 *__restrict__ y_in, usize n) noexcept
{
  const double alpha = static_cast<double>(alpha_in);
  const double *__restrict__ x = reinterpret_cast<const double *>(x_in);
  double *__restrict__ y = reinterpret_cast<double *>(y_in);
  const __m256d va = simd::avx::splat_f64(alpha);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d vx = simd::avx::loadu_f64(x + i);
    __m256d vy = simd::avx::loadu_f64(y + i);
    vy = simd::fma::fma_f64(va, vx, vy);
    simd::avx::storeu_f64(y + i, vy);
  }
  for ( ; i < n; ++i ) y[i] = alpha * x[i] + y[i];
}

[[gnu::always_inline]] inline void
avx2_axpy_f32(f32 alpha_in, const f32 *__restrict__ x_in, f32 *__restrict__ y_in, usize n) noexcept
{
  const float alpha = static_cast<float>(alpha_in);
  const float *__restrict__ x = reinterpret_cast<const float *>(x_in);
  float *__restrict__ y = reinterpret_cast<float *>(y_in);
  const __m256 va = simd::avx::splat_f32(alpha);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 vx = simd::avx::loadu_f32(x + i);
    __m256 vy = simd::avx::loadu_f32(y + i);
    vy = simd::fma::fma_f32(va, vx, vy);
    simd::avx::storeu_f32(y + i, vy);
  }
  for ( ; i < n; ++i ) y[i] = alpha * x[i] + y[i];
}

[[gnu::always_inline]] inline void
avx2_scal_f64(f64 alpha_in, f64 *__restrict__ x_in, usize n) noexcept
{
  const double alpha = static_cast<double>(alpha_in);
  double *__restrict__ x = reinterpret_cast<double *>(x_in);
  const __m256d va = simd::avx::splat_f64(alpha);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d vx = simd::avx::loadu_f64(x + i);
    vx = simd::avx::mul_f64(va, vx);
    simd::avx::storeu_f64(x + i, vx);
  }
  for ( ; i < n; ++i ) x[i] *= alpha;
}

[[gnu::always_inline]] inline void
avx2_scal_f32(f32 alpha_in, f32 *__restrict__ x_in, usize n) noexcept
{
  const float alpha = static_cast<float>(alpha_in);
  float *__restrict__ x = reinterpret_cast<float *>(x_in);
  const __m256 va = simd::avx::splat_f32(alpha);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 vx = simd::avx::loadu_f32(x + i);
    vx = simd::avx::mul_f32(va, vx);
    simd::avx::storeu_f32(x + i, vx);
  }
  for ( ; i < n; ++i ) x[i] *= alpha;
}

#endif

#if defined(__micron_arm_neon)

[[gnu::always_inline]] inline float32x4_t
neon_fma_f32(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
  return simd::neon::fma_f32(acc, a, b);
#else
  return simd::neon::add(acc, simd::neon::mul(a, b));
#endif
}

[[gnu::always_inline]] inline f32
neon_dot_f32(const f32 *__restrict__ a_in, const f32 *__restrict__ b_in, usize n) noexcept
{
  const float *__restrict__ a = reinterpret_cast<const float *>(a_in);
  const float *__restrict__ b = reinterpret_cast<const float *>(b_in);
  float32x4_t acc0 = simd::neon::splat_f32(0.0f);
  float32x4_t acc1 = simd::neon::splat_f32(0.0f);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    float32x4_t va0 = simd::neon::load_f32(a + i);
    float32x4_t vb0 = simd::neon::load_f32(b + i);
    float32x4_t va1 = simd::neon::load_f32(a + i + 4);
    float32x4_t vb1 = simd::neon::load_f32(b + i + 4);
    acc0 = neon_fma_f32(acc0, va0, vb0);
    acc1 = neon_fma_f32(acc1, va1, vb1);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t va = simd::neon::load_f32(a + i);
    float32x4_t vb = simd::neon::load_f32(b + i);
    acc0 = neon_fma_f32(acc0, va, vb);
  }
  float32x4_t acc = simd::neon::add(acc0, acc1);
  float s = (acc[0] + acc[1]) + (acc[2] + acc[3]);
  for ( ; i < n; ++i ) s += a[i] * b[i];
  return static_cast<f32>(s);
}

[[gnu::always_inline]] inline f32
neon_norm_sq_f32(const f32 *__restrict__ a_in, usize n) noexcept
{
  const float *__restrict__ a = reinterpret_cast<const float *>(a_in);
  float32x4_t acc0 = simd::neon::splat_f32(0.0f);
  float32x4_t acc1 = simd::neon::splat_f32(0.0f);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    float32x4_t v0 = simd::neon::load_f32(a + i);
    float32x4_t v1 = simd::neon::load_f32(a + i + 4);
    acc0 = neon_fma_f32(acc0, v0, v0);
    acc1 = neon_fma_f32(acc1, v1, v1);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t v = simd::neon::load_f32(a + i);
    acc0 = neon_fma_f32(acc0, v, v);
  }
  float32x4_t acc = simd::neon::add(acc0, acc1);
  float s = (acc[0] + acc[1]) + (acc[2] + acc[3]);
  for ( ; i < n; ++i ) s += a[i] * a[i];
  return static_cast<f32>(s);
}

[[gnu::always_inline]] inline void
neon_axpy_f32(f32 alpha_in, const f32 *__restrict__ x_in, f32 *__restrict__ y_in, usize n) noexcept
{
  const float alpha = static_cast<float>(alpha_in);
  const float *__restrict__ x = reinterpret_cast<const float *>(x_in);
  float *__restrict__ y = reinterpret_cast<float *>(y_in);
  const float32x4_t va = simd::neon::splat_f32(alpha);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t vx = simd::neon::load_f32(x + i);
    float32x4_t vy = simd::neon::load_f32(y + i);
    vy = neon_fma_f32(vy, va, vx);
    simd::neon::store_f32(y + i, vy);
  }
  for ( ; i < n; ++i ) y[i] = alpha * x[i] + y[i];
}

[[gnu::always_inline]] inline void
neon_scal_f32(f32 alpha_in, f32 *__restrict__ x_in, usize n) noexcept
{
  const float alpha = static_cast<float>(alpha_in);
  float *__restrict__ x = reinterpret_cast<float *>(x_in);
  const float32x4_t va = simd::neon::splat_f32(alpha);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t vx = simd::neon::load_f32(x + i);
    vx = simd::neon::mul(va, vx);
    simd::neon::store_f32(x + i, vx);
  }
  for ( ; i < n; ++i ) x[i] *= alpha;
}

#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline float64x2_t
neon_fma_f64(float64x2_t acc, float64x2_t a, float64x2_t b) noexcept
{
  return simd::neon::fma_f64(acc, a, b);
}

[[gnu::always_inline]] inline f64
neon_dot_f64(const f64 *__restrict__ a_in, const f64 *__restrict__ b_in, usize n) noexcept
{
  const double *__restrict__ a = reinterpret_cast<const double *>(a_in);
  const double *__restrict__ b = reinterpret_cast<const double *>(b_in);
  float64x2_t acc0 = simd::neon::splat_f64(0.0);
  float64x2_t acc1 = simd::neon::splat_f64(0.0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    float64x2_t va0 = simd::neon::load_f64(a + i);
    float64x2_t vb0 = simd::neon::load_f64(b + i);
    float64x2_t va1 = simd::neon::load_f64(a + i + 2);
    float64x2_t vb1 = simd::neon::load_f64(b + i + 2);
    acc0 = neon_fma_f64(acc0, va0, vb0);
    acc1 = neon_fma_f64(acc1, va1, vb1);
  }
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t va = simd::neon::load_f64(a + i);
    float64x2_t vb = simd::neon::load_f64(b + i);
    acc0 = neon_fma_f64(acc0, va, vb);
  }
  float64x2_t acc = simd::neon::add(acc0, acc1);
  double s = acc[0] + acc[1];
  for ( ; i < n; ++i ) s += a[i] * b[i];
  return static_cast<f64>(s);
}

[[gnu::always_inline]] inline f64
neon_norm_sq_f64(const f64 *__restrict__ a_in, usize n) noexcept
{
  const double *__restrict__ a = reinterpret_cast<const double *>(a_in);
  float64x2_t acc0 = simd::neon::splat_f64(0.0);
  float64x2_t acc1 = simd::neon::splat_f64(0.0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    float64x2_t v0 = simd::neon::load_f64(a + i);
    float64x2_t v1 = simd::neon::load_f64(a + i + 2);
    acc0 = neon_fma_f64(acc0, v0, v0);
    acc1 = neon_fma_f64(acc1, v1, v1);
  }
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t v = simd::neon::load_f64(a + i);
    acc0 = neon_fma_f64(acc0, v, v);
  }
  float64x2_t acc = simd::neon::add(acc0, acc1);
  double s = acc[0] + acc[1];
  for ( ; i < n; ++i ) s += a[i] * a[i];
  return static_cast<f64>(s);
}

[[gnu::always_inline]] inline void
neon_axpy_f64(f64 alpha_in, const f64 *__restrict__ x_in, f64 *__restrict__ y_in, usize n) noexcept
{
  const double alpha = static_cast<double>(alpha_in);
  const double *__restrict__ x = reinterpret_cast<const double *>(x_in);
  double *__restrict__ y = reinterpret_cast<double *>(y_in);
  const float64x2_t va = simd::neon::splat_f64(alpha);
  usize i = 0;
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t vx = simd::neon::load_f64(x + i);
    float64x2_t vy = simd::neon::load_f64(y + i);
    vy = neon_fma_f64(vy, va, vx);
    simd::neon::store_f64(y + i, vy);
  }
  for ( ; i < n; ++i ) y[i] = alpha * x[i] + y[i];
}

[[gnu::always_inline]] inline void
neon_scal_f64(f64 alpha_in, f64 *__restrict__ x_in, usize n) noexcept
{
  const double alpha = static_cast<double>(alpha_in);
  double *__restrict__ x = reinterpret_cast<double *>(x_in);
  const float64x2_t va = simd::neon::splat_f64(alpha);
  usize i = 0;
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t vx = simd::neon::load_f64(x + i);
    vx = simd::neon::mul(va, vx);
    simd::neon::store_f64(x + i, vx);
  }
  for ( ; i < n; ++i ) x[i] *= alpha;
}

#endif
#endif

};     // namespace __impl_sparse_simd

template <typename F> inline constexpr bool is_simd_f64_eligible = micron::is_same_v<F, f64> || micron::is_same_v<F, double>;

template <typename F> inline constexpr bool is_simd_f32_eligible = micron::is_same_v<F, f32> || micron::is_same_v<F, float>;

template <ieee754_floating F>
[[gnu::always_inline]] inline F
dot_values(const F *__restrict__ a, const F *__restrict__ b, usize n) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( is_simd_f64_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::avx2_dot_f64(reinterpret_cast<const f64 *>(a), reinterpret_cast<const f64 *>(b), n));
  else if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::avx2_dot_f32(reinterpret_cast<const f32 *>(a), reinterpret_cast<const f32 *>(b), n));
  else
    return __impl_sparse_simd::scalar_dot<F>(a, b, n);
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( is_simd_f64_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_dot_f64(reinterpret_cast<const f64 *>(a), reinterpret_cast<const f64 *>(b), n));
  else if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_dot_f32(reinterpret_cast<const f32 *>(a), reinterpret_cast<const f32 *>(b), n));
  else
    return __impl_sparse_simd::scalar_dot<F>(a, b, n);
#elif defined(__micron_arm_neon)
  if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_dot_f32(reinterpret_cast<const f32 *>(a), reinterpret_cast<const f32 *>(b), n));
  else
    return __impl_sparse_simd::scalar_dot<F>(a, b, n);
#else
  return __impl_sparse_simd::scalar_dot<F>(a, b, n);
#endif
}

template <ieee754_floating F>
[[gnu::always_inline]] inline F
norm_sq_values(const F *__restrict__ a, usize n) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( is_simd_f64_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::avx2_norm_sq_f64(reinterpret_cast<const f64 *>(a), n));
  else if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::avx2_norm_sq_f32(reinterpret_cast<const f32 *>(a), n));
  else
    return __impl_sparse_simd::scalar_norm_sq<F>(a, n);
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( is_simd_f64_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_norm_sq_f64(reinterpret_cast<const f64 *>(a), n));
  else if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_norm_sq_f32(reinterpret_cast<const f32 *>(a), n));
  else
    return __impl_sparse_simd::scalar_norm_sq<F>(a, n);
#elif defined(__micron_arm_neon)
  if constexpr ( is_simd_f32_eligible<F> )
    return static_cast<F>(__impl_sparse_simd::neon_norm_sq_f32(reinterpret_cast<const f32 *>(a), n));
  else
    return __impl_sparse_simd::scalar_norm_sq<F>(a, n);
#else
  return __impl_sparse_simd::scalar_norm_sq<F>(a, n);
#endif
}

template <ieee754_floating F>
[[gnu::always_inline]] inline void
axpy_values(F alpha, const F *__restrict__ x, F *__restrict__ y, usize n) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( is_simd_f64_eligible<F> )
    __impl_sparse_simd::avx2_axpy_f64(static_cast<f64>(alpha), reinterpret_cast<const f64 *>(x), reinterpret_cast<f64 *>(y), n);
  else if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::avx2_axpy_f32(static_cast<f32>(alpha), reinterpret_cast<const f32 *>(x), reinterpret_cast<f32 *>(y), n);
  else
    __impl_sparse_simd::scalar_axpy<F>(alpha, x, y, n);
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( is_simd_f64_eligible<F> )
    __impl_sparse_simd::neon_axpy_f64(static_cast<f64>(alpha), reinterpret_cast<const f64 *>(x), reinterpret_cast<f64 *>(y), n);
  else if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::neon_axpy_f32(static_cast<f32>(alpha), reinterpret_cast<const f32 *>(x), reinterpret_cast<f32 *>(y), n);
  else
    __impl_sparse_simd::scalar_axpy<F>(alpha, x, y, n);
#elif defined(__micron_arm_neon)
  if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::neon_axpy_f32(static_cast<f32>(alpha), reinterpret_cast<const f32 *>(x), reinterpret_cast<f32 *>(y), n);
  else
    __impl_sparse_simd::scalar_axpy<F>(alpha, x, y, n);
#else
  __impl_sparse_simd::scalar_axpy<F>(alpha, x, y, n);
#endif
}

template <ieee754_floating F>
[[gnu::always_inline]] inline void
scal_values(F alpha, F *__restrict__ x, usize n) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( is_simd_f64_eligible<F> )
    __impl_sparse_simd::avx2_scal_f64(static_cast<f64>(alpha), reinterpret_cast<f64 *>(x), n);
  else if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::avx2_scal_f32(static_cast<f32>(alpha), reinterpret_cast<f32 *>(x), n);
  else
    __impl_sparse_simd::scalar_scal<F>(alpha, x, n);
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( is_simd_f64_eligible<F> )
    __impl_sparse_simd::neon_scal_f64(static_cast<f64>(alpha), reinterpret_cast<f64 *>(x), n);
  else if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::neon_scal_f32(static_cast<f32>(alpha), reinterpret_cast<f32 *>(x), n);
  else
    __impl_sparse_simd::scalar_scal<F>(alpha, x, n);
#elif defined(__micron_arm_neon)
  if constexpr ( is_simd_f32_eligible<F> )
    __impl_sparse_simd::neon_scal_f32(static_cast<f32>(alpha), reinterpret_cast<f32 *>(x), n);
  else
    __impl_sparse_simd::scalar_scal<F>(alpha, x, n);
#else
  __impl_sparse_simd::scalar_scal<F>(alpha, x, n);
#endif
}

template <ieee754_floating F, micron::integral I>
[[gnu::always_inline, gnu::flatten]] inline void
spmv_col_scatter(F alpha, F x_j, const F *__restrict__ vals, const I *__restrict__ idx, F *__restrict__ y, usize nnz_col) noexcept
{
  const F ax = alpha * x_j;
  for ( usize k = 0; k < nnz_col; ++k ) y[idx[k]] = math::fma<F>(ax, vals[k], y[idx[k]]);
}

};     // namespace sparse
};     // namespace math
};     // namespace micron
