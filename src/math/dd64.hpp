//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../types.hpp"
#include "ieee.hpp"

namespace micron
{
namespace math
{

struct dd64 {
  f64 hi;
  f64 lo;

  constexpr dd64() noexcept : hi(0.0), lo(0.0) { }

  constexpr dd64(f64 h, f64 l) noexcept : hi(h), lo(l) { }

  constexpr dd64(f64 h) noexcept : hi(h), lo(0.0) { }

  [[nodiscard, gnu::always_inline]] constexpr f64
  to_double() const noexcept
  {
    return hi + lo;
  }

  explicit constexpr
  operator f64() const noexcept
  {
    return hi + lo;
  }
};

static_assert(sizeof(dd64) == 16, "dd64 must be 2 doubles wide");

namespace dd
{

// Knuth's two sum (exact when a + b == s + e)
[[nodiscard, gnu::always_inline]] inline constexpr dd64
two_sum(f64 a, f64 b) noexcept
{
  f64 s = a + b;
  f64 bp = s - a;
  f64 e = (a - (s - bp)) + (b - bp);
  return { s, e };
}

// dekker's fast two sum (exact when abs(a) >= abs(b))
[[nodiscard, gnu::always_inline]] inline constexpr dd64
fast_two_sum(f64 a, f64 b) noexcept
{
  f64 s = a + b;
  f64 e = b - (s - a);
  return { s, e };
}

// two product via FMA (exact when a * b == p + e)
[[nodiscard, gnu::always_inline]] inline constexpr dd64
two_prod(f64 a, f64 b) noexcept
{
  f64 p = a * b;
  // clang doesn't support __builtin_fma in a constexpr somehow?!?!?
  if consteval {
#if defined(__micron_compiler_gcc)
    f64 e = __builtin_fma(a, b, -p);
#else
    f64 e = (a * b) - p;
#endif
    return { p, e };
  }
  f64 e = __builtin_fma(a, b, -p);
  return { p, e };
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
add(dd64 a, dd64 b) noexcept
{
  dd64 s = two_sum(a.hi, b.hi);
  dd64 t = two_sum(a.lo, b.lo);
  f64 c = s.lo + t.hi;
  dd64 r1 = two_sum(s.hi, c);
  f64 d = r1.lo + t.lo;
  return two_sum(r1.hi, d);
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
add(dd64 a, f64 b) noexcept
{
  dd64 s = two_sum(a.hi, b);
  return two_sum(s.hi, s.lo + a.lo);
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
sub(dd64 a, dd64 b) noexcept
{
  return add(a, dd64{ -b.hi, -b.lo });
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
mul(dd64 a, dd64 b) noexcept
{
  dd64 p = two_prod(a.hi, b.hi);
  f64 t = a.hi * b.lo + a.lo * b.hi;
  return fast_two_sum(p.hi, p.lo + t);
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
mul(dd64 a, f64 b) noexcept
{
  dd64 p = two_prod(a.hi, b);
  return fast_two_sum(p.hi, p.lo + a.lo * b);
}

[[nodiscard, gnu::always_inline]] inline constexpr dd64
div(dd64 a, dd64 b) noexcept
{
  f64 q1 = a.hi / b.hi;
  dd64 r = sub(a, mul(b, dd64{ q1, 0.0 }));
  f64 q2 = r.hi / b.hi;
  return fast_two_sum(q1, q2);
}

};      // namespace dd
};      // namespace math
};      // namespace micron
