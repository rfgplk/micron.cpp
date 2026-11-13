//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
// duck build benches/fft_microbench.cpp --std c++23 --isa v3 --perf --fp --no-ssp --no-lto -i . -i ./src -o bin/fft/tuning
#include "../src/chrono/clock.hpp"
#include "../src/io/console.hpp"
#include "../src/math/compute/thread_pool.hpp"
#include "../src/math/fft.hpp"
#include "../src/math/fft/parallel.hpp"

namespace ft = micron::math::fft;
using micron::math::complex;
static volatile double sink;

static bool
equal(const char *a, const char *b)
{
  while ( *a && *a == *b ) ++a, ++b;
  return *a == *b;
}

static usize
number(const char *s)
{
  usize n = 0;
  if ( !*s ) micron::exc<micron::except::invalid_argument>("empty benchmark argument");
  for ( ; *s; ++s ) {
    if ( *s < '0' || *s > '9' || n > (usize(1) << 30) / 10 ) micron::exc<micron::except::invalid_argument>("invalid benchmark argument");
    n = 10 * n + usize(*s - '0');
  }
  return n;
}

struct options {
  const char *mode, *precision;
  usize n, iterations, samples, offset, workers;
};

template<typename Function>
__attribute__((noinline)) void
measure(const options &o, usize items, Function &&fn)
{
  for ( usize i = 0; i < 8; ++i ) fn();
  usize iterations = o.iterations ? o.iterations : 1;
  if ( !o.iterations ) {
    for ( ;; ) {
      auto begin = micron::chrono::mono_ns();
      for ( usize i = 0; i < iterations; ++i ) fn();
      if ( micron::chrono::mono_ns() - begin >= 3000000 || iterations >= (usize(1) << 24) ) break;
      iterations *= 2;
    }
  }
  double samples[31];
  for ( usize s = 0; s < o.samples; ++s ) {
    auto begin = micron::chrono::mono_ns();
    for ( usize i = 0; i < iterations; ++i ) fn();
    samples[s] = double(micron::chrono::mono_ns() - begin) / double(iterations);
  }
  for ( usize i = 1; i < o.samples; ++i )
    for ( usize j = i; j && samples[j] < samples[j - 1]; --j ) {
      auto t = samples[j];
      samples[j] = samples[j - 1];
      samples[j - 1] = t;
    }
  micron::io::println("operation,precision,n,items,offset,workers,iterations,samples,median_ns,min_ns,p25_ns,p75_ns,max_ns");
  micron::io::println(o.mode, ",", o.precision, ",", o.n, ",", items, ",", o.offset, ",", o.workers, ",", iterations, ",", o.samples, ",",
                      samples[o.samples / 2], ",", samples[0], ",", samples[o.samples / 4], ",", samples[3 * o.samples / 4], ",",
                      samples[o.samples - 1]);
}

template<typename T>
static void
observe(T *p)
{
  asm volatile("" : : "g"(p) : "memory");
}

