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
#include "householder_seq.hpp"
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

template<ieee754_floating F, usize N> struct lu_result {
  mat<F, N, N> L;
  mat<F, N, N> U;
  bool singular;
};

template<ieee754_floating F, usize N>
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
template<ieee754_floating F, usize N> struct chol_result {
  mat<F, N, N> L;
  bool spd;
};

template<ieee754_floating F, usize N>
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
template<ieee754_floating F, usize R_, usize C_> struct qr_result {
  mat<F, R_, C_> Q;
  mat<F, C_, C_> R;
};

template<ieee754_floating F, usize R_, usize C_>
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

template<ieee754_floating F> struct eigen_result2 {
  vec<F, 2> values;
  mat<F, 2, 2> vectors;
};

template<ieee754_floating F> struct eigen_result3 {
  vec<F, 3> values;
  mat<F, 3, 3> vectors;
};

template<ieee754_floating F>
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

template<ieee754_floating F>
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

template<ieee754_floating F> struct svd2_result {
  mat<F, 2, 2> U;
  vec<F, 2> S;
  mat<F, 2, 2> V;
};

template<ieee754_floating F>
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

  vec<F, 2> v0{ r.V.data[0], r.V.data[2] };      // col 0 of V
  vec<F, 2> v1{ r.V.data[1], r.V.data[3] };      // col 1 of V
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

template<ieee754_floating F> struct svd3_result {
  mat<F, 3, 3> U;
  vec<F, 3> S;
  mat<F, 3, 3> V;
};

template<ieee754_floating F>
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

template<ieee754_floating F, usize N> struct lu_pivot_result {
  mat<F, N, N> LU;
  usize perm[N];
  int sign;
  bool singular;
};

template<ieee754_floating F, usize N>
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
      if ( si == F(0) ) continue;      // zero row: skip
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
template<ieee754_floating F> struct lu_pivot_result_dyn {
  dynmat<F> LU;
  micron::vector<usize, micron::allocator_serial<>, false> perm;
  int sign;
  bool singular;
};

template<ieee754_floating F>
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

template<ieee754_floating F, usize N>
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

template<ieee754_floating F>
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

template<ieee754_floating F, usize N>
[[nodiscard]] inline F
det(const mat<F, N, N> &A) noexcept
{
  auto lu = lu_pivot<F, N>(A);
  if ( lu.singular ) return F(0);
  F p = F(lu.sign);
  for ( usize i = 0; i < N; ++i ) p *= lu.LU.data[i * N + i];
  return p;
}

template<ieee754_floating F>
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

template<ieee754_floating F> struct log_det_result {
  F log_abs;
  int sign;
};

template<ieee754_floating F, usize N>
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

template<ieee754_floating F>
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

template<ieee754_floating F, usize N> struct inv_result {
  mat<F, N, N> X;
  bool singular;
};

template<ieee754_floating F, usize N>
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

template<ieee754_floating F> struct inv_result_dyn {
  dynmat<F> X;
  bool singular;
};

template<ieee754_floating F>
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

template<ieee754_floating F, usize R_, usize C_> struct qr_householder_result {
  mat<F, R_, R_> Q;
  mat<F, R_, C_> R;
};

template<ieee754_floating F, usize R_, usize C_>
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

template<ieee754_floating F> struct qr_householder_result_dyn {
  dynmat<F> Q;
  dynmat<F> R;
};

template<ieee754_floating F>
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// full-pivoting LU
//
// P_r * A * P_c = L * U, with L unit lower / U upper packed into LU

template<ieee754_floating F, usize N> struct lu_full_result {
  mat<F, N, N> LU;
  usize perm_row[N];
  usize perm_col[N];
  int sign;
  bool singular;
  usize rank;
};

