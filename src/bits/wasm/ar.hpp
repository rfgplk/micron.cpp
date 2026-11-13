//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "runtime.hpp"

namespace micron::ar
{
inline constexpr bool has_native_frames = false;

[[gnu::always_inline]] inline void *
stack_pointer() noexcept
{
  return __builtin_frame_address(0);
}

[[gnu::always_inline]] inline void *
frame_pointer() noexcept
{
  return __builtin_frame_address(0);
}

template<unsigned Level = 0>
inline void *
frame_address() noexcept
{
  static_assert(Level == 0, "wasm has no inspectable native frame chain");
  return __builtin_frame_address(0);
}

inline void *
thread_pointer() noexcept
{
#if defined(__micron_wasm_threads)
  return __builtin_wasm_tls_base();
#else
  return nullptr;
#endif
}

inline bool set_thread_pointer(void *) = delete;
inline void *program_counter() = delete;
template<unsigned Level = 0> void *return_address() = delete;
inline void set_stack_pointer(void *) = delete;
inline void call_on_stack(void *, void (*)(void *), void *) = delete;

struct record {
  void *sp, *fp, *pc;
};

inline record
here() noexcept
{
  return { stack_pointer(), frame_pointer(), nullptr };
}

inline bool
caller(record &) noexcept
{
  return false;
}
}      // namespace micron::ar
