//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0xc82b947du
#endif
#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 16
#endif

#include "../../src/bits/eh/cxa_eh_globals.hpp"
#include "../../src/control.hpp"
#include "../../src/thread/thread.hpp"
#include "fuzz_support.hpp"

#if !defined(__micron_wasm_threads) || !defined(__micron_eh)
#error "runtime_cancel_eh requires shared WebAssembly memory and native exceptions"
#endif

extern "C" void *__cxa_allocate_exception(usize) noexcept;

#define CHECK(expression)                                                                                                                  \
  do {                                                                                                                                     \
    if ( !WASM_FUZZ_CHECK(expression) ) return 6;                                                                                          \
  } while ( false )

namespace
{
struct job {
  unsigned mode, ready = 0, failures = 0;
  unsigned started[2]{}, destroyed[2]{}, order[2]{}, count = 0, nested = 0, tls_destroyed = 0;
  void *allocation[2]{};
};

thread_local job *current = nullptr;

[[noreturn]] __attribute__((noinline)) void
checkpoint_loop() noexcept
{
  __atomic_store_n(&current->ready, 1u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&current->ready);
  for ( ;; ) micron::cancellation_point();
}

struct nested_exception {
  ~nested_exception() { ++current->nested; }
};

struct owned_exception {
  unsigned index;

  explicit owned_exception(unsigned i) : index(i)
  {
    current->allocation[i] = __cxxabiv1::__get_exception_header_from_object(this);
    if ( current->mode == 4 ) checkpoint_loop();
    if ( current->mode == 5 ) throw 91u;
  }

  ~owned_exception()
  {
    ++current->started[index];
    if ( current->mode == 6 ) checkpoint_loop();
    // Cleanup must tolerate new exception allocations while releasing this one.
    try {
      throw nested_exception{};
    } catch ( const nested_exception & ) {
    }
    current->order[current->count++] = index;
    ++current->destroyed[index];
    micron::cancellation_point();
  }
};

struct tls_lifetime {
  ~tls_lifetime()
  {
    auto *g = __cxa_get_globals();
    if ( g->caughtExceptions || g->uncaughtExceptions ) ++current->failures;
    ++current->tls_destroyed;
  }
};

thread_local tls_lifetime local;

struct unwind_checkpoint {
  ~unwind_checkpoint()
  {
    auto *g = __cxa_get_globals();
    if ( g->uncaughtExceptions != 1 || g->caughtExceptions ) ++current->failures;
    checkpoint_loop();
  }
};

__attribute__((noinline)) int
run(job &j)
{
  current = &j;
  (void)&local;
  auto *g = __cxa_get_globals();
  if ( g->caughtExceptions || g->uncaughtExceptions ) ++j.failures;
  switch ( j.mode ) {
  case 0:
    try {
      throw owned_exception(0);
    } catch ( owned_exception & ) {
      if ( !g->caughtExceptions || g->uncaughtExceptions ) ++j.failures;
      checkpoint_loop();
    }
  case 1:
    try {
      throw owned_exception(0);
    } catch ( owned_exception & ) {
      void *outer = g->caughtExceptions;
      try {
        throw owned_exception(1);
      } catch ( ... ) {
        if ( !g->caughtExceptions || g->caughtExceptions == outer || g->uncaughtExceptions ) ++j.failures;
        checkpoint_loop();
      }
    }
  case 2:
    try {
      try {
        throw owned_exception(0);
      } catch ( ... ) {
        throw;
      }
    } catch ( owned_exception & ) {
      if ( !g->caughtExceptions || g->uncaughtExceptions ) ++j.failures;
      checkpoint_loop();
    }
  case 3:
    try {
      unwind_checkpoint scope;
      throw owned_exception(0);
    } catch ( ... ) {
      ++j.failures;
    }
    break;
  case 4:
    throw owned_exception(0);
  case 5:
    try {
      throw owned_exception(0);
    } catch ( unsigned value ) {
      if ( value != 91 || micron::wasmalloc::is_present(j.allocation[0]) ) ++j.failures;
      j.allocation[0] = nullptr;
      j.allocation[1] = g->caughtExceptions;
      checkpoint_loop();
    }
    break;
  case 6:
    try {
      throw owned_exception(0);
    } catch ( const owned_exception & ) {
    }
    break;
  case 7:
    j.allocation[0] = __cxxabiv1::__get_exception_header_from_object(__cxa_allocate_exception(32));
    checkpoint_loop();
  }
  return -1;
}
}      // namespace

int
main()
{
  usize baseline = 0;
  for ( unsigned step = 0; step < wasm_fuzz::steps; ++step ) {
    wasm_fuzz::step = step;
    job j{ step % 8 };
    {
      micron::thread<> thread{ [&] { return run(j); } };
      while ( !__atomic_load_n(&j.ready, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&j.ready, 0);
      CHECK(thread.cancel() == 0);
      CHECK(!thread.alive() && !thread.native_handle().slot);
    }
    const unsigned expected = j.mode <= 3 ? (j.mode == 1 ? 2 : 1) : 0;
    CHECK(j.count == expected && j.nested == expected);
    CHECK(j.failures == 0 && j.tls_destroyed == 1);
    CHECK(j.destroyed[0] == (expected > 0) && j.destroyed[1] == (expected == 2));
    CHECK(j.started[0] == (expected > 0 || j.mode == 6) && j.started[1] == (expected == 2));
    if ( expected == 2 ) CHECK(j.order[0] == 1 && j.order[1] == 0);
    for ( const void *allocation : j.allocation )
      if ( allocation ) CHECK(!micron::wasmalloc::is_present(allocation));
    if ( !step ) baseline = micron::wasmalloc::musage();
    CHECK(micron::wasmalloc::musage() == baseline);
    CHECK(!micron::wasm::__live_threads);
  }
  return wasm_fuzz::finish("cancel-eh");
}
