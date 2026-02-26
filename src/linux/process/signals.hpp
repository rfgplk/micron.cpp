//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../sys/signal.hpp"

#include "../../array/arrays.hpp"
#include "../../types.hpp"

namespace micron
{

enum class signal : i32 {
  error = -1,
  deflt = 0,
  ignore = 1,
  hangup = posix::sig_hup,
  interrupt = posix::sig_int,
  quit = posix::sig_quit,
  illegal = posix::sig_ill,
  trap = posix::sig_trap,
  abort = posix::sig_abrt,
  iot = posix::sig_iot,
  floating_error = posix::sig_fpe,
  kill9 = posix::sig_kill,
  user_signal_1 = posix::sig_usr1,
  segfault = posix::sig_segv,
  user_signal_2 = posix::sig_usr2,
  pipe = posix::sig_pipe,
  alarm = posix::sig_alrm,
  terminate = posix::sig_term,
  urgent = posix::sig_urg,
  child = posix::sig_chld,
  cont = posix::sig_cont,
  stop = posix::sig_stop,
  tstp = posix::sig_tstp,
  ttin = posix::sig_ttin,     // forgot what this one is xc won't loop it up
  ttou = posix::sig_ttou,     // likewise
  urgent_2 = posix::sig_urg,
  polling = posix::sig_poll,
  xcpu = posix::sig_xcpu,
  file_limit = posix::sig_xfsz,
  virt_alarm = posix::sig_vtalrm,
  profile_expire = posix::sig_prof,
  window_resize = posix::sig_winch,
  usr1 = posix::sig_usr1,
  usr2 = posix::sig_usr2
};

inline constexpr const char *
signal_name(signal sig) noexcept
{
  switch ( sig ) {
  case signal::hangup :
    return "SIGHUP";
  case signal::interrupt :
    return "SIGINT";
  case signal::quit :
    return "SIGQUIT";
  case signal::illegal :
    return "SIGILL";
  case signal::trap :
    return "SIGTRAP";
  case signal::abort :
    return "SIGABRT";
  case signal::floating_error :
    return "SIGFPE";
  case signal::kill9 :
    return "SIGKILL";
  case signal::user_signal_1 :
    return "SIGUSR1";
  case signal::segfault :
    return "SIGSEGV";
  case signal::user_signal_2 :
    return "SIGUSR2";
  case signal::pipe :
    return "SIGPIPE";
  case signal::alarm :
    return "SIGALRM";
  case signal::terminate :
    return "SIGTERM";
  case signal::urgent :
    return "SIGURG";
  case signal::child :
    return "SIGCHLD";
  case signal::cont :
    return "SIGCONT";
  case signal::stop :
    return "SIGSTOP";
  case signal::tstp :
    return "SIGTSTP";
  case signal::ttin :
    return "SIGTTIN";
  case signal::ttou :
    return "SIGTTOU";
  case signal::xcpu :
    return "SIGXCPU";
  case signal::file_limit :
    return "SIGXFSZ";
  case signal::virt_alarm :
    return "SIGVTALRM";
  case signal::profile_expire :
    return "SIGPROF";
  case signal::window_resize :
    return "SIGWINCH";
  case signal::polling :
    return "SIGPOLL";
  default :
    return "SIGUNKNOWN";
  }
}

inline constexpr bool
signal_maskable(signal sig) noexcept
{
  return sig != signal::kill9 && sig != signal::stop;
}

// does the sig produce a coredump or not
inline constexpr bool
signal_dumps_core(signal sig) noexcept
{
  switch ( sig ) {
  case signal::quit :
  case signal::illegal :
  case signal::trap :
  case signal::abort :
  case signal::floating_error :
  case signal::segfault :
  case signal::xcpu :
  case signal::file_limit :
    return true;
  default :
    return false;
  }
}

// by default, raise() like behavior
auto
send(signal sig, posix::pid_t who = posix::getpid())
{
  return posix::kill(who, static_cast<int>(sig));
}

auto
send_group(signal sig, posix::pid_t pgid)
{
  return posix::kill(-pgid, static_cast<int>(sig));
}

auto
send_all(signal sig)
{
  return posix::kill(-1, static_cast<int>(sig));
}

inline int
send_thread(posix::pid_t tgid, posix::pid_t tid, signal sig)
{
  return static_cast<int>(micron::syscall(SYS_tgkill, tgid, tid, static_cast<int>(sig)));
}

template <typename Fn>
  requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn, int>)
