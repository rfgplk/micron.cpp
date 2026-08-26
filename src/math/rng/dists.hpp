//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../generic.hpp"
#include "../log.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "dist.hpp"
#include "engines.hpp"

namespace micron
{
namespace math
{
namespace rng
{
namespace dists
{

namespace __impl
{

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
lgamma(F z) noexcept
{
  return mk::special::lgamma<F>(z);
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
tgamma(F z) noexcept
{
  return mk::special::tgamma<F>(z);
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
lbeta(F a, F b) noexcept
{
  return lgamma<F>(a) + lgamma<F>(b) - lgamma<F>(a + b);
}

template<ieee754_floating F>
[[nodiscard]] inline F
__gammap(F a, F x) noexcept
{
  if ( x <= F(0) || a <= F(0) ) return F(0);
  constexpr F tolerance = sizeof(F) == 4 ? F(2e-6) : F(2e-15);
  constexpr F tiny = sizeof(F) == 4 ? F(1e-30) : F(1e-300);
  const F log_pre = a * mk::log_ns::log<F>(x) - x - lgamma<F>(a);
  if ( x < a + F(1) ) {
    // series
    F ap = a;
    F del = F(1) / a;
    F sum = del;
    for ( int k = 0; k < 200; ++k ) {
      ap = ap + F(1);
      del = del * x / ap;
      sum = sum + del;
      if ( mk::manip::fabs<F>(del) < mk::manip::fabs<F>(sum) * tolerance ) break;
    }
    return sum * mk::exp_ns::exp<F>(log_pre);
  }
  // continued fraction (Lentz)
  F b = x + F(1) - a;
  F c = F(1) / tiny;
  F d = F(1) / b;
  F h = d;
  for ( int k = 1; k <= 200; ++k ) {
    const F an = -F(k) * (F(k) - a);
    b = b + F(2);
    d = an * d + b;
    if ( mk::manip::fabs<F>(d) < tiny ) d = tiny;
    c = b + an / c;
    if ( mk::manip::fabs<F>(c) < tiny ) c = tiny;
    d = F(1) / d;
    const F del = d * c;
    h = h * del;
    if ( mk::manip::fabs<F>(del - F(1)) < tolerance ) break;
  }
  return F(1) - h * mk::exp_ns::exp<F>(log_pre);
}

template<ieee754_floating F>
[[nodiscard]] inline F
__gammaq(F a, F x) noexcept
{
  return F(1) - __gammap<F>(a, x);
}

template<ieee754_floating F>
[[nodiscard]] inline F
__betainc(F a, F b, F x) noexcept
{
  if ( x <= F(0) ) return F(0);
  if ( x >= F(1) ) return F(1);
  const bool swap = x > (a + F(1)) / (a + b + F(2));
  const F xs = swap ? F(1) - x : x;
  const F as = swap ? b : a;
  const F bs = swap ? a : b;
  const F log_pre = as * mk::log_ns::log<F>(xs) + bs * mk::log_ns::log<F>(F(1) - xs) - lbeta<F>(as, bs);
  constexpr F tolerance = sizeof(F) == 4 ? F(2e-6) : F(2e-15);
  constexpr F tiny = sizeof(F) == 4 ? F(1e-30) : F(1e-300);
  F qab = as + bs;
  F qap = as + F(1);
  F qam = as - F(1);
  F c = F(1);
  F d = F(1) - qab * xs / qap;
  if ( mk::manip::fabs<F>(d) < tiny ) d = tiny;
  d = F(1) / d;
  F h = d;
  for ( int m = 1; m <= 200; ++m ) {
    const F m_f = F(m);
    F aa = m_f * (bs - m_f) * xs / ((qam + F(2) * m_f) * (as + F(2) * m_f));
    d = F(1) + aa * d;
    if ( mk::manip::fabs<F>(d) < tiny ) d = tiny;
    c = F(1) + aa / c;
    if ( mk::manip::fabs<F>(c) < tiny ) c = tiny;
    d = F(1) / d;
    h = h * d * c;
    aa = -(as + m_f) * (qab + m_f) * xs / ((as + F(2) * m_f) * (qap + F(2) * m_f));
    d = F(1) + aa * d;
    if ( mk::manip::fabs<F>(d) < tiny ) d = tiny;
    c = F(1) + aa / c;
    if ( mk::manip::fabs<F>(c) < tiny ) c = tiny;
    d = F(1) / d;
    const F del = d * c;
    h = h * del;
    if ( mk::manip::fabs<F>(del - F(1)) < tolerance ) break;
  }
  const F r = mk::exp_ns::exp<F>(log_pre) * h / as;
  return swap ? F(1) - r : r;
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
norm_ppf(F p) noexcept
{
  if ( p <= F(0) ) return -math::ieee::inf_v<F>(0);
  if ( p >= F(1) ) return math::ieee::inf_v<F>(0);
  constexpr F a[6] = { F(-3.969683028665376e+01), F(2.209460984245205e+02),  F(-2.759285104469687e+02),
                       F(1.383577518672690e+02),  F(-3.066479806614716e+01), F(2.506628277459239e+00) };
  constexpr F b[5] = { F(-5.447609879822406e+01), F(1.615858368580409e+02), F(-1.556989798598866e+02), F(6.680131188771972e+01),
                       F(-1.328068155288572e+01) };
  constexpr F c[6] = { F(-7.784894002430293e-03), F(-3.223964580411365e-01), F(-2.400758277161838e+00),
                       F(-2.549732539343734e+00), F(4.374664141464968e+00),  F(2.938163982698783e+00) };
  constexpr F d[4] = { F(7.784695709041462e-03), F(3.224671290700398e-01), F(2.445134137142996e+00), F(3.754408661907416e+00) };
  constexpr F p_low = F(0.02425);
  constexpr F p_high = F(1) - p_low;
  F q, r, x;
  if ( p < p_low ) {
    q = mk::pow_ns::sqrt<F>(F(-2) * mk::log_ns::log<F>(p));
    x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + F(1));
  } else if ( p <= p_high ) {
    q = p - F(0.5);
    r = q * q;
    x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q
        / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + F(1));
  } else {
    q = mk::pow_ns::sqrt<F>(F(-2) * mk::log_ns::log<F>(F(1) - p));
    x = -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + F(1));
  }
  return x;
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
norm_cdf(F x) noexcept
{
  constexpr F inv_sqrt2 = F(0.7071067811865475);
  return F(0.5) * mk::special::erfc<F>(-x * inv_sqrt2);
}

};      // namespace __impl

template<typename I = i64> class poisson_dist
{
  dist::__impl::__poisson_cache __cache;
  f64 __cdf[32];
  f64 __mass31;
  bool __table;

  template<rng_concept Rng>
  [[nodiscard, gnu::always_inline]] I
  __table_draw(Rng &g) const noexcept
  {
    const f64 u = dist::uniform_real<f64>(g);
    I k = I(u > __cdf[0]) + I(u > __cdf[1]) + I(u > __cdf[2]) + I(u > __cdf[3]) + I(u > __cdf[4]) + I(u > __cdf[5]) + I(u > __cdf[6])
          + I(u > __cdf[7]) + I(u > __cdf[8]) + I(u > __cdf[9]) + I(u > __cdf[10]) + I(u > __cdf[11]) + I(u > __cdf[12]) + I(u > __cdf[13])
          + I(u > __cdf[14]) + I(u > __cdf[15]) + I(u > __cdf[16]) + I(u > __cdf[17]) + I(u > __cdf[18]) + I(u > __cdf[19])
          + I(u > __cdf[20]) + I(u > __cdf[21]) + I(u > __cdf[22]) + I(u > __cdf[23]) + I(u > __cdf[24]) + I(u > __cdf[25])
          + I(u > __cdf[26]) + I(u > __cdf[27]) + I(u > __cdf[28]) + I(u > __cdf[29]) + I(u > __cdf[30]) + I(u > __cdf[31]);
    if ( k != I(32) ) return k;

    f64 mass = __mass31;
    f64 cumulative = __cdf[31];
    k = I(31);
    do {
      ++k;
      mass *= __cache.lambda / f64(k);
      cumulative += mass;
    } while ( u > cumulative );
    return k;
  }

public:
  using value_type = I;

  constexpr explicit poisson_dist(f64 lambda) noexcept : __cache(lambda), __cdf{}, __mass31(0.0), __table(__cache.valid && __cache.small)
  {
    if ( !__table ) return;
    f64 mass = __cache.cutoff;
    f64 cumulative = mass;
    for ( usize k = 0; k < 32; ++k ) {
      __cdf[k] = cumulative;
      if ( k != 31 ) {
        mass *= __cache.lambda / f64(k + 1);
        cumulative += mass;
      }
    }
    __mass31 = mass;
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  operator()(Rng &g) const noexcept
  {
    return __table ? __table_draw(g) : dist::__impl::__poisson_cached<I>(g, __cache);
  }

  [[nodiscard]] constexpr f64
  mean() const noexcept
  {
    return __cache.valid ? __cache.lambda : 0.0;
  }

  [[nodiscard]] constexpr f64
  variance() const noexcept
  {
    return mean();
  }
};

template<ieee754_floating F = f64> class __gammadist
{
  F __alpha;
  F __theta;
  F __d;
  F __c;
  F __inv_alpha;
  F __log_norm;
  bool __boost;

public:
  using value_type = F;

  constexpr __gammadist() noexcept : __gammadist(F(1), F(1)) { }

  constexpr __gammadist(F shape, F scale = F(1)) noexcept
      : __alpha(shape), __theta(scale), __d((shape < F(1) ? shape + F(1) : shape) - F(1) / F(3)),
        __c(__d > F(0) ? F(1) / mk::pow_ns::sqrt<F>(F(9) * __d) : F(0)), __inv_alpha(shape > F(0) ? F(1) / shape : F(0)),
        __log_norm(shape > F(0) && scale > F(0) ? shape * mk::log_ns::log<F>(scale) + __impl::lgamma<F>(shape) : F(0)),
        __boost(shape < F(1))
  {
  }

  [[nodiscard]] constexpr F
  shape() const noexcept
  {
    return __alpha;
  }

  [[nodiscard]] constexpr F
  scale() const noexcept
  {
    return __theta;
  }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__theta) || !(__alpha > F(0)) || !(__theta > F(0)) )
      return math::ieee::qnan_v<F>();
    for ( ;; ) {
      F x, v;
      do {
        x = dist::normal<F>(g);
        v = F(1) + __c * x;
      } while ( v <= F(0) );
      v = v * v * v;
      const F x2 = x * x;
      const F u = dist::uniform_open_real<F>(g);
      if ( u < F(1) - F(0.0331) * x2 * x2 || mk::log_ns::log<F>(u) < F(0.5) * x2 + __d * (F(1) - v + mk::log_ns::log<F>(v)) ) {
        F sample = __theta * __d * v;
        if ( __boost ) sample *= mk::pow_ns::pow<F>(dist::uniform_open_real<F>(g), __inv_alpha);
        return sample;
      }
    }
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__theta) || !(__alpha > F(0)) || !(__theta > F(0)) )
      return math::ieee::qnan_v<F>();
    if ( x < F(0) ) return F(0);
    if ( x == F(0) ) {
      if ( __alpha < F(1) ) return math::ieee::inf_v<F>(0);
      if ( __alpha == F(1) ) return F(1) / __theta;
      return F(0);
    }
    const F log_p = (__alpha - F(1)) * mk::log_ns::log<F>(x) - x / __theta - __log_norm;
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__theta) || !(__alpha > F(0)) || !(__theta > F(0)) )
      return math::ieee::qnan_v<F>();
    if ( x <= F(0) ) return F(0);
    return __impl::__gammap<F>(__alpha, x / __theta);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __alpha * __theta;
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return __alpha * __theta * __theta;
  }
};

