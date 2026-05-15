//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// univariate polynomial utilities
//
//  polyval(c, x):  Horner evaluation
//  polyder(c):  coefficient-wise derivative
//  polyint(c, k0):  antiderivative with integration constant
//  polyfit(x, y, deg): least-squares fit via QR of the Vandermonde
//  roots(c): roots via companion-matrix eigenvalues

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../mk.hpp"
#include "decomp.hpp"
#include "decomp_ext.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace poly
{

// Horner evaluation
template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
polyval(const dynvec<F> &c, micron::__type_identity_t<F> x) noexcept
{
  const usize N = c.size();
  if ( N == 0 ) return F(0);
  const F *__restrict__ p = c.data();
  const F *__restrict__ end = p + N;
  F acc = *p++;
  while ( p != end ) acc = math::fma<F>(acc, F(x), *p++);
  return acc;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
polyval(const F *c, usize n, micron::__type_identity_t<F> x) noexcept
{
  if ( n == 0 ) return F(0);
  const F *__restrict__ p = c;
  const F *__restrict__ end = p + n;
  F acc = *p++;
  while ( p != end ) acc = math::fma<F>(acc, F(x), *p++);
  return acc;
}

// derivative of c (high2low)
template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
polyder(const dynvec<F> &c) noexcept
{
  const usize N = c.size();
  if ( N <= 1 ) return dynvec<F>::zero(0);
  dynvec<F> r(N - 1);
  const F *__restrict__ src = c.data();
  F *__restrict__ dst = r.data();
  const usize deg = N - 1;
  for ( usize i = 0; i < deg; ++i ) *dst++ = F(deg - i) * *src++;
  return r;
}

// antiderivative with constatn k0
template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
polyint(const dynvec<F> &c, micron::__type_identity_t<F> k0 = F(0)) noexcept
{
  const usize N = c.size();
  dynvec<F> r(N + 1);
  const F *__restrict__ src = c.data();
  F *__restrict__ dst = r.data();
  const usize new_deg = N;
  for ( usize i = 0; i < N; ++i ) *dst++ = *src++ / F(new_deg - i);
  *dst = k0;
  return r;
}

// least squares polynomial fit
template<ieee754_floating F>
[[nodiscard]] inline dynvec<F>
polyfit(const dynvec<F> &x, const dynvec<F> &y, usize deg) noexcept
{
  const usize n = x.size();
  const usize m = deg + 1;
  dynmat<F> V(n, m);
  const F *__restrict__ xp = x.data();
  for ( usize i = 0; i < n; ++i ) {
    F xi = xp[i];
    F p = F(1);
    F *__restrict__ row = V.data() + i * V.ld;
    F *__restrict__ rev = row + (m - 1);
    for ( usize j = 0; j < m; ++j ) {
      *rev-- = p;
      p *= xi;
    }
  }

  auto qr = decomp::qr_householder(V);
  dynvec<F> qty(n, F(0));
  const F *__restrict__ yp = y.data();
  for ( usize i = 0; i < n; ++i ) {
    F s = F(0);
    const F *__restrict__ qcol = qr.Q.data() + i;
    for ( usize k = 0; k < n; ++k ) s = math::fma<F>(qcol[k * qr.Q.ld], yp[k], s);
    qty[i] = s;
  }
  dynvec<F> c(m);
  for ( usize ii = m; ii-- > 0; ) {
    F s = qty[ii];
    const F *__restrict__ row = qr.R.data() + ii * qr.R.ld;
    for ( usize j = ii + 1; j < m; ++j ) s = math::fma<F>(-row[j], c[j], s);
    c[ii] = s / row[ii];
  }
  return c;
}

// roots of a polynomial via the Frobenius companion matrix
template<ieee754_floating F> struct roots_result {
  dynvec<F> re;
  dynvec<F> im;
  bool converged;
};

namespace __impl_poly
{

template<ieee754_floating F, usize N>
[[nodiscard]] inline mat<F, N, N>
companion_fixed(const F *a) noexcept
{
  mat<F, N, N> C = mat<F, N, N>::zero();
  if constexpr ( N >= 2 ) {
    for ( usize i = 1; i < N; ++i ) C.data[i * N + (i - 1)] = F(1);
  }
  for ( usize i = 0; i < N; ++i ) C.data[i * N + (N - 1)] = -a[i];
  return C;
}

template<ieee754_floating F, usize N>
inline bool
fixed_companion_eigen(const F *a, F *re, F *im) noexcept
{
  if constexpr ( N == 1 ) {
    re[0] = -a[0];
    im[0] = F(0);
    return true;
  } else {
    auto C = companion_fixed<F, N>(a);
    auto er = decomp::eigen<F, N>(C);
    F *__restrict__ rep = re;
    F *__restrict__ imp = im;
    const F *__restrict__ vre = er.values_re.data;
    const F *__restrict__ vim = er.values_im.data;
    for ( usize i = 0; i < N; ++i ) {
      rep[i] = vre[i];
      imp[i] = vim[i];
    }
    return er.converged;
  }
}

template<ieee754_floating F>
inline bool
dispatch_fixed_companion_eigen(usize n, const F *a, F *re, F *im) noexcept
{
  switch ( n ) {
  case 1:
    return fixed_companion_eigen<F, 1>(a, re, im);
  case 2:
    return fixed_companion_eigen<F, 2>(a, re, im);
  case 3:
    return fixed_companion_eigen<F, 3>(a, re, im);
  case 4:
    return fixed_companion_eigen<F, 4>(a, re, im);
  case 5:
    return fixed_companion_eigen<F, 5>(a, re, im);
  case 6:
    return fixed_companion_eigen<F, 6>(a, re, im);
  case 7:
    return fixed_companion_eigen<F, 7>(a, re, im);
  case 8:
    return fixed_companion_eigen<F, 8>(a, re, im);
  case 9:
    return fixed_companion_eigen<F, 9>(a, re, im);
  case 10:
    return fixed_companion_eigen<F, 10>(a, re, im);
  case 11:
    return fixed_companion_eigen<F, 11>(a, re, im);
  default:
    return false;
  }
}

};      // namespace __impl_poly

template<ieee754_floating F>
[[nodiscard]] inline roots_result<F>
roots(const dynvec<F> &c) noexcept
{
  usize lead = 0;
  while ( lead < c.size() && c[lead] == F(0) ) ++lead;
  const usize n_eff = (c.size() > lead) ? (c.size() - lead - 1) : 0;
  roots_result<F> r{ dynvec<F>(n_eff), dynvec<F>(n_eff), true };
  if ( n_eff == 0 ) return r;
  micron::vector<F, micron::allocator_serial<>, false> a(n_eff);
  const F *__restrict__ cp = c.data() + lead;
  const F inv_lead = F(1) / cp[0];
  F *__restrict__ ap = a.data();
  for ( usize k = 0; k < n_eff; ++k ) ap[k] = cp[n_eff - k] * inv_lead;
  r.converged = __impl_poly::dispatch_fixed_companion_eigen<F>(n_eff, ap, r.re.data(), r.im.data());
  return r;
}

};      // namespace poly
};      // namespace linalg
};      // namespace math
};      // namespace micron
