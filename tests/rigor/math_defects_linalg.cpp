// math_defects_linalg.cpp
// Wave 3 group w3a: decompositions and matrix functions.
//
// Every section asserts a property that the pre-patch tree FAILS:
//  (a) one-sided Jacobi SVD stopped on an ABSOLUTE off-diagonal test, so a small- or large-magnitude
//      matrix came back with wrong singular values and a non-orthogonal U while reporting converged
//  (b) pinv inherited (a) through pseudoinv::svd
//  (c) eigen_sym3's hardcoded F(1e-12) and eigen_sym's absolute default_eps did the same
//  (d) pseudoinv::null read its basis out of the padded (zero) columns of svd(A^T).U for rows < cols
//  (e) svd_bdc dropped B(R-1, R) for rows < cols
//  (f) schur::{sin,cos,sinh,cosh}mat returned the zero matrix for any repeated eigenvalue
//
// Section (g) is the -Ofast control: the scale-free stopping tests are written with the tqli
// `|x| + dd == dd` idiom, so a literal operand and a volatile-laundered one must agree.

#include "../../src/math/linalg/decomp.hpp"
#include "../../src/math/linalg/decomp_ext.hpp"
#include "../../src/math/linalg/matfunc_schur.hpp"
#include "../../src/math/linalg/pseudoinv.hpp"
#include "../../src/math/linalg/svd_bdc.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace ml = micron::math::linalg;

using F = double;

static F
absv(F x) noexcept
{
  return x < F(0) ? -x : x;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fixed-size helpers

template<usize R, usize C>
static F
max_utu_dev(const ml::mat<F, R, R> &U) noexcept
{
  F mx = F(0);
  for ( usize i = 0; i < C; ++i )
    for ( usize j = 0; j < C; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < R; ++k ) s += U.data[k * R + i] * U.data[k * R + j];
      F d = absv(s - (i == j ? F(1) : F(0)));
      if ( d > mx ) mx = d;
    }
  return mx;
}

template<usize R, usize C>
static F
max_recon_dev(const ml::mat<F, R, C> &A, const ml::decomp::svd_result<F, R, C> &r) noexcept
{
  F mx = F(0);
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < C; ++k ) s += r.U.data[i * R + k] * r.S.data[k] * r.V.data[j * C + k];
      F d = absv(s - A.data[i * C + j]);
      if ( d > mx ) mx = d;
    }
  return mx;
}

