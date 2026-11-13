//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
// duck build benches/fft_bench.cpp --std c++23 --isa base --perf --fp --no-ssp --no-lto -i . -i ./src -o bin/fft/bench
#include "../src/chrono/clock.hpp"
#include "../src/io/console.hpp"
#include "../src/math/fft.hpp"
namespace ft = micron::math::fft;
using micron::math::complex;
static volatile double sink;

template<typename Function>
void
measure(const char *kind, usize n, usize iterations, Function &&fn)
{
  double samples[7];
  for ( auto &value : samples ) {
    auto start = micron::chrono::mono_ns();
    for ( usize i = 0; i < iterations; ++i ) fn();
    value = double(micron::chrono::mono_ns() - start) / double(iterations);
  }
  for ( usize i = 1; i < 7; ++i )
    for ( usize j = i; j && samples[j] < samples[j - 1]; --j ) {
      auto t = samples[j];
      samples[j] = samples[j - 1];
      samples[j - 1] = t;
    }
  micron::io::println(kind, ",", n, ",", iterations, ",", samples[3], ",", samples[0], ",", samples[6]);
}

int
main()
{
  micron::io::println("operation,n,iterations,median_ns,min_ns,max_ns");
  for ( usize n : { usize(16), usize(64), usize(256), usize(1000), usize(1009), usize(1024), usize(4096), usize(16384) } ) {
    ft::buffer<complex<double>> in(n), out(n);
    for ( usize i = 0; i < n; ++i ) in[i] = { double(int(i % 13) - 6) / 8, double(int(i % 7) - 3) / 16 };
    ft::plan<double> p(n);
    ft::workspace<double> w(p);
    p.execute(in.data(), out.data(), w.view());
    usize iterations = 200000 / n + 8;
    measure("c2c_execute", n, iterations, [&] {
      p.execute(in.data(), out.data(), w.view());
      asm volatile("" ::"g"(out.data()) : "memory");
    });
    sink = out[n / 3].real;
    measure("plan_lifecycle", n, 7, [&] {
      ft::plan<double> fresh(n);
      sink = double(fresh.workspace_size());
    });
    measure("workspace_lifecycle", n, 7, [&] {
      ft::workspace<double> fresh(p);
      sink = double(fresh.size());
    });
    ft::buffer<double> real(n), back(n);
    for ( usize i = 0; i < n; ++i ) real[i] = in[i].real;
    ft::descriptor<double> d(n);
    d.kind = ft::domain::real_forward;
    ft::plan<double> rp(d);
    ft::workspace<double> rw(rp);
    measure("r2c_execute", n, iterations, [&] {
      rp.execute(real.data(), out.data(), rw.view());
      asm volatile("" ::"g"(out.data()) : "memory");
    });
    sink = out[n / 3].real;
    ft::r2r_plan<double> dp(n, ft::r2r_kind::dct_ii);
    ft::workspace<double> dw(dp);
    measure("dct_ii_execute", n, iterations, [&] {
      dp.execute(real.data(), back.data(), dw.view());
      asm volatile("" ::"g"(back.data()) : "memory");
    });
    sink = back[n / 3];
    ft::convolution_descriptor cd(n, 31);
    double kernel[31];
    for ( usize j = 0; j < 31; ++j ) kernel[j] = double(j) / 31;
    ft::convolution_plan<double> cp(cd, kernel);
    ft::workspace<double> cw(cp);
    ft::buffer<double> conv(cp.output_size());
    measure("fixed_convolve", n, iterations, [&] {
      cp.execute(real.data(), conv.data(), cw.view());
      asm volatile("" ::"g"(conv.data()) : "memory");
    });
    sink = conv[n / 3];
  }
  for ( usize n : { usize(32), usize(128) } ) {
    ft::descriptor<double> d({ n, n });
    ft::plan<double> p(d);
    ft::workspace<double> w(p);
    ft::buffer<complex<double>> x(n * n), y(n * n);
    for ( usize i = 0; i < n * n; ++i ) x[i] = { double(i % 13) / 8 };
    measure("c2c_2d_execute", n, 16, [&] {
      p.execute(x.data(), y.data(), w.view());
      asm volatile("" ::"g"(y.data()) : "memory");
    });
    sink = y[n].real;
  }
  return 0;
}
