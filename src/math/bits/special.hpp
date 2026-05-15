//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// specials live here (erf erfc tgamma lgamma)

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "coeff/lanczos.hpp"
#include "exp.hpp"
#include "log.hpp"
#include "manip.hpp"
#include "pow.hpp"
#include "round.hpp"
#include "sqrt.hpp"
#include "trig.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace special_ns
{

namespace __erf_data
{
// erf(x) ≈ x + x*P(z)/Q(z) z = x²
inline constexpr f64 efx = 1.28379167095512586316e-01;      // 2/sqrt(pi) - 1
inline constexpr f64 pp[5] = { 1.28379167095512558561e-01, -3.25042107247001499370e-01, -2.84817495755985104766e-02,
                               -5.77027029648944159157e-03, -2.37630166566501626084e-05 };
inline constexpr f64 qq[5]
    = { 1.0, 3.97917223959155352819e-01, 6.50222499887672944485e-02, 5.08130628187576562776e-03, 1.32494738004321644526e-04 };

// erf(x) = 1 - erfc(x)
inline constexpr f64 erx = 8.45062911510467529297e-01;
inline constexpr f64 pa[7]
    = { -2.36211856075265944077e-03, 4.14856118683748331666e-01, -3.72207876035701323847e-01, 3.18346619901161753674e-01,
        -1.10894694282396677476e-01, 3.54783043256182359371e-02, -2.16637559486879084300e-03 };
inline constexpr f64 qa[6] = { 1.0,
                               1.06420880400844228286e-01,
                               5.40397917702171048937e-01,
                               7.18286544141962662868e-02,
                               1.26171219808761642112e-01,
                               1.36370839120290507362e-02 };

inline constexpr f64 ra[8]
    = { -9.86494403484714822705e-03, -6.93858572707181764372e-01, -1.05586262253232909814e+01, -6.23753324503260060396e+01,
        -1.62396669462573470355e+02, -1.84605092906711035994e+02, -8.12874355063065934246e+01, -9.81432934416256659757e+00 };
inline constexpr f64 sa[8] = { 1.0,
                               1.96512716674392571292e+01,
                               1.37657754143519042600e+02,
                               4.34565877475229228821e+02,
                               6.45387271733267880336e+02,
                               4.29008140027567833386e+02,
                               1.08635005541779435134e+02,
                               6.57024977031928170135e+00 };

inline constexpr f64 rb[7]
    = { -9.86494292470009928597e-03, -7.99283237680523006574e-01, -1.77579549177547519889e+01, -1.60636384855821916062e+02,
        -6.37566443368389627722e+02, -1.25029579241453833978e+03, -1.05532763257948929011e+03 };
inline constexpr f64 sb[7] = { 1.0,
                               3.03380607434824582929e+01,
                               3.25792512996573918826e+02,
                               1.53672958608443695994e+03,
                               3.19985821950859553908e+03,
                               2.55305040643316442583e+03,
                               4.74528541206955367215e+02 };
};      // namespace __erf_data

[[nodiscard, gnu::flatten]] inline constexpr f64
erf_f64(f64 x) noexcept
{
  using namespace __erf_data;
  if ( ieee::is_nan(x) ) return x;
  if ( ieee::is_inf(x) ) return manip::copysign<f64>(1.0, x);
  f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p-28 ) return x + efx * x;      // tiny: erf(x) ≈ x*(1 + 2/sqrt(π))
  if ( ax < 0.84375 ) {
    f64 z = x * x;
    f64 r = pp[0] + z * (pp[1] + z * (pp[2] + z * (pp[3] + z * pp[4])));
    f64 s = qq[0] + z * (qq[1] + z * (qq[2] + z * (qq[3] + z * qq[4])));
    return x + x * (r / s);
  }
  if ( ax < 1.25 ) {
    f64 s = ax - 1.0;
    f64 P = pa[0] + s * (pa[1] + s * (pa[2] + s * (pa[3] + s * (pa[4] + s * (pa[5] + s * pa[6])))));
    f64 Q = qa[0] + s * (qa[1] + s * (qa[2] + s * (qa[3] + s * (qa[4] + s * qa[5]))));
    f64 r = erx + P / Q;
    return manip::signbit(x) ? -r : r;
  }
  if ( ax >= 6.0 ) return manip::copysign<f64>(1.0, x);
  f64 s = 1.0 / (ax * ax);
  f64 R, S;
  if ( ax < 2.857142857142857 ) {
    R = ra[0] + s * (ra[1] + s * (ra[2] + s * (ra[3] + s * (ra[4] + s * (ra[5] + s * (ra[6] + s * ra[7]))))));
    S = sa[0] + s * (sa[1] + s * (sa[2] + s * (sa[3] + s * (sa[4] + s * (sa[5] + s * (sa[6] + s * sa[7]))))));
  } else {
    R = rb[0] + s * (rb[1] + s * (rb[2] + s * (rb[3] + s * (rb[4] + s * (rb[5] + s * rb[6])))));
    S = sb[0] + s * (sb[1] + s * (sb[2] + s * (sb[3] + s * (sb[4] + s * (sb[5] + s * sb[6])))));
  }
  f64 z_hi = ieee::from_bits<f64>(ieee::to_bits(ax) & 0xffffffff00000000ULL);
  f64 r = exp_ns::exp_f64(-z_hi * z_hi - 0.5625) * exp_ns::exp_f64((z_hi - ax) * (z_hi + ax) + R / S) / ax;
  f64 res = 1.0 - r;
  return manip::signbit(x) ? -res : res;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
erfc_f64(f64 x) noexcept
{
  using namespace __erf_data;
  if ( ieee::is_nan(x) ) return x;
  if ( ieee::is_inf(x) ) return manip::signbit(x) ? 2.0 : 0.0;
  f64 ax = manip::fabs(x);
  if ( ax < 0.84375 ) {
    if ( ax < 0x1.0p-56 ) return 1.0 - x;      // tiny
    f64 z = x * x;
    f64 r = pp[0] + z * (pp[1] + z * (pp[2] + z * (pp[3] + z * pp[4])));
    f64 s = qq[0] + z * (qq[1] + z * (qq[2] + z * (qq[3] + z * qq[4])));
    f64 y = r / s;
    if ( ax < 0.25 ) return 1.0 - (x + x * y);
    f64 r2 = x * y;
    r2 += (x - 0.5);
    return 0.5 - r2;
  }
  if ( ax < 1.25 ) {
    f64 s = ax - 1.0;
    f64 P = pa[0] + s * (pa[1] + s * (pa[2] + s * (pa[3] + s * (pa[4] + s * (pa[5] + s * pa[6])))));
    f64 Q = qa[0] + s * (qa[1] + s * (qa[2] + s * (qa[3] + s * (qa[4] + s * qa[5]))));
    if ( manip::signbit(x) ) return 1.0 + (erx + P / Q);
    return 1.0 - erx - P / Q;
  }
  if ( ax >= 28.0 ) return manip::signbit(x) ? 2.0 : 0.0;
  f64 s = 1.0 / (ax * ax);
  f64 R, S;
  if ( ax < 2.857142857142857 ) {
    R = ra[0] + s * (ra[1] + s * (ra[2] + s * (ra[3] + s * (ra[4] + s * (ra[5] + s * (ra[6] + s * ra[7]))))));
    S = sa[0] + s * (sa[1] + s * (sa[2] + s * (sa[3] + s * (sa[4] + s * (sa[5] + s * (sa[6] + s * sa[7]))))));
  } else {
    R = rb[0] + s * (rb[1] + s * (rb[2] + s * (rb[3] + s * (rb[4] + s * (rb[5] + s * rb[6])))));
    S = sb[0] + s * (sb[1] + s * (sb[2] + s * (sb[3] + s * (sb[4] + s * (sb[5] + s * sb[6])))));
  }
  f64 z_hi = ieee::from_bits<f64>(ieee::to_bits(ax) & 0xffffffff00000000ULL);
  f64 r = exp_ns::exp_f64(-z_hi * z_hi - 0.5625) * exp_ns::exp_f64((z_hi - ax) * (z_hi + ax) + R / S) / ax;
  return manip::signbit(x) ? f64(2.0 - r) : r;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
tgamma_f64(f64 x) noexcept
{
  using namespace coeff::lanczos_data;
  if ( ieee::is_nan(x) ) return x;
  if ( x == 0.0 ) return manip::copysign<f64>(ieee::inf_v<f64>(0), x);
  if ( ieee::is_inf(x) && !manip::signbit(x) ) return x;

  if ( x < 0.5 ) {
    f64 sin_pix = trig_ns::sin_f64(0x1.921fb54442d18p+1 * x);
    if ( sin_pix == 0.0 ) return ieee::qnan_v<f64>();
    return 0x1.921fb54442d18p+1 / (sin_pix * tgamma_f64(1.0 - x));
  }
  f64 z = x - 1.0;
  f64 a = p[0];
  for ( int i = 1; i < 9; ++i ) a += p[i] / (z + f64(i));
  f64 t = z + g + 0.5;
  f64 sqrt_2pi = 0x1.40d931ff62705p+1;
  return sqrt_2pi * pow_ns::pow<f64>(t, z + 0.5) * exp_ns::exp_f64(-t) * a;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
lgamma_f64(f64 x) noexcept
{
  using namespace coeff::lanczos_data;
  if ( ieee::is_nan(x) ) return x;
  if ( ieee::is_inf(x) ) return ieee::inf_v<f64>(0);
  if ( x == 0.0 ) return ieee::inf_v<f64>(0);

  f64 ax = manip::fabs(x);
  if ( x < 0.5 ) {
    if ( x <= 0.0 ) {
      f64 fl = round_ns::floor<f64>(x);
      if ( fl == x ) return ieee::inf_v<f64>(0);      // negative integer
    }
    f64 sin_pix = trig_ns::sin_f64(0x1.921fb54442d18p+1 * x);
    f64 abs_sinpix = manip::fabs(sin_pix);
    return f64(1.1447298858494002) - log_ns::log_f64(abs_sinpix) - lgamma_f64(1.0 - x);
  }
  f64 z = x - 1.0;
  f64 a = p[0];
  for ( int i = 1; i < 9; ++i ) a += p[i] / (z + f64(i));
  f64 t = z + g + 0.5;
  (void)ax;
  return 0.91893853320467267 + (z + 0.5) * log_ns::log_f64(t) - t + log_ns::log_f64(a);
}

namespace __bessel
{

inline constexpr f64 pi_quarter = 0x1.921fb54442d18p-1;
inline constexpr f64 two_over_pi = 0x1.45f306dc9c883p-1;

[[nodiscard, gnu::flatten]] inline constexpr f64
j0_small(f64 x) noexcept
{
  const f64 z = 0.25 * x * x;
  constexpr f64 C[21] = {
    1.0,
    -1.0,
    1.0 / 4.0,
    -1.0 / 36.0,
    1.0 / 576.0,
    -1.0 / 14400.0,
    1.0 / 518400.0,
    -1.0 / 25401600.0,
    1.0 / 1625702400.0,
    -1.0 / 131681894400.0,
    1.0 / 13168189440000.0,
    -1.0 / 1593350922240000.0,
    1.0 / 229442532802560000.0,
    -1.0 / 38775788043632640000.0,
    1.0 / 7600054456551997440000.0,
    -1.0 / 1710012252724199424000000.0,
    1.0 / 437763136697395052544000000.0,
    -1.0 / 126514546433452,
    0.0,
    0.0,
    0.0,
  };
  f64 r = C[15];
  for ( int k = 14; k >= 0; --k ) r = hw::fmadd_sd(r, z, C[k]);
  return r;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
j1_small(f64 x) noexcept
{
  const f64 z = 0.25 * x * x;
  constexpr f64 C[16] = {
    1.0,
    -1.0 / 2.0,
    1.0 / 12.0,
    -1.0 / 144.0,
    1.0 / 2880.0,
    -1.0 / 86400.0,
    1.0 / 3628800.0,
    -1.0 / 203212800.0,
    1.0 / 14631321600.0,
    -1.0 / 1316818944000.0,
    1.0 / 144850083840000.0,
    -1.0 / 19120211066880000.0,
    1.0 / 2982752926433280000.0,
    -1.0 / 542860532610656256.0,
    0.0,
    0.0,
  };
  f64 r = C[13];
  for ( int k = 12; k >= 0; --k ) r = hw::fmadd_sd(r, z, C[k]);
  return 0.5 * x * r;
}

[[gnu::flatten]] inline constexpr f64
hankel_PQ_j0(f64 x, f64 *P, f64 *Q) noexcept
{
  const f64 ax = manip::fabs(x);
  const f64 r = 1.0 / ax;
  const f64 u = r * r;
  *P = 1.0 + u * (-0.0703125 + u * (0.11215718746185303 + u * (-0.5725276993751526)));
  *Q = r * (-0.125 + u * (0.0732421875 + u * (-0.1442108154296875)));
  return ax;
}

[[gnu::flatten]] inline constexpr f64
hankel_PQ_j1(f64 x, f64 *P, f64 *Q) noexcept
{
  const f64 ax = manip::fabs(x);
  const f64 r = 1.0 / ax;
  const f64 u = r * r;
  *P = 1.0 + u * (0.1171875 + u * (-0.14420127868652344 + u * (0.6765925884246826)));
  *Q = r * (0.375 + u * (-0.1025390625 + u * (0.1716690063476563)));
  return ax;
}

};      // namespace __bessel

[[nodiscard, gnu::flatten]] inline constexpr f64
j0_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return 0.0;
  const f64 ax = manip::fabs(x);
  if ( ax < 8.0 ) return __bessel::j0_small(x);
  f64 P, Q;
  __bessel::hankel_PQ_j0(x, &P, &Q);
  const f64 beta = ax - __bessel::pi_quarter;
  f64 sb, cb;
  trig_ns::sincos_f64(beta, &sb, &cb);
  const f64 amp = sqrt_ns::sqrt<f64>(__bessel::two_over_pi / ax);
  return amp * (P * cb - Q * sb);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
j1_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return 0.0;
  const f64 ax = manip::fabs(x);
  if ( ax < 8.0 ) return __bessel::j1_small(x);
  f64 P, Q;
  __bessel::hankel_PQ_j1(x, &P, &Q);
  const f64 beta = ax - 3.0 * __bessel::pi_quarter;
  f64 sb, cb;
  trig_ns::sincos_f64(beta, &sb, &cb);
  const f64 amp = sqrt_ns::sqrt<f64>(__bessel::two_over_pi / ax);
  const f64 r = amp * (P * cb - Q * sb);
  return manip::signbit(x) ? -r : r;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
y0_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x == 0.0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( x < 0.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( ieee::is_inf(x) ) [[unlikely]]
    return 0.0;
  if ( x < 8.0 ) {
    constexpr f64 gamma_e = 0.5772156649015329;
    const f64 z = 0.25 * x * x;
    f64 sum = 0.0;
    f64 z_pow = 1.0;
    f64 fact_sq = 1.0;
    f64 hk = 0.0;
    for ( int k = 1; k < 25; ++k ) {
      z_pow *= z;
      fact_sq *= f64(k) * f64(k);
      hk += 1.0 / f64(k);
      const f64 sign = (k & 1) ? 1.0 : -1.0;
      sum += sign * hk * z_pow / fact_sq;
    }
    const f64 j0v = __bessel::j0_small(x);
    return (__bessel::two_over_pi) * ((log_ns::log_f64(0.5 * x) + gamma_e) * j0v + sum);
  }
  f64 P, Q;
  __bessel::hankel_PQ_j0(x, &P, &Q);
  const f64 beta = x - __bessel::pi_quarter;
  f64 sb, cb;
  trig_ns::sincos_f64(beta, &sb, &cb);
  const f64 amp = sqrt_ns::sqrt<f64>(__bessel::two_over_pi / x);
  return amp * (P * sb + Q * cb);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
y1_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x == 0.0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( x < 0.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( ieee::is_inf(x) ) [[unlikely]]
    return 0.0;
  if ( x < 8.0 ) {
    constexpr f64 gamma_e = 0.5772156649015329;
    const f64 z = 0.25 * x * x;
    f64 sum = 0.0;
    f64 z_pow_neg = 1.0;
    f64 fact_prod = 1.0;
    f64 hk = 0.0;
    f64 hk1 = 1.0;
    for ( int k = 0; k < 25; ++k ) {
      sum += (hk + hk1) * z_pow_neg / fact_prod;
      z_pow_neg *= -z;
      fact_prod *= f64(k + 1) * f64(k + 2);
      hk = hk1;
      hk1 += 1.0 / f64(k + 2);
    }
    const f64 j1v = __bessel::j1_small(x);
    return (__bessel::two_over_pi) * (-1.0 / x + (log_ns::log_f64(0.5 * x) + gamma_e) * j1v - 0.25 * x * sum);
  }
  f64 P, Q;
  __bessel::hankel_PQ_j1(x, &P, &Q);
  const f64 beta = x - 3.0 * __bessel::pi_quarter;
  f64 sb, cb;
  trig_ns::sincos_f64(beta, &sb, &cb);
  const f64 amp = sqrt_ns::sqrt<f64>(__bessel::two_over_pi / x);
  return amp * (P * sb + Q * cb);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
j0(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(j0_f64(f64(x)));
  else
    return F(j0_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
j1(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(j1_f64(f64(x)));
  else
    return F(j1_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
y0(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(y0_f64(f64(x)));
  else
    return F(y0_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
y1(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(y1_f64(f64(x)));
  else
    return F(y1_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
erf_f32(f32 x) noexcept
{
  return f32(erf_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
erfc_f32(f32 x) noexcept
{
  return f32(erfc_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
tgamma_f32(f32 x) noexcept
{
  return f32(tgamma_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
lgamma_f32(f32 x) noexcept
{
  return f32(lgamma_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
erf(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(erf_f32(f32(x)));
  else
    return F(erf_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
erfc(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(erfc_f32(f32(x)));
  else
    return F(erfc_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
tgamma(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(tgamma_f32(f32(x)));
  else
    return F(tgamma_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lgamma(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(lgamma_f32(f32(x)));
  else
    return F(lgamma_f64(f64(x)));
}

};      // namespace special_ns
};      // namespace mkbits
};      // namespace math
};      // namespace micron

#pragma GCC pop_options
