//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/sys/__threads.hpp"
#include "../signal.hpp"

#include "../../control.hpp"

namespace micron
{

void
__thread_sigchld(int)
{
}

void
__thread_sigthrottle(int)
{
  // configure
  micron::ssleep(1);
}

void
__thread_cancel(int)
{
  pthread::cancel();
}

void
__thread_yield(int)
{
  micron::yield();
}

void
__thread_stop(int)
{
  pthread::__exit_thread();
}

void
__thread_handler()
{
  sigaction_t sa = {};
  sa.sigaction_handler.sa_handler = __thread_sigthrottle;
  micron::sigemptyset(sa.sa_mask);
  sa.sa_flags = sa_restart;
  micron::sigaction(sig_usr1, sa, nullptr);
  // YIELD
  sa.sigaction_handler.sa_handler = __thread_yield;
  micron::sigaction(sig_alrm, sa, nullptr);
  // YIELD

  // CANCEL
  sa.sigaction_handler.sa_handler = __thread_cancel;
  micron::sigemptyset(sa.sa_mask);
  sa.sa_flags = 0;
  micron::sigaction(sig_usr2, sa, nullptr);

  // TODO: investigate
  sa.sigaction_handler.sa_handler = __thread_stop;
  micron::sigemptyset(sa.sa_mask);
  sa.sa_flags = 0;
  micron::sigaction(sig_term, sa, nullptr);
  //
}

};     // namespace micron