template<usize N>
static F
max_eig_resid(const ml::mat<F, N, N> &A, const ml::vec<F, N> &lam, const ml::mat<F, N, N> &V) noexcept
{
  F mx = F(0);
  for ( usize j = 0; j < N; ++j )
    for ( usize i = 0; i < N; ++i ) {
      F s = F(0);
      for ( usize k = 0; k < N; ++k ) s += A.data[i * N + k] * V.data[k * N + j];
      F d = absv(s - lam.data[j] * V.data[i * N + j]);
      if ( d > mx ) mx = d;
    }
  return mx;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int
main()
{
  // ------------------------------------------------------------------
  test_case("(a) decomp::svd is scale-invariant and returns an orthogonal U");
  {
    static constexpr F base[16] = { 4, -2, 7, 1, 3, 9, -5, 2, -6, 1, 8, 4, 2, 5, 3, -7 };
    // reference at unit scale
    ml::mat<F, 4, 4> A1{};
    for ( usize i = 0; i < 16; ++i ) A1.data[i] = base[i];
    auto r1 = ml::decomp::svd<F, 4, 4>(A1);
    require_true(r1.converged);
    require_true(max_utu_dev<4, 4>(r1.U) < F(1e-13));

    static constexpr F scales[4] = { 1e-8, 1e-16, 1e-100, 1e150 };
    for ( usize s = 0; s < 4; ++s ) {
      const F sc = scales[s];
      ml::mat<F, 4, 4> A{};
      for ( usize i = 0; i < 16; ++i ) A.data[i] = base[i] * sc;
      auto r = ml::decomp::svd<F, 4, 4>(A);
      require_true(r.converged);
      // U must stay orthogonal at every scale -- 1.7e-1 at 1e-8 and 4.2e-1 at 1e150 before the fix
      require_true(max_utu_dev<4, 4>(r.U) < F(1e-13));
      // the singular values must scale exactly
      for ( usize i = 0; i < 4; ++i ) require_true(absv(r.S.data[i] / sc - r1.S.data[i]) <= F(1e-12) * r1.S.data[0]);
      require_true(max_recon_dev<4, 4>(A, r) <= F(1e-12) * sc * r1.S.data[0]);
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(a) decomp::svd 5x4 tall, small scale");
  {
    static constexpr F base[20] = { 1, 2, 3, 4, 5, 1, 2, 0, 2, 4, 1, 3, 7, -1, 2, 5, 0, 1, 3, -2 };
    ml::mat<F, 5, 4> A1{};
    for ( usize i = 0; i < 20; ++i ) A1.data[i] = base[i];
    auto r1 = ml::decomp::svd<F, 5, 4>(A1);
    require_true(r1.converged);

    ml::mat<F, 5, 4> A{};
    for ( usize i = 0; i < 20; ++i ) A.data[i] = base[i] * F(1e-7);
    auto r = ml::decomp::svd<F, 5, 4>(A);
    require_true(r.converged);
    // 4.0e-4 before the fix
    require_true(max_utu_dev<5, 4>(r.U) < F(1e-13));
    for ( usize i = 0; i < 4; ++i ) require_true(absv(r.S.data[i] / F(1e-7) - r1.S.data[i]) <= F(1e-12) * r1.S.data[0]);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(b) pseudoinv::pinv satisfies A P A == A at 1e-7 scale");
  {
    static constexpr F base[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 10 };
    static constexpr F scales[2] = { 1.0, 1e-7 };
    for ( usize s = 0; s < 2; ++s ) {
      const F sc = scales[s];
      m::dynmat<F> A(3, 3);
      F na = F(0);
      for ( usize i = 0; i < 3; ++i )
        for ( usize j = 0; j < 3; ++j ) {
          A.at(i, j) = base[i * 3 + j] * sc;
          if ( absv(A.at(i, j)) > na ) na = absv(A.at(i, j));
        }
      auto P = ml::pseudoinv::pinv<F>(A);
      F mx = F(0);
      for ( usize i = 0; i < 3; ++i )
        for ( usize j = 0; j < 3; ++j ) {
          F acc = F(0);
          for ( usize k = 0; k < 3; ++k ) {
            F t = F(0);
            for ( usize l = 0; l < 3; ++l ) t += P.at(k, l) * A.at(l, j);
            acc += A.at(i, k) * t;
          }
          F d = absv(acc - A.at(i, j));
          if ( d > mx ) mx = d;
        }
      // 8.6e-5 relative at 1e-7 before the fix
      require_true(mx <= F(1e-12) * na);
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(c) decomp::eigen_sym3 is scale-invariant");
  {
    static constexpr F base[9] = { 2, 1, 0, 1, 2, 1, 0, 1, 2 };
    // exact spectrum of [[2,1,0],[1,2,1],[0,1,2]]
    const F root2 = m::fsqrt(F(2));
    const F want[3] = { F(2) - root2, F(2), F(2) + root2 };
    static constexpr F scales[4] = { 1.0, 1e-12, 1e-13, 1e-200 };
    for ( usize s = 0; s < 4; ++s ) {
      const F sc = scales[s];
      ml::mat<F, 3, 3> A{};
      for ( usize i = 0; i < 9; ++i ) A.data[i] = base[i] * sc;
      auto e = ml::decomp::eigen_sym3<F>(A);
      // returned (1,2,3)*sc at 1e-12 and (2,2,2)*sc at 1e-13 before the fix
      for ( usize i = 0; i < 3; ++i ) require_true(absv(e.values.data[i] / sc - want[i]) <= F(1e-12) * want[2]);
      require_true(max_eig_resid<3>(A, e.values, e.vectors) <= F(1e-12) * sc * want[2]);
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(c) decomp::eigen_sym agrees with eigen_sym_qr at 1e-13 scale");
  {
    ml::mat<F, 2, 2> A{};
    A.data[0] = F(3e-13);
    A.data[1] = F(1e-13);
    A.data[2] = F(1e-13);
    A.data[3] = F(3e-13);
    auto e = ml::decomp::eigen_sym<F, 2>(A);
    require_true(e.converged);
    // returned (3e-13, 3e-13) with V == I and converged == true before the fix
    F lo = e.values.data[0] < e.values.data[1] ? e.values.data[0] : e.values.data[1];
    F hi = e.values.data[0] < e.values.data[1] ? e.values.data[1] : e.values.data[0];
    require_true(absv(lo - F(2e-13)) <= F(1e-25));
    require_true(absv(hi - F(4e-13)) <= F(1e-25));
    require_true(max_eig_resid<2>(A, e.values, e.vectors) <= F(1e-25));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(c) decomp::eigen_sym 5x5 residual is scale-free");
  {
    // fixed hex seed, xorshift64
    u64 st = 0x9E3779B97F4A7C15ull;
    auto rnd = [&]() {
      st ^= st << 13;
      st ^= st >> 7;
      st ^= st << 17;
      return F(st >> 11) / F(9007199254740992.0) * F(2) - F(1);
    };
    F M[25];
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = i; j < 5; ++j ) {
        F v = rnd();
        M[i * 5 + j] = v;
        M[j * 5 + i] = v;
      }
    static constexpr F scales[5] = { 1.0, 1e-4, 1e-8, 1e-90, 1e90 };
    for ( usize s = 0; s < 5; ++s ) {
      const F sc = scales[s];
      ml::mat<F, 5, 5> A{};
      F na = F(0);
      for ( usize i = 0; i < 25; ++i ) {
        A.data[i] = M[i] * sc;
        if ( absv(A.data[i]) > na ) na = absv(A.data[i]);
      }
      auto e = ml::decomp::eigen_sym<F, 5>(A);
      require_true(e.converged);
      // 8.2e-5 relative at 1e-8 and 1.0 at 1e-90 before the fix
      require_true(max_eig_resid<5>(A, e.values, e.vectors) <= F(1e-13) * na);
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(d) pseudoinv::null returns a real basis when rows < cols");
  {
    // 1x2 [1 0] -> (0, 1)
    m::dynmat<F> A(1, 2);
    A.at(0, 0) = F(1);
    A.at(0, 1) = F(0);
    m::dynmat<F> N = ml::pseudoinv::null<F>(A);
    require_true(N.rows == 2);
    require_true(N.cols == 1);
    F nn = N.at(0, 0) * N.at(0, 0) + N.at(1, 0) * N.at(1, 0);
    // the whole column was zero before the fix
    require_true(absv(nn - F(1)) <= F(1e-12));
    require_true(absv(A.at(0, 0) * N.at(0, 0) + A.at(0, 1) * N.at(1, 0)) <= F(1e-12));

    // 2x3 rank 1: a two-dimensional null space, both columns must be unit and in ker(A)
    m::dynmat<F> B(2, 3);
    B.at(0, 0) = F(1);
    B.at(0, 1) = F(2);
    B.at(0, 2) = F(3);
    B.at(1, 0) = F(2);
    B.at(1, 1) = F(4);
    B.at(1, 2) = F(6);
    m::dynmat<F> NB = ml::pseudoinv::null<F>(B);
    require_true(NB.rows == 3);
    require_true(NB.cols == 2);
    for ( usize j = 0; j < NB.cols; ++j ) {
      F n2 = F(0);
      for ( usize i = 0; i < 3; ++i ) n2 += NB.at(i, j) * NB.at(i, j);
      require_true(absv(n2 - F(1)) <= F(1e-12));
      for ( usize i = 0; i < 2; ++i ) {
        F s = F(0);
        for ( usize k = 0; k < 3; ++k ) s += B.at(i, k) * NB.at(k, j);
        require_true(absv(s) <= F(1e-12));
      }
    }
    // and orthogonal to each other
    F dot = F(0);
    for ( usize i = 0; i < 3; ++i ) dot += NB.at(i, 0) * NB.at(i, 1);
    require_true(absv(dot) <= F(1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(e) decomp::svd_bdc reconstructs A when rows < cols");
  {
    static constexpr usize rr[2] = { 2, 3 };
    static constexpr usize cc[2] = { 3, 4 };
    static constexpr F d0[6] = { 1, 2, 3, 4, 5, 6 };
    static constexpr F d1[12] = { 0.8, -0.3, 1.7, 0.2, 0.5, 2.1, -0.9, 1.1, -1.4, 0.6, 0.3, 0.7 };
    for ( usize t = 0; t < 2; ++t ) {
      const usize R = rr[t];
      const usize C = cc[t];
      const F *src = (t == 0) ? d0 : d1;
      m::dynmat<F> A(R, C);
      F na = F(0);
      for ( usize i = 0; i < R; ++i )
        for ( usize j = 0; j < C; ++j ) {
          A.at(i, j) = src[i * C + j];
          if ( absv(A.at(i, j)) > na ) na = absv(A.at(i, j));
        }
      auto r = ml::decomp::svd_bdc<F>(A);
      require_true(r.converged);
      const usize K = (R < C) ? R : C;
      F mx = F(0);
      for ( usize i = 0; i < R; ++i )
        for ( usize j = 0; j < C; ++j ) {
          F s = F(0);
          for ( usize k = 0; k < K; ++k ) s += r.U.at(i, k) * r.S[k] * r.V.at(j, k);
          F d = absv(s - A.at(i, j));
          if ( d > mx ) mx = d;
        }
      // 2.7e-1 (2x3) and 8.1e-1 (3x4) before the fix
      require_true(mx <= F(1e-12) * na);
      // and it must now agree with the D&C entry point, which always had the rows<cols guard
      auto r2 = ml::decomp::svd_bdc_dnc<F>(A);
      for ( usize k = 0; k < K; ++k ) require_true(absv(r.S[k] - r2.S[k]) <= F(1e-11) * na);
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(f) schur trig matrix functions handle a repeated eigenvalue");
  {
    // diag(2,2): every one of the four returned the 2x2 zero matrix with converged == false
    ml::mat<F, 2, 2> D{};
    D.data[0] = F(2);
    D.data[3] = F(2);
    auto s2 = ml::schur::sinmat<F, 2>(D);
    auto c2 = ml::schur::cosmat<F, 2>(D);
    auto sh2 = ml::schur::sinhmat<F, 2>(D);
    auto ch2 = ml::schur::coshmat<F, 2>(D);
    require_true(s2.converged && c2.converged && sh2.converged && ch2.converged);
    const F s_ref = m::sin<F>(F(2));
    const F c_ref = m::cos<F>(F(2));
    const F sh_ref = m::sinh<F>(F(2));
    const F ch_ref = m::cosh<F>(F(2));
    for ( usize k = 0; k < 2; ++k ) {
      const usize dg = k * 2 + k;
      const usize of = k * 2 + (1 - k);
      require_true(absv(s2.X.data[dg] - s_ref) <= F(1e-13));
      require_true(absv(c2.X.data[dg] - c_ref) <= F(1e-13));
      require_true(absv(sh2.X.data[dg] - sh_ref) <= F(1e-13));
      require_true(absv(ch2.X.data[dg] - ch_ref) <= F(1e-13));
      require_true(absv(s2.X.data[of]) <= F(1e-13));
      require_true(absv(c2.X.data[of]) <= F(1e-13));
    }

    // a rotated symmetric 3x3 with spectrum {1,1,4}: Q = I - 2 v v^T, |v| = 1
    static constexpr F v[3] = { 0.6, 0.48, 0.64 };
    F Q[9];
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) Q[i * 3 + j] = (i == j ? F(1) : F(0)) - F(2) * v[i] * v[j];
    static constexpr F lam[3] = { 1, 1, 4 };
    ml::mat<F, 3, 3> B{};
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        F acc = F(0);
        for ( usize k = 0; k < 3; ++k ) acc += Q[i * 3 + k] * lam[k] * Q[j * 3 + k];
        B.data[i * 3 + j] = acc;
      }
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = i + 1; j < 3; ++j ) B.data[j * 3 + i] = B.data[i * 3 + j];

    auto sb3 = ml::schur::sinmat<F, 3>(B);
    auto cb3 = ml::schur::cosmat<F, 3>(B);
    require_true(sb3.converged && cb3.converged);
    F ms = F(0), mc = F(0);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        F a = F(0), c = F(0);
        for ( usize k = 0; k < 3; ++k ) {
          a += Q[i * 3 + k] * m::sin<F>(lam[k]) * Q[j * 3 + k];
          c += Q[i * 3 + k] * m::cos<F>(lam[k]) * Q[j * 3 + k];
        }
        F d1v = absv(sb3.X.data[i * 3 + j] - a);
        F d2v = absv(cb3.X.data[i * 3 + j] - c);
        if ( d1v > ms ) ms = d1v;
        if ( d2v > mc ) mc = d2v;
      }
    // 7.9e-1 / 5.6e-1 before the fix (the zero matrix)
    require_true(ms <= F(1e-13));
    require_true(mc <= F(1e-13));

    // sin^2 + cos^2 == I on the degenerate spectrum
    F mi = F(0);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        F acc = F(0);
        for ( usize k = 0; k < 3; ++k )
          acc += sb3.X.data[i * 3 + k] * sb3.X.data[k * 3 + j] + cb3.X.data[i * 3 + k] * cb3.X.data[k * 3 + j];
        F d = absv(acc - (i == j ? F(1) : F(0)));
        if ( d > mi ) mi = d;
      }
    require_true(mi <= F(1e-13));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(f) the non-symmetric Parlett breakdown still reports failure, it does not fake a zero");
  {
    // T = [[2,1],[0,2]] has rhs == 0 with a singular Sylvester denominator, and the CORRECT
    // off-diagonal is cos(2), not 0 -- so taking x = 0 there would be a wrong answer, not a fix
    ml::mat<F, 2, 2> J{};
    J.data[0] = F(2);
    J.data[1] = F(1);
    J.data[2] = F(0);
    J.data[3] = F(2);
    auto sj = ml::schur::sinmat<F, 2>(J);
    if ( sj.converged ) require_true(absv(sj.X.data[1] - m::cos<F>(F(2))) <= F(1e-9));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("(g) -Ofast control: literal and volatile-laundered operands agree");
  {
    // the scale-free stopping tests use `|x| + dd == dd`; if -ffast-math folded that to `x == 0`
    // the two paths below would diverge
    ml::mat<F, 2, 2> lit{};
    lit.data[0] = F(3e-13);
    lit.data[1] = F(1e-13);
    lit.data[2] = F(1e-13);
    lit.data[3] = F(3e-13);
    auto a = ml::decomp::eigen_sym<F, 2>(lit);

    volatile F v0 = F(3e-13);
    volatile F v1 = F(1e-13);
    ml::mat<F, 2, 2> vol{};
    vol.data[0] = v0;
    vol.data[1] = v1;
    vol.data[2] = v1;
    vol.data[3] = v0;
    auto b = ml::decomp::eigen_sym<F, 2>(vol);

    require_true(a.converged == b.converged);
    for ( usize i = 0; i < 2; ++i ) require_true(a.values.data[i] == b.values.data[i]);
    for ( usize i = 0; i < 4; ++i ) require_true(a.vectors.data[i] == b.vectors.data[i]);

    static constexpr F base3[9] = { 2, 1, 0, 1, 2, 1, 0, 1, 2 };
    ml::mat<F, 3, 3> l3{};
    for ( usize i = 0; i < 9; ++i ) l3.data[i] = base3[i] * F(1e-13);
    auto e3l = ml::decomp::eigen_sym3<F>(l3);
    volatile F sc = F(1e-13);
    ml::mat<F, 3, 3> v3{};
    for ( usize i = 0; i < 9; ++i ) v3.data[i] = base3[i] * sc;
    auto e3v = ml::decomp::eigen_sym3<F>(v3);
    for ( usize i = 0; i < 3; ++i ) require_true(e3l.values.data[i] == e3v.values.data[i]);
  }
  end_test_case();

  sb::print("=== MATH_DEFECTS_LINALG PASSED ===");
  return 1;
}
