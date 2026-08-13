//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../mutex/mutex.hpp"

#include "event_count.hpp"
#include "futex.hpp"
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
      if ( counter.compare_exchange_weak(cur, cur - step, memory_order::seq_cst, memory_order::relaxed) ) {
        // used to issue no wake
        if ( cur - step <= 0 ) micron::wake_futex(counter.ptr(), 0x7fffffff);
        return;
      }
    }
  }

  // spin briefly, then park
  void
  wait() const noexcept
  {
    default_backoff bo;
    for ( ;; ) {
      const int cur = counter.get(memory_order::acquire);
      if ( cur <= 0 ) return;
      if ( bo.next() != spin_step::park ) continue;
      auto r = micron::__futex(const_cast<u32 *>(reinterpret_cast<const u32 *>(counter.ptr())), futex_wait | futex_private_flag,
                               static_cast<u32>(cur), nullptr, nullptr, 0);
      if ( r < 0 && r != -11 && r != -4 ) return;
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
  mutable event_count gate;

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
        if ( cnt == 1 ) {
          gate.notify_all();
          return 0;
        }
        default_backoff bo;
        for ( ;; ) {
          if ( (u32)(state.get(memory_order::acquire) >> 32) != gen ) break;
          if ( bo.next() != spin_step::park ) continue;
          const u32 key = gate.prepare_wait();
          if ( (u32)(state.get(memory_order::acquire) >> 32) != gen ) {
            gate.cancel_wait();
            break;
          }
          gate.commit_wait(key);
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
      if ( state.compare_exchange_weak(cur, desired, memory_order::acq_rel, memory_order::acquire) ) {
        if ( cnt == 1 ) gate.notify_all();
        return;
      }
    }
  }

  int
  expected() const noexcept
  {
    return (int)((u32)state.get(memory_order::acquire));
  }
};

};      // namespace micron
