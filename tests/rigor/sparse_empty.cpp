//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/sparse/product.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1:print_stacktrace=1";
}

template<class F, class I>
void
run()
{
  namespace ms = micron::math::sparse;
  sb::test_case("zero-nonzero compressed storage, empty conversion and products");
  for ( usize n = 0; n < 4; ++n ) {
    ms::csr<F, I> r(n, n);
    ms::csc<F, I> c(n, n);
    auto rc = ms::to_csc(r);
    auto cr = ms::to_csr(c);
    auto triplets = ms::csc<F, I>::from_triplets_sorted(n, n, nullptr, nullptr, nullptr, 0);
    sb::require_true(r.nnz() == 0 && c.nnz() == 0 && rc.nnz() == 0 && cr.nnz() == 0 && triplets.nnz() == 0);
    sb::require_true(rc.rows == n && cr.cols == n && rc.outer.size() == n + 1 && cr.outer.size() == n + 1);
    for ( usize j = 0; j <= n; ++j ) sb::require_true(rc.outer.data()[j] == 0 && cr.outer.data()[j] == 0);
    if ( n != 0 ) {
      micron::math::dynvec<F> x(n, F(1)), y(n, F(7));
      ms::spmv(F(1), rc, x, F(0), y);
      for ( usize j = 0; j < n; ++j ) sb::require_true(y[j] == F(0));
      ms::spmv_transposed(F(1), rc, x, F(0), y);
      for ( usize j = 0; j < n; ++j ) sb::require_true(y[j] == F(0));
      ms::spmv(F(1), cr, x, F(0), y);
      for ( usize j = 0; j < n; ++j ) sb::require_true(y[j] == F(0));
    }
  }
  sb::end_test_case();
  sb::test_case("nonempty round trip keeps values and empty compressed segments");
  const I rows[]{ I(0), I(2), I(1) }, cols[]{ I(0), I(0), I(2) };
  const F values[]{ F(2), F(-1), F(3) };
  auto a = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, values, 3);
  auto b = ms::to_csr(a);
  auto c = ms::to_csc(b);
  sb::require_true(c.nnz() == 3);
  for ( usize i = 0; i < 4; ++i ) sb::require_true(a.outer.data()[i] == c.outer.data()[i]);
  for ( usize i = 0; i < 3; ++i ) sb::require_true(a.inner.data()[i] == c.inner.data()[i] && a.values.data()[i] == c.values.data()[i]);
  micron::math::dynvec<F> x(3, F(2)), y(3, F(0));
  ms::spmv(F(1), b, x, F(0), y);
  sb::require_true(y[0] == F(4) && y[1] == F(6) && y[2] == F(-2));
  sb::end_test_case();
}

int
main()
{
  run<f32, u32>();
  run<f64, i32>();
  return 1;
}
