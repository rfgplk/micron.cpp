//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "common.hpp"

namespace micron::math::fft
{

namespace __impl
{
template<fft_scalar F, bool Packed>
buffer<F>
__frequency_bins(usize n, F spacing)
{
  if ( !n || n > __limit || !(spacing > F(0)) ) exc<except::invalid_argument>("fft: invalid sampling interval or length");
  buffer<F> out(Packed ? n / 2 + 1 : n);
  constexpr F minimum = [] {
    if constexpr ( sizeof(F) == sizeof(float) )
      return F(micron::numeric_limits<float>::min());
    else if constexpr ( sizeof(F) == sizeof(double) )
      return F(micron::numeric_limits<double>::min());
    else
      return F(micron::numeric_limits<long double>::min());
  }();
  constexpr F maximum = [] {
    if constexpr ( sizeof(F) == sizeof(float) )
      return F(micron::numeric_limits<float>::max());
    else if constexpr ( sizeof(F) == sizeof(double) )
      return F(micron::numeric_limits<double>::max());
    else
      return F(micron::numeric_limits<long double>::max());
  }();
  // These spacing bounds keep the reciprocal normal for every supported length.
  if ( spacing >= F(2) / maximum && spacing <= (F(1) / F(__limit)) / minimum ) [[likely]] {
    F step = F(1) / (F(n) * spacing);
    for ( usize k = 0; k < out.size(); ++k ) out[k] = (Packed || k < (n + 1) / 2 ? F(k) : -F(n - k)) * step;
  } else {
    out[0] = F(0);
    for ( usize k = 1; k < out.size(); ++k ) out[k] = ((Packed || k < (n + 1) / 2 ? F(k) : -F(n - k)) / F(n)) / spacing;
  }
  return out;
}
}      // namespace __impl

template<fft_scalar F = double>
buffer<F>
fftfreq(usize n, F spacing = F(1))
{
  return __impl::__frequency_bins<F, false>(n, spacing);
}

template<fft_scalar F = double>
buffer<F>
rfftfreq(usize n, F spacing = F(1))
{
  return __impl::__frequency_bins<F, true>(n, spacing);
}

// The input and output ranges must be disjoint. In-place overloads use a temporary.
template<typename T>
void
fftshift(const T *input, T *output, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) output[i] = input[(i + (n + 1) / 2) % n];
}

template<typename T>
void
ifftshift(const T *input, T *output, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) output[i] = input[(i + n / 2) % n];
}

template<typename T>
buffer<T>
shift(const T *input, const buffer<usize> &shape, const buffer<usize> &axes = {}, bool inverse = false)
{
  usize n = __impl::__product(shape);
  if ( !input || shape.empty() ) exc<except::invalid_argument>("fft: invalid shift input");
  buffer<usize> selected = axes;
  if ( selected.empty() )
    for ( usize i = 0; i < shape.size(); ++i ) selected.push_back(i);
  for ( usize i = 0; i < selected.size(); ++i ) {
    if ( selected[i] >= shape.size() ) exc<except::invalid_argument>("fft: invalid shift axis");
    for ( usize j = 0; j < i; ++j )
      if ( selected[i] == selected[j] ) exc<except::invalid_argument>("fft: duplicate shift axis");
  }
  buffer<T> out(n);
  for ( usize i = 0; i < n; ++i ) {
    usize index = i, source = 0, step = 1;
    for ( usize a = shape.size(); a-- > 0; ) {
      usize coordinate = index % shape[a];
      index /= shape[a];
      for ( usize axis : selected )
        if ( axis == a ) coordinate = (coordinate + (inverse ? shape[a] / 2 : (shape[a] + 1) / 2)) % shape[a];
      source += coordinate * step;
      step *= shape[a];
    }
    out[i] = input[source];
  }
  return out;
}

template<typename T>
void
fftshift(T *data, usize n)
{
  if ( !n ) return;
  auto out = shift(data, buffer<usize>{ n });
  for ( usize i = 0; i < n; ++i ) data[i] = out[i];
}

template<typename T>
void
ifftshift(T *data, usize n)
{
  if ( !n ) return;
  auto out = shift(data, buffer<usize>{ n }, {}, true);
  for ( usize i = 0; i < n; ++i ) data[i] = out[i];
}

template<typename T>
buffer<T>
fftshift(const T *input, const buffer<usize> &shape, const buffer<usize> &axes = {})
{
  return shift(input, shape, axes, false);
}

template<typename T>
buffer<T>
ifftshift(const T *input, const buffer<usize> &shape, const buffer<usize> &axes = {})
{
  return shift(input, shape, axes, true);
}

}      // namespace micron::math::fft
