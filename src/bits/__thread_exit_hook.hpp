//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// per-thread exit hooks
namespace micron
{
inline void (*__thread_exit_hook)() noexcept = nullptr;

// set by the regular thread kernel to point at its __thread_payload::alive atomic_token<bool>
// WARNING: this is the ONLY per-thread (thread_local) var the threading core may add
inline thread_local void *__micron_thread_alive_word = nullptr;
}      // namespace micron
