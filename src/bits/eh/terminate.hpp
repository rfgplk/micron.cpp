//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../exit.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "unwind.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// std::terminate && terminate/unexpected entry points

// special must be std namespace
namespace std
{
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();
};      // namespace std

namespace micron::eh
{

inline void
__eh_diag(const char *msg, usize len) noexcept
{
  micron::syscall(SYS_write, 2, reinterpret_cast<const void *>(msg), len);
}

inline std::terminate_handler __terminate_handler_slot = nullptr;
inline std::unexpected_handler __unexpected_handler_slot = nullptr;

[[noreturn]] inline void
__default_terminate() noexcept
{
  static const char m[] = "terminate called: uncaught exception\n";
  __eh_diag(m, sizeof(m) - 1);
  micron::abort(6);
}

[[noreturn]] inline void
__terminate() noexcept
{
  if ( __terminate_handler_slot ) {
    __terminate_handler_slot();
    // a terminate handler must not return; if it does, abort hard
  }
  __default_terminate();
}

};      // namespace micron::eh

namespace std
{

[[noreturn]] inline void
terminate() noexcept
{
  micron::eh::__terminate();
}

inline terminate_handler
set_terminate(terminate_handler h) noexcept
{
  terminate_handler old = micron::eh::__terminate_handler_slot;
  micron::eh::__terminate_handler_slot = h;
  return old;
}

inline terminate_handler
get_terminate() noexcept
{
  return micron::eh::__terminate_handler_slot;
}

inline unexpected_handler
set_unexpected(unexpected_handler h) noexcept
{
  unexpected_handler old = micron::eh::__unexpected_handler_slot;
  micron::eh::__unexpected_handler_slot = h;
  return old;
}

};      // namespace std

extern "C" {

[[noreturn]] inline void
__cxa_call_terminate(void *)
{
  micron::eh::__terminate();
}

[[noreturn]] inline void
__cxa_call_unexpected(void *)
{
  micron::eh::__terminate();
}

[[noreturn]] inline void
__cxa_pure_virtual()
{
  static const char m[] = "pure virtual function called\n";
  micron::eh::__eh_diag(m, sizeof(m) - 1);
  micron::abort(6);
}

[[noreturn]] inline void
__cxa_deleted_virtual()
{
  static const char m[] = "deleted virtual function called\n";
  micron::eh::__eh_diag(m, sizeof(m) - 1);
  micron::abort(6);
}

};      // extern "C"

#endif      // __micron_eh
