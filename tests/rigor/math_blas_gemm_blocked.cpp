// math_blas_gemm_blocked.cpp — Snowball tests for the blocked GEMM
// path at matrix/pack.hpp::gemm_blocked, dispatched from
// blas::bits::gemm_kernel for sufficiently large inputs.
//
// Each case computes the same product two ways: through level3::gemm
// (which routes to gemm_blocked when dims are large enough) and a
// hand-rolled triple-loop reference.  The relative residual must
// stay under ~1e-10 (f64) / ~1e-4 (f32).

#include "../../src/math/blas/blas.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/std.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// utilities

template<typename T>
static T
fabs_(T x) noexcept
{
  return x < T(0) ? -x : x;
}

// Naive ijk gemm reference — pure scalar, no SIMD, no blocking.
// Computes C = α·op(A)·op(B) + β·C with op selected by trA / trB.
template<typename T>
static void
ref_gemm(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, usize lda, const T *B, usize ldb, T beta, T *C,
         usize ldc) noexcept
{
  for ( usize i = 0; i < m; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      T s{};
      for ( usize p = 0; p < k; ++p ) {
        const T a = trA ? A[p * lda + i] : A[i * lda + p];
        const T b = trB ? B[j * ldb + p] : B[p * ldb + j];
        s = s + a * b;
      }
      T &c = C[i * ldc + j];
      c = alpha * s + beta * c;
    }
  }
}

// Deterministic in [-0.5, 0.5) — distinct enough across (m, n, k, seed)
// that any indexing bug in the blocked path manifests as a residual.
template<typename T>
static void
fill_pattern(T *p, usize len, u64 seed) noexcept
{
  u64 x = seed | 1u;
  for ( usize i = 0; i < len; ++i ) {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    const u64 m = x & 0xFFFFFFu;
    p[i] = T(f64(m) / f64(0x1000000) - 0.5);
  }
}

