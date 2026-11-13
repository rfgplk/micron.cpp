//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft.hpp"
#include "../../src/memory/allocation/abcmalloc/stats.hpp"
#include "../snowball/snowball.hpp"
namespace ft = micron::math::fft;
using ft::buffer;
using micron::math::complex;
#include "../support/fft/oracle.hpp"

static void
layouts()
{
  ft::descriptor<double> d({ 3, 4, 5 });
  d.axes = { 2, 0 };
  d.batches = 3;
  d.input.strides = { 90, 21, -3 };
  d.input.offset = 12;
  d.input.distance = 1;
  d.output.strides = { 28, 7, 1 };
  d.output.offset = 2;
  d.output.distance = 84;
  buffer<complex<double>> input(300), output(300, complex<double>{ 987, 654 }), expected(180);
  for ( usize b = 0; b < 3; ++b )
    for ( usize a = 0; a < 3; ++a )
      for ( usize c = 0; c < 4; ++c )
        for ( usize e = 0; e < 5; ++e ) input[12 + b + 90 * a + 21 * c - 3 * e] = { double(sample()), double(sample()) };
  for ( auto mode : { ft::normalization::none, ft::normalization::backward, ft::normalization::forward, ft::normalization::ortho } ) {
    d.norm = mode;
    d.forward_scale = 2;
    ft::plan<double> p(d);
    ft::workspace<double> w(p);
    auto before = abc::stats();
    auto result = p.execute(input.data(), output.data(), w.view());
    auto after = abc::stats();
    sb::require_true(result == 180 * max_t(sizeof(complex<double>)));
#if MICRON_ABC_STATS
    sb::require_true(before.enabled && after.alloc_requests == before.alloc_requests && after.dealloc_requests == before.dealloc_requests);
#else
    (void)before;
    (void)after;
#endif
    for ( usize b = 0; b < 3; ++b )
      for ( usize a = 0; a < 3; ++a )
        for ( usize c = 0; c < 4; ++c )
          for ( usize e = 0; e < 5; ++e ) {
            complex<long double> sum{};
            for ( usize i = 0; i < 3; ++i )
              for ( usize j = 0; j < 5; ++j ) {
                long double angle = -6.283185307179586476925286766559005768L
                                    * (static_cast<long double>(i * a) / 3 + static_cast<long double>(j * e) / 5);
                sum += complex<long double>(input[12 + b + 90 * i + 21 * c - 3 * j]) * complex<long double>{ cosine(angle), sine(angle) };
              }
            long double scale = 2;
            if ( mode == ft::normalization::forward ) scale /= 15;
            if ( mode == ft::normalization::ortho ) scale /= 3.8729833462074168851792653997823996108L;
            auto actual = output[2 + 84 * b + 28 * a + 7 * c + e];
            check(actual.real, sum.real * scale, 2e-12L);
            check(actual.imag, sum.imag * scale, 2e-12L);
          }
    sb::require_true(output[0] == complex<double>(987, 654));
    sb::require_true(output[7] == complex<double>(987, 654));
    auto saved = output;
    sb::require_true(p.execute(input.data(), output.data(), { w.view().data, w.size() - 1 }) == -105);
    for ( usize i = 0; i < 300; ++i ) sb::require_true(saved[i] == output[i]);
  }
  buffer<double> re(300), im(300), ore(300), oim(300);
  for ( usize i = 0; i < 300; ++i ) {
    re[i] = input[i].real;
    im[i] = input[i].imag;
  }
  ft::plan<double> p(d);
  ft::workspace<double> w(p);
  sb::require_true(p.execute(ft::split_input<double>{ re.data(), im.data() }, ft::split_output<double>{ ore.data(), oim.data() }, w.view())
                   > 0);
  for ( usize b = 0; b < 3; ++b )
    for ( usize a = 0; a < 3; ++a )
      for ( usize c = 0; c < 4; ++c )
        for ( usize e = 0; e < 5; ++e ) {
          usize i = 2 + 84 * b + 28 * a + 7 * c + e;
          check(ore[i], output[i].real, 1e-12L);
          check(oim[i], output[i].imag, 1e-12L);
        }
}

