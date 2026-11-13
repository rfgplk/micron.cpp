//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"

#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 128
#endif

struct task {
  unsigned gate = 0;
  int value = 0;
};

static unsigned observed = 0, resume = 0;
static micron::wasm::thread_handle target;

static void
publish(unsigned &word, unsigned value)
{
  __atomic_store_n(&word, value, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&word);
}

static int
blocked(void *p)
{
  auto &t = *static_cast<task *>(p);
  while ( !__atomic_load_n(&t.gate, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&t.gate, 0);
  return t.value;
}

static void
before_state(micron::wasm::thread_handle handle)
{
  if ( handle.slot != target.slot || handle.generation != target.generation ) return;
  if ( !__atomic_exchange_n(&observed, 1u, __ATOMIC_ACQ_REL) ) {
    (void)micron::wasm::wake32(&observed);
    while ( !__atomic_load_n(&resume, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&resume, 0);
  }
}

static int
stale_join(void *)
{
  auto copy = target;
  return micron::wasm::__join<before_state>(copy);
}

int
main()
{
#if defined(__micron_wasm_threads)
  using namespace micron::wasm;
  for ( unsigned round = 0; round < MICRON_WASM_FUZZ_STEPS; ++round ) {
    task old{ 0, static_cast<int>(round + 17) }, next{ 0, static_cast<int>(round + 31) };
    observed = resume = 0;
    if ( spawn(blocked, &old, target) ) return 6;
    auto stale = target;
    thread_handle observer;
    if ( spawn(stale_join, nullptr, observer) ) return 6;
    while ( !__atomic_load_n(&observed, __ATOMIC_ACQUIRE) ) (void)wait32(&observed, 0);
    publish(old.gate, 1);
    int value = 0;
    if ( join(target, &value) || value != old.value ) return 6;
    thread_handle replacement;
    if ( spawn(blocked, &next, replacement) ) return 6;
    if ( replacement.slot != stale.slot || replacement.generation == stale.generation ) return 6;
    publish(resume, 1);
    if ( join(observer, &value) || value != -22 ) return 6;
    if ( request_cancel(stale) != -22 || __set_park(stale, false) != -22 || join(stale) != -22 ) return 6;
    const auto state = __atomic_load_n(&__threads[replacement.slot].state, __ATOMIC_ACQUIRE);
    if ( state != 1 && state != 2 ) return 6;
    publish(next.gate, 1);
    if ( join(replacement, &value) || value != next.value || __atomic_load_n(&__live_threads, __ATOMIC_ACQUIRE) ) return 6;
  }
#endif
  return 1;
}
