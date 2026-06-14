//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// __cxa_get_globals / __cxa_get_globals_fast
// Itanium ABI 3.3.2

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "cxa_exception.hpp"

namespace __cxxabiv1
{
// undef once we do TLS
#if defined(__micron_eh_single_threaded)
inline __cxa_eh_globals __micron_eh_globals_storage{};
#else
inline thread_local __cxa_eh_globals __micron_eh_globals_storage{};
#endif
}      // namespace __cxxabiv1

// must be defs in global scope
extern "C" inline __cxxabiv1::__cxa_eh_globals *
__cxa_get_globals() noexcept
{
  return &__cxxabiv1::__micron_eh_globals_storage;
}

extern "C" inline __cxxabiv1::__cxa_eh_globals *
__cxa_get_globals_fast() noexcept
{
  return &__cxxabiv1::__micron_eh_globals_storage;
}

#endif      // __micron_eh
