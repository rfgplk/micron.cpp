//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "sync/pause.hpp"

#include "linux/__includes.hpp"
#include "thread/signal.hpp"
#include "types.hpp"

#include "exit.hpp"

namespace micron
{

inline void
alarm()
{
}

// a method to quickly crash a ring3 program
// pick your poison
template <int x = 0>
__attribute__((noreturn)) void
crash(void)
{
  if constexpr ( !x ) {
    __asm__ __volatile__("hlt");     // illegal instruction, the worst kind of userspace violation. immediately killthe
                                     // running program (including all children & threads), uncatchable and unstoppable,
                                     // guaranteed to work on any OS/kernel running x64 cpus. cannot be optimized away
  }
  if constexpr ( x == 1 ) {
    posix::raise(sig_segv);
  } else if constexpr ( x == 2 ) {
    // assert(false);
  } else if constexpr ( x ) {
    char *a = nullptr;
    for ( int i = 0;; i++ )
      *(a + i) = 0xFF;
  }
}

template <int P = poll_in>
inline auto
make_poll(const io::fd_t &hnd) -> pollfd
{
  if ( hnd.has_error() )
    return {};
  pollfd pfd = {};
  pfd.fd = hnd.fd;
  pfd.events = P;
  return pfd;
}

inline int
poll_for(pollfd &pfd, const int timeout)
{
  return micron::poll(pfd, 1, timeout);
}

// routines for controlling program state
inline void
halt()
{
}

inline void
stop()
{
  posix::raise(sig_stop);
}

__attribute__((noinline)) __attribute__((used)) __attribute__((optimize("O0"))) void
mark()
{
}
};     // namespace micron