template<ieee754_floating F = f64> class __betadist
{
  F __alpha, __beta;
  F __log_beta;
  __gammadist<F> __ga;
  __gammadist<F> __gb;

public:
  using value_type = F;

  constexpr __betadist(F alpha, F beta) noexcept
      : __alpha(alpha), __beta(beta), __log_beta(alpha > F(0) && beta > F(0) ? __impl::lbeta<F>(alpha, beta) : F(0)), __ga(alpha, F(1)),
        __gb(beta, F(1))
  {
  }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__beta) || !(__alpha > F(0)) || !(__beta > F(0)) )
      return math::ieee::qnan_v<F>();
    const F x = __ga(g);
    const F y = __gb(g);
    return x / (x + y);
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__beta) || !(__alpha > F(0)) || !(__beta > F(0)) )
      return math::ieee::qnan_v<F>();
    if ( x < F(0) || x > F(1) ) return F(0);
    if ( x == F(0) ) {
      if ( __alpha < F(1) ) return math::ieee::inf_v<F>(0);
      if ( __alpha == F(1) ) return __beta;
      return F(0);
    }
    if ( x == F(1) ) {
      if ( __beta < F(1) ) return math::ieee::inf_v<F>(0);
      if ( __beta == F(1) ) return __alpha;
      return F(0);
    }
    const F log_p = (__alpha - F(1)) * mk::log_ns::log<F>(x) + (__beta - F(1)) * mk::log_ns::log<F>(F(1) - x) - __log_beta;
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( !math::ieee::is_finite(__alpha) || !math::ieee::is_finite(__beta) || !(__alpha > F(0)) || !(__beta > F(0)) )
      return math::ieee::qnan_v<F>();
    return __impl::__betainc<F>(__alpha, __beta, x);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __alpha / (__alpha + __beta);
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    const F s = __alpha + __beta;
    return (__alpha * __beta) / (s * s * (s + F(1)));
  }
};