template<ieee754_floating F, usize N>
[[nodiscard]] inline lu_full_result<F, N>
lu_pivot_full(const mat<F, N, N> &A, F tol = F(0)) noexcept
{
  lu_full_result<F, N> r{ A, {}, {}, 1, false, 0 };
  for ( usize i = 0; i < N; ++i ) {
    r.perm_row[i] = i;
    r.perm_col[i] = i;
  }

  for ( usize k = 0; k < N; ++k ) {
    usize pi = k, pj = k;
    F best = math::fabs(r.LU.data[k * N + k]);
    for ( usize i = k; i < N; ++i ) {
      for ( usize j = k; j < N; ++j ) {
        F a = math::fabs(r.LU.data[i * N + j]);
        if ( a > best ) {
          best = a;
          pi = i;
          pj = j;
        }
      }
    }
    if ( best <= tol ) {
      r.singular = true;
      r.rank = k;
      return r;
    }
    if ( pi != k ) {
      for ( usize j = 0; j < N; ++j ) {
        F t = r.LU.data[k * N + j];
        r.LU.data[k * N + j] = r.LU.data[pi * N + j];
        r.LU.data[pi * N + j] = t;
      }
      usize t = r.perm_row[k];
      r.perm_row[k] = r.perm_row[pi];
      r.perm_row[pi] = t;
      r.sign = -r.sign;
    }
    if ( pj != k ) {
      for ( usize i = 0; i < N; ++i ) {
        F t = r.LU.data[i * N + k];
        r.LU.data[i * N + k] = r.LU.data[i * N + pj];
        r.LU.data[i * N + pj] = t;
      }
      usize t = r.perm_col[k];
      r.perm_col[k] = r.perm_col[pj];
      r.perm_col[pj] = t;
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
    r.rank = k + 1;
  }
  return r;
}

template<ieee754_floating F> struct lu_full_result_dyn {
  dynmat<F> LU;
  micron::vector<usize, micron::allocator_serial<>, false> perm_row;
  micron::vector<usize, micron::allocator_serial<>, false> perm_col;
  int sign;
  bool singular;
  usize rank;
};

template<ieee754_floating F>
[[nodiscard]] inline lu_full_result_dyn<F>
lu_pivot_full(const dynmat<F> &A, F tol = F(0)) noexcept
{
  const usize N = A.rows;
  lu_full_result_dyn<F> r{
    A, micron::vector<usize, micron::allocator_serial<>, false>(N), micron::vector<usize, micron::allocator_serial<>, false>(N), 1, false,
    0,
  };
  for ( usize i = 0; i < N; ++i ) {
    r.perm_row.data()[i] = i;
    r.perm_col.data()[i] = i;
  }
  F *__restrict__ LU = r.LU.data();
  const usize ld = r.LU.ld;

  for ( usize k = 0; k < N; ++k ) {
    usize pi = k, pj = k;
    F best = math::fabs(LU[k * ld + k]);
    for ( usize i = k; i < N; ++i ) {
      for ( usize j = k; j < N; ++j ) {
        F a = math::fabs(LU[i * ld + j]);
        if ( a > best ) {
          best = a;
          pi = i;
          pj = j;
        }
      }
    }
    if ( best <= tol ) {
      r.singular = true;
      r.rank = k;
      return r;
    }
    if ( pi != k ) {
      for ( usize j = 0; j < N; ++j ) {
        F t = LU[k * ld + j];
        LU[k * ld + j] = LU[pi * ld + j];
        LU[pi * ld + j] = t;
      }
      usize t = r.perm_row.data()[k];
      r.perm_row.data()[k] = r.perm_row.data()[pi];
      r.perm_row.data()[pi] = t;
      r.sign = -r.sign;
    }
    if ( pj != k ) {
      for ( usize i = 0; i < N; ++i ) {
        F t = LU[i * ld + k];
        LU[i * ld + k] = LU[i * ld + pj];
        LU[i * ld + pj] = t;
      }
      usize t = r.perm_col.data()[k];
      r.perm_col.data()[k] = r.perm_col.data()[pj];
      r.perm_col.data()[pj] = t;
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
    r.rank = k + 1;
  }
  return r;
}

template<ieee754_floating F, usize N>
[[nodiscard]] inline vec<F, N>
lu_full_solve(const lu_full_result<F, N> &lu, const vec<F, N> &b) noexcept
{
  // y = P_r * b
  vec<F, N> y{};
  for ( usize i = 0; i < N; ++i ) y.data[i] = b.data[lu.perm_row[i]];
  for ( usize i = 0; i < N; ++i ) {
    F s = y.data[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-lu.LU.data[i * N + j], y.data[j], s);
    y.data[i] = s;
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = y.data[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-lu.LU.data[ii * N + j], y.data[j], s);
    y.data[ii] = s / lu.LU.data[ii * N + ii];
  }
  // x = P_c * w  ->  x[perm_col[k]] = w[k]
  vec<F, N> x{};
  for ( usize i = 0; i < N; ++i ) x.data[i] = F(0);
  for ( usize k = 0; k < N; ++k ) x.data[lu.perm_col[k]] = y.data[k];
  return x;
}

template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
lu_full_solve(const lu_full_result_dyn<F> &lu, const dynvec<F> &b) noexcept
{
  const usize N = lu.LU.rows;
  const usize ld = lu.LU.ld;
  const F *__restrict__ LU = lu.LU.data();
  dynvec<F> y(N, F(0));
  for ( usize i = 0; i < N; ++i ) y[i] = b[lu.perm_row.data()[i]];
  for ( usize i = 0; i < N; ++i ) {
    F s = y[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-LU[i * ld + j], y[j], s);
    y[i] = s;
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = y[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-LU[ii * ld + j], y[j], s);
    y[ii] = s / LU[ii * ld + ii];
  }
  dynvec<F> x(N, F(0));
  for ( usize k = 0; k < N; ++k ) x[lu.perm_col.data()[k]] = y[k];
  return x;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// column-pivoting QR
//
// A * P = Q * R, R upper trapezoidal, Q orthogonal stored compactly

template<ieee754_floating F, usize R_, usize C_>
  requires(R_ >= 2)
struct qr_colpiv_result {
  householder_sequence<F, R_, C_> Q;
  mat<F, R_, C_> R;
  usize perm[C_];
  int sign;
  usize rank;
};

template<ieee754_floating F, usize R_, usize C_>
  requires(R_ >= 2)
[[nodiscard]] inline qr_colpiv_result<F, R_, C_>
qr_householder_colpiv(const mat<F, R_, C_> &A, F tol = F(0)) noexcept
{
  qr_colpiv_result<F, R_, C_> r{};
  constexpr usize K = (R_ < C_) ? R_ : C_;
  r.R = A;
  for ( usize j = 0; j < C_; ++j ) r.perm[j] = j;
  r.sign = 1;
  r.rank = 0;
  for ( usize i = 0; i < R_ * K; ++i ) r.Q.V.data[i] = F(0);
  for ( usize k = 0; k < K; ++k ) r.Q.betas[k] = F(0);

  F c[C_];
  for ( usize j = 0; j < C_; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < R_; ++i ) {
      const F v = r.R.data[i * C_ + j];
      s = math::fma<F>(v, v, s);
    }
    c[j] = s;
  }

  for ( usize k = 0; k < K; ++k ) {
    usize p = k;
    F cmax = c[k];
    for ( usize j = k + 1; j < C_; ++j ) {
      if ( c[j] > cmax ) {
        cmax = c[j];
        p = j;
      }
    }
    if ( cmax <= tol ) break;

    if ( p != k ) {
      for ( usize i = 0; i < R_; ++i ) {
        F t = r.R.data[i * C_ + k];
        r.R.data[i * C_ + k] = r.R.data[i * C_ + p];
        r.R.data[i * C_ + p] = t;
      }
      F ct = c[k];
      c[k] = c[p];
      c[p] = ct;
      usize pt = r.perm[k];
      r.perm[k] = r.perm[p];
      r.perm[p] = pt;
      r.sign = -r.sign;
    }

    vec<F, R_> x{};
    for ( usize i = 0; i < R_; ++i ) x.data[i] = F(0);
    for ( usize i = k; i < R_; ++i ) x.data[i] = r.R.data[i * C_ + k];
    auto h = householder_reflector<F, R_>(x, k);
    if ( h.beta == F(0) ) break;
    apply_householder_left<F, R_, C_>(r.R, h.v, h.beta, k);
    for ( usize i = k + 1; i < R_; ++i ) r.R.data[i * C_ + k] = F(0);
    for ( usize i = 0; i < R_; ++i ) r.Q.V.data[i * K + k] = h.v.data[i];
    r.Q.betas[k] = h.beta;
    r.rank = k + 1;

    for ( usize j = k + 1; j < C_; ++j ) {
      const F akj = r.R.data[k * C_ + j];
      F nc = c[j] - akj * akj;
      if ( nc < F(0) ) nc = F(0);
      c[j] = nc;
    }
  }
  return r;
}

// dynamic-shape twin
template<ieee754_floating F> struct qr_colpiv_result_dyn {
  householder_sequence_dyn<F> Q;
  dynmat<F> R;
  micron::vector<usize, micron::allocator_serial<>, false> perm;
  int sign;
  usize rank;
};

template<ieee754_floating F>
[[nodiscard]] inline qr_colpiv_result_dyn<F>
qr_householder_colpiv(const dynmat<F> &A, F tol = F(0)) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  qr_colpiv_result_dyn<F> r{};
  r.R = A;
  r.perm = micron::vector<usize, micron::allocator_serial<>, false>(C);
  for ( usize j = 0; j < C; ++j ) r.perm.data()[j] = j;
  r.sign = 1;
  r.rank = 0;
  r.Q.R = R;
  r.Q.K = K;
  r.Q.V = dynmat<F>::zero(R, K);
  r.Q.betas = micron::vector<F, micron::allocator_serial<>, false>(K, F(0));

  micron::vector<F, micron::allocator_serial<>, false> c(C, F(0));
  for ( usize j = 0; j < C; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < R; ++i ) {
      const F v = r.R.at(i, j);
      s = math::fma<F>(v, v, s);
    }
    c.data()[j] = s;
  }

  micron::vector<F, micron::allocator_serial<>, false> x(R, F(0));
  micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
  for ( usize k = 0; k < K; ++k ) {
    usize p = k;
    F cmax = c.data()[k];
    for ( usize j = k + 1; j < C; ++j ) {
      if ( c.data()[j] > cmax ) {
        cmax = c.data()[j];
        p = j;
      }
    }
    if ( cmax <= tol ) break;

    if ( p != k ) {
      for ( usize i = 0; i < R; ++i ) {
        F t = r.R.at(i, k);
        r.R.at(i, k) = r.R.at(i, p);
        r.R.at(i, p) = t;
      }
      F ct = c.data()[k];
      c.data()[k] = c.data()[p];
      c.data()[p] = ct;
      usize pt = r.perm.data()[k];
      r.perm.data()[k] = r.perm.data()[p];
      r.perm.data()[p] = pt;
      r.sign = -r.sign;
    }

    for ( usize i = 0; i < R; ++i ) x.data()[i] = F(0);
    for ( usize i = k; i < R; ++i ) x.data()[i] = r.R.at(i, k);
    F beta = __impl_decomp::householder_reflector_dyn<F>(x.data(), R, k, v.data());
    if ( beta == F(0) ) break;
    __impl_decomp::apply_householder_left_dyn<F>(r.R.data(), R, C, r.R.ld, v.data(), beta, k);
    for ( usize i = k + 1; i < R; ++i ) r.R.at(i, k) = F(0);
    for ( usize i = 0; i < R; ++i ) r.Q.V.at(i, k) = v.data()[i];
    r.Q.betas.data()[k] = beta;
    r.rank = k + 1;

    for ( usize j = k + 1; j < C; ++j ) {
      const F akj = r.R.at(k, j);
      F nc = c.data()[j] - akj * akj;
      if ( nc < F(0) ) nc = F(0);
      c.data()[j] = nc;
    }
  }
  return r;
}

