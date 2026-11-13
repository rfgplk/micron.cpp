//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft/helpers.hpp"
#include "../snowball/snowball.hpp"

namespace ft = micron::math::fft;

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1";
}

template<typename F>
static void
frequency_range(F largest, F smallest)
{
  auto signed_bins = ft::fftfreq<F>(4, largest / F(2));
  auto real_bins = ft::rfftfreq<F>(4, largest / F(2));
  volatile F reference = F(0.5) / largest, nyquist = F(1) / largest;
  sb::require_true(reference > F(0));
  sb::require_true(signed_bins[0] == F(0) && real_bins[0] == F(0));
  sb::require_true(signed_bins[1] == reference && real_bins[1] == reference);
  sb::require_true(signed_bins[2] == -nyquist && real_bins[2] == nyquist);
  sb::require_true(signed_bins[3] == -reference);

  signed_bins = ft::fftfreq<F>(4, smallest / F(32));
  real_bins = ft::rfftfreq<F>(4, smallest / F(32));
  sb::require_true(signed_bins[0] == F(0) && real_bins[0] == F(0));
  sb::require_true(signed_bins[1] > largest && real_bins[1] > largest);
  sb::require_true(signed_bins[2] < -largest && real_bins[2] > largest);
}

int
main()
{
  sb::test_case("Frequency bins preserve representable extreme scales and the zero-frequency bin");
  // ARMv7 NEON follows the documented flush-to-zero float contract.
#if !defined(__micron_arch_arm32) || defined(__micron_simd_generic)
  frequency_range<float>(micron::numeric_limits<float>::max(), micron::numeric_limits<float>::min());
#endif
  frequency_range<double>(micron::numeric_limits<double>::max(), micron::numeric_limits<double>::min());
  // The freestanding binary128 runtime also documents subnormal-result flushing.
#if !defined(__micron_arch_arm64)
  frequency_range<long double>(micron::numeric_limits<long double>::max(), micron::numeric_limits<long double>::min());
#endif
  sb::end_test_case();
  return 1;
}
