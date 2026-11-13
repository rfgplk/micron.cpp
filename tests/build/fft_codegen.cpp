//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
// Stable entry points for objdump, compiler remarks and llvm-mca. Compile with --raw-obj or --asm.
#include "../../src/math/fft/kernels.hpp"
namespace ft = micron::math::fft;
using micron::math::complex;

#define FFT_PROBE(F, SUFFIX, R)                                                                                                            \
  extern "C" [[gnu::noinline]] void fft_stage##R##_##SUFFIX(complex<F> *out, const complex<F> *tw, usize m) noexcept                       \
  {                                                                                                                                        \
    ft::__impl::__stage<R, false>(out, tw, m, 0, m);                                                                                       \
  }
FFT_PROBE(float, f32, 2)
FFT_PROBE(float, f32, 3)
FFT_PROBE(float, f32, 4)
FFT_PROBE(float, f32, 5)
FFT_PROBE(float, f32, 7)
FFT_PROBE(double, f64, 2)
FFT_PROBE(double, f64, 3)
FFT_PROBE(double, f64, 4)
FFT_PROBE(double, f64, 5)
FFT_PROBE(double, f64, 7)
#undef FFT_PROBE

extern "C" [[gnu::noinline]] void
fft_multiply_f32(complex<float> *out, const complex<float> *right, usize n) noexcept
{
  ft::__impl::__multiply(out, right, n);
}

extern "C" [[gnu::noinline]] void
fft_multiply_f64(complex<double> *out, const complex<double> *right, usize n) noexcept
{
  ft::__impl::__multiply(out, right, n);
}
