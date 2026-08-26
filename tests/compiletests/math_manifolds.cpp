//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// compile-validity gate for the manifold helper and SIMD batch surfaces on
// every architecture / ISA / freestanding matrix cell. Not run.

#include "../../src/math/manifolds/manifolds.hpp"

namespace mf = micron::math::manifolds;
namespace lie = micron::math::manifolds::lie;

using micron::math::mat;
using micron::math::quat;
using micron::math::vec;

constexpr auto cR = lie::SO3<f64>::identity();
constexpr auto cv = lie::SO3<f64>::rotate(cR, vec<f64, 3>{ 1.0, 2.0, 3.0 });
static_assert(cv.data[0] == 1.0 && cv.data[1] == 2.0 && cv.data[2] == 3.0);

template<typename F>
static F
touch_manifolds() noexcept
{
  const vec<F, 3> v{ F(1), F(2), F(3) };
  const vec<F, 3> w{ F(0.1), F(-0.2), F(0.3) };
  const auto R = lie::SO3<F>::exp_map(w);
  const auto Rn = lie::SO3<F>::from_quat_normalized(quat<F>{ F(0.1), F(0.2), F(0.3), F(0.9) });
  const auto Rb = lie::SO3<F>::between(R, Rn);
  const auto Ri = lie::SO3<F>::interpolate(R, Rn, F(0.25));
  F acc = lie::SO3<F>::inverse_rotate(Rb, v).data[0] + lie::SO3<F>::distance(R, Ri);

  const lie::SE3<F> T = lie::SE3<F>::from_rt(R, v);
  const auto Tm = lie::SE3<F>::from_matrix(lie::SE3<F>::to_matrix(T));
  const auto Tb = lie::SE3<F>::between(T, Tm);
  const vec<F, 6> xi{ F(1), F(2), F(3), F(0.1), F(0.2), F(0.3) };
  acc += lie::SE3<F>::act(T, v).data[0];
  acc += lie::SE3<F>::inverse_act(T, v).data[1];
  acc += lie::SE3<F>::adjoint_apply(Tb, xi).data[2];
  acc += lie::SE3<F>::coadjoint_apply(T, xi).data[3];
  acc += lie::SE3<F>::coadjoint(T).data[0];

  vec<F, 3> in[9]{}, out[9]{};
  for ( usize i = 0; i < 9; ++i ) in[i] = v;
  lie::rotate_many(R, in, out, 9);
  lie::inverse_rotate_many(R, out, out, 9);
  lie::transform_many(T, in, out, 9);
  lie::inverse_transform_many(T, out, out, 9);
  acc += out[0].data[0];

  F in_x[9]{}, in_y[9]{}, in_z[9]{};
  F out_x[9]{}, out_y[9]{}, out_z[9]{};
  const lie::vec3_soa_const_view<F> soa_in{ in_x, in_y, in_z };
  const lie::vec3_soa_view<F> soa_out{ out_x, out_y, out_z };
  lie::rotate_many_soa(R, soa_in, soa_out, 9);
  lie::inverse_rotate_many_soa(R, soa_out, soa_out, 9);
  lie::transform_many_soa(T, soa_in, soa_out, 9);
  lie::inverse_transform_many_soa(T, soa_out, soa_out, 9);
  acc += out_x[0];

  const auto e = mf::identity<lie::SO3<F>>();
  const auto rel = mf::between<lie::SO3<F>>(e, R);
  acc += mf::interpolate<lie::SO3<F>>(e, rel, F(0.5)).q.data[3];
  acc += mf::act<lie::SO3<F>>(R, v).data[0];

  const auto S2 = lie::SE2<F>::exp_map(vec<F, 3>{ F(1), F(2), F(0.3) });
  const auto S2m = lie::SE2<F>::from_matrix(lie::SE2<F>::to_matrix(S2));
  acc += lie::SE2<F>::adjoint_apply(S2m, vec<F, 3>{ F(1), F(2), F(3) }).data[0];

  const mat<F, 3, 3> P = mat<F, 3, 3>::identity();
  const auto roots = mf::spd<F, 3>::sqrt_pair_pd(P);
  acc += roots.root.data[0] + roots.inverse_root.data[4];
  return acc;
}

int
main()
{
  return static_cast<int>(touch_manifolds<f32>() + touch_manifolds<f64>());
}
