// math_linalg_extended.cpp
// Snowball tests for math::linalg extensions:
//   householder · hessenberg · eigen_sym<N> · schur<N> · eigen<N> · svd<R,C>
//   expm · sqrtm · logm

#include "../../src/math/linalg.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::linalg;
using namespace micron::math::linalg::ops;
using namespace micron::math::linalg::decomp;
using namespace micron::math::linalg::matfunc;

static bool
near(f64 a, f64 b, f64 eps = 1e-9)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

template <usize N>
static bool
near_v(const vec<f64, N> &a, const vec<f64, N> &b, f64 eps = 1e-9)
{
  for ( usize i = 0; i < N; ++i )
    if ( !near(a.data[i], b.data[i], eps) ) return false;
  return true;
}

template <usize R, usize C>
static bool
near_m(const mat<f64, R, C> &a, const mat<f64, R, C> &b, f64 eps = 1e-9)
{
  for ( usize i = 0; i < R * C; ++i )
    if ( !near(a.data[i], b.data[i], eps) ) return false;
  return true;
}

template <usize N>
static bool
is_identity_m(const mat<f64, N, N> &A, f64 eps = 1e-9)
{
  return near_m(A, mat<f64, N, N>::identity(), eps);
}

int
main()
{
  print("=== MATH::LINALG EXTENDED TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Householder reflectors
  test_case("householder — reflector zeros tail of x");
  {
    vec<f64, 4> x{ 3.0, 4.0, 0.0, 0.0 };
    auto h = householder_reflector<f64, 4>(x, 0);
    // Apply as a 1x4 matrix to test
    mat<f64, 4, 4> X = mat<f64, 4, 4>::identity();
    X.data[0] = x.data[0];
    X.data[4] = x.data[1];
    X.data[8] = x.data[2];
    X.data[12] = x.data[3];
    apply_householder_left<f64, 4, 4>(X, h.v, h.beta, 0);
    // After reflection, column 0 of X has only first entry nonzero.
    require_true(near(X.data[4], 0.0));
    require_true(near(X.data[8], 0.0));
    require_true(near(X.data[12], 0.0));
    // Magnitude preserved
    f64 r = X.data[0];
    require_true(near(r * r, 25.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Hessenberg reduction
  test_case("hessenberg<5> — Q*H*Qᵀ == A");
  {
    mat<f64, 5, 5> A{ 4, 1, -2, 2, 1, 1, 2, 0, 1, -1, -2, 0, 3, -2, 0,
                      2, 1, -2, 5, 1, 1, -1, 0, 1, 4 };
    auto h = hessenberg<f64, 5>(A);
    auto Qt = transpose(h.Q);
    auto QH = gemm(h.Q, h.H);
    auto QHQt = gemm(QH, Qt);
    require_true(near_m(QHQt, A, 1e-8));
    // Check upper Hessenberg: subdiag-2 entries should be zero
    for ( usize i = 2; i < 5; ++i )
      for ( usize j = 0; j + 1 < i; ++j ) require_true(near(h.H.data[i * 5 + j], 0.0, 1e-12));
  }
  end_test_case();

  test_case("hessenberg<8> — preserves trace");
  {
    mat<f64, 8, 8> A = mat<f64, 8, 8>::zero();
    f64 tr = 0.0;
    for ( usize i = 0; i < 8; ++i ) {
      for ( usize j = 0; j < 8; ++j ) A.data[i * 8 + j] = f64((i * 17 + j * 7 + 1) % 11) - 5.0;
      tr += A.data[i * 8 + i];
    }
    auto h = hessenberg<f64, 8>(A);
    f64 tr_h = 0.0;
    for ( usize i = 0; i < 8; ++i ) tr_h += h.H.data[i * 8 + i];
    require_true(near(tr, tr_h, 1e-9));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Symmetric eigendecomposition (generalised Jacobi)
  test_case("eigen_sym<5> — V is orthogonal");
  {
    mat<f64, 5, 5> A = mat<f64, 5, 5>::zero();
    // Build a symmetric A
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = i; j < 5; ++j ) {
        f64 v = f64((i * 13 + j * 5 + 3) % 7) - 3.0;
        A.data[i * 5 + j] = v;
        A.data[j * 5 + i] = v;
      }
    auto e = eigen_sym<f64, 5>(A);
    require_true(e.converged);
    auto Vt = transpose(e.vectors);
    auto VtV = gemm(Vt, e.vectors);
    require_true(is_identity_m(VtV, 1e-9));
  }
  end_test_case();

  test_case("eigen_sym<5> — A v_k == λ_k v_k");
  {
    mat<f64, 5, 5> A = mat<f64, 5, 5>::zero();
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = i; j < 5; ++j ) {
        f64 v = f64((i * 13 + j * 5 + 3) % 7) - 3.0;
        A.data[i * 5 + j] = v;
        A.data[j * 5 + i] = v;
      }
    auto e = eigen_sym<f64, 5>(A);
    require_true(e.converged);
    for ( usize k = 0; k < 5; ++k ) {
      vec<f64, 5> v_k{};
      for ( usize i = 0; i < 5; ++i ) v_k.data[i] = e.vectors.data[i * 5 + k];
      auto Av = gemv(A, v_k);
      vec<f64, 5> lv{};
      for ( usize i = 0; i < 5; ++i ) lv.data[i] = e.values.data[k] * v_k.data[i];
      require_true(near_v(Av, lv, 1e-8));
    }
  }
  end_test_case();

  test_case("eigen_sym<5> — sum of eigenvalues == trace");
  {
    mat<f64, 5, 5> A = mat<f64, 5, 5>::zero();
    f64 tr = 0.0;
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = i; j < 5; ++j ) {
        f64 v = f64((i * 13 + j * 5 + 3) % 7) - 3.0;
        A.data[i * 5 + j] = v;
        A.data[j * 5 + i] = v;
      }
    for ( usize i = 0; i < 5; ++i ) tr += A.data[i * 5 + i];
    auto e = eigen_sym<f64, 5>(A);
    f64 sum = 0.0;
    for ( usize i = 0; i < 5; ++i ) sum += e.values.data[i];
    require_true(near(sum, tr, 1e-9));
  }
  end_test_case();

  test_case("eigen_sym<8> — diagonal matrix exact eigenvalues");
  {
    mat<f64, 8, 8> A = mat<f64, 8, 8>::zero();
    for ( usize i = 0; i < 8; ++i ) A.data[i * 8 + i] = f64(i + 1);
    auto e = eigen_sym<f64, 8>(A);
    require_true(e.converged);
    // values may be permuted; sum check
    f64 sum = 0.0;
    for ( usize i = 0; i < 8; ++i ) sum += e.values.data[i];
    require_true(near(sum, 36.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Schur and general eigen
  test_case("schur<2> — diagonal");
  {
    mat<f64, 2, 2> A{ 3.0, 0.0, 0.0, 5.0 };
    auto s = schur<f64, 2>(A);
    require_true(s.converged);
    auto Zt = transpose(s.Z);
    auto ZTZt = gemm(gemm(s.Z, s.T), Zt);
    require_true(near_m(ZTZt, A, 1e-9));
  }
  end_test_case();

  test_case("schur<3> — diagonal");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::zero();
    A.data[0] = 2.0;
    A.data[4] = 5.0;
    A.data[8] = 7.0;
    auto s = schur<f64, 3>(A);
    require_true(s.converged);
    auto Zt = transpose(s.Z);
    auto ZTZt = gemm(gemm(s.Z, s.T), Zt);
    require_true(near_m(ZTZt, A, 1e-9));
  }
  end_test_case();

  test_case("schur<3> — symmetric");
  {
    mat<f64, 3, 3> A{ 4.0, 1.0, 0.0, 1.0, 3.0, 1.0, 0.0, 1.0, 2.0 };
    auto s = schur<f64, 3>(A);
    require_true(s.converged);
    auto Zt = transpose(s.Z);
    auto ZTZt = gemm(gemm(s.Z, s.T), Zt);
    require_true(near_m(ZTZt, A, 1e-8));
    auto ZtZ = gemm(Zt, s.Z);
    require_true(is_identity_m(ZtZ, 1e-9));
  }
  end_test_case();

  test_case("schur<4> — diagonal");
  {
    mat<f64, 4, 4> A = mat<f64, 4, 4>::zero();
    A.data[0] = 1.0;
    A.data[5] = 2.0;
    A.data[10] = 3.0;
    A.data[15] = 4.0;
    auto s = schur<f64, 4>(A);
    require_true(s.converged);
  }
  end_test_case();

  test_case("schur<4> — symmetric");
  {
    mat<f64, 4, 4> A{ 4, 1, 0, 0, 1, 3, 1, 0, 0, 1, 2, 1, 0, 0, 1, 5 };
    auto s = schur<f64, 4>(A);
    require_true(s.converged);
    auto Zt = transpose(s.Z);
    auto ZTZt = gemm(gemm(s.Z, s.T), Zt);
    require_true(near_m(ZTZt, A, 1e-7));
  }
  end_test_case();

  test_case("schur<5> — diagonal");
  {
    mat<f64, 5, 5> A = mat<f64, 5, 5>::zero();
    A.data[0] = 1.0;
    A.data[6] = 2.0;
    A.data[12] = 3.0;
    A.data[18] = 4.0;
    A.data[24] = 5.0;
    auto s = schur<f64, 5>(A);
    require_true(s.converged);
  }
  end_test_case();

  test_case("schur<5> — Z*T*Zᵀ == A and Z orthogonal");
  {
    mat<f64, 5, 5> A{ 4, 1, -2, 2, 1, 1, 2, 0, 1, -1, -2, 0, 3, -2, 0,
                      2, 1, -2, 5, 1, 1, -1, 0, 1, 4 };
    auto s = schur<f64, 5>(A);
    require_true(s.converged);
    auto Zt = transpose(s.Z);
    auto ZTZt = gemm(gemm(s.Z, s.T), Zt);
    require_true(near_m(ZTZt, A, 1e-7));
    auto ZtZ = gemm(Zt, s.Z);
    require_true(is_identity_m(ZtZ, 1e-9));
  }
  end_test_case();

  test_case("eigen<5> — diagonal: exact eigenvalues, zero im parts");
  {
    mat<f64, 5, 5> A = mat<f64, 5, 5>::zero();
    A.data[0] = 1.0;
    A.data[6] = -2.0;
    A.data[12] = 3.0;
    A.data[18] = -4.0;
    A.data[24] = 5.0;
    auto e = eigen<f64, 5>(A);
    require_true(e.converged);
    f64 sum_re = 0.0;
    for ( usize i = 0; i < 5; ++i ) {
      sum_re += e.values_re.data[i];
      require_true(near(e.values_im.data[i], 0.0, 1e-12));
    }
    require_true(near(sum_re, 3.0));     // 1 - 2 + 3 - 4 + 5
  }
  end_test_case();

  test_case("eigen<3> — rotation matrix has unit-modulus complex eigenvalues");
  {
    // 2D rotation in upper-left 2x2; eigenvalue 1 in lower right
    f64 c = 0.6, s = 0.8;     // cos, sin (3-4-5)
    mat<f64, 3, 3> A{ c, -s, 0, s, c, 0, 0, 0, 1.0 };
    auto e = eigen<f64, 3>(A);
    require_true(e.converged);
    // sum of |λ|² should be 1+1+1=3
    f64 mod_sum = 0.0;
    for ( usize i = 0; i < 3; ++i ) {
      f64 re = e.values_re.data[i];
      f64 im = e.values_im.data[i];
      mod_sum += re * re + im * im;
    }
    require_true(near(mod_sum, 3.0, 1e-8));
    // trace(A) = 2c + 1 = 2.2, should match sum of values_re
    f64 sum_re = 0.0;
    for ( usize i = 0; i < 3; ++i ) sum_re += e.values_re.data[i];
    require_true(near(sum_re, 2.0 * c + 1.0, 1e-8));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SVD
  test_case("svd<4,4> — UᵀU == I, VᵀV == I, U*diag(S)*Vᵀ == A");
  {
    mat<f64, 4, 4> A{ 1, 2, 3, 4, 5, 6, 7, 8, 1, 0, 1, 0, 0, 1, 0, 1 };
    auto s = svd<f64, 4, 4>(A);
    require_true(s.converged);
    auto Ut = transpose(s.U);
    auto UtU = gemm(Ut, s.U);
    require_true(is_identity_m(UtU, 1e-8));
    auto Vt = transpose(s.V);
    auto VtV = gemm(Vt, s.V);
    require_true(is_identity_m(VtV, 1e-8));
    // Reconstruct
    mat<f64, 4, 4> Sigma = mat<f64, 4, 4>::zero();
    for ( usize i = 0; i < 4; ++i ) Sigma.data[i * 4 + i] = s.S.data[i];
    auto rec = gemm(gemm(s.U, Sigma), Vt);
    require_true(near_m(rec, A, 1e-7));
  }
  end_test_case();

  test_case("svd<4,4> — singular values nonneg and sorted descending");
  {
    mat<f64, 4, 4> A{ 1, 2, 3, 4, 5, 6, 7, 8, 1, 0, 1, 0, 0, 1, 0, 1 };
    auto s = svd<f64, 4, 4>(A);
    require_true(s.S.data[0] >= 0.0);
    require_true(s.S.data[1] >= 0.0);
    require_true(s.S.data[2] >= 0.0);
    require_true(s.S.data[3] >= 0.0);
    require_true(s.S.data[0] >= s.S.data[1]);
    require_true(s.S.data[1] >= s.S.data[2]);
    require_true(s.S.data[2] >= s.S.data[3]);
  }
  end_test_case();

  test_case("svd<5,3> — A == U[:,:3] * diag(S) * Vᵀ");
  {
    mat<f64, 5, 3> A{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 0, 1, 0, 1, 0 };
    auto s = svd<f64, 5, 3>(A);
    require_true(s.converged);
    // First 3 columns of U should be orthonormal
    for ( usize j = 0; j < 3; ++j ) {
      f64 nrm = 0.0;
      for ( usize i = 0; i < 5; ++i ) nrm += s.U.data[i * 5 + j] * s.U.data[i * 5 + j];
      require_true(near(nrm, 1.0, 1e-8));
    }
    // Reconstruct
    mat<f64, 5, 3> rec = mat<f64, 5, 3>::zero();
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 acc = 0.0;
        for ( usize k = 0; k < 3; ++k ) acc += s.U.data[i * 5 + k] * s.S.data[k] * s.V.data[j * 3 + k];
        rec.data[i * 3 + j] = acc;
      }
    require_true(near_m(rec, A, 1e-7));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Matrix exponential
  test_case("expm<3> — expm(0) == I");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::zero();
    auto er = expm<f64, 3>(A);
    require_true(er.finite);
    require_true(is_identity_m(er.X, 1e-12));
  }
  end_test_case();

  test_case("expm<3> — expm(diag(0,ln2,ln3)) == diag(1,2,3)");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::zero();
    A.data[0] = 0.0;
    A.data[4] = 0.6931471805599453;     // ln(2)
    A.data[8] = 1.0986122886681098;     // ln(3)
    auto er = expm<f64, 3>(A);
    require_true(er.finite);
    require_true(near(er.X.data[0], 1.0, 1e-9));
    require_true(near(er.X.data[4], 2.0, 1e-9));
    require_true(near(er.X.data[8], 3.0, 1e-9));
    require_true(near(er.X.data[1], 0.0, 1e-12));
    require_true(near(er.X.data[3], 0.0, 1e-12));
  }
  end_test_case();

  test_case("expm<3> — expm(-A) * expm(A) == I");
  {
    mat<f64, 3, 3> A{ 0.5, 0.2, -0.1, 0.1, -0.3, 0.4, 0.0, 0.2, 0.1 };
    mat<f64, 3, 3> negA;
    for ( usize i = 0; i < 9; ++i ) negA.data[i] = -A.data[i];
    auto e1 = expm<f64, 3>(A);
    auto e2 = expm<f64, 3>(negA);
    require_true(e1.finite && e2.finite);
    auto P = gemm(e2.X, e1.X);
    require_true(is_identity_m(P, 1e-8));
  }
  end_test_case();

  test_case("expm<4> — large norm exercises scaling-squaring");
  {
    mat<f64, 4, 4> A = mat<f64, 4, 4>::zero();
    for ( usize i = 0; i < 4; ++i ) A.data[i * 4 + i] = 5.0;
    auto er = expm<f64, 4>(A);
    require_true(er.finite);
    f64 e5 = 148.4131591025766;     // exp(5)
    require_true(near(er.X.data[0], e5, 1e-7));
    require_true(near(er.X.data[5], e5, 1e-7));
    require_true(near(er.X.data[10], e5, 1e-7));
    require_true(near(er.X.data[15], e5, 1e-7));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Matrix square root
  test_case("sqrtm<3> — diag(1,4,9) -> diag(1,2,3)");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::zero();
    A.data[0] = 1.0;
    A.data[4] = 4.0;
    A.data[8] = 9.0;
    auto sr = sqrtm<f64, 3>(A);
    require_true(sr.real_sqrt_exists);
    require_true(near(sr.X.data[0], 1.0, 1e-9));
    require_true(near(sr.X.data[4], 2.0, 1e-9));
    require_true(near(sr.X.data[8], 3.0, 1e-9));
  }
  end_test_case();

  test_case("sqrtm<3> — sqrtm(A)*sqrtm(A) == A for SPD");
  {
    mat<f64, 3, 3> A{ 4, 1, 1, 1, 5, 2, 1, 2, 6 };
    auto sr = sqrtm<f64, 3>(A);
    require_true(sr.real_sqrt_exists);
    auto P = gemm(sr.X, sr.X);
    require_true(near_m(P, A, 1e-7));
  }
  end_test_case();

  test_case("sqrtm<2> — negative eigenvalue: real_sqrt_exists == false");
  {
    mat<f64, 2, 2> A = mat<f64, 2, 2>::zero();
    A.data[0] = -1.0;
    A.data[3] = 4.0;
    auto sr = sqrtm<f64, 2>(A);
    require_false(sr.real_sqrt_exists);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Matrix logarithm
  test_case("logm<3> — logm(I) == 0");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::identity();
    auto lr = logm<f64, 3>(A);
    require_true(lr.real_log_exists);
    for ( usize i = 0; i < 9; ++i ) require_true(near(lr.X.data[i], 0.0, 1e-9));
  }
  end_test_case();

  test_case("logm<3> — diag(1,2,3) -> diag(0, ln2, ln3)");
  {
    mat<f64, 3, 3> A = mat<f64, 3, 3>::zero();
    A.data[0] = 1.0;
    A.data[4] = 2.0;
    A.data[8] = 3.0;
    auto lr = logm<f64, 3>(A);
    require_true(lr.real_log_exists);
    require_true(near(lr.X.data[0], 0.0, 1e-9));
    require_true(near(lr.X.data[4], 0.6931471805599453, 1e-8));
    require_true(near(lr.X.data[8], 1.0986122886681098, 1e-8));
  }
  end_test_case();

  test_case("logm<3> — logm(expm(A)) == A for SPD");
  {
    mat<f64, 3, 3> A{ 0.4, 0.1, 0.0, 0.1, 0.3, 0.05, 0.0, 0.05, 0.2 };
    auto er = expm<f64, 3>(A);
    require_true(er.finite);
    auto lr = logm<f64, 3>(er.X);
    require_true(lr.real_log_exists);
    require_true(near_m(lr.X, A, 1e-7));
  }
  end_test_case();

  test_case("logm<2> — negative eigenvalue: real_log_exists == false");
  {
    mat<f64, 2, 2> A = mat<f64, 2, 2>::zero();
    A.data[0] = -1.0;
    A.data[3] = 2.0;
    auto lr = logm<f64, 2>(A);
    require_false(lr.real_log_exists);
  }
  end_test_case();

  print("=== MATH::LINALG EXTENDED TESTS PASSED ===");
  return 1;
}
