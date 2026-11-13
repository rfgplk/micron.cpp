//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "arch/packet_generic.hpp"
#if !defined(MICRON_NO_SIMD) && !defined(__micron_simd_generic)
#if defined(__micron_x86_sse2)
#include "arch/packet_amd64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/packet_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/packet_arm32.hpp"
#endif
#endif

namespace micron::math::fft::__impl
{
template<fft_scalar F>
void
__multiply(complex<F> *left, const complex<F> *right, usize n) noexcept
{
  using P = __complex_packet<F>;
  const usize vector_end = n - n % P::width;
  for ( usize k = 0; k < vector_end; k += P::width ) P::store(left + k, P::mul(P::load(left + k), P::load(right + k)));
  for ( usize tail = 0; tail < n % P::width; ++tail ) left[vector_end + tail] *= right[vector_end + tail];
}
};      // namespace micron::math::fft::__impl
