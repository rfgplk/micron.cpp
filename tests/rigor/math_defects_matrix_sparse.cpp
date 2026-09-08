// math_defects_matrix_sparse.cpp
// The GEMM-packing and sparse-kernel defects: gemm_blocked's two heap panel buffers sized
// Mc*Kc / Kc*Nc while the packers write the MR/NR-rounded product, the vmovapd C epilogue in
// micro_kernel_6x8_avx2_f64_asm_aligned that needs every C row 32-byte aligned, the
// cross-iteration B preload the four software-pipelined asm kernels issue at the end of their
// last K block, and the beta pre-scaling in the three spmv entry points running over y.size()
// instead of the matrix extent.
//
// Sections (a) and (d) are arch-neutral. (b) and (c) need the amd64 AVX2+FMA kernels and are
// gated; (c) places a B panel flush against an mprotect(PROT_NONE) guard page, so the assertion
// is "the process is still here", which is exactly what the defect fails.

#include "../../src/math/blas/blas.hpp"
#include "../../src/math/matrix/pack.hpp"
#include "../../src/math/sparse.hpp"
#include "../../src/memory/mman.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

namespace ms = micron::math::sparse;
namespace mp = micron::math::matrix::pack;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// utilities

static u64 __seed = 0xA5C3F00D5A3C0FF0ull;      // fixed

static double
rnd(void) noexcept
{
  __seed ^= __seed << 13;
  __seed ^= __seed >> 7;
  __seed ^= __seed << 17;
  return double(__seed >> 11) / 9007199254740992.0 * 2.0 - 1.0;
}

static double
absd(double x) noexcept
{
  return x < 0.0 ? -x : x;
}

// naive ijk reference for row-major C = A*B
static void
ref_gemm(usize m, usize n, usize k, const f64 *A, const f64 *B, f64 *R) noexcept
{
  for ( usize i = 0; i < m * n; ++i ) R[i] = f64(0);
  for ( usize i = 0; i < m; ++i )
    for ( usize p = 0; p < k; ++p ) {
      const f64 a = A[i * k + p];
      for ( usize j = 0; j < n; ++j ) R[i * n + j] += a * B[p * n + j];
    }
}