template<ieee754_floating F, usize R_, usize C_>
[[nodiscard]] inline vec<F, C_>
qr_colpiv_solve(const qr_colpiv_result<F, R_, C_> &qr, const vec<F, R_> &b) noexcept
{
  vec<F, R_> y{};
  for ( usize i = 0; i < R_; ++i ) y.data[i] = b.data[i];
  // y := Q^T b
  mat<F, R_, 1> Y{};
  for ( usize i = 0; i < R_; ++i ) Y.data[i] = y.data[i];
  qr.Q.template apply_left_transposed<1>(Y);
  for ( usize i = 0; i < R_; ++i ) y.data[i] = Y.data[i];
  // back-substitute R[0:rank, 0:rank] * z = y[0:rank]; z[rank:] = 0
  vec<F, C_> z{};
  for ( usize j = 0; j < C_; ++j ) z.data[j] = F(0);
  if ( qr.rank > 0 ) {
    for ( usize ii = qr.rank; ii-- > 0; ) {
      F s = y.data[ii];
      for ( usize j = ii + 1; j < qr.rank; ++j ) s -= qr.R.data[ii * C_ + j] * z.data[j];
      z.data[ii] = s / qr.R.data[ii * C_ + ii];
    }
  }
  // x = P z   (z is in pivoted order; x[perm[k]] = z[k])
  vec<F, C_> x{};
  for ( usize j = 0; j < C_; ++j ) x.data[j] = F(0);
  for ( usize k = 0; k < C_; ++k ) x.data[qr.perm[k]] = z.data[k];
  return x;
}

