//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%
// _continue

#if defined(MICRON_ATTACH_MODULE) && defined(MICRON_MX_CONTINUATION)

#include "../defs.hpp"
#include "../types.hpp"

#include "cont_args.hpp"
#include "info.hpp"

#if !defined(MICRON_CONT_ENTRY)
#define MICRON_CONT_ENTRY "mx_continue_main"
#endif
extern "C" i64 __micron_cont_user_entry(const micron_cont_args *args) noexcept __asm__(MICRON_CONT_ENTRY);

extern "C" __attribute__((used, retain, visibility("default"))) i64
__micron_continuec(void *args) noexcept
{
  static_assert(!micron::except::__use_exceptions, "continuations must build freestanding without __micron_eh");

  if ( args == nullptr ) return cont_efault;

  const micron_cont_args *a = static_cast<const micron_cont_args *>(args);
  if ( !micron::__cont_args_valid(a) ) return cont_ebadargs;

  if ( (a->flags & micron_cont_args_flag_replace) == 0 && micron::__micron_attach_info == nullptr ) return cont_enotattached;

  micron::__micron_cont_args = a;

  return __micron_cont_user_entry(a);
}

#endif
