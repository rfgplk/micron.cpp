//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// narrow-band  solvers
//
// -> tridiag_solve
// -> pent_spd_solve
// -> pent_spd_factor
// -> pent_spd_solve_factored
// -> pent_spd_takahashi

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{

// Thomas algorithm
//
//   a : sub-diagonal,   length n-1   (a[i] = A[i+1, i])
//   b : main diagonal,  length n
//   c : super-diagonal, length n-1   (c[i] = A[i, i+1])
//   d : right-hand side,length n
//
// NOTE: A must be diagonally dominant
template<ieee754_floating F>
[[gnu::flatten]] inline void
tridiag_solve(F *__restrict__ a, F *__restrict__ b, F *__restrict__ c, F *__restrict__ d, usize n) noexcept
{
  if ( n == 0 ) return;
  // pivot guard
  constexpr F tiny = default_eps<F>() * F(4);
  if ( n == 1 ) {
    d[0] = (math::fabs(b[0]) > tiny) ? d[0] / b[0] : F(0);
    return;
  }

  if ( math::fabs(b[0]) > tiny ) {
    const F inv0 = F(1) / b[0];
    c[0] = c[0] * inv0;
    d[0] = d[0] * inv0;
  } else {
    c[0] = F(0);
    d[0] = F(0);
  }
  for ( usize i = 1; i < n; ++i ) {
    const F m = b[i] - a[i - 1] * c[i - 1];
    if ( math::fabs(m) > tiny ) {
      const F inv_m = F(1) / m;
      if ( i + 1 < n ) c[i] = c[i] * inv_m;
      d[i] = (d[i] - a[i - 1] * d[i - 1]) * inv_m;
    } else {
      if ( i + 1 < n ) c[i] = F(0);
      d[i] = F(0);
    }
  }

  for ( usize i = n - 1; i-- > 0; ) d[i] = d[i] - c[i] * d[i + 1];
}

// LDL^T factorisation of a 5-banded SPD A
//
//   d2  : offset-2 super-diagonal,  length n-2  (d2[i] = A[i, i+2])
//   d1  : offset-1 super-diagonal,  length n-1  (d1[i] = A[i, i+1])
//   diag: main diagonal,            length n
template<ieee754_floating F>
[[nodiscard]] [[gnu::flatten]] inline bool
pent_spd_factor(F *__restrict__ d2, F *__restrict__ d1, F *__restrict__ diag, usize n) noexcept
{
  if ( n == 0 ) return true;
  if ( n == 1 ) return diag[0] > F(0);
  for ( usize i = 0; i < n; ++i ) {
    F l2_i = F(0);
    F l1_i = F(0);
    if ( i >= 2 ) {
      l2_i = d2[i - 2] / diag[i - 2];
      d2[i - 2] = l2_i;
    }
    if ( i >= 1 ) {
      F num = d1[i - 1];
      if ( i >= 2 ) num -= l2_i * diag[i - 2] * d1[i - 2];
      l1_i = num / diag[i - 1];
      d1[i - 1] = l1_i;
    }
    F D_i = diag[i];
    if ( i >= 1 ) D_i -= l1_i * l1_i * diag[i - 1];
    if ( i >= 2 ) D_i -= l2_i * l2_i * diag[i - 2];
    if ( D_i <= F(0) ) return false;
    diag[i] = D_i;
  }
  return true;
}

// triangular given an LDL^T factor
//
//   l2  : L offset-2 (length n-2)
//   l1  : L offset-1 (length n-1)
//   D   : diagonal of D (length n)
//   rhs : right-hand side (length n)
template<ieee754_floating F>
[[gnu::flatten]] inline void
pent_spd_solve_factored(const F *__restrict__ l2, const F *__restrict__ l1, const F *__restrict__ D, F *__restrict__ rhs, usize n) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    rhs[0] = rhs[0] / D[0];
    return;
  }
  // L y = b
  for ( usize i = 1; i < n; ++i ) {
    F y = rhs[i] - l1[i - 1] * rhs[i - 1];
    if ( i >= 2 ) y -= l2[i - 2] * rhs[i - 2];
    rhs[i] = y;
  }
  for ( usize i = 0; i < n; ++i ) rhs[i] = rhs[i] / D[i];
  // L^T x = z
  for ( usize i = n - 1; i-- > 0; ) {
    F x = rhs[i] - l1[i] * rhs[i + 1];
    if ( i + 2 < n ) x -= l2[i] * rhs[i + 2];
    rhs[i] = x;
  }
}