static void
real_nd()
{
  for ( usize last : { usize(5), usize(6) } )
    for ( auto axes : { buffer<usize>{ 0, 2 }, buffer<usize>{ 2, 0 }, buffer<usize>{ 2, 1 } } ) {
      ft::descriptor<double> d({ 3, 4, last });
      d.axes = axes;
      d.batches = 2;
      usize volume = 3 * 4 * last;
      buffer<double> in(volume * 2), restored(volume * 2);
      buffer<complex<double>> c(volume * 2), full(volume * 2);
      for ( usize i = 0; i < in.size(); ++i ) {
        in[i] = double(sample());
        c[i] = { in[i] };
      }
      ft::plan<double> cp(d);
      ft::workspace<double> cw(cp);
      cp.execute(c.data(), full.data(), cw.view());
      d.kind = ft::domain::real_forward;
      ft::plan<double> p(d);
      ft::workspace<double> w(p);
      buffer<complex<double>> out(p.output_size());
      sb::require_true(p.execute(in.data(), out.data(), w.view()) > 0);
      const auto &shape = p.output_shape();
      usize packed = shape[0] * shape[1] * shape[2];
      for ( usize b = 0; b < 2; ++b )
        for ( usize i = 0; i < shape[0]; ++i )
          for ( usize j = 0; j < shape[1]; ++j )
            for ( usize k = 0; k < shape[2]; ++k ) {
              auto z = out[b * packed + (i * shape[1] + j) * shape[2] + k], ref = full[b * volume + (i * 4 + j) * last + k];
              check(z.real, ref.real, 1e-11L);
              check(z.imag, ref.imag, 1e-11L);
            }
      d.kind = ft::domain::real_backward;
      ft::plan<double> back(d);
      ft::workspace<double> bw(back);
      auto preserve = out;
      sb::require_true(back.execute(out.data(), restored.data(), bw.view()) > 0);
      for ( usize i = 0; i < in.size(); ++i ) check(restored[i], in[i], 1e-12L);
      for ( usize i = 0; i < out.size(); ++i ) sb::require_true(out[i] == preserve[i]);
    }
  for ( usize n : { usize(5), usize(6) } ) {
    usize slots = 2 * (n / 2 + 1), size = 6 * slots;
    buffer<double> data(size + 2, 321.0), original(size + 2);
    for ( usize r = 0; r < 6; ++r )
      for ( usize j = 0; j < n; ++j ) data[r * slots + j] = double(sample());
    original = data;
    ft::descriptor<double> d({ 3, n });
    d.batches = 2;
    d.kind = ft::domain::real_forward;
    d.input.strides = { ssize_t(slots), 1 };
    d.input.distance = ssize_t(3 * slots);
    ft::plan<double> f(d);
    ft::workspace<double> w(f);
    sb::require_true(f.execute_inplace(data.data(), w.view()) > 0);
    d.kind = ft::domain::real_backward;
    d.output = d.input;
    d.input = {};
    ft::plan<double> b(d);
    ft::workspace<double> v(b);
    sb::require_true(b.execute_inplace(data.data(), v.view()) > 0);
    for ( usize r = 0; r < 6; ++r )
      for ( usize j = 0; j < n; ++j ) check(data[r * slots + j], original[r * slots + j], 1e-12L);
    sb::require_true(data[size] == 321 && data[size + 1] == 321);
  }
}

