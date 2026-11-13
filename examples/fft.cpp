//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../src/math/fft.hpp"
#include "../src/io/console.hpp"

#if defined(__micron_freestanding)
extern "C"
#endif
    int
    main()
{
  namespace ft = micron::math::fft;
  double signal[9]{ 1, 2, 3, 4, 5, 4, 3, 2, 1 };
  auto spectrum = ft::rfft(signal, 9);
  auto restored = ft::irfft(spectrum.data(), 9);
  micron::io::println("DC: ", spectrum[0].real, "; recovered sample 4: ", restored[4]);

  ft::descriptor<double> description({ 3, 6 });
  description.kind = ft::domain::real_forward;
  description.input.strides = { 8, 1 };
  ft::plan<double> forward(description);
  ft::workspace<double> scratch(forward);
  double padded[24]{};
  padded[0] = 1;
  micron::io::println("Packed 2D output bytes: ", forward.execute_inplace(padded, scratch.view()));

  ft::convolution_descriptor convolution(9, 3);
  convolution.mode = ft::convolution_mode::same;
  double kernel[3]{ 0.25, 0.5, 0.25 };
  ft::convolution_plan<double> filter(convolution, kernel);
  ft::workspace<double> filter_scratch(filter);
  double smoothed[9];
  filter.execute(signal, smoothed, filter_scratch.view());
  micron::io::println("Smoothed sample 4: ", smoothed[4]);
  return 0;
}