template<ieee754_floating F>
[[nodiscard]] [[gnu::flatten]] inline bool
pent_spd_solve(F *__restrict__ d2, F *__restrict__ d1, F *__restrict__ diag, F *__restrict__ rhs, usize n) noexcept
{
  if ( !pent_spd_factor<F>(d2, d1, diag, n) ) return false;
  pent_spd_solve_factored<F>(d2, d1, diag, rhs, n);
  return true;
}

// selective inverse of A given its LDL^T factor
template<ieee754_floating F>
[[gnu::flatten]] inline void
pent_spd_takahashi(const F *__restrict__ l2, const F *__restrict__ l1, const F *__restrict__ D, usize n, F *__restrict__ z_diag,
                   F *__restrict__ z_d1, F *__restrict__ z_d2) noexcept
{
  if ( n == 0 ) return;
  z_diag[n - 1] = F(1) / D[n - 1];
  if ( n == 1 ) return;

  // i = n-2
  z_d1[n - 2] = -l1[n - 2] * z_diag[n - 1];
  z_diag[n - 2] = F(1) / D[n - 2] - l1[n - 2] * z_d1[n - 2];

  // i = n-3 down to 0
  for ( usize ii = n - 2; ii-- > 0; ) {
    const usize i = ii;
    // Z[i, i+2] = -l1[i] * Z[i+1, i+2] - l2[i] * Z[i+2, i+2]
    z_d2[i] = -l1[i] * z_d1[i + 1] - l2[i] * z_diag[i + 2];
    // Z[i, i+1] = -l1[i] * Z[i+1, i+1] - l2[i] * Z[i+1, i+2]
    z_d1[i] = -l1[i] * z_diag[i + 1] - l2[i] * z_d1[i + 1];
    // Z[i, i] = 1/D[i] - l1[i] * Z[i, i+1] - l2[i] * Z[i, i+2]
    z_diag[i] = F(1) / D[i] - l1[i] * z_d1[i] - l2[i] * z_d2[i];
  }
}

template<ieee754_floating F>
[[nodiscard]] inline bool
gband_lu_solve(F *__restrict__ AB, usize kl, usize ku, usize n, F *__restrict__ rhs) noexcept
{
  const usize nb = kl + ku + 1;      // number of bands
  auto at = [&](usize band, usize col) -> F & { return AB[band * n + col]; };
  auto at_const = [&](usize band, usize col) -> const F & { return AB[band * n + col]; };

  for ( usize k = 0; k + 1 < n; ++k ) {
    const F piv = at(ku, k);
    if ( piv == F(0) ) return false;
    const F inv_piv = F(1) / piv;
    const usize r_end = (k + kl < n - 1) ? (k + kl) : (n - 1);

    for ( usize r = k + 1; r <= r_end; ++r ) {
      const usize band_rk = (r + ku) - k;
      const F m = at(band_rk, k) * inv_piv;
      at(band_rk, k) = m;
      const usize c_end = (k + ku < n - 1) ? (k + ku) : (n - 1);
      for ( usize c = k + 1; c <= c_end; ++c ) {
        if ( c > r + ku ) continue;
        const usize band_rc = (r + ku) - c;
        const usize band_kc = (k + ku) - c;
        if ( band_rc >= nb ) continue;
        if ( c > k + ku ) continue;
        at(band_rc, c) -= m * at_const(band_kc, c);
      }
    }
  }
  if ( at(ku, n - 1) == F(0) ) return false;

  for ( usize r = 0; r < n; ++r ) {
    F s = rhs[r];
    const usize j_lo = (r > kl) ? (r - kl) : 0;
    for ( usize j = j_lo; j < r; ++j ) {
      const usize band = (r + ku) - j;
      s -= at_const(band, j) * rhs[j];
    }
    rhs[r] = s;
  }

  for ( usize ri = n; ri-- > 0; ) {
    const usize r = ri;
    F s = rhs[r];
    const usize j_hi = (r + ku < n - 1) ? (r + ku) : (n - 1);
    for ( usize j = r + 1; j <= j_hi; ++j ) {
      if ( j > r + ku ) continue;
      const usize band = (r + ku) - j;
      s -= at_const(band, j) * rhs[j];
    }
    rhs[r] = s / at_const(ku, r);
  }
  return true;
}

