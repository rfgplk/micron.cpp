//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#if defined(__unix__) || defined(__APPLE__)
#else
#error "Unsupported platform."
#endif

#ifndef MICRON_ABCMALLOC_STD
#define MICRON_ABCMALLOC_STD 1
#endif

#define TWORKERS 1

#ifdef TWORKERS
#define __micron_enable_concurrency_at_startup_var
#endif

namespace micron::except
{
// NOTE: this must be enabled for tests
constexpr static const bool __use_exceptions = false;
};     // namespace micron::except
