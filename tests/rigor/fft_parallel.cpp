//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/compute/thread_pool.hpp"
#include "../../src/math/fft/parallel.hpp"
#include "../snowball/snowball.hpp"
namespace ft = micron::math::fft;
using ft::buffer;
using micron::math::complex;
#include "../support/fft/oracle.hpp"

struct reject_executor {
  usize calls{};

  bool
  submit(void (*)(void *) noexcept, void *) noexcept
  {
    ++calls;
    return false;
  }
};

struct partial_executor {
  micron::math::compute::thread_pool_executor &pool;
  usize calls{};

  bool
  submit(void (*fn)(void *) noexcept, void *argument) noexcept
  {
    return (++calls % 2) ? pool.submit(fn, argument) : false;
  }
};

struct shared_job {
  const ft::plan<double> &plan;
  ft::workspace<double> &work;
  const complex<double> *input;
  complex<double> *output;
  u32 *remaining;

  static void
  run(void *context) noexcept
  {
    auto &j = *static_cast<shared_job *>(context);
    auto *remaining = j.remaining;
    j.plan.execute(j.input, j.output, j.work.view());
    micron::atom::fetch_sub(remaining, u32(1), micron::atomic_release);
  }
};

int
main()
{
  micron::concurrent_arena arena;
  (void)arena.create();
  (void)arena.create();
  micron::math::compute::thread_pool_executor pool(arena);
  partial_executor partial{ pool };
  reject_executor reject;
  sb::test_case("Parallel axes, batches and large 1D stages; rejection falls back and drains accepted work");
  for ( auto shape : { buffer<usize>{ 8, 11, 16 }, buffer<usize>{ 4096 }, buffer<usize>{ 2053 } } ) {
    ft::descriptor<double> d(shape);
    d.batches = shape.size() == 1 ? 1 : 3;
    ft::plan<double> p(d);
    ft::workspace<double> w(p);
    ft::parallel_workspace<double> parallel(p, 4);
    buffer<complex<double>> in(p.input_size()), reference(p.output_size()), out(p.output_size()), other(p.output_size());
    for ( auto &z : in ) z = { double(sample()), double(sample()) };
    p.execute(in.data(), reference.data(), w.view());
    for ( unsigned i = 0; i < 5; ++i ) {
      sb::require_true(parallel.execute(p, in.data(), out.data(), pool) > 0);
      sb::require_true(parallel.execute(p, in.data(), other.data(), partial) > 0);
      for ( usize k = 0; k < out.size(); ++k ) {
        check(out[k].real, reference[k].real, 1e-10L);
        check(out[k].imag, reference[k].imag, 1e-10L);
        sb::require_true(out[k] == other[k]);
      }
    }
    sb::require_true(parallel.execute(p, in.data(), out.data(), reject) > 0);
    for ( usize k = 0; k < out.size(); ++k ) sb::require_true(out[k] == reference[k]);
    ft::workspace<double> second(p);
    alignas(4) u32 remaining = 2;
    shared_job a{ p, w, in.data(), out.data(), &remaining }, b{ p, second, in.data(), other.data(), &remaining };
    sb::require_true(pool.submit(&shared_job::run, &a));
    sb::require_true(pool.submit(&shared_job::run, &b));
    while ( micron::atom::load(&remaining, micron::atomic_acquire) ) micron::atom::signal_fence(micron::atomic_seq_cst);
    for ( usize k = 0; k < out.size(); ++k ) {
      sb::require_true(out[k] == reference[k]);
      sb::require_true(other[k] == reference[k]);
    }
  }
  sb::require_true(reject.calls > 0 && partial.calls > 0);
  sb::end_test_case();
  sb::test_case("Parallel packed real and separable DCT/DST");
  ft::descriptor<double> d({ 9, 16 });
  d.batches = 2;
  d.kind = ft::domain::real_forward;
  ft::plan<double> p(d);
  ft::parallel_workspace<double> pw(p, 3);
  buffer<double> in(p.input_size());
  for ( auto &x : in ) x = double(sample());
  buffer<complex<double>> out(p.output_size());
  sb::require_true(pw.execute(p, in.data(), out.data(), partial) > 0);
  d.kind = ft::domain::real_backward;
  ft::plan<double> b(d);
  ft::parallel_workspace<double> bw(b, 3);
  buffer<double> back(in.size());
  sb::require_true(bw.execute(b, out.data(), back.data(), partial) > 0);
  for ( usize i = 0; i < in.size(); ++i ) check(back[i], in[i], 1e-12L);
  d.kind = ft::domain::complex;
  ft::r2r_plan<double> r(d, { ft::r2r_kind::dct_ii, ft::r2r_kind::dst_iv });
  ft::parallel_workspace<double> rw(r, 3);
  buffer<double> freq(in.size());
  sb::require_true(rw.execute(r, in.data(), freq.data(), partial) > 0);
  sb::require_true(rw.execute(r, freq.data(), back.data(), partial, ft::direction::backward) > 0);
  for ( usize i = 0; i < in.size(); ++i ) check(back[i], in[i], 1e-12L);
  sb::end_test_case();

  sb::test_case("Parallel convolution, long DCT, split and in-place adapters");
  ft::convolution_descriptor cd(in.size(), 7);
  double kernel[7] = { 1, 2, 3, 4, 3, 2, 1 };
  ft::convolution_plan<double> cp(cd, kernel);
  ft::parallel_workspace<double> cw(cp, 3);
  ft::workspace<double> serial(cp);
  buffer<double> conv(cp.output_size()), conv_ref(cp.output_size());
  sb::require_true(cw.execute(cp, in.data(), conv.data(), partial) > 0);
  cp.execute(in.data(), conv_ref.data(), serial.view());
  for ( usize k = 0; k < conv.size(); ++k ) check(conv[k], conv_ref[k], 1e-11L);
  ft::r2r_plan<double> large(1024, ft::r2r_kind::dct_iv);
  ft::parallel_workspace<double> lw(large, 4);
  buffer<double> lx(1024), ly(1024), lz(1024);
  for ( auto &x : lx ) x = double(sample());
  sb::require_true(lw.execute(large, lx.data(), ly.data(), partial) > 0);
  sb::require_true(lw.execute(large, ly.data(), lz.data(), partial, ft::direction::backward) > 0);
  for ( usize k = 0; k < 1024; ++k ) check(lz[k], lx[k], 1e-12L);
  ft::plan<double> sp(32);
  ft::parallel_workspace<double> sw(sp, 3);
  double re[32]{}, im[32]{}, ore[32]{}, oim[32]{};
  re[0] = 1;
  sb::require_true(sw.execute(sp, ft::split_input<double>{ re, im }, ft::split_output<double>{ ore, oim }, partial) > 0);
  for ( usize k = 0; k < 32; ++k ) sb::require_true(ore[k] == 1 && oim[k] == 0);
  ft::descriptor<double> ip(16);
  ip.kind = ft::domain::real_forward;
  ft::plan<double> ipf(ip);
  ft::parallel_workspace<double> ipw(ipf, 3);
  double data[18]{};
  data[2] = 1;
  sb::require_true(ipw.execute_inplace(ipf, data, partial) > 0);
  ip.kind = ft::domain::real_backward;
  ft::plan<double> ipb(ip);
  ft::parallel_workspace<double> ibw(ipb, 3);
  sb::require_true(ibw.execute_inplace(ipb, data, partial) > 0);
  for ( usize k = 0; k < 16; ++k ) check(data[k], k == 2 ? 1 : 0, 1e-12L);
  sb::end_test_case();
  return 1;
}
