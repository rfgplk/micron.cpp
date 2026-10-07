//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "../vector/vector.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Inclusive associative scan with setup-owned blocks; no coroutine or thread dependency

namespace micron::parallel
{

struct serial_scan_executor {
  template<auto Op, class Work>
  max_t
  run(Work *items, usize count) noexcept
  {
    for ( usize i = 0; i < count; ++i ) {
      const max_t result = Op(items[i]);
      if ( result < 0 ) return result;
    }
    return 0;
  }
};

template<class T, class Op> class scan_workspace
{
  struct __item {
    const T *input{};
    T *output{};
    usize first{}, last{};
    T *total{};
    const T *carry{};
    const Op *operation{};
  };

  const usize __capacity;
  micron::vector<T, micron::allocator_serial<>, false> __totals, __carries;
  micron::vector<__item, micron::allocator_serial<>, false> __items;

  static max_t
  __local(__item &job) noexcept
  {
    T accumulated = job.input[job.first];
    job.output[job.first] = accumulated;
    for ( usize i = job.first + 1; i < job.last; ++i ) {
      accumulated = (*job.operation)(accumulated, job.input[i]);
      job.output[i] = accumulated;
    }
    *job.total = accumulated;
    return 0;
  }

  static max_t
  __carry(__item &job) noexcept
  {
    if ( job.carry )
      for ( usize i = job.first; i < job.last; ++i ) job.output[i] = (*job.operation)(*job.carry, job.output[i]);
    return 0;
  }

public:
  explicit scan_workspace(usize blocks)
      : __capacity(blocks), __totals(blocks ? blocks : 1, T{}), __carries(blocks ? blocks : 1, T{}), __items(blocks ? blocks : 1, __item{})
  {
  }

  scan_workspace(const scan_workspace &) = delete;
  scan_workspace &operator=(const scan_workspace &) = delete;

  [[nodiscard]] usize
  capacity() const noexcept
  {
    return __capacity;
  }

  // Executor.run<Op>(items,count) joins every item before returning 0 or a negative fault.
  // Input/output may be identical or disjoint; partial overlap is rejected. Op must be associative.
  template<class Executor>
  [[nodiscard]] max_t
  scan(const T *input, T *output, usize count, const Op &operation, Executor &executor) noexcept
  {
    static_assert(noexcept(operation(*input, *input)));
    if ( !__capacity || count > usize(-1) / sizeof(T) ) return -22;
    if ( !count ) return 0;
    if ( !input || !output ) return -22;
    const uintptr_t a = reinterpret_cast<uintptr_t>(input), b = reinterpret_cast<uintptr_t>(output);
    const usize bytes = count * sizeof(T);
    if ( a != b && (a < b ? b - a < bytes : a - b < bytes) ) return -22;
    const usize blocks = count < __capacity ? count : __capacity;
    const usize width = count / blocks, extra = count % blocks;
    usize first{};
    for ( usize i = 0; i < blocks; ++i ) {
      const usize last = first + width + usize(i < extra);
      __items[i] = { input, output, first, last, __totals.data() + i, nullptr, &operation };
      first = last;
    }
    max_t result = executor.template run<__local>(__items.data(), blocks);
    if ( result < 0 || blocks == 1 ) return result;
    T accumulated = __totals[0];
    for ( usize i = 1; i < blocks; ++i ) {
      __carries[i] = accumulated;
      __items[i].carry = __carries.data() + i;
      accumulated = operation(accumulated, __totals[i]);
    }
    return executor.template run<__carry>(__items.data(), blocks);
  }
};

template<class T, class Op, class Executor>
[[nodiscard]] max_t
scan_into(const T *input, T *output, usize count, const Op &operation, scan_workspace<T, Op> &workspace, Executor &executor) noexcept
{
  return workspace.scan(input, output, count, operation, executor);
}

};      // namespace micron::parallel
