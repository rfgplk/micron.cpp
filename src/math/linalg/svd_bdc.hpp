//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bidiagonal divide-and-conquer SVD
//
// Golub-Reinsch (LAPACK xBDSQR) for dynamic matrices

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "decomp.hpp"
#include "decomp_ext.hpp"
#include "householder.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace decomp
{

template <ieee754_floating F> struct bidiag_result_dyn {
  dynmat<F> B;
  dynmat<F> U;
  dynmat<F> V;
};

// A = U x B x V^T where B is upper bidiagonal in the top-left K×K block
template <ieee754_floating F>
[[nodiscard]] inline bidiag_result_dyn<F>
bidiagonalize(const dynmat<F> &A) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  bidiag_result_dyn<F> r{ A, dynmat<F>::identity(R), dynmat<F>::identity(C) };

  micron::vector<F, micron::allocator_serial<>, false> x_row(R, F(0));
  micron::vector<F, micron::allocator_serial<>, false> v_row(R, F(0));
  micron::vector<F, micron::allocator_serial<>, false> x_col(C, F(0));
  micron::vector<F, micron::allocator_serial<>, false> v_col(C, F(0));

  for ( usize k = 0; k < K; ++k ) {
    // left Householder on column k below row k
    for ( usize i = 0; i < R; ++i ) x_row.data()[i] = F(0);
    for ( usize i = k; i < R; ++i ) x_row.data()[i] = r.B.at(i, k);
    F beta_l = __impl_decomp::householder_reflector_dyn<F>(x_row.data(), R, k, v_row.data());
    if ( beta_l != F(0) ) {
      __impl_decomp::apply_householder_left_dyn<F>(r.B.data(), R, C, r.B.ld, v_row.data(), beta_l, k);
      __impl_decomp::apply_householder_right_dyn<F>(r.U.data(), R, R, r.U.ld, v_row.data(), beta_l, k);
      for ( usize i = k + 1; i < R; ++i ) r.B.at(i, k) = F(0);
    }
    // right Householder on row k right of column k+1
    if ( k + 2 <= C ) {
      for ( usize j = 0; j < C; ++j ) x_col.data()[j] = F(0);
      for ( usize j = k + 1; j < C; ++j ) x_col.data()[j] = r.B.at(k, j);
      F beta_r = __impl_decomp::householder_reflector_dyn<F>(x_col.data(), C, k + 1, v_col.data());
      if ( beta_r != F(0) ) {
        __impl_decomp::apply_householder_right_dyn<F>(r.B.data(), R, C, r.B.ld, v_col.data(), beta_r, k + 1);
        __impl_decomp::apply_householder_right_dyn<F>(r.V.data(), C, C, r.V.ld, v_col.data(), beta_r, k + 1);
        for ( usize j = k + 2; j < C; ++j ) r.B.at(k, j) = F(0);
      }
    }
  }
  return r;
}

