//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__arch.hpp"

#if defined(__micron_eh)

// unwinder backend select
//   DWARF  (.eh_frame / .gcc_except_table / __gxx_personality_v0) -> amd64, i386, arm64
//   EHABI  (.ARM.exidx / .ARM.extab / __aeabi_unwind_cpp_pr*)     -> arm32
#if defined(__micron_arch_arm32)
#define __micron_eh_ehabi 1
#else
#define __micron_eh_dwarf 1
#endif

// freestanding currently runs single threaded only at startup, remove this once we do TLS proper
#ifndef __micron_eh_single_threaded
#define __micron_eh_single_threaded 1
#endif

#endif      // __micron_eh
