//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Shared multithreading helper for rigor tests.
//
// Replaces the `std::vector<std::thread>` + join-loop idiom with micron's own
// threading (micron::auto_thread, via micron::solo::spawn/join). This keeps tests off
// libstdc++ <thread>/<mutex>, which (a) is the project rule (the pthread shim collides
// with libstdc++), and (b) does not cross-compile on the Linaro arm toolchain
// (bits/std_mutex.h references an undeclared __PTHREAD_MUTEX_INITIALIZER), so converted
// tests now build and run under qemu-arm/qemu-aarch64 too.
//
// auto_thread is non-copyable/non-movable, so it cannot live in a std::vector; spawn
// returns a move-only __thread_pointer handle that can.
//
// spawn heap-allocates the non-copyable auto_thread and returns a movable handle. auto_thread keeps
// only a separately mapped stack pointer in the object, so parallel() does not scale the caller's
// frame with the worker count on any backend.

#include "../../src/thread/thread.hpp"

namespace mtest
{

// Run fn(i) on `n` worker threads for i in [0, n), then join them all. `fn` must be
// copyable (it is copied into each worker). n must be <= 64.
template<typename Fn>
inline void
parallel(int n, Fn fn)
{
  micron::__thread_pointer<micron::auto_thread<>> ts[64];
  for ( int i = 0; i < n; ++i ) ts[i] = micron::solo::spawn([fn, i]() { fn(i); });
  for ( int i = 0; i < n; ++i ) micron::solo::join(ts[i]);
}

}      // namespace mtest
