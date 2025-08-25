//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#ifndef SIG_ERR
#define SIG_ERR ((sig_t) - 1)
#endif

#ifndef SIG_DFL
#define SIG_DFL ((sig_t)0)
#endif

#ifndef SIG_IGN
#define SIG_IGN ((sig_t)1)
#endif

#ifndef SIG_HUP
#define SIG_HUP 1
#endif

#ifndef SIG_INT
#define SIG_INT 2
#endif

#ifndef SIG_QUIT
#define SIG_QUIT 3
#endif

#ifndef SIG_ILL
#define SIG_ILL 4
#endif

#ifndef SIG_TRAP
#define SIG_TRAP 5
#endif

#ifndef SIG_ABRT
#define SIG_ABRT 6
#endif

#ifndef SIG_IOT
#define SIG_IOT 6
#endif

#ifndef SIG_FPE
#define SIG_FPE 8
#endif

#ifndef SIG_KILL
#define SIG_KILL 9
#endif

#ifndef SIG_USR1
#define SIG_USR1 10
#endif
#ifndef SIG_SEGV
#define SIG_SEGV 11
#endif

#ifndef SIG_USR2
#define SIG_USR2 12
#endif

#ifndef SIG_PIPE
#define SIG_PIPE 13
#endif

#ifndef SIG_ALRM
#define SIG_ALRM 14
#endif

#ifndef SIG_TERM
#define SIG_TERM 15
#endif

#ifndef SIG_URG
#define SIG_URG 16
#endif

#ifndef SIG_CHLD
#define SIG_CHLD 17
#endif

#ifndef SIG_CONT
#define SIG_CONT 18
#endif

#ifndef SIG_STOP
#define SIG_STOP 19
#endif

#ifndef SIG_TSTP
#define SIG_TSTP 20
#endif

#ifndef SIG_TTIN
#define SIG_TTIN 21
#endif

#ifndef SIG_TTOU
#define SIG_TTOU 22
#endif

#ifndef SIG_URG
#define SIG_URG 23
#endif

#ifndef SIG_POLL
#define SIG_POLL 23
#endif

#ifndef SIG_XCPU
#define SIG_XCPU 24
#endif

#ifndef SIG_XFSZ
#define SIG_XFSZ 25
#endif

#ifndef SIG_VTALRM
#define SIG_VTALRM 26
#endif

#ifndef SIG_PROF
#define SIG_PROF 27
#endif

#ifndef SIG_WINCH
#define SIG_WINCH 28
#endif

#ifndef SIG_USR1
#define SIG_USR1 30
#endif

#ifndef SIG_USR2
#define SIG_USR2 31
#endif

// #include <linux/sched.h> /* Definition of struct clone_args */
#include <sched.h> /* Definition of CLONE_* constants */
#include <spawn.h>
// #include <sys/syscall.h>  /* Definition of SYS_* constants */
//#include <signal.h>
#include "../linux/sys/signal.hpp"
//#include <sys/wait.h>     // waitpid
//#include <time.h>
#include "../type_traits.hpp"
#include <unistd.h>     // fork, close, daemon

#include "array.hpp"
#include "types.hpp"
namespace micron
{

enum class signals : i32 {
  error = -1,
  deflt = 0,
  ignore = 1,
  hangup = SIG_HUP,
  interrupt = SIG_INT,
  quit = SIG_QUIT,
  illegal = SIG_ILL,
  trap = SIG_TRAP,
  abort = SIG_ABRT,
  iot = SIG_IOT,
  floating_error = SIG_FPE,
  kill9 = SIG_KILL,
  user_signal_1 = SIG_USR1,
  segfault = SIG_SEGV,
  user_signal_2 = SIG_USR2,
  pipe = SIG_PIPE,
  alarm = SIG_ALRM,
  terminate = SIG_TERM,
  urgent = SIG_URG,
  child = SIG_CHLD,
  cont = SIG_CONT,
  stop = SIG_STOP,
  tstp = SIG_TSTP,
  ttin = SIG_TTIN,     // forgot what this one is xc won't loop it up
  ttou = SIG_TTOU,     // likewise
  urgent_2 = SIG_URG,
  polling = SIG_POLL,
  xcpu = SIG_XCPU,
  file_limit = SIG_XFSZ,
  virt_alarm = SIG_VTALRM,
  profile_expire = SIG_PROF,
  window_resize = SIG_WINCH,
  usr1 = SIG_USR1,
  usr2 = SIG_USR2
};

// mark procmask on constructor if true
template <bool M = false> class signal
{
  sigset_t _signal;
  // BUG: since multiple signals have the same index, the acts array is technically too large and will have conflicting
  // signals
  micron::array<struct sigaction, 32> _acts;
  int _sig;

public:
  ~signal()
  {
    // revert all changes to default
    sigset_t _default_signal;
    ::sigemptyset(&_default_signal);
    ::sigprocmask(SIG_SETMASK, &_default_signal, NULL);
  }
  signal(void) : _acts(), _sig(0) { ::sigemptyset(&_signal); }
  template <typename... Sigs> signal(const Sigs... sig) : _acts{}
  {
    // micron::czero<sizeof(sigaction)>(&act);
    ::sigemptyset(&_signal);
    (::sigaddset(&_signal, (i32)sig), ...);
    if constexpr ( M ) {
      ::sigprocmask(SIG_BLOCK, &_signal, nullptr);
    }
    _sig = 0;
  }
  signal(const signal &) = default;
  signal(signal &&) = default;
  signal &operator=(const signal &) = default;
  signal &operator=(signal &&) = default;
  // mask masks the signals, to be caugh via operator()
  void
  mask(void) const
  {
    ::sigprocmask(SIG_BLOCK, &_signal, nullptr);
  }
  // on signal sets a universal function callback, when a signal is received call the function
  void
  on_signal(const signals s, void (*fhandler)(int))
  {
    if ( (u64)s > _acts.size() )
      return;
    _acts[(i32)s].sa_handler = fhandler;
    ::sigaction((i32)s, &_acts[(i32)s], nullptr);
  }
  int
  operator()()     // wait for signal
  {
    return ::sigwait(&_signal, &_sig);
  }
};
};
