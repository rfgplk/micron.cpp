//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/cmemory/memcpy.hpp"
#include "../numerics.hpp"
#include "bits.hpp"

namespace micron::math
{

template<typename F>
concept complex_scalar = micron::is_floating_point_v<F>;

template<complex_scalar F> struct complex {
  F real{}, imag{};

  constexpr complex() noexcept = default;

  constexpr complex(F r, F i = F(0)) noexcept : real(r), imag(i) { }

  template<complex_scalar G> explicit constexpr complex(const complex<G> &z) noexcept : real(F(z.real)), imag(F(z.imag)) { }

  constexpr complex
  operator+() const noexcept
  {
    return *this;
  }

  constexpr complex
  operator-() const noexcept
  {
    return { -real, -imag };
  }

  constexpr complex &
  operator+=(complex z) noexcept
  {
    real += z.real;
    imag += z.imag;
    return *this;
  }

  constexpr complex &
  operator-=(complex z) noexcept
  {
    real -= z.real;
    imag -= z.imag;
    return *this;
  }

  constexpr complex &
  operator*=(complex z) noexcept
  {
    F r = real * z.real - imag * z.imag;
    imag = real * z.imag + imag * z.real;
    real = r;
    return *this;
  }

  constexpr complex &
  operator*=(F x) noexcept
  {
    real *= x;
    imag *= x;
    return *this;
  }

  constexpr complex &
  operator/=(F x) noexcept
  {
    real /= x;
    imag /= x;
    return *this;
  }

private:
  [[gnu::noinline]] constexpr complex &
  __divide_scaled(complex z) noexcept
  {
    constexpr F epsilon = [] {
      if constexpr ( micron::is_same_v<F, long double> )
        return F(__micron_ldbl_epsilon);
      else if constexpr ( sizeof(F) == 2 )
        return F(0x1p-10);
      else if constexpr ( sizeof(F) == 4 )
        return F(micron::numeric_limits<float>::epsilon());
      else if constexpr ( sizeof(F) == 8 )
        return F(micron::numeric_limits<double>::epsilon());
      else
        return F(0x1p-112);
    }();
    constexpr F smallest = [] {
      if constexpr ( micron::is_same_v<F, long double> )
        return F(micron::numeric_limits<long double>::min());
      else if constexpr ( sizeof(F) == 2 )
        return F(0x1p-14);
      else if constexpr ( sizeof(F) == 4 )
        return F(micron::numeric_limits<float>::min());
      else if constexpr ( sizeof(F) == 8 )
        return F(micron::numeric_limits<double>::min());
      else {
        F value = F(0x1p-1022);
        for ( unsigned i = 0; i < 4; ++i ) value *= value;
        return value * F(0x1p-30);
      }
    }();
    constexpr F largest = [=] {
      if constexpr ( micron::is_same_v<F, long double> )
        return F(micron::numeric_limits<long double>::max());
      else if constexpr ( sizeof(F) == 2 )
        return F(65504);
      else if constexpr ( sizeof(F) == 4 )
        return F(micron::numeric_limits<float>::max());
      else if constexpr ( sizeof(F) == 8 )
        return F(micron::numeric_limits<double>::max());
      else
        return (F(2) / smallest) * (F(2) - epsilon);
    }();
    constexpr F small = F(2) * (smallest / epsilon);
    constexpr F boost = sizeof(F) == 2 ? F(1) / epsilon : F(2) / (epsilon * epsilon);
    F ar = z.real < 0 ? -z.real : z.real, ai = z.imag < 0 ? -z.imag : z.imag;
    F numerator = (real < 0 ? -real : real), other = (imag < 0 ? -imag : imag);
    if ( other > numerator ) numerator = other;
    F denominator = ar > ai ? ar : ai, a = real, b = imag, c = z.real, d = z.imag, scale = 1;
    // Scale Smith's formula before its sums overflow or products underflow.
    if ( numerator > largest / F(2) ) {
      a *= F(0.5);
      b *= F(0.5);
      scale *= F(2);
    } else if ( numerator && numerator < small ) {
      a *= boost;
      b *= boost;
      scale /= boost;
    }
    if ( denominator > largest / F(2) ) {
      c *= F(0.5);
      d *= F(0.5);
      scale *= F(0.5);
    } else if ( denominator && denominator < small ) {
      c *= boost;
      d *= boost;
      scale *= boost;
    }
    auto quotient_sum = [=](F x, F y, F r, F divisor) {
      F product = y * r;
      if ( y && r && (product < 0 ? -product : product) < smallest ) return x / divisor + (y / divisor) * r;
      return (x + product) / divisor;
    };
    auto product_ratio = [=](F x, F y, F divisor) {
      F r = y / divisor, magnitude = x < 0 ? -x : x;
      if ( (r < 0 ? -r : r) >= smallest || magnitude <= F(1) ) return x * r;
      F absolute_divisor = divisor < 0 ? -divisor : divisor;
      if ( absolute_divisor >= F(1) || magnitude <= largest * absolute_divisor ) return (x / divisor) * y;
      return (x * y) / divisor;
    };
    F r, divisor, re, im;
    if ( ar >= ai ) {
      r = d / c;
      divisor = c + d * r;
      if ( d && (r < 0 ? -r : r) < smallest ) {
        re = a / c + product_ratio(b / c, d, c);
        im = b / c - product_ratio(a / c, d, c);
      } else {
        re = quotient_sum(a, b, r, divisor);
        im = quotient_sum(b, -a, r, divisor);
      }
    } else {
      r = c / d;
      divisor = d + c * r;
      if ( c && (r < 0 ? -r : r) < smallest ) {
        re = b / d + product_ratio(a / d, c, d);
        im = -a / d + product_ratio(b / d, c, d);
      } else {
        re = quotient_sum(b, a, r, divisor);
        im = quotient_sum(-a, b, r, divisor);
      }
    }
    real = re * scale;
    imag = im * scale;
    return *this;
  }

public:
  [[gnu::always_inline]] constexpr complex &
  operator/=(complex z) noexcept
  {
    if constexpr ( sizeof(F) == 2 || sizeof(F) == 4 ) {
      using W = micron::conditional_t<sizeof(F) == 2, float, double>;
      W a = real, b = imag, c = z.real, d = z.imag;
      W reciprocal = W(1) / (c * c + d * d);
      real = F((a * c + b * d) * reciprocal);
      imag = F((b * c - a * d) * reciprocal);
      return *this;
    }
    bool bounded;
    if constexpr ( sizeof(F) == 8 ) {
      using U = u64;
      constexpr U sign_clear = ~(U(1) << 63), offset = U(129) << 52;
      constexpr U mask = U(15) << 60, target = U(4) << 60;
      U a = bits::bit_cast<U>(real) & sign_clear, b = bits::bit_cast<U>(imag) & sign_clear;
      U c = bits::bit_cast<U>(z.real) & sign_clear, d = bits::bit_cast<U>(z.imag) & sign_clear;
      auto outside = [=](U value) { return value ? ((value + offset) ^ target) & mask : U(0); };
      bounded = !(outside(a) | outside(b) | outside(c) | outside(d)) && bool(c | d);
    } else if constexpr ( micron::is_same_v<F, long double> && __micron_ldbl_mant_dig == 64 ) {
      struct __value {
        u64 significand;
        u16 exponent;
      };

      auto representation = [](const F &value) {
        if consteval {
          return bits::bit_cast<__value>(value);
        }
        // Read only x87's value bytes; its trailing padding is indeterminate.
        __value result;
        micron::cbytecpy<sizeof(result.significand)>(&result.significand, reinterpret_cast<const byte *>(&value));
        micron::cbytecpy<sizeof(result.exponent)>(&result.exponent, reinterpret_cast<const byte *>(&value) + sizeof(result.significand));
        return result;
      };
      auto a = representation(real), b = representation(imag);
      auto c = representation(z.real), d = representation(z.imag);
      auto outside = [](__value value) {
        unsigned exponent = value.exponent & 0x7fffu;
        return value.significand ? ((exponent + 129u) ^ 0x4000u) & 0xff00u : 0u;
      };
      bounded = !(outside(a) | outside(b) | outside(c) | outside(d)) && bool(c.significand | d.significand);
    } else {
      constexpr F lower = F(0x1p-128), upper = F(0x1p128);
      F a = real < 0 ? -real : real, b = imag < 0 ? -imag : imag;
      F c = z.real < 0 ? -z.real : z.real, d = z.imag < 0 ? -z.imag : z.imag;
      F numerator = a > b ? a : b, denominator = c > d ? c : d;
      bounded = numerator <= upper && denominator <= upper && denominator && (!a || a >= lower) && (!b || b >= lower) && (!c || c >= lower)
                && (!d || d >= lower);
    }
    if ( bounded ) {
      F reciprocal = F(1) / (z.real * z.real + z.imag * z.imag);
      F re = (real * z.real + imag * z.imag) * reciprocal;
      imag = (imag * z.real - real * z.imag) * reciprocal;
      real = re;
      return *this;
    }
    return __divide_scaled(z);
  }

  friend constexpr bool operator==(complex, complex) noexcept = default;

  friend constexpr complex
  operator+(complex a, complex b) noexcept
  {
    return a += b;
  }

  friend constexpr complex
  operator-(complex a, complex b) noexcept
  {
    return a -= b;
  }

  friend constexpr complex
  operator*(complex a, complex b) noexcept
  {
    return a *= b;
  }

  friend constexpr complex
  operator/(complex a, complex b) noexcept
  {
    return a /= b;
  }

  friend constexpr complex
  operator*(complex a, F b) noexcept
  {
    return a *= b;
  }

  friend constexpr complex
  operator*(F a, complex b) noexcept
  {
    return b *= a;
  }

  friend constexpr complex
  operator/(complex a, F b) noexcept
  {
    return a /= b;
  }
};

template<complex_scalar F>
constexpr complex<F>
conj(complex<F> z) noexcept
{
  return { z.real, -z.imag };
}

template<complex_scalar F>
constexpr F
norm(complex<F> z) noexcept
{
  return z.real * z.real + z.imag * z.imag;
}

};      // namespace micron::math