template<ieee754_floating F> struct bandwidth_t {
  usize kl;
  usize ku;
};

template<ieee754_floating F>
[[nodiscard]] inline bandwidth_t<F>
bandwidth(const dynmat<F> &A, F tol = F(0)) noexcept
{
  bandwidth_t<F> b{ 0, 0 };
  const usize N = (A.rows < A.cols) ? A.rows : A.cols;
  for ( usize i = 0; i < A.rows; ++i ) {
    for ( usize j = 0; j < A.cols; ++j ) {
      F v = A.at(i, j);
      F a = (v < F(0)) ? -v : v;
      if ( a <= tol ) continue;
      if ( i > j ) {
        const usize d = i - j;
        if ( d > b.kl ) b.kl = d;
      } else if ( j > i ) {
        const usize d = j - i;
        if ( d > b.ku ) b.ku = d;
      }
    }
  }
  (void)N;
  return b;
}

template<ieee754_floating F, usize R, usize C>
[[nodiscard]] inline bandwidth_t<F>
bandwidth(const mat<F, R, C> &A, F tol = F(0)) noexcept
{
  bandwidth_t<F> b{ 0, 0 };
  for ( usize i = 0; i < R; ++i ) {
    for ( usize j = 0; j < C; ++j ) {
      F v = A.data[i * C + j];
      F a = (v < F(0)) ? -v : v;
      if ( a <= tol ) continue;
      if ( i > j ) {
        const usize d = i - j;
        if ( d > b.kl ) b.kl = d;
      } else if ( j > i ) {
        const usize d = j - i;
        if ( d > b.ku ) b.ku = d;
      }
    }
  }
  return b;
}

template<ieee754_floating F> struct chol_banded_result_dyn {
  dynmat<F> L;
  usize kl;
  bool spd;
};

template<ieee754_floating F>
[[nodiscard]] inline chol_banded_result_dyn<F>
chol_banded(const dynmat<F> &A, usize kl) noexcept
{
  const usize N = A.rows;
  chol_banded_result_dyn<F> r{ dynmat<F>::zero(N, N), kl, true };
  F *__restrict__ L = r.L.data();
  const usize ld = r.L.ld;
  for ( usize i = 0; i < N; ++i ) {
    const usize j_lo = (i > kl) ? (i - kl) : 0;
    for ( usize j = j_lo; j <= i; ++j ) {
      F s = A.at(i, j);
      const usize k_lo = (j > kl) ? (j - kl) : 0;      // L(i, k) and L(j, k) both nonzero only for k >= max(i-kl, j-kl)
      const usize k_lo_eff = (k_lo > j_lo) ? k_lo : j_lo;
      for ( usize k = k_lo_eff; k < j; ++k ) s = math::fma<F>(-L[i * ld + k], L[j * ld + k], s);
      if ( i == j ) {
        if ( s <= F(0) ) {
          r.spd = false;
          return r;
        }
        L[i * ld + j] = math::fsqrt(s);
      } else {
        L[i * ld + j] = s / L[j * ld + j];
      }
    }
  }
  return r;
}

};      // namespace linalg
};      // namespace math
};      // namespace micron