static void
convolution()
{
  for ( usize n = 1; n < 22; ++n )
    for ( usize m = 1; m < 12; ++m ) {
      buffer<complex<double>> x(n), y(m), oracle(n + m - 1);
      for ( auto &z : x ) z = { double(sample()), double(sample()) };
      for ( auto &z : y ) z = { double(sample()), double(sample()) };
      for ( bool correlation : { false, true } ) {
        for ( auto &z : oracle ) z = {};
        for ( usize i = 0; i < n; ++i )
          for ( usize j = 0; j < m; ++j )
            oracle[i + (correlation ? m - 1 - j : j)] += x[i] * (correlation ? micron::math::conj(y[j]) : y[j]);
        for ( auto mode :
              { ft::convolution_mode::full, ft::convolution_mode::same, ft::convolution_mode::valid, ft::convolution_mode::circular } ) {
          if ( mode == ft::convolution_mode::valid && n < m ) continue;
          ft::convolution_descriptor d(n, m);
          d.mode = mode;
          d.correlation = correlation;
          usize period = n > m ? n : m;
          d.period = { period };
          ft::convolution_plan<double> p(d, y.data());
          ft::workspace<double> w(p);
          buffer<complex<double>> out(p.output_size());
          sb::require_true(p.execute(x.data(), out.data(), w.view()) > 0);
          if ( mode == ft::convolution_mode::circular ) {
            buffer<complex<double>> ref(period);
            for ( usize i = 0; i < n; ++i )
              for ( usize j = 0; j < m; ++j )
                ref[correlation ? (i + period - j) % period : (i + j) % period] += x[i] * (correlation ? micron::math::conj(y[j]) : y[j]);
            for ( usize k = 0; k < period; ++k ) {
              check(out[k].real, ref[k].real, 1e-11L);
              check(out[k].imag, ref[k].imag, 1e-11L);
            }
          } else {
            usize offset = mode == ft::convolution_mode::same ? (m - 1) / 2 : mode == ft::convolution_mode::valid ? m - 1 : 0;
            for ( usize k = 0; k < out.size(); ++k ) {
              check(out[k].real, oracle[k + offset].real, 1e-11L);
              check(out[k].imag, oracle[k + offset].imag, 1e-11L);
            }
          }
        }
      }
    }
  double x[12], kernel[4] = { 2, -1, 3, 4 };
  for ( usize i = 0; i < 12; ++i ) x[i] = double(i) - 5;
  ft::convolution_descriptor d(buffer<usize>{ 3, 4 }, buffer<usize>{ 2, 2 });
  auto out = ft::convolve(x, kernel, d);
  double oracle[20]{};
  for ( usize i = 0; i < 3; ++i )
    for ( usize j = 0; j < 4; ++j )
      for ( usize a = 0; a < 2; ++a )
        for ( usize b = 0; b < 2; ++b ) oracle[(i + a) * 5 + j + b] += x[i * 4 + j] * kernel[a * 2 + b];
  for ( usize k = 0; k < 20; ++k ) check(out[k], oracle[k], 1e-11L);
  auto empty = ft::convolve(static_cast<const double *>(nullptr), 0, kernel, 4);
  sb::require_true(empty.empty());
}

