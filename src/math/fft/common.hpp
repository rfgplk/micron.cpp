//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../vector/vector.hpp"
#include "../complex.hpp"

namespace micron::math::fft
{

template<typename F>
concept fft_scalar = micron::math::complex_scalar<F> && (sizeof(F) == 4 || sizeof(F) == 8 || micron::is_same_v<F, long double>);

template<typename T> using buffer = micron::vector<T, micron::allocator_serial<>, false>;
enum class direction { forward, backward };
enum class normalization { backward, none, forward, ortho };
enum class domain { complex, real_forward, real_backward };

namespace __impl
{
inline constexpr usize __limit = usize(micron::numeric_limits<intptr_t>::max());

inline usize
__add(usize a, usize b)
{
  if ( b > __limit || a > __limit - b ) exc<except::length_error>("fft: size overflow");
  return a + b;
}

inline usize
__mul(usize a, usize b)
{
  if ( b && a > __limit / b ) exc<except::length_error>("fft: size overflow");
  return a * b;
}

inline usize
__product(const buffer<usize> &shape)
{
  usize n = 1;
  for ( usize v : shape ) {
    if ( !v ) exc<except::invalid_argument>("fft: zero dimension");
    n = __mul(n, v);
  }
  return n;
}

template<fft_scalar F>
constexpr F
__abs(F x) noexcept
{
  return x < F(0) ? -x : x;
}

template<fft_scalar F>
constexpr F
__epsilon() noexcept
{
  if constexpr ( sizeof(F) == sizeof(float) )
    return F(micron::numeric_limits<float>::epsilon());
  else if constexpr ( sizeof(F) == sizeof(double) )
    return F(micron::numeric_limits<double>::epsilon());
  else
    return F(__micron_ldbl_epsilon);
}

template<fft_scalar F>
F
__sqrt(F x) noexcept
{
  if ( x == F(0) ) return x;
  F scale = 1;
  while ( x > F(4) ) {
    x /= F(4);
    scale *= F(2);
  }
  while ( x < F(1) ) {
    x *= F(4);
    scale /= F(2);
  }
  F y = (x + F(1)) / F(2);
  for ( unsigned i = 0; i < 12; ++i ) y = (y + x / y) / F(2);
  return y * scale;
}

// exp(-2*pi*i*k/n), with integer quadrant reduction before floating conversion.
template<fft_scalar F>
complex<F>
__root(usize k, usize n) noexcept
{
  k %= n;
  unsigned quadrant = 0;
  // k*4 is bounded by the plan's checked phase denominator.
  usize scaled = k * 4;
  quadrant = unsigned(scaled / n);
  usize residue = scaled % n;
  bool reflect = residue > n / 2;
  usize reduced = reflect ? n - residue : residue;
  F x = (F(reduced) / F(n)) * F(1.570796326794896619231321691639751442098584699687552910487L);
  F xx = x * x, s = x, c = 1, st = x, ct = 1;
  for ( unsigned j = 1; j <= 20; ++j ) {
    st *= -xx / F((2 * j) * (2 * j + 1));
    s += st;
    ct *= -xx / F((2 * j - 1) * (2 * j));
    c += ct;
  }
  if ( reflect ) {
    F tmp = s;
    s = c;
    c = tmp;
  }
  switch ( quadrant ) {
  case 0:
    return { c, -s };
  case 1:
    return { -s, -c };
  case 2:
    return { -c, s };
  default:
    return { s, c };
  }
}

template<fft_scalar F>
F
__scale(normalization mode, direction dir, usize n) noexcept
{
  if ( mode == normalization::ortho ) return F(1) / __sqrt(F(n));
  if ( (mode == normalization::backward && dir == direction::backward) || (mode == normalization::forward && dir == direction::forward) )
    return F(1) / F(n);
  return F(1);
}
}      // namespace __impl

// Smallest 2/3/5/7-smooth length >= n; no linear scan through candidate lengths.
inline usize
next_fast_len(usize n)
{
  if ( n <= 1 ) return 1;
  if ( n > __impl::__limit / 8 ) exc<except::length_error>("fft: length too large");
  usize best = 1;
  while ( best < n ) best *= 2;
  for ( usize a = 1; a <= best; ) {
    for ( usize b = a; b <= best; ) {
      for ( usize c = b; c <= best; ) {
        usize d = c;
        while ( d < n && d <= best / 2 ) d *= 2;
        if ( d >= n && d < best ) best = d;
        if ( c > best / 3 ) break;
        c *= 3;
      }
      if ( b > best / 5 ) break;
      b *= 5;
    }
    if ( a > best / 7 ) break;
    a *= 7;
  }
  return best;
}

template<fft_scalar F> struct workspace_view {
  complex<F> *data{};
  usize size{};
};

template<fft_scalar F> class workspace
{
  buffer<complex<F>> __storage;

public:
  explicit workspace(usize elements) : __storage(elements) { }

  template<typename Plan> explicit workspace(const Plan &p) : workspace(p.workspace_size()) { }

  workspace_view<F>
  view() noexcept
  {
    return { __storage.data(), __storage.size() };
  }

  usize
  size() const noexcept
  {
    return __storage.size();
  }
};

struct layout {
  buffer<ssize_t> strides;
  ssize_t offset{};
  ssize_t distance{};      // zero selects the contiguous logical volume
};

template<fft_scalar F> struct descriptor {
  buffer<usize> shape;
  buffer<usize> axes;      // empty selects every axis, in row-major order
  layout input, output;
  usize batches{ 1 };
  domain kind{ domain::complex };
  normalization norm{ normalization::backward };
  F forward_scale{ 1 }, backward_scale{ 1 };

  explicit descriptor(usize n) : shape{ n } { }

  descriptor(std::initializer_list<usize> dimensions) : shape(dimensions) { }

  explicit descriptor(const buffer<usize> &dimensions) : shape(dimensions) { }
};

};      // namespace micron::math::fft
