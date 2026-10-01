// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/blas/level3.hpp"
#include "../snowball/snowball.hpp"

consteval bool
folds()
{
  f64 a[]{ 2, 4 }, b[]{ 3, 5 }, c[]{ 0 };
  micron::math::blas::level3::gemm_row(false, false, 1, 1, 1, f64(0.5), a, 1, b, 1, f64(0), c, 1);
  if ( c[0] != 3 ) return false;
  micron::math::blas::level3::gemm_row(false, false, 1, 1, 2, f64(1), a, 2, b, 1, f64(0), c, 1);
  return c[0] == 26;
}

static_assert(folds());

// Rank-one GEMM preserves all transpose/stride combinations and never reads beta-zero C.
template<class F>
void
run()
{
  // Preserve the scalar fallback's alpha*A multiplication order: A*B alone overflows.
  F large = F(sizeof(F) == 4 ? 1e20 : 1e200), small = F(1) / large, out{};
  micron::math::blas::level3::gemm_row(false, false, 1, 1, 1, small, &large, 1, &large, 1, F(0), &out, 1);
  sb::require_true(out > large * F(0.99) && out < large * F(1.01));
  for ( bool ta : { false, true } )
    for ( bool tb : { false, true } )
      for ( usize m : { usize(1), usize(3), usize(8) } )
        for ( usize n : { usize(1), usize(7), usize(16) } )
          for ( F beta : { F(0), F(1), F(-0.5) } ) {
            F a[128]{}, b[128]{}, c[160], reference[160];
            const usize lda = ta ? m + 1 : 2, ldb = tb ? 2 : n + 1, ldc = n + 2;
            for ( usize i = 0; i < m; ++i ) a[i * (ta ? 1 : lda)] = F(i + 1) / F(3);
            for ( usize j = 0; j < n; ++j ) b[j * (tb ? ldb : 1)] = F(j + 2) / F(5);
            for ( usize i = 0; i < 160; ++i ) c[i] = reference[i] = F(123);
            for ( usize i = 0; i < m; ++i )
              for ( usize j = 0; j < n; ++j ) {
                if ( beta == F(0) ) c[i * ldc + j] = F(__builtin_nan(""));
                reference[i * ldc + j] = F(0.75) * (a[i * (ta ? 1 : lda)] * b[j * (tb ? ldb : 1)]) + beta * F(123);
              }
            micron::math::blas::level3::gemm_row(ta, tb, m, n, usize(1), F(0.75), a, lda, b, ldb, beta, c, ldc);
            for ( usize i = 0; i < 160; ++i ) {
              F d = c[i] - reference[i];
              if ( d < 0 ) d = -d;
              sb::require_true(d < (sizeof(F) == 4 ? F(2e-5) : F(2e-13)));
            }
          }
}

int
main()
{
  sb::test_case("rank-one GEMM");
  run<f32>();
  run<f64>();
  sb::end_test_case();
  return 1;
}
