//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "decomp.hpp"
#include "generic.hpp"
#include "householder.hpp"
#include "ops.hpp"
#include "types.hpp"

// doing it this way due to spaghetti

namespace micron
{
namespace math
{
namespace linalg
{
namespace decomp
{

template <ieee754_floating F, usize N> struct eigen_sym_result {
  vec<F, N> values;
  mat<F, N, N> vectors;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline eigen_sym_result<F, N>
eigen_sym(const mat<F, N, N> &A) noexcept
{
  eigen_sym_result<F, N> r{};
  if constexpr ( N == 1 ) {
    r.values.data[0] = A.data[0];
    r.vectors = mat<F, N, N>::identity();
    r.converged = true;
    return r;
  } else {
    mat<F, N, N> M = A;
    mat<F, N, N> V = mat<F, N, N>::identity();
    constexpr int max_sweeps = 50;
    const F eps = default_eps<F>();
    bool converged = false;

    for ( int sweep = 0; sweep < max_sweeps; ++sweep ) {
      // off-diagonal absolute sum (upper triangle only)
      F off = F(0);
      for ( usize i = 0; i + 1 < N; ++i )
        for ( usize j = i + 1; j < N; ++j ) off += math::fabs(M.data[i * N + j]);
      if ( off < eps ) {
        converged = true;
        break;
      }

      for ( usize p = 0; p + 1 < N; ++p ) {
        for ( usize q = p + 1; q < N; ++q ) {
          F apq = M.data[p * N + q];
          if ( math::fabs(apq) < eps ) continue;
          F app = M.data[p * N + p];
          F aqq = M.data[q * N + q];
          F theta = (aqq - app) / (F(2) * apq);
          F t;
          if ( theta >= F(0) )
            t = F(1) / (theta + math::fsqrt(F(1) + theta * theta));
          else
            t = F(1) / (theta - math::fsqrt(F(1) + theta * theta));
          F c = F(1) / math::fsqrt(F(1) + t * t);
          F s = t * c;

          M.data[p * N + p] = app - t * apq;
          M.data[q * N + q] = aqq + t * apq;
          M.data[p * N + q] = F(0);
          M.data[q * N + p] = F(0);

          for ( usize rr = 0; rr < N; ++rr ) {
            if ( rr == p || rr == q ) continue;
            F arp = M.data[rr * N + p];
            F arq = M.data[rr * N + q];
            F new_arp = c * arp - s * arq;
            F new_arq = s * arp + c * arq;
            M.data[rr * N + p] = new_arp;
            M.data[p * N + rr] = new_arp;
            M.data[rr * N + q] = new_arq;
            M.data[q * N + rr] = new_arq;
          }
          for ( usize rr = 0; rr < N; ++rr ) {
            F vrp = V.data[rr * N + p];
            F vrq = V.data[rr * N + q];
            V.data[rr * N + p] = c * vrp - s * vrq;
            V.data[rr * N + q] = s * vrp + c * vrq;
          }
        }
      }
    }

    for ( usize i = 0; i < N; ++i ) r.values.data[i] = M.data[i * N + i];
    r.vectors = V;
    r.converged = converged;
    return r;
  }
}

template <ieee754_floating F, usize N> struct schur_result {
  mat<F, N, N> T;
  mat<F, N, N> Z;
  bool converged;
};

namespace __impl_decomp_schur
{

template <ieee754_floating F, usize N>
inline void
francis_step(mat<F, N, N> &H, mat<F, N, N> &Z, usize lo, usize hi, F shift_s, F shift_t) noexcept
{

  const F a00 = H.data[lo * N + lo];
  const F a01 = H.data[lo * N + (lo + 1)];
  const F a10 = H.data[(lo + 1) * N + lo];
  const F a11 = H.data[(lo + 1) * N + (lo + 1)];
  const F a21 = H.data[(lo + 2) * N + (lo + 1)];

  F x = a00 * a00 + a01 * a10 - shift_s * a00 + shift_t;
  F y = a10 * (a00 + a11 - shift_s);
  F z = a10 * a21;

  for ( usize k = lo; k + 1 <= hi; ++k ) {
    const bool short_ref = (k + 1 == hi);
    if ( k > lo ) {
      x = H.data[k * N + (k - 1)];
      y = H.data[(k + 1) * N + (k - 1)];
      z = (k + 2 <= hi) ? H.data[(k + 2) * N + (k - 1)] : F(0);
    }
    if ( short_ref ) z = F(0);

    F nrm_sq = x * x + y * y + z * z;
    if ( nrm_sq == F(0) ) continue;
    F nrm = math::fsqrt(nrm_sq);
    F sign_x = (x >= F(0)) ? F(1) : F(-1);
    F alpha = -sign_x * nrm;
    F v0 = x - alpha;
    F v1 = y;
    F v2 = z;
    F vv = v0 * v0 + v1 * v1 + v2 * v2;
    if ( vv == F(0) ) continue;
    F beta = F(2) / vv;

    const usize col_lo = (k > lo) ? (k - 1) : lo;
    for ( usize cc = col_lo; cc < N; ++cc ) {
      F h0 = H.data[k * N + cc];
      F h1 = H.data[(k + 1) * N + cc];
      F h2 = short_ref ? F(0) : H.data[(k + 2) * N + cc];
      F w = v0 * h0 + v1 * h1 + (short_ref ? F(0) : v2 * h2);
      F bw = beta * w;
      H.data[k * N + cc] = h0 - bw * v0;
      H.data[(k + 1) * N + cc] = h1 - bw * v1;
      if ( !short_ref ) H.data[(k + 2) * N + cc] = h2 - bw * v2;
    }

    usize row_top = short_ref ? (k + 3) : (k + 4);
    if ( row_top > hi + 1 ) row_top = hi + 1;
    if ( row_top > N ) row_top = N;
    for ( usize rr = 0; rr < row_top; ++rr ) {
      F h0 = H.data[rr * N + k];
      F h1 = H.data[rr * N + (k + 1)];
      F h2 = short_ref ? F(0) : H.data[rr * N + (k + 2)];
      F w = h0 * v0 + h1 * v1 + (short_ref ? F(0) : h2 * v2);
      F bw = beta * w;
      H.data[rr * N + k] = h0 - bw * v0;
      H.data[rr * N + (k + 1)] = h1 - bw * v1;
      if ( !short_ref ) H.data[rr * N + (k + 2)] = h2 - bw * v2;
    }

    for ( usize rr = 0; rr < N; ++rr ) {
      F z0 = Z.data[rr * N + k];
      F z1 = Z.data[rr * N + (k + 1)];
      F z2 = short_ref ? F(0) : Z.data[rr * N + (k + 2)];
      F w = z0 * v0 + z1 * v1 + (short_ref ? F(0) : z2 * v2);
      F bw = beta * w;
      Z.data[rr * N + k] = z0 - bw * v0;
      Z.data[rr * N + (k + 1)] = z1 - bw * v1;
      if ( !short_ref ) Z.data[rr * N + (k + 2)] = z2 - bw * v2;
    }

    if ( k > lo ) {
      H.data[(k + 1) * N + (k - 1)] = F(0);
      if ( !short_ref && (k + 2 <= hi) ) H.data[(k + 2) * N + (k - 1)] = F(0);
    }
  }
}

};     // namespace __impl_decomp_schur

template <ieee754_floating F, usize N>
[[nodiscard]] inline schur_result<F, N>
schur(const mat<F, N, N> &A) noexcept
{
  schur_result<F, N> r{};
  if constexpr ( N == 1 ) {
    r.T = A;
    r.Z = mat<F, 1, 1>::identity();
    r.converged = true;
    return r;
  } else {
    auto h = hessenberg<F, N>(A);
    r.T = h.H;
    r.Z = h.Q;
    r.converged = false;

    const F eps = default_eps<F>();
    const int max_iter = int(30 * N) + 30;
    int iter = 0;
    int stagnant = 0;
    usize hi = N - 1;

    while ( true ) {

      while ( hi > 0 ) {
        F sub = math::fabs(r.T.data[hi * N + (hi - 1)]);
        F dd = math::fabs(r.T.data[(hi - 1) * N + (hi - 1)]) + math::fabs(r.T.data[hi * N + hi]);
        if ( sub <= eps * dd || sub == F(0) ) {
          r.T.data[hi * N + (hi - 1)] = F(0);
          --hi;
          stagnant = 0;
          continue;
        }
        break;
      }
      if ( hi == 0 ) {
        r.converged = true;
        break;
      }

      usize lo = hi;
      while ( lo > 0 ) {
        F sub = math::fabs(r.T.data[lo * N + (lo - 1)]);
        F dd = math::fabs(r.T.data[(lo - 1) * N + (lo - 1)]) + math::fabs(r.T.data[lo * N + lo]);
        if ( sub <= eps * dd ) {
          r.T.data[lo * N + (lo - 1)] = F(0);
          break;
        }
        --lo;
      }

      if ( hi - lo == 1 ) {

        if ( lo == 0 ) {
          r.converged = true;
          break;
        }
        hi = lo - 1;
        stagnant = 0;
        continue;
      }

      F shift_s, shift_t;
      const F p = r.T.data[(hi - 1) * N + (hi - 1)];
      const F q = r.T.data[hi * N + hi];
      const F rr_off = r.T.data[(hi - 1) * N + hi];
      const F ss_off = r.T.data[hi * N + (hi - 1)];
      if ( stagnant >= 10 && stagnant % 10 == 0 ) {

        F s_excep = math::fabs(r.T.data[hi * N + (hi - 1)]) + (hi >= 2 ? math::fabs(r.T.data[(hi - 1) * N + (hi - 2)]) : F(0));
        s_excep *= F(1.5);
        shift_s = F(2) * s_excep;
        shift_t = s_excep * s_excep;
      } else {
        shift_s = p + q;
        shift_t = p * q - rr_off * ss_off;
      }
      __impl_decomp_schur::francis_step<F, N>(r.T, r.Z, lo, hi, shift_s, shift_t);
      ++iter;
      ++stagnant;
      if ( iter >= max_iter ) {
        r.converged = false;
        break;
      }
    }
    return r;
  }
}

template <ieee754_floating F, usize N> struct eigen_result {
  vec<F, N> values_re;
  vec<F, N> values_im;
  mat<F, N, N> Z;
  mat<F, N, N> T;
  bool converged;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline eigen_result<F, N>
eigen(const mat<F, N, N> &A) noexcept
{
  eigen_result<F, N> r{};
  auto sr = schur<F, N>(A);
  r.T = sr.T;
  r.Z = sr.Z;
  r.converged = sr.converged;

  for ( usize i = 0; i < N; ++i ) {
    r.values_re.data[i] = F(0);
    r.values_im.data[i] = F(0);
  }
  usize i = 0;
  while ( i < N ) {
    bool is_block = (i + 1 < N) && (r.T.data[(i + 1) * N + i] != F(0));
    if ( !is_block ) {
      r.values_re.data[i] = r.T.data[i * N + i];
      r.values_im.data[i] = F(0);
      ++i;
    } else {
      F a = r.T.data[i * N + i];
      F b = r.T.data[i * N + (i + 1)];
      F c = r.T.data[(i + 1) * N + i];
      F d = r.T.data[(i + 1) * N + (i + 1)];
      F tr = a + d;
      F det = a * d - b * c;
      F disc = tr * tr * F(0.25) - det;
      if ( disc >= F(0) ) {
        F sq = math::fsqrt(disc);
        r.values_re.data[i] = tr * F(0.5) + sq;
        r.values_im.data[i] = F(0);
        r.values_re.data[i + 1] = tr * F(0.5) - sq;
        r.values_im.data[i + 1] = F(0);
      } else {
        F sq = math::fsqrt(-disc);
        r.values_re.data[i] = tr * F(0.5);
        r.values_im.data[i] = sq;
        r.values_re.data[i + 1] = tr * F(0.5);
        r.values_im.data[i + 1] = -sq;
      }
      i += 2;
    }
  }
  return r;
}

namespace __impl_decomp_svd
{

template <usize A, usize B> inline constexpr usize min_v = (A < B) ? A : B;

};

template <ieee754_floating F, usize R_, usize C_> struct svd_result {
  mat<F, R_, R_> U;
  vec<F, __impl_decomp_svd::min_v<R_, C_>> S;
  mat<F, C_, C_> V;
  bool converged;
};

template <ieee754_floating F, usize R_, usize C_>
  requires(R_ >= C_)
[[nodiscard]] inline svd_result<F, R_, C_>
svd(const mat<F, R_, C_> &A) noexcept
{
  svd_result<F, R_, C_> r{};
  r.U = mat<F, R_, R_>::zero();
  r.V = mat<F, C_, C_>::identity();
  r.converged = false;

  mat<F, R_, C_> W = A;
  constexpr int max_sweeps = 30;
  const F eps = default_eps<F>();

  for ( int sweep = 0; sweep < max_sweeps; ++sweep ) {
    F off = F(0);
    for ( usize p = 0; p + 1 < C_; ++p ) {
      for ( usize q = p + 1; q < C_; ++q ) {

        F a = F(0), b = F(0), c = F(0);
        for ( usize i = 0; i < R_; ++i ) {
          const F wp = W.data[i * C_ + p];
          const F wq = W.data[i * C_ + q];
          a = math::fma<F>(wp, wp, a);
          b = math::fma<F>(wq, wq, b);
          c = math::fma<F>(wp, wq, c);
        }
        off += math::fabs(c);
        if ( math::fabs(c) < eps * math::fsqrt(a * b + F(1e-30)) ) continue;

        F theta = (b - a) / (F(2) * c);
        F t;
        if ( theta >= F(0) )
          t = F(1) / (theta + math::fsqrt(F(1) + theta * theta));
        else
          t = F(1) / (theta - math::fsqrt(F(1) + theta * theta));
        F cs = F(1) / math::fsqrt(F(1) + t * t);
        F sn = t * cs;

        for ( usize i = 0; i < R_; ++i ) {
          F wp = W.data[i * C_ + p];
          F wq = W.data[i * C_ + q];
          W.data[i * C_ + p] = cs * wp - sn * wq;
          W.data[i * C_ + q] = sn * wp + cs * wq;
        }

        for ( usize i = 0; i < C_; ++i ) {
          F vp = r.V.data[i * C_ + p];
          F vq = r.V.data[i * C_ + q];
          r.V.data[i * C_ + p] = cs * vp - sn * vq;
          r.V.data[i * C_ + q] = sn * vp + cs * vq;
        }
      }
    }
    if ( off < eps ) {
      r.converged = true;
      break;
    }
  }

  for ( usize j = 0; j < C_; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < R_; ++i ) s = math::fma<F>(W.data[i * C_ + j], W.data[i * C_ + j], s);
    s = math::fsqrt(s);
    r.S.data[j] = s;
    if ( s > F(0) ) {
      F inv = F(1) / s;
      for ( usize i = 0; i < R_; ++i ) r.U.data[i * R_ + j] = W.data[i * C_ + j] * inv;
    } else {
      for ( usize i = 0; i < R_; ++i ) r.U.data[i * R_ + j] = F(0);
    }
  }

  for ( usize i = 0; i + 1 < C_; ++i ) {
    usize maxj = i;
    for ( usize j = i + 1; j < C_; ++j )
      if ( r.S.data[j] > r.S.data[maxj] ) maxj = j;
    if ( maxj != i ) {
      F tmp = r.S.data[i];
      r.S.data[i] = r.S.data[maxj];
      r.S.data[maxj] = tmp;
      for ( usize rr = 0; rr < R_; ++rr ) {
        F t = r.U.data[rr * R_ + i];
        r.U.data[rr * R_ + i] = r.U.data[rr * R_ + maxj];
        r.U.data[rr * R_ + maxj] = t;
      }
      for ( usize rr = 0; rr < C_; ++rr ) {
        F t = r.V.data[rr * C_ + i];
        r.V.data[rr * C_ + i] = r.V.data[rr * C_ + maxj];
        r.V.data[rr * C_ + maxj] = t;
      }
    }
  }
  return r;
}

namespace __impl_decomp_qrtri
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
tqli(F *d, F *e, F *Z, usize N, usize ld_Z, int max_iter = 30) noexcept
{
  if ( N <= 1 ) return true;

  for ( usize l = 0; l < N; ++l ) {
    int iter = 0;
    usize m;
    do {

      for ( m = l; m + 1 < N; ++m ) {
        F dd = math::fabs(d[m]) + math::fabs(d[m + 1]);
        if ( math::fabs(e[m]) + dd == dd ) break;
      }
      if ( m != l ) {
        if ( ++iter > max_iter ) return false;
        F g = (d[l + 1] - d[l]) / (F(2) * e[l]);
        F r = pythag<F>(g, F(1));
        g = d[m] - d[l] + e[l] / (g + sgn_copy<F>(r, g));
        F s = F(1), c = F(1), p = F(0);
        usize i = m;
        bool inner_break = false;
        while ( i > l ) {
          --i;
          F f = s * e[i];
          F b = c * e[i];
          r = pythag<F>(f, g);
          e[i + 1] = r;
          if ( r == F(0) ) {
            d[i + 1] -= p;
            e[m] = F(0);
            inner_break = true;
            break;
          }
          s = f / r;
          c = g / r;
          g = d[i + 1] - p;
          r = (d[i] - g) * s + F(2) * c * b;
          p = s * r;
          d[i + 1] = g + p;
          g = c * r - b;

          for ( usize k = 0; k < N; ++k ) {
            F zi = Z[k * ld_Z + i];
            F zip = Z[k * ld_Z + i + 1];
            Z[k * ld_Z + i + 1] = s * zi + c * zip;
            Z[k * ld_Z + i] = c * zi - s * zip;
          }
        }
        if ( inner_break ) continue;
        d[l] -= p;
        e[l] = g;
        e[m] = F(0);
      }
    } while ( m != l );
  }
  return true;
}

};     // namespace __impl_decomp_qrtri

template <ieee754_floating F, usize N>
[[nodiscard]] inline eigen_sym_result<F, N>
eigen_sym_qr(const mat<F, N, N> &A) noexcept
{
  eigen_sym_result<F, N> r{};
  if constexpr ( N == 1 ) {
    r.values.data[0] = A.data[0];
    r.vectors = mat<F, N, N>::identity();
    r.converged = true;
    return r;
  } else {
    auto tri = tridiagonalize_sym<F, N>(A);
    F d[N];
    F e[N];
    for ( usize i = 0; i < N; ++i ) d[i] = tri.diag.data[i];
    for ( usize i = 0; i < N; ++i ) e[i] = tri.subdiag.data[i];
    r.vectors = tri.Q;
    r.converged = __impl_decomp_qrtri::tqli<F>(d, e, r.vectors.data, N, N);
    for ( usize i = 0; i < N; ++i ) r.values.data[i] = d[i];

    for ( usize i = 0; i + 1 < N; ++i ) {
      usize mn = i;
      for ( usize j = i + 1; j < N; ++j )
        if ( r.values.data[j] < r.values.data[mn] ) mn = j;
      if ( mn != i ) {
        F tv = r.values.data[i];
        r.values.data[i] = r.values.data[mn];
        r.values.data[mn] = tv;
        for ( usize k = 0; k < N; ++k ) {
          F tt = r.vectors.data[k * N + i];
          r.vectors.data[k * N + i] = r.vectors.data[k * N + mn];
          r.vectors.data[k * N + mn] = tt;
        }
      }
    }
    return r;
  }
}

template <ieee754_floating F> struct eigen_sym_result_dyn {
  dynvec<F> values;
  dynmat<F> vectors;
  bool converged;
};

template <ieee754_floating F>
[[nodiscard]] inline eigen_sym_result_dyn<F>
eigen_sym_qr(const dynmat<F> &A) noexcept
{
  const usize N = A.rows;
  eigen_sym_result_dyn<F> r{ dynvec<F>(N), dynmat<F>::identity(N), true };
  if ( N <= 1 ) {
    if ( N == 1 ) r.values[0] = A.at(0, 0);
    return r;
  }

  dynmat<F> H = A;
  micron::vector<F, micron::allocator_serial<>, false> v(N);
  micron::vector<F, micron::allocator_serial<>, false> w(N);
  for ( usize k = 0; k + 2 < N; ++k ) {

    for ( usize i = 0; i < N; ++i ) v.data()[i] = F(0);
    F *col_buf = v.data();
    F xmax = F(0);
    for ( usize i = k + 1; i < N; ++i ) {
      F a = math::fabs(H.at(i, k));
      if ( a > xmax ) xmax = a;
    }
    if ( xmax == F(0) ) continue;
    F sum = F(0);
    F inv_xmax = F(1) / xmax;
    for ( usize i = k + 1; i < N; ++i ) {
      F s = H.at(i, k) * inv_xmax;
      sum = math::fma<F>(s, s, sum);
    }
    F norm_x = xmax * math::fsqrt(sum);
    F xk = H.at(k + 1, k);
    F sign_xk = (xk >= F(0)) ? F(1) : F(-1);
    F alpha = -sign_xk * norm_x;
    col_buf[k + 1] = xk - alpha;
    for ( usize i = k + 2; i < N; ++i ) col_buf[i] = H.at(i, k);
    F vv = col_buf[k + 1] * col_buf[k + 1];
    for ( usize i = k + 2; i < N; ++i ) vv = math::fma<F>(col_buf[i], col_buf[i], vv);
    F beta = (vv > F(0)) ? F(2) / vv : F(0);
    if ( beta == F(0) ) continue;

    for ( usize j = 0; j < N; ++j ) w.data()[j] = F(0);
    for ( usize i = k + 1; i < N; ++i ) {
      F vi = col_buf[i];
      if ( vi == F(0) ) continue;
      for ( usize j = 0; j < N; ++j ) w.data()[j] = math::fma<F>(vi, H.at(i, j), w.data()[j]);
    }
    for ( usize i = k + 1; i < N; ++i ) {
      F vi = col_buf[i];
      if ( vi == F(0) ) continue;
      F bvi = beta * vi;
      for ( usize j = 0; j < N; ++j ) H.at(i, j) -= bvi * w.data()[j];
    }

    for ( usize i = 0; i < N; ++i ) {
      F acc = F(0);
      for ( usize j = k + 1; j < N; ++j ) acc = math::fma<F>(H.at(i, j), col_buf[j], acc);
      w.data()[i] = acc;
    }
    for ( usize i = 0; i < N; ++i ) {
      F bwi = beta * w.data()[i];
      for ( usize j = k + 1; j < N; ++j ) H.at(i, j) -= bwi * col_buf[j];
    }

    for ( usize i = 0; i < N; ++i ) {
      F acc = F(0);
      for ( usize j = k + 1; j < N; ++j ) acc = math::fma<F>(r.vectors.at(i, j), col_buf[j], acc);
      w.data()[i] = acc;
    }
    for ( usize i = 0; i < N; ++i ) {
      F bwi = beta * w.data()[i];
      for ( usize j = k + 1; j < N; ++j ) r.vectors.at(i, j) -= bwi * col_buf[j];
    }

    for ( usize i = k + 2; i < N; ++i ) H.at(i, k) = F(0);
  }

  micron::vector<F, micron::allocator_serial<>, false> d(N);
  micron::vector<F, micron::allocator_serial<>, false> e(N);
  for ( usize i = 0; i < N; ++i ) d.data()[i] = H.at(i, i);
  for ( usize i = 0; i + 1 < N; ++i ) e.data()[i] = H.at(i + 1, i);
  e.data()[N - 1] = F(0);

  r.converged = __impl_decomp_qrtri::tqli<F>(d.data(), e.data(), r.vectors.data(), N, r.vectors.ld);
  for ( usize i = 0; i < N; ++i ) r.values[i] = d.data()[i];

  for ( usize i = 0; i + 1 < N; ++i ) {
    usize mn = i;
    for ( usize j = i + 1; j < N; ++j )
      if ( r.values[j] < r.values[mn] ) mn = j;
    if ( mn != i ) {
      F tv = r.values[i];
      r.values[i] = r.values[mn];
      r.values[mn] = tv;
      for ( usize k = 0; k < N; ++k ) {
        F tt = r.vectors.at(k, i);
        r.vectors.at(k, i) = r.vectors.at(k, mn);
        r.vectors.at(k, mn) = tt;
      }
    }
  }
  return r;
}

};     // namespace decomp
};     // namespace linalg
};     // namespace math
};     // namespace micron
