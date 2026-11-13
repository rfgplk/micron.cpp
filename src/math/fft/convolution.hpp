//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "plan.hpp"

namespace micron::math::fft
{

enum class convolution_mode { full, same, valid, circular };

struct convolution_descriptor {
  buffer<usize> input_shape, kernel_shape, period;
  convolution_mode mode{ convolution_mode::full };
  bool correlation{};

  convolution_descriptor(usize n, usize m) : input_shape{ n }, kernel_shape{ m } { }

  convolution_descriptor(const buffer<usize> &input, const buffer<usize> &kernel) : input_shape(input), kernel_shape(kernel) { }
};

namespace __impl
{
struct __convolution_geometry {
  buffer<usize> shape, output, crop;
  usize input_size{ 1 }, kernel_size{ 1 }, output_size{ 1 }, volume{ 1 };
  bool empty{};

  explicit __convolution_geometry(const convolution_descriptor &d)
  {
    usize rank = d.input_shape.size();
    if ( !rank || rank != d.kernel_shape.size() || unsigned(d.mode) > unsigned(convolution_mode::circular) )
      exc<except::invalid_argument>("fft: invalid convolution shape");
    if ( d.mode == convolution_mode::circular && d.period.size() != rank )
      exc<except::invalid_argument>("fft: circular convolution requires a period per axis");
    shape.resize(rank);
    output.resize(rank);
    crop.resize(rank);
    for ( usize a = 0; a < rank; ++a )
      if ( !d.input_shape[a] || !d.kernel_shape[a] ) {
        empty = true;
        break;
      }
    if ( empty ) {
      for ( usize a = 0; a < rank; ++a ) {
        usize n = d.input_shape[a], m = d.kernel_shape[a];
        if ( d.mode == convolution_mode::circular ) {
          usize p = d.period[a];
          if ( !p || n > p || m > p ) exc<except::invalid_argument>("fft: inputs exceed circular period");
        } else if ( d.mode == convolution_mode::valid && n && m && n < m )
          exc<except::invalid_argument>("fft: valid convolution requires input >= kernel");
        shape[a] = 1;
        output[a] = 0;
      }
      input_size = kernel_size = output_size = 0;
      return;
    }
    for ( usize a = 0; a < rank; ++a ) {
      usize n = d.input_shape[a], m = d.kernel_shape[a];
      input_size = __mul(input_size, n);
      kernel_size = __mul(kernel_size, m);
      if ( d.mode == convolution_mode::circular ) {
        usize p = d.period[a];
        if ( !p || n > p || m > p ) exc<except::invalid_argument>("fft: inputs exceed circular period");
        shape[a] = output[a] = p;
      } else {
        usize full = n && m ? __add(n, m) - 1 : 0;
        shape[a] = next_fast_len(full);
        switch ( d.mode ) {
        case convolution_mode::full:
          output[a] = full;
          break;
        case convolution_mode::same:
          output[a] = n;
          crop[a] = m ? (m - 1) / 2 : 0;
          break;
        case convolution_mode::valid:
          if ( n && m && n < m ) exc<except::invalid_argument>("fft: valid convolution requires input >= kernel");
          output[a] = n && m ? n - m + 1 : 0;
          crop[a] = m ? m - 1 : 0;
          break;
        default:
          break;
        }
      }
      volume = __mul(volume, shape[a]);
      output_size = __mul(output_size, output[a]);
    }
  }
};

inline usize
__embed_index(usize index, const buffer<usize> &source, const buffer<usize> &target, bool reverse = false) noexcept
{
  usize result = 0, step = 1;
  for ( usize a = source.size(); a-- > 0; ) {
    usize c = index % source[a];
    index /= source[a];
    result += (reverse ? source[a] - 1 - c : c) * step;
    step *= target[a];
  }
  return result;
}
}      // namespace __impl

// NOTE: a fixed kernels spectrum is computed once
// i/o use contiguous row-major storage
template<fft_scalar F> class convolution_plan
{
  convolution_descriptor __desc;
  __impl::__convolution_geometry __geometry;
  plan<F> __fft;
  buffer<complex<F>> __kernel;

  template<typename Kernel>
  void
  __prepare(const Kernel *kernel)
  {
    if ( __geometry.empty ) return;
    if ( !kernel ) exc<except::invalid_argument>("fft: null convolution kernel");
    buffer<complex<F>> time(__geometry.volume);
    bool reverse = __desc.correlation && __desc.mode != convolution_mode::circular;
    for ( usize k = 0; k < __geometry.kernel_size; ++k ) {
      complex<F> z{ kernel[k] };
      if ( reverse ) z = conj(z);
      time[__impl::__embed_index(k, __desc.kernel_shape, __geometry.shape, reverse)] = z;
    }
    workspace<F> work(__fft);
    __fft.execute(time.data(), __kernel.data(), work.view());
    if ( __desc.correlation && __desc.mode == convolution_mode::circular )
      for ( auto &z : __kernel ) z = conj(z);
  }

public:
  template<typename Input, typename Output, typename Runner>
  max_t
  __execute(const Input *input, Output *output, workspace_view<F> work, Runner &runner) const noexcept
  {
    if ( __geometry.empty ) return 0;
    if ( !input || !output ) return -22;
    usize base = 3 * __geometry.volume, per_worker = __fft.workspace_size() - __geometry.volume;
    if ( !runner.workers || runner.workers > (__impl::__limit - base) / per_worker || !work.data
         || work.size < base + runner.workers * per_worker )
      return -105;
    usize n = __geometry.volume;
    auto *time = work.data, *freq = time + n;
    workspace_view<F> scratch{ freq + n, work.size - 2 * n };
    for ( usize k = 0; k < n; ++k ) time[k] = {};
    for ( usize k = 0; k < __geometry.input_size; ++k )
      time[__impl::__embed_index(k, __desc.input_shape, __geometry.shape)] = complex<F>{ input[k] };
    __fft.__execute([&](ssize_t i) { return time[i]; }, [&](ssize_t i, complex<F> z) { freq[i] = z; }, scratch, direction::forward, runner);
    __impl::__multiply(freq, __kernel.data(), n);
    __fft.__execute([&](ssize_t i) { return freq[i]; }, [&](ssize_t i, complex<F> z) { time[i] = z; }, scratch, direction::backward,
                    runner);
    for ( usize k = 0; k < __geometry.output_size; ++k ) {
      usize index = k, source = 0, step = 1;
      for ( usize a = __geometry.output.size(); a-- > 0; ) {
        source += (index % __geometry.output[a] + __geometry.crop[a]) * step;
        index /= __geometry.output[a];
        step *= __geometry.shape[a];
      }
      if constexpr ( micron::is_same_v<Output, F> )
        output[k] = time[source].real;
      else
        output[k] = time[source];
    }
    return max_t(__geometry.output_size * sizeof(Output));
  }

public:
  template<typename Kernel>
  convolution_plan(const convolution_descriptor &d, const Kernel *kernel)
      : __desc(d), __geometry(d), __fft(descriptor<F>(__geometry.shape)), __kernel(__geometry.volume)
  {
    __impl::__mul(__geometry.output_size, sizeof(complex<F>));
    __impl::__mul(workspace_size(), sizeof(complex<F>));
    __prepare(kernel);
  }

  const buffer<usize> &
  output_shape() const noexcept
  {
    return __geometry.output;
  }

  usize
  output_size() const noexcept
  {
    return __geometry.output_size;
  }

  usize
  workspace_size(usize workers = 1) const
  {
    return __geometry.empty ? 0 : __impl::__add(2 * __geometry.volume, __fft.workspace_size(workers));
  }

  usize
  workspace_bytes(usize workers = 1) const
  {
    return __impl::__mul(workspace_size(workers), sizeof(complex<F>));
  }

  max_t
  execute(const complex<F> *input, complex<F> *output, workspace_view<F> work) const noexcept
  {
    __impl::__serial<F> runner;
    return __execute(input, output, work, runner);
  }

  max_t
  execute(const F *input, F *output, workspace_view<F> work) const noexcept
  {
    __impl::__serial<F> runner;
    return __execute(input, output, work, runner);
  }
};

template<typename T> struct __convolution_scalar {
  using type = T;
};

template<fft_scalar F> struct __convolution_scalar<complex<F>> {
  using type = F;
};

template<typename T>
buffer<T>
convolve(const T *input, const T *kernel, const convolution_descriptor &d)
{
  using F = typename __convolution_scalar<T>::type;
  convolution_plan<F> p(d, kernel);
  workspace<F> work(p);
  buffer<T> out(p.output_size());
  if ( p.execute(input, out.data(), work.view()) < 0 ) exc<except::invalid_argument>("fft: invalid convolution input");
  return out;
}

template<typename T>
buffer<T>
convolve(const T *input, usize n, const T *kernel, usize m, convolution_mode mode = convolution_mode::full)
{
  convolution_descriptor d(n, m);
  d.mode = mode;
  return convolve(input, kernel, d);
}

template<typename T>
buffer<T>
correlate(const T *input, usize n, const T *kernel, usize m, convolution_mode mode = convolution_mode::full)
{
  convolution_descriptor d(n, m);
  d.mode = mode;
  d.correlation = true;
  return convolve(input, kernel, d);
}

};      // namespace micron::math::fft
