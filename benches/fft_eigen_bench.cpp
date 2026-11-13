// External reference executable only; Eigen/STL are never dependencies of micron.
// Same duck flags as fft_bench.cpp, plus -i /code/Git/eigen.
#include <algorithm>
#include <chrono>
#include <contrib/Eigen/FFT>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static volatile double sink;

template<typename F>
void
measure(const char *precision, std::size_t n)
{
  std::vector<std::complex<F>> x(n), y(n);
  for ( std::size_t i = 0; i < n; ++i ) x[i] = { F(int(i % 13) - 6) / F(8), F(int(i % 7) - 3) / F(16) };
  Eigen::FFT<F> fft;
  auto run = [&](std::size_t iterations) {
    auto start = std::chrono::steady_clock::now();
    for ( std::size_t i = 0; i < iterations; ++i ) {
      fft.fwd(y.data(), x.data(), int(n));
      asm volatile("" ::"g"(y.data()) : "memory");
    }
    return double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count());
  };
  run(8);
  std::size_t iterations = 1;
  while ( run(iterations) < 3000000 && iterations < (1u << 24) ) iterations *= 2;
  double samples[7];
  for ( auto &sample : samples ) sample = run(iterations) / iterations;
  std::sort(samples, samples + 7);
  sink = y[n / 3].real();
  std::printf("eigen_kiss_c2c,%s,%zu,%zu,%.3f,%.3f,%.3f,%.3f,%.3f\n", precision, n, iterations, samples[3], samples[0], samples[1],
              samples[5], samples[6]);
}

int
main(int argc, char **argv)
{
  if ( argc > 3 || (argc > 1 && std::strcmp(argv[1], "f32") && std::strcmp(argv[1], "f64")) ) return 2;
  std::puts("operation,precision,n,iterations,median_ns,min_ns,p25_ns,p75_ns,max_ns");
  auto run = [&](std::size_t n) {
    if ( argc == 1 || !std::strcmp(argv[1], "f32") ) measure<float>("f32", n);
    if ( argc == 1 || !std::strcmp(argv[1], "f64") ) measure<double>("f64", n);
  };
  if ( argc == 3 ) {
    char *end;
    auto n = std::strtoul(argv[2], &end, 10);
    if ( !n || *end || n > 1048576 ) return 2;
    run(n);
  } else
    for ( std::size_t n : { 16u, 64u, 256u, 1000u, 1009u, 1024u, 4096u, 16384u, 65536u, 262144u } ) run(n);
  return 0;
}
