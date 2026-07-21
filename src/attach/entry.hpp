//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// _attach / _detach

#if defined(MICRON_ATTACH_MODULE)

#include "../defs.hpp"

#include "../bits/__attach_hook.hpp"
#include "../bits/__thread_exit_hook.hpp"
#include "../exit.hpp"

#include "info.hpp"

#include "../memory/cmemory.hpp"

#include "../thread/pool.hpp"

#include "../io/__std.hpp"

#include "../math/__gcc_int_syms.hpp"
#include "../math/__gcc_math_syms.hpp"

#include "aeabi_shims.hpp"
#include "cxa_module.hpp"

extern "C" {
__attribute__((weak)) void __micron_mem_init(void) noexcept;
}

namespace micron
{
inline bool __micron_module_detached = false;
};      // namespace micron

// primary _attach() method
// NOTE: for external readers this may not make a lot of sense nor be technically useable, it's currently used mainly for internal .bmg
// modules; to make use of this you need a custom linking procedure or have your custom ELF jump into this without _start while running
// within the address space of a live _start()ed image
extern "C" __attribute__((used, retain, visibility("default"))) int
_attach(const micron_attach_info *info) noexcept
{
  // NOTE: the bmg linker currently doesn't provide for frame unwinding, so we can't use exceptions at all; this is here for that reason
  // only once we extend the linker remove this
  static_assert(!micron::except::__use_exceptions, "modules must build freestanding without __micron_eh");

  if ( micron::__micron_attach_info != nullptr ) return attach_ealready;
  if ( micron::__micron_module_detached ) return attach_ealready;      // spent: statics are destroyed, guards latched
  if ( !micron::__attach_info_valid(info) ) return attach_ebadinfo;

  micron::__micron_attach_fatal = info->fatal;
  micron::__micron_attach_thread_atexit = info->thread_atexit;
  micron::__micron_attach_run_thread_dtors = info->run_thread_dtors;
  micron::__micron_attach_info = info;

  if ( __micron_mem_init ) __micron_mem_init();
  micron::__boot_threadpool();
  if ( info->flags & attach_f_io_buffers ) micron::io::__boot_io_buffers();

  return attach_ok;
}

extern "C" __attribute__((used, retain, visibility("default"))) void
_detach(void) noexcept
{
  if ( micron::__micron_attach_info == nullptr || micron::__micron_module_detached ) return;
  micron::__micron_module_detached = true;

  // WARNING: order matters
  if ( micron::__global_parallelpool != nullptr ) {
    micron::__global_parallelpool->stop_all();
    micron::__global_parallelpool->join_all();
  }
  if ( micron::__global_threadpool != nullptr ) micron::__global_threadpool->join_all(500);

  micron::__run_thread_dtors();             // the attaching thread's thread_local dtors (LIFO)
  micron::__drain_atexit_table();           // module __cxa_atexit registrations + fini array
  micron::io::__shutdown_io_buffers();      // flush module stdio (no-op if never booted); never closes fd 0/1/2

  // WARNING: the hooks stay installed on purpose
}

#endif