template<typename T>
static T
max_rel_residual(const T *X, const T *Y, usize n) noexcept
{
  T num{};
  T den{};
  for ( usize i = 0; i < n; ++i ) {
    const T d = fabs_(X[i] - Y[i]);
    if ( d > num ) num = d;
    const T y = fabs_(Y[i]);
    if ( y > den ) den = y;
  }
  return den > T(0) ? num / den : num;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-case driver: build random A, B, C, run reference + blas, check.

template<typename T>
static void
run_case(usize m, usize n, usize k, bool trA, bool trB, T alpha, T beta, u64 seed, T tol) noexcept
{
  const usize a_rows = trA ? k : m;
  const usize a_cols = trA ? m : k;
  const usize b_rows = trB ? n : k;
  const usize b_cols = trB ? k : n;

  dynmat<T> A(a_rows, a_cols);
  dynmat<T> B(b_rows, b_cols);
  dynmat<T> C0(m, n);      // initial C used by both paths
  fill_pattern(A.data(), a_rows * a_cols, seed);
  fill_pattern(B.data(), b_rows * b_cols, seed ^ 0xDEADBEEFu);
  fill_pattern(C0.data(), m * n, seed ^ 0xCAFEBABEu);

  dynmat<T> C_ref = C0;
  dynmat<T> C_blas = C0;

  ref_gemm<T>(trA, trB, m, n, k, alpha, A.data(), a_cols, B.data(), b_cols, beta, C_ref.data(), n);

  auto Av = as_row_view(A);
  auto Bv = as_row_view(B);
  auto Cv = as_row_view(C_blas);
  if ( !trA && !trB )
    blas::level3::gemm(alpha, Av, Bv, beta, Cv);
  else if ( !trA && trB )
    blas::level3::gemm<blas::op::none, blas::op::trans>(alpha, Av, Bv, beta, Cv);
  else if ( trA && !trB )
    blas::level3::gemm<blas::op::trans, blas::op::none>(alpha, Av, Bv, beta, Cv);
  else
    blas::level3::gemm<blas::op::trans, blas::op::trans>(alpha, Av, Bv, beta, Cv);

  const T r = max_rel_residual<T>(C_blas.data(), C_ref.data(), m * n);
  require_true(r < tol);
}

int
main()
{
  print("=== BLAS L3 BLOCKED GEMM TESTS ===");

  test_case("1×1 (degenerate, scalar path)");
  {
    run_case<f64>(1, 1, 1, false, false, 1.0, 0.0, 0x1ull, 1e-10);
    run_case<f64>(1, 1, 1, false, false, -2.5, 1.0, 0x2ull, 1e-10);
  }
  end_test_case();

  test_case("7×11×13 (small, scalar path, non-divisible dims)");
  {
    run_case<f64>(7, 11, 13, false, false, 1.0, 0.0, 0x10ull, 1e-10);
    run_case<f64>(7, 11, 13, true, false, 1.0, 0.5, 0x11ull, 1e-10);
    run_case<f64>(7, 11, 13, false, true, 0.7, -1.3, 0x12ull, 1e-10);
    run_case<f64>(7, 11, 13, true, true, 1.0, 1.0, 0x13ull, 1e-10);
  }
  end_test_case();

  test_case("32×32×32 (right at the blocked-path threshold, no transpose)");
  {
    // m*n*k = 32768 < 64³ → still goes through scalar; exercises
    // the existing AVX2 4x4 fast path.
    run_case<f64>(32, 32, 32, false, false, 1.0, 0.0, 0x100ull, 1e-10);
    run_case<f64>(32, 32, 32, false, false, 1.5, 0.5, 0x101ull, 1e-10);
  }
  end_test_case();

  test_case("64×64×64 (blocked path engages, dims a multiple of MR/NR)");
  {
    run_case<f64>(64, 64, 64, false, false, 1.0, 0.0, 0x200ull, 1e-10);
    run_case<f64>(64, 64, 64, false, false, 0.0, 1.0, 0x201ull, 1e-10);      // alpha=0 → C·=β
    run_case<f64>(64, 64, 64, false, false, 1.0, 1.0, 0x202ull, 1e-10);
    run_case<f64>(64, 64, 64, true, false, 1.0, 0.0, 0x203ull, 1e-10);
    run_case<f64>(64, 64, 64, false, true, 1.0, 0.0, 0x204ull, 1e-10);
    run_case<f64>(64, 64, 64, true, true, 1.0, 0.0, 0x205ull, 1e-10);
    run_case<f64>(64, 64, 64, false, false, 2.5, -1.5, 0x206ull, 1e-10);
  }
  end_test_case();

  test_case("73×97×113 (blocked path, non-divisible dims → micro_kernel_partial)");
  {
    run_case<f64>(73, 97, 113, false, false, 1.0, 0.0, 0x300ull, 1e-10);
    run_case<f64>(73, 97, 113, true, false, 1.0, 0.5, 0x301ull, 1e-10);
    run_case<f64>(73, 97, 113, false, true, 0.5, -0.5, 0x302ull, 1e-10);
    run_case<f64>(73, 97, 113, true, true, 1.5, 2.0, 0x303ull, 1e-10);
  }
  end_test_case();

  test_case("256×256×256 (blocked path, multiple Mc/Kc/Nc tiles)");
  {
    run_case<f64>(256, 256, 256, false, false, 1.0, 0.0, 0x400ull, 1e-10);
    run_case<f64>(256, 256, 256, true, true, 1.0, 1.0, 0x401ull, 1e-10);
  }
  end_test_case();

  test_case("blocked path on f32");
  {
    run_case<f32>(64, 64, 64, false, false, 1.0f, 0.0f, 0x500ull, 1e-4f);
    run_case<f32>(73, 97, 113, false, false, 1.0f, 0.0f, 0x501ull, 1e-4f);
    run_case<f32>(73, 97, 113, true, true, 1.0f, 1.0f, 0x502ull, 1e-4f);
  }
  end_test_case();

  test_case("aliasing: C aliases A (in-place: C ← α·A·B + β·C with C ≡ A)");
  {
    // Use one buffer for both A and C; B is separate.  The aliasing
    // guard in level3::gemm should detect overlap and route through
    // a temporary.
    const usize m = 64, n = 64, k = 64;
    dynmat<f64> AC(m, k);
    dynmat<f64> B(k, n);
    fill_pattern(AC.data(), m * k, 0x600ull);
    fill_pattern(B.data(), k * n, 0x601ull);
    dynmat<f64> A_orig = AC;
    dynmat<f64> C_ref = AC;      // same contents as A initially

    // reference: read from a separate copy of A, write to C_ref
    ref_gemm<f64>(false, false, m, n, k, 1.0, A_orig.data(), k, B.data(), n, 0.5, C_ref.data(), n);

    // aliased: A and C share the same buffer
    auto Av = as_row_view(AC);
    auto Bv = as_row_view(B);
    auto Cv = as_row_view(AC);      // same view storage as Av
    blas::level3::gemm(f64(1.0), Av, Bv, f64(0.5), Cv);

    const f64 r = max_rel_residual<f64>(AC.data(), C_ref.data(), m * n);
    require_true(r < 1e-10);
  }
  end_test_case();

  test_case("aliasing: C aliases B");
  {
    const usize m = 32, n = 32, k = 32;
    dynmat<f64> A(m, k);
    dynmat<f64> BC(k, n);
    fill_pattern(A.data(), m * k, 0x700ull);
    fill_pattern(BC.data(), k * n, 0x701ull);
    dynmat<f64> B_orig = BC;
    dynmat<f64> C_ref = BC;

    ref_gemm<f64>(false, false, m, n, k, 1.0, A.data(), k, B_orig.data(), n, -0.25, C_ref.data(), n);

    auto Av = as_row_view(A);
    auto Bv = as_row_view(BC);
    auto Cv = as_row_view(BC);
    blas::level3::gemm(f64(1.0), Av, Bv, f64(-0.25), Cv);

    const f64 r = max_rel_residual<f64>(BC.data(), C_ref.data(), m * n);
    require_true(r < 1e-10);
  }
  end_test_case();

  return 0;
}
