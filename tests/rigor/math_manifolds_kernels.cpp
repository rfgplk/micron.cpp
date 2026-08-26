//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/manifolds/manifolds.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::manifolds::lie;

namespace
{

u64 state = 0x7f4a7c159e3779b9ull;

[[nodiscard]] u64
next_u64() noexcept
{
  state ^= state >> 12;
  state ^= state << 25;
  state ^= state >> 27;
  return state * 0x2545f4914f6cdd1dull;
}

template<ieee754_floating F>
[[nodiscard]] F
sample() noexcept
{
  const i32 v = static_cast<i32>(next_u64() >> 32);
  return static_cast<F>(v) / F(2147483648.0);
}

template<ieee754_floating F>
[[nodiscard]] bool
same_bits(F a, F b) noexcept
{
  if constexpr ( same_as<F, f32> )
    return __builtin_bit_cast(u32, a) == __builtin_bit_cast(u32, b);
  else
    return __builtin_bit_cast(u64, a) == __builtin_bit_cast(u64, b);
}

template<ieee754_floating F>
[[nodiscard]] bool
rotate_is_bit_exact() noexcept
{
  for ( usize i = 0; i < 8192; ++i ) {
    const auto g = SO3<F>::exp_map(vec<F, 3>{ sample<F>(), sample<F>(), sample<F>() });
    const vec<F, 3> v{ sample<F>() * F(32), sample<F>() * F(32), sample<F>() * F(32) };
    const auto oracle = linalg::ops::rotate<F>(g.q, v);
    const auto got = SO3<F>::rotate(g, v);
    for ( usize lane = 0; lane < 3; ++lane )
      if ( !same_bits(got.data[lane], oracle.data[lane]) ) return false;
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] bool
same_vec_bits(const vec<F, 3> &a, const vec<F, 3> &b) noexcept
{
  for ( usize lane = 0; lane < 3; ++lane )
    if ( !same_bits(a.data[lane], b.data[lane]) ) return false;
  return true;
}

template<ieee754_floating F>
[[nodiscard]] bool
batch_is_bit_exact() noexcept
{
  constexpr usize count = 67;
  vec<F, 3> in[count], got[count], oracle[count];
  for ( usize i = 0; i < count; ++i ) in[i] = vec<F, 3>{ sample<F>() * F(32), sample<F>() * F(32), sample<F>() * F(32) };
  const auto R = SO3<F>::exp_map(vec<F, 3>{ sample<F>(), sample<F>(), sample<F>() });
  const SE3<F> T{ R, vec<F, 3>{ sample<F>(), sample<F>(), sample<F>() } };

  for ( usize n = 0; n <= count; ++n ) {
    rotate_many(R, in, got, n);
    for ( usize i = 0; i < n; ++i ) {
      oracle[i] = SO3<F>::rotate(R, in[i]);
      if ( !same_vec_bits(got[i], oracle[i]) ) return false;
    }

    transform_many(T, in, got, n);
    for ( usize i = 0; i < n; ++i ) {
      oracle[i] = SO3<F>::rotate(T.R, in[i]) + T.t;
      if ( !same_vec_bits(got[i], oracle[i]) ) return false;
    }
  }

  for ( usize i = 0; i < count; ++i ) got[i] = in[i];
  rotate_many(R, got, got, count);
  for ( usize i = 0; i < count; ++i )
    if ( !same_vec_bits(got[i], SO3<F>::rotate(R, in[i])) ) return false;

  inverse_rotate_many(R, in, got, count);
  const auto Rinv = SO3<F>::inverse(R);
  for ( usize i = 0; i < count; ++i )
    if ( !same_vec_bits(got[i], SO3<F>::rotate(Rinv, in[i])) ) return false;

  inverse_transform_many(T, in, got, count);
  const auto Tinv = SE3<F>::inverse(T);
  for ( usize i = 0; i < count; ++i )
    if ( !same_vec_bits(got[i], SO3<F>::rotate(Tinv.R, in[i]) + Tinv.t) ) return false;
  return true;
}

template<ieee754_floating F>
[[nodiscard]] bool
soa_batch_is_bit_exact() noexcept
{
  constexpr usize count = 67;
  F in_x[count], in_y[count], in_z[count];
  F out_x[count], out_y[count], out_z[count];
  for ( usize i = 0; i < count; ++i ) {
    in_x[i] = sample<F>() * F(32);
    in_y[i] = sample<F>() * F(32);
    in_z[i] = sample<F>() * F(32);
  }
  const auto R = SO3<F>::exp_map(vec<F, 3>{ sample<F>(), sample<F>(), sample<F>() });
  const SE3<F> T{ R, vec<F, 3>{ sample<F>(), sample<F>(), sample<F>() } };
  const vec3_soa_const_view<F> in{ in_x, in_y, in_z };
  const vec3_soa_view<F> out{ out_x, out_y, out_z };

  for ( usize n = 0; n <= count; ++n ) {
    rotate_many_soa(R, in, out, n);
    for ( usize i = 0; i < n; ++i ) {
      const auto oracle = SO3<F>::rotate(R, vec<F, 3>{ in_x[i], in_y[i], in_z[i] });
      if ( !same_bits(out_x[i], oracle.data[0]) || !same_bits(out_y[i], oracle.data[1]) || !same_bits(out_z[i], oracle.data[2]) )
        return false;
    }

    transform_many_soa(T, in, out, n);
    for ( usize i = 0; i < n; ++i ) {
      const auto oracle = SO3<F>::rotate(T.R, vec<F, 3>{ in_x[i], in_y[i], in_z[i] }) + T.t;
      if ( !same_bits(out_x[i], oracle.data[0]) || !same_bits(out_y[i], oracle.data[1]) || !same_bits(out_z[i], oracle.data[2]) )
        return false;
    }
  }

  for ( usize i = 0; i < count; ++i ) {
    out_x[i] = in_x[i];
    out_y[i] = in_y[i];
    out_z[i] = in_z[i];
  }
  const vec3_soa_view<F> in_place{ out_x, out_y, out_z };
  rotate_many_soa(R, in_place, in_place, count);
  for ( usize i = 0; i < count; ++i ) {
    const auto oracle = SO3<F>::rotate(R, vec<F, 3>{ in_x[i], in_y[i], in_z[i] });
    if ( !same_bits(out_x[i], oracle.data[0]) || !same_bits(out_y[i], oracle.data[1]) || !same_bits(out_z[i], oracle.data[2]) )
      return false;
  }

  inverse_rotate_many_soa(R, in, out, count);
  const auto Rinv = SO3<F>::inverse(R);
  for ( usize i = 0; i < count; ++i ) {
    const auto oracle = SO3<F>::rotate(Rinv, vec<F, 3>{ in_x[i], in_y[i], in_z[i] });
    if ( !same_bits(out_x[i], oracle.data[0]) || !same_bits(out_y[i], oracle.data[1]) || !same_bits(out_z[i], oracle.data[2]) )
      return false;
  }

  inverse_transform_many_soa(T, in, out, count);
  const auto Tinv = SE3<F>::inverse(T);
  for ( usize i = 0; i < count; ++i ) {
    const auto oracle = SO3<F>::rotate(Tinv.R, vec<F, 3>{ in_x[i], in_y[i], in_z[i] }) + Tinv.t;
    if ( !same_bits(out_x[i], oracle.data[0]) || !same_bits(out_y[i], oracle.data[1]) || !same_bits(out_z[i], oracle.data[2]) )
      return false;
  }
  return true;
}

template<ieee754_floating F, usize N>
[[nodiscard]] bool
same_mat_bits(const mat<F, N, N> &a, const mat<F, N, N> &b) noexcept
{
  for ( usize i = 0; i < N * N; ++i )
    if ( !same_bits(a.data[i], b.data[i]) ) return false;
  return true;
}

template<ieee754_floating F>
[[nodiscard]] bool
spd_pair_is_bit_exact() noexcept
{
  using SPD = manifolds::spd<F, 3>;
  for ( usize sample_i = 0; sample_i < 12; ++sample_i ) {
    const F a = sample<F>() * F(0.04), b = sample<F>() * F(0.04), c = sample<F>() * F(0.04);
    const mat<F, 3, 3> P{ F(1.5) + sample<F>() * F(0.1), a, b, a, F(2) + sample<F>() * F(0.1), c, b, c, F(2.5) + sample<F>() * F(0.1) };
    const F v01 = sample<F>() * F(0.02), v02 = sample<F>() * F(0.02), v12 = sample<F>() * F(0.02);
    const mat<F, 3, 3> V{ sample<F>() * F(0.03), v01, v02, v01, sample<F>() * F(0.03), v12, v02, v12, sample<F>() * F(0.03) };

    const auto root = SPD::sqrt_pd(P);
    const auto inverse_root = SPD::inv_sqrt_pd(P);
    const auto pair = SPD::sqrt_pair_pd(P);
    if ( !same_mat_bits(root, pair.root) || !same_mat_bits(inverse_root, pair.inverse_root) ) return false;

    const auto Vs = manifolds::__spd_impl::symmetrize<F, 3>(V);
    const auto inner = linalg::ops::gemm(linalg::ops::gemm(inverse_root, Vs), inverse_root);
    const auto E = linalg::matfunc::expm<F, 3>(inner).X;
    const auto exp_oracle = linalg::ops::gemm(linalg::ops::gemm(root, E), root);
    const auto exp_got = SPD::exp_map(P, V);
    if ( !same_mat_bits(exp_oracle, exp_got) ) return false;

    const auto log_inner = linalg::ops::gemm(linalg::ops::gemm(inverse_root, exp_oracle), inverse_root);
    const auto L = linalg::matfunc::logm<F, 3>(log_inner).X;
    const auto log_oracle = linalg::ops::gemm(linalg::ops::gemm(root, L), root);
    const auto log_got = SPD::log_map(P, exp_oracle);
    if ( !same_mat_bits(log_oracle, log_got) ) return false;
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] bool
sincos_is_bit_exact() noexcept
{
  for ( usize i = 0; i < 16384; ++i ) {
    const F x = sample<F>() * F(1000);
    F s, c;
    math::sincos<F>(x, s, c);
    if ( !same_bits(s, math::sin<F>(x)) || !same_bits(c, math::cos<F>(x)) ) return false;
  }
  return true;
}

};      // namespace

int
main()
{
  print("=== MATH::MANIFOLDS::KERNEL TESTS ===");

  test_case("SO3 SIMD rotate remains bit-exact to the scalar API");
  require_true(rotate_is_bit_exact<f32>());
  require_true(rotate_is_bit_exact<f64>());
  end_test_case();

  test_case("SO3/SE3 batch actions are bit-exact, tail-safe, and in-place safe");
  require_true(batch_is_bit_exact<f32>());
  require_true(batch_is_bit_exact<f64>());
  end_test_case();

  test_case("SO3/SE3 SoA actions are bit-exact, tail-safe, and in-place safe");
  require_true(soa_batch_is_bit_exact<f32>());
  require_true(soa_batch_is_bit_exact<f64>());
  end_test_case();

  test_case("SPD paired spectral decomposition preserves the former results bit-for-bit");
  require_true(spd_pair_is_bit_exact<f32>());
  require_true(spd_pair_is_bit_exact<f64>());
  end_test_case();

  test_case("shared manifold sin/cos reduction preserves both component results bit-for-bit");
  require_true(sincos_is_bit_exact<f32>());
  require_true(sincos_is_bit_exact<f64>());
  end_test_case();

  return 1;
}
