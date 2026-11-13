//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
// duck build benches/complex_div_bench.cpp --std c++23 --isa base --perf --fp --no-ssp --no-lto -i . -i ./src -o bin/fft/division-bench
#include "../src/chrono/clock.hpp"
#include "../src/io/console.hpp"
#include "../src/math/complex.hpp"

using micron::math::complex;

template<typename F>
static complex<F>
smith_reference(complex<F> a, complex<F> b)
{
  F ar = b.real < 0 ? -b.real : b.real, ai = b.imag < 0 ? -b.imag : b.imag;
  if ( ar >= ai ) {
    F r = b.imag / b.real, divisor = b.real + b.imag * r;
    return { (a.real + a.imag * r) / divisor, (a.imag - a.real * r) / divisor };
  }
  F r = b.real / b.imag, divisor = b.imag + b.real * r;
  return { (a.real * r + a.imag) / divisor, (a.imag * r - a.real) / divisor };
}

template<typename F, bool Reference>
[[gnu::noinline]] static double
measure()
{
  complex<F> input[1024], denominator[1024], output[1024];
  for ( usize i = 0; i < 1024; ++i ) {
    input[i] = { F(int(i % 13) - 6) / F(8), F(int(i % 19) - 9) / F(16) };
    denominator[i] = { F(i % 29 + 1) / F(16), F(int(i % 23) - 11) / F(32) };
  }
  double samples[7];
  for ( auto &sample : samples ) {
    auto start = micron::chrono::mono_ns();
    for ( unsigned repeat = 0; repeat < 1024; ++repeat ) {
      asm volatile("" ::: "memory");
      for ( usize i = 0; i < 1024; ++i ) {
        if constexpr ( Reference )
          output[i] = smith_reference(input[i], denominator[i]);
        else
          output[i] = input[i] / denominator[i];
      }
      asm volatile("" : : "g"(output) : "memory");
    }
    sample = double(micron::chrono::mono_ns() - start) / 1048576;
  }
  for ( unsigned i = 1; i < 7; ++i )
    for ( unsigned j = i; j && samples[j] < samples[j - 1]; --j ) {
      auto value = samples[j];
      samples[j] = samples[j - 1];
      samples[j - 1] = value;
    }
  volatile F sink = output[17].real;
  (void)sink;
  return samples[3];
}

template<typename F>
static void
run(const char *precision)
{
  for ( unsigned round = 0; round < 7; ++round ) {
    double reference, current;
    if ( round & 1 ) {
      current = measure<F, false>();
      reference = measure<F, true>();
    } else {
      reference = measure<F, true>();
      current = measure<F, false>();
    }
    micron::io::println(precision, ",", round, ",", reference, ",", current, ",", reference / current);
  }
}

int
main()
{
  micron::io::println("precision,round,smith_ns,current_ns,speedup");
  run<float>("f32");
  run<double>("f64");
  run<long double>("long_double");
  return 0;
}
