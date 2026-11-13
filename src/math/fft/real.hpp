//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "plan.hpp"

namespace micron::math::fft
{

enum class r2r_kind { dct_i, dct_ii, dct_iii, dct_iv, dst_i, dst_ii, dst_iii, dst_iv };

namespace __impl
{
inline r2r_kind
__inverse_kind(r2r_kind kind) noexcept
{
  switch ( kind ) {
  case r2r_kind::dct_ii:
    return r2r_kind::dct_iii;
  case r2r_kind::dct_iii:
    return r2r_kind::dct_ii;
  case r2r_kind::dst_ii:
    return r2r_kind::dst_iii;
  case r2r_kind::dst_iii:
    return r2r_kind::dst_ii;
  default:
    return kind;
  }
}

inline usize
__logical_size(usize n, r2r_kind kind)
{
  if ( !n || (kind == r2r_kind::dct_i && n < 2) ) exc<except::invalid_argument>("fft: invalid real transform length");
  if ( kind == r2r_kind::dct_i ) return __mul(2, n - 1);
  if ( kind == r2r_kind::dst_i ) return __mul(2, __add(n, 1));
  return __mul(2, n);
}

inline usize
__embedding_size(usize n, r2r_kind kind)
{
  usize logical = __logical_size(n, kind);
  return kind == r2r_kind::dct_iv || kind == r2r_kind::dst_iv ? __mul(4, logical) : logical;
}

template<fft_scalar F> class __real_engine
{
  usize __n, __logical, __length;
  r2r_kind __kind;
  __engine<F> __fft;
  buffer<complex<F>> __phase;

public:
  __real_engine(usize n, r2r_kind kind)
      : __n(n), __logical(__logical_size(n, kind)), __length(__embedding_size(n, kind)), __kind(kind), __fft(__length), __phase(n + 1)
  {
    if ( unsigned(kind) > unsigned(r2r_kind::dst_iv) ) exc<except::invalid_argument>("fft: invalid real transform kind");
    for ( usize k = 0; k <= n; ++k ) __phase[k] = __root<F>(k, 4 * n);
  }

  usize
  logical_size() const noexcept
  {
    return __logical;
  }

  usize
  scratch_size() const noexcept
  {
    return 2 * __length + __fft.scratch_size();
  }

  template<typename Runner>
  void
  execute(const complex<F> *input, complex<F> *output, complex<F> *scratch, bool inverse, bool ortho, Runner &runner,
          bool parallel) const noexcept
  {
    r2r_kind kind = inverse ? __inverse_kind(__kind) : __kind;
    auto *time = scratch, *freq = time + __length, *temp = freq + __length;
    for ( usize i = 0; i < __length; ++i ) time[i] = {};
    F root2 = __sqrt(F(2));
    for ( usize j = 0; j < __n; ++j ) {
      F x = input[j].real;
      if ( ortho
           && ((kind == r2r_kind::dct_i && (j == 0 || j + 1 == __n)) || (kind == r2r_kind::dct_iii && j == 0)
               || (kind == r2r_kind::dst_iii && j + 1 == __n)) )
        x *= root2;
      switch ( kind ) {
      case r2r_kind::dct_i:
        time[j] = { x };
        if ( j && j + 1 < __n ) time[__length - j] = { x };
        break;
      case r2r_kind::dct_ii:
        time[j] = { x };
        time[__length - 1 - j] = { x };
        break;
      case r2r_kind::dct_iii:
        time[j] = conj(__phase[j]) * x;
        if ( j ) time[__length - j] = conj(time[j]);
        break;
      case r2r_kind::dct_iv:
        time[2 * j + 1] = { x };
        time[__length - (2 * j + 1)] = { x };
        break;
      case r2r_kind::dst_i:
        time[j + 1] = { x };
        time[__length - j - 1] = { -x };
        break;
      case r2r_kind::dst_ii:
        time[j] = { x };
        time[__length - 1 - j] = { -x };
        break;
      case r2r_kind::dst_iii:
        time[j + 1] = complex<F>{ 0, -x } * conj(__phase[j + 1]);
        if ( j + 1 < __n ) time[__length - j - 1] = conj(time[j + 1]);
        break;
      case r2r_kind::dst_iv:
        time[2 * j + 1] = { x };
        time[__length - (2 * j + 1)] = { -x };
        break;
      }
    }
    if ( parallel )
      __fft.execute_parallel(time, freq, temp, kind == r2r_kind::dct_iii || kind == r2r_kind::dst_iii, runner);
    else
      __fft.execute(time, freq, temp, kind == r2r_kind::dct_iii || kind == r2r_kind::dst_iii);
    for ( usize k = 0; k < __n; ++k ) {
      F x{};
      switch ( kind ) {
      case r2r_kind::dct_i:
      case r2r_kind::dct_iii:
      case r2r_kind::dst_iii:
        x = freq[k].real;
        break;
      case r2r_kind::dct_ii:
        x = (freq[k] * __phase[k]).real;
        break;
      case r2r_kind::dct_iv:
        x = freq[2 * k + 1].real;
        break;
      case r2r_kind::dst_i:
        x = -freq[k + 1].imag;
        break;
      case r2r_kind::dst_ii:
        x = -(freq[k + 1] * __phase[k + 1]).imag;
        break;
      case r2r_kind::dst_iv:
        x = -freq[2 * k + 1].imag;
        break;
      }
      if ( ortho
           && ((kind == r2r_kind::dct_i && (k == 0 || k + 1 == __n)) || (kind == r2r_kind::dct_ii && k == 0)
               || (kind == r2r_kind::dst_ii && k + 1 == __n)) )
        x /= root2;
      output[k] = { x };
    }
  }
};
}      // namespace __impl

template<fft_scalar F> class r2r_plan
{
  descriptor<F> __desc;
  buffer<__impl::__real_engine<F>> __engines;
  buffer<usize> __steps;
  usize __volume{}, __logical{ 1 }, __line_size{}, __scratch_size{}, __data_size{};

public:
  r2r_plan(usize n, r2r_kind kind, normalization norm = normalization::backward)
      : r2r_plan(
            [&] {
              descriptor<F> d(n);
              d.norm = norm;
              return d;
            }(),
            buffer<r2r_kind>{ kind })
  {
  }

  r2r_plan(const descriptor<F> &d, const buffer<r2r_kind> &kinds) : __desc(d)
  {
    if ( unsigned(d.norm) > unsigned(normalization::ortho) ) exc<except::invalid_argument>("fft: invalid normalization");
    if ( d.shape.empty() || d.kind != domain::complex ) exc<except::invalid_argument>("fft: invalid real transform descriptor");
    __volume = __impl::__product(d.shape);
    if ( __desc.axes.empty() )
      for ( usize i = 0; i < d.shape.size(); ++i ) __desc.axes.push_back(i);
    if ( kinds.size() != __desc.axes.size() ) exc<except::invalid_argument>("fft: one real transform kind per axis required");
    usize engine_scratch = 0;
    for ( usize i = 0; i < __desc.axes.size(); ++i ) {
      usize axis = __desc.axes[i];
      if ( axis >= d.shape.size() ) exc<except::invalid_argument>("fft: axis out of range");
      for ( usize j = 0; j < i; ++j )
        if ( __desc.axes[j] == axis ) exc<except::invalid_argument>("fft: duplicate axis");
      __engines.push_back(__impl::__real_engine<F>(d.shape[axis], kinds[i]));
      __logical = __impl::__mul(__logical, __engines[i].logical_size());
      if ( __engines[i].scratch_size() > engine_scratch ) engine_scratch = __engines[i].scratch_size();
      if ( d.shape[axis] > __line_size ) __line_size = d.shape[axis];
    }
    __scratch_size = __impl::__add(__impl::__mul(2, __line_size), engine_scratch);
    __data_size = __impl::__mul(__volume, d.batches);
    __steps.resize(d.shape.size());
    usize step = 1;
    for ( usize i = d.shape.size(); i-- > 0; ) {
      __steps[i] = step;
      step *= d.shape[i];
    }
    __impl::__mul(__impl::__add(__impl::__complete_layout(__desc.input, d.shape, d.batches), 1), sizeof(F));
    __impl::__mul(__impl::__add(__impl::__complete_layout(__desc.output, d.shape, d.batches), 1), sizeof(F));
    __impl::__mul(workspace_size(), sizeof(complex<F>));
  }

  const descriptor<F> &
  configuration() const noexcept
  {
    return __desc;
  }

  usize
  size() const noexcept
  {
    return __data_size;
  }

  usize
  workspace_size(usize workers = 1) const
  {
    if ( !workers ) exc<except::invalid_argument>("fft: zero workers");
    return __impl::__add(__data_size, __impl::__mul(workers, __scratch_size));
  }

  usize
  workspace_bytes(usize workers = 1) const
  {
    return __impl::__mul(workspace_size(workers), sizeof(complex<F>));
  }

  template<typename Runner>
  max_t
  __execute(const F *input, F *output, workspace_view<F> work, direction dir, Runner &runner) const noexcept
  {
    if ( dir != direction::forward && dir != direction::backward ) return -22;
    if ( !__desc.batches ) return 0;
    if ( !input || !output ) return -22;
    if ( !runner.workers || runner.workers > (__impl::__limit - __data_size) / __scratch_size || !work.data
         || work.size < __data_size + runner.workers * __scratch_size )
      return -105;
    auto *data = work.data;
    for ( usize b = 0; b < __desc.batches; ++b )
      for ( usize k = 0; k < __volume; ++k ) data[b * __volume + k] = { input[__impl::__offset(k, __desc.shape, __desc.input, b)] };
    for ( usize a = 0; a < __desc.axes.size(); ++a ) {
      usize axis = __desc.axes[a], n = __desc.shape[axis], stride = __steps[axis];
      runner(__data_size / n, data + __data_size, __scratch_size, [&](usize line, complex<F> *scratch) noexcept {
        usize base = (line / stride) * (stride * n) + line % stride;
        auto *in = scratch, *out = scratch + __line_size, *temp = out + __line_size;
        for ( usize j = 0; j < n; ++j ) in[j] = data[base + j * stride];
        __engines[a].execute(in, out, temp, dir == direction::backward, __desc.norm == normalization::ortho, runner, __data_size / n == 1);
        for ( usize j = 0; j < n; ++j ) data[base + j * stride] = out[j];
      });
    }
    F scale = __impl::__scale<F>(__desc.norm, dir, __logical) * (dir == direction::forward ? __desc.forward_scale : __desc.backward_scale);
    for ( usize b = 0; b < __desc.batches; ++b )
      for ( usize k = 0; k < __volume; ++k )
        output[__impl::__offset(k, __desc.shape, __desc.output, b)] = data[b * __volume + k].real * scale;
    return max_t(__data_size * sizeof(F));
  }

  max_t
  execute(const F *input, F *output, workspace_view<F> work, direction dir = direction::forward) const noexcept
  {
    __impl::__serial<F> runner;
    return __execute(input, output, work, dir, runner);
  }
};

template<fft_scalar F>
buffer<F>
real_transform(const F *input, usize n, r2r_kind kind, direction dir = direction::forward, normalization norm = normalization::backward)
{
  r2r_plan<F> p(n, kind, norm);
  workspace<F> work(p);
  buffer<F> out(n);
  if ( p.execute(input, out.data(), work.view(), dir) < 0 ) exc<except::invalid_argument>("fft: invalid real transform input");
  return out;
}

template<fft_scalar F>
buffer<F>
dct(const F *input, usize n, unsigned type = 2, normalization norm = normalization::backward, direction dir = direction::forward)
{
  if ( type < 1 || type > 4 ) exc<except::invalid_argument>("fft: DCT type must be 1..4");
  return real_transform(input, n, r2r_kind(type - 1), dir, norm);
}

template<fft_scalar F>
buffer<F>
dst(const F *input, usize n, unsigned type = 2, normalization norm = normalization::backward, direction dir = direction::forward)
{
  if ( type < 1 || type > 4 ) exc<except::invalid_argument>("fft: DST type must be 1..4");
  return real_transform(input, n, r2r_kind(type + 3), dir, norm);
}

};      // namespace micron::math::fft
