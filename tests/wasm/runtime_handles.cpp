//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"
static unsigned gate = 0;

static int
blocked(void *)
{
  while ( !__atomic_load_n(&gate, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&gate, 0);
  return micron::wasm::cancellation_requested() ? 42 : 17;
}

static int
self_join(void *)
{
  const auto slot = micron::wasm::thread_id() - 1;
  micron::wasm::thread_handle own{ slot, __atomic_load_n(&micron::wasm::__threads[slot].generation, __ATOMIC_ACQUIRE) };
  return micron::wasm::join(own);
}

struct join_job {
  micron::wasm::thread_handle target;
  int status = 99;
};

static int
competing_join(void *p)
{
  auto &job = *static_cast<join_job *>(p);
  job.status = micron::wasm::join(job.target);
  return 1;
}

int
main()
{
#if defined(__micron_wasm_threads)
  using namespace micron::wasm;
  thread_handle self;
  if ( spawn(self_join, nullptr, self) ) return 6;
  auto stale = self;
  int result = 0;
  if ( join(self, &result) || result != -35 || join(stale) != -22 ) return 6;
  thread_handle waiting[MICRON_WASM_MAX_THREADS]{};
  unsigned count = 0;
  for ( ; count < MICRON_WASM_MAX_THREADS; ++count ) {
    const int r = spawn(blocked, nullptr, waiting[count]);
    if ( r == -11 ) break;
    if ( r ) return 6;
  }
  if ( !count || count == MICRON_WASM_MAX_THREADS ) return 6;
  if ( join(stale) != -22 || request_cancel(stale) != -22 ) return 6;
  if ( request_cancel(waiting[0]) ) return 6;
  __atomic_store_n(&gate, 1u, __ATOMIC_RELEASE);
  wake32(&gate);
  for ( unsigned i = 0; i < count; ++i ) {
    if ( join(waiting[i], &result) || result != (i ? 17 : 42) ) return 6;
  }
  if ( __atomic_load_n(&__live_threads, __ATOMIC_ACQUIRE) ) return 6;
  gate = 0;
  thread_handle target;
  if ( spawn(blocked, nullptr, target) ) return 6;
  join_job first{ target }, second{ target };
  thread_handle a, b;
  if ( spawn(competing_join, &first, a) || spawn(competing_join, &second, b) ) return 6;
  __atomic_store_n(&gate, 1u, __ATOMIC_RELEASE);
  wake32(&gate);
  if ( join(a) || join(b) ) return 6;
  if ( !((first.status == 0 && second.status == -22) || (first.status == -22 && second.status == 0)) ) return 6;
  if ( join(target) != -22 || __atomic_load_n(&__live_threads, __ATOMIC_ACQUIRE) ) return 6;
#endif
  return 1;
}
