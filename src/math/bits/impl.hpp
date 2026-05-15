//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"

namespace micron
{
namespace math
{

template<ieee754_floating F, usize Dg> struct poly_coeffs {
  static constexpr usize size = Dg + 1;
  static constexpr usize degree = Dg;
  using value_type = F;
  F data[size];

  [[nodiscard, gnu::always_inline]] constexpr const F &
  operator[](usize i) const noexcept
  {
    return data[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr F &
  operator[](usize i) noexcept
  {
    return data[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F *
  begin() const noexcept
  {
    return data;
  }

  [[nodiscard, gnu::always_inline]] constexpr const F *
  end() const noexcept
  {
    return data + size;
  }
};

template<ieee754_floating F, usize N>
inline constexpr usize vec_align_v = (N * sizeof(F) <= 16)   ? 16
                                     : (N * sizeof(F) <= 32) ? 32
                                     : (N * sizeof(F) <= 64) ? 64
                                                             : 128;

template<typename F, usize R, usize C>
inline constexpr usize mat_align_v = (R * C * sizeof(F) <= 16)   ? 16
                                     : (R * C * sizeof(F) <= 32) ? 32
                                                                 : 64;

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fma(F a, F b, F c) noexcept
{
  // __builtins are ok, compiler splices in place
  if constexpr ( sizeof(F) == sizeof(float) )
    return F(__builtin_fmaf(float(a), float(b), float(c)));
  else if constexpr ( sizeof(F) == sizeof(double) )
    return F(__builtin_fma(double(a), double(b), double(c)));
  else
    return F(__builtin_fmal(static_cast<long double>(a), static_cast<long double>(b), static_cast<long double>(c)));
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr F
horner(const poly_coeffs<F, N> &c, F x) noexcept
{
  const F *__restrict__ const p = c.data;
  F r = p[N];
  for ( usize i = N; i-- > 0; ) r = fma<F>(r, x, p[i]);
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
horner_p(const F *__restrict__ c, usize n, F x) noexcept
{
  F r = c[n];
  while ( n-- != 0 ) r = fma<F>(r, x, c[n]);
  return r;
}

// 2-chain split Horner for even degree polynomials in s = x**2
// p(s) = (c0 + c2*s + c4*s**2 + ...) + s*(c1 + c3*s + c5*s**2 + ..)
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
horner_split2(const F *__restrict__ even, usize ne, const F *__restrict__ odd, usize no, F s) noexcept
{
  // p0/p1 will run in parallel
  F pe = even[ne];
  F po = odd[no];
  for ( usize i = ne; i-- > 0; ) pe = fma<F>(pe, s, even[i]);
  for ( usize i = no; i-- > 0; ) po = fma<F>(po, s, odd[i]);
  return fma<F>(po, s, pe);
}

[[nodiscard, gnu::always_inline]] inline f32
rsqrt_nr2(f32 x) noexcept
{
  f32 h = x * 0.5f;
  u32 i = bits::bit_cast<u32>(x);
  i = 0x5f3759dfu - (i >> 1);
  f32 r = bits::bit_cast<f32>(i);
  r = r * (1.5f - h * r * r);
  r = r * (1.5f - h * r * r);
  return r;
}

[[nodiscard, gnu::always_inline]] inline f64
rsqrt_nr2(f64 x) noexcept
{
  f64 h = x * 0.5;
  u64 i = bits::bit_cast<u64>(x);
  i = 0x5fe6eb50c7b537a9ULL - (i >> 1);
  f64 r = bits::bit_cast<f64>(i);
  r = r * (1.5 - h * r * r);
  r = r * (1.5 - h * r * r);
  r = r * (1.5 - h * r * r);
  return r;
}

};      // namespace math
};      // namespace micron
