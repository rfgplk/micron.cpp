//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// 256-layer ziggurat tables generated at compile time
//   .. region 0 is the base strip + tail (boundary x = R, density floor f[0]=1);
//   .. regions 1..254 are the equal-area rectangles, x increasing toward 0;
//   .. region 255 is the smallest box at the peak;
//   .. f[i] = exp(-x[i]^2/2) is DECREASING with i (f[0]=1, f[255]=f(R));
//   .. w[i] = x[i]/2^53 (region 0 uses the area-based width q = V/f(R));
//   .. k[i] = floor( (x[i]/x[i-1]) * 2^53 ) is the fast-accept threshold

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../bits/exp.hpp"
#include "../bits/log.hpp"
#include "../generic.hpp"
#include "../log.hpp"
#include "../sqrt.hpp"
#include "engines.hpp"

namespace micron
{
namespace math
{
namespace rng
{
namespace dist
{

namespace mkbits
{

struct ziggurat_tables {
  f64 w[256];      // x[i] / 2^53  (region 0: area-based width q/2^53)
  f64 f[256];      // exp(-x[i]^2 / 2)
  u64 k[256];      // floor((x[i]/x[i-1]) * 2^53)  fast-accept thresholds

  static constexpr f64 R = 3.6541528853610088;
  static constexpr f64 V = 0.00492867323399;
};

// f(x) = exp(-x^2/2)
[[nodiscard]] consteval f64
__zig_f(f64 x) noexcept
{
  return math::mkbits::exp_ns::exp_f64(-0.5 * x * x);
}

[[nodiscard]] consteval ziggurat_tables
build_ziggurat() noexcept
{
  ziggurat_tables t{};
  constexpr f64 M = 9007199254740992.0;      // 2^53

  const f64 R = ziggurat_tables::R;
  const f64 V = ziggurat_tables::V;
  const f64 fR = __zig_f(R);
  const f64 q = V / fR;      // width of the base rectangle incl. tail mass

  // base strip + tail
  t.k[0] = u64((R / q) * M);
  t.w[0] = q / M;
  t.f[0] = 1.0;

  // the topmost (smallest, peak) box uses x = R as its scale seed
  t.w[255] = R / M;
  t.f[255] = fR;
  t.k[1] = 0;

  // walking the stack downward in index
  f64 dn = R;      // x[255]
  f64 tn = R;      // previous x (for the k ratio)
  bool closed = true;
  for ( int i = 254; i >= 1; --i ) {
    const f64 arg = V / dn + __zig_f(dn);
    if ( !(arg > 0.0 && arg < 1.0) ) closed = false;
    dn = math::fsqrt(-2.0 * math::mkbits::log_ns::log_f64(arg));
    t.k[i + 1] = u64((dn / tn) * M);
    tn = dn;
    t.f[i] = __zig_f(dn);
    t.w[i] = dn / M;
  }

  const f64 smallest = dn;      // x[1]
  const bool sane = closed && smallest > 0.20 && smallest < 0.23 && t.f[1] > 0.97 && t.f[1] < 0.98;
  if ( !sane ) {
    // static_assert(false, "division by zero in build_ziggurat");
    // int *boom = nullptr;
    //(void)*boom;      // actually hard compile err if evaluated, above doesn't work
    // neither did that
    __builtin_unreachable();      // THIS SHOULD
  }
  return t;
}

inline constinit const ziggurat_tables ziggurat = build_ziggurat();

};      // namespace mkbits

template<ieee754_floating F = f64, rng_concept Rng>
[[nodiscard]] inline F
normal_ziggurat(Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  const f64 *__restrict__ wt = mkbits::ziggurat.w;
  const f64 *__restrict__ ft = mkbits::ziggurat.f;
  const u64 *__restrict__ kt = mkbits::ziggurat.k;
  const f64 R = mkbits::ziggurat_tables::R;

  u64 u = g.next();
  i64 hz = i64(u >> 11);      // 53-bit signed magnitude
  if ( u & 1ULL ) hz = -hz;
  u64 iz = (u >> 1) & 0xFFULL;
  u64 ahz = u64(hz < 0 ? -hz : hz);

  if ( ahz < kt[iz] ) [[likely]]
    return F(mu + sigma * F(f64(hz) * wt[iz]));

  // slow path: tail (iz == 0) or wedge rejection
  for ( ;; ) {
    f64 x = f64(hz) * wt[iz];
    if ( iz == 0 ) {
      // exponential-tail fallback for the base strip
      f64 xt;
      f64 yt;
      do {
        f64 u1 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
        f64 u2 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
        if ( u1 <= 0.0 ) u1 = 1e-300;
        if ( u2 <= 0.0 ) u2 = 1e-300;
        xt = -math::mkbits::log_ns::log_f64(u1) / R;
        yt = -math::mkbits::log_ns::log_f64(u2);
      } while ( yt + yt < xt * xt );
      f64 v = (hz > 0) ? (R + xt) : -(R + xt);
      return F(mu + sigma * F(v));
    }
    // wedge
    f64 u1 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
    f64 fy = ft[iz] + u1 * (ft[iz - 1] - ft[iz]);
    f64 ax = x < 0 ? -x : x;
    f64 phi_x = math::mkbits::exp_ns::exp_f64(-ax * ax * 0.5);
    if ( fy < phi_x ) return F(mu + sigma * F(x));

    // rejected
    u = g.next();
    hz = i64(u >> 11);
    if ( u & 1ULL ) hz = -hz;
    iz = (u >> 1) & 0xFFULL;
    ahz = u64(hz < 0 ? -hz : hz);
    if ( ahz < kt[iz] ) return F(mu + sigma * F(f64(hz) * wt[iz]));
  }
}

};      // namespace dist
};      // namespace rng
};      // namespace math
};      // namespace micron
