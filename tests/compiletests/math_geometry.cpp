// compile-validity gate: the graphics-math kernel surface (normalize / cross /
// inv4 / look_at / perspective) + the __vsimd register cores compile on every
// arch/ISA/opt cell, f32 and f64, runtime and consteval. Not run.
#include "../../src/math/__vec_simd.hpp"
#include "../../src/math/geometry/projection.hpp"
#include "../../src/math/linalg.hpp"
#include "../../src/math/quants/vecs.hpp"

namespace geo = micron::math::geometry;
namespace lops = micron::math::linalg::ops;

using micron::math::mat;
using micron::math::vec;

// consteval pins: the scalar bodies must stay constant-evaluable
constexpr vec<f64, 3> cx = lops::cross<f64>(vec<f64, 3>{ 1.0, 0.0, 0.0 }, vec<f64, 3>{ 0.0, 1.0, 0.0 });
static_assert(cx.data[2] == 1.0);
constexpr mat<f64, 4, 4> cid = mat<f64, 4, 4>::identity();
constexpr mat<f64, 4, 4> cinv = lops::inv4<f64>(cid);
static_assert(cinv.data[0] == 1.0 && cinv.data[1] == 0.0);

template<typename F>
static F
touch_kernels(void)
{
  const vec<F, 3> a{ F(1), F(2), F(3) };
  const vec<F, 3> b{ F(4), F(5), F(6) };
  const vec<F, 4> c{ F(1), F(2), F(3), F(4) };
  F acc = F(0);

  acc += lops::normalize<F, 3>(a).data[0];
  acc += lops::normalize<F, 4>(c).data[0];
  acc += lops::normalize(a, micron::math::policy::fast).data[1];
  acc += lops::cross<F>(a, b).data[1];
  acc += lops::norm<F, 3>(a);

  mat<F, 4, 4> m = mat<F, 4, 4>::identity();
  m.data[1] = F(2);
  m.data[4] = F(-1);
  acc += lops::inv4<F>(m).data[5];
  acc += lops::det4<F>(m);

  const auto lt = geo::look_at<geo::handedness::right, F>(a, b, vec<F, 3>{ F(0), F(1), F(0) });
  acc += lt.M.data[0];
  const auto ll = geo::look_at<geo::handedness::left, F>(a, b, vec<F, 3>{ F(0), F(1), F(0) });
  acc += ll.M.data[0];
  const auto lf = geo::look_at<geo::handedness::right, F>(a, b, vec<F, 3>{ F(0), F(1), F(0) }, micron::math::policy::fast);
  acc += lf.M.data[5];
  const auto pp = geo::perspective_projection<geo::handedness::right, geo::clip_depth::zero_to_one, F>(F(0.9), F(1.5), F(0.1), F(100));
  acc += pp.M.data[5];
  return acc;
}

static f32
touch_vector3(void)
{
  micron::vector_3<f32> v{ 1.0f, 2.0f, 3.0f };
  micron::vector_3<f32> w{ 4.0f, 5.0f, 6.0f };
  f32 acc = v.normalized().x;
  acc += v.normalized(micron::math::policy::fast).y;
  v.normalize();
  v.normalize(micron::math::policy::fast);
  acc += v.cross(w).y;
  micron::vector_2<f32> v2{ 3.0f, 4.0f };
  acc += v2.normalized().x;
  micron::vector_4<f32> v4{ 1.0f, 2.0f, 3.0f, 4.0f };
  acc += v4.normalized().w;
  micron::vector_3<f64> vd{ 1.0, 2.0, 3.0 };
  acc += static_cast<f32>(vd.normalized().z);
  return acc;
}

static f32
touch_vsimd(void)
{
  namespace vs = micron::math::__vsimd;
  f32 acc = vs::__inv_sqrt_exact_s(4.0f) + static_cast<f32>(vs::__inv_sqrt_exact_s(9.0)) + vs::__inv_sqrt_fast_s(16.0f);
#if defined(__micron_gfx_simd)
  alignas(16) float in[4] = { 1.0f, 2.0f, 3.0f, 0.0f };
  alignas(16) float out[4];
#if defined(__micron_arch_x86_any)
  const micron::simd::f128 va = micron::simd::sse::load_f32(in);
#else
  const micron::simd::f128 va = micron::simd::neon::load_f32(in);
#endif
  micron::simd::f128 r = vs::__cross3(va, vs::__zero3(va));
  r = vs::__select(vs::__cmp_gt(vs::__dot3_splat(va, va), vs::__dot4_splat(r, r)), r, vs::__inv_sqrt_exact(vs::__dot3_splat(va, va)));
  r = vs::__insert_lane3(r, vs::__inv_sqrt_fast(vs::__dot4_splat(va, va)));
#if defined(__micron_arch_x86_any)
  micron::simd::sse::store_f32(out, r);
#else
  micron::simd::neon::store_f32(out, r);
#endif
  acc += out[0] + out[3];
#endif
  return acc;
}

int
main(void)
{
  f64 d = static_cast<f64>(touch_kernels<f32>());
  d += touch_kernels<f64>();
  d += static_cast<f64>(touch_vector3());
  d += static_cast<f64>(touch_vsimd());
  return static_cast<int>(d) & 0x7f;
}
