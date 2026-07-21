//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// attach fatal hook
//
// WARNING: when a micron image is attached into a host process, a process exit must never call SYS_exit_group; only SYS_exit
// exit.hpp can branch without pulling the full attach ABI

namespace micron
{
inline void (*__micron_attach_fatal)(int) noexcept = nullptr;

inline int (*__micron_attach_thread_atexit)(void (*)(void *), void *) noexcept = nullptr;
inline void (*__micron_attach_run_thread_dtors)() noexcept = nullptr;
};      // namespace micron
