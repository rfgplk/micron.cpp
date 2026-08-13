//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// projections: perspective, orthographic, frustum, look_at
//
// standard right-handed graphics conventions:
//   -> eye looks down -Z
//   -> +X right, +Y up

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../linalg/ops.hpp"
#include "../mk.hpp"
#include "../quants/vec.hpp"
#include "../sqrt.hpp"
#include "../trig.hpp"
#include "transform.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

//   right          = OpenGL/GLM default, eye looks down -Z (current behavior)
//   left           = D3D-style, eye looks down +Z
//   neg_one_to_one = OpenGL NDC, clip z in [-1, 1]  (current behavior)
//   zero_to_one    = Vulkan/D3D/Metal/WebGPU NDC, clip z in [0, 1]  ("ZO")
enum class handedness { right, left };
enum class clip_depth { neg_one_to_one, zero_to_one };

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
perspective_projection(F fov_y, F aspect, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};      // value-init zeroes M once
  const F f = F(1) / math::tan(fov_y * F(0.5));
  out.M.data[0 * 4 + 0] = f / aspect;
  out.M.data[1 * 4 + 1] = f;
  if constexpr ( Hd == handedness::right ) {
    out.M.data[3 * 4 + 2] = F(-1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (F(2) * far_z * near_z) / (near_z - far_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (far_z * near_z) / (near_z - far_z);
    }
  } else {
    out.M.data[3 * 4 + 2] = F(1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(far_z * near_z) / (far_z - near_z);
    }
  }
  // M[3, 3] stays zero
  return out;
}

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
orthographic_projection(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};      // value-init zeroes M once
  out.M.data[0 * 4 + 0] = F(2) / (right - left);
  out.M.data[1 * 4 + 1] = F(2) / (top - bottom);
  out.M.data[0 * 4 + 3] = -(right + left) / (right - left);
  out.M.data[1 * 4 + 3] = -(top + bottom) / (top - bottom);
  if constexpr ( Cd == clip_depth::neg_one_to_one ) {
    out.M.data[2 * 4 + 2] = (Hd == handedness::right ? F(-2) : F(2)) / (far_z - near_z);
    out.M.data[2 * 4 + 3] = -(far_z + near_z) / (far_z - near_z);
  } else {
    out.M.data[2 * 4 + 2] = (Hd == handedness::right ? F(-1) : F(1)) / (far_z - near_z);
    out.M.data[2 * 4 + 3] = -near_z / (far_z - near_z);
  }
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
frustum(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};      // value-init zeroes M once
  out.M.data[0 * 4 + 0] = F(2) * near_z / (right - left);
  out.M.data[1 * 4 + 1] = F(2) * near_z / (top - bottom);
  if constexpr ( Hd == handedness::right ) {
    out.M.data[0 * 4 + 2] = (right + left) / (right - left);
    out.M.data[1 * 4 + 2] = (top + bottom) / (top - bottom);
    out.M.data[3 * 4 + 2] = F(-1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = -(far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (far_z * near_z) / (near_z - far_z);
    }
  } else {
    out.M.data[0 * 4 + 2] = -(right + left) / (right - left);
    out.M.data[1 * 4 + 2] = -(top + bottom) / (top - bottom);
    out.M.data[3 * 4 + 2] = F(1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(far_z * near_z) / (far_z - near_z);
    }
  }
  return out;
}

#if defined(__micron_gfx_simd)
// shared f32 kernel; Fast selects the rsqrt+Newton tier for the two normalizes
template<handedness Hd, bool Fast>
[[nodiscard, gnu::always_inline]] inline transform<f32, 3, transform_mode::isometry>
__look_at_f32(const vec<f32, 3> &eye, const vec<f32, 3> &target, const vec<f32, 3> &up) noexcept
{
  namespace vs = math::__vsimd;
  const simd::f128 e = vs::__load(reinterpret_cast<const float *>(eye.data));
  const simd::f128 t = vs::__load(reinterpret_cast<const float *>(target.data));
  const simd::f128 u = vs::__load(reinterpret_cast<const float *>(up.data));

  // forward = normalize(target - eye), guarded on fn2 > 0 like the scalar path
  const simd::f128 fv = vs::__sub(t, e);
  const simd::f128 fn2 = vs::__dot3_splat(fv, fv);
  simd::f128 fi;
  if constexpr ( Fast )
    fi = vs::__inv_sqrt_fast(fn2);
  else
    fi = vs::__inv_sqrt_exact(fn2);
  const simd::f128 fN = vs::__select(vs::__cmp_gt(fn2, vs::__splat(0.0f)), vs::__mul(fv, fi), fv);
  // s (side): RH = f x up, LH = up x f
  simd::f128 sv;
  if constexpr ( Hd == handedness::right )
    sv = vs::__cross3(fN, u);
  else
    sv = vs::__cross3(u, fN);
  const simd::f128 sn2 = vs::__dot3_splat(sv, sv);
  simd::f128 si;
  if constexpr ( Fast )
    si = vs::__inv_sqrt_fast(sn2);
  else
    si = vs::__inv_sqrt_exact(sn2);
  const simd::f128 sN = vs::__select(vs::__cmp_gt(sn2, vs::__splat(0.0f)), vs::__mul(sv, si), sv);
  // u (true up): RH = s x f, LH = f x s
  simd::f128 uv;
  if constexpr ( Hd == handedness::right )
    uv = vs::__cross3(sN, fN);
  else
    uv = vs::__cross3(fN, sN);
  // forward row is -f for RH, +f for LH; sign flip is exact
  simd::f128 fz;
  if constexpr ( Hd == handedness::right )
    fz = vs::__xor(fN, vs::__splat(-0.0f));
  else
    fz = fN;

  // translation: -R * eye via the column form -- transpose the three rotation
  // rows into columns once, then one mul + two fma cover all three dots
  const simd::f128 lo = vs::__shuf2<0, 1, 0, 1>(sN, uv);      // (s0 s1 u0 u1)
  const simd::f128 hi = vs::__shuf2<2, 3, 2, 3>(sN, uv);      // (s2 s3 u2 u3)
  const simd::f128 c0 = vs::__shuf2<0, 2, 0, 0>(lo, fz);      // (s0 u0 f0 .)
  const simd::f128 c1 = vs::__shuf2<1, 3, 1, 1>(lo, fz);      // (s1 u1 f1 .)
  const simd::f128 c2 = vs::__shuf2<0, 2, 2, 2>(hi, fz);      // (s2 u2 f2 .)
  simd::f128 acc = vs::__mul(c0, vs::__swz<0, 0, 0, 0>(e));
  acc = vs::__fma(c1, vs::__swz<1, 1, 1, 1>(e), acc);
  acc = vs::__fma(c2, vs::__swz<2, 2, 2, 2>(e), acc);
  const simd::f128 tv = vs::__xor(acc, vs::__splat(-0.0f));      // (t0 t1 t2 .)

  transform<f32, 3, transform_mode::isometry> out{ mat<f32, 4, 4>(micron::__mat_uninit) };
  float *op = reinterpret_cast<float *>(out.M.data);
  vs::__store(op + 0, vs::__insert_lane3(sN, tv));
  vs::__store(op + 4, vs::__insert_lane3(uv, vs::__swz<1, 1, 1, 1>(tv)));
  vs::__store(op + 8, vs::__insert_lane3(fz, vs::__swz<2, 2, 2, 2>(tv)));
  vs::__store(op + 12, vs::__setr(0.0f, 0.0f, 0.0f, 1.0f));
  return out;
}
#endif

