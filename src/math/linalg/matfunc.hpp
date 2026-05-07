//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// analytic functions of matrices
//  expm: pade with scaling-and-squaring
//  sqrtm: Hammarling Parlett recurrence
//  logm: Inverse scaling-squaring + pada

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "decomp.hpp"
#include "decomp_ext.hpp"
#include "generic.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace matfunc
{

namespace __impl_matfunc
{

template <ieee754_floating F, usize N>
inline void
nan_fill(mat<F, N, N> &X) noexcept
{
  const F nan = ieee::qnan_v<F>();
  for ( usize i = 0; i < N * N; ++i ) X.data[i] = nan;
}

template <ieee754_floating F>
inline bool
sqrt_2x2_block(F &x00, F &x01, F &x10, F &x11, F a, F b, F c, F d) noexcept
{
  F tr = a + d;
  F det = a * d - b * c;
  if ( det < F(0) ) return false;
  F sd = math::fsqrt(det);
  F tau_sq = tr + F(2) * sd;
  if ( tau_sq <= F(0) ) return false;
  F tau = math::fsqrt(tau_sq);
  F inv_tau = F(1) / tau;
  x00 = (a + sd) * inv_tau;
  x01 = b * inv_tau;
  x10 = c * inv_tau;
  x11 = (d + sd) * inv_tau;
  return true;
}

template <ieee754_floating F>
inline bool
log_2x2_block(F &x00, F &x01, F &x10, F &x11, F a, F b, F c, F d) noexcept
{
  F tr = a + d;
  F det = a * d - b * c;
  F disc = (tr * F(0.5)) * (tr * F(0.5)) - det;
  if ( disc >= F(0) ) {
    F sq = math::fsqrt(disc);
    F l1 = tr * F(0.5) + sq;
    F l2 = tr * F(0.5) - sq;
    if ( l1 <= F(0) || l2 <= F(0) ) return false;
    F denom = l1 - l2;
    if ( denom == F(0) ) {
      F l = math::log<F>(l1);
      x00 = l + (a - l1) / l1;
      x01 = b / l1;
      x10 = c / l1;
      x11 = l + (d - l1) / l1;
      return true;
    }
    F log_l1 = math::log<F>(l1);
    F log_l2 = math::log<F>(l2);
    F inv = F(1) / denom;
    F p1_00 = (a - l2) * inv;
    F p1_01 = b * inv;
    F p1_10 = c * inv;
    F p1_11 = (d - l2) * inv;
    F p2_00 = -(a - l1) * inv;
    F p2_01 = -b * inv;
    F p2_10 = -c * inv;
    F p2_11 = -(d - l1) * inv;
    x00 = log_l1 * p1_00 + log_l2 * p2_00;
    x01 = log_l1 * p1_01 + log_l2 * p2_01;
    x10 = log_l1 * p1_10 + log_l2 * p2_10;
    x11 = log_l1 * p1_11 + log_l2 * p2_11;
    return true;
  }
  F alpha = tr * F(0.5);
  F mu = math::fsqrt(-disc);
  if ( det <= F(0) ) return false;
  F log_r = F(0.5) * math::log<F>(det);
  F theta = math::atan2<F>(mu, alpha);
  F coeff = theta / mu;
  x00 = log_r + coeff * (a - alpha);
  x01 = coeff * b;
  x10 = coeff * c;
  x11 = log_r + coeff * (d - alpha);
  return true;
}

template <ieee754_floating F, usize N>
inline usize
identify_blocks(const mat<F, N, N> &T, usize *out_starts, usize *out_sizes) noexcept
{
  usize n_blocks = 0;
  usize i = 0;
  while ( i < N ) {
    bool is_2x2 = (i + 1 < N) && (T.data[(i + 1) * N + i] != F(0));
    out_starts[n_blocks] = i;
    out_sizes[n_blocks] = is_2x2 ? 2 : 1;
    ++n_blocks;
    i += is_2x2 ? 2 : 1;
  }
  return n_blocks;
}

template <ieee754_floating F, usize N>
inline bool
solve_sylvester_block(mat<F, N, N> &U, usize start_i, usize bi, usize start_j, usize bj, const F *rhs, F eps) noexcept
{
  if ( bi == 1 && bj == 1 ) {
    F u_ii = U.data[start_i * N + start_i];
    F u_jj = U.data[start_j * N + start_j];
    F denom = u_ii + u_jj;
    if ( math::fabs(denom) < eps ) return false;
    U.data[start_i * N + start_j] = rhs[0] / denom;
    return true;
  }
  if ( bi == 1 && bj == 2 ) {
    // u_ii * [x0 x1] + [x0 x1] * U_jj = rhs[0..2]
    // u_ii * x0 + x0 * U_jj[0,0] + x1 * U_jj[1,0] = rhs[0]
    // u_ii * x1 + x0 * U_jj[0,1] + x1 * U_jj[1,1] = rhs[1]
    F u_ii = U.data[start_i * N + start_i];
    F j00 = U.data[start_j * N + start_j];
    F j01 = U.data[start_j * N + (start_j + 1)];
    F j10 = U.data[(start_j + 1) * N + start_j];
    F j11 = U.data[(start_j + 1) * N + (start_j + 1)];
    mat<F, 2, 2> M{};
    M.data[0] = u_ii + j00;
    M.data[1] = j10;
    M.data[2] = j01;
    M.data[3] = u_ii + j11;
    F d = ops::det2<F>(M);
    if ( math::fabs(d) < eps ) return false;
    auto Mi = ops::inv2<F>(M);
    F x0 = Mi.data[0] * rhs[0] + Mi.data[1] * rhs[1];
    F x1 = Mi.data[2] * rhs[0] + Mi.data[3] * rhs[1];
    U.data[start_i * N + start_j] = x0;
    U.data[start_i * N + (start_j + 1)] = x1;
    return true;
  }
  if ( bi == 2 && bj == 1 ) {
    // U_ii * [x0; x1] + [x0; x1] * u_jj = rhs[0..2]
    // U_ii[0,0]*x0 + U_ii[0,1]*x1 + x0*u_jj = rhs[0]
    // U_ii[1,0]*x0 + U_ii[1,1]*x1 + x1*u_jj = rhs[1]
    F i00 = U.data[start_i * N + start_i];
    F i01 = U.data[start_i * N + (start_i + 1)];
    F i10 = U.data[(start_i + 1) * N + start_i];
    F i11 = U.data[(start_i + 1) * N + (start_i + 1)];
    F u_jj = U.data[start_j * N + start_j];
    mat<F, 2, 2> M{};
    M.data[0] = i00 + u_jj;
    M.data[1] = i01;
    M.data[2] = i10;
    M.data[3] = i11 + u_jj;
    F d = ops::det2<F>(M);
    if ( math::fabs(d) < eps ) return false;
    auto Mi = ops::inv2<F>(M);
    F x0 = Mi.data[0] * rhs[0] + Mi.data[1] * rhs[1];
    F x1 = Mi.data[2] * rhs[0] + Mi.data[3] * rhs[1];
    U.data[start_i * N + start_j] = x0;
    U.data[(start_i + 1) * N + start_j] = x1;
    return true;
  }
  if ( bi == 2 && bj == 2 ) {
    F i00 = U.data[start_i * N + start_i];
    F i01 = U.data[start_i * N + (start_i + 1)];
    F i10 = U.data[(start_i + 1) * N + start_i];
    F i11 = U.data[(start_i + 1) * N + (start_i + 1)];
    F j00 = U.data[start_j * N + start_j];
    F j01 = U.data[start_j * N + (start_j + 1)];
    F j10 = U.data[(start_j + 1) * N + start_j];
    F j11 = U.data[(start_j + 1) * N + (start_j + 1)];
    mat<F, 4, 4> M = mat<F, 4, 4>::zero();
    // row 0 (eqn r=0, c=0): X00 coefficient = i00 + j00; X01 coeff = j10; X10 coeff = i01; X11 coeff = 0
    M.data[0] = i00 + j00;
    M.data[1] = j10;
    M.data[2] = i01;
    M.data[3] = F(0);
    // row 1 (eqn r=0, c=1): X00 coeff = j01; X01 coeff = i00 + j11; X10 coeff = 0; X11 coeff = i01
    M.data[4] = j01;
    M.data[5] = i00 + j11;
    M.data[6] = F(0);
    M.data[7] = i01;
    // row 2 (eqn r=1, c=0): X00 coeff = i10; X01 coeff = 0; X10 coeff = i11 + j00; X11 coeff = j10
    M.data[8] = i10;
    M.data[9] = F(0);
    M.data[10] = i11 + j00;
    M.data[11] = j10;
    // row 3 (eqn r=1, c=1): X00 coeff = 0; X01 coeff = i10; X10 coeff = j01; X11 coeff = i11 + j11
    M.data[12] = F(0);
    M.data[13] = i10;
    M.data[14] = j01;
    M.data[15] = i11 + j11;
    F d = ops::det4<F>(M);
    if ( math::fabs(d) < eps ) return false;
    vec<F, 4> b{ rhs[0], rhs[1], rhs[2], rhs[3] };
    vec<F, 4> x = ops::solve4<F>(M, b);
    U.data[start_i * N + start_j] = x.data[0];
    U.data[start_i * N + (start_j + 1)] = x.data[1];
    U.data[(start_i + 1) * N + start_j] = x.data[2];
    U.data[(start_i + 1) * N + (start_j + 1)] = x.data[3];
    return true;
  }
  return false;
}

};     // namespace __impl_matfunc

template <ieee754_floating F, usize N> struct expm_result {
  mat<F, N, N> X;
  bool finite;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline expm_result<F, N>
expm(const mat<F, N, N> &A) noexcept
{
  expm_result<F, N> r{};
  r.X = mat<F, N, N>::zero();
  r.finite = true;
  if constexpr ( N == 0 ) {
    return r;
  } else {
    constexpr F b[14] = { F(64764752532480000.0),
                          F(32382376266240000.0),
                          F(7771770303897600.0),
                          F(1187353796428800.0),
                          F(129060195264000.0),
                          F(10559470521600.0),
                          F(670442572800.0),
                          F(33522128640.0),
                          F(1323241920.0),
                          F(40840800.0),
                          F(960960.0),
                          F(16380.0),
                          F(182.0),
                          F(1.0) };
    constexpr F theta13 = F(5.371920351148152);

    F alpha = norm_l1_mat<F, N, N>(A);
    if ( !ieee::is_finite(alpha) ) {
      r.finite = false;
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }

    int s = 0;
    F bound = theta13;
    while ( alpha > bound && s < 60 ) {
      bound *= F(2);
      ++s;
    }
    F scale_factor = F(1);
    for ( int i = 0; i < s; ++i ) scale_factor *= F(0.5);

    mat<F, N, N> B = A;
    for ( usize i = 0; i < N * N; ++i ) B.data[i] *= scale_factor;

    auto B2 = ops::gemm(B, B);
    auto B4 = ops::gemm(B2, B2);
    auto B6 = ops::gemm(B4, B2);

    mat<F, N, N> u_inner{};
    for ( usize i = 0; i < N * N; ++i ) u_inner.data[i] = b[13] * B6.data[i] + b[11] * B4.data[i] + b[9] * B2.data[i];
    auto u_mid = ops::gemm(B6, u_inner);
    mat<F, N, N> u_pre{};
    for ( usize i = 0; i < N * N; ++i ) u_pre.data[i] = u_mid.data[i] + b[7] * B6.data[i] + b[5] * B4.data[i] + b[3] * B2.data[i];
    for ( usize i = 0; i < N; ++i ) u_pre.data[i * N + i] += b[1];
    auto U = ops::gemm(B, u_pre);

    mat<F, N, N> v_inner{};
    for ( usize i = 0; i < N * N; ++i ) v_inner.data[i] = b[12] * B6.data[i] + b[10] * B4.data[i] + b[8] * B2.data[i];
    auto v_mid = ops::gemm(B6, v_inner);
    mat<F, N, N> V{};
    for ( usize i = 0; i < N * N; ++i ) V.data[i] = v_mid.data[i] + b[6] * B6.data[i] + b[4] * B4.data[i] + b[2] * B2.data[i];
    for ( usize i = 0; i < N; ++i ) V.data[i * N + i] += b[0];

    mat<F, N, N> VU_minus{}, VU_plus{};
    for ( usize i = 0; i < N * N; ++i ) {
      VU_minus.data[i] = V.data[i] - U.data[i];
      VU_plus.data[i] = V.data[i] + U.data[i];
    }
    auto VU_inv = inv<F, N>(VU_minus);
    if ( ieee::is_nan(VU_inv.data[0]) ) {
      r.finite = false;
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }
    auto R_mat = ops::gemm(VU_inv, VU_plus);

    for ( int i = 0; i < s; ++i ) {
      R_mat = ops::gemm(R_mat, R_mat);
      if ( !ieee::is_finite(R_mat.data[0]) ) {
        r.finite = false;
        __impl_matfunc::nan_fill<F, N>(r.X);
        return r;
      }
    }
    r.X = R_mat;
    return r;
  }
}

template <ieee754_floating F, usize N> struct parlett_sqrt_result {
  mat<F, N, N> U;
  bool real_sqrt_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline parlett_sqrt_result<F, N>
parlett_sqrt(const mat<F, N, N> &T) noexcept
{
  parlett_sqrt_result<F, N> r{};
  r.U = mat<F, N, N>::zero();
  r.real_sqrt_exists = true;
  r.converged = true;
  if constexpr ( N == 0 ) return r;

  const F eps = default_eps<F>();

  // Identify blocks
  usize block_starts[N + 1];
  usize block_sizes[N + 1];
  for ( usize i = 0; i <= N; ++i ) {
    block_starts[i] = 0;
    block_sizes[i] = 0;
  }
  usize n_blocks = __impl_matfunc::identify_blocks<F, N>(T, block_starts, block_sizes);

  for ( usize bk = 0; bk < n_blocks; ++bk ) {
    usize i = block_starts[bk];
    if ( block_sizes[bk] == 1 ) {
      F t = T.data[i * N + i];
      if ( t < F(0) ) {
        r.real_sqrt_exists = false;
        r.U.data[i * N + i] = ieee::qnan_v<F>();
      } else {
        r.U.data[i * N + i] = math::fsqrt(t);
      }
    } else {
      F a = T.data[i * N + i];
      F b = T.data[i * N + (i + 1)];
      F c = T.data[(i + 1) * N + i];
      F d = T.data[(i + 1) * N + (i + 1)];
      F x00, x01, x10, x11;
      if ( !__impl_matfunc::sqrt_2x2_block<F>(x00, x01, x10, x11, a, b, c, d) ) {
        r.real_sqrt_exists = false;
        r.U.data[i * N + i] = ieee::qnan_v<F>();
        r.U.data[i * N + (i + 1)] = ieee::qnan_v<F>();
        r.U.data[(i + 1) * N + i] = ieee::qnan_v<F>();
        r.U.data[(i + 1) * N + (i + 1)] = ieee::qnan_v<F>();
      } else {
        r.U.data[i * N + i] = x00;
        r.U.data[i * N + (i + 1)] = x01;
        r.U.data[(i + 1) * N + i] = x10;
        r.U.data[(i + 1) * N + (i + 1)] = x11;
      }
    }
  }

  if ( !r.real_sqrt_exists ) {
    __impl_matfunc::nan_fill<F, N>(r.U);
    r.converged = false;
    return r;
  }

  for ( usize bj = 1; bj < n_blocks; ++bj ) {
    for ( usize bi_signed = bj; bi_signed-- > 0; ) {
      usize bi = bi_signed;
      usize si = block_starts[bi];
      usize sj = block_starts[bj];
      usize bs_i = block_sizes[bi];
      usize bs_j = block_sizes[bj];
      F rhs[4] = { F(0), F(0), F(0), F(0) };
      for ( usize ii = 0; ii < bs_i; ++ii ) {
        for ( usize jj = 0; jj < bs_j; ++jj ) {
          F t = T.data[(si + ii) * N + (sj + jj)];
          F sum = F(0);
          for ( usize bk = bi + 1; bk < bj; ++bk ) {
            usize sk = block_starts[bk];
            usize bs_k = block_sizes[bk];
            for ( usize kk = 0; kk < bs_k; ++kk )
              sum = math::fma<F>(r.U.data[(si + ii) * N + (sk + kk)], r.U.data[(sk + kk) * N + (sj + jj)], sum);
          }
          rhs[ii * bs_j + jj] = t - sum;
        }
      }
      if ( !__impl_matfunc::solve_sylvester_block<F, N>(r.U, si, bs_i, sj, bs_j, rhs, eps) ) {
        r.real_sqrt_exists = false;
        r.converged = false;
        __impl_matfunc::nan_fill<F, N>(r.U);
        return r;
      }
    }
  }
  return r;
}

template <ieee754_floating F, usize N> struct sqrtm_result {
  mat<F, N, N> X;
  bool real_sqrt_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline sqrtm_result<F, N>
sqrtm(const mat<F, N, N> &A) noexcept
{
  sqrtm_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.real_sqrt_exists = true;
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    r.converged = sr.converged;
    if ( !sr.converged ) {
      r.real_sqrt_exists = false;
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }
    auto pr = parlett_sqrt<F, N>(sr.T);
    r.real_sqrt_exists = pr.real_sqrt_exists;
    r.converged = r.converged && pr.converged;
    if ( !pr.real_sqrt_exists ) {
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }
    // X = Z * U * Zᵀ
    auto Zt = ops::transpose(sr.Z);
    auto ZU = ops::gemm(sr.Z, pr.U);
    r.X = ops::gemm(ZU, Zt);
    return r;
  }
}

template <ieee754_floating F, usize N> struct parlett_log_result {
  mat<F, N, N> L;
  bool real_log_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline parlett_log_result<F, N>
parlett_log(const mat<F, N, N> &T_in) noexcept
{
  parlett_log_result<F, N> r{};
  r.L = mat<F, N, N>::zero();
  r.real_log_exists = true;
  r.converged = true;
  if constexpr ( N == 0 ) return r;

  for ( usize i = 0; i < N; ++i ) {
    bool is_2x2 = (i + 1 < N) && (T_in.data[(i + 1) * N + i] != F(0));
    if ( !is_2x2 ) {
      if ( T_in.data[i * N + i] <= F(0) ) {
        r.real_log_exists = false;
        __impl_matfunc::nan_fill<F, N>(r.L);
        return r;
      }
    } else {
      F a = T_in.data[i * N + i];
      F b = T_in.data[i * N + (i + 1)];
      F c = T_in.data[(i + 1) * N + i];
      F d = T_in.data[(i + 1) * N + (i + 1)];
      F det_block = a * d - b * c;
      if ( det_block <= F(0) ) {
        r.real_log_exists = false;
        __impl_matfunc::nan_fill<F, N>(r.L);
        return r;
      }
      ++i;
    }
  }

  constexpr F threshold = F(0.005);
  mat<F, N, N> T = T_in;
  int s = 0;
  while ( s < 60 ) {
    F diff = F(0);
    for ( usize i = 0; i < N; ++i ) {
      for ( usize j = 0; j < N; ++j ) {
        F e = T.data[i * N + j] - ((i == j) ? F(1) : F(0));
        diff += e * e;
      }
    }
    diff = math::fsqrt(diff);
    if ( diff < threshold ) break;
    auto pr = parlett_sqrt<F, N>(T);
    if ( !pr.real_sqrt_exists ) {
      r.real_log_exists = false;
      r.converged = false;
      __impl_matfunc::nan_fill<F, N>(r.L);
      return r;
    }
    T = pr.U;
    ++s;
  }
  if ( s >= 60 ) r.converged = false;

  mat<F, N, N> X = T;
  for ( usize i = 0; i < N; ++i ) X.data[i * N + i] -= F(1);

  mat<F, N, N> X_pow = X;     // X^1
  mat<F, N, N> L = mat<F, N, N>::zero();
  for ( usize i = 0; i < N * N; ++i ) L.data[i] = X.data[i];
  constexpr int n_terms = 20;
  F sign = F(-1);
  for ( int k = 2; k <= n_terms; ++k ) {
    X_pow = ops::gemm(X_pow, X);
    F coeff = sign / F(k);
    for ( usize i = 0; i < N * N; ++i ) L.data[i] += coeff * X_pow.data[i];
    sign = -sign;
  }

  F factor = F(1);
  for ( int i = 0; i < s; ++i ) factor *= F(2);
  for ( usize i = 0; i < N * N; ++i ) L.data[i] *= factor;
  r.L = L;
  return r;
}

template <ieee754_floating F, usize N> struct logm_result {
  mat<F, N, N> X;
  bool real_log_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline logm_result<F, N>
logm(const mat<F, N, N> &A) noexcept
{
  logm_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.real_log_exists = true;
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    r.converged = sr.converged;
    if ( !sr.converged ) {
      r.real_log_exists = false;
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }
    auto pl = parlett_log<F, N>(sr.T);
    r.real_log_exists = pl.real_log_exists;
    r.converged = r.converged && pl.converged;
    if ( !pl.real_log_exists ) {
      __impl_matfunc::nan_fill<F, N>(r.X);
      return r;
    }
    auto Zt = ops::transpose(sr.Z);
    auto ZL = ops::gemm(sr.Z, pl.L);
    r.X = ops::gemm(ZL, Zt);
    return r;
  }
}

};     // namespace matfunc
};     // namespace linalg
};     // namespace math
};     // namespace micron