template<typename F>
static bool
stage_bench(const options &o)
{
  usize radix = 0;
  if ( equal(o.mode, "stage2") ) radix = 2;
  if ( equal(o.mode, "stage3") ) radix = 3;
  if ( equal(o.mode, "stage4") ) radix = 4;
  if ( equal(o.mode, "stage5") ) radix = 5;
  if ( equal(o.mode, "stage7") ) radix = 7;
  if ( !radix ) return false;
  ft::buffer<complex<F>> output(radix * o.n + o.offset), tw((radix - 1) * o.n + o.offset);
  auto *out = output.data() + o.offset, *roots = tw.data() + o.offset;
  for ( usize j = 1; j < radix; ++j )
    for ( usize k = 0; k < o.n; ++k ) roots[(j - 1) * o.n + k] = ft::__impl::__root<F>(j * k, radix * o.n);
  // Zero input keeps repeated unnormalized butterflies finite. Memory clobbers
  // prevent constant propagation; correctness is checked independently in fft_kernels.cpp.
  switch ( radix ) {
  case 2:
    measure(o, 2 * o.n, [&] {
      ft::__impl::__stage<2, false>(out, roots, o.n, 0, o.n);
      observe(out);
    });
    break;
  case 3:
    measure(o, 3 * o.n, [&] {
      ft::__impl::__stage<3, false>(out, roots, o.n, 0, o.n);
      observe(out);
    });
    break;
  case 4:
    measure(o, 4 * o.n, [&] {
      ft::__impl::__stage<4, false>(out, roots, o.n, 0, o.n);
      observe(out);
    });
    break;
  case 5:
    measure(o, 5 * o.n, [&] {
      ft::__impl::__stage<5, false>(out, roots, o.n, 0, o.n);
      observe(out);
    });
    break;
  case 7:
    measure(o, 7 * o.n, [&] {
      ft::__impl::__stage<7, false>(out, roots, o.n, 0, o.n);
      observe(out);
    });
    break;
  }
  sink = double(out[0].real);
  return true;
}