template<ieee754_floating F = f64> class chi2_dist
{
  F __kf;
  __gammadist<F> __gamma;

public:
  using value_type = F;

  constexpr explicit chi2_dist(F k) noexcept : __kf(k), __gamma(k / F(2), F(2)) { }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    return __gamma(g);
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    const F half_k = __kf / F(2);
    const F log_p = (half_k - F(1)) * mk::log_ns::log<F>(x) - x / F(2) - half_k * mk::log_ns::log<F>(F(2)) - __impl::lgamma<F>(half_k);
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    return __impl::__gammap<F>(__kf / F(2), x / F(2));
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __kf;
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return F(2) * __kf;
  }
};

template<ieee754_floating F = f64> class student_t_dist
{
  F __nu;
  F __inv_nu;
  chi2_dist<F> __chi;

public:
  using value_type = F;

  constexpr explicit student_t_dist(F nu) noexcept : __nu(nu), __inv_nu(nu > F(0) ? F(1) / nu : F(0)), __chi(nu) { }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    if ( __nu <= F(0) ) return math::ieee::qnan_v<F>();
    const F z = dist::normal<F>(g);
    const F y = __chi(g);
    return z / mk::pow_ns::sqrt<F>(y * __inv_nu);
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    constexpr F log_pi = F(1.1447298858494002);
    const F log_p = __impl::lgamma<F>((__nu + F(1)) / F(2)) - __impl::lgamma<F>(__nu / F(2)) - F(0.5) * (mk::log_ns::log<F>(__nu) + log_pi)
                    - ((__nu + F(1)) / F(2)) * mk::log_ns::log1p<F>(x * x / __nu);
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    const F z = __nu / (__nu + x * x);
    const F half = F(0.5) * __impl::__betainc<F>(__nu / F(2), F(0.5), z);
    return x >= F(0) ? F(1) - half : half;
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __nu > F(1) ? F(0) : math::ieee::qnan_v<F>();
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    if ( __nu > F(2) ) return __nu / (__nu - F(2));
    if ( __nu > F(1) ) return math::ieee::inf_v<F>(0);
    return math::ieee::qnan_v<F>();
  }
};