template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
qr_colpiv_solve(const qr_colpiv_result_dyn<F> &qr, const dynvec<F> &b) noexcept
{
  const usize R = qr.Q.R;
  const usize C = qr.R.cols;
  dynmat<F> Y(R, 1);
  for ( usize i = 0; i < R; ++i ) Y.at(i, 0) = b[i];
  qr.Q.apply_left_transposed(Y);
  dynvec<F> z(C, F(0));
  if ( qr.rank > 0 ) {
    for ( usize ii = qr.rank; ii-- > 0; ) {
      F s = Y.at(ii, 0);
      for ( usize j = ii + 1; j < qr.rank; ++j ) s -= qr.R.at(ii, j) * z[j];
      z[ii] = s / qr.R.at(ii, ii);
    }
  }
  dynvec<F> x(C, F(0));
  for ( usize k = 0; k < C; ++k ) x[qr.perm.data()[k]] = z[k];
  return x;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// full-pivoting Householder QR (rank-revealing, max stability)
template<ieee754_floating F, usize R_, usize C_>
  requires(R_ >= 2)
struct qr_fullpiv_result {
  householder_sequence<F, R_, C_> Q;      // Householder part only (no row swaps)
  mat<F, R_, C_> R;
  usize row_transp[R_];      // sequence of row pivots (K used)
  usize perm_row[R_];        // cumulative row permutation
  usize perm_col[C_];
  int sign;
  usize rank;
};

template<ieee754_floating F, usize R_, usize C_>
  requires(R_ >= 2)
[[nodiscard]] inline qr_fullpiv_result<F, R_, C_>
qr_householder_fullpiv(const mat<F, R_, C_> &A, F tol = F(0)) noexcept
{
  qr_fullpiv_result<F, R_, C_> r{};
  constexpr usize K = (R_ < C_) ? R_ : C_;
  r.R = A;
  for ( usize i = 0; i < R_; ++i ) {
    r.perm_row[i] = i;
    r.row_transp[i] = i;
  }
  for ( usize j = 0; j < C_; ++j ) r.perm_col[j] = j;
  r.sign = 1;
  r.rank = 0;
  for ( usize i = 0; i < R_ * K; ++i ) r.Q.V.data[i] = F(0);
  for ( usize k = 0; k < K; ++k ) r.Q.betas[k] = F(0);

  for ( usize k = 0; k < K; ++k ) {
    usize pi = k, pj = k;
    F best = math::fabs(r.R.data[k * C_ + k]);
    for ( usize i = k; i < R_; ++i ) {
      for ( usize j = k; j < C_; ++j ) {
        F a = math::fabs(r.R.data[i * C_ + j]);
        if ( a > best ) {
          best = a;
          pi = i;
          pj = j;
        }
      }
    }
    if ( best <= tol ) break;
    r.row_transp[k] = pi;
    if ( pi != k ) {
      for ( usize j = 0; j < C_; ++j ) {
        F t = r.R.data[k * C_ + j];
        r.R.data[k * C_ + j] = r.R.data[pi * C_ + j];
        r.R.data[pi * C_ + j] = t;
      }
      usize t = r.perm_row[k];
      r.perm_row[k] = r.perm_row[pi];
      r.perm_row[pi] = t;
      r.sign = -r.sign;
    }
    if ( pj != k ) {
      for ( usize i = 0; i < R_; ++i ) {
        F t = r.R.data[i * C_ + k];
        r.R.data[i * C_ + k] = r.R.data[i * C_ + pj];
        r.R.data[i * C_ + pj] = t;
      }
      usize t = r.perm_col[k];
      r.perm_col[k] = r.perm_col[pj];
      r.perm_col[pj] = t;
      r.sign = -r.sign;
    }

    vec<F, R_> x{};
    for ( usize i = 0; i < R_; ++i ) x.data[i] = F(0);
    for ( usize i = k; i < R_; ++i ) x.data[i] = r.R.data[i * C_ + k];
    auto h = householder_reflector<F, R_>(x, k);
    if ( h.beta == F(0) ) break;
    apply_householder_left<F, R_, C_>(r.R, h.v, h.beta, k);
    for ( usize i = k + 1; i < R_; ++i ) r.R.data[i * C_ + k] = F(0);
    for ( usize i = 0; i < R_; ++i ) r.Q.V.data[i * K + k] = h.v.data[i];
    r.Q.betas[k] = h.beta;
    r.rank = k + 1;
  }
  return r;
}

template<ieee754_floating F> struct qr_fullpiv_result_dyn {
  householder_sequence_dyn<F> Q;
  dynmat<F> R;
  micron::vector<usize, micron::allocator_serial<>, false> row_transp;      // length K
  micron::vector<usize, micron::allocator_serial<>, false> perm_row;        // cumulative
  micron::vector<usize, micron::allocator_serial<>, false> perm_col;
  int sign;
  usize rank;
};

template<ieee754_floating F>
[[nodiscard]] inline qr_fullpiv_result_dyn<F>
qr_householder_fullpiv(const dynmat<F> &A, F tol = F(0)) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  qr_fullpiv_result_dyn<F> r{};
  r.R = A;
  r.row_transp = micron::vector<usize, micron::allocator_serial<>, false>(K);
  r.perm_row = micron::vector<usize, micron::allocator_serial<>, false>(R);
  r.perm_col = micron::vector<usize, micron::allocator_serial<>, false>(C);
  for ( usize i = 0; i < K; ++i ) r.row_transp.data()[i] = i;
  for ( usize i = 0; i < R; ++i ) r.perm_row.data()[i] = i;
  for ( usize j = 0; j < C; ++j ) r.perm_col.data()[j] = j;
  r.sign = 1;
  r.rank = 0;
  r.Q.R = R;
  r.Q.K = K;
  r.Q.V = dynmat<F>::zero(R, K);
  r.Q.betas = micron::vector<F, micron::allocator_serial<>, false>(K, F(0));

  micron::vector<F, micron::allocator_serial<>, false> x(R, F(0));
  micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
  for ( usize k = 0; k < K; ++k ) {
    usize pi = k, pj = k;
    F best = math::fabs(r.R.at(k, k));
    for ( usize i = k; i < R; ++i ) {
      for ( usize j = k; j < C; ++j ) {
        F a = math::fabs(r.R.at(i, j));
        if ( a > best ) {
          best = a;
          pi = i;
          pj = j;
        }
      }
    }
    if ( best <= tol ) break;
    r.row_transp.data()[k] = pi;
    if ( pi != k ) {
      for ( usize j = 0; j < C; ++j ) {
        F t = r.R.at(k, j);
        r.R.at(k, j) = r.R.at(pi, j);
        r.R.at(pi, j) = t;
      }
      usize t = r.perm_row.data()[k];
      r.perm_row.data()[k] = r.perm_row.data()[pi];
      r.perm_row.data()[pi] = t;
      r.sign = -r.sign;
    }
    if ( pj != k ) {
      for ( usize i = 0; i < R; ++i ) {
        F t = r.R.at(i, k);
        r.R.at(i, k) = r.R.at(i, pj);
        r.R.at(i, pj) = t;
      }
      usize t = r.perm_col.data()[k];
      r.perm_col.data()[k] = r.perm_col.data()[pj];
      r.perm_col.data()[pj] = t;
      r.sign = -r.sign;
    }

    for ( usize i = 0; i < R; ++i ) x.data()[i] = F(0);
    for ( usize i = k; i < R; ++i ) x.data()[i] = r.R.at(i, k);
    F beta = __impl_decomp::householder_reflector_dyn<F>(x.data(), R, k, v.data());
    if ( beta == F(0) ) break;
    __impl_decomp::apply_householder_left_dyn<F>(r.R.data(), R, C, r.R.ld, v.data(), beta, k);
    for ( usize i = k + 1; i < R; ++i ) r.R.at(i, k) = F(0);
    for ( usize i = 0; i < R; ++i ) r.Q.V.at(i, k) = v.data()[i];
    r.Q.betas.data()[k] = beta;
    r.rank = k + 1;
  }
  return r;
}

