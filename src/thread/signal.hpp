//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sys/signal.hpp"

#include "../linux/sys/signal.hpp"
#include "../type_traits.hpp"
#include <sched.h> /* Definition of CLONE_* constants */
#include <spawn.h>
#include <unistd.h>     // fork, close, daemon

#include "../array/arrays.hpp"
#include "../types.hpp"
namespace micron
{

enum class signals : i32 {
  error = -1,
  deflt = 0,
  ignore = 1,
  hangup = sig_hup,
  interrupt = sig_int,
  quit = sig_quit,
  illegal = sig_ill,
  trap = sig_trap,
  abort = sig_abrt,
  iot = sig_iot,
  floating_error = sig_fpe,
  kill9 = sig_kill,
  user_signal_1 = sig_usr1,
  segfault = sig_segv,
  user_signal_2 = sig_usr2,
  pipe = sig_pipe,
  alarm = sig_alrm,
  terminate = sig_term,
  urgent = sig_urg,
  child = sig_chld,
  cont = sig_cont,
  stop = sig_stop,
  tstp = sig_tstp,
  ttin = sig_ttin,     // forgot what this one is xc won't loop it up
  ttou = sig_ttou,     // likewise
  urgent_2 = sig_urg,
  polling = sig_poll,
  xcpu = sig_xcpu,
  file_limit = sig_xfsz,
  virt_alarm = sig_vtalrm,
  profile_expire = sig_prof,
  window_resize = sig_winch,
  usr1 = sig_usr1,
  usr2 = sig_usr2
};

// mark procmask on constructor if true
template <bool M = false> class signal
{
  alignas(16) micron::sigset_t _signal;
  // BUG: since multiple signals have the same index, the acts array is technically too large and will have conflicting
  // signals
  micron::array<sigaction_t, 32> _acts;
  int _sig;

public:
  ~signal()
  {
    // revert all changes to default
    micron::sigset_t _default_signal;
    micron::sigemptyset(_default_signal);
    micron::sigprocmask(sig_setmask, _default_signal, NULL);
  }
  signal(void) : _acts(), _sig(0) { micron::sigemptyset(_signal); }
  template <typename... Sigs> signal(const Sigs... sig) : _acts{}
  {
    // micron::czero<sizeof(sigaction)>(&act);
    micron::sigemptyset(_signal);
    (micron::sigaddset(_signal, (i32)sig), ...);
    if constexpr ( M ) {
      micron::sigprocmask(sig_block, _signal, nullptr);
    }
    _sig = 0;
  }
  signal(const signal &) = default;
  signal(signal &&) = default;
  signal &operator=(const signal &) = default;
  signal &operator=(signal &&) = default;
  // mask masks the signals, to be caugh via operator()
  int
  mask(void) const
  {
    return micron::sigprocmask(sig_block, _signal, nullptr);
  }
  // on signal sets a universal function callback, when a signal is received call the function
  int
  on_signal(const signals s, void (*fhandler)(int))
  {
    if ( (u64)s > _acts.size() )
      return -1;
    _acts[(i32)s].sigaction_handler.sa_handler = fhandler;
    return micron::sigaction((i32)s, _acts[(i32)s], nullptr);
  }
  micron::sigset_t &
  get_signal()
  {
    return _signal;
  };
  int
  wait(void) // NOTE: can't be const sigwait modifies _sig
  {
    return micron::sigwait(_signal, _sig);
  }
  int
  operator()(void)     // wait for signal
  {
    return micron::sigwait(_signal, _sig);
  }
};
};