template<ieee754_floating F = f64> class f_dist
{
  F __d1, __d2;
  F __inv_d1, __inv_d2;
  chi2_dist<F> __c1, __c2;

public:
  using value_type = F;

  constexpr f_dist(F d1, F d2) noexcept
      : __d1(d1), __d2(d2), __inv_d1(d1 > F(0) ? F(1) / d1 : F(0)), __inv_d2(d2 > F(0) ? F(1) / d2 : F(0)), __c1(d1), __c2(d2)
  {
  }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    if ( __d1 <= F(0) || __d2 <= F(0) ) return math::ieee::qnan_v<F>();
    const F u1 = __c1(g) * __inv_d1;
    const F u2 = __c2(g) * __inv_d2;
    return u1 / u2;
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    const F r = __d1 / __d2;
    const F log_p = F(0.5) * __d1 * mk::log_ns::log<F>(__d1 * x) + F(0.5) * __d2 * mk::log_ns::log<F>(__d2)
                    - F(0.5) * (__d1 + __d2) * mk::log_ns::log<F>(__d1 * x + __d2) - __impl::lbeta<F>(__d1 / F(2), __d2 / F(2))
                    - mk::log_ns::log<F>(x);
    (void)r;
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    const F z = (__d1 * x) / (__d1 * x + __d2);
    return __impl::__betainc<F>(__d1 / F(2), __d2 / F(2), z);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __d2 > F(2) ? __d2 / (__d2 - F(2)) : math::ieee::qnan_v<F>();
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    if ( __d2 <= F(4) ) return math::ieee::inf_v<F>(0);
    const F num = F(2) * __d2 * __d2 * (__d1 + __d2 - F(2));
    const F den = __d1 * (__d2 - F(2)) * (__d2 - F(2)) * (__d2 - F(4));
    return num / den;
  }
};

template<ieee754_floating F = f64> class lognormal_dist
{
  F __mu, __sigma;

public:
  using value_type = F;

  constexpr lognormal_dist(F mu = F(0), F sigma = F(1)) noexcept : __mu(mu), __sigma(sigma) { }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    return mk::exp_ns::exp<F>(dist::normal<F>(g, __mu, __sigma));
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    constexpr F log_2pi_half = F(0.9189385332046727);
    const F lx = mk::log_ns::log<F>(x);
    const F z = (lx - __mu) / __sigma;
    return mk::exp_ns::exp<F>(-F(0.5) * z * z - lx - mk::log_ns::log<F>(__sigma) - log_2pi_half);
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    return __impl::norm_cdf<F>((mk::log_ns::log<F>(x) - __mu) / __sigma);
  }

  [[nodiscard]] constexpr F
  ppf(F p) const noexcept
  {
    return mk::exp_ns::exp<F>(__mu + __sigma * __impl::norm_ppf<F>(p));
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return mk::exp_ns::exp<F>(__mu + F(0.5) * __sigma * __sigma);
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    const F s2 = __sigma * __sigma;
    return (mk::exp_ns::exp<F>(s2) - F(1)) * mk::exp_ns::exp<F>(F(2) * __mu + s2);
  }
};