template<typename F>
static int
run(const options &o)
{
  using C = complex<F>;
  usize n = o.n;
  if ( stage_bench<F>(o) ) return 0;
  if ( equal(o.mode, "plan") ) {
    measure(o, n, [&] {
      ft::plan<F> p(n);
      sink = double(p.workspace_size());
    });
    return 0;
  }
  ft::descriptor<F> d(n);
  if ( equal(o.mode, "batch") ) d.batches = 16;
  if ( equal(o.mode, "nd") ) d = ft::descriptor<F>({ n, n });
  if ( equal(o.mode, "strided") ) d.input.strides = { 3 }, d.output.strides = { 3 };
  if ( equal(o.mode, "r2c") ) d.kind = ft::domain::real_forward;
  if ( equal(o.mode, "c2r") ) d.kind = ft::domain::real_backward;
  ft::plan<F> p(d);
  ft::workspace<F> w(p);
  usize count = n * d.batches;
  if ( equal(o.mode, "nd") ) count *= n;
  usize capacity = count * 3 + o.offset + 8;
  ft::buffer<C> input(capacity), output(capacity), factor(capacity);
  auto *x = input.data() + o.offset, *y = output.data() + o.offset, *z = factor.data() + o.offset;
  ft::buffer<F> re(capacity), im(capacity), ore(capacity), oim(capacity);
  for ( usize i = 0; i < capacity - o.offset; ++i ) {
    x[i] = { F(int(i % 13) - 6) / F(8), F(int(i % 7) - 3) / F(16) };
    re[i] = x[i].real;
    im[i] = x[i].imag;
    z[i] = { F(0), (i & 1) ? F(1) : F(-1) };
  }
  if ( equal(o.mode, "workspace") ) {
    measure(o, n, [&] {
      ft::workspace<F> fresh(p);
      observe(fresh.view().data);
    });
  } else if ( equal(o.mode, "engine") ) {
    ft::__impl::__engine<F> e(n);
    ft::buffer<C> scratch(e.scratch_size());
    measure(o, n, [&] {
      e.execute(x, y, scratch.data(), false);
      observe(y);
    });
  } else if ( equal(o.mode, "multiply") ) {
    measure(o, n, [&] {
      ft::__impl::__multiply(x, z, n);
      observe(x);
    });
  } else if ( equal(o.mode, "copy") ) {
    measure(o, n, [&] {
      for ( usize k = 0; k < n; ++k ) y[k] = x[k];
      observe(y);
    });
  } else if ( equal(o.mode, "gather") ) {
    measure(o, n, [&] {
      for ( usize k = 0; k < n; ++k ) y[k] = x[3 * k];
      observe(y);
    });
  } else if ( equal(o.mode, "parallel") ) {
    micron::concurrent_arena arena;
    for ( usize i = 1; i < o.workers; ++i ) (void)arena.create();
    micron::math::compute::thread_pool_executor executor(arena);
    ft::parallel_workspace<F> work(p, o.workers);
    measure(o, n, [&] {
      work.execute(p, x, y, executor);
      observe(y);
    });
  } else if ( equal(o.mode, "split") ) {
    measure(o, n, [&] {
      p.execute(ft::split_input<F>{ re.data(), im.data() }, ft::split_output<F>{ ore.data(), oim.data() }, w.view());
      observe(ore.data());
      observe(oim.data());
    });
  } else if ( equal(o.mode, "r2c") ) {
    measure(o, n, [&] {
      p.execute(re.data(), y, w.view());
      observe(y);
    });
  } else if ( equal(o.mode, "c2r") ) {
    x[0].imag = F(0);
    if ( n % 2 == 0 ) x[n / 2].imag = F(0);
    measure(o, n, [&] {
      p.execute(x, ore.data(), w.view());
      observe(ore.data());
    });
  } else if ( equal(o.mode, "dct") ) {
    ft::r2r_plan<F> rp(n, ft::r2r_kind::dct_ii);
    ft::workspace<F> rw(rp);
    measure(o, n, [&] {
      rp.execute(re.data(), ore.data(), rw.view());
      observe(ore.data());
    });
  } else if ( equal(o.mode, "convolve") ) {
    F kernel[31];
    for ( usize i = 0; i < 31; ++i ) kernel[i] = F(i + 1) / F(32);
    ft::convolution_plan<F> cp(ft::convolution_descriptor(n, 31), kernel);
    ft::workspace<F> cw(cp);
    ft::buffer<F> result(cp.output_size());
    measure(o, cp.output_size(), [&] {
      cp.execute(re.data(), result.data(), cw.view());
      observe(result.data());
    });
  } else if ( equal(o.mode, "inplace_pair") ) {
    measure(o, 2 * n, [&] {
      p.execute(x, x, w.view());
      observe(x);
      p.execute(x, x, w.view(), ft::direction::backward);
      observe(x);
    });
  } else if ( equal(o.mode, "c2c") || equal(o.mode, "inverse") || equal(o.mode, "batch") || equal(o.mode, "nd")
              || equal(o.mode, "strided") ) {
    auto dir = equal(o.mode, "inverse") ? ft::direction::backward : ft::direction::forward;
    measure(o, count, [&] {
      p.execute(x, y, w.view(), dir);
      observe(y);
    });
  } else {
    micron::io::println("unknown mode");
    return 2;
  }
  sink = double(y[n / 3].real) + double(x[n / 3].imag) + double(ore[n / 3]);
  return 0;
}

int
main(int argc, char **argv)
{
  if ( argc < 4 || argc > 8 ) {
    micron::io::println("fft_microbench MODE f32|f64|f80 N [ITERATIONS=0 [SAMPLES=7 [OFFSET=0 [WORKERS=4]]]]");
    micron::io::println(
        "modes: c2c inverse inplace_pair engine multiply copy gather batch strided split r2c c2r nd dct convolve parallel plan workspace");
    return 2;
  }
  options o{ argv[1],
             argv[2],
             number(argv[3]),
             argc > 4 ? number(argv[4]) : 0,
             argc > 5 ? number(argv[5]) : 7,
             argc > 6 ? number(argv[6]) : 0,
             argc > 7 ? number(argv[7]) : 4 };
  if ( argc > 8 || !o.n || o.n > (usize(1) << 24) || !o.samples || o.samples > 31 || o.offset > 15 || !o.workers || o.workers > 64
       || (equal(o.mode, "nd") && o.n > 2048) )
    return 2;
  if ( equal(o.precision, "f32") ) return run<float>(o);
  if ( equal(o.precision, "f64") ) return run<double>(o);
  if ( equal(o.precision, "f80") ) return run<long double>(o);
  return 2;
}
