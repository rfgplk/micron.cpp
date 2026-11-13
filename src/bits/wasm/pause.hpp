//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../chrono.hpp"
#include "../__thread_exit_hook.hpp"

namespace micron
{
inline constexpr u64 __wasm_sleep_max_ns = 0x7fffffffffffffffull;

template<u64 Scale>
[[nodiscard]] inline constexpr bool
__wasm_sleep_unsigned(umax_t duration, i64 &ns) noexcept
{
  static_assert(Scale != 0);
  if ( duration > __wasm_sleep_max_ns / Scale ) return false;
  ns = static_cast<i64>(duration * Scale);
  return true;
}

[[nodiscard]] inline constexpr bool
__wasm_sleep_milliseconds(fduration_t milliseconds, i64 &ns) noexcept
{
  if ( !__builtin_isfinite(milliseconds) ) return false;
  if ( milliseconds <= 0 ) {
    ns = 0;
    return true;
  }
  if ( milliseconds > 0x1p63 / 1000000.0 ) return false;
  const fduration_t scaled = milliseconds * 1000000.0;
  if ( scaled >= 0x1p63 ) return false;
  ns = static_cast<i64>(scaled);
  return true;
}

inline void
__wasm_sleep_checked(i64 ns)
{
  __micron_park_checkpoint();
  const auto r = chrono::sleep_ns(ns);
  __micron_park_checkpoint();
  if ( r == -22 ) exc<except::thread_error>("micron: sleep duration is out of range");
  if ( r < 0 ) exc<except::thread_error>("micron: synchronous sleep requires a shared-memory worker");
}

template<u64 Scale>
inline void
__wasm_sleep_units(umax_t duration)
{
  i64 ns = 0;
  if ( !__wasm_sleep_unsigned<Scale>(duration, ns) ) exc<except::thread_error>("micron: sleep duration is out of range");
  __wasm_sleep_checked(ns);
}

template<umax_t Nanoseconds>
inline void
cpu_pause()
{
  __wasm_sleep_units<1>(Nanoseconds);
}

inline void
sleep_nano(umax_t ns)
{
  __wasm_sleep_units<1>(ns);
}

inline void
sleep_for(umax_t ms)
{
  __wasm_sleep_units<1000000>(ms);
}

inline void
sleep(ulong_t ms)
{
  sleep_for(ms);
}

inline void
ssleep(umax_t seconds)
{
  __wasm_sleep_units<1000000000>(seconds);
}

inline void
sleep_duration(fduration_t milliseconds)
{
  i64 ns = 0;
  if ( !__wasm_sleep_milliseconds(milliseconds, ns) ) exc<except::thread_error>("micron: sleep duration is out of range");
  __wasm_sleep_checked(ns);
}

inline void
wait_for(fduration_t milliseconds)
{
  sleep_duration(milliseconds);
}

inline void
spin_for(fduration_t milliseconds)
{
  sleep_duration(milliseconds);
}

inline void pause() = delete;
template<typename F, typename... Args> void pause(F, Args...) = delete;
inline int wait(int) = delete;
inline int wait_thread(int) = delete;
inline int can_wait(int) = delete;
inline int try_wait_thread(int) = delete;
template<typename... Args> int await(Args...) = delete;
}      // namespace micron