int
main()
{
  print("=== GEMM packing / sparse kernel defects ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (a) gemm_blocked's panel buffers must hold what the packers write.
  //
  // Both packers round the panel up to whole MR-row / NR-column blocks and zero-fill the
  // remainder, so pack_a_panel writes ceil(Mc/MR)*MR*Kc and pack_b_panel ceil(Nc/NR)*NR*Kc.
  // gemm_blocked allocated Mc*Kc and Kc*Nc. These shapes are the ones where the A-panel
  // overrun clears the whole distance to the B-panel allocation: on the defective tree the
  // allocator detects the corruption and hard-exits, so this section never reaches its
  // requires. Kept first so the heap state is as close to fresh as the harness allows.
  test_case("gemm_blocked panel buffer sizing");
  {
    constexpr usize MR = mp::gemm_mr_v<f64>;
    constexpr usize NR = mp::gemm_nr_v<f64>;
    static const usize shapes[][3] = { { 13, 9, 256 },  { 14, 9, 256 },  { 37, 33, 200 }, { 38, 33, 200 }, { 61, 33, 256 },
                                       { 62, 33, 256 }, { 61, 65, 255 }, { 13, 7, 300 },  { 5, 5, 300 },   { 9, 17, 512 },
                                       { 71, 71, 300 }, { 66, 66, 200 }, { 17, 23, 251 }, { 30, 29, 272 }, { 70, 64, 256 } };
    bool any_rounded = false;
    for ( const usize *s : shapes ) {
      const usize m = s[0], n = s[1], k = s[2];
      const usize Mc = (m < mp::default_mc) ? m : mp::default_mc;
      const usize Kc = (k < mp::default_kc) ? k : mp::default_kc;
      const usize Nc = (n < mp::default_nc) ? n : mp::default_nc;
      if ( ((Mc + MR - 1) / MR) * MR != Mc || ((Nc + NR - 1) / NR) * NR != Nc ) any_rounded = true;

      vector<f64> A(m * k), B(k * n), C(m * n), R(m * n);
      for ( usize i = 0; i < m * k; ++i ) A[i] = f64(rnd());
      for ( usize i = 0; i < k * n; ++i ) B[i] = f64(rnd());
      for ( usize i = 0; i < m * n; ++i ) C[i] = f64(0);
      ref_gemm(m, n, k, A.data(), B.data(), R.data());
      mp::gemm_blocked<f64>(m, n, k, f64(1), A.data(), ssize_t(k), 1, B.data(), ssize_t(n), 1, f64(0), C.data(), ssize_t(n), 1);
      double e = 0.0;
      for ( usize i = 0; i < m * n; ++i ) {
        const double d = absd(double(C[i]) - double(R[i]));
        if ( d > e ) e = d;
      }
      require_true(e < 1e-9);
    }
    // the shape list must actually exercise the rounding on this target
    require_true(any_rounded);
  }
  end_test_case();

#if defined(__micron_arch_amd64) && defined(__AVX2__) && defined(__FMA__)

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (b) micro_kernel_6x8_avx2_f64_asm_aligned stores C with vmovapd, so all six row
  // pointers C + i*rs_C need 32-byte alignment: an aligned base is not enough, rs_C has to
  // be a multiple of 4 doubles too. Nothing upstream checks or documents either, and
  // gemm_row_aligned is a public entry point with no stated ldc constraint.
  test_case("aligned 6x8 asm kernel vs C row alignment");
  {
    alignas(64) static f64 Ap[6 * 64];
    alignas(64) static f64 Bp[8 * 64];
    alignas(64) static f64 Craw[6 * 16 + 8];
    for ( usize i = 0; i < 6 * 64; ++i ) Ap[i] = f64(1);
    for ( usize i = 0; i < 8 * 64; ++i ) Bp[i] = f64(1);
    // every C base displacement x every row stride, including the misaligned ones
    for ( usize off = 0; off < 4; ++off ) {
      for ( ssize_t rs = 8; rs <= 12; ++rs ) {
        f64 *C = Craw + off;
        for ( usize i = 0; i < 6 * 16; ++i ) Craw[i] = f64(0);
        mp::micro_kernel_6x8_avx2_f64_asm_aligned(reinterpret_cast<const double *>(Ap), reinterpret_cast<const double *>(Bp), 4, 1.0, 0.0,
                                                  reinterpret_cast<double *>(C), rs, 1);
        for ( usize i = 0; i < 6; ++i )
          for ( usize j = 0; j < 8; ++j ) require_true(absd(double(C[ssize_t(i) * rs + ssize_t(j)]) - 4.0) < 1e-12);
      }
    }
    // and through the public aligned dispatch, with an ldc that is not a multiple of 4
    constexpr usize M = 73, N = 96, K = 113;
    for ( usize ldc : { usize(96), usize(97), usize(98), usize(99), usize(100) } ) {
      vector<f64> A(M * K), B(K * ldc), C(M * ldc);
      for ( usize i = 0; i < M * K; ++i ) A[i] = f64(1);
      for ( usize i = 0; i < K * ldc; ++i ) B[i] = f64(0);
      for ( usize p = 0; p < K; ++p )
        for ( usize j = 0; j < N; ++j ) B[p * ldc + j] = f64(1);
      for ( usize i = 0; i < M * ldc; ++i ) C[i] = f64(0);
      blas::level3::gemm_row_aligned<f64>(false, false, M, N, K, f64(1), A.data(), K, B.data(), ldc, f64(0), C.data(), ldc);
      for ( usize i = 0; i < M; ++i )
        for ( usize j = 0; j < N; ++j ) require_true(absd(double(C[i * ldc + j]) - double(K)) < 1e-12);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (c) the four software-pipelined asm kernels issued their cross-iteration B preload
  // unconditionally at the end of the last K block, reading one whole B row past the panel
  // (64 bytes for the 6x8 shapes, 96 for the 4x12). Placing the panel flush against a
  // PROT_NONE guard page turns that read into a SIGSEGV. The three kernels with no
  // cross-iteration preload are the ungated control: they must survive the same harness.
  test_case("software-pipelined B preload stays inside the panel");
  {
    const usize pg = 4096;
    addr_t *base = micron::mmap(0, 3 * pg, prot_read | prot_write, map_private | map_anonymous, -1, 0);
    require_true(!micron::mmap_failed(base));
    byte *raw = reinterpret_cast<byte *>(base);
    require_true(micron::mprotect(reinterpret_cast<addr_t *>(raw + 2 * pg), pg, prot_none) == 0);

    static double Apad[12 * 64];
    for ( usize i = 0; i < 12 * 64; ++i ) Apad[i] = 1.0;
    double Cq[6 * 12];

    for ( usize k = 0; k <= 17; ++k ) {
      // 6x8 panel: 8 doubles per K step, flush against the guard boundary
      double *Bp8 = reinterpret_cast<double *>(raw + 2 * pg - 8 * sizeof(double) * k);
      for ( usize i = 0; i < 8 * k; ++i ) Bp8[i] = 1.0;
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_6x8_avx2_f64_asm_unr4_sp(Apad, Bp8, k, 1.0, 0.0, Cq, 8, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_6x8_avx2_f64_asm_relax(Apad, Bp8, k, 1.0, 0.0, Cq, 8, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_6x8_avx2_f64_asm_roll(Apad, Bp8, k, 1.0, 0.0, Cq, 8, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);
      // CONTROL: the three kernels with no cross-iteration preload
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_6x8_avx2_f64_asm(Apad, Bp8, k, 1.0, 0.0, Cq, 8, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_6x8_avx2_f64_asm_unr4(Apad, Bp8, k, 1.0, 0.0, Cq, 8, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);

      // 4x12 panel: 12 doubles per K step
      double *Bp12 = reinterpret_cast<double *>(raw + 2 * pg - 12 * sizeof(double) * k);
      for ( usize i = 0; i < 12 * k; ++i ) Bp12[i] = 1.0;
      for ( usize i = 0; i < 6 * 12; ++i ) Cq[i] = 0.0;
      mp::micro_kernel_4x12_avx2_f64_asm_unr4_sp(Apad, Bp12, k, 1.0, 0.0, Cq, 12, 1);
      require_true(absd(Cq[0] - double(k)) < 1e-12);
    }
    micron::munmap(base, 3 * pg);
  }
  end_test_case();

#endif      // amd64 AVX2 + FMA

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // (d) spmv's beta pre-scaling ran over y.size(); the operation is only defined over
  // A.rows (A.cols for the transposed form) and the header comments state the precondition
  // as y.size() >= that, so a longer y is legal and its tail must come back untouched.
  test_case("spmv beta pre-scaling extent");
  {
    u32 ri[3] = { 0, 1, 2 };
    u32 ci[3] = { 0, 1, 2 };
    f64 vv[3] = { f64(1), f64(1), f64(1) };
    auto A = ms::csc<f64, u32>::from_triplets_sorted(3, 3, ri, ci, vv, 3);
    auto Ar = ms::to_csr<f64, u32>(A);
    dynvec<f64> x(3, f64(1));

    for ( f64 beta : { f64(0), f64(1), f64(2) } ) {
      // csc
      dynvec<f64> y(7);
      for ( usize i = 0; i < 7; ++i ) y[i] = f64(100 + int(i));
      ms::spmv<f64, u32>(f64(1), A, x, beta, y);
      for ( usize i = 0; i < 3; ++i ) require_true(absd(double(y[i]) - (1.0 + double(beta) * double(100 + int(i)))) < 1e-12);
      for ( usize i = 3; i < 7; ++i ) require_true(double(y[i]) == double(100 + int(i)));
      // csc transposed
      for ( usize i = 0; i < 7; ++i ) y[i] = f64(100 + int(i));
      ms::spmv_transposed<f64, u32>(f64(1), A, x, beta, y);
      for ( usize i = 0; i < 3; ++i ) require_true(absd(double(y[i]) - (1.0 + double(beta) * double(100 + int(i)))) < 1e-12);
      for ( usize i = 3; i < 7; ++i ) require_true(double(y[i]) == double(100 + int(i)));
      // csr
      for ( usize i = 0; i < 7; ++i ) y[i] = f64(100 + int(i));
      ms::spmv<f64, u32>(f64(1), Ar, x, beta, y);
      for ( usize i = 0; i < 3; ++i ) require_true(absd(double(y[i]) - (1.0 + double(beta) * double(100 + int(i)))) < 1e-12);
      for ( usize i = 3; i < 7; ++i ) require_true(double(y[i]) == double(100 + int(i)));
    }
    // CONTROL: a y sized exactly to the extent is unaffected either way
    dynvec<f64> ye(3);
    for ( usize i = 0; i < 3; ++i ) ye[i] = f64(5);
    ms::spmv<f64, u32>(f64(1), A, x, f64(2), ye);
    for ( usize i = 0; i < 3; ++i ) require_true(absd(double(ye[i]) - 11.0) < 1e-12);
  }
  end_test_case();

  print("=== GEMM packing / sparse kernel defects ok ===");
  return 1;
}
