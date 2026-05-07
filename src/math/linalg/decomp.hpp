//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// numeric decomposition
//  -> lu (Doolittle)
//  -> qr (modified Gram-Schmidt)
//  -> chol (lower)
//  -> svd2, svd3 (analytical; Jacobi)
//  -> eigen_sym2
//  -> eigen_sym3

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "householder.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace decomp
{

template <ieee754_floating F, usize N> struct lu_result {
  mat<F, N, N> L;
  mat<F, N, N> U;
  bool singular;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline lu_result<F, N>
lu(const mat<F, N, N> &A) noexcept
{
  lu_result<F, N> r{ mat<F, N, N>::zero(), mat<F, N, N>::zero(), false };
  // L diagonal = 1
  for ( usize i = 0; i < N; ++i ) r.L.data[i * N + i] = F(1);

  for ( usize i = 0; i < N; ++i ) {
    // U row i
    for ( usize j = i; j < N; ++j ) {
      F s = A.data[i * N + j];
      for ( usize k = 0; k < i; ++k ) s -= r.L.data[i * N + k] * r.U.data[k * N + j];
      r.U.data[i * N + j] = s;
    }
    F piv = r.U.data[i * N + i];
    if ( piv == F(0) ) {
      r.singular = true;
      return r;
    }
    F inv = F(1) / piv;
    // L column i
    for ( usize j = i + 1; j < N; ++j ) {
      F s = A.data[j * N + i];
      for ( usize k = 0; k < i; ++k ) s -= r.L.data[j * N + k] * r.U.data[k * N + i];
      r.L.data[j * N + i] = s * inv;
    }
  }
  return r;
}

// Cholesky decomposition
template <ieee754_floating F, usize N> struct chol_result {
  mat<F, N, N> L;
  bool spd;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline chol_result<F, N>
chol(const mat<F, N, N> &A) noexcept
{
  chol_result<F, N> r{ mat<F, N, N>::zero(), true };
  for ( usize i = 0; i < N; ++i ) {
    for ( usize j = 0; j <= i; ++j ) {
      F s = A.data[i * N + j];
      for ( usize k = 0; k < j; ++k ) s -= r.L.data[i * N + k] * r.L.data[j * N + k];
      if ( i == j ) {
        if ( s <= F(0) ) {
          r.spd = false;
          return r;
        }
        r.L.data[i * N + j] = math::fsqrt(s);
      } else {
        r.L.data[i * N + j] = s / r.L.data[j * N + j];
      }
    }
  }
  return r;
}

// QR decomposition (modified Gram-Schmidt)
template <ieee754_floating F, usize R_, usize C_> struct qr_result {
  mat<F, R_, C_> Q;
  mat<F, C_, C_> R;
};

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard]] inline qr_result<F, R_, C_>
qr(const mat<F, R_, C_> &A) noexcept
{
  qr_result<F, R_, C_> r{ mat<F, R_, C_>::zero(), mat<F, C_, C_>::zero() };
  vec<F, R_> q[C_];
  for ( usize j = 0; j < C_; ++j )
    for ( usize i = 0; i < R_; ++i ) q[j].data[i] = A.data[i * C_ + j];

  for ( usize j = 0; j < C_; ++j ) {
    F nj = math::fsqrt(ops::dot(q[j], q[j]));
    r.R.data[j * C_ + j] = nj;
    if ( nj > F(0) ) {
      F inv = F(1) / nj;
      for ( usize i = 0; i < R_; ++i ) q[j].data[i] *= inv;
    }
    for ( usize k = j + 1; k < C_; ++k ) {
      F p = ops::dot(q[j], q[k]);
      r.R.data[j * C_ + k] = p;
      for ( usize i = 0; i < R_; ++i ) q[k].data[i] -= p * q[j].data[i];
    }
  }
  // pack q[] into Q
  for ( usize j = 0; j < C_; ++j )
    for ( usize i = 0; i < R_; ++i ) r.Q.data[i * C_ + j] = q[j].data[i];
  return r;
}

template <ieee754_floating F> struct eigen_result2 {
  vec<F, 2> values;
  mat<F, 2, 2> vectors;
};

template <ieee754_floating F> struct eigen_result3 {
  vec<F, 3> values;
  mat<F, 3, 3> vectors;
};

template <ieee754_floating F>
[[nodiscard]] inline eigen_result2<F>
eigen_sym2(const mat<F, 2, 2> &A) noexcept
{
  F a = A.data[0], b = A.data[1], c = A.data[3];
  F tr = a + c;
  F det = a * c - b * b;
  F disc = math::fsqrt(tr * tr * F(0.25) - det);
  F l1 = tr * F(0.5) + disc;
  F l2 = tr * F(0.5) - disc;

  eigen_result2<F> r{};
  r.values.data[0] = l1;
  r.values.data[1] = l2;

  // eigenvectors
  if ( b != F(0) ) {
    F v1y = b;
    F v1x = l1 - c;
    F n1 = math::fsqrt(v1x * v1x + v1y * v1y);
    r.vectors.data[0] = v1x / n1;
    r.vectors.data[2] = v1y / n1;
    F v2y = b;
    F v2x = l2 - c;
    F n2 = math::fsqrt(v2x * v2x + v2y * v2y);
    r.vectors.data[1] = v2x / n2;
    r.vectors.data[3] = v2y / n2;
  } else {
    // already diagonal
    r.vectors.data[0] = F(1);
    r.vectors.data[1] = F(0);
    r.vectors.data[2] = F(0);
    r.vectors.data[3] = F(1);
  }
  return r;
}

template <ieee754_floating F>
[[nodiscard]] inline eigen_result3<F>
eigen_sym3(const mat<F, 3, 3> &A) noexcept
{
  mat<F, 3, 3> M = A;
  mat<F, 3, 3> V = mat<F, 3, 3>::identity();
  constexpr int max_sweeps = 50;
  constexpr F eps = F(1e-12);

  for ( int sweep = 0; sweep < max_sweeps; ++sweep ) {
    F off = math::fabs(M.data[1]) + math::fabs(M.data[2]) + math::fabs(M.data[5]);
    if ( off < eps ) break;

    static constexpr int pq[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
    for ( int k = 0; k < 3; ++k ) {
      int p = pq[k][0], q = pq[k][1];
      F app = M.data[p * 3 + p];
      F aqq = M.data[q * 3 + q];
      F apq = M.data[p * 3 + q];
      if ( math::fabs(apq) < eps ) continue;
      F theta = (aqq - app) / (F(2) * apq);
      F t;
      if ( theta >= F(0) )
        t = F(1) / (theta + math::fsqrt(F(1) + theta * theta));
      else
        t = F(1) / (theta - math::fsqrt(F(1) + theta * theta));
      F c = F(1) / math::fsqrt(F(1) + t * t);
      F s = t * c;

      F new_app = app - t * apq;
      F new_aqq = aqq + t * apq;
      M.data[p * 3 + p] = new_app;
      M.data[q * 3 + q] = new_aqq;
      M.data[p * 3 + q] = F(0);
      M.data[q * 3 + p] = F(0);

      // update remaining off-diagonals
      for ( int r = 0; r < 3; ++r ) {
        if ( r == p || r == q ) continue;
        F arp = M.data[r * 3 + p];
        F arq = M.data[r * 3 + q];
        F new_arp = c * arp - s * arq;
        F new_arq = s * arp + c * arq;
        M.data[r * 3 + p] = new_arp;
        M.data[p * 3 + r] = new_arp;
        M.data[r * 3 + q] = new_arq;
        M.data[q * 3 + r] = new_arq;
      }
      // update eigenvector matrix V
      for ( int r = 0; r < 3; ++r ) {
        F vrp = V.data[r * 3 + p];
        F vrq = V.data[r * 3 + q];
        V.data[r * 3 + p] = c * vrp - s * vrq;
        V.data[r * 3 + q] = s * vrp + c * vrq;
      }
    }
  }

  eigen_result3<F> res{};
  res.values.data[0] = M.data[0];
  res.values.data[1] = M.data[4];
  res.values.data[2] = M.data[8];
  res.vectors = V;
  return res;
}

template <ieee754_floating F> struct svd2_result {
  mat<F, 2, 2> U;
  vec<F, 2> S;
  mat<F, 2, 2> V;
};

template <ieee754_floating F>
[[nodiscard]] inline svd2_result<F>
svd2(const mat<F, 2, 2> &A) noexcept
{
  mat<F, 2, 2> AtA{};
  AtA.data[0] = A.data[0] * A.data[0] + A.data[2] * A.data[2];
  AtA.data[1] = A.data[0] * A.data[1] + A.data[2] * A.data[3];
  AtA.data[2] = AtA.data[1];
  AtA.data[3] = A.data[1] * A.data[1] + A.data[3] * A.data[3];
  auto e = eigen_sym2(AtA);

  svd2_result<F> r{};
  r.V = e.vectors;
  r.S.data[0] = math::fsqrt(e.values.data[0] < F(0) ? F(0) : e.values.data[0]);
  r.S.data[1] = math::fsqrt(e.values.data[1] < F(0) ? F(0) : e.values.data[1]);

  vec<F, 2> v0{ r.V.data[0], r.V.data[2] };     // col 0 of V
  vec<F, 2> v1{ r.V.data[1], r.V.data[3] };     // col 1 of V
  vec<F, 2> u0 = ops::gemv(A, v0);
  vec<F, 2> u1 = ops::gemv(A, v1);
  if ( r.S.data[0] > F(0) ) {
    F inv = F(1) / r.S.data[0];
    u0.data[0] *= inv;
    u0.data[1] *= inv;
  }
  if ( r.S.data[1] > F(0) ) {
    F inv = F(1) / r.S.data[1];
    u1.data[0] *= inv;
    u1.data[1] *= inv;
  }
  r.U.data[0] = u0.data[0];
  r.U.data[2] = u0.data[1];
  r.U.data[1] = u1.data[0];
  r.U.data[3] = u1.data[1];
  return r;
}

template <ieee754_floating F> struct svd3_result {
  mat<F, 3, 3> U;
  vec<F, 3> S;
  mat<F, 3, 3> V;
};

template <ieee754_floating F>
[[nodiscard]] inline svd3_result<F>
svd3(const mat<F, 3, 3> &A) noexcept
{
  mat<F, 3, 3> AtA{};
  // AtA[i,j] = sum_k A[k,i] * A[k,j]
  for ( usize i = 0; i < 3; ++i )
    for ( usize j = 0; j < 3; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < 3; ++k ) s += A.data[k * 3 + i] * A.data[k * 3 + j];
      AtA.data[i * 3 + j] = s;
    }
  auto e = eigen_sym3(AtA);

  svd3_result<F> r{};
  r.V = e.vectors;
  for ( usize i = 0; i < 3; ++i ) {
    F v = e.values.data[i];
    r.S.data[i] = math::fsqrt(v < F(0) ? F(0) : v);
  }

  // U columns = A * V columns scaled by 1/S
  for ( usize j = 0; j < 3; ++j ) {
    vec<F, 3> vj{ r.V.data[j], r.V.data[3 + j], r.V.data[6 + j] };
    vec<F, 3> uj = ops::gemv(A, vj);
    if ( r.S.data[j] > F(0) ) {
      F inv = F(1) / r.S.data[j];
      uj.data[0] *= inv;
      uj.data[1] *= inv;
      uj.data[2] *= inv;
    }
    r.U.data[j] = uj.data[0];
    r.U.data[3 + j] = uj.data[1];
    r.U.data[6 + j] = uj.data[2];
  }
  return r;
}

template <ieee754_floating F, usize N> struct lu_pivot_result {
  mat<F, N, N> LU;
  usize perm[N];
  int sign;
  bool singular;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline lu_pivot_result<F, N>
lu_pivot(const mat<F, N, N> &A) noexcept
{
  lu_pivot_result<F, N> r{ A, {}, 1, false };
  for ( usize i = 0; i < N; ++i ) r.perm[i] = i;

  F scale[N];
  for ( usize i = 0; i < N; ++i ) {
    F m = F(0);
    for ( usize j = 0; j < N; ++j ) {
      const F v = math::fabs(r.LU.data[i * N + j]);
      if ( v > m ) m = v;
    }
    scale[i] = m;
  }

  for ( usize k = 0; k < N; ++k ) {
    F best_a = F(0);
    F best_s = F(1);
    usize piv = k;
    for ( usize i = k; i < N; ++i ) {
      const F a = math::fabs(r.LU.data[i * N + k]);
      const F si = scale[i];
      if ( si == F(0) ) continue;     // zero row: skip
      if ( a * best_s > best_a * si ) {
        best_a = a;
        best_s = si;
        piv = i;
      }
    }
    if ( best_a == F(0) ) {
      r.singular = true;
      return r;
    }

    if ( piv != k ) {
      for ( usize j = 0; j < N; ++j ) {
        F t = r.LU.data[k * N + j];
        r.LU.data[k * N + j] = r.LU.data[piv * N + j];
        r.LU.data[piv * N + j] = t;
      }
      usize t = r.perm[k];
      r.perm[k] = r.perm[piv];
      r.perm[piv] = t;
      F ts = scale[k];
      scale[k] = scale[piv];
      scale[piv] = ts;
      r.sign = -r.sign;
    }

    const F inv_piv = F(1) / r.LU.data[k * N + k];
    for ( usize i = k + 1; i < N; ++i ) r.LU.data[i * N + k] *= inv_piv;
    for ( usize i = k + 1; i < N; ++i ) {
      const F m = r.LU.data[i * N + k];
      for ( usize j = k + 1; j < N; ++j ) {
        r.LU.data[i * N + j] = math::fma<F>(-m, r.LU.data[k * N + j], r.LU.data[i * N + j]);
      }
    }
  }
  return r;
}

// dynamic-shape twin
template <ieee754_floating F> struct lu_pivot_result_dyn {
  dynmat<F> LU;
  micron::vector<usize, micron::allocator_serial<>, false> perm;
  int sign;
  bool singular;
};

template <ieee754_floating F>
[[nodiscard]] inline lu_pivot_result_dyn<F>
lu_pivot(const dynmat<F> &A) noexcept
{
  lu_pivot_result_dyn<F> r{ A, micron::vector<usize, micron::allocator_serial<>, false>(A.rows), 1, false };
  const usize N = A.rows;
  for ( usize i = 0; i < N; ++i ) r.perm.data()[i] = i;
  F *__restrict__ LU = r.LU.data();
  const usize ld = r.LU.ld;

  micron::vector<F, micron::allocator_serial<>, false> scale(N);
  for ( usize i = 0; i < N; ++i ) {
    F m = F(0);
    for ( usize j = 0; j < N; ++j ) {
      const F v = math::fabs(LU[i * ld + j]);
      if ( v > m ) m = v;
    }
    scale.data()[i] = m;
  }

  for ( usize k = 0; k < N; ++k ) {
    F best_a = F(0);
    F best_s = F(1);
    usize piv = k;
    for ( usize i = k; i < N; ++i ) {
      const F a = math::fabs(LU[i * ld + k]);
      const F si = scale.data()[i];
      if ( si == F(0) ) continue;
      if ( a * best_s > best_a * si ) {
        best_a = a;
        best_s = si;
        piv = i;
      }
    }
    if ( best_a == F(0) ) {
      r.singular = true;
      return r;
    }

    if ( piv != k ) {
      for ( usize j = 0; j < N; ++j ) {
        F t = LU[k * ld + j];
        LU[k * ld + j] = LU[piv * ld + j];
        LU[piv * ld + j] = t;
      }
      usize t = r.perm.data()[k];
      r.perm.data()[k] = r.perm.data()[piv];
      r.perm.data()[piv] = t;
      F ts = scale.data()[k];
      scale.data()[k] = scale.data()[piv];
      scale.data()[piv] = ts;
      r.sign = -r.sign;
    }

    const F inv_piv = F(1) / LU[k * ld + k];
    for ( usize i = k + 1; i < N; ++i ) LU[i * ld + k] *= inv_piv;
    for ( usize i = k + 1; i < N; ++i ) {
      const F m = LU[i * ld + k];
      for ( usize j = k + 1; j < N; ++j ) {
        LU[i * ld + j] = math::fma<F>(-m, LU[k * ld + j], LU[i * ld + j]);
      }
    }
  }
  return r;
}

template <ieee754_floating F, usize N>
inline void
lu_solve(const lu_pivot_result<F, N> &lu, vec<F, N> &x_inout) noexcept
{
  vec<F, N> b{};
  for ( usize i = 0; i < N; ++i ) b.data[i] = x_inout.data[lu.perm[i]];
  for ( usize i = 0; i < N; ++i ) {
    F s = b.data[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-lu.LU.data[i * N + j], b.data[j], s);
    b.data[i] = s;
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = b.data[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-lu.LU.data[ii * N + j], b.data[j], s);
    b.data[ii] = s / lu.LU.data[ii * N + ii];
  }
  x_inout = b;
}

template <ieee754_floating F>
inline void
lu_solve(const lu_pivot_result_dyn<F> &lu, dynvec<F> &x_inout) noexcept
{
  const usize N = lu.LU.rows;
  const usize ld = lu.LU.ld;
  const F *__restrict__ LU = lu.LU.data();
  dynvec<F> b(N);
  for ( usize i = 0; i < N; ++i ) b[i] = x_inout[lu.perm.data()[i]];
  for ( usize i = 0; i < N; ++i ) {
    F s = b[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-LU[i * ld + j], b[j], s);
    b[i] = s;
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = b[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-LU[ii * ld + j], b[j], s);
    b[ii] = s / LU[ii * ld + ii];
  }
  x_inout = micron::move(b);
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline F
det(const mat<F, N, N> &A) noexcept
{
  auto lu = lu_pivot<F, N>(A);
  if ( lu.singular ) return F(0);
  F p = F(lu.sign);
  for ( usize i = 0; i < N; ++i ) p *= lu.LU.data[i * N + i];
  return p;
}

template <ieee754_floating F>
[[nodiscard]] inline F
det(const dynmat<F> &A) noexcept
{
  auto lu = lu_pivot<F>(A);
  if ( lu.singular ) return F(0);
  const usize N = A.rows;
  const usize ld = lu.LU.ld;
  F p = F(lu.sign);
  for ( usize i = 0; i < N; ++i ) p *= lu.LU.data()[i * ld + i];
  return p;
}

template <ieee754_floating F> struct log_det_result {
  F log_abs;
  int sign;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline log_det_result<F>
log_det(const mat<F, N, N> &A) noexcept
{
  auto lu = lu_pivot<F, N>(A);
  if ( lu.singular ) return { F(0), 0 };
  F la = F(0);
  int s = lu.sign;
  for ( usize i = 0; i < N; ++i ) {
    F u = lu.LU.data[i * N + i];
    if ( u < F(0) ) {
      s = -s;
      u = -u;
    }
    la += math::log(u);
  }
  return { la, s };
}

template <ieee754_floating F>
[[nodiscard]] inline log_det_result<F>
log_det(const dynmat<F> &A) noexcept
{
  auto lu = lu_pivot<F>(A);
  if ( lu.singular ) return { F(0), 0 };
  const usize N = A.rows;
  const usize ld = lu.LU.ld;
  F la = F(0);
  int s = lu.sign;
  for ( usize i = 0; i < N; ++i ) {
    F u = lu.LU.data()[i * ld + i];
    if ( u < F(0) ) {
      s = -s;
      u = -u;
    }
    la += math::log(u);
  }
  return { la, s };
}

template <ieee754_floating F, usize N> struct inv_result {
  mat<F, N, N> X;
  bool singular;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline inv_result<F, N>
inv(const mat<F, N, N> &A) noexcept
{
  auto lu = lu_pivot<F, N>(A);
  inv_result<F, N> r{ mat<F, N, N>::zero(), lu.singular };
  if ( r.singular ) return r;
  for ( usize j = 0; j < N; ++j ) {
    vec<F, N> e{};
    for ( usize i = 0; i < N; ++i ) e.data[i] = (i == j) ? F(1) : F(0);
    lu_solve<F, N>(lu, e);
    for ( usize i = 0; i < N; ++i ) r.X.data[i * N + j] = e.data[i];
  }
  return r;
}

template <ieee754_floating F> struct inv_result_dyn {
  dynmat<F> X;
  bool singular;
};

template <ieee754_floating F>
[[nodiscard]] inline inv_result_dyn<F>
inv(const dynmat<F> &A) noexcept
{
  auto lu = lu_pivot<F>(A);
  const usize N = A.rows;
  inv_result_dyn<F> r{ dynmat<F>::zero(N, N), lu.singular };
  if ( r.singular ) return r;
  for ( usize j = 0; j < N; ++j ) {
    dynvec<F> e(N, F(0));
    e[j] = F(1);
    lu_solve<F>(lu, e);
    for ( usize i = 0; i < N; ++i ) r.X.at(i, j) = e[i];
  }
  return r;
}

template <ieee754_floating F, usize R_, usize C_> struct qr_householder_result {
  mat<F, R_, R_> Q;
  mat<F, R_, C_> R;
};

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard]] inline qr_householder_result<F, R_, C_>
qr_householder(const mat<F, R_, C_> &A) noexcept
{
  qr_householder_result<F, R_, C_> r{ mat<F, R_, R_>::identity(), A };
  constexpr usize K = (R_ < C_) ? R_ : C_;
  for ( usize k = 0; k < K; ++k ) {
    vec<F, R_> x{};
    for ( usize i = 0; i < R_; ++i ) x.data[i] = F(0);
    for ( usize i = k; i < R_; ++i ) x.data[i] = r.R.data[i * C_ + k];
    auto h = householder_reflector<F, R_>(x, k);
    if ( h.beta == F(0) ) continue;
    apply_householder_left<F, R_, C_>(r.R, h.v, h.beta, k);
    apply_householder_right<F, R_, R_>(r.Q, h.v, h.beta, k);
    for ( usize i = k + 1; i < R_; ++i ) r.R.data[i * C_ + k] = F(0);
  }
  return r;
}

template <ieee754_floating F> struct qr_householder_result_dyn {
  dynmat<F> Q;
  dynmat<F> R;
};

namespace __impl_decomp
{

template <ieee754_floating F>
inline void
apply_householder_left_dyn(F *__restrict__ A, usize R, usize C, usize ld_A, const F *v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  micron::vector<F, micron::allocator_serial<>, false> w(C, F(0));
  for ( usize i = k0; i < R; ++i ) {
    F vi = v[i];
    if ( vi == F(0) ) continue;
    const F *row = A + i * ld_A;
    for ( usize j = 0; j < C; ++j ) w.data()[j] = math::fma<F>(vi, row[j], w.data()[j]);
  }
  for ( usize i = k0; i < R; ++i ) {
    F vi = v[i];
    if ( vi == F(0) ) continue;
    F bvi = beta * vi;
    F *row = A + i * ld_A;
    for ( usize j = 0; j < C; ++j ) row[j] -= bvi * w.data()[j];
  }
}

template <ieee754_floating F>
inline void
apply_householder_right_dyn(F *__restrict__ A, usize R, usize C, usize ld_A, const F *v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  micron::vector<F, micron::allocator_serial<>, false> w(R, F(0));
  for ( usize i = 0; i < R; ++i ) {
    F acc = F(0);
    const F *row = A + i * ld_A;
    for ( usize j = k0; j < C; ++j ) acc = math::fma<F>(row[j], v[j], acc);
    w.data()[i] = acc;
  }
  for ( usize i = 0; i < R; ++i ) {
    F bwi = beta * w.data()[i];
    F *row = A + i * ld_A;
    for ( usize j = k0; j < C; ++j ) row[j] -= bwi * v[j];
  }
}

template <ieee754_floating F>
inline F
householder_reflector_dyn(const F *x, usize R, usize k0, F *v_out) noexcept
{
  for ( usize i = 0; i < R; ++i ) v_out[i] = F(0);
  if ( k0 >= R ) return F(0);
  F xmax = F(0);
  for ( usize i = k0; i < R; ++i ) {
    F a = math::fabs(x[i]);
    if ( a > xmax ) xmax = a;
  }
  if ( xmax == F(0) ) return F(0);
  F sum = F(0);
  F inv_xmax = F(1) / xmax;
  for ( usize i = k0; i < R; ++i ) {
    F s = x[i] * inv_xmax;
    sum = math::fma<F>(s, s, sum);
  }
  F norm_x = xmax * math::fsqrt(sum);
  F xk = x[k0];
  F sign_xk = (xk >= F(0)) ? F(1) : F(-1);
  F alpha = -sign_xk * norm_x;
  v_out[k0] = xk - alpha;
  for ( usize i = k0 + 1; i < R; ++i ) v_out[i] = x[i];
  F vv = v_out[k0] * v_out[k0];
  for ( usize i = k0 + 1; i < R; ++i ) vv = math::fma<F>(v_out[i], v_out[i], vv);
  return (vv > F(0)) ? F(2) / vv : F(0);
}

};     // namespace __impl_decomp

template <ieee754_floating F>
[[nodiscard]] inline qr_householder_result_dyn<F>
qr_householder(const dynmat<F> &A) noexcept
{
  qr_householder_result_dyn<F> r{ dynmat<F>::identity(A.rows), A };
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  micron::vector<F, micron::allocator_serial<>, false> x(R);
  micron::vector<F, micron::allocator_serial<>, false> v(R);
  for ( usize k = 0; k < K; ++k ) {
    for ( usize i = 0; i < R; ++i ) x.data()[i] = F(0);
    for ( usize i = k; i < R; ++i ) x.data()[i] = r.R.at(i, k);
    F beta = __impl_decomp::householder_reflector_dyn<F>(x.data(), R, k, v.data());
    if ( beta == F(0) ) continue;
    __impl_decomp::apply_householder_left_dyn<F>(r.R.data(), R, C, r.R.ld, v.data(), beta, k);
    __impl_decomp::apply_householder_right_dyn<F>(r.Q.data(), R, R, r.Q.ld, v.data(), beta, k);
    for ( usize i = k + 1; i < R; ++i ) r.R.at(i, k) = F(0);
  }
  return r;
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline F
rcond(const mat<F, N, N> &A) noexcept
{
  F nA = F(0);
  for ( usize j = 0; j < N; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < N; ++i ) s += math::fabs(A.data[i * N + j]);
    if ( s > nA ) nA = s;
  }
  if ( nA == F(0) ) return F(0);
  auto inv_r = inv<F, N>(A);
  if ( inv_r.singular ) return F(0);
  F nInv = F(0);
  for ( usize j = 0; j < N; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < N; ++i ) s += math::fabs(inv_r.X.data[i * N + j]);
    if ( s > nInv ) nInv = s;
  }
  if ( nInv == F(0) ) return F(0);
  return F(1) / (nA * nInv);
}

template <ieee754_floating F>
[[nodiscard]] inline F
rcond(const dynmat<F> &A) noexcept
{
  const usize N = A.rows;
  F nA = F(0);
  for ( usize j = 0; j < N; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < N; ++i ) s += math::fabs(A.at(i, j));
    if ( s > nA ) nA = s;
  }
  if ( nA == F(0) ) return F(0);
  auto inv_r = inv<F>(A);
  if ( inv_r.singular ) return F(0);
  F nInv = F(0);
  for ( usize j = 0; j < N; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < N; ++i ) s += math::fabs(inv_r.X.at(i, j));
    if ( s > nInv ) nInv = s;
  }
  if ( nInv == F(0) ) return F(0);
  return F(1) / (nA * nInv);
}

};     // namespace decomp
};     // namespace linalg
};     // namespace math
};     // namespace micron
