#pragma once

#include "../../errno.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../linux_types.hpp"

#include "../../memory/cmemory.hpp"

#include "time.hpp"

// NOTE: this is a bare-minimum implementation to make signals work. a LOT of functions/code are missing (especially for
// more advanced signal handling).

// WARNING: this is an absolute minumum for signal trampolines to work. there is a LOT of code missing, specifically in regards to debuggers (gdb et al.) which will entirely mess up any attempts at debugging/backtracing through signals. although the below code is 100% valid and will work in all instances, it will fail for debugging.
naked_fn
restore_rt(void)
{
  asm volatile("mov $15, %rax\n\t"
               "syscall\n\t");
}
namespace micron
{

using sig_t = void (*)(int);

// children exit codes
constexpr static const int cld_exited = 1;
constexpr static const int cld_killed = 2;
constexpr static const int cld_dumped = 3;
constexpr static const int cld_trapped = 4;
constexpr static const int cld_stopped = 5;
constexpr static const int cld_continued = 6;

constexpr static const int sig_hup = 1;
constexpr static const int sig_int = 2;
constexpr static const int sig_quit = 3;
constexpr static const int sig_ill = 4;
constexpr static const int sig_trap = 5;
constexpr static const int sig_abrt = 6;
constexpr static const int sig_iot = 6;
constexpr static const int sig_fpe = 8;
constexpr static const int sig_kill = 9;
constexpr static const int sig_usr1 = 10;
constexpr static const int sig_segv = 11;
constexpr static const int sig_usr2 = 12;
constexpr static const int sig_pipe = 13;
constexpr static const int sig_alrm = 14;
constexpr static const int sig_term = 15;
constexpr static const int sig_urg = 16;
constexpr static const int sig_chld = 17;
constexpr static const int sig_cont = 18;
constexpr static const int sig_stop = 19;
constexpr static const int sig_tstp = 20;
constexpr static const int sig_ttin = 21;
constexpr static const int sig_ttou = 22;
constexpr static const int sig_xcpu = 24;
constexpr static const int sig_xfsz = 25;
constexpr static const int sig_vtalrm = 26;
constexpr static const int sig_prof = 27;
constexpr static const int sig_winch = 28;
constexpr static const int sig_poll = 29;
constexpr static const int sig_io = 29;

constexpr static const int sig_block = 0;             /* Block signals.  */
constexpr static const int sig_unlock = 1;            /* Unblock signals.  */
constexpr static const int sig_setmask = 2;           /* Set the set of blocked signals.  */
constexpr static const int sa_nocldstop = 1;          /* Don't send SIGCHLD when children stop.  */
constexpr static const int sa_nocldwait = 2;          /* Don't create zombie on child death.  */
constexpr static const int sa_siginfo = 4;            /* Invoke signal-catching function with
                            three arguments instead of one.  */
constexpr static const int sa_onstack = 0x08000000;   /* Use signal stack by using `sa_restorer'. */
constexpr static const int sa_restart = 0x10000000;   /* Restart syscall on signal return.  */
constexpr static const int sa_nodefer = 0x40000000;   /* Don't automatically block the signal when
                                     its handler is being executed.  */
constexpr static const int sa_resethand = 0x80000000; /* Reset to SIG_DFL on entry to handler.  */
constexpr static const int sa_interrupt = 0x20000000; /* Historical no-op.  */

constexpr static const int sa_nomask = sa_nodefer;
constexpr static const int sa_oneshot = sa_resethand;
constexpr static const int sa_stack = sa_onstack;

#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

constexpr static const int __sigwords = 1;     // ???
constexpr static const int __sigsize
    = (1024 / (8 * sizeof(unsigned long int)));     //__sigwords * sizeof(unsigned long int);

struct sigset_t {
  unsigned long int __val[__sigsize];
};

union sigval_t {
  int sival_int;
  void *sival_ptr;
};

typedef void (*sighandler_t)(int);
struct siginfo_t {
  int si_signo; /* Signal number.  */
  int si_errno; /* If non-zero, an errno value associated with
                   this signal, as defined in <errno.h>.  */
  int si_code;  /* Signal code.  */
  int __pad0;   /* Explicit padding.  */

  union {
    int _pad[((128 / sizeof(int)) - 4)];

    /* kill().  */
    struct {
      posix::pid_t si_pid; /* Sending process ID.  */
      posix::uid_t si_uid; /* Real user ID of sending process.  */
    } _kill;

    /* POSIX.1b timers.  */
    struct {
      int si_tid;         /* Timer ID.  */
      int si_overrun;     /* Overrun count.  */
      sigval_t si_sigval; /* Signal value.  */
    } _timer;

    /* POSIX.1b signals.  */
    struct {
      posix::pid_t si_pid; /* Sending process ID.  */
      posix::uid_t si_uid; /* Real user ID of sending process.  */
      sigval_t si_sigval;  /* Signal value.  */
    } _rt;

    /* SIGCHLD.  */
    struct {
      posix::pid_t si_pid; /* Which child.	 */
      posix::uid_t si_uid; /* Real user ID of sending process.  */
      int si_status;       /* Exit value or signal.  */
      posix::clock_t si_utime;
      posix::clock_t si_stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS.  */
    struct {
      void *si_addr;         /* Faulting insn/memory ref.  */
      short int si_addr_lsb; /* Valid LSB of the reported address.  */
      union {
        /* used when si_code=SEGV_BNDERR */
        struct {
          void *_lower;
          void *_upper;
        } _addr_bnd;
        /* used when si_code=SEGV_PKUERR */
        uint32_t _pkey;
      } _bounds;
    } _sigfault;

    /* SIGPOLL.  */
    struct {
      long int si_band; /* Band event for SIGPOLL.  */
      int si_fd;
    } _sigpoll;

    /* SIGSYS.  */
    struct {
      void *_call_addr;   /* Calling user insn.  */
      int _syscall;       /* Triggering system call number.  */
      unsigned int _arch; /* AUDIT_ARCH_* of syscall.  */
    } _sigsys;
  } _sifields;
};

struct __syscall_sigaction_t {
  sighandler_t k_sa_handler;
  unsigned long sa_flags;
  void (*sa_restorer)(void);
  micron::sigset_t sa_mask;
};

struct sigaction_t {
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, siginfo_t *, void *);
  } sigaction_handler;

  micron::sigset_t sa_mask;

  int sa_flags;

  void (*sa_restorer)(void);
};

// start funcs

inline constexpr u64
sigmask(const u64 mask)
{
  return (1UL << (((mask)-1) % (8 * sizeof(unsigned long))));
}

inline constexpr int
sigword(const int mask)
{
  return ((mask - 1) / (8 * sizeof(unsigned long)));
}

inline void
sigemptyset(micron::sigset_t &set)
{
  auto cnt = __sigsize;
  while ( --cnt >= 0 )
    set.__val[cnt] = 0;
}

inline void
sigfillset(micron::sigset_t &set)
{
  auto cnt = __sigwords;
  while ( --cnt >= 0 )
    set.__val[cnt] = ~0UL;
}

inline int
sigisemptyset(const micron::sigset_t &set)
{
  auto cnt = __sigwords;
  u64 ret = set.__val[--cnt];
  while ( ret == 0 && --cnt >= 0 )
    ret = set.__val[cnt];
  return (ret == 0);
}

inline micron::sigset_t
sigandset(const micron::sigset_t &a, const micron::sigset_t &b)
{
  micron::sigset_t ret;
  auto cnt = __sigwords;
  while ( --cnt >= 0 )
    ret.__val[cnt] = a.__val[cnt] & b.__val[cnt];
  return ret;
}

inline micron::sigset_t
sigorset(const micron::sigset_t &a, const micron::sigset_t &b)
{
  micron::sigset_t ret;
  auto cnt = __sigwords;
  while ( --cnt >= 0 )
    ret.__val[cnt] = a.__val[cnt] | b.__val[cnt];
  return ret;
}

inline int
sigismember(const micron::sigset_t &a, int sig)
{
  u64 mask = sigmask(sig);
  int word = sigword(sig);
  return a.__val[word] & mask ? 1 : 0;
}

inline void
sigaddset(micron::sigset_t &a, int sig)
{
  u64 mask = sigmask(sig);
  int word = sigword(sig);
  a.__val[word] |= mask;
}

inline void
sigdelset(micron::sigset_t &a, int sig)
{
  u64 mask = sigmask(sig);
  int word = sigword(sig);
  a.__val[word] &= ~mask;
}

constexpr u64 __sig_syscall_size = 8;     // NOT THIS sizeof(micron::sigset_t);

// start of syscalls
inline int
sigprocmask(int how, const micron::sigset_t &set, micron::sigset_t *old)
{
  return static_cast<int>(micron::syscall(SYS_rt_sigprocmask, how, &set, old, __sig_syscall_size));
}

inline int
sigaction(int sig, const sigaction_t &action, sigaction_t *old)
{
  __syscall_sigaction_t __system_action = {};
  __syscall_sigaction_t __system_oldaction = {};

  __system_action.k_sa_handler = action.sigaction_handler.sa_handler;
  micron::bytecpy(&__system_action.sa_mask, &action.sa_mask, sizeof(micron::sigset_t));
  __system_action.sa_flags = (u32)action.sa_flags;
  __system_action.sa_flags |= 0x04000000;
  __system_action.sa_restorer = &restore_rt;
  // restorer
  int result = static_cast<int>(
      micron::syscall(SYS_rt_sigaction, sig, &__system_action, old ? &__system_oldaction : nullptr, __sig_syscall_size));
  if ( old && result >= 0 ) {
    old->sigaction_handler.sa_handler = __system_oldaction.k_sa_handler;
    micron::bytecpy(&old->sa_mask, &__system_oldaction.sa_mask, sizeof(micron::sigset_t));
    old->sa_flags = static_cast<int>(__system_oldaction.sa_flags);
    old->sa_restorer = __system_action.sa_restorer;
  }
  return result;
}

int
sigwait(const micron::sigset_t &set, int &sig)
{
  siginfo_t info = {};
  timespec_t ts = {};
  int ret = 0;
  do {
    ret = static_cast<int>(micron::syscall(SYS_rt_sigtimedwait, &set, &info, nullptr, __sig_syscall_size));
  } while ( ret < 0 && errno == EINTR );
  if ( ret < 0 )
    return errno;
  sig = info.si_signo;
  return 0;
}

};
