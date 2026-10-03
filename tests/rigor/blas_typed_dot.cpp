//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/io/console.hpp>
#include <micron/math/blas/level1.hpp>
#include <micron/math/quants/dynvec.hpp>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// A reused typed buffer must be reloaded by packed dot and its scalar tail.

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:halt_on_error=1:abort_on_error=0:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1:print_stacktrace=1";
}

template<class F, usize n>
[[gnu::noinline]] bool
check(F start)
{
  micron::math::dynvec<F> r(n, F(0));
  auto value = [&](F t) {
    for ( usize k = 0; k < n; ++k ) r[k] = F(k + 1) * t - F(k % 3);
    return F(0.5) * micron::math::blas::level1::dot<F>(r.data(), r.data() + n, r.data());
  };
  const volatile F warm = value(start);
  (void)warm;
  F expected{};
  for ( usize k = 0; k < n; ++k ) expected += F(k + 1) * (F(k + 1) * start - F(k % 3));
  const F fd = (value(start + F(1)) - value(start - F(1))) / F(2);
  return fd == expected;
}

int
main(int argc, char **)
{
  bool ok = true;
  for ( usize i = 0; i < 32; ++i ) {
    bool pass = check<f64, 5>(f64(argc + i));
    pass &= check<f32, 9>(f32(argc + i));
    pass &= check<double, 5>(double(argc + i));
    pass &= check<float, 9>(float(argc + i));
    if ( !pass ) micron::console("typed dot reload failed: ", i);
    ok &= pass;
  }
  return ok ? 1 : 2;
}
