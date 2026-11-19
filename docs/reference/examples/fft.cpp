#include <micron/math/fft.hpp>

int main()
{
  double samples[4]{ 1, 0, 0, 0 };
  auto spectrum = micron::math::fft::rfft(samples, 4);
  auto restored = micron::math::fft::irfft(spectrum.data(), 4);
  return restored[0] > 0.999 && restored[0] < 1.001 ? 0 : 1;
}
