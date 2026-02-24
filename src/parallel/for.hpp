//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "../except.hpp"
#include "../thread/cpu.hpp"
#include "../thread/thread.hpp"

#include "../thread/spawn.hpp"

#include "../sync/async.hpp"
#include "../sync/expect.hpp"

#include "../concepts.hpp"
#include "../stacks/sstack.hpp"

namespace micron
{

// heavy parallel functions - intended this way

template <auto Fn, typename Tr, is_iterable_container T>
inline __attribute__((always_inline)) auto
__parallel_for(typename T::iterator start, typename T::iterator end)
{
  return solo::spawn<Tr>([start, end]() mutable {
    for ( ; start != end; ++start )
      Fn(start);
  });
}

template <auto Fn, is_iterable_container T>
  requires(micron::is_invocable_v<decltype(Fn), typename T::iterator> or micron::is_invocable_v<decltype(Fn), typename T::const_iterator>)
void
parallel_for(T &data)
{
  if ( data.empty() )
    return;
  u64 cn = cpu_count();

  fsstack<uptr<thread<>>, 1024> threads;
  u64 segment = (data.size() + cn - 1) / cn;
  u64 start = 0;

  for ( u64 i = 0; i < cn && start < data.size(); ++i ) {
    u64 end = start + segment;
    if ( end > data.size() )
      end = data.size();
    threads.move(__parallel_for<Fn, thread<>, T>(data.begin() + start, data.begin() + end));
    start = end;
  }
  while ( !threads.empty() ) {
    solo::join(threads.top());
    threads.pop();
  }
}

// fork_for - lightweight parallel functions, uses concurrent arena

template <auto Fn, typename Tr, is_iterable_container T>
inline __attribute__((always_inline)) auto
__fork_for(typename T::iterator start, typename T::iterator end)
{
  return &async(
      [](typename T::iterator __start, typename T::iterator __end) {
        for ( ; __start != __end; ++__start )
          Fn(__start);
      },
      start, end);
}

template <auto Fn, is_iterable_container T>
  requires(micron::is_invocable_v<decltype(Fn), typename T::iterator> or micron::is_invocable_v<decltype(Fn), typename T::const_iterator>)
void
fork_for(T &data)
{
  if ( data.empty() )
    return;
  u64 cn = cpu_count();
  fsstack<const concurrent_arena::type *, 1024> threads;
  u64 segment = (data.size() + cn - 1) / cn;
  u64 start = 0;

  for ( u64 i = 0; i < cn && start < data.size(); ++i ) {
    u64 end = start + segment;
    if ( end > data.size() )
      end = data.size();
    threads.move(__fork_for<Fn, concurrent_arena::thread_class, T>(data.begin() + start, data.begin() + end));
    start = end;
  }
  while ( !threads.empty() ) {
    expect<memory_order::acquire>(threads.top()->thread.get_status(), (u32)0);
    threads.pop();
  }
}
};     // namespace micron