template<ieee754_floating F = f64> class weibull_dist
{
  F __lambdaf, __kf;
  F __inv_k;

public:
  using value_type = F;

  constexpr weibull_dist(F lambda, F k) noexcept : __lambdaf(lambda), __kf(k), __inv_k(k > F(0) ? F(1) / k : F(0)) { }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    if ( __lambdaf <= F(0) || __kf <= F(0) ) return math::ieee::qnan_v<F>();
    return __lambdaf * mk::pow_ns::pow<F>(-mk::log_ns::log<F>(dist::uniform_open_real<F>(g)), __inv_k);
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    if ( x < F(0) ) return F(0);
    if ( x == F(0) ) return __kf == F(1) ? F(1) / __lambdaf : (__kf < F(1) ? math::ieee::inf_v<F>(0) : F(0));
    const F r = x / __lambdaf;
    return (__kf / __lambdaf) * mk::pow_ns::pow<F>(r, __kf - F(1)) * mk::exp_ns::exp<F>(-mk::pow_ns::pow<F>(r, __kf));
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    if ( x <= F(0) ) return F(0);
    return F(1) - mk::exp_ns::exp<F>(-mk::pow_ns::pow<F>(x / __lambdaf, __kf));
  }

  [[nodiscard]] constexpr F
  ppf(F p) const noexcept
  {
    if ( p <= F(0) ) return F(0);
    if ( p >= F(1) ) return math::ieee::inf_v<F>(0);
    return __lambdaf * mk::pow_ns::pow<F>(-mk::log_ns::log<F>(F(1) - p), __inv_k);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __lambdaf * __impl::tgamma<F>(F(1) + __inv_k);
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    const F m = __impl::tgamma<F>(F(1) + __inv_k);
    const F s = __impl::tgamma<F>(F(1) + F(2) * __inv_k);
    return __lambdaf * __lambdaf * (s - m * m);
  }
};

template<ieee754_floating F = f64> class cauchy_dist
{
  F __x0, __gamma;

public:
  using value_type = F;

  constexpr cauchy_dist(F x0 = F(0), F gamma = F(1)) noexcept : __x0(x0), __gamma(gamma) { }

  template<rng_concept Rng>
  [[nodiscard]] F
  operator()(Rng &g) const noexcept
  {
    constexpr F pi = F(3.141592653589793);
    if ( __gamma <= F(0) ) return __gamma == F(0) ? __x0 : math::ieee::qnan_v<F>();
    return __x0 + __gamma * mk::trig::tan<F>(pi * (dist::uniform_open_real<F>(g) - F(0.5)));
  }

  [[nodiscard]] constexpr F
  pdf(F x) const noexcept
  {
    constexpr F inv_pi = F(0.3183098861837907);
    const F z = (x - __x0) / __gamma;
    return inv_pi / (__gamma * (F(1) + z * z));
  }

  [[nodiscard]] constexpr F
  cdf(F x) const noexcept
  {
    constexpr F inv_pi = F(0.3183098861837907);
    return F(0.5) + inv_pi * mk::trig::atan<F>((x - __x0) / __gamma);
  }

  [[nodiscard]] constexpr F
  ppf(F p) const noexcept
  {
    constexpr F pi = F(3.141592653589793);
    return __x0 + __gamma * mk::trig::tan<F>(pi * (p - F(0.5)));
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return math::ieee::qnan_v<F>();
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return math::ieee::qnan_v<F>();
  }
};

