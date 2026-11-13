//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft/packet.hpp"
#include "../snowball/snowball.hpp"

namespace ft = micron::math::fft;
using micron::math::complex;
#include "../support/fft/oracle.hpp"

template<typename F>
F
scaled_sample()
{
  F v = F(sample());
  v = v < 0 ? v * F(0.5) - F(0.5) : v * F(0.5) + F(0.5);
  int exponent = int((random_state >> 32) % 81) - 40;
  while ( exponent > 0 ) {
    v *= F(2);
    --exponent;
  }
  while ( exponent < 0 ) {
    v *= F(0.5);
    ++exponent;
  }
  return v;
}

template<typename F>
void
product_check(complex<F> actual, complex<F> a, complex<F> b)
{
  long double ar = a.real, ai = a.imag, br = b.real, bi = b.imag;
  long double scale = absolute(ar * br) + absolute(ai * bi) + absolute(ar * bi) + absolute(ai * br);
  check(actual.real, ar * br - ai * bi, epsilon<F>() * scale * 8);
  check(actual.imag, ar * bi + ai * br, epsilon<F>() * scale * 8);
}

template<typename F, usize N = 1>
[[gnu::noinline]] void
exact_extents()
{
  // Stack redzones expose overreads that the allocator's spare capacity can hide.
  complex<F> left[N], right[N], before[N];
  for ( usize iteration = 0; iteration < 128; ++iteration ) {
    for ( usize j = 0; j < N; ++j ) {
      before[j] = left[j] = { scaled_sample<F>(), scaled_sample<F>() };
      right[j] = { scaled_sample<F>(), scaled_sample<F>() };
    }
    ft::__impl::__multiply(left, right, N);
    for ( usize j = 0; j < N; ++j ) product_check(left[j], before[j], right[j]);
    for ( usize j = 0; j < N; ++j ) left[j] = before[j];
    ft::__impl::__multiply(left, left, N);
    for ( usize j = 0; j < N; ++j ) product_check(left[j], before[j], before[j]);
  }
  if constexpr ( N < 17 ) exact_extents<F, N + 1>();
}

template<typename F>
void
packets()
{
  using P = ft::__impl::__complex_packet<F>;
  complex<F> input[P::width], other[P::width], backward[P::width], forward[P::width], conjugated[P::width], result[P::width];
  for ( usize iteration = 0; iteration < 256; ++iteration ) {
    for ( auto &z : input ) z = { scaled_sample<F>(), scaled_sample<F>() };
    for ( auto &z : other ) z = { scaled_sample<F>(), scaled_sample<F>() };
    auto v = P::load(input), w = P::load(other);
    P::store(conjugated, P::conjugate(v));
    P::store(backward, P::template rotate<true>(v));
    P::store(forward, P::template rotate<false>(v));
    for ( usize j = 0; j < P::width; ++j ) {
      sb::require_true(conjugated[j] == complex<F>{ input[j].real, -input[j].imag });
      sb::require_true(backward[j] == complex<F>{ -input[j].imag, input[j].real });
      sb::require_true(forward[j] == complex<F>{ input[j].imag, -input[j].real });
    }
    P::store(result, v);
    for ( usize j = 0; j < P::width; ++j ) sb::require_true(result[j] == input[j]);
    P::store(result, P::add(v, w));
    for ( usize j = 0; j < P::width; ++j ) {
      check(result[j].real, (long double)input[j].real + other[j].real,
            epsilon<F>() * (absolute(input[j].real) + absolute(other[j].real)) * 2);
      check(result[j].imag, (long double)input[j].imag + other[j].imag,
            epsilon<F>() * (absolute(input[j].imag) + absolute(other[j].imag)) * 2);
    }
    P::store(result, P::sub(v, w));
    for ( usize j = 0; j < P::width; ++j ) {
      check(result[j].real, (long double)input[j].real - other[j].real,
            epsilon<F>() * (absolute(input[j].real) + absolute(other[j].real)) * 2);
      check(result[j].imag, (long double)input[j].imag - other[j].imag,
            epsilon<F>() * (absolute(input[j].imag) + absolute(other[j].imag)) * 2);
    }
    F factor = scaled_sample<F>();
    P::store(result, P::scale(v, factor));
    for ( usize j = 0; j < P::width; ++j ) {
      check(result[j].real, (long double)input[j].real * factor, epsilon<F>() * absolute((long double)input[j].real * factor) * 2);
      check(result[j].imag, (long double)input[j].imag * factor, epsilon<F>() * absolute((long double)input[j].imag * factor) * 2);
    }
    P::store(result, P::mul(v, w));
    for ( usize j = 0; j < P::width; ++j ) product_check(result[j], input[j], other[j]);
  }
  exact_extents<F>();
}

int
main()
{
  sb::test_case("FFT packet forward/inverse rotations retain distinct sign masks under fast math");
  ft::__impl::__multiply<float>(nullptr, nullptr, 0);
  ft::__impl::__multiply<double>(nullptr, nullptr, 0);
  ft::__impl::__multiply<long double>(nullptr, nullptr, 0);
  packets<float>();
  packets<double>();
  packets<long double>();
  sb::end_test_case();
  return 1;
}