static void
helpers_and_errors()
{
  for ( usize n = 1; n < 2000; ++n ) {
    usize fast = ft::next_fast_len(n);
    sb::require_true(fast >= n);
    usize k = fast;
    for ( usize r : { 2, 3, 5, 7 } )
      while ( k % r == 0 ) k /= r;
    sb::require_true(k == 1);
    for ( usize i = n; i < fast; ++i ) {
      usize v = i;
      for ( usize r : { 2, 3, 5, 7 } )
        while ( v % r == 0 ) v /= r;
      sb::require_true(v != 1);
    }
  }
  for ( usize n = 1; n < 30; ++n ) {
    buffer<double> x(n);
    for ( usize i = 0; i < n; ++i ) x[i] = double(i);
    auto saved = x;
    ft::fftshift(x.data(), n);
    ft::ifftshift(x.data(), n);
    for ( usize i = 0; i < n; ++i ) sb::require_true(x[i] == saved[i]);
  }
  auto f = ft::fftfreq<double>(4, 0.5);
  sb::require_true(f[0] == 0 && f[1] == 0.5 && f[2] == -1 && f[3] == -0.5);
  auto r = ft::rfftfreq<double>(5);
  check(r[2], 0.4L, 1e-16L);
  ft::descriptor<double> d(8);
  d.batches = 0;
  ft::plan<double> p(d);
  sb::require_true(p.execute(static_cast<const complex<double> *>(nullptr), static_cast<complex<double> *>(nullptr), {}) == 0);
#if defined(__cpp_exceptions)
  unsigned caught = 0;
  try {
    ft::plan<double> bad(0);
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::descriptor<double> x({ 3, 4 });
    x.axes = { 0, 0 };
    ft::plan<double> bad(x);
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::descriptor<double> x({ 3, 4 });
    x.axes = { 2 };
    ft::plan<double> bad(x);
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::descriptor<double> x({ 3, 4 });
    x.input.strides = { 1 };
    ft::plan<double> bad(x);
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::descriptor<double> x({ 3, 4 });
    x.input.strides = { micron::numeric_limits<intptr_t>::max(), 1 };
    ft::plan<double> bad(x);
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::plan<double> bad{ usize(micron::numeric_limits<intptr_t>::max()) };
  } catch ( ... ) {
    ++caught;
  }
  try {
    ft::r2r_plan<double> bad(1, ft::r2r_kind::dct_i);
  } catch ( ... ) {
    ++caught;
  }
  sb::require_true(caught == 7);
#endif
}

static void
allocation_contract()
{
  ft::descriptor<double> d(37);
  d.kind = ft::domain::real_forward;
  ft::plan<double> p(d);
  ft::workspace<double> w(p);
  ft::r2r_plan<double> r(37, ft::r2r_kind::dst_iv);
  ft::workspace<double> rw(r);
  double input[37]{}, kernel[3] = { 1, 2, 1 }, output[39]{};
  complex<double> spectrum[19];
  ft::convolution_plan<double> c(ft::convolution_descriptor(37, 3), kernel);
  ft::workspace<double> cw(c);
  auto before = abc::stats();
  for ( unsigned i = 0; i < 5; ++i ) {
    p.execute(input, spectrum, w.view());
    r.execute(input, output, rw.view());
    c.execute(input, output, cw.view());
  }
  auto after = abc::stats();
#if MICRON_ABC_STATS
  sb::require_true(before.enabled && after.alloc_requests == before.alloc_requests && after.dealloc_requests == before.dealloc_requests);
#else
  (void)before;
  (void)after;
#endif
  ft::plan<double> small(7);
  complex<double> external[64], in[7]{}, out[7];
  in[0] = { 1 };
  sb::require_true(small.execute(in, out, { external, 64 }) == sizeof(out));
  for ( auto z : out ) sb::require_true(z == complex<double>(1));
  auto inverse = ft::ifft(out, 7);
  for ( usize i = 0; i < 7; ++i ) check(inverse[i].real, in[i].real, 1e-12L);
  auto cosine = ft::dct(input, 37), sine = ft::dst(input, 37);
  sb::require_true(cosine.size() == 37 && sine.size() == 37);
  buffer<usize> shape{ 3, 5 };
  auto shifted = ft::fftshift(input, shape);
  auto restored = ft::ifftshift(shifted.data(), shape);
  for ( usize i = 0; i < 15; ++i ) sb::require_true(restored[i] == input[i]);
}

int
main()
{
  sb::test_case("Signed strides, interleaved batches, axes, split storage, scales and workspace contracts");
  layouts();
  sb::end_test_case();
  sb::test_case("ND packed spectra, selected-axis Hermitian reconstruction and padded in-place real storage");
  real_nd();
  sb::end_test_case();
  sb::test_case("Cached convolution and correlation, full/same/valid/circular and ND direct oracles");
  convolution();
  sb::end_test_case();
  sb::test_case("Frequency/shift helpers, fast lengths, zero batches and rejected descriptors");
  helpers_and_errors();
  sb::end_test_case();
  sb::test_case("Prepared real, real-to-real and convolution execution allocates nothing; external scratch and conveniences");
  allocation_contract();
  sb::end_test_case();
  return 1;
}
