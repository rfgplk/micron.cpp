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
  explicit latch(int expected) : counter(expected) {}

  void
  count_down(int n = 1) noexcept
  {
    for ( int i = 0; i < n; ++i ) {
      --counter;
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
    return counter.get(memory_order::seq_cst) == 0;
  }

  int
  expected() const noexcept
  {
    return counter.get(memory_order::seq_cst);
  }
};

class barrier
{
  atomic_token<int> count;
  atomic_token<int> generation;
  const int threshold;

public:
  explicit barrier(int num_threads) : count(num_threads), generation(0), threshold(num_threads) {}

  int
  arrive_and_wait() noexcept
  {
    int gen = generation.get(memory_order::seq_cst);
    int cnt = --count;

    if ( cnt == 0 ) {
      count.store(threshold, memory_order::seq_cst);
      ++generation;
      return 0;
    } else {
      while ( generation.get(memory_order::seq_cst) == gen ) {
        yield();
      }
      return cnt;
    }
  }

  void
  arrive_and_drop() noexcept
  {
    int cnt = --count;
    if ( cnt == 0 ) {
      count.store(threshold, memory_order::seq_cst);
      ++generation;
    }
  }

  int
  expected() const noexcept
  {
    return count.get(memory_order::seq_cst);
  }
};

};     // namespace micron