namespace __impl_svd_bdc
{

template <ieee754_floating F>
[[gnu::always_inline]] inline F
pythag(F a, F b) noexcept
{
  F absa = math::fabs(a), absb = math::fabs(b);
  if ( absa > absb ) {
    F r = absb / absa;
    return absa * math::fsqrt(F(1) + r * r);
  }
  if ( absb == F(0) ) return F(0);
  F r = absa / absb;
  return absb * math::fsqrt(F(1) + r * r);
}

template <ieee754_floating F>
[[gnu::always_inline]] inline F
sgn_copy(F a, F b) noexcept
{
  return (b >= F(0)) ? math::fabs(a) : -math::fabs(a);
}

template <ieee754_floating F>
inline bool
bidiag_svd_diag(F *__restrict__ d, F *__restrict__ e, usize n, F *__restrict__ U_data, usize U_rows, usize U_ld, F *__restrict__ V_data,
                usize V_rows, usize V_ld, int max_iter) noexcept
{
  if ( n == 0 ) return true;
  if ( n == 1 ) {
    if ( d[0] < F(0) ) {
      d[0] = -d[0];
      for ( usize j = 0; j < V_rows; ++j ) V_data[j * V_ld + 0] = -V_data[j * V_ld + 0];
    }
    return true;
  }

  // scale for negligibility tests
  F anorm = math::fabs(d[0]);
  for ( usize i = 0; i + 1 < n; ++i ) {
    F v = math::fabs(d[i + 1]) + math::fabs(e[i]);
    if ( v > anorm ) anorm = v;
  }

  // process from bottom up
  for ( usize k_top = n; k_top-- > 0; ) {
    const usize k = k_top;
    int iter = 0;
    while ( true ) {
      // find l in [0..k] such that the submatrix B[l..k] is irreducible
      bool flag = true;
      usize l = 0;
      bool found = false;
      for ( usize ll = k; ll-- > 0; ) {
        // ll in [0..k-1];  test e[ll]
        if ( math::fabs(e[ll]) + anorm == anorm ) {
          flag = false;
          e[ll] = F(0);
          l = ll + 1;
          found = true;
          break;
        }
        if ( math::fabs(d[ll]) + anorm == anorm ) {
          l = ll + 1;
          found = true;
          break;
        }
      }
      if ( !found ) {
        l = 0;
        flag = false;
      }

      if ( flag ) {
        // chase the zero in d[l-1] across e[l-1..k-1]; left Givens into U cols (l-1, i)
        F c = F(0), s = F(1);
        const usize nm = l - 1;
        for ( usize i = l; i <= k; ++i ) {
          F f = s * e[i - 1];
          e[i - 1] = c * e[i - 1];
          if ( math::fabs(f) + anorm == anorm ) break;
          F g = d[i];
          F h = pythag<F>(f, g);
          d[i] = h;
          h = F(1) / h;
          c = g * h;
          s = -f * h;
          for ( usize j = 0; j < U_rows; ++j ) {
            F y = U_data[j * U_ld + nm];
            F z = U_data[j * U_ld + i];
            U_data[j * U_ld + nm] = y * c + z * s;
            U_data[j * U_ld + i] = z * c - y * s;
          }
        }
      }

      F zk = d[k];
      if ( l == k ) {
        if ( zk < F(0) ) {
          d[k] = -zk;
          for ( usize j = 0; j < V_rows; ++j ) V_data[j * V_ld + k] = -V_data[j * V_ld + k];
        }
        break;
      }

      if ( ++iter > max_iter ) return false;

      // shift from bottom 2x2 of B^T B over rows l..k
      F x = d[l];
      const usize nm = k - 1;
      F y = d[nm];
      F g = (nm > 0) ? e[nm - 1] : F(0);
      F h = e[nm];
      F f = ((y - zk) * (y + zk) + (g - h) * (g + h)) / (F(2) * h * y);
      F gg = pythag<F>(f, F(1));
      f = ((x - zk) * (x + zk) + h * ((y / (f + sgn_copy<F>(gg, f))) - h)) / x;

      // QR sweep over j in [l..nm]
      F c = F(1), s = F(1);
      for ( usize j = l; j <= nm; ++j ) {
        const usize i = j + 1;
        F gv = e[j];
        F yv = d[i];
        F hv = s * gv;
        gv = c * gv;
        F zv = pythag<F>(f, hv);
        if ( j > 0 ) e[j - 1] = zv;
        if ( zv != F(0) ) {
          F inv_z = F(1) / zv;
          c = f * inv_z;
          s = hv * inv_z;
        }
        f = x * c + gv * s;
        gv = gv * c - x * s;
        hv = yv * s;
        yv = yv * c;
        // right Givens into V cols (j, i)
        for ( usize jj = 0; jj < V_rows; ++jj ) {
          F xv = V_data[jj * V_ld + j];
          F zw = V_data[jj * V_ld + i];
          V_data[jj * V_ld + j] = xv * c + zw * s;
          V_data[jj * V_ld + i] = zw * c - xv * s;
        }
        F zv2 = pythag<F>(f, hv);
        d[j] = zv2;
        if ( zv2 != F(0) ) {
          F inv_z = F(1) / zv2;
          c = f * inv_z;
          s = hv * inv_z;
        }
        f = c * gv + s * yv;
        x = c * yv - s * gv;
        // left Givens into U cols (j, i)
        for ( usize jj = 0; jj < U_rows; ++jj ) {
          F yu = U_data[jj * U_ld + j];
          F zu = U_data[jj * U_ld + i];
          U_data[jj * U_ld + j] = yu * c + zu * s;
          U_data[jj * U_ld + i] = zu * c - yu * s;
        }
      }
      // cleanup the chased entries
      if ( l > 0 ) e[l - 1] = F(0);
      e[k - 1] = f;
      d[k] = x;
    }
  }
  return true;
}

// Cuppen's algorithm to T = B^T B
template <ieee754_floating F>
inline F
solve_secular_root(const F *d, const F *z2, usize n, usize idx, F rho, F upper_pad, int max_iter) noexcept
{
  F lo = d[idx];
  F hi = (idx + 1 < n) ? d[idx + 1] : (d[n - 1] + upper_pad);
  F lam = (lo + hi) * F(0.5);

  for ( int iter = 0; iter < max_iter; ++iter ) {
    F f = F(1);
    F fp = F(0);
    bool degenerate = false;
    for ( usize i = 0; i < n; ++i ) {
      F denom = d[i] - lam;
      F scale = math::fabs(d[i]) + math::fabs(lam) + F(1);
      if ( math::fabs(denom) < default_eps<F>() * scale * F(8) ) {
        degenerate = true;
        break;
      }
      F t = z2[i] / denom;
      f += rho * t;
      fp += rho * t / denom;
    }

    if ( degenerate ) {
      // fall back to bisection step
      F mid = (lo + hi) * F(0.5);
      if ( mid == lam ) break;
      lam = mid;
      continue;
    }

    if ( math::fabs(f) <= default_eps<F>() * F(100) * (math::fabs(lam) + F(1)) ) break;

    if ( f > F(0) )
      hi = lam;
    else
      lo = lam;

    F delta = (fp != F(0)) ? -f / fp : F(0);
    F new_lam = lam + delta;
    if ( delta == F(0) || new_lam <= lo || new_lam >= hi ) {
      new_lam = (lo + hi) * F(0.5);
    }
    if ( new_lam == lam ) break;
    lam = new_lam;
  }
  return lam;
}

// Cuppen's recursive symmetric tridiagonal D&C
template <ieee754_floating F>
inline bool
cuppen_tridiag_dnc(F *d, F *e, usize n, F *Q_data, usize Q_ld, usize threshold, int max_iter) noexcept
{
  if ( n == 0 ) return true;
  if ( n == 1 ) return true;

  if ( n <= threshold ) {
    return decomp::__impl_decomp_qrtri::tqli<F>(d, e, Q_data, n, Q_ld, max_iter);
  }

  const usize k = n / 2;
  const F beta = e[k - 1];

  const F rho = math::fabs(beta);
  const F sign_beta = (beta >= F(0)) ? F(1) : F(-1);

  d[k - 1] -= beta;
  d[k] -= beta;

  // recurse on sub-blocks; each sub-block of Q is currently identity
  if ( !cuppen_tridiag_dnc<F>(d, e, k, Q_data, Q_ld, threshold, max_iter) ) return false;
  if ( !cuppen_tridiag_dnc<F>(d + k, e + k, n - k, Q_data + k * Q_ld + k, Q_ld, threshold, max_iter) ) return false;

  // u_i = (Q^T v)_i where v = e_{k-1} + sign_beta * e_k
  micron::vector<F, micron::allocator_serial<>, false> u(n, F(0));
  for ( usize i = 0; i < k; ++i ) u.data()[i] = Q_data[(k - 1) * Q_ld + i];
  for ( usize i = k; i < n; ++i ) u.data()[i] = sign_beta * Q_data[k * Q_ld + i];

  // sort d ascending; carry u alongside
  micron::vector<usize, micron::allocator_serial<>, false> perm(n);
  for ( usize i = 0; i < n; ++i ) perm.data()[i] = i;
  for ( usize i = 0; i + 1 < n; ++i ) {
    usize best = i;
    for ( usize j = i + 1; j < n; ++j ) {
      if ( d[perm.data()[j]] < d[perm.data()[best]] ) best = j;
    }
    if ( best != i ) {
      usize t = perm.data()[i];
      perm.data()[i] = perm.data()[best];
      perm.data()[best] = t;
    }
  }
  micron::vector<F, micron::allocator_serial<>, false> d_sorted(n, F(0));
  micron::vector<F, micron::allocator_serial<>, false> u_sorted(n, F(0));
  for ( usize i = 0; i < n; ++i ) {
    d_sorted.data()[i] = d[perm.data()[i]];
    u_sorted.data()[i] = u.data()[perm.data()[i]];
  }

  // ρ for secular root finder
  micron::vector<F, micron::allocator_serial<>, false> z2(n);
  F sum_z2 = F(0);
  for ( usize i = 0; i < n; ++i ) {
    z2.data()[i] = u_sorted.data()[i] * u_sorted.data()[i];
    sum_z2 += z2.data()[i];
  }
  F upper_pad = rho * sum_z2 + (math::fabs(d_sorted.data()[n - 1]) + F(1)) * F(0.01);
  if ( upper_pad <= F(0) ) upper_pad = F(1);

  // find new eigenvalues
  micron::vector<F, micron::allocator_serial<>, false> lam(n, F(0));
  for ( usize i = 0; i < n; ++i ) {
    lam.data()[i] = solve_secular_root<F>(d_sorted.data(), z2.data(), n, i, rho, upper_pad, max_iter);
  }

  // normalize each column
  micron::vector<F, micron::allocator_serial<>, false> V_merge(n * n, F(0));
  for ( usize i = 0; i < n; ++i ) {
    F norm_sq = F(0);
    for ( usize j = 0; j < n; ++j ) {
      F denom = d_sorted.data()[j] - lam.data()[i];
      if ( math::fabs(denom) < default_eps<F>() * F(8) ) denom = (denom >= F(0)) ? default_eps<F>() : -default_eps<F>();
      F vj = u_sorted.data()[j] / denom;
      V_merge.data()[j * n + i] = vj;
      norm_sq = math::fma<F>(vj, vj, norm_sq);
    }
    F norm = math::fsqrt(norm_sq);
    if ( norm > F(0) ) {
      F inv = F(1) / norm;
      for ( usize j = 0; j < n; ++j ) V_merge.data()[j * n + i] *= inv;
    }
  }

  micron::vector<F, micron::allocator_serial<>, false> V_unperm(n * n, F(0));
  for ( usize i = 0; i < n; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      // row in original ordering = perm.data()[j]; column = i
      V_unperm.data()[perm.data()[j] * n + i] = V_merge.data()[j * n + i];
    }
  }
  for ( usize i = 0; i < n; ++i ) d[i] = lam.data()[i];

