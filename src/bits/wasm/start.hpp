//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../exit.hpp"
#include "../../math/__gcc_fp128_syms.hpp"
#include "../../math/__gcc_int_syms.hpp"
#include "../../memory/new.hpp"
#include "builtins.hpp"
#include "math/conversions.hpp"
#include "math/symbols.hpp"
#include "runtime.hpp"
#include "threads.hpp"

namespace micron::wasm
{
inline thread_local unsigned *__active_guards[64]{};
inline thread_local unsigned __guard_count = 0;
}      // namespace micron::wasm

extern "C" {
extern unsigned char __stack_low, __stack_high;
void __wasm_call_ctors();
extern int __micron_user_main() asm("__main_void") __attribute__((weak));
extern int __micron_user_main_args(int, char **) asm("__main_argc_argv") __attribute__((weak));
__attribute__((weak)) void *__dso_handle = &__dso_handle;

__attribute__((weak, used)) int
__cxa_atexit(void (*fn)(void *), void *arg, void *dso) noexcept
{
  return micron::__exit_internal::__push(fn, arg, dso);
}

__attribute__((weak, used)) void
__cxa_finalize(void *dso) noexcept
{
  micron::__exit_internal::__finalize_dso(dso);
}

__attribute__((weak, used)) int
__cxa_thread_atexit(void (*fn)(void *), void *arg, void *) noexcept
{
  return micron::__push_thread_dtor(fn, arg) ? 0 : -1;
}

__attribute__((weak, used)) int
__cxa_thread_atexit_impl(void (*fn)(void *), void *arg, void *dso) noexcept
{
  return __cxa_thread_atexit(fn, arg, dso);
}

__attribute__((weak, used, noreturn)) void
__cxa_pure_virtual()
{
  micron::wasm::write_diag("micron: pure virtual call\n");
  micron::wasm::panic();
}

__attribute__((weak, used, noreturn)) void
__cxa_deleted_virtual()
{
  micron::wasm::write_diag("micron: deleted virtual call\n");
  micron::wasm::panic();
}

__attribute__((weak, used)) int
__cxa_guard_acquire(__micron_guard_t *guard)
{
  auto *p = reinterpret_cast<unsigned *>(guard);
  for ( unsigned i = 0; i < micron::wasm::__guard_count; ++i )
    if ( micron::wasm::__active_guards[i] == p ) micron::wasm::panic();
  for ( ;; ) {
    unsigned v = __atomic_load_n(p, __ATOMIC_ACQUIRE);
    if ( v & 1u ) return 0;
    if ( !(v & 0x100u) && __atomic_compare_exchange_n(p, &v, 0x100u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ) {
      if ( micron::wasm::__guard_count == 64 ) micron::wasm::panic();
      micron::wasm::__active_guards[micron::wasm::__guard_count++] = p;
      return 1;
    }
    if ( micron::wasm::wait32(p, 0x100u) == -95 ) micron::wasm::panic();
  }
}

__attribute__((weak, used)) void
__cxa_guard_release(__micron_guard_t *guard)
{
  auto *p = reinterpret_cast<unsigned *>(guard);
  if ( micron::wasm::__guard_count ) --micron::wasm::__guard_count;
  __atomic_store_n(p, 1u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(p);
}

__attribute__((weak, used)) void
__cxa_guard_abort(__micron_guard_t *guard)
{
  auto *p = reinterpret_cast<unsigned *>(guard);
  if ( micron::wasm::__guard_count ) --micron::wasm::__guard_count;
  __atomic_store_n(p, 0u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(p);
}

__attribute__((export_name("__micron_wasm_prepare"))) unsigned
__micron_wasm_prepare() noexcept
{
#if defined(__micron_wasm_threads)
  return reinterpret_cast<unsigned>(micron::wasm::linear_reserve(__builtin_wasm_tls_size(), __builtin_wasm_tls_align()));
#else
  return 0;
#endif
}

__attribute__((export_name("__micron_wasm_abi_version"))) unsigned
__micron_wasm_get_abi_version() noexcept
{
  return micron::wasm::host_abi_version;
}

__attribute__((export_name("__micron_wasm_initialize"))) int
__micron_wasm_initialize(unsigned worker) noexcept
{
  using namespace micron::wasm;
#if defined(__micron_wasm_threads)
  if ( !worker ) return -95;
#endif
  __can_wait = worker != 0;
  __thread_id = 1;
  __stack_begin = reinterpret_cast<unsigned>(&__stack_low);
  __stack_bytes = reinterpret_cast<unsigned>(&__stack_high) - __stack_begin;
  unsigned expected = 0;
  if ( !__atomic_compare_exchange_n(&__runtime_state, &expected, 1u, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) )
    return expected == 2 ? 0 : -16;
  linear_memory_init();
#if defined(__micron_eh)
  try {
    __wasm_call_ctors();
  } catch ( ... ) {
    __atomic_store_n(&__runtime_state, 4u, __ATOMIC_RELEASE);
    return -5;
  }
#else
  __wasm_call_ctors();
#endif
  __atomic_store_n(&__runtime_state, 2u, __ATOMIC_RELEASE);
  return 0;
}

__attribute__((export_name("__micron_wasm_run_main"))) int
__micron_wasm_run_main(int argc, char **argv)
{
  if ( __atomic_load_n(&micron::wasm::__runtime_state, __ATOMIC_ACQUIRE) != 2 ) return -16;
  if ( !__micron_user_main_args && !__micron_user_main ) return -38;
  char *empty_arguments[] = { nullptr };
  if ( argc < 0 || (argc && !argv) ) return -22;
  if ( !argv ) argv = empty_arguments;
#if defined(__micron_eh)
  try {
    return __micron_user_main_args ? __micron_user_main_args(argc, argv) : __micron_user_main();
  } catch ( ... ) {
    micron::wasm::write_diag("micron: uncaught exception at main boundary\n");
    return -5;
  }
#else
  return __micron_user_main_args ? __micron_user_main_args(argc, argv) : __micron_user_main();
#endif
}

__attribute__((export_name("__micron_wasm_shutdown"))) int
__micron_wasm_shutdown() noexcept
{
  using namespace micron::wasm;
  __threads_lock.lock();
  if ( __atomic_load_n(&__live_threads, __ATOMIC_ACQUIRE) ) {
    __threads_lock.unlock();
    return -16;
  }
  unsigned expected = 2;
  if ( !__atomic_compare_exchange_n(&__runtime_state, &expected, 3u, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) ) {
    __threads_lock.unlock();
    return expected == 4 ? 0 : -16;
  }
  __threads_lock.unlock();
  micron::__run_exit_sequence();
  __atomic_store_n(&__runtime_state, 4u, __ATOMIC_RELEASE);
  return 0;
}

__attribute__((export_name("__micron_wasm_worker_entry"))) int
__micron_wasm_worker_entry(unsigned slot)
{
  using namespace micron::wasm;
  if ( !slot || slot >= MICRON_WASM_MAX_THREADS ) return -22;
  auto &t = __threads[slot];
  __can_wait = true;
  __thread_id = slot + 1;
  __stack_begin = t.stack;
  __stack_bytes = t.stack_bytes;
  micron::__micron_thread_park = &t.park;
  micron::__micron_thread_die = __exit_worker;
  unsigned expected = 1;
  if ( !__atomic_compare_exchange_n(&t.state, &expected, 2u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ) return -22;
  bool failed = false;
#if defined(__micron_eh)
  try {
    t.result = t.entry(t.arg);
  } catch ( ... ) {
    t.result = -5;
    failed = true;
  }
#else
  t.result = t.entry(t.arg);
#endif
  finish_thread();
  return failed ? -5 : 0;
}

__attribute__((export_name("__micron_wasm_worker_cleanup"))) int
__micron_wasm_worker_cleanup(unsigned slot) noexcept
{
  using namespace micron::wasm;
  if ( !slot || slot >= MICRON_WASM_MAX_THREADS || __thread_id != slot + 1 ) return -22;
  auto &t = __threads[slot];
  if ( __atomic_load_n(&t.state, __ATOMIC_ACQUIRE) != 2 ) return -22;
  t.result = -125;
  while ( __guard_count ) {
    auto *p = __active_guards[--__guard_count];
    __atomic_store_n(p, 0u, __ATOMIC_RELEASE);
    (void)wake32(p);
  }
  finish_thread(true);
  return 0;
}

__attribute__((export_name("__micron_wasm_worker_failed"))) void
__micron_wasm_worker_failed(unsigned slot) noexcept
{
  if ( !slot || slot >= MICRON_WASM_MAX_THREADS ) return;
  auto &t = micron::wasm::__threads[slot];
  t.result = -5;
  __atomic_store_n(&t.state, 4u, __ATOMIC_RELEASE);
  (void)micron::wasm::wake32(&t.state);
}
}
