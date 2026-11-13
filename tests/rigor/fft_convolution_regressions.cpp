//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft/convolution.hpp"
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

int
main()
{
  sb::test_case("Empty multidimensional convolution does not compute unrepresentable padding volumes");
  constexpr usize side = usize(1) << (sizeof(usize) == 4 ? 12 : 22);
  for ( usize empty_axis = 0; empty_axis < 4; ++empty_axis ) {
    ft::buffer<usize> shape{ side, side, side, side };
    shape[empty_axis] = 0;
    for ( auto mode :
          { ft::convolution_mode::full, ft::convolution_mode::same, ft::convolution_mode::valid, ft::convolution_mode::circular } ) {
      ft::convolution_descriptor d(shape, shape);
      d.mode = mode;
      d.period = { side, side, side, side };
      ft::convolution_plan<double> p(d, static_cast<const double *>(nullptr));
      sb::require_true(p.output_size() == 0 && p.workspace_size() == 0);
      for ( usize n : p.output_shape() ) sb::require_true(n == 0);
      sb::require_true(p.execute(static_cast<const double *>(nullptr), static_cast<double *>(nullptr), {}) == 0);
    }
  }
  sb::end_test_case();
  return 1;
}
