// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/blas/level3.hpp"
#include "../../src/memory/allocation/abcmalloc/stats.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1";
}

// Full panels, all transpose combinations and ragged edges cross MC/NC/KC together.
template<class F>
void
check(bool ta, bool tb)
{
  constexpr usize m = 73, n = 259, k = 257, ldc = n + 3;
  const usize lda = (ta ? m : k) + 3, ldb = (tb ? k : n) + 3;
  micron::vector<F, micron::allocator_serial<>, false> a((ta ? k : m) * lda), b((tb ? n : k) * ldb), c(m * ldc), reference(m * ldc);
  for ( usize i = 0; i < a.size(); ++i ) a[i] = F(i32(i % 17) - 8) / F(17);
  for ( usize i = 0; i < b.size(); ++i ) b[i] = F(i32((i * 7) % 19) - 9) / F(19);
  for ( usize i = 0; i < c.size(); ++i ) c[i] = reference[i] = F(3);
  for ( usize i = 0; i < m; ++i )
    for ( usize j = 0; j < n; ++j ) {
      f64 sum{};
      for ( usize z = 0; z < k; ++z ) sum += f64(a[ta ? z * lda + i : i * lda + z]) * f64(b[tb ? j * ldb + z : z * ldb + j]);
      reference[i * ldc + j] = F(f64(.5) * sum - f64(.75));
    }
  micron::math::matrix::pack::workspace<F> workspace;
  micron::math::matrix::pack::scoped_workspace<F> prepared(workspace);
  const auto before = abc::stats();
  for ( usize repeat = 0; repeat < 4; ++repeat ) {
    for ( usize i = 0; i < c.size(); ++i ) c[i] = F(3);
    micron::math::blas::level3::gemm_row(ta, tb, m, n, k, F(.5), a.data(), lda, b.data(), ldb, F(-.25), c.data(), ldc);
  }
  const auto after = abc::stats();
#if MICRON_ABC_STATS
  sb::require_true(before.enabled && before.alloc_requests == after.alloc_requests && before.dealloc_requests == after.dealloc_requests);
#else
  (void)before;
  (void)after;
#endif
  bool good = true;
  for ( usize i = 0; i < c.size(); ++i ) {
    F d = c[i] - reference[i];
    if ( d < 0 ) d = -d;
    good &= d < (sizeof(F) == 4 ? F(3e-5) : F(3e-12));
  }
  sb::require_true(good);
}

int
main()
{
  sb::test_case("GEMM packing reuses bounded per-thread typed panels");
  for ( bool ta : { false, true } )
    for ( bool tb : { false, true } ) {
      check<f32>(ta, tb);
      check<f64>(ta, tb);
    }
  sb::end_test_case();
  return 1;
}
