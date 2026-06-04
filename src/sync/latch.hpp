//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../mutex/mutex.hpp"

#include "yield.hpp"

namespace micron
{

class latch
{
  atomic_token<int> counter;

public:
  explicit latch(int expected) : counter(expected) { }

  void
  count_down(int n = 1) noexcept
  {
    if ( n <= 0 ) return;
    int cur = counter.get(memory_order::relaxed);
    for ( ;; ) {
      if ( cur <= 0 ) return;
      int step = (cur < n) ? cur : n;
      if ( counter.compare_exchange_weak(cur, cur - step, memory_order::seq_cst, memory_order::relaxed) ) return;
    }
  }

  void
  wait() const noexcept
  {
    while ( counter.get(memory_order::seq_cst) > 0 ) {
      yield();
    }
  }

  bool
  try_wait() const noexcept
  {
    return counter.get(memory_order::seq_cst) <= 0;
  }

  int
  expected() const noexcept
  {
    return counter.get(memory_order::seq_cst);
  }
};

class barrier
{
  atomic_token<u64> state;
  const u32 threshold;

  static constexpr u64
  pack(u32 gen, u32 cnt) noexcept
  {
    return ((u64)gen << 32) | (u64)cnt;
  }

public:
  explicit barrier(int num_threads) : state(pack(0, (u32)num_threads)), threshold((u32)num_threads) { }

  int
  arrive_and_wait() noexcept
  {
    u64 cur = state.get(memory_order::acquire);
    for ( ;; ) {
      u32 gen = (u32)(cur >> 32);
      u32 cnt = (u32)cur;
      u64 desired;
      if ( cnt == 1 ) {
        desired = pack(gen + 1, threshold);
      } else {
        desired = pack(gen, cnt - 1);
      }
      if ( state.compare_exchange_weak(cur, desired, memory_order::acq_rel, memory_order::acquire) ) {
        if ( cnt == 1 ) return 0;
        while ( (u32)(state.get(memory_order::acquire) >> 32) == gen ) {
          yield();
        }
        return (int)(cnt - 1);
      }
    }
  }

  void
  arrive_and_drop() noexcept
  {
    u64 cur = state.get(memory_order::acquire);
    for ( ;; ) {
      u32 gen = (u32)(cur >> 32);
      u32 cnt = (u32)cur;
      u64 desired = (cnt == 1) ? pack(gen + 1, threshold) : pack(gen, cnt - 1);
      if ( state.compare_exchange_weak(cur, desired, memory_order::acq_rel, memory_order::acquire) ) return;
    }
  }

  int
  expected() const noexcept
  {
    return (int)((u32)state.get(memory_order::acquire));
  }
};

};      // namespace micron
