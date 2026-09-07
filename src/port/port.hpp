//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port -- the entire OS/environment surface
//
// Everything micron needs from the thing it is running inside, and nothing else. One namespace,
// one backend selected at compile time by __backend.hpp: userspace Linux (the reference), a Linux
// kernel module, or bare metal.
//
// This umbrella pulls all EIGHT facets. Depend on the single facet you need instead --
// port/yield.hpp costs 17 headers, this costs every backend's dependencies at once.
//
//   panic.hpp    write_diag  halt  halt_local
//   yield.hpp    yield  cpu_relax
//   pages.hpp    page_size  page_alloc/free  page_reserve/commit/decommit  page_protect
//                page_discard  heap_extent  addr_readable
//   clock.hpp    mono_ticks  real_ticks  ticks_per_sec  sleep_ns  + the posix surface chrono/ uses
//   ident.hpp    exec_id  process_id  thread_alive  cpu_id
//   wait.hpp     wait  wake            (+ __futex_linux, linux only)
//   rawmap.hpp   raw_map  raw_unmap
//   irq.hpp      irq_state  irq_save  irq_restore
//
// irq/ arrived at Phase 6, for the reason its own banner gives: the barebones allocator declares
// interrupt-safety a non-goal, so the masking primitive has to live where every caller reaches it.
// It is a no-op on linux and that is correct, not a stub -- a process has no interrupts to mask.
//
// wait/ and rawmap/ arrived at Phase 4 and were missing from this list; rawmap was missing from the
// includes too. It is deliberately the last one added and the easiest to leave out: nothing in the
// keep-set reaches it except bits/eh/cxa_throw.hpp, which includes the facet directly and must --
// see rawmap.hpp for why it is not part of pages.

#include "clock.hpp"
#include "ident.hpp"
#include "irq.hpp"
#include "pages.hpp"
#include "panic.hpp"
#include "rawmap.hpp"
#include "wait.hpp"
#include "yield.hpp"
