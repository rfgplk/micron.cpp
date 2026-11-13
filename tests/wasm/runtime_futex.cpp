//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"
#include "../../src/sync/futex.hpp"
static unsigned wait_word = 0;

static int
wake(void *)
{
  __atomic_store_n(&wait_word, 1u, __ATOMIC_RELEASE);
  micron::wake_futex(&wait_word);
  return 1;
}

int
main()
{
  micron::timespec_t zero{};
  if ( micron::__futex(&wait_word, micron::futex_wait | micron::futex_private_flag, 1, &zero, nullptr, 0) != -11 ) return 6;
  if ( micron::__futex(&wait_word, micron::futex_lock_pi, 0, nullptr, nullptr, 0) != -95 ) return 6;
  if ( micron::__futex(&wait_word, micron::futex_wait_bitset, 0, &zero, nullptr, 1) != -95 ) return 6;
#if defined(__micron_wasm_threads)
  if ( micron::__futex(&wait_word, micron::futex_wait, 0, &zero, nullptr, 0) != -110 ) return 6;
  micron::wasm::thread_handle h;
  if ( micron::wasm::spawn(wake, nullptr, h) ) return 6;
  micron::wait_futex(&wait_word, 0u);
  int result = 0;
  if ( micron::wasm::join(h, &result) || result != 1 ) return 6;
#else
  if ( micron::__futex(&wait_word, micron::futex_wait, 0, &zero, nullptr, 0) != -95 ) return 6;
#endif
  return 1;
}