template<ieee754_floating F, usize R_, usize C_>
[[nodiscard]] inline vec<F, C_>
qr_fullpiv_solve(const qr_fullpiv_result<F, R_, C_> &qr, const vec<F, R_> &b) noexcept
{
  constexpr usize K = (R_ < C_) ? R_ : C_;
  vec<F, R_> y{};
  for ( usize i = 0; i < R_; ++i ) y.data[i] = b.data[i];
  for ( usize k = 0; k < K; ++k ) {
    usize pi = qr.row_transp[k];
    if ( pi != k ) {
      F t = y.data[k];
      y.data[k] = y.data[pi];
      y.data[pi] = t;
    }
    F beta = qr.Q.betas[k];
    if ( beta == F(0) ) continue;
    F w = F(0);
    for ( usize i = k; i < R_; ++i ) w = math::fma<F>(qr.Q.V.data[i * K + k], y.data[i], w);
    F bw = beta * w;
    for ( usize i = k; i < R_; ++i ) y.data[i] -= bw * qr.Q.V.data[i * K + k];
  }
  vec<F, C_> z{};
  for ( usize j = 0; j < C_; ++j ) z.data[j] = F(0);
  if ( qr.rank > 0 ) {
    for ( usize ii = qr.rank; ii-- > 0; ) {
      F s = y.data[ii];
      for ( usize j = ii + 1; j < qr.rank; ++j ) s -= qr.R.data[ii * C_ + j] * z.data[j];
      z.data[ii] = s / qr.R.data[ii * C_ + ii];
    }
  }
  vec<F, C_> x{};
  for ( usize j = 0; j < C_; ++j ) x.data[j] = F(0);
  for ( usize k = 0; k < C_; ++k ) x.data[qr.perm_col[k]] = z.data[k];
  return x;
}

