//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  Schur derived matrix fns

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../mk.hpp"
#include "decomp.hpp"
#include "decomp_ext.hpp"
#include "matfunc.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace schur
{

namespace __impl_matfunc_schur
{

//   val(x)             f(x) for real x
//   deriv(x)           f'(x) (used when a 2x2 block has a repeated eigenvalue)
//   complex(a,b,re,im) real and imaginary parts of f(a + i b)
template <ieee754_floating F> struct mfn_exp_desc {
  [[nodiscard, gnu::always_inline]] static inline F
  val(F x) noexcept
  {
    return math::exp<F>(x);
  }

  [[nodiscard, gnu::always_inline]] static inline F
  deriv(F x) noexcept
  {
    return math::exp<F>(x);
  }

  [[gnu::always_inline]] static inline void
  complex(F a, F b, F &re, F &im) noexcept
  {
    const F ea = math::exp<F>(a);
    re = ea * math::cos<F>(b);
    im = ea * math::sin<F>(b);
  }
};

template <ieee754_floating F> struct mfn_sin_desc {
  [[nodiscard, gnu::always_inline]] static inline F
  val(F x) noexcept
  {
    return math::sin<F>(x);
  }

  [[nodiscard, gnu::always_inline]] static inline F
  deriv(F x) noexcept
  {
    return math::cos<F>(x);
  }

  [[gnu::always_inline]] static inline void
  complex(F a, F b, F &re, F &im) noexcept
  {
    // sin(a+ib) = sin(a) cosh(b) + i cos(a) sinh(b)
    re = math::sin<F>(a) * math::cosh<F>(b);
    im = math::cos<F>(a) * math::sinh<F>(b);
  }
};

template <ieee754_floating F> struct mfn_cos_desc {
  [[nodiscard, gnu::always_inline]] static inline F
  val(F x) noexcept
  {
    return math::cos<F>(x);
  }

  [[nodiscard, gnu::always_inline]] static inline F
  deriv(F x) noexcept
  {
    return -math::sin<F>(x);
  }

  [[gnu::always_inline]] static inline void
  complex(F a, F b, F &re, F &im) noexcept
  {
    // cos(a+ib) = cos(a) cosh(b) - i sin(a) sinh(b)
    re = math::cos<F>(a) * math::cosh<F>(b);
    im = -math::sin<F>(a) * math::sinh<F>(b);
  }
};

template <ieee754_floating F> struct mfn_sinh_desc {
  [[nodiscard, gnu::always_inline]] static inline F
  val(F x) noexcept
  {
    return math::sinh<F>(x);
  }

  [[nodiscard, gnu::always_inline]] static inline F
  deriv(F x) noexcept
  {
    return math::cosh<F>(x);
  }

  [[gnu::always_inline]] static inline void
  complex(F a, F b, F &re, F &im) noexcept
  {
    // sinh(a+ib) = sinh(a) cos(b) + i cosh(a) sin(b)
    re = math::sinh<F>(a) * math::cos<F>(b);
    im = math::cosh<F>(a) * math::sin<F>(b);
  }
};

template <ieee754_floating F> struct mfn_cosh_desc {
  [[nodiscard, gnu::always_inline]] static inline F
  val(F x) noexcept
  {
    return math::cosh<F>(x);
  }

  [[nodiscard, gnu::always_inline]] static inline F
  deriv(F x) noexcept
  {
    return math::sinh<F>(x);
  }

  [[gnu::always_inline]] static inline void
  complex(F a, F b, F &re, F &im) noexcept
  {
    // cosh(a+ib) = cosh(a) cos(b) + i sinh(a) sin(b)
    re = math::cosh<F>(a) * math::cos<F>(b);
    im = math::sinh<F>(a) * math::sin<F>(b);
  }
};

// given a 2x2 block B = [[a, b], [c, d]] from real Schur form, computes the 2x2 matrix function f(B) in the {I, B} basis
// f(B) = coef_I * I + coef_M * B
template <typename FnDesc, ieee754_floating F>
[[gnu::flatten]] inline void
generic_2x2_block(F &x00, F &x01, F &x10, F &x11, F a, F b, F c, F d) noexcept
{
  const F tr = a + d;
  const F det = a * d - b * c;
  const F alpha = tr * F(0.5);
  const F disc = alpha * alpha - det;
  F coef_M;
  F coef_I;
  if ( disc > F(0) ) {
    const F sq = math::fsqrt(disc);
    const F l1 = alpha + sq;
    const F l2 = alpha - sq;
    const F f1 = FnDesc::val(l1);
    const F f2 = FnDesc::val(l2);
    coef_M = (f1 - f2) / (l1 - l2);
    coef_I = f1 - coef_M * l1;
  } else if ( disc < F(0) ) {
    const F beta = math::fsqrt(-disc);
    F re_f;
    F im_f;
    FnDesc::complex(alpha, beta, re_f, im_f);
    coef_M = im_f / beta;
    coef_I = re_f - alpha * coef_M;
  } else {
    const F fa = FnDesc::val(alpha);
    const F fp = FnDesc::deriv(alpha);
    coef_M = fp;
    coef_I = fa - alpha * fp;
  }
  x00 = coef_M * a + coef_I;
  x01 = coef_M * b;
  x10 = coef_M * c;
  x11 = coef_M * d + coef_I;
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
exp_2x2_block(F &x00, F &x01, F &x10, F &x11, F a, F b, F c, F d) noexcept
{
  const F tr = a + d;
  const F det = a * d - b * c;
  const F alpha = tr * F(0.5);
  const F disc = alpha * alpha - det;
  F coef_M;     // alpha
  F coef_I;     // beta
  if ( disc > F(0) ) {
    const F sq = math::fsqrt(disc);
    const F l1 = alpha + sq;
    const F l2 = alpha - sq;
    const F e1 = math::exp<F>(l1);
    const F e2 = math::exp<F>(l2);
    coef_M = (e1 - e2) / (l1 - l2);
    coef_I = e1 - coef_M * l1;
  } else if ( disc < F(0) ) {
    const F beta = math::fsqrt(-disc);
    const F ea = math::exp<F>(alpha);
    const F sb = math::sin<F>(beta);
    const F cb = math::cos<F>(beta);
    const F sb_over_b = sb / beta;
    coef_M = ea * sb_over_b;
    coef_I = ea * (cb - alpha * sb_over_b);
  } else {
    const F ea = math::exp<F>(alpha);
    coef_M = ea;
    coef_I = ea * (F(1) - alpha);
  }
  x00 = coef_M * a + coef_I;
  x01 = coef_M * b;
  x10 = coef_M * c;
  x11 = coef_M * d + coef_I;
}

template <ieee754_floating F>
inline bool
solve_sylvester_minus(F a00, F a01, F a10, F a11, usize bi, F b00, F b01, F b10, F b11, usize bj, const F *rhs, F *out, F eps) noexcept
{
  if ( bi == 1 && bj == 1 ) {
    const F denom = a00 - b00;
    if ( math::fabs(denom) < eps ) return false;
    out[0] = rhs[0] / denom;
    return true;
  }
  if ( bi == 1 && bj == 2 ) {
    // a00*[x0 x1] − [x0 x1]*B = rhs
    // a00*x0 − (x0*b00 + x1*b10) = rhs[0]
    // a00*x1 − (x0*b01 + x1*b11) = rhs[1]
    mat<F, 2, 2> M{};
    M.data[0] = a00 - b00;
    M.data[1] = -b10;
    M.data[2] = -b01;
    M.data[3] = a00 - b11;
    F dM = ops::det2<F>(M);
    if ( math::fabs(dM) < eps ) return false;
    auto Mi = ops::inv2<F>(M);
    out[0] = Mi.data[0] * rhs[0] + Mi.data[1] * rhs[1];
    out[1] = Mi.data[2] * rhs[0] + Mi.data[3] * rhs[1];
    return true;
  }
  if ( bi == 2 && bj == 1 ) {
    // A*[x0; x1] − [x0; x1]*b00 = rhs
    // (A[0,0] − b00)*x0 + A[0,1]*x1 = rhs[0]
    // A[1,0]*x0 + (A[1,1] − b00)*x1 = rhs[1]
    mat<F, 2, 2> M{};
    M.data[0] = a00 - b00;
    M.data[1] = a01;
    M.data[2] = a10;
    M.data[3] = a11 - b00;
    F dM = ops::det2<F>(M);
    if ( math::fabs(dM) < eps ) return false;
    auto Mi = ops::inv2<F>(M);
    out[0] = Mi.data[0] * rhs[0] + Mi.data[1] * rhs[1];
    out[1] = Mi.data[2] * rhs[0] + Mi.data[3] * rhs[1];
    return true;
  }
  if ( bi == 2 && bj == 2 ) {
    // (A*X)[r, c] = A[r, 0]*X[0, c] + A[r, 1]*X[1, c]
    // (X*B)[r, c] = X[r, 0]*B[0, c] + X[r, 1]*B[1, c]
    mat<F, 4, 4> M = mat<F, 4, 4>::zero();
    F *__restrict__ Mp = M.data;
    // (r, c) = (0, 0)
    Mp[0] = a00 - b00;
    Mp[1] = -b10;
    Mp[2] = a01;
    Mp[3] = F(0);
    // (r, c) = (0, 1)
    Mp[4] = -b01;
    Mp[5] = a00 - b11;
    Mp[6] = F(0);
    Mp[7] = a01;
    // (r, c) = (1, 0)
    Mp[8] = a10;
    Mp[9] = F(0);
    Mp[10] = a11 - b00;
    Mp[11] = -b10;
    // (r, c) = (1, 1)
    Mp[12] = F(0);
    Mp[13] = a10;
    Mp[14] = -b01;
    Mp[15] = a11 - b11;

    auto lu = decomp::lu_pivot<F, 4>(M);
    if ( lu.singular ) return false;
    vec<F, 4> r{};
    F *__restrict__ rp = r.data;
    rp[0] = rhs[0];
    rp[1] = rhs[1];
    rp[2] = rhs[2];
    rp[3] = rhs[3];
    decomp::lu_solve<F, 4>(lu, r);
    out[0] = rp[0];
    out[1] = rp[1];
    out[2] = rp[2];
    out[3] = rp[3];
    return true;
  }
  return false;
}

template <ieee754_floating F, usize N> struct parlett_exp_result {
  mat<F, N, N> F_T;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline parlett_exp_result<F, N>
parlett_exp(const mat<F, N, N> &T) noexcept
{
  parlett_exp_result<F, N> r{ mat<F, N, N>::zero(), true };
  if constexpr ( N == 0 ) return r;

  const F eps = default_eps<F>();
  usize block_starts[N + 1];
  usize block_sizes[N + 1];
  for ( usize i = 0; i <= N; ++i ) {
    block_starts[i] = 0;
    block_sizes[i] = 0;
  }
  const usize n_blocks = matfunc::__impl_matfunc::identify_blocks<F, N>(T, block_starts, block_sizes);

  F *__restrict__ F_T = r.F_T.data;
  const F *__restrict__ Tp = T.data;
  for ( usize bk = 0; bk < n_blocks; ++bk ) {
    const usize i = block_starts[bk];
    if ( block_sizes[bk] == 1 ) {
      F_T[i * N + i] = math::exp<F>(Tp[i * N + i]);
    } else {
      F a = Tp[i * N + i];
      F b = Tp[i * N + (i + 1)];
      F c = Tp[(i + 1) * N + i];
      F d = Tp[(i + 1) * N + (i + 1)];
      F x00, x01, x10, x11;
      exp_2x2_block<F>(x00, x01, x10, x11, a, b, c, d);
      F_T[i * N + i] = x00;
      F_T[i * N + (i + 1)] = x01;
      F_T[(i + 1) * N + i] = x10;
      F_T[(i + 1) * N + (i + 1)] = x11;
    }
  }

  for ( usize offset = 1; offset < n_blocks; ++offset ) {
    for ( usize bi = 0; bi + offset < n_blocks; ++bi ) {
      const usize bj = bi + offset;
      const usize si = block_starts[bi];
      const usize sj = block_starts[bj];
      const usize bs_i = block_sizes[bi];
      const usize bs_j = block_sizes[bj];
      F rhs[4] = { F(0), F(0), F(0), F(0) };
      for ( usize ii = 0; ii < bs_i; ++ii ) {
        for ( usize jj = 0; jj < bs_j; ++jj ) {
          F sum = F(0);
          for ( usize kk = 0; kk < bs_i; ++kk ) sum = math::fma<F>(F_T[(si + ii) * N + (si + kk)], Tp[(si + kk) * N + (sj + jj)], sum);
          for ( usize kk = 0; kk < bs_j; ++kk ) sum = math::fma<F>(-Tp[(si + ii) * N + (sj + kk)], F_T[(sj + kk) * N + (sj + jj)], sum);
          rhs[ii * bs_j + jj] = sum;
        }
      }
      for ( usize bk = bi + 1; bk < bj; ++bk ) {
        const usize sk = block_starts[bk];
        const usize bs_k = block_sizes[bk];
        for ( usize ii = 0; ii < bs_i; ++ii ) {
          for ( usize jj = 0; jj < bs_j; ++jj ) {
            F sum = F(0);
            for ( usize kk = 0; kk < bs_k; ++kk ) sum = math::fma<F>(F_T[(si + ii) * N + (sk + kk)], Tp[(sk + kk) * N + (sj + jj)], sum);
            for ( usize kk = 0; kk < bs_k; ++kk ) sum = math::fma<F>(-Tp[(si + ii) * N + (sk + kk)], F_T[(sk + kk) * N + (sj + jj)], sum);
            rhs[ii * bs_j + jj] += sum;
          }
        }
      }
      F a00 = Tp[si * N + si];
      F a01 = (bs_i == 2) ? Tp[si * N + (si + 1)] : F(0);
      F a10 = (bs_i == 2) ? Tp[(si + 1) * N + si] : F(0);
      F a11 = (bs_i == 2) ? Tp[(si + 1) * N + (si + 1)] : F(0);
      F b00 = Tp[sj * N + sj];
      F b01 = (bs_j == 2) ? Tp[sj * N + (sj + 1)] : F(0);
      F b10 = (bs_j == 2) ? Tp[(sj + 1) * N + sj] : F(0);
      F b11 = (bs_j == 2) ? Tp[(sj + 1) * N + (sj + 1)] : F(0);
      F out[4] = { F(0), F(0), F(0), F(0) };
      if ( !solve_sylvester_minus<F>(a00, a01, a10, a11, bs_i, b00, b01, b10, b11, bs_j, rhs, out, eps) ) {
        r.converged = false;
        return r;
      }
      for ( usize ii = 0; ii < bs_i; ++ii )
        for ( usize jj = 0; jj < bs_j; ++jj ) F_T[(si + ii) * N + (sj + jj)] = out[ii * bs_j + jj];
    }
  }
  return r;
}

template <ieee754_floating F, usize N> struct parlett_func_result {
  mat<F, N, N> F_T;
  bool converged;
};

template <typename FnDesc, ieee754_floating F, usize N>
[[nodiscard]] inline parlett_func_result<F, N>
parlett_func(const mat<F, N, N> &T) noexcept
{
  parlett_func_result<F, N> r{ mat<F, N, N>::zero(), true };
  if constexpr ( N == 0 ) return r;

  const F eps = default_eps<F>();
  usize block_starts[N + 1];
  usize block_sizes[N + 1];
  for ( usize i = 0; i <= N; ++i ) {
    block_starts[i] = 0;
    block_sizes[i] = 0;
  }
  const usize n_blocks = matfunc::__impl_matfunc::identify_blocks<F, N>(T, block_starts, block_sizes);

  F *__restrict__ F_T = r.F_T.data;
  const F *__restrict__ Tp = T.data;
  for ( usize bk = 0; bk < n_blocks; ++bk ) {
    const usize i = block_starts[bk];
    if ( block_sizes[bk] == 1 ) {
      F_T[i * N + i] = FnDesc::val(Tp[i * N + i]);
    } else {
      F a = Tp[i * N + i];
      F b = Tp[i * N + (i + 1)];
      F c = Tp[(i + 1) * N + i];
      F d = Tp[(i + 1) * N + (i + 1)];
      F x00, x01, x10, x11;
      generic_2x2_block<FnDesc, F>(x00, x01, x10, x11, a, b, c, d);
      F_T[i * N + i] = x00;
      F_T[i * N + (i + 1)] = x01;
      F_T[(i + 1) * N + i] = x10;
      F_T[(i + 1) * N + (i + 1)] = x11;
    }
  }

  for ( usize offset = 1; offset < n_blocks; ++offset ) {
    for ( usize bi = 0; bi + offset < n_blocks; ++bi ) {
      const usize bj = bi + offset;
      const usize si = block_starts[bi];
      const usize sj = block_starts[bj];
      const usize bs_i = block_sizes[bi];
      const usize bs_j = block_sizes[bj];
      F rhs[4] = { F(0), F(0), F(0), F(0) };
      for ( usize ii = 0; ii < bs_i; ++ii ) {
        for ( usize jj = 0; jj < bs_j; ++jj ) {
          F sum = F(0);
          for ( usize kk = 0; kk < bs_i; ++kk ) sum = math::fma<F>(F_T[(si + ii) * N + (si + kk)], Tp[(si + kk) * N + (sj + jj)], sum);
          for ( usize kk = 0; kk < bs_j; ++kk ) sum = math::fma<F>(-Tp[(si + ii) * N + (sj + kk)], F_T[(sj + kk) * N + (sj + jj)], sum);
          rhs[ii * bs_j + jj] = sum;
        }
      }
      for ( usize bk = bi + 1; bk < bj; ++bk ) {
        const usize sk = block_starts[bk];
        const usize bs_k = block_sizes[bk];
        for ( usize ii = 0; ii < bs_i; ++ii ) {
          for ( usize jj = 0; jj < bs_j; ++jj ) {
            F sum = F(0);
            for ( usize kk = 0; kk < bs_k; ++kk ) sum = math::fma<F>(F_T[(si + ii) * N + (sk + kk)], Tp[(sk + kk) * N + (sj + jj)], sum);
            for ( usize kk = 0; kk < bs_k; ++kk ) sum = math::fma<F>(-Tp[(si + ii) * N + (sk + kk)], F_T[(sk + kk) * N + (sj + jj)], sum);
            rhs[ii * bs_j + jj] += sum;
          }
        }
      }
      F a00 = Tp[si * N + si];
      F a01 = (bs_i == 2) ? Tp[si * N + (si + 1)] : F(0);
      F a10 = (bs_i == 2) ? Tp[(si + 1) * N + si] : F(0);
      F a11 = (bs_i == 2) ? Tp[(si + 1) * N + (si + 1)] : F(0);
      F b00 = Tp[sj * N + sj];
      F b01 = (bs_j == 2) ? Tp[sj * N + (sj + 1)] : F(0);
      F b10 = (bs_j == 2) ? Tp[(sj + 1) * N + sj] : F(0);
      F b11 = (bs_j == 2) ? Tp[(sj + 1) * N + (sj + 1)] : F(0);
      F out[4] = { F(0), F(0), F(0), F(0) };
      if ( !solve_sylvester_minus<F>(a00, a01, a10, a11, bs_i, b00, b01, b10, b11, bs_j, rhs, out, eps) ) {
        r.converged = false;
        return r;
      }
      for ( usize ii = 0; ii < bs_i; ++ii )
        for ( usize jj = 0; jj < bs_j; ++jj ) F_T[(si + ii) * N + (sj + jj)] = out[ii * bs_j + jj];
    }
  }
  return r;
}

};     // namespace __impl_matfunc_schur

template <ieee754_floating F, usize N> struct expmat_result {
  mat<F, N, N> X;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline expmat_result<F, N>
expmat(const mat<F, N, N> &A) noexcept
{
  expmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    if ( sr.converged ) {
      auto pr = __impl_matfunc_schur::parlett_exp<F, N>(sr.T);
      if ( pr.converged ) {
        // X = Z · F_T · Zᵀ
        mat<F, N, N> ZF = mat<F, N, N>::zero();
        for ( usize i = 0; i < N; ++i ) {
          F *__restrict__ row_zf = ZF.data + i * N;
          const F *__restrict__ row_z = sr.Z.data + i * N;
          for ( usize j = 0; j < N; ++j ) {
            F s = F(0);
            const F *__restrict__ col_f = pr.F_T.data + j;
            for ( usize k = 0; k < N; ++k ) s = math::fma<F>(row_z[k], col_f[k * N], s);
            row_zf[j] = s;
          }
        }
        r.X = mat<F, N, N>::zero();
        for ( usize i = 0; i < N; ++i ) {
          F *__restrict__ row_x = r.X.data + i * N;
          const F *__restrict__ row_zf = ZF.data + i * N;
          for ( usize j = 0; j < N; ++j ) {
            F s = F(0);
            const F *__restrict__ row_z_j = sr.Z.data + j * N;     // Zᵀ col j == Z row j
            for ( usize k = 0; k < N; ++k ) s = math::fma<F>(row_zf[k], row_z_j[k], s);
            row_x[j] = s;
          }
        }
        r.converged = true;
        return r;
      }
    }
    auto fb = matfunc::expm<F, N>(A);
    r.X = fb.X;
    r.converged = fb.finite;
    return r;
  }
}

template <ieee754_floating F, usize N> struct sqrtmat_result {
  mat<F, N, N> X;
  bool real_sqrt_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline sqrtmat_result<F, N>
sqrtmat(const mat<F, N, N> &A) noexcept
{
  auto r = matfunc::sqrtm<F, N>(A);
  return { r.X, r.real_sqrt_exists, r.converged };
}

template <ieee754_floating F, usize N> struct logmat_result {
  mat<F, N, N> X;
  bool real_log_exists;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline logmat_result<F, N>
logmat(const mat<F, N, N> &A) noexcept
{
  auto r = matfunc::logm<F, N>(A);
  return { r.X, r.real_log_exists, r.converged };
}

// generic matrix function via Schur + Parlett
namespace __impl_matfunc_schur
{

template <ieee754_floating F, usize N>
inline mat<F, N, N>
back_transform(const mat<F, N, N> &Z, const mat<F, N, N> &F_T) noexcept
{
  // ZF = Z * F_T
  mat<F, N, N> ZF = mat<F, N, N>::zero();
  for ( usize i = 0; i < N; ++i ) {
    F *__restrict__ row_zf = ZF.data + i * N;
    const F *__restrict__ row_z = Z.data + i * N;
    for ( usize j = 0; j < N; ++j ) {
      F s = F(0);
      const F *__restrict__ col_f = F_T.data + j;
      for ( usize k = 0; k < N; ++k ) s = math::fma<F>(row_z[k], col_f[k * N], s);
      row_zf[j] = s;
    }
  }
  // X = ZF * Z_T
  mat<F, N, N> X = mat<F, N, N>::zero();
  for ( usize i = 0; i < N; ++i ) {
    F *__restrict__ row_x = X.data + i * N;
    const F *__restrict__ row_zf = ZF.data + i * N;
    for ( usize j = 0; j < N; ++j ) {
      F s = F(0);
      const F *__restrict__ row_z_j = Z.data + j * N;     // Zᵀ col j == Z row j
      for ( usize k = 0; k < N; ++k ) s = math::fma<F>(row_zf[k], row_z_j[k], s);
      row_x[j] = s;
    }
  }
  return X;
}

};     // namespace __impl_matfunc_schur

template <ieee754_floating F, usize N> struct sinmat_result {
  mat<F, N, N> X;
  bool converged;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trigs

template <ieee754_floating F, usize N>
[[nodiscard]] inline sinmat_result<F, N>
sinmat(const mat<F, N, N> &A) noexcept
{
  sinmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    if ( !sr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    auto pr = __impl_matfunc_schur::parlett_func<__impl_matfunc_schur::mfn_sin_desc<F>, F, N>(sr.T);
    if ( !pr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    r.X = __impl_matfunc_schur::back_transform<F, N>(sr.Z, pr.F_T);
    r.converged = true;
    return r;
  }
}

template <ieee754_floating F, usize N> struct cosmat_result {
  mat<F, N, N> X;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline cosmat_result<F, N>
cosmat(const mat<F, N, N> &A) noexcept
{
  cosmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    if ( !sr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    auto pr = __impl_matfunc_schur::parlett_func<__impl_matfunc_schur::mfn_cos_desc<F>, F, N>(sr.T);
    if ( !pr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    r.X = __impl_matfunc_schur::back_transform<F, N>(sr.Z, pr.F_T);
    r.converged = true;
    return r;
  }
}

template <ieee754_floating F, usize N> struct sinhmat_result {
  mat<F, N, N> X;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline sinhmat_result<F, N>
sinhmat(const mat<F, N, N> &A) noexcept
{
  sinhmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    if ( !sr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    auto pr = __impl_matfunc_schur::parlett_func<__impl_matfunc_schur::mfn_sinh_desc<F>, F, N>(sr.T);
    if ( !pr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    r.X = __impl_matfunc_schur::back_transform<F, N>(sr.Z, pr.F_T);
    r.converged = true;
    return r;
  }
}

template <ieee754_floating F, usize N> struct coshmat_result {
  mat<F, N, N> X;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline coshmat_result<F, N>
coshmat(const mat<F, N, N> &A) noexcept
{
  coshmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else {
    auto sr = decomp::schur<F, N>(A);
    if ( !sr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    auto pr = __impl_matfunc_schur::parlett_func<__impl_matfunc_schur::mfn_cosh_desc<F>, F, N>(sr.T);
    if ( !pr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    r.X = __impl_matfunc_schur::back_transform<F, N>(sr.Z, pr.F_T);
    r.converged = true;
    return r;
  }
}

// matrix power  A^p
template <ieee754_floating F, usize N> struct powmat_result {
  mat<F, N, N> X;
  bool converged;
};

namespace __impl_matfunc_schur
{

template <ieee754_floating F, usize N>
inline mat<F, N, N>
matmul(const mat<F, N, N> &A, const mat<F, N, N> &B) noexcept
{
  mat<F, N, N> C = mat<F, N, N>::zero();
  for ( usize i = 0; i < N; ++i ) {
    const F *__restrict__ row_a = A.data + i * N;
    F *__restrict__ row_c = C.data + i * N;
    for ( usize j = 0; j < N; ++j ) {
      F s = F(0);
      const F *__restrict__ col_b = B.data + j;
      for ( usize k = 0; k < N; ++k ) s = math::fma<F>(row_a[k], col_b[k * N], s);
      row_c[j] = s;
    }
  }
  return C;
}

template <ieee754_floating F>
[[gnu::always_inline]] inline bool
is_integer_value(F p) noexcept
{
  const F r = F(static_cast<long long>(p));
  return r == p;
}

};     // namespace __impl_matfunc_schur

template <ieee754_floating F, usize N>
[[nodiscard]] inline powmat_result<F, N>
powmat(const mat<F, N, N> &A, F p) noexcept
{
  powmat_result<F, N> r{};
  if constexpr ( N == 0 ) {
    r.X = mat<F, N, N>::zero();
    r.converged = true;
    return r;
  } else if ( __impl_matfunc_schur::is_integer_value<F>(p) ) {
    long long e = static_cast<long long>(p);
    if ( e == 0 ) {
      r.X = mat<F, N, N>::identity();
      r.converged = true;
      return r;
    }
    bool negate = (e < 0);
    unsigned long long n = negate ? static_cast<unsigned long long>(-e) : static_cast<unsigned long long>(e);
    mat<F, N, N> base = A;
    mat<F, N, N> acc = mat<F, N, N>::identity();
    bool have_acc = false;
    while ( n > 0 ) {
      if ( n & 1ULL ) {
        if ( !have_acc ) {
          acc = base;
          have_acc = true;
        } else {
          acc = __impl_matfunc_schur::matmul<F, N>(acc, base);
        }
      }
      n >>= 1ULL;
      if ( n > 0 ) base = __impl_matfunc_schur::matmul<F, N>(base, base);
    }
    if ( negate ) {
      auto inv_r = decomp::inv<F, N>(acc);
      if ( inv_r.singular ) {
        r.X = mat<F, N, N>::zero();
        r.converged = false;
        return r;
      }
      r.X = inv_r.X;
    } else {
      r.X = acc;
    }
    r.converged = true;
    return r;
  } else {
    auto lr = logmat<F, N>(A);
    if ( !lr.converged ) {
      r.X = mat<F, N, N>::zero();
      r.converged = false;
      return r;
    }
    mat<F, N, N> pL = lr.X;
    for ( usize i = 0; i < N * N; ++i ) pL.data[i] = p * pL.data[i];
    auto er = expmat<F, N>(pL);
    r.X = er.X;
    r.converged = er.converged;
    return r;
  }
}

};     // namespace schur
};     // namespace linalg
};     // namespace math
};     // namespace micron
