//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE PORT LAYER, INSTANTIATED.
//
// Built, never run. micron::port is the entire OS/environment surface: one namespace, one backend
// chosen at compile time. This file exists so that every facet ladder resolves and every name the
// backend owes is actually called -- a backend that forgets one fails here rather than at some
// distant call site in a later phase.
//
// WARNING: INTEGER-ONLY, like barebones_core.cpp, because this is swept in the kernel and
// bare-metal cells where returning a float is a hard error before the body is considered.
//
// Each facet is also included on its own, first, to prove it is standalone: the ladders must not
// depend on port.hpp having been included, and a backend must not depend on another backend's
// includes having landed. abcmalloc/printing.hpp is the cautionary tale -- it reads names it never
// includes and only compiles because its three includers happen to pull them first.

#include "../../src/port/clock.hpp"
#include "../../src/port/ident.hpp"
#include "../../src/port/irq.hpp"
#include "../../src/port/pages.hpp"
#include "../../src/port/panic.hpp"
#include "../../src/port/wait.hpp"
#include "../../src/port/yield.hpp"

#include "../../src/port/port.hpp"

namespace
{

namespace p = micron::port;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// panic -- the diagnostic surface. halt/halt_local are [[noreturn]] and are pinned by
// diverge() below rather than called here.

[[gnu::used]] void
diagnostics()
{
  p::write_diag("port compiletest", 16);
  p::write_diag("port compiletest");      // strlen overload
  p::write_diag("", 0);                   // zero length must be legal
}

[[gnu::used, noreturn]] void
diverge_group(int code)
{
  p::halt(code);
}

[[gnu::used, noreturn]] void
diverge_local(int code)
{
  p::halt_local(code);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// yield -- must be usable from a bare spin loop with no other machinery

[[gnu::used]] u64
spin(u64 rounds)
{
  u64 acc = 0;
  for ( u64 i = 0; i < rounds; ++i ) {
    p::cpu_relax();
    if ( (i & 0xFFull) == 0xFFull ) p::yield();
    acc += i;
  }
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pages -- the full surface, including the reserve/commit/decommit triple the VA-reserving
// allocator needs and which a naive alloc/free pair cannot express

[[gnu::used]] usize
paging()
{
  usize acc = p::page_size + p::large_page_size;

  auto s = p::page_alloc(p::page_size * 2);
  if ( !s.failed() ) {
    acc += s.len;
    (void)p::page_protect(s.ptr, s.len, micron::prot_read);
    p::page_discard(s.ptr, s.len);
    p::page_free(s);
  }

  auto huge = p::page_alloc(p::large_page_size, true);
  if ( !huge.failed() ) p::page_free(huge);

  auto res = p::page_reserve(p::page_size * 8);
  if ( !res.failed() ) {
    if ( p::page_commit(res.ptr, p::page_size) ) {
      acc += p::page_size;
      p::page_decommit(res.ptr, p::page_size);
    }
    p::page_free(res);
  }

  const auto he = p::heap_extent();
  acc += he.total ^ he.free;
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// clock -- integer nanoseconds throughout; no FP anywhere on this path

[[gnu::used]] i64
timing()
{
  const i64 a = p::mono_ticks();
  const i64 b = p::real_ticks();
  p::sleep_ns(0);      // the early-out branch
  return (b - a) / p::ticks_per_sec() + (a ^ b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ident

[[gnu::used]] i64
identity()
{
  const i32 t = p::exec_id();
  const i32 pi = p::process_id();
  const i32 c = p::cpu_id();
  return static_cast<i64>(t) + pi + c + (p::thread_alive(t) ? 1 : 0);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wait -- the blocking pair. Phase 4 moved this out of sync/futex.hpp; before that every container
// reached linux/sys/time.hpp and kernel.hpp through mutex/locks.hpp -> futex_mutex.hpp.
//
// wait/wake are the PORTABLE pair a kernel or metal backend owes. __futex_linux is deliberately
// linux-only and is pinned separately, so a build that loses it fails here and not at a call site.

[[gnu::used]] i64
blocking()
{
  alignas(4) u32 word = 0;
  i64 acc = 0;
  acc += p::wait(&word, 1, -1);                 // infinite
  acc += p::wait(&word, 1, 1'000'000);          // 1 ms
  acc += p::wait(&word, 1, 0);                  // zero timeout must be legal
  acc += p::wake(&word, 1);
  acc += p::wake(&word, -1);                    // -1 == wake all
  return acc;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// irq -- the eighth facet. A no-op on linux (a process has no interrupts to mask), two
// instructions on metal, local_irq_save in a module. Pinned here because it is the facet most
// likely to be forgotten by a new backend: nothing in the keep-set calls it yet, so without this
// line a backend that omits it fails at some future call site rather than at the contract.
//
// Nesting must work, so the test nests.

[[gnu::used]] u64
masking(u64 __v)
{
  const auto __a = p::irq_save();
  __v ^= 0x9E3779B9ull;
  const auto __b = p::irq_save();
  __v += 1;
  p::irq_restore(__b);
  p::irq_restore(__a);
  return __v + static_cast<u64>(__a) + static_cast<u64>(__b);
}

#if defined(__micron_port_linux)
[[gnu::used]] i64
blocking_linux_raw()
{
  alignas(4) u32 word = 0;
  return static_cast<i64>(p::__futex_linux(&word, p::futex_wait | p::futex_private_flag, 0, nullptr, nullptr, 0));
}
#endif

};      // namespace

int
main()
{
  return 1;
}
