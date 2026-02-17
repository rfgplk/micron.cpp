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
  u64 cn = cpu_count();
  if ( data.size() % cn != 0 )
    exc<except::thread_error>("micron parallel_for(): size of container isn't divisible by thread count");
  fsstack<uptr<thread<>>, 1024> threads;
  u64 segment = (data.size() / cn);
  u64 start = 0;
  for ( u64 i = 0; i < cn; ++i ) {
    console("start: ", i);
    threads.move(__parallel_for<Fn, thread<>, T>(data.begin() + start, data.begin() + start + segment));
    start += segment;
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
  return solo::spawn<Tr>([start, end]() mutable {
    for ( ; start != end; ++start )
      Fn(start);
  });
}

template <auto Fn, is_iterable_container T>
  requires(micron::is_invocable_v<decltype(Fn), typename T::iterator> or micron::is_invocable_v<decltype(Fn), typename T::const_iterator>)
void
fork_for(T &data)
{
  u64 cn = cpu_count();
  if ( data.size() % cn != 0 )
    exc<except::thread_error>("micron fork_for(): size of container isn't divisible by thread count");
  fsstack<uptr<thread<>>, 1024> threads;
  u64 segment = (data.size() / cn);
  u64 start = 0;
  for ( u64 i = 0; i < cn; ++i ) {
    console("start: ", i);
    threads.move(__fork_for<Fn, thread<>, T>(data.begin() + start, data.begin() + start + segment));
    start += segment;
  }
  while ( !threads.empty() ) {
    solo::join(threads.top());
    threads.pop();
  }
}
};
