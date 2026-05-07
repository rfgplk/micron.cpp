// math.cpp
// Tour of micron::math (src/math/) — micron's scalar / linear-algebra
// math layer. micron does not include <cmath>; everything here is
// computed in pure C++ via constexpr-friendly bit-twiddling and a
// table-driven Ryu-like layer.
//
// What this example shows:
//   - constants:    pi / e / sqrt2 / log2e via math::constant_*
//   - scalars:      sqrt, log, exp, pow, fabs, fmin, fmax, fclamp
//   - branchless:   abs / sign / clamp without conditional jumps
//   - ratios:       compile-time rational numbers (kilo/milli/...)
//   - linalg:       vec<T,N> dot/cross/norm/normalize, mat<T,R,C>
//   - quaternions:  quat<F> normalize/norm
//
// Major STL deltas:
//   - No <cmath> — micron's math is a first-party implementation.
//   - Constants are templated:  constant_pi<f64> vs M_PI macro.
//   - Branchless toolkit lives at src/math/branchless.hpp; the names
//     are abs8 / abs16 / abs32 / abs64 (per-width).
//   - vec / mat / quat are *fixed-size, alignas-controlled* aggregates,
//     not heap-allocated. dynvec / dynmat are the heap variants.
//   - linalg ops (dot, cross, norm, normalize) operate on those types
//     and live in math::linalg::ops.

#include "../src/io/console.hpp"
#include "../src/math/branchless.hpp"
#include "../src/math/constants.hpp"
#include "../src/math/generic.hpp"
#include "../src/math/linalg.hpp"
#include "../src/math/log.hpp"
#include "../src/math/ratios.hpp"
#include "../src/math/sqrt.hpp"
#include "../src/math/trig.hpp"

int
main()
{
  // ================================================================
  // 1. Constants
  // ----------------------------------------------------------------
  // Templated by float type so you pick the precision at the call site.
  // ================================================================
  micron::io::println("-- 1. constants --");

  micron::io::println("pi    = ", micron::math::constant_pi<f64>);
  micron::io::println("e     = ", micron::math::constant_e<f64>);
  micron::io::println("sqrt2 = ", micron::math::constant_sqrt2<f64>);
  micron::io::println("ln2   = ", micron::math::constant_ln2<f64>);
  micron::io::println("log2e = ", micron::math::constant_log2e<f64>);

  // ================================================================
  // 2. Scalar functions: sqrt, log, exp, pow
  // ----------------------------------------------------------------
  // Each of these has overloads for f32 / f64 / long double.
  // Most are constexpr-friendly so they evaluate at compile time
  // when the input is a literal.
  // ================================================================
  micron::io::println("-- 2. scalar fns --");

  micron::io::println("sqrt(2)        = ", micron::math::fsqrt(2.0));
  micron::io::println("fsqrt(9.0)     = ", micron::math::fsqrt(9.0));
  micron::io::println("log(e)         = ", micron::math::log(micron::math::constant_e<f64>));
  micron::io::println("exp(1)         = ", micron::math::exp(1.0));
  micron::io::println("pow(2, 10)     = ", micron::math::pow(2.0, 10));

  // ================================================================
  // 3. fmin / fmax / fclamp
  // ----------------------------------------------------------------
  // NaN-aware floating-point min/max. fclamp pins x into [lo, hi].
  // ================================================================
  micron::io::println("-- 3. fmin/fmax/fclamp --");

  micron::io::println("fmin(3.0, 5.0) = ", micron::math::fmin<f64>(3.0, 5.0));
  micron::io::println("fmax(3.0, 5.0) = ", micron::math::fmax<f64>(3.0, 5.0));
  micron::io::println("fclamp(15, 0, 10) = ", micron::math::fclamp<f64>(15.0, 0.0, 10.0));

  // ================================================================
  // 4. Branchless integer primitives
  // ----------------------------------------------------------------
  // micron::math::branchless:: — abs/sign/min/max with no jumps.
  // Useful in hot loops where branch mispredict would dominate.
  // ================================================================
  micron::io::println("-- 4. branchless --");

  micron::io::println("abs32(-7)       = ", micron::math::branchless::abs32(-7));
  micron::io::println("abs64(-1234567) = ", micron::math::branchless::abs64(-1234567LL));

  // ================================================================
  // 5. ratios — compile-time rationals (SI prefix style)
  // ----------------------------------------------------------------
  // ratio<N, D> stores num/denom as constexpr static members. Aliases
  // exist for SI prefixes: kilo, mega, giga, milli, micro, nano, ...
  // ================================================================
  micron::io::println("-- 5. ratios --");

  micron::io::println("kilo  = ", micron::kilo::num,  "/", micron::kilo::denom);
  micron::io::println("milli = ", micron::milli::num, "/", micron::milli::denom);
  micron::io::println("nano  = ", micron::nano::num,  "/", micron::nano::denom);

  // ================================================================
  // 6. vec<T, N> — fixed-size, alignas-controlled vector
  // ----------------------------------------------------------------
  // Aggregate type with .data[N]. Linalg ops live in linalg::.
  // ================================================================
  micron::io::println("-- 6. vec --");

  micron::math::vec<f64, 3> u{1.0, 2.0, 3.0};
  micron::math::vec<f64, 3> v{4.0, 5.0, 6.0};

  // dot product
  f64 d = micron::math::linalg::ops::dot(u, v);
  micron::io::println("dot(u,v)    = ", d);

  // cross product (3-vectors only)
  auto x = micron::math::linalg::ops::cross(u, v);
  micron::io::println("cross(u,v)  = [", x[0], ", ", x[1], ", ", x[2], "]");

  // norm / normalize
  f64 n = micron::math::linalg::ops::norm(u);
  auto un = micron::math::linalg::ops::normalize(u);
  micron::io::println("|u|         = ", n);
  micron::io::println("normalize(u)= [", un[0], ", ", un[1], ", ", un[2], "]");

  // ================================================================
  // 7. mat<T, R, C> — fixed-size matrix
  // ----------------------------------------------------------------
  // Aggregate row-major matrix. transpose returns a mat<T, C, R>.
  // ================================================================
  micron::io::println("-- 7. mat --");

  micron::math::mat<f64, 2, 3> m{
    1.0, 2.0, 3.0,
    4.0, 5.0, 6.0
  };

  auto mt = micron::math::linalg::ops::transpose(m);
  micron::io::println("m.at(0,0)=",  m.at(0, 0),  " m.at(1,2)=",  m.at(1, 2));
  micron::io::println("mt.at(0,0)=", mt.at(0, 0), " mt.at(2,1)=", mt.at(2, 1));

  // ================================================================
  // 8. Where to look next
  // ----------------------------------------------------------------
  //   src/math/linalg/decomp.hpp  — LU / QR / Cholesky factorisations
  //   src/math/linalg/ops.hpp     — matmul, vec ops, quaternion ops
  //   src/math/blas/              — BLAS levels 1/2/3
  //   src/math/quants/            — quat, dynvec, fixed-size vec
  //   src/math/splines/           — bspline / bezier / catmull-rom
  //   src/math/integrate/         — quadrature
  //   src/math/rng/               — seeded RNG (xoshiro/pcg/...)
  //   src/math/simd/              — vectorised exp/log/trig
  // ================================================================
  return 0;
}
