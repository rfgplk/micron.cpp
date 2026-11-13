//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

namespace micron::math::fft
{

template<fft_scalar F> struct split_input {
  const F *real{}, *imag{};
};

template<fft_scalar F> struct split_output {
  F *real{}, *imag{};
};

namespace __impl
{
inline usize
__complete_layout(layout &l, const buffer<usize> &shape, usize batches)
{
  usize volume = __product(shape);
  if ( l.strides.empty() ) {
    l.strides.resize(shape.size());
    usize stride = 1;
    for ( usize i = shape.size(); i-- > 0; ) {
      l.strides[i] = ssize_t(stride);
      stride = __mul(stride, shape[i]);
    }
  }
  if ( l.strides.size() != shape.size() ) exc<except::invalid_argument>("fft: stride rank mismatch");
  if ( !l.distance ) l.distance = ssize_t(volume);
  usize extent = 0;
  for ( usize i = 0; i <= shape.size(); ++i ) {
    ssize_t s = i == shape.size() ? l.distance : l.strides[i];
    usize n = i == shape.size() ? batches : shape[i];
    if ( s == -ssize_t(__limit) - 1 || (!s && n > 1) ) exc<except::invalid_argument>("fft: invalid stride");
    usize magnitude = usize(s < 0 ? -s : s);
    extent = __add(extent, __mul(magnitude, n ? n - 1 : 0));
  }
  if ( l.offset == -ssize_t(__limit) - 1 ) exc<except::length_error>("fft: offset overflow");
  return __add(extent, usize(l.offset < 0 ? -l.offset : l.offset));
}

inline ssize_t
__offset(usize index, const buffer<usize> &shape, const layout &l, usize batch) noexcept
{
  ssize_t offset = l.offset + ssize_t(batch) * l.distance;
  if ( shape.size() == 1 ) return offset + ssize_t(index) * l.strides[0];
  for ( usize j = shape.size(); j-- > 0; ) {
    offset += ssize_t(index % shape[j]) * l.strides[j];
    index /= shape[j];
  }
  return offset;
}

inline bool
__dense(const buffer<usize> &shape, const layout &l) noexcept
{
  usize stride = 1;
  for ( usize j = shape.size(); j-- > 0; ) {
    if ( l.strides[j] != ssize_t(stride) ) return false;
    stride *= shape[j];
  }
  return true;
}

template<fft_scalar F> struct __serial {
  usize workers{ 1 };

  template<typename Function>
  void
  operator()(usize count, complex<F> *scratch, usize, Function &&fn) const noexcept
  {
    for ( usize i = 0; i < count; ++i ) fn(i, scratch);
  }
};
}      // namespace __impl

template<fft_scalar F> class plan
{
  descriptor<F> __desc;
  buffer<usize> __input_shape, __output_shape, __steps;
  buffer<__impl::__engine<F>> __engines;
  buffer<complex<F>> __real_twiddles;
  usize __volume{}, __input_volume{}, __output_volume{}, __transform_volume{ 1 };
  usize __line_size{}, __engine_scratch{}, __data_size{}, __scratch_size{}, __packed_axis{};
  bool __input_dense{}, __output_dense{};

  usize
  __packed_index(usize index, bool &mirror) const noexcept
  {
    usize coordinate = (index / __steps[__packed_axis]) % __desc.shape[__packed_axis];
    mirror = coordinate >= __input_shape[__packed_axis];
    usize result = 0, step = 1;
    for ( usize a = __desc.shape.size(); a-- > 0; ) {
      usize c = (index / __steps[a]) % __desc.shape[a];
      if ( mirror )
        for ( usize axis : __desc.axes )
          if ( a == axis && c ) c = __desc.shape[a] - c;
      result += c * step;
      step *= __input_shape[a];
    }
    return result;
  }

public:
  explicit plan(usize n, normalization norm = normalization::backward)
      : plan([&] {
          descriptor<F> d(n);
          d.norm = norm;
          return d;
        }())
  {
  }

  explicit plan(const descriptor<F> &d) : __desc(d), __input_shape(d.shape), __output_shape(d.shape)
  {
    if ( unsigned(d.kind) > unsigned(domain::real_backward) || unsigned(d.norm) > unsigned(normalization::ortho) )
      exc<except::invalid_argument>("fft: invalid transform configuration");
    if ( d.shape.empty() ) exc<except::invalid_argument>("fft: empty rank");
    __volume = __impl::__product(d.shape);
    if ( __desc.axes.empty() )
      for ( usize i = 0; i < d.shape.size(); ++i ) __desc.axes.push_back(i);
    __packed_axis = __desc.axes[__desc.axes.size() - 1];
    for ( usize i = 0; i < __desc.axes.size(); ++i ) {
      usize axis = __desc.axes[i];
      if ( axis >= d.shape.size() ) exc<except::invalid_argument>("fft: axis out of range");
      for ( usize j = 0; j < i; ++j )
        if ( __desc.axes[j] == axis ) exc<except::invalid_argument>("fft: duplicate axis");
      usize n = d.shape[axis];
      __transform_volume = __impl::__mul(__transform_volume, n);
      bool half = d.kind != domain::complex && axis == __packed_axis && n % 2 == 0;
      __engines.push_back(__impl::__engine<F>(half ? n / 2 : n));
      if ( half ) {
        __real_twiddles.resize(n / 2 + 1);
        for ( usize k = 0; k <= n / 2; ++k ) __real_twiddles[k] = __impl::__root<F>(k, n);
      }
      if ( n > __line_size ) __line_size = n;
      usize scratch = __engines[i].scratch_size();
      if ( scratch > __engine_scratch ) __engine_scratch = scratch;
    }
    __packed_axis = __desc.axes[__desc.axes.size() - 1];
    if ( d.kind == domain::real_forward ) __output_shape[__packed_axis] = d.shape[__packed_axis] / 2 + 1;
    if ( d.kind == domain::real_backward ) __input_shape[__packed_axis] = d.shape[__packed_axis] / 2 + 1;
    __input_volume = __impl::__product(__input_shape);
    __output_volume = __impl::__product(__output_shape);
    __steps.resize(d.shape.size());
    usize step = 1;
    for ( usize i = d.shape.size(); i-- > 0; ) {
      __steps[i] = step;
      step *= d.shape[i];
    }
    __impl::__mul(__impl::__add(__impl::__complete_layout(__desc.input, __input_shape, d.batches), 1),
                  d.kind == domain::real_forward ? sizeof(F) : sizeof(complex<F>));
    __impl::__mul(__impl::__add(__impl::__complete_layout(__desc.output, __output_shape, d.batches), 1),
                  d.kind == domain::real_backward ? sizeof(F) : sizeof(complex<F>));
    __input_dense = __impl::__dense(__input_shape, __desc.input);
    __output_dense = __impl::__dense(__output_shape, __desc.output);
    __data_size = __impl::__mul(__volume, d.batches);
    __scratch_size = __impl::__add(__impl::__mul(5, __line_size), __engine_scratch);
    __impl::__mul(workspace_size(), sizeof(complex<F>));
    __impl::__mul(__impl::__mul(__output_volume, d.batches), d.kind == domain::real_backward ? sizeof(F) : sizeof(complex<F>));
  }

  const descriptor<F> &
  configuration() const noexcept
  {
    return __desc;
  }

  const buffer<usize> &
  input_shape() const noexcept
  {
    return __input_shape;
  }

  const buffer<usize> &
  output_shape() const noexcept
  {
    return __output_shape;
  }

  usize
  input_size() const noexcept
  {
    return __input_volume * __desc.batches;
  }

  usize
  output_size() const noexcept
  {
    return __output_volume * __desc.batches;
  }

  usize
  workspace_size(usize workers = 1) const
  {
    if ( !workers ) exc<except::invalid_argument>("fft: zero workers");
    return __impl::__add(__data_size, __impl::__mul(__scratch_size, workers));
  }

  usize
  workspace_bytes(usize workers = 1) const
  {
    return __impl::__mul(workspace_size(workers), sizeof(complex<F>));
  }

  // Internal runner seam: one barrier per separable axis, one scratch slice per worker.
  template<typename Load, typename Store, typename Runner>
  max_t
  __execute(Load load, Store store, workspace_view<F> work, direction dir, Runner &runner) const noexcept
  {
    if ( dir != direction::forward && dir != direction::backward ) return -22;
    if ( !__desc.batches ) return 0;
    if ( !runner.workers || runner.workers > (__impl::__limit - __data_size) / __scratch_size || !work.data
         || work.size < __data_size + runner.workers * __scratch_size )
      return -105;
    auto *data = work.data;
    for ( usize b = 0; b < __desc.batches; ++b )
      for ( usize k = 0; k < __volume; ++k ) {
        bool mirror = false;
        usize source = __desc.kind == domain::real_backward ? __packed_index(k, mirror) : k;
        ssize_t index = __input_dense ? __desc.input.offset + ssize_t(b) * __desc.input.distance + ssize_t(source)
                                      : __impl::__offset(source, __input_shape, __desc.input, b);
        auto z = load(index);
        data[b * __volume + k] = mirror ? conj(z) : z;
      }
    for ( usize pass = 0; pass < __desc.axes.size(); ++pass ) {
      usize a = __desc.kind == domain::real_forward ? (pass ? pass - 1 : __desc.axes.size() - 1) : pass;
      usize axis = __desc.axes[a], n = __desc.shape[axis], stride = __steps[axis];
      usize lines = __data_size / n;
      usize tile = stride >= 4 ? 4 : 1;
      runner((lines + tile - 1) / tile, data + __data_size, __scratch_size, [&](usize group, complex<F> *scratch) noexcept {
        if ( __desc.kind == domain::complex && stride == 1 ) {
          auto *line = data + group * n;
          auto *temp = scratch + __line_size;
          if ( lines == 1 )
            __engines[a].execute_parallel(line, scratch, temp, dir == direction::backward, runner);
          else
            __engines[a].execute(line, scratch, temp, dir == direction::backward);
          for ( usize j = 0; j < n; ++j ) line[j] = scratch[j];
          return;
        }
        usize count = lines - group * tile < tile ? lines - group * tile : tile;
        usize bases[4];
        for ( usize lane = 0; lane < count; ++lane ) {
          usize line = group * tile + lane;
          bases[lane] = (line / stride) * (stride * n) + line % stride;
        }
        for ( usize j = 0; j < n; ++j )
          for ( usize lane = 0; lane < count; ++lane ) scratch[lane * __line_size + j] = data[bases[lane] + j * stride];
        auto *out = scratch + 4 * __line_size, *temp = out + __line_size;
        for ( usize lane = 0; lane < count; ++lane ) {
          auto *in = scratch + lane * __line_size;
          usize base = bases[lane];
          if ( !__real_twiddles.empty() && axis == __packed_axis ) {
            usize h = n / 2;
            if ( dir == direction::forward ) {
              for ( usize j = 0; j < h; ++j ) in[j] = { in[2 * j].real, in[2 * j + 1].real };
              if ( lines == 1 )
                __engines[a].execute_parallel(in, out, temp, false, runner);
              else
                __engines[a].execute(in, out, temp, false);
              for ( usize k = 0; k <= h; ++k ) {
                auto x = out[k % h], y = conj(out[(h - k) % h]);
                auto difference = (x - y) * __real_twiddles[k];
                auto z = (x + y + complex<F>{ difference.imag, -difference.real }) * F(0.5);
                in[k] = z;
                if ( k && k < h ) in[n - k] = conj(z);
              }
            } else {
              for ( usize k = 0; k < h; ++k ) {
                auto x = data[base + k * stride], y = conj(data[base + (h - k) * stride]);
                auto difference = (x - y) * conj(__real_twiddles[k]);
                in[k] = (x + y + complex<F>{ -difference.imag, difference.real }) * F(0.5);
              }
              if ( lines == 1 )
                __engines[a].execute_parallel(in, out, temp, true, runner);
              else
                __engines[a].execute(in, out, temp, true);
              for ( usize k = 0; k < h; ++k ) {
                in[2 * k] = { F(2) * out[k].real };
                in[2 * k + 1] = { F(2) * out[k].imag };
              }
            }
          } else {
            if ( lines == 1 )
              __engines[a].execute_parallel(in, out, temp, dir == direction::backward, runner);
            else
              __engines[a].execute(in, out, temp, dir == direction::backward);
            for ( usize j = 0; j < n; ++j ) in[j] = out[j];
          }
        }
        for ( usize j = 0; j < n; ++j )
          for ( usize lane = 0; lane < count; ++lane ) data[bases[lane] + j * stride] = scratch[lane * __line_size + j];
      });
    }

    F scale = __impl::__scale<F>(__desc.norm, dir, __transform_volume)
              * (dir == direction::forward ? __desc.forward_scale : __desc.backward_scale);
    for ( usize b = 0; b < __desc.batches; ++b )
      for ( usize k = 0; k < __output_volume; ++k ) {
        usize source = k;
        if ( __desc.kind == domain::real_forward ) {
          usize t = k;
          source = 0;
          for ( usize j = __desc.shape.size(); j-- > 0; ) {
            source += (t % __output_shape[j]) * __steps[j];
            t /= __output_shape[j];
          }
        }
        ssize_t index = __output_dense ? __desc.output.offset + ssize_t(b) * __desc.output.distance + ssize_t(k)
                                       : __impl::__offset(k, __output_shape, __desc.output, b);
        store(index, data[b * __volume + source] * scale);
      }
    return max_t(output_size() * (__desc.kind == domain::real_backward ? sizeof(F) : sizeof(complex<F>)));
  }

  max_t
  execute(const complex<F> *in, complex<F> *out, workspace_view<F> work, direction dir = direction::forward) const noexcept
  {
    if ( __desc.kind != domain::complex || (__desc.batches && (!in || !out)) ) return -22;
    if ( dir != direction::forward && dir != direction::backward ) return -22;
    if ( __desc.shape.size() == 1 && __desc.batches == 1 && __desc.input.offset == 0 && __desc.output.offset == 0
         && __desc.input.strides[0] == 1 && __desc.output.strides[0] == 1 ) {
      if ( !work.data || work.size < __data_size + __scratch_size ) return -105;
      const complex<F> *source = in;
      if ( in == out ) {
        for ( usize k = 0; k < __volume; ++k ) work.data[k] = in[k];
        source = work.data;
      }
      __engines[0].execute(source, out, work.data + __volume, dir == direction::backward);
      F scale = __impl::__scale<F>(__desc.norm, dir, __transform_volume)
                * (dir == direction::forward ? __desc.forward_scale : __desc.backward_scale);
      if ( scale != F(1) )
        for ( usize k = 0; k < __volume; ++k ) out[k] *= scale;
      return max_t(__volume * sizeof(complex<F>));
    }
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return in[i]; }, [&](ssize_t i, complex<F> z) { out[i] = z; }, work, dir, runner);
  }

  max_t
  execute(split_input<F> in, split_output<F> out, workspace_view<F> work, direction dir = direction::forward) const noexcept
  {
    if ( __desc.kind != domain::complex || (__desc.batches && (!in.real || !in.imag || !out.real || !out.imag)) ) return -22;
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return complex<F>{ in.real[i], in.imag[i] }; },
                     [&](ssize_t i, complex<F> z) {
                       out.real[i] = z.real;
                       out.imag[i] = z.imag;
                     },
                     work, dir, runner);
  }

  max_t
  execute(const F *in, complex<F> *out, workspace_view<F> work) const noexcept
  {
    if ( __desc.kind != domain::real_forward || (__desc.batches && (!in || !out)) ) return -22;
    if ( __desc.shape.size() == 1 && __desc.batches == 1 && __input_dense && __output_dense ) {
      if ( !work.data || work.size < __data_size + __scratch_size ) return -105;
      in += __desc.input.offset;
      out += __desc.output.offset;
      auto *packed = work.data, *spectrum = packed + __volume, *temp = spectrum + __volume;
      usize n = __volume, h = n / 2;
      F scale = __impl::__scale<F>(__desc.norm, direction::forward, n) * __desc.forward_scale;
      if ( n % 2 == 0 ) {
        for ( usize j = 0; j < h; ++j ) packed[j] = { in[2 * j], in[2 * j + 1] };
        __engines[0].execute(packed, spectrum, temp, false);
        for ( usize k = 0; k <= h; ++k ) {
          auto x = spectrum[k == h ? 0 : k], y = conj(spectrum[k ? h - k : 0]);
          auto difference = (x - y) * __real_twiddles[k];
          out[k] = (x + y + complex<F>{ difference.imag, -difference.real }) * (F(0.5) * scale);
        }
      } else {
        for ( usize j = 0; j < n; ++j ) packed[j] = { in[j] };
        __engines[0].execute(packed, spectrum, temp, false);
        for ( usize k = 0; k <= h; ++k ) out[k] = spectrum[k] * scale;
      }
      return max_t(__output_volume * sizeof(complex<F>));
    }
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return complex<F>{ in[i] }; }, [&](ssize_t i, complex<F> z) { out[i] = z; }, work, direction::forward,
                     runner);
  }

  max_t
  execute(const complex<F> *in, F *out, workspace_view<F> work) const noexcept
  {
    if ( __desc.kind != domain::real_backward || (__desc.batches && (!in || !out)) ) return -22;
    if ( __desc.shape.size() == 1 && __desc.batches == 1 && __input_dense && __output_dense ) {
      if ( !work.data || work.size < __data_size + __scratch_size ) return -105;
      in += __desc.input.offset;
      out += __desc.output.offset;
      auto *packed = work.data, *time = packed + __volume, *temp = time + __volume;
      usize n = __volume, h = n / 2;
      F scale = __impl::__scale<F>(__desc.norm, direction::backward, n) * __desc.backward_scale;
      if ( n % 2 == 0 ) {
        for ( usize k = 0; k < h; ++k ) {
          auto x = in[k], y = conj(in[h - k]);
          auto difference = (x - y) * conj(__real_twiddles[k]);
          packed[k] = (x + y + complex<F>{ -difference.imag, difference.real }) * F(0.5);
        }
        __engines[0].execute(packed, time, temp, true);
        for ( usize k = 0; k < h; ++k ) {
          out[2 * k] = time[k].real * (F(2) * scale);
          out[2 * k + 1] = time[k].imag * (F(2) * scale);
        }
      } else {
        for ( usize k = 0; k <= h; ++k ) packed[k] = in[k];
        for ( usize k = h + 1; k < n; ++k ) packed[k] = conj(in[n - k]);
        __engines[0].execute(packed, time, temp, true);
        for ( usize k = 0; k < n; ++k ) out[k] = time[k].real * scale;
      }
      return max_t(__output_volume * sizeof(F));
    }
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return in[i]; }, [&](ssize_t i, complex<F> z) { out[i] = z.real; }, work, direction::backward,
                     runner);
  }

  max_t
  execute(const F *in, split_output<F> out, workspace_view<F> work) const noexcept
  {
    if ( __desc.kind != domain::real_forward || (__desc.batches && (!in || !out.real || !out.imag)) ) return -22;
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return complex<F>{ in[i] }; },
                     [&](ssize_t i, complex<F> z) {
                       out.real[i] = z.real;
                       out.imag[i] = z.imag;
                     },
                     work, direction::forward, runner);
  }

  max_t
  execute(split_input<F> in, F *out, workspace_view<F> work) const noexcept
  {
    if ( __desc.kind != domain::real_backward || (__desc.batches && (!in.real || !in.imag || !out)) ) return -22;
    __impl::__serial<F> runner;
    return __execute([&](ssize_t i) { return complex<F>{ in.real[i], in.imag[i] }; }, [&](ssize_t i, complex<F> z) { out[i] = z.real; },
                     work, direction::backward, runner);
  }

  // Spectrum strides count complex pairs; real-domain strides count F elements.
  // Padding and capacity belong to the caller. No complex objects alias F storage.
  max_t
  execute_inplace(F *storage, workspace_view<F> work) const noexcept
  {
    if ( __desc.kind == domain::complex || (__desc.batches && !storage) ) return -22;
    __impl::__serial<F> runner;
    if ( __desc.kind == domain::real_forward )
      return __execute([&](ssize_t i) { return complex<F>{ storage[i] }; },
                       [&](ssize_t i, complex<F> z) {
                         storage[2 * i] = z.real;
                         storage[2 * i + 1] = z.imag;
                       },
                       work, direction::forward, runner);
    return __execute([&](ssize_t i) { return complex<F>{ storage[2 * i], storage[2 * i + 1] }; },
                     [&](ssize_t i, complex<F> z) { storage[i] = z.real; }, work, direction::backward, runner);
  }
};

