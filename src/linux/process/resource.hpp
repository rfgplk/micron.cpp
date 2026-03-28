//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../io/filesystem.hpp"
#include "../../memory/cmemory.hpp"
#include "../../string/strings.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"

#include "../sys/limits.hpp"
#include "../sys/resource.hpp"

namespace micron
{
// export it to ::micron
using rlim_t = posix::rlim_t;

struct proc_stat_t {
  pid_t pid;
  char comm[256];     // executable name (without path)
  char state;         // R=running, S=sleeping, D=disk-wait, Z=zombie, T=traced
  pid_t ppid;
  pid_t pgrp;
  int session;
  int tty_nr;
  pid_t tpgid;
  u32 flags;
  u64 minflt, cminflt;     // minor faults (self, waited children)
  u64 majflt, cmajflt;     // major faults (self, waited children)
  u64 utime, stime;        // user/system time in clock ticks
  i64 cutime, cstime;      // waited-children user/system ticks
  i64 priority, nice;
  i64 num_threads;
  u64 starttime;     // clock ticks since boot
  u64 vsize;         // virtual memory size in bytes
  i64 rss;           // resident set size in pages
  u64 rsslim;        // current soft limit on rss
};

// Parsed fields from /proc/PID/status
struct proc_status_t {
  char name[256];
  char state;
  pid_t pid, ppid, tracerpid;
  uid_t uid[4];     // [0]=real [1]=eff [2]=saved [3]=fs
  gid_t gid[4];
  i32 fd_size;              // number of file descriptor slots
  u64 vm_peak, vm_size;     // kB
  u64 vm_lck, vm_pin;
  u64 vm_hwm, vm_rss;
  u64 vm_data, vm_stk;
  u64 vm_exe, vm_lib;
  u64 vm_pte, vm_swap;
  i64 threads;
  u64 sig_pnd, shd_pnd;     // pending signals (thread/process)
  u64 sig_blk, sig_ign, sig_cgt;
  u64 cap_inh, cap_prm, cap_eff, cap_bnd, cap_amb;     // hex bitmasks
  u64 voluntary_ctxt, nonvoluntary_ctxt;
};

// Combined snapshot of all observable process resources
struct proc_resource_t {
  proc_stat_t stat;
  proc_status_t status;
  posix::rusage_t usage;
  posix::limits_t limits;
};

namespace __impl
{
template <usize N = 80>
micron::sstring<N>
proc_path(pid_t pid, const char *file)
{
  micron::sstring<N> p;
  p += "/proc/";
  if ( pid <= 0 ) {
    p += "self/";
  } else {
    char buf[24];
    int n = 0;
    pid_t t = pid;
    do {
      buf[n++] = char('0' + t % 10);
      t /= 10;
    } while ( t );
    for ( int i = n - 1; i >= 0; --i )
      p += buf[i];
    p += '/';
  }
  p += file;
  return p;
}

inline i64
parse_i64(const char *&itr, const char *end)
{
  while ( itr < end && (*itr == ' ' || *itr == '\t' || *itr == '\n') )
    ++itr;
  bool neg = (itr < end && *itr == '-');
  if ( neg )
    ++itr;
  i64 v = 0;
  while ( itr < end && *itr >= '0' && *itr <= '9' )
    v = v * 10 + (*itr++ - '0');
  return neg ? -v : v;
}

inline u64
parse_u64(const char *&itr, const char *end)
{
  while ( itr < end && (*itr == ' ' || *itr == '\t' || *itr == '\n') )
    ++itr;
  u64 v = 0;
  while ( itr < end && *itr >= '0' && *itr <= '9' )
    v = v * 10 + (*itr++ - '0');
  return v;
}

inline u64
parse_hex(const char *&itr, const char *end)
{
  while ( itr < end && (*itr == ' ' || *itr == '\t' || *itr == '\n') )
    ++itr;
  u64 v = 0;
  while ( itr < end ) {
    char c = *itr;
    if ( c >= '0' && c <= '9' ) {
      v = v * 16 + u64(c - '0');
      ++itr;
    } else if ( c >= 'a' && c <= 'f' ) {
      v = v * 16 + u64(c - 'a' + 10);
      ++itr;
    } else if ( c >= 'A' && c <= 'F' ) {
      v = v * 16 + u64(c - 'A' + 10);
      ++itr;
    } else
      break;
  }
  return v;
}

inline const char *
find_ch(const char *p, const char *end, char needle)
{
  for ( ; p < end; ++p )
    if ( *p == needle )
      return p;
  return nullptr;
}

inline void
parse_status_line(proc_status_t &s, const char *ls, const char *le)
{
  const char *colon = find_ch(ls, le, ':');
  if ( !colon )
    return;
  usize klen = static_cast<usize>(colon - ls);
  const char *val = colon + 1;

#define KIS(k) (klen == sizeof(k) - 1 && micron::memcmp<byte>(ls, k, klen) == 0)

  if ( KIS("Name") ) {
    usize n = 0;
    while ( val + n < le && val[n] != '\n' && n < 255 )
      ++n;
    micron::memcpy(s.name, val, n);
    s.name[n] = '\0';
  } else if ( KIS("State") ) {
    while ( val < le && *val == ' ' )
      ++val;
    if ( val < le )
      s.state = *val;
  } else if ( KIS("Pid") ) {
    s.pid = static_cast<pid_t>(parse_i64(val, le));
  } else if ( KIS("PPid") ) {
    s.ppid = static_cast<pid_t>(parse_i64(val, le));
  } else if ( KIS("TracerPid") ) {
    s.tracerpid = static_cast<pid_t>(parse_i64(val, le));
  } else if ( KIS("Uid") ) {
    for ( int i = 0; i < 4; ++i )
      s.uid[i] = static_cast<uid_t>(parse_u64(val, le));
  } else if ( KIS("Gid") ) {
    for ( int i = 0; i < 4; ++i )
      s.gid[i] = static_cast<gid_t>(parse_u64(val, le));
  } else if ( KIS("FDSize") ) {
    s.fd_size = static_cast<i32>(parse_i64(val, le));
  } else if ( KIS("VmPeak") ) {
    s.vm_peak = parse_u64(val, le);
  } else if ( KIS("VmSize") ) {
    s.vm_size = parse_u64(val, le);
  } else if ( KIS("VmLck") ) {
    s.vm_lck = parse_u64(val, le);
  } else if ( KIS("VmPin") ) {
    s.vm_pin = parse_u64(val, le);
  } else if ( KIS("VmHWM") ) {
    s.vm_hwm = parse_u64(val, le);
  } else if ( KIS("VmRSS") ) {
    s.vm_rss = parse_u64(val, le);
  } else if ( KIS("VmData") ) {
    s.vm_data = parse_u64(val, le);
  } else if ( KIS("VmStk") ) {
    s.vm_stk = parse_u64(val, le);
  } else if ( KIS("VmExe") ) {
    s.vm_exe = parse_u64(val, le);
  } else if ( KIS("VmLib") ) {
    s.vm_lib = parse_u64(val, le);
  } else if ( KIS("VmPTE") ) {
    s.vm_pte = parse_u64(val, le);
  } else if ( KIS("VmSwap") ) {
    s.vm_swap = parse_u64(val, le);
  } else if ( KIS("Threads") ) {
    s.threads = parse_i64(val, le);
  } else if ( KIS("SigPnd") ) {
    s.sig_pnd = parse_hex(val, le);
  } else if ( KIS("ShdPnd") ) {
    s.shd_pnd = parse_hex(val, le);
  } else if ( KIS("SigBlk") ) {
    s.sig_blk = parse_hex(val, le);
  } else if ( KIS("SigIgn") ) {
    s.sig_ign = parse_hex(val, le);
  } else if ( KIS("SigCgt") ) {
    s.sig_cgt = parse_hex(val, le);
  } else if ( KIS("CapInh") ) {
    s.cap_inh = parse_hex(val, le);
  } else if ( KIS("CapPrm") ) {
    s.cap_prm = parse_hex(val, le);
  } else if ( KIS("CapEff") ) {
    s.cap_eff = parse_hex(val, le);
  } else if ( KIS("CapBnd") ) {
    s.cap_bnd = parse_hex(val, le);
  } else if ( KIS("CapAmb") ) {
    s.cap_amb = parse_hex(val, le);
  } else if ( KIS("voluntary_ctxt_switches") ) {
    s.voluntary_ctxt = parse_u64(val, le);
  } else if ( KIS("nonvoluntary_ctxt_switches") ) {
    s.nonvoluntary_ctxt = parse_u64(val, le);
  }

#undef KIS
}

};     // namespace __impl

inline proc_stat_t
read_proc_stat(pid_t pid = 0)
{
  proc_stat_t s{};
  micron::string raw;
  fsys::system<micron::io::rd> sys;
  sys[__impl::proc_path(pid, "stat").c_str()] >> raw;
  if ( raw.empty() )
    return s;

  const char *p = raw.cbegin();
  const char *end = raw.cend();

  s.pid = static_cast<pid_t>(__impl::parse_i64(p, end));

  // comm is inside parentheses; find them from both ends to handle embedded ')'
  while ( p < end && *p != '(' )
    ++p;
  const char *cn_s = ++p;
  while ( p < end && *p != ')' )
    ++p;     // last ')' found via reverse search is safer
  // find the last ')' from end
  {
    const char *rp = end - 1;
    while ( rp > cn_s && *rp != ')' )
      --rp;
    usize cn = static_cast<usize>(rp - cn_s);
    if ( cn >= 255 )
      cn = 255;
    micron::memcpy(s.comm, cn_s, cn);
    s.comm[cn] = '\0';
    p = rp + 1;
  }

  // state
  while ( p < end && *p == ' ' )
    ++p;
  if ( p < end ) {
    s.state = *p;
    ++p;
  }

  s.ppid = static_cast<pid_t>(__impl::parse_i64(p, end));
  s.pgrp = static_cast<pid_t>(__impl::parse_i64(p, end));
  s.session = static_cast<int>(__impl::parse_i64(p, end));
  s.tty_nr = static_cast<int>(__impl::parse_i64(p, end));
  s.tpgid = static_cast<pid_t>(__impl::parse_i64(p, end));
  s.flags = static_cast<u32>(__impl::parse_u64(p, end));
  s.minflt = __impl::parse_u64(p, end);
  s.cminflt = __impl::parse_u64(p, end);
  s.majflt = __impl::parse_u64(p, end);
  s.cmajflt = __impl::parse_u64(p, end);
  s.utime = __impl::parse_u64(p, end);
  s.stime = __impl::parse_u64(p, end);
  s.cutime = __impl::parse_i64(p, end);
  s.cstime = __impl::parse_i64(p, end);
  s.priority = __impl::parse_i64(p, end);
  s.nice = __impl::parse_i64(p, end);
  s.num_threads = __impl::parse_i64(p, end);
  __impl::parse_i64(p, end);     // itrealvalue — always 0, obsolete
  s.starttime = __impl::parse_u64(p, end);
  s.vsize = __impl::parse_u64(p, end);
  s.rss = __impl::parse_i64(p, end);
  s.rsslim = __impl::parse_u64(p, end);
  return s;
}

inline proc_status_t
read_proc_status(pid_t pid = 0)
{
  proc_status_t s{};
  micron::string raw;
  fsys::system<micron::io::rd> sys;
  sys[__impl::proc_path(pid, "status").c_str()] >> raw;
  if ( raw.empty() )
    return s;

  const char *p = raw.cbegin();
  const char *end = raw.cend();
  while ( p < end ) {
    const char *le = __impl::find_ch(p, end, '\n');
    if ( !le )
      le = end;
    __impl::parse_status_line(s, p, le);
    p = le + 1;
  }
  return s;
}

template <posix::limits L>
inline posix::rlimit_t
get_limit(pid_t pid = 0)
{
  posix::rlimit_t rl{};
  posix::get_process_limits(pid, static_cast<rlim_t>(L), rl);
  return rl;
}

template <posix::limits L>
inline int
set_limit(pid_t pid, posix::rlimit_t rl)
{
  return static_cast<int>(posix::set_process_limits(pid, static_cast<rlim_t>(L), rl));
}

template <posix::limits L>
inline int
set_soft_limit(pid_t pid, rlim_t soft)
{
  posix::rlimit_t rl{};
  posix::get_process_limits(pid, static_cast<rlim_t>(L), rl);
  rl.rlim_cur = soft;
  return static_cast<int>(posix::set_process_limits(pid, static_cast<rlim_t>(L), rl));
}

template <posix::limits L>
inline int
set_hard_limit(pid_t pid, rlim_t hard)
{
  posix::rlimit_t rl{};
  posix::get_process_limits(pid, static_cast<rlim_t>(L), rl);
  rl.rlim_max = hard;
  return static_cast<int>(posix::set_process_limits(pid, static_cast<rlim_t>(L), rl));
}

inline int
apply_limits(pid_t pid, const posix::limits_t &lims)
{
  int err = 0;
  for ( rlim_t i = 0; i < posix::rlimit_nlimits; ++i ) {
    auto rl = lims.lim[i];
    auto r = posix::set_process_limits(pid, i, rl);
    if ( r < 0 && err == 0 )
      err = static_cast<int>(r);
  }
  return err;
}

constexpr long user_hz = 100;

inline posix::rusage_t
get_rusage_proc(pid_t pid)
{
  posix::rusage_t r{};
  proc_stat_t st = read_proc_stat(pid);
  proc_status_t sts = read_proc_status(pid);

  r.ru_utime.tv_sec = static_cast<long>(st.utime / user_hz);
  r.ru_utime.tv_usec = static_cast<long>((st.utime % user_hz) * (1000000 / user_hz));
  r.ru_stime.tv_sec = static_cast<long>(st.stime / user_hz);
  r.ru_stime.tv_usec = static_cast<long>((st.stime % user_hz) * (1000000 / user_hz));
  r.ru_maxrss = static_cast<kernel_long_t>(sts.vm_hwm);
  r.ru_minflt = static_cast<kernel_long_t>(st.minflt);
  r.ru_majflt = static_cast<kernel_long_t>(st.majflt);
  r.ru_nvcsw = static_cast<kernel_long_t>(sts.voluntary_ctxt);
  r.ru_nivcsw = static_cast<kernel_long_t>(sts.nonvoluntary_ctxt);
  return r;
}

inline proc_resource_t
get_process_resources(pid_t pid)
{
  proc_resource_t res;
  res.stat = read_proc_stat(pid);
  res.status = read_proc_status(pid);
  res.limits = posix::limits_t(pid);
  if ( pid == 0 || pid == posix::getpid() )
    posix::getrusage(posix::rusage_self, res.usage);
  else
    res.usage = get_rusage_proc(pid);
  return res;
}

inline proc_resource_t
this_process_resources()
{
  return get_process_resources(0);
}

};     // namespace micron
