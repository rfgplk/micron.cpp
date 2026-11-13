//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"
#include "../../src/errno.hpp"
#include "../../src/memory/allocation/wasmalloc/wasmalloc.hpp"
#include "../../src/thread/thread.hpp"
static unsigned counter = 0, destructors = 0;
static micron::wasm::mutex lock;

struct local {
  ~local() { __atomic_fetch_add(&destructors, 1u, __ATOMIC_RELAXED); }
};

static thread_local local tls_destructor;

static int
worker(void *arg)
{
  (void)&tls_destructor;
  const unsigned identity = static_cast<unsigned>(reinterpret_cast<usize>(arg));
  __micron_errno = identity;
  auto *p = static_cast<unsigned *>(micron::wasmalloc::try_alloc(8192));
  if ( !p ) return 6;
  p[0] = identity;
  for ( unsigned i = 0; i < 2000; ++i ) {
    lock.lock();
    ++counter;
    lock.unlock();
  }
#if defined(__micron_eh)
  try {
    throw identity;
  } catch ( unsigned v ) {
    if ( v != identity ) return 6;
  }
#endif
  if ( __micron_errno != static_cast<int>(identity) || p[0] != identity ) return 6;
  micron::wasmalloc::dealloc(p);
  return 1;
}

int
main()
{
#if defined(__micron_wasm_threads)
  micron::thread<> public_thread([](int x) { return x + 11; }, 8);
  if ( public_thread.result<int>() != 19 || public_thread.join() ) return 6;
  micron::wasm::thread_handle handles[4];
  for ( unsigned round = 0; round < 3; ++round ) {
    for ( unsigned i = 0; i < 4; ++i )
      if ( micron::wasm::spawn(worker, reinterpret_cast<void *>(i + 7), handles[i]) ) return 6;
    for ( auto &h : handles ) {
      int result;
      if ( micron::wasm::join(h, &result) || result != 1 ) return 6;
    }
  }
  if ( counter != 24000 || destructors != 12 ) return 6;
#endif
  return 1;
}
