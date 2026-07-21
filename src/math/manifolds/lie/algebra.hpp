//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hat / vee operators; Lie bracket; generic algebra
//
//  -> hat_so3
//  -> vee_so3
//  -> hat_se3
//  -> vee_se3
//  -> hat_son<F,N>
//  -> vee_son<F,N>
//  -> lie_bracket
//  -> algebra_exp
//  -> algebra_log

#include "../../../concepts.hpp"
#include "../../../except.hpp"
#include "../../../types.hpp"
#include "../../linalg/matfunc.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../../quants/vec.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 3, 3>
hat_so3(const vec<F, 3> &omega) noexcept
{
  mat<F, 3, 3> M = mat<F, 3, 3>::zero();
  M.data[1] = -omega.data[2];
  M.data[2] = omega.data[1];
  M.data[3] = omega.data[2];
  M.data[5] = -omega.data[0];
  M.data[6] = -omega.data[1];
  M.data[7] = omega.data[0];
  return M;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
vee_so3(const mat<F, 3, 3> &M) noexcept
{
  return vec<F, 3>{ M.data[7], M.data[2], M.data[3] };
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr mat<F, 4, 4>
hat_se3(const vec<F, 6> &xi) noexcept
{
  mat<F, 4, 4> M = mat<F, 4, 4>::zero();
  M.data[0 * 4 + 1] = -xi.data[5];
  M.data[0 * 4 + 2] = xi.data[4];
  M.data[1 * 4 + 0] = xi.data[5];
  M.data[1 * 4 + 2] = -xi.data[3];
  M.data[2 * 4 + 0] = -xi.data[4];
  M.data[2 * 4 + 1] = xi.data[3];
  M.data[0 * 4 + 3] = xi.data[0];
  M.data[1 * 4 + 3] = xi.data[1];
  M.data[2 * 4 + 3] = xi.data[2];
  return M;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 6>
vee_se3(const mat<F, 4, 4> &M) noexcept
{
  return vec<F, 6>{ M.data[0 * 4 + 3], M.data[1 * 4 + 3], M.data[2 * 4 + 3], M.data[2 * 4 + 1], M.data[0 * 4 + 2], M.data[1 * 4 + 0] };
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, N, N>
hat_son(const mat<F, N, N> &M) noexcept
{
  return M;
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, N, N>
vee_son(const mat<F, N, N> &M) noexcept
{
  return M;
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::flatten]] inline constexpr mat<F, N, N>
lie_bracket(const mat<F, N, N> &X, const mat<F, N, N> &Y) noexcept
{
  const auto XY = linalg::ops::gemm(X, Y);
  const auto YX = linalg::ops::gemm(Y, X);
  return XY - YX;
}

template<ieee754_floating F, usize N>
[[nodiscard]] inline mat<F, N, N>
algebra_exp(const mat<F, N, N> &X) noexcept(!micron::except::__use_exceptions)
{
  const auto res = linalg::matfunc::expm<F, N>(X);
#if !defined(__micron_freestanding) || defined(__micron_eh)
  if ( !res.finite ) exc<except::range_error>("algebra_exp: matrix exponential did not produce a finite result");
#endif
  return res.X;
}

template<ieee754_floating F, usize N>
[[nodiscard]] inline mat<F, N, N>
algebra_log(const mat<F, N, N> &G) noexcept(!micron::except::__use_exceptions)
{
  const auto res = linalg::matfunc::logm<F, N>(G);
#if !defined(__micron_freestanding) || defined(__micron_eh)
  if ( !res.real_log_exists || !res.converged ) exc<except::domain_error>("algebra_log: matrix has no real logarithm or did not converge");
#endif
  return res.X;
}

};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron
