//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

#include "signal.hpp"
#include "types.hpp"

namespace micron
{
namespace posix
{

// classic requests
constexpr static const i32 ptrace_traceme = 0;
constexpr static const i32 ptrace_peektext = 1;
constexpr static const i32 ptrace_peekdata = 2;
constexpr static const i32 ptrace_peekuser = 3;
constexpr static const i32 ptrace_poketext = 4;
constexpr static const i32 ptrace_pokedata = 5;
constexpr static const i32 ptrace_pokeuser = 6;
constexpr static const i32 ptrace_cont = 7;
constexpr static const i32 ptrace_kill = 8;
constexpr static const i32 ptrace_singlestep = 9;
// NOTE: getregs/setregs are x86 only. arm64 has neither
constexpr static const i32 ptrace_getregs = 12;
constexpr static const i32 ptrace_setregs = 13;
constexpr static const i32 ptrace_attach = 16;
constexpr static const i32 ptrace_detach = 17;
constexpr static const i32 ptrace_syscall = 24;

// >=2.6.x extended requests
constexpr static const i32 ptrace_setoptions = 0x4200;
constexpr static const i32 ptrace_geteventmsg = 0x4201;
constexpr static const i32 ptrace_getsiginfo = 0x4202;
constexpr static const i32 ptrace_setsiginfo = 0x4203;
constexpr static const i32 ptrace_getregset = 0x4204;
constexpr static const i32 ptrace_setregset = 0x4205;
// >=3.4: attach without stopping, and stop without a signal
constexpr static const i32 ptrace_seize = 0x4206;
constexpr static const i32 ptrace_interrupt = 0x4207;
constexpr static const i32 ptrace_listen = 0x4208;

// options
constexpr static const u32 ptrace_o_tracesysgood = 0x00000001u;
constexpr static const u32 ptrace_o_tracefork = 0x00000002u;
constexpr static const u32 ptrace_o_tracevfork = 0x00000004u;
constexpr static const u32 ptrace_o_traceclone = 0x00000008u;
constexpr static const u32 ptrace_o_traceexec = 0x00000010u;
constexpr static const u32 ptrace_o_tracevforkdone = 0x00000020u;
constexpr static const u32 ptrace_o_traceexit = 0x00000040u;
constexpr static const u32 ptrace_o_traceseccomp = 0x00000080u;
constexpr static const u32 ptrace_o_suspend_seccomp = 0x00200000u;
// WARNING: exitkill kills every tracee when the tracer dies
constexpr static const u32 ptrace_o_exitkill = 0x00100000u;

// wait status event codes, read with ptrace_event_of()
constexpr static const i32 ptrace_event_fork = 1;
constexpr static const i32 ptrace_event_vfork = 2;
constexpr static const i32 ptrace_event_clone = 3;
constexpr static const i32 ptrace_event_exec = 4;
constexpr static const i32 ptrace_event_vfork_done = 5;
constexpr static const i32 ptrace_event_exit = 6;
constexpr static const i32 ptrace_event_seccomp = 7;
// the stop a seize'd tracee reports for PTRACE_INTERRUPT and for group-stop
constexpr static const i32 ptrace_event_stop = 128;

inline long
ptrace(i32 req, pid_t pid, void *addr, void *data)
{
  return micron::syscall(SYS_ptrace, req, pid, addr, data);
}

namespace __impl
{

[[gnu::always_inline]] inline void *
pt_arg(u64 v)
{
  return reinterpret_cast<void *>(static_cast<uintptr_t>(v));
}

};      // namespace __impl

inline i32
traceme(void)
{
  return static_cast<i32>(ptrace(ptrace_traceme, 0, nullptr, nullptr));
}

// attach without stopping; the tracee keeps running until an interrupt
inline i32
seize(pid_t pid, u32 opts = 0)
{
  return static_cast<i32>(ptrace(ptrace_seize, pid, nullptr, __impl::pt_arg(opts)));
}

// stop a seize'd tracee without injecting a signal; reports ptrace_event_stop
inline i32
interrupt(pid_t pid)
{
  return static_cast<i32>(ptrace(ptrace_interrupt, pid, nullptr, nullptr));
}

inline i32
detach(pid_t pid, i32 sig = 0)
{
  return static_cast<i32>(ptrace(ptrace_detach, pid, nullptr, __impl::pt_arg(static_cast<u64>(sig))));
}

inline i32
cont(pid_t pid, i32 sig = 0)
{
  return static_cast<i32>(ptrace(ptrace_cont, pid, nullptr, __impl::pt_arg(static_cast<u64>(sig))));
}

inline i32
singlestep(pid_t pid, i32 sig = 0)
{
  return static_cast<i32>(ptrace(ptrace_singlestep, pid, nullptr, __impl::pt_arg(static_cast<u64>(sig))));
}

inline i32
listen(pid_t pid)
{
  return static_cast<i32>(ptrace(ptrace_listen, pid, nullptr, nullptr));
}

inline i32
setoptions(pid_t pid, u32 opts)
{
  return static_cast<i32>(ptrace(ptrace_setoptions, pid, nullptr, __impl::pt_arg(opts)));
}

inline i32
geteventmsg(pid_t pid, unsigned long &msg)
{
  return static_cast<i32>(ptrace(ptrace_geteventmsg, pid, nullptr, &msg));
}

inline i32
getsiginfo(pid_t pid, siginfo_t &info)
{
  return static_cast<i32>(ptrace(ptrace_getsiginfo, pid, nullptr, &info));
}

inline i32
setsiginfo(pid_t pid, const siginfo_t &info)
{
  return static_cast<i32>(ptrace(ptrace_setsiginfo, pid, nullptr, const_cast<siginfo_t *>(&info)));
}

inline i32
peekdata(pid_t pid, u64 addr, unsigned long &word)
{
  return static_cast<i32>(ptrace(ptrace_peekdata, pid, __impl::pt_arg(addr), &word));
}

inline i32
pokedata(pid_t pid, u64 addr, unsigned long word)
{
  return static_cast<i32>(ptrace(ptrace_pokedata, pid, __impl::pt_arg(addr), __impl::pt_arg(word)));
}

inline i32
peektext(pid_t pid, u64 addr, unsigned long &word)
{
  return static_cast<i32>(ptrace(ptrace_peektext, pid, __impl::pt_arg(addr), &word));
}

inline i32
poketext(pid_t pid, u64 addr, unsigned long word)
{
  return static_cast<i32>(ptrace(ptrace_poketext, pid, __impl::pt_arg(addr), __impl::pt_arg(word)));
}

// how many bytes one peek/poke moves
constexpr static const usize ptrace_word = sizeof(unsigned long);

// the event code out of a wait status, or 0 when the stop carried none
constexpr i32
ptrace_event_of(i32 status)
{
  return (status >> 16) & 0xff;
}

constexpr bool
ptrace_is_event_stop(i32 status)
{
  return ptrace_event_of(status) == ptrace_event_stop;
}

};      // namespace posix
};      // namespace micron