auto
create_handler(Fn &&fn, signal sig)
{
  posix::sigaction_t sa = {};
  sa.sigaction_handler.sa_handler = fn;
  micron::posix::sigemptyset(sa.sa_mask);
  sa.sa_flags = posix::sa_restart;
  micron::posix::sigaction(static_cast<int>(sig), sa, nullptr);
  return sa;
}

template <typename Fn>
  requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn, int>)
auto
add_action(posix::sigaction_t &sa, Fn &&fn, signal sig)
{
  sa.sigaction_handler.sa_handler = fn;
  micron::posix::sigaction(static_cast<int>(sig), sa, nullptr);
  return sa;
}

template <typename Fn>
  requires(micron::is_invocable_v<Fn, int, posix::siginfo_t *, void *>)
auto
create_info_handler(Fn &&fn, signal sig, int extra_flags = 0)
{
  posix::sigaction_t sa = {};
  sa.sigaction_handler.sa_sigaction = fn;
  micron::posix::sigemptyset(sa.sa_mask);
  sa.sa_flags = posix::sa_restart | posix::sa_siginfo | extra_flags;
  micron::posix::sigaction(static_cast<int>(sig), sa, nullptr);
  return sa;
}

inline int
ignore(signal sig)
{
  posix::sigaction_t sa = {};
  sa.sigaction_handler.sa_handler = reinterpret_cast<posix::sighandler_t>(1);     // SIG_IGN == (void*)1
  micron::posix::sigemptyset(sa.sa_mask);
  sa.sa_flags = 0;
  return micron::posix::sigaction(static_cast<int>(sig), sa, nullptr);
}

inline int
restore_default(signal sig)
{
  posix::sigaction_t sa = {};
  sa.sigaction_handler.sa_handler = reinterpret_cast<posix::sighandler_t>(0);     // SIG_DFL == (void*)0
  micron::posix::sigemptyset(sa.sa_mask);
  sa.sa_flags = 0;
  return micron::posix::sigaction(static_cast<int>(sig), sa, nullptr);
}

inline posix::sigaction_t
query_handler(signal sig)
{
  posix::sigaction_t old = {};
  posix::sigaction_t dummy = {};
  dummy.sigaction_handler.sa_handler = reinterpret_cast<posix::sighandler_t>(1);
  micron::posix::sigemptyset(dummy.sa_mask);
  dummy.sa_flags = 0;
  micron::posix::sigaction(static_cast<int>(sig), dummy, &old);
  micron::posix::sigaction(static_cast<int>(sig), old, nullptr);
  return old;
}

// similar to critical_section, blocks signal for lifetime duration
struct signal_mask_guard {
  posix::sigset_t saved = {};
  bool active = false;

  ~signal_mask_guard()
  {
    if ( active )
      micron::posix::sigprocmask(posix::sig_setmask, saved, nullptr);
  }

  signal_mask_guard() = default;

  explicit signal_mask_guard(const posix::sigset_t &mask) { block(mask); }

  template <typename... Sigs>
    requires(... && micron::is_same_v<Sigs, signal>)
  explicit signal_mask_guard(Sigs... sigs)
  {
    posix::sigset_t m = {};
    micron::posix::sigemptyset(m);
    (micron::posix::sigaddset(m, static_cast<int>(sigs)), ...);
    block(m);
  }

  // not copyable
  signal_mask_guard(const signal_mask_guard &) = delete;
  signal_mask_guard &operator=(const signal_mask_guard &) = delete;

  signal_mask_guard(signal_mask_guard &&o) noexcept : saved(o.saved), active(o.active) { o.active = false; }

  void
  block(const posix::sigset_t &mask)
  {
    micron::posix::sigprocmask(posix::sig_block, mask, &saved);
    active = true;
  }

  void
  release()
  {
    if ( active ) {
      micron::posix::sigprocmask(posix::sig_setmask, saved, nullptr);
      active = false;
    }
  }
};

inline posix::sigset_t
block_signal(signal sig)
{
  posix::sigset_t m = {}, old = {};
  micron::posix::sigemptyset(m);
  micron::posix::sigaddset(m, static_cast<int>(sig));
  micron::posix::sigprocmask(posix::sig_block, m, &old);
  return old;
}