  // Q := Q * V_unperm   (Q has columns = old eigenvectors; we transform to new basis)
  micron::vector<F, micron::allocator_serial<>, false> tmp(n * n, F(0));
  for ( usize i = 0; i < n; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      F s = F(0);
      for ( usize kk = 0; kk < n; ++kk ) s = math::fma<F>(Q_data[i * Q_ld + kk], V_unperm.data()[kk * n + j], s);
      tmp.data()[i * n + j] = s;
    }
  }
  for ( usize i = 0; i < n; ++i )
    for ( usize j = 0; j < n; ++j ) Q_data[i * Q_ld + j] = tmp.data()[i * n + j];

  return true;
}

};     // namespace __impl_svd_bdc

template <ieee754_floating F> struct svd_bdc_result_dyn {
  dynmat<F> U;
  dynvec<F> S;
  dynmat<F> V;
  bool converged;
};

template <ieee754_floating F>
[[nodiscard]] inline svd_bdc_result_dyn<F>
svd_bdc(const dynmat<F> &A, int max_iter = 60) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  svd_bdc_result_dyn<F> r{};

  if ( K == 0 ) {
    r.U = dynmat<F>::identity(R);
    r.S = dynvec<F>(0);
    r.V = dynmat<F>::identity(C);
    r.converged = true;
    return r;
  }

  auto bd = bidiagonalize<F>(A);

  micron::vector<F, micron::allocator_serial<>, false> d(K, F(0));
  micron::vector<F, micron::allocator_serial<>, false> e(K > 0 ? K - 1 : 0, F(0));
  for ( usize i = 0; i < K; ++i ) d.data()[i] = bd.B.at(i, i);
  for ( usize i = 0; i + 1 < K; ++i ) e.data()[i] = bd.B.at(i, i + 1);

  r.converged = __impl_svd_bdc::bidiag_svd_diag<F>(d.data(), e.data(), K, bd.U.data(), R, bd.U.ld, bd.V.data(), C, bd.V.ld, max_iter);

  // sort singular values descending; permute cols of U (first K) and V (first K)
  micron::vector<usize, micron::allocator_serial<>, false> idx(K);
  for ( usize i = 0; i < K; ++i ) idx.data()[i] = i;
  // simple selection sort by d (small K typically)
  for ( usize i = 0; i + 1 < K; ++i ) {
    usize best = i;
    F best_v = d.data()[idx.data()[i]];
    for ( usize j = i + 1; j < K; ++j ) {
      F v = d.data()[idx.data()[j]];
      if ( v > best_v ) {
        best = j;
        best_v = v;
      }
    }
    if ( best != i ) {
      usize t = idx.data()[i];
      idx.data()[i] = idx.data()[best];
      idx.data()[best] = t;
    }
  }

  r.S = dynvec<F>(K);
  for ( usize i = 0; i < K; ++i ) r.S[i] = d.data()[idx.data()[i]];

  // permute columns of U[:, 0:K] and V[:, 0:K] using idx
  r.U = dynmat<F>(R, R);
  for ( usize i = 0; i < R; ++i ) {
    for ( usize j = 0; j < K; ++j ) r.U.at(i, j) = bd.U.at(i, idx.data()[j]);
    for ( usize j = K; j < R; ++j ) r.U.at(i, j) = bd.U.at(i, j);
  }
  r.V = dynmat<F>(C, C);
  for ( usize i = 0; i < C; ++i ) {
    for ( usize j = 0; j < K; ++j ) r.V.at(i, j) = bd.V.at(i, idx.data()[j]);
    for ( usize j = K; j < C; ++j ) r.V.at(i, j) = bd.V.at(i, j);
  }

  return r;
}

