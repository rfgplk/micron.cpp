//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/threads.hpp"
#include "../../src/control.hpp"
#include "../../src/exit.hpp"

static unsigned gate = 0, finalized = 0, shutdown_fault = 0, checksum = 0;
static unsigned host_control[2]{};
static unsigned cancel_control[4]{}, cancel_mode = 0;
static micron::wasm::thread_handle pending;

static thread_local unsigned cancel_tls = 0;

struct cancel_lifetime {
  ~cancel_lifetime()
  {
    if ( cancel_tls != 0x7a31d95bu ) __builtin_trap();
    __atomic_fetch_add(&cancel_control[3], 1u, __ATOMIC_RELAXED);
    __atomic_store_n(&cancel_control[2], 1u, __ATOMIC_RELEASE);
    micron::wasm::wake32(&cancel_control[2]);
    if ( cancel_mode == 1 ) __builtin_trap();
    if ( cancel_mode == 2 ) micron::wasm::panic(57);
    if ( cancel_mode == 3 )
      while ( !__atomic_load_n(&cancel_control[1], __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&cancel_control[1], 0);
  }
};

static thread_local cancel_lifetime cancelled_local;

struct lifetime {
  ~lifetime()
  {
    ++finalized;
    if ( shutdown_fault == 1 ) __builtin_trap();
    if ( shutdown_fault == 2 ) micron::wasm::panic(57);
    if ( shutdown_fault == 3 ) micron::wasm::write_diag("F", 1, 1);
  }
};

static lifetime object;

static int
wait_for_release(void *)
{
  while ( !__atomic_load_n(&gate, __ATOMIC_ACQUIRE) ) (void)micron::wasm::wait32(&gate, 0);
  return 57;
}

static int
wait_for_cancel(void *)
{
  (void)&cancelled_local;
  cancel_tls = 0x7a31d95bu;
  __atomic_store_n(&cancel_control[0], 1u, __ATOMIC_RELEASE);
  micron::wasm::wake32(&cancel_control[0]);
  for ( ;; ) {
    micron::cancellation_point();
    unsigned delay = 0;
    (void)micron::wasm::wait32(&delay, 0, 1000000);
  }
}

extern "C" __attribute__((export_name("lifecycle_start"))) int
lifecycle_start()
{
  if ( pending.slot ) return -16;
  __atomic_store_n(&gate, 0u, __ATOMIC_RELEASE);
  return micron::wasm::spawn(wait_for_release, nullptr, pending);
}

extern "C" __attribute__((export_name("lifecycle_finish"))) int
lifecycle_finish()
{
  if ( !pending.slot ) return -22;
  __atomic_store_n(&gate, 1u, __ATOMIC_RELEASE);
  micron::wasm::wake32(&gate);
  int result = 0;
  return micron::wasm::join(pending, &result) == 0 && result == 57 ? 1 : 6;
}

extern "C" __attribute__((export_name("lifecycle_release"))) int
lifecycle_release()
{
  if ( !pending.slot ) return -22;
  __atomic_store_n(&gate, 1u, __ATOMIC_RELEASE);
  micron::wasm::wake32(&gate);
  auto *state = &micron::wasm::__threads[pending.slot].state;
  unsigned value;
  while ( (value = __atomic_load_n(state, __ATOMIC_ACQUIRE)) == 1 || value == 2 ) (void)micron::wasm::wait32(state, value);
  return value == 3 ? 1 : 6;
}

extern "C" __attribute__((export_name("lifecycle_finalized"))) unsigned
lifecycle_finalized()
{
  return reinterpret_cast<unsigned>(&finalized);
}

extern "C" __attribute__((export_name("lifecycle_eh"))) unsigned
lifecycle_eh()
{
#if defined(__micron_wasm_eh)
  return 1;
#else
  return 0;
#endif
}

extern "C" __attribute__((export_name("lifecycle_step"))) unsigned
lifecycle_step(unsigned value)
{
  checksum = checksum * 1664525u + value + 1013904223u;
  return checksum;
}

extern "C" __attribute__((export_name("lifecycle_shutdown_fault"))) void
lifecycle_shutdown_fault(unsigned mode)
{
  shutdown_fault = mode;
}

extern "C" __attribute__((export_name("lifecycle_trap"))) void
lifecycle_trap()
{
  __builtin_trap();
}

extern "C" __attribute__((export_name("lifecycle_control"))) unsigned
lifecycle_control()
{
  return reinterpret_cast<unsigned>(host_control);
}

extern "C" __attribute__((export_name("lifecycle_block"))) int
lifecycle_block()
{
  __atomic_store_n(&host_control[0], 1u, __ATOMIC_RELEASE);
  micron::wasm::wake32(&host_control[0]);
  while ( !__atomic_load_n(&host_control[1], __ATOMIC_ACQUIRE) )
    if ( micron::wasm::wait32(&host_control[1], 0) == -95 ) return -95;
  return 19;
}

extern "C" __attribute__((export_name("lifecycle_write_byte"))) int
lifecycle_write_byte(unsigned value, unsigned stream)
{
  const char byte = static_cast<char>(value);
  return micron::wasm::write_diag(&byte, 1, stream);
}

extern "C" __attribute__((export_name("lifecycle_cancel_control"))) unsigned
lifecycle_cancel_control()
{
  return reinterpret_cast<unsigned>(cancel_control);
}

extern "C" __attribute__((export_name("lifecycle_cancel_start"))) int
lifecycle_cancel_start(unsigned mode)
{
  if ( pending.slot ) return -16;
  for ( auto &value : cancel_control ) __atomic_store_n(&value, 0u, __ATOMIC_RELEASE);
  cancel_mode = mode;
  return micron::wasm::spawn(wait_for_cancel, nullptr, pending);
}

extern "C" __attribute__((export_name("lifecycle_cancel_request"))) int
lifecycle_cancel_request()
{
  return micron::wasm::request_cancel(pending);
}

extern "C" __attribute__((export_name("lifecycle_cancel_finish"))) int
lifecycle_cancel_finish()
{
  int result = 0;
  return micron::wasm::join(pending, &result) == 0 && result == -125 ? 1 : 6;
}

int
main()
{
  return finalized ? 6 : 1;
}
