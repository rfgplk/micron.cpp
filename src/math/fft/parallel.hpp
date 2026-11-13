//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/intrin.hpp"
#include "../../sync/futex.hpp"
#include "convolution.hpp"
#include "real.hpp"

namespace micron::math::fft
{

// matches math::compute's executor contract
// a false submission must not retain the job
template<fft_scalar F> class parallel_workspace
{
  struct __job {
    void *context{};
    void (*function)(void *, usize, complex<F> *) noexcept {};
    complex<F> *scratch{};
    usize begin{}, end{};
    u32 *remaining{};

    static void
    __entry(void *opaque) noexcept
    {
      auto &j = *static_cast<__job *>(opaque);
      for ( usize i = j.begin; i < j.end; ++i ) j.function(j.context, i, j.scratch);
      auto *remaining = j.remaining;
      if ( micron::atom::fetch_sub(remaining, u32(1), micron::atomic_acq_rel) == 1 ) micron::wake_futex(remaining, 0x7fffffff);
    }
  };

  static usize
  __checked_workers(usize n)
  {
    if ( !n || n > 0x7fffffff ) exc<except::invalid_argument>("fft: invalid worker count");
    return n;
  }

  workspace<F> __work;
  buffer<__job> __jobs;
  alignas(4) u32 __remaining{};

  template<typename Executor> struct __runner {
    parallel_workspace &owner;
    Executor &executor;
    usize workers;

    template<typename Function>
    void
    operator()(usize count, complex<F> *scratch, usize scratch_size, Function &&fn) noexcept
    {
      usize jobs = count < workers ? count : workers;
      if ( !jobs ) return;
      if ( jobs == 1 ) {
        for ( usize i = 0; i < count; ++i ) fn(i, scratch);
        return;
      }
      micron::atom::store(&owner.__remaining, u32(jobs), micron::atomic_release);
      for ( usize w = 0; w < jobs; ++w ) {
        auto &job = owner.__jobs[w];
        job.context = &fn;
        job.function
            = [](void *context, usize i, complex<F> *s) noexcept { (*static_cast<micron::remove_reference_t<Function> *>(context))(i, s); };
        job.scratch = scratch + w * scratch_size;
        usize portion = count / jobs, remainder = count % jobs;
        job.begin = w * portion + (w < remainder ? w : remainder);
        job.end = job.begin + portion + (w < remainder ? 1 : 0);
        job.remaining = &owner.__remaining;
        if ( w + 1 == jobs || !executor.submit(&__job::__entry, &job) ) __job::__entry(&job);
      }
      while ( u32 n = micron::atom::load(&owner.__remaining, micron::atomic_acquire) ) (void)micron::try_wait_futex(&owner.__remaining, n);
    }
  };

public:
  template<typename Plan>
  parallel_workspace(const Plan &p, usize workers) : __work(p.workspace_size(__checked_workers(workers))), __jobs(workers)
  {
    if ( !workers || workers > 0x7fffffff ) exc<except::invalid_argument>("fft: invalid worker count");
  }

  parallel_workspace(const parallel_workspace &) = delete;
  parallel_workspace &operator=(const parallel_workspace &) = delete;

  usize
  workers() const noexcept
  {
    return __jobs.size();
  }

  template<typename Executor, typename Input, typename Output>
  max_t
  execute(const plan<F> &p, const Input *in, Output *out, Executor &executor, direction dir = direction::forward) noexcept
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "FFT executor submit must be noexcept");
    static_assert((micron::is_same_v<Input, F> && micron::is_same_v<Output, complex<F>>)
                      || (micron::is_same_v<Input, complex<F>> && (micron::is_same_v<Output, F> || micron::is_same_v<Output, complex<F>>)),
                  "FFT input/output types must match the transform domains");
    const auto &d = p.configuration();
    if ( d.batches && (!in || !out) ) return -22;
    if constexpr ( micron::is_same_v<Input, F> ) {
      if ( d.kind != domain::real_forward ) return -22;
      dir = direction::forward;
    } else if constexpr ( micron::is_same_v<Output, F> ) {
      if ( d.kind != domain::real_backward ) return -22;
      dir = direction::backward;
    } else if ( d.kind != domain::complex )
      return -22;
    __runner<Executor> runner{ *this, executor, workers() };
    return p.__execute([&](ssize_t i) { return complex<F>{ in[i] }; },
                       [&](ssize_t i, complex<F> z) {
                         if constexpr ( micron::is_same_v<Output, F> )
                           out[i] = z.real;
                         else
                           out[i] = z;
                       },
                       __work.view(), dir, runner);
  }

  template<typename Executor, typename Input, typename Output>
  max_t
  execute(const convolution_plan<F> &p, const Input *input, Output *output, Executor &executor) noexcept
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "FFT executor submit must be noexcept");
    __runner<Executor> runner{ *this, executor, workers() };
    return p.__execute(input, output, __work.view(), runner);
  }

  template<typename Executor>
  max_t
  execute(const plan<F> &p, split_input<F> input, split_output<F> output, Executor &executor, direction dir = direction::forward) noexcept
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "FFT executor submit must be noexcept");
    if ( p.configuration().kind != domain::complex
         || (p.configuration().batches && (!input.real || !input.imag || !output.real || !output.imag)) )
      return -22;
    __runner<Executor> runner{ *this, executor, workers() };
    return p.__execute([&](ssize_t i) { return complex<F>{ input.real[i], input.imag[i] }; },
                       [&](ssize_t i, complex<F> z) {
                         output.real[i] = z.real;
                         output.imag[i] = z.imag;
                       },
                       __work.view(), dir, runner);
  }

  template<typename Executor>
  max_t
  execute_inplace(const plan<F> &p, F *storage, Executor &executor) noexcept
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "FFT executor submit must be noexcept");
    auto kind = p.configuration().kind;
    if ( kind == domain::complex || (p.configuration().batches && !storage) ) return -22;
    __runner<Executor> runner{ *this, executor, workers() };
    if ( kind == domain::real_forward )
      return p.__execute([&](ssize_t i) { return complex<F>{ storage[i] }; },
                         [&](ssize_t i, complex<F> z) {
                           storage[2 * i] = z.real;
                           storage[2 * i + 1] = z.imag;
                         },
                         __work.view(), direction::forward, runner);
    return p.__execute([&](ssize_t i) { return complex<F>{ storage[2 * i], storage[2 * i + 1] }; },
                       [&](ssize_t i, complex<F> z) { storage[i] = z.real; }, __work.view(), direction::backward, runner);
  }

  template<typename Executor>
  max_t
  execute(const r2r_plan<F> &p, const F *in, F *out, Executor &executor, direction dir = direction::forward) noexcept
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "FFT executor submit must be noexcept");
    __runner<Executor> runner{ *this, executor, workers() };
    return p.__execute(in, out, __work.view(), dir, runner);
  }
};

}      // namespace micron::math::fft