// SVD via Cuppen
template <ieee754_floating F>
[[nodiscard]] inline svd_bdc_result_dyn<F>
svd_bdc_dnc(const dynmat<F> &A, usize threshold = 16, int max_iter = 60) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  svd_bdc_result_dyn<F> r{};

  if ( R == 0 || C == 0 ) {
    r.U = dynmat<F>::identity(R);
    r.S = dynvec<F>(0);
    r.V = dynmat<F>::identity(C);
    r.converged = true;
    return r;
  }

  // For R < C, SVD(A) = (V, S, U) of SVD(A^T)
  if ( R < C ) {
    dynmat<F> At(C, R);
    for ( usize i = 0; i < R; ++i )
      for ( usize j = 0; j < C; ++j ) At.at(j, i) = A.at(i, j);
    auto rt = svd_bdc_dnc<F>(At, threshold, max_iter);
    r.U = micron::move(rt.V);
    r.S = micron::move(rt.S);
    r.V = micron::move(rt.U);
    r.converged = rt.converged;
    return r;
  }

  // R >= C path
  const usize K = C;
  auto bd = bidiagonalize<F>(A);

  // build tridiagonal T = B^T B (C x C symmetric, in d_t and e_t)
  micron::vector<F, micron::allocator_serial<>, false> d_t(K, F(0));
  micron::vector<F, micron::allocator_serial<>, false> e_t(K > 0 ? K - 1 : 0, F(0));
  micron::vector<F, micron::allocator_serial<>, false> d_b(K, F(0));     // diag of B
  micron::vector<F, micron::allocator_serial<>, false> e_b(K > 0 ? K - 1 : 0, F(0));
  for ( usize i = 0; i < K; ++i ) d_b.data()[i] = bd.B.at(i, i);
  for ( usize i = 0; i + 1 < K; ++i ) e_b.data()[i] = bd.B.at(i, i + 1);

  d_t.data()[0] = d_b.data()[0] * d_b.data()[0];
  for ( usize i = 1; i < K; ++i ) d_t.data()[i] = d_b.data()[i] * d_b.data()[i] + e_b.data()[i - 1] * e_b.data()[i - 1];
  for ( usize i = 0; i + 1 < K; ++i ) e_t.data()[i] = d_b.data()[i] * e_b.data()[i];

  // Cuppen on T
  dynmat<F> Q = dynmat<F>::identity(K);
  bool conv = __impl_svd_bdc::cuppen_tridiag_dnc<F>(d_t.data(), e_t.data(), K, Q.data(), Q.ld, threshold, max_iter);
  r.converged = conv;

  // d_t now holds eigenvalues; sigma = sqrt(eigenvalue); clamp non-negative
  micron::vector<F, micron::allocator_serial<>, false> sigma(K, F(0));
  for ( usize i = 0; i < K; ++i ) {
    F lam = d_t.data()[i];
    if ( lam < F(0) ) lam = F(0);
    sigma.data()[i] = math::fsqrt(lam);
  }

  // right singular vectors of B (in bidiag space) = columns of Q
  // left singular vectors of B = B * Q * diag(1/sigma)
  dynmat<F> U_inner = dynmat<F>::zero(R, K);
  // For thin form: U_inner[:, j] = (B * Q[:, j]) / sigma[j]
  for ( usize j = 0; j < K; ++j ) {
    // compute B * Q[:, j] using bidiagonal structure
    // (B * q)_i = d_b[i] * q_i + e_b[i] * q_{i+1}  for i in [0..K-2]
    // (B * q)_{K-1} = d_b[K-1] * q_{K-1}
    // (B * q)_i = 0   for i >= K  (since B's bidiagonal is in top K rows; row indices >= K are zero in B)
    for ( usize i = 0; i + 1 < K; ++i ) {
      F bq = d_b.data()[i] * Q.at(i, j) + e_b.data()[i] * Q.at(i + 1, j);
      U_inner.at(i, j) = bq;
    }
    if ( K > 0 ) U_inner.at(K - 1, j) = d_b.data()[K - 1] * Q.at(K - 1, j);
    for ( usize i = K; i < R; ++i ) U_inner.at(i, j) = F(0);
    // normalize by sigma if nonzero; else the vector itself is in the null space
    if ( sigma.data()[j] > F(0) ) {
      F inv = F(1) / sigma.data()[j];
      for ( usize i = 0; i < R; ++i ) U_inner.at(i, j) *= inv;
    } else {
      // sigma == 0: leave U_inner[:, j] as-is (will be re-orthogonalized below or zeroed)
      for ( usize i = 0; i < R; ++i ) U_inner.at(i, j) = F(0);
    }
  }

  // sort sigma descending; permute U_inner cols and Q cols accordingly
  micron::vector<usize, micron::allocator_serial<>, false> idx(K);
  for ( usize i = 0; i < K; ++i ) idx.data()[i] = i;
  for ( usize i = 0; i + 1 < K; ++i ) {
    usize best = i;
    for ( usize j = i + 1; j < K; ++j ) {
      if ( sigma.data()[idx.data()[j]] > sigma.data()[idx.data()[best]] ) best = j;
    }
    if ( best != i ) {
      usize t = idx.data()[i];
      idx.data()[i] = idx.data()[best];
      idx.data()[best] = t;
    }
  }

  r.S = dynvec<F>(K);
  for ( usize i = 0; i < K; ++i ) r.S[i] = sigma.data()[idx.data()[i]];

  r.U = dynmat<F>(R, R);
  for ( usize i = 0; i < R; ++i ) {
    for ( usize j = 0; j < K; ++j ) {
      F s = F(0);
      for ( usize kk = 0; kk < R; ++kk ) s = math::fma<F>(bd.U.at(i, kk), U_inner.at(kk, idx.data()[j]), s);
      r.U.at(i, j) = s;
    }
    for ( usize j = K; j < R; ++j ) r.U.at(i, j) = bd.U.at(i, j);
  }

  r.V = dynmat<F>(C, C);
  for ( usize i = 0; i < C; ++i ) {
    for ( usize j = 0; j < K; ++j ) {
      F s = F(0);
      for ( usize kk = 0; kk < C; ++kk ) s = math::fma<F>(bd.V.at(i, kk), Q.at(kk, idx.data()[j]), s);
      r.V.at(i, j) = s;
    }
    for ( usize j = K; j < C; ++j ) r.V.at(i, j) = bd.V.at(i, j);
  }

  return r;
}

};     // namespace decomp
};     // namespace linalg
};     // namespace math
};     // namespace micron