inline posix::sigset_t
unblock_signal(signal sig)
{
  posix::sigset_t m = {}, old = {};
  micron::posix::sigemptyset(m);
  micron::posix::sigaddset(m, static_cast<int>(sig));
  micron::posix::sigprocmask(posix::sig_unblock, m, &old);
  return old;
}

inline posix::sigset_t
block_all()
{
  posix::sigset_t full = {}, old = {};
  micron::posix::sigfillset(full);
  micron::posix::sigprocmask(posix::sig_setmask, full, &old);
  return old;
}

inline void
restore_mask(const posix::sigset_t &saved)
{
  micron::posix::sigprocmask(posix::sig_setmask, saved, nullptr);
}

inline posix::sigset_t
current_mask()
{
  posix::sigset_t cur = {};
  posix::sigset_t empty = {};
  micron::posix::sigemptyset(empty);
  micron::posix::sigprocmask(posix::sig_block, empty, &cur);
  return cur;
}

inline bool
is_blocked(signal sig)
{
  return micron::posix::sigismember(current_mask(), static_cast<int>(sig)) == 1;
}

inline int
sig_wait_for(const posix::sigset_t &set, signal &out)
{
  int raw = 0;
  int err = micron::posix::sigwait(set, raw);
  out = static_cast<signal>(raw);
  return err;
}

inline int
sig_wait_for(signal sig)
{
  posix::sigset_t m = {};
  micron::posix::sigemptyset(m);
  micron::posix::sigaddset(m, static_cast<int>(sig));
  int raw = 0;
  return micron::posix::sigwait(m, raw);
}

template <typename... Sigs>
  requires(... && micron::is_same_v<Sigs, signal>)
inline posix::sigset_t
make_sigset(Sigs... sigs)
{
  posix::sigset_t m = {};
  micron::posix::sigemptyset(m);
  (micron::posix::sigaddset(m, static_cast<int>(sigs)), ...);
  return m;
}

template <typename... Sigs>
  requires(... && micron::is_same_v<Sigs, signal>)
inline posix::sigset_t
make_sigset_except(Sigs... sigs)
{
  posix::sigset_t m = {};
  micron::posix::sigfillset(m);
  (micron::posix::sigdelset(m, static_cast<int>(sigs)), ...);
  return m;
}

// block delivery of sigs
struct critical_section {
  posix::sigset_t saved = {};

  void
  enter()
  {
    saved = block_all();
  }

  void
  leave()
  {
    restore_mask(saved);
  }
};

// mark procmask on constructor if true
template <bool M = false> class signal_t
{
  alignas(16) posix::sigset_t __signal;
  // BUG: since multiple signals have the same index, the acts array is technically too large and will have conflicting
  // signals
  micron::array<posix::sigaction_t, 32> __acts;
  mutable int __sig;

public:
  ~signal_t()
  {
    // revert all changes to default
    posix::sigset_t default_signal;
    posix::sigemptyset(default_signal);
    posix::sigprocmask(posix::sig_setmask, default_signal, NULL);
  }

  signal_t(void) : __acts(), __sig(0) { posix::sigemptyset(__signal); }

  template <typename... Sigs> signal_t(const Sigs... sig) : __acts{}
  {
    posix::sigemptyset(__signal);
    (posix::sigaddset(__signal, (i32)sig), ...);
    if constexpr ( M ) {
      posix::sigprocmask(posix::sig_block, __signal, nullptr);
    }
    __sig = 0;
  }

  signal_t(const signal_t &) = default;
  signal_t(signal_t &&) = default;
  signal_t &operator=(const signal_t &) = default;
  signal_t &operator=(signal_t &&) = default;

  // mask masks the signals, to be caugh via operator()
  int
  mask(void) const
  {
    return posix::sigprocmask(posix::sig_block, __signal, nullptr);
  }

  // on signal sets a universal function callback, when a signal is received call the function
  int
  on_signal(const micron::signal s, void (*fhandler)(int))
  {
    if ( (u64)s > __acts.size() )
      return -1;
    __acts[(i32)s].sigaction_handler.sa_handler = fhandler;
    return posix::sigaction((i32)s, __acts[(i32)s], nullptr);
  }

  posix::sigset_t &
  get_signal()
  {
    return __signal;
  };

  int
  wait(void) const
  {
    return posix::sigwait(__signal, __sig);
  }

  int
  operator()(void) const
  {
    return posix::sigwait(__signal, __sig);
  }
};
};     // namespace micron