template<fft_scalar F>
buffer<complex<F>>
transform(const complex<F> *input, usize n, direction dir = direction::forward, normalization norm = normalization::backward)
{
  plan<F> p(n, norm);
  workspace<F> work(p);
  buffer<complex<F>> result(n);
  if ( p.execute(input, result.data(), work.view(), dir) < 0 ) exc<except::invalid_argument>("fft: invalid input");
  return result;
}

template<fft_scalar F>
buffer<complex<F>>
rfft(const F *input, usize n, normalization norm = normalization::backward)
{
  descriptor<F> d(n);
  d.kind = domain::real_forward;
  d.norm = norm;
  plan<F> p(d);
  workspace<F> work(p);
  buffer<complex<F>> result(p.output_size());
  if ( p.execute(input, result.data(), work.view()) < 0 ) exc<except::invalid_argument>("fft: invalid input");
  return result;
}

template<fft_scalar F>
buffer<F>
irfft(const complex<F> *input, usize n, normalization norm = normalization::backward)
{
  descriptor<F> d(n);
  d.kind = domain::real_backward;
  d.norm = norm;
  plan<F> p(d);
  workspace<F> work(p);
  buffer<F> result(n);
  if ( p.execute(input, result.data(), work.view()) < 0 ) exc<except::invalid_argument>("fft: invalid input");
  return result;
}

template<fft_scalar F>
buffer<complex<F>>
ifft(const complex<F> *input, usize n, normalization norm = normalization::backward)
{
  return transform(input, n, direction::backward, norm);
}

}      // namespace micron::math::fft