template<handedness Hd = handedness::right, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::isometry>
look_at(const vec<F, 3> &eye, const vec<F, 3> &target, const vec<F, 3> &up) noexcept
{
#if defined(__micron_gfx_simd)
  if constexpr ( micron::same_as<F, f32> ) {
    return __look_at_f32<Hd, false>(eye, target, up);
  }
#endif
  // forward = normalize(target - eye)
  vec<F, 3> f{};
  for ( usize i = 0; i < 3; ++i ) f.data[i] = target.data[i] - eye.data[i];
  F fn2 = f.data[0] * f.data[0] + f.data[1] * f.data[1] + f.data[2] * f.data[2];
  if ( fn2 > F(0) ) {
    F inv = math::__vsimd::__inv_sqrt_exact_s(fn2);
    for ( usize i = 0; i < 3; ++i ) f.data[i] *= inv;
  }
  // s (side): RH = f x up, LH = up x f
  vec<F, 3> s{};
  if constexpr ( Hd == handedness::right )
    s = linalg::ops::cross<F>(f, up);
  else
    s = linalg::ops::cross<F>(up, f);
  F sn2 = s.data[0] * s.data[0] + s.data[1] * s.data[1] + s.data[2] * s.data[2];
  if ( sn2 > F(0) ) {
    F inv = math::__vsimd::__inv_sqrt_exact_s(sn2);
    for ( usize i = 0; i < 3; ++i ) s.data[i] *= inv;
  }
  // u (true up): RH = s x f, LH = f x s
  vec<F, 3> u{};
  if constexpr ( Hd == handedness::right )
    u = linalg::ops::cross<F>(s, f);
  else
    u = linalg::ops::cross<F>(f, s);

  transform<F, 3, transform_mode::isometry> out{};      // value-init zeroes M once
  // rotation part: rows are (s, u, -f) for RH, (s, u, +f) for LH
  const F fz = (Hd == handedness::right) ? F(-1) : F(1);
  out.M.data[0 * 4 + 0] = s.data[0];
  out.M.data[0 * 4 + 1] = s.data[1];
  out.M.data[0 * 4 + 2] = s.data[2];
  out.M.data[1 * 4 + 0] = u.data[0];
  out.M.data[1 * 4 + 1] = u.data[1];
  out.M.data[1 * 4 + 2] = u.data[2];
  out.M.data[2 * 4 + 0] = fz * f.data[0];
  out.M.data[2 * 4 + 1] = fz * f.data[1];
  out.M.data[2 * 4 + 2] = fz * f.data[2];
  // translation: -R * eye
  F tx = F(0), ty = F(0), tz = F(0);
  for ( usize j = 0; j < 3; ++j ) {
    tx -= out.M.data[0 * 4 + j] * eye.data[j];
    ty -= out.M.data[1 * 4 + j] * eye.data[j];
    tz -= out.M.data[2 * 4 + j] * eye.data[j];
  }
  out.M.data[0 * 4 + 3] = tx;
  out.M.data[1 * 4 + 3] = ty;
  out.M.data[2 * 4 + 3] = tz;
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

// policy::fast
template<handedness Hd = handedness::right, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::isometry>
look_at(const vec<F, 3> &eye, const vec<F, 3> &target, const vec<F, 3> &up, math::policy::fast_tag) noexcept
{
#if defined(__micron_gfx_simd)
  if constexpr ( micron::same_as<F, f32> ) {
    return __look_at_f32<Hd, true>(eye, target, up);
  }
#endif
  return look_at<Hd, F>(eye, target, up);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// closed-form O(1) inverses of the projection matrices above
template<ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
inv_perspective(const transform<F, 3, transform_mode::projective> &p) noexcept
{
  const F A = p.M.data[0 * 4 + 0];
  const F B = p.M.data[1 * 4 + 1];
  const F P = p.M.data[0 * 4 + 2];
  const F Q = p.M.data[1 * 4 + 2];
  const F C = p.M.data[2 * 4 + 2];
  const F D = p.M.data[2 * 4 + 3];
  const F E = p.M.data[3 * 4 + 2];
  transform<F, 3, transform_mode::projective> out{};      // value-init zeroes M once
  out.M.data[0 * 4 + 0] = F(1) / A;
  out.M.data[0 * 4 + 3] = -P / (A * E);
  out.M.data[1 * 4 + 1] = F(1) / B;
  out.M.data[1 * 4 + 3] = -Q / (B * E);
  out.M.data[2 * 4 + 3] = F(1) / E;
  out.M.data[3 * 4 + 2] = F(1) / D;
  out.M.data[3 * 4 + 3] = -C / (D * E);
  return out;
}

// inverse of an orthographic matrix; affine-diagonal layout
template<ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
inv_orthographic(const transform<F, 3, transform_mode::projective> &o) noexcept
{
  const F A = o.M.data[0 * 4 + 0];
  const F B = o.M.data[1 * 4 + 1];
  const F C = o.M.data[2 * 4 + 2];
  const F Tx = o.M.data[0 * 4 + 3];
  const F Ty = o.M.data[1 * 4 + 3];
  const F Tz = o.M.data[2 * 4 + 3];
  transform<F, 3, transform_mode::projective> out{};      // value-init zeroes M once
  out.M.data[0 * 4 + 0] = F(1) / A;
  out.M.data[1 * 4 + 1] = F(1) / B;
  out.M.data[2 * 4 + 2] = F(1) / C;
  out.M.data[0 * 4 + 3] = -Tx / A;
  out.M.data[1 * 4 + 3] = -Ty / B;
  out.M.data[2 * 4 + 3] = -Tz / C;
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

};      // namespace geometry
};      // namespace math
};      // namespace micron