template<typename I = i64, ieee754_floating F = f64> class geometric_dist
{
  F __p;
  F __inv_log_q;
  bool __half;

public:
  using value_type = I;

  constexpr explicit geometric_dist(F p) noexcept
      : __p(p), __inv_log_q(p > F(0) && p < F(1) ? F(1) / mk::log_ns::log1p<F>(-p) : F(0)), __half(p == F(0.5))
  {
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  operator()(Rng &g) const noexcept
  {
    if ( __p >= F(1) ) return I(0);
    if ( __p <= F(0) ) return numeric_limits<I>::max();
    if ( __half ) {
      I count = 0;
      for ( ;; ) {
        const u64 bits = rng::__impl::next64(g);
        if ( bits != 0 ) return count + I(__builtin_ctzll(bits));
        count += I(64);
      }
    }
    return I(mk::round_ns::floor<F>(mk::log_ns::log<F>(dist::uniform_open_real<F>(g)) * __inv_log_q));
  }

  [[nodiscard]] constexpr F
  pmf(I k) const noexcept
  {
    if ( k < I(0) ) return F(0);
    return __p * mk::pow_ns::pow<F>(F(1) - __p, F(k));
  }

  [[nodiscard]] constexpr F
  cdf(I k) const noexcept
  {
    if ( k < I(0) ) return F(0);
    return F(1) - mk::pow_ns::pow<F>(F(1) - __p, F(k + I(1)));
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return (F(1) - __p) / __p;
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return (F(1) - __p) / (__p * __p);
  }
};

template<typename I = i64, ieee754_floating F = f64> class binomial_dist
{
  I __n;
  F __p;
  I __m;
  F __r, __q, __odds, __nr, __npq, __b, __a, __c, __alpha, __vr, __urvr, __qn;
  F __cdf[16];
  bool __flip;
  bool __inversion;
  bool __direct;
  bool __half;

  template<rng_concept Rng>
  [[nodiscard]] I
  __direct_draw(Rng &g) const noexcept
  {
    const F u = dist::uniform_real<F>(g);
    return I(u > __cdf[0]) + I(u > __cdf[1]) + I(u > __cdf[2]) + I(u > __cdf[3]) + I(u > __cdf[4]) + I(u > __cdf[5]) + I(u > __cdf[6])
           + I(u > __cdf[7]) + I(u > __cdf[8]) + I(u > __cdf[9]) + I(u > __cdf[10]) + I(u > __cdf[11]) + I(u > __cdf[12]) + I(u > __cdf[13])
           + I(u > __cdf[14]) + I(u > __cdf[15]);
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  __half_draw(Rng &g) const noexcept
  {
    I hits = 0;
    I left = __n;
    while ( left >= I(64) ) {
      hits += I(__builtin_popcountll(rng::__impl::next64(g)));
      left -= I(64);
    }
    if ( left != I(0) ) {
      const u32 bits = u32(left);
      const u64 mask = ~u64(0) >> (64u - bits);
      hits += I(__builtin_popcountll(rng::__impl::next64(g) & mask));
    }
    return hits;
  }

  [[nodiscard]] static constexpr F
  __fc(I k) noexcept
  {
    constexpr F table[10]
        = { F(0.08106146679532726), F(0.04134069595540929), F(0.02767792568499834), F(0.02079067210382509),  F(0.01664469118982119),
            F(0.01387612882307075), F(0.01189670994589177), F(0.01041126526197209), F(0.009255462182712733), F(0.008330563433362871) };
    if ( k < I(10) ) return table[usize(k)];
    const F inv = F(1) / F(k + I(1));
    const F inv2 = inv * inv;
    return (F(1) / F(12) - (F(1) / F(360) - (F(1) / F(1260)) * inv2) * inv2) * inv;
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  __invert(Rng &g) const noexcept
  {
    const F ratio = __r / __q;
    const F scaled = F(__n + I(1)) * ratio;
    F u = dist::uniform_real<F>(g);
    F mass = __qn;
    I x = 0;
    while ( u > mass && x < __n ) {
      u -= mass;
      ++x;
      mass *= scaled / F(x) - ratio;
    }
    return x;
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  __btrd(Rng &g) const noexcept
  {
    for ( ;; ) {
      F u;
      F v = dist::uniform_open_real<F>(g);
      if ( v <= __urvr ) {
        u = v / __vr - F(0.43);
        return I(mk::round_ns::floor<F>((F(2) * __a / (F(0.5) - mk::manip::fabs<F>(u)) + __b) * u + __c));
      }

      if ( v >= __vr ) {
        u = dist::uniform_open_real<F>(g) - F(0.5);
      } else {
        u = v / __vr - F(0.93);
        u = (u < F(0) ? F(-0.5) : F(0.5)) - u;
        v = dist::uniform_open_real<F>(g) * __vr;
      }

      const F us = F(0.5) - mk::manip::fabs<F>(u);
      const I k = I(mk::round_ns::floor<F>((F(2) * __a / us + __b) * u + __c));
      if ( k < I(0) || k > __n ) continue;
      v *= __alpha / (__a / (us * us) + __b);
      const I delta_i = k > __m ? k - __m : __m - k;
      const F delta = F(delta_i);
      if ( delta_i <= I(15) ) {
        F f = F(1);
        if ( __m < k ) {
          I i = __m;
          do {
            ++i;
            f *= __nr / F(i) - __odds;
          } while ( i != k );
        } else if ( __m > k ) {
          I i = k;
          do {
            ++i;
            v *= __nr / F(i) - __odds;
          } while ( i != __m );
        }
        if ( v <= f ) return k;
        continue;
      }

      const F log_v = mk::log_ns::log<F>(v);
      const F rho = (delta / __npq) * (((delta / F(3) + F(0.625)) * delta + F(1) / F(6)) / __npq + F(0.5));
      const F t = -delta * delta / (F(2) * __npq);
      if ( log_v < t - rho ) return k;
      if ( log_v > t + rho ) continue;

      const I nm_i = __n - __m + I(1);
      const I nk_i = __n - k + I(1);
      const F nm = F(nm_i);
      const F nk = F(nk_i);
      const F h = (F(__m) + F(0.5)) * mk::log_ns::log<F>(F(__m + I(1)) / (__odds * nm)) + __fc(__m) + __fc(__n - __m);
      const F bound = h + F(__n + I(1)) * mk::log_ns::log<F>(nm / nk) + (F(k) + F(0.5)) * mk::log_ns::log<F>(nk * __odds / (F(k) + F(1)))
                      - __fc(k) - __fc(__n - k);
      if ( log_v <= bound ) return k;
    }
  }

public:
  using value_type = I;

  constexpr binomial_dist(I n, F p) noexcept
      : __n(n), __p(p), __m(0), __r(p > F(0.5) ? F(1) - p : p), __q(F(1) - __r), __odds(0), __nr(0), __npq(0), __b(0), __a(0), __c(0),
        __alpha(0), __vr(0), __urvr(0), __qn(0), __cdf{}, __flip(p > F(0.5)), __inversion(n <= I(0) || __r <= F(0) || F(n) * __r < F(10)),
        __direct(n > I(0) && n <= I(16)), __half(p == F(0.5))
  {
    if ( n <= I(0) || !math::ieee::is_finite(__r) || !(__r > F(0)) || !(__r < F(1)) ) return;
    __odds = __r / __q;
    __m = I(F(n + I(1)) * __r);
    if ( __direct ) {
      F mass = mk::exp_ns::exp<F>(F(n) * mk::log_ns::log1p<F>(-__r));
      F cumulative = mass;
      for ( usize i = 0; i < 16; ++i ) {
        if ( I(i) >= n ) {
          __cdf[i] = F(1);
          continue;
        }
        __cdf[i] = cumulative;
        mass *= F(n - I(i)) * __odds / F(i + 1);
        cumulative += mass;
      }
      return;
    }
    if ( __inversion ) {
      __qn = mk::exp_ns::exp<F>(F(n) * mk::log_ns::log1p<F>(-__r));
      return;
    }
    __nr = F(n + I(1)) * __odds;
    __npq = F(n) * __r * __q;
    const F root = mk::pow_ns::sqrt<F>(__npq);
    __b = F(1.15) + F(2.53) * root;
    __a = F(-0.0873) + F(0.0248) * __b + F(0.01) * __r;
    __c = F(n) * __r + F(0.5);
    __alpha = (F(2.83) + F(5.1) / __b) * root;
    __vr = F(0.92) - F(4.2) / __b;
    __urvr = F(0.86) * __vr;
  }

  template<rng_concept Rng>
  [[nodiscard]] I
  operator()(Rng &g) const noexcept
  {
    if ( __n <= I(0) || !math::ieee::is_finite(__p) || !(__p > F(0)) ) return I(0);
    if ( __p >= F(1) ) return __n;
    if ( __half ) return __half_draw(g);
    const I k = __direct ? __direct_draw(g) : (__inversion ? __invert(g) : __btrd(g));
    return __flip ? __n - k : k;
  }

  [[nodiscard]] constexpr F
  pmf(I k) const noexcept
  {
    if ( k < I(0) || k > __n ) return F(0);
    if ( __p <= F(0) ) return k == I(0) ? F(1) : F(0);
    if ( __p >= F(1) ) return k == __n ? F(1) : F(0);
    const F log_pmf = __impl::lgamma<F>(F(__n + I(1))) - __impl::lgamma<F>(F(k + I(1))) - __impl::lgamma<F>(F(__n - k + I(1)))
                      + F(k) * mk::log_ns::log<F>(__p) + F(__n - k) * mk::log_ns::log1p<F>(-__p);
    return mk::exp_ns::exp<F>(log_pmf);
  }

  [[nodiscard]] constexpr F
  cdf(I k) const noexcept
  {
    if ( k < I(0) ) return F(0);
    if ( k >= __n ) return F(1);
    if ( __p <= F(0) ) return F(1);
    if ( __p >= F(1) ) return F(0);
    return __impl::__betainc<F>(F(__n - k), F(k + I(1)), F(1) - __p);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return F(__n) * __p;
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return F(__n) * __p * (F(1) - __p);
  }
};

template<typename I = i64, ieee754_floating F = f64> class negative_binomial_dist
{
  F __r;
  F __p;
  __gammadist<F> __gamma;

public:
  using value_type = I;

  constexpr negative_binomial_dist(F r, F p) noexcept : __r(r), __p(p), __gamma(r, p > F(0) ? (F(1) - p) / p : F(0)) { }

  template<rng_concept Rng>
  [[nodiscard]] I
  operator()(Rng &g) const noexcept
  {
    if ( __r <= F(0) || __p <= F(0) || __p > F(1) ) return I(0);
    if ( __p == F(1) ) return I(0);
    const F lam = __gamma(g);
    return dist::poisson<I>(g, f64(lam));
  }

  [[nodiscard]] constexpr F
  pmf(I k) const noexcept
  {
    if ( k < I(0) ) return F(0);
    const F log_pmf = __impl::lgamma<F>(F(k) + __r) - __impl::lgamma<F>(F(k + I(1))) - __impl::lgamma<F>(__r)
                      + __r * mk::log_ns::log<F>(__p) + F(k) * mk::log_ns::log1p<F>(-__p);
    return mk::exp_ns::exp<F>(log_pmf);
  }

  [[nodiscard]] constexpr F
  cdf(I k) const noexcept
  {
    if ( k < I(0) ) return F(0);
    return __impl::__betainc<F>(__r, F(k + I(1)), __p);
  }

  [[nodiscard]] constexpr F
  mean() const noexcept
  {
    return __r * (F(1) - __p) / __p;
  }

  [[nodiscard]] constexpr F
  variance() const noexcept
  {
    return __r * (F(1) - __p) / (__p * __p);
  }
};

template<ieee754_floating F = f64, usize K = 0> class dirichlet_dist;

template<ieee754_floating F, usize K> class dirichlet_dist
{
  static_assert(K != 0);

  F __alpha[K];
  __gammadist<F> __gamma[K];
  F __log_norm;

public:
  using value_type = F;

  constexpr explicit dirichlet_dist(const F *alpha, usize = K) noexcept : __alpha{}, __gamma{}, __log_norm(0)
  {
    F sum = F(0);
    for ( usize i = 0; i < K; ++i ) {
      __alpha[i] = alpha[i];
      __gamma[i] = __gammadist<F>(alpha[i], F(1));
      __log_norm -= __impl::lgamma<F>(alpha[i]);
      sum += alpha[i];
    }
    __log_norm += __impl::lgamma<F>(sum);
  }

  template<rng_concept Rng>
  inline void
  operator()(Rng &g, F *__restrict__ out) const noexcept
  {
    F sum = F(0);
    for ( usize i = 0; i < K; ++i ) {
      out[i] = __gamma[i](g);
      sum += out[i];
    }
    if ( sum > F(0) ) {
      const F inv = F(1) / sum;
      for ( usize i = 0; i < K; ++i ) out[i] *= inv;
    }
  }

  [[nodiscard]] inline F
  pdf(const F *x) const noexcept
  {
    F log_p = __log_norm;
    for ( usize i = 0; i < K; ++i ) log_p += (__alpha[i] - F(1)) * mk::log_ns::log<F>(x[i]);
    return mk::exp_ns::exp<F>(log_p);
  }

  [[nodiscard]] static constexpr usize
  size() noexcept
  {
    return K;
  }
};

template<ieee754_floating F> class dirichlet_dist<F, 0>
{
  const F *__alpha;      // pointer to K-element parameter vector
  usize __kf;

public:
  using value_type = F;

  constexpr dirichlet_dist(const F *alpha, usize k) noexcept : __alpha(alpha), __kf(k) { }

  template<rng_concept Rng>
  inline void
  operator()(Rng &g, F *out) const noexcept
  {
    F sum = F(0);
    for ( usize i = 0; i < __kf; ++i ) {
      __gammadist<F> gd(__alpha[i], F(1));
      out[i] = gd(g);
      sum = sum + out[i];
    }
    if ( sum > F(0) ) {
      const F inv = F(1) / sum;
      for ( usize i = 0; i < __kf; ++i ) out[i] = out[i] * inv;
    }
  }

  [[nodiscard]] inline F
  pdf(const F *x) const noexcept
  {
    F log_norm = F(0);
    F log_p = F(0);
    F sum_a = F(0);
    for ( usize i = 0; i < __kf; ++i ) {
      log_norm = log_norm - __impl::lgamma<F>(__alpha[i]);
      sum_a = sum_a + __alpha[i];
      log_p = log_p + (__alpha[i] - F(1)) * mk::log_ns::log<F>(x[i]);
    }
    log_norm = log_norm + __impl::lgamma<F>(sum_a);
    return mk::exp_ns::exp<F>(log_norm + log_p);
  }

  [[nodiscard]] constexpr usize
  size() const noexcept
  {
    return __kf;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// auxilliary higher level fns (consider moving these out and expanding)

template<typename T, rng_concept Rng>
inline void
shuffle(T *first, T *last, Rng &g) noexcept
{
  const usize n = usize(last - first);
  for ( usize i = n; i > 1; --i ) {
    const u64 j = dist::uniform_uint_below<u64>(g, u64(i));
    T tmp = first[i - 1];
    first[i - 1] = first[j];
    first[j] = tmp;
  }
}

template<is_iterable_container C, rng_concept Rng>
inline void
shuffle(C &c, Rng &g) noexcept
{
  shuffle<typename C::value_type>(c.begin(), c.end(), g);
}

template<rng_concept Rng, typename... Cs>
  requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)
inline void
shuffle(Rng &g, Cs &...cs) noexcept
{
  ((shuffle<typename Cs::value_type>(cs.begin(), cs.end(), g)), ...);
}

template<typename I, rng_concept Rng>
  requires(micron::is_integral_v<I>)
inline void
permutation(I *first, usize n, Rng &g) noexcept
{
  for ( usize i = 0; i < n; ++i ) first[i] = I(i);
  shuffle<I>(first, first + n, g);
}

template<typename T, rng_concept Rng>
[[nodiscard]] inline T
choice(const T *first, usize n, Rng &g) noexcept
{
  const u64 idx = dist::uniform_uint_below<u64>(g, u64(n));
  return first[idx];
}

template<typename T, ieee754_floating F, rng_concept Rng>
[[nodiscard]] inline T
choice(const T *items, const F *weights, usize n, Rng &g) noexcept
{
  F sum = F(0);
  for ( usize i = 0; i < n; ++i ) sum = sum + weights[i];
  F u = dist::uniform_real<F>(g) * sum;
  for ( usize i = 0; i < n; ++i ) {
    u = u - weights[i];
    if ( u <= F(0) ) return items[i];
  }
  return items[n - 1];
}

template<is_iterable_container C, rng_concept Rng>
[[nodiscard]] inline typename C::value_type
choice(const C &c, Rng &g) noexcept
{
  return choice<typename C::value_type>(c.cbegin(), c.size(), g);
}

template<ieee754_floating F = f64> using gamma_dist = __gammadist<F>;
template<ieee754_floating F = f64> using beta_dist = __betadist<F>;

};      // namespace dists
};      // namespace rng
};      // namespace math
};      // namespace micron
