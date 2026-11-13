//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__arch.hpp"

#if !defined(__micron_arch_wasm32)
#error "micron: the browser host ABI requires wasm32"
#endif

extern "C" {
__attribute__((import_module("micron"), import_name("random"))) int __micron_wasm_host_random(void *, unsigned) noexcept;
__attribute__((import_module("micron"), import_name("write"))) int __micron_wasm_host_write(const void *, unsigned, unsigned) noexcept;
__attribute__((import_module("micron"), import_name("clock"))) long long __micron_wasm_host_clock(unsigned) noexcept;
__attribute__((import_module("micron"), import_name("panic"), noreturn)) void __micron_wasm_host_panic(int) noexcept;
__attribute__((import_module("micron"), import_name("yield"))) void __micron_wasm_host_yield() noexcept;
__attribute__((import_module("micron"), import_name("worker_spawn"))) int __micron_wasm_host_worker_spawn(unsigned, unsigned, unsigned,
                                                                                                          unsigned) noexcept;
__attribute__((import_module("micron"), import_name("thread_exit"), noreturn)) void __micron_wasm_host_thread_exit() noexcept;
}

namespace micron::wasm
{
inline constexpr unsigned host_abi_version = 1;

inline int
write_diag(const char *p, unsigned n, unsigned stream = 2) noexcept
{
  return __micron_wasm_host_write(p, n, stream);
}

inline void
write_diag(const char *p) noexcept
{
  unsigned n = 0;
  while ( p[n] ) ++n;
  (void)write_diag(p, n);
}

[[noreturn]] inline void
panic(int code = 134) noexcept
{
  __micron_wasm_host_panic(code);
  __builtin_trap();
}

inline void
yield() noexcept
{
#if defined(__micron_wasm_threads)
  __micron_wasm_host_yield();
#endif
}
}      // namespace micron::wasm