template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
qr_fullpiv_solve(const qr_fullpiv_result_dyn<F> &qr, const dynvec<F> &b) noexcept
{
  const usize R = qr.Q.R;
  const usize C = qr.R.cols;
  const usize K = qr.Q.K;
  dynvec<F> y(R, F(0));
  for ( usize i = 0; i < R; ++i ) y[i] = b[i];
  for ( usize k = 0; k < K; ++k ) {
    usize pi = qr.row_transp.data()[k];
    if ( pi != k ) {
      F t = y[k];
      y[k] = y[pi];
      y[pi] = t;
    }
    F beta = qr.Q.betas.data()[k];
    if ( beta == F(0) ) continue;
    F w = F(0);
    for ( usize i = k; i < R; ++i ) w = math::fma<F>(qr.Q.V.at(i, k), y[i], w);
    F bw = beta * w;
    for ( usize i = k; i < R; ++i ) y[i] -= bw * qr.Q.V.at(i, k);
  }
  dynvec<F> z(C, F(0));
  if ( qr.rank > 0 ) {
    for ( usize ii = qr.rank; ii-- > 0; ) {
      F s = y[ii];
      for ( usize j = ii + 1; j < qr.rank; ++j ) s -= qr.R.at(ii, j) * z[j];
      z[ii] = s / qr.R.at(ii, ii);
    }
  }
  dynvec<F> x(C, F(0));
  for ( usize k = 0; k < C; ++k ) x[qr.perm_col.data()[k]] = z[k];
  return x;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// orthogonal decomposition
//
// A * P_c = Q * [T  0] * Z

template<ieee754_floating F> struct cod_result_dyn {
  householder_sequence_dyn<F> Q;
  dynmat<F> T_lower;      // rank x rank lower triangular (rest zero)
  householder_sequence_dyn<F> Z;
  micron::vector<usize, micron::allocator_serial<>, false> perm_col;
  usize rank;
};

template<ieee754_floating F>
[[nodiscard]] inline cod_result_dyn<F>
cod(const dynmat<F> &A, F tol = F(0)) noexcept
{
  auto qr = qr_householder_colpiv<F>(A, tol);
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = qr.rank;

  cod_result_dyn<F> r{};
  r.Q = micron::move(qr.Q);
  r.perm_col = micron::move(qr.perm);
  r.rank = K;

  if ( K == 0 ) {
    r.T_lower = dynmat<F>::zero(0, 0);
    r.Z.R = C;
    r.Z.K = 0;
    r.Z.V = dynmat<F>::zero(C, 0);
    r.Z.betas = micron::vector<F, micron::allocator_serial<>, false>(0);
    return r;
  }

  // B = R[0:K, :]^T   (size C x K)
  dynmat<F> B(C, K);
  for ( usize i = 0; i < K; ++i )
    for ( usize j = 0; j < C; ++j ) B.at(j, i) = qr.R.at(i, j);

  // Phase 2: QR of B
  micron::vector<F, micron::allocator_serial<>, false> x(C, F(0));
  micron::vector<F, micron::allocator_serial<>, false> v(C, F(0));
  r.Z.R = C;
  r.Z.K = K;
  r.Z.V = dynmat<F>::zero(C, K);
  r.Z.betas = micron::vector<F, micron::allocator_serial<>, false>(K, F(0));

  for ( usize k = 0; k < K; ++k ) {
    for ( usize i = 0; i < C; ++i ) x.data()[i] = F(0);
    for ( usize i = k; i < C; ++i ) x.data()[i] = B.at(i, k);
    F beta = __impl_decomp::householder_reflector_dyn<F>(x.data(), C, k, v.data());
    if ( beta == F(0) ) continue;
    __impl_decomp::apply_householder_left_dyn<F>(B.data(), C, K, B.ld, v.data(), beta, k);
    for ( usize i = k + 1; i < C; ++i ) B.at(i, k) = F(0);
    for ( usize i = 0; i < C; ++i ) r.Z.V.at(i, k) = v.data()[i];
    r.Z.betas.data()[k] = beta;
  }

  // T = (B[0:K, :])^T   ->  K x K lower triangular
  r.T_lower = dynmat<F>::zero(K, K);
  for ( usize i = 0; i < K; ++i )
    for ( usize j = 0; j <= i; ++j ) r.T_lower.at(i, j) = B.at(j, i);

  return r;
}

template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
cod_solve(const cod_result_dyn<F> &c, const dynvec<F> &b) noexcept
{
  const usize R = c.Q.R;
  const usize C = c.Z.R;
  const usize K = c.rank;

  // y = Q^T b
  dynmat<F> Y(R, 1);
  for ( usize i = 0; i < R; ++i ) Y.at(i, 0) = b[i];
  c.Q.apply_left_transposed(Y);

  // forward solve T_lower * z = y[0:K]
  dynvec<F> z(K, F(0));
  for ( usize i = 0; i < K; ++i ) {
    F s = Y.at(i, 0);
    for ( usize j = 0; j < i; ++j ) s -= c.T_lower.at(i, j) * z[j];
    z[i] = s / c.T_lower.at(i, i);
  }

  // w has length C; w[0:K] = z, rest = 0
  dynmat<F> W(C, 1);
  for ( usize i = 0; i < C; ++i ) W.at(i, 0) = F(0);
  for ( usize i = 0; i < K; ++i ) W.at(i, 0) = z[i];

  c.Z.apply_left(W);

  dynvec<F> x_perm(C, F(0));
  for ( usize i = 0; i < C; ++i ) x_perm[i] = W.at(i, 0);

  dynvec<F> x(C, F(0));
  for ( usize k = 0; k < C; ++k ) x[c.perm_col.data()[k]] = x_perm[k];
  return x;
}

template<ieee754_floating F, usize N>
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

template<ieee754_floating F>
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

};      // namespace decomp
};      // namespace linalg
};      // namespace math
};      // namespace micron
