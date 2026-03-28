//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// libelves

#include "../process/spawn.hpp"
#include "../sys/clone.hpp"
#include "../sys/limits.hpp"

#include "../../pointer.hpp"
#include "../io.hpp"
#include "../sys/fcntl.hpp"

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../string/strings.hpp"
#include "../../vector/fvector.hpp"
#include "../../vector/svector.hpp"
#include "../../vector/vector.hpp"
#include "../__includes.hpp"
#include "fork.hpp"
#include "wait.hpp"

#include "capabilities.hpp"
#include "resource.hpp"

#include "../../io/filesystem.hpp"

#include "callbacks.hpp"

extern char **environ;

namespace micron
{

/*

   // TODO: add this

   struct uelf_t {
  int fd;
  byte *elf;
};

template <is_string S>
uelf_t
create_elf_memory(uelf_t &elf, const S &str, const S &str_)
{
  //uelf_t elf{
  //  memfd_create(str.c_str(), 0),
  //};
}*/

struct upid_t {
  uid_t uid;
  uid_t euid;
  gid_t gid;
  pid_t pid;
  pid_t ppid;
};

// TODO: extend with mmap regions
struct runtime_t {
  byte *stack;
  byte *heap;
};

inline runtime_t
load_stack_heap(void)
{
  micron::string dt;
  fsys::system<micron::io::rd> sys;
  io::path_t path = "/proc/self/maps";
  sys["/proc/self/maps"] >> dt;
  micron::string::iterator stck = micron::format::find(dt, "[stack]");
  micron::string::iterator heap = micron::format::find(dt, "[heap]");
  // NOTE: the reason this is here is because we might not always have a heap, in which case heap will be nullptr
  if ( !stck and !heap ) {
    runtime_t rt = { .stack = nullptr, .heap = nullptr };
    return rt;
  } else if ( heap ) {
    micron::string::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::string::iterator heap_nl = micron::format::find_reverse(dt, heap, "\n") + 1;
    micron::string::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    micron::string::iterator heap_ptr = micron::format::find(dt, heap_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::string>(stck_nl, stck_ptr);
    byte *hptr = micron::format::to_pointer_addr<byte, micron::string>(heap_nl, heap_ptr);
    runtime_t rt = { .stack = sptr, .heap = hptr };
    return rt;
  } else {
    micron::string::iterator stck_nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::string::iterator stck_ptr = micron::format::find(dt, stck_nl, "-");
    byte *sptr = micron::format::to_pointer_addr<byte, micron::string>(stck_nl, stck_ptr);
    runtime_t rt = { .stack = sptr, .heap = nullptr };
    return rt;
  }
  runtime_t rt = { .stack = nullptr, .heap = nullptr };
  return rt;
}

inline runtime_t
load_stack_heap_pid(pid_t pid)
{
  micron::string dt;
  fsys::system<micron::io::rd> sys;
  auto path = __impl::proc_path(pid, "maps");
  sys[path.c_str()] >> dt;

  runtime_t rt{ nullptr, nullptr };
  if ( dt.empty() )
    return rt;

  micron::string::iterator stck = micron::format::find(dt, "[stack]");
  micron::string::iterator heap = micron::format::find(dt, "[heap]");

  if ( stck ) {
    micron::string::iterator nl = micron::format::find_reverse(dt, stck, "\n") + 1;
    micron::string::iterator dash = micron::format::find(dt, nl, "-");
    rt.stack = micron::format::to_pointer_addr<byte, micron::string>(nl, dash);
  }
  if ( heap ) {
    micron::string::iterator nl = micron::format::find_reverse(dt, heap, "\n") + 1;
    micron::string::iterator dash = micron::format::find(dt, nl, "-");
    rt.heap = micron::format::to_pointer_addr<byte, micron::string>(nl, dash);
  }
  return rt;
}

micron::ptr_arr<char *>
vector_to_argv(const micron::svector<micron::string> &vec)
{
  auto argv = micron::unique_arr<char *>(vec.size() + 1);     // for nullptr
  for ( usize i = 0; i < vec.size(); ++i ) {
    argv[i] = const_cast<char *>(vec[i].c_str());
  }
  argv[vec.size()] = nullptr;
  return argv;
}

// uses containers rather than PODs, redesigned with spawn_ctx as info
struct uprocess_t {
  upid_t pids;     // pid of process
  micron::sstring<posix::path_max + 1> path;
  micron::svector<micron::string> argv;     // can be variable, up to cca 2 MiB
  micron::svector<micron::string> envp;
  posix::limits_t lims;     // limits of the process, slower to get, but makes handling effortless
  posix::cpu_set_t affinity;
  int status;
  ~uprocess_t() = default;

  uprocess_t(void)
      : pids{ posix::getuid(), posix::geteuid(), posix::getgid(), posix::getpid(), posix::getppid() }, path(), argv{}, envp{}, lims(0),
        affinity(), status(0)
  {
    // programatically get argv and environ
    micron::string str;
    fsys::system<micron::io::rd> sys;
    sys["/proc/self/cmdline"] >> str;
    umax_t k = 0;
    umax_t t = 0;
    // stored as null term strings, so we're iterating according to what we read
    for ( umax_t i = 0; i < str.size(); ++i ) {
      if ( str[i] == 0x0 ) {
        t = i;
        argv.emplace_back(str.begin() + k, str.begin() + t);
        k = t + 1;
      }
    }
    // argv[0] is always path, not absolute but works without reparsing
    path = argv[0];
    str.clear();
    k = 0;
    t = 0;
    sys["/proc/self/environ"] >> str;
    for ( umax_t i = 0; i < str.size(); ++i ) {
      if ( str[i] == 0x0 ) {
        t = i;
        envp.emplace_back(str.begin() + k, str.begin() + t);
        // our vector can only hold 64 elements, and as it turns out environ could easily exceed that
        if ( envp.full_or_overflowed() )
          break;
        k = t + 1;
      }
    }
    posix::get_affinity(pids.pid, affinity);
  }

  template <typename... Args>
  uprocess_t(const char *str, Args &&...args)
      : pids{}, path(str), argv{ micron::forward<Args>(args)... }, envp{}, lims(0), affinity(), status(0)
  {
  }

  template <typename... Args>
  uprocess_t(micron::sstring<posix::path_max + 1> &&o, Args &&...args)
      : pids{}, path(micron::move(o)), argv{ micron::forward<Args>(args)... }, envp{}, lims(0), affinity(), status(0)
  {
  }

  uprocess_t(const uprocess_t &o)
      : pids(o.pids), path(o.path), argv(o.argv), envp(o.envp), lims(o.lims), affinity(o.affinity), status(o.status)
  {
  }

  uprocess_t(uprocess_t &&o)
      : pids(micron::move(o.pids)), path(micron::move(o.path)), argv(micron::move(o.argv)), envp(micron::move(o.envp)),
        lims(micron::move(o.lims)), affinity(micron::move(o.affinity)), status(o.status)
  {
    o.status = 0;
  }

  template <typename... Args>
  uprocess_t &
  operator=(uprocess_t &&o)
  {

    pids = micron::move(o.pids);
    path = micron::move(o.path);
    argv = micron::move(o.argv);
    envp = micron::move(o.envp);
    lims = micron::move(o.lims);
    affinity = micron::move(o.affinity);
    status = o.status;
    o.status = 0;
    return *this;
  }

  uprocess_t &
  operator=(const uprocess_t &o)
  {
    pids = o.pids;
    path = o.path;
    argv = o.argv;
    envp = o.envp;
    lims = o.lims;
    affinity = o.affinity;
    status = o.status;
    return *this;
  }
};

typedef micron::fvector<uprocess_t> process_list_t;

template <is_string... S>
process_list_t
create_processes(S... names)
{
  process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

template <typename... S>
process_list_t
create_processes(S... names)
{
  process_list_t p;
  (p.emplace_back(names, names), ...);
  return p;
}

void
run_processes(process_list_t &n)
{
  for ( auto &t : n ) {
    micron::svector<char *> argv;
    for ( usize i = 0; i < t.argv.size(); i++ )
      argv.push_back(&t.argv[i][0]);
    argv.push_back(nullptr);
    t.pids.uid = posix::getuid();
    t.pids.gid = posix::getgid();
    if ( micron::spawn(t.pids.pid, t.path.c_str(), &argv[0], environ) ) {
      exc<except::system_error>("micron process failed to start spawn");
    }
  }
}

// implementation of daemon
template <int Stack = default_stack_size, typename F, typename... Args>
  requires micron::is_function_v<F> or micron::is_function_v<micron::remove_pointer_t<F>>
int
daemon(F f, Args &&...args)
{
  int pid = micron::fork();     // much nicer to do this
  if ( pid > 0 )                // parent
    micron::posix::exit(0);
  if ( posix::setsid() < 0 )
    exc<except::runtime_error>("micron process daemon failed to create new session");
  // don't change dir
  micron::umask(0);
  micron::close(stdin_fileno);
  micron::close(stdout_fileno);
  micron::close(stderr_fileno);

  micron::open("/dev/null", posix::o_rdwr);
  micron::dup(0);
  micron::dup(0);
  f(args...);
  return 0;
}

// ex this_task
uprocess_t
this_process(void)
{
  return uprocess_t();
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// /proc interface

struct proc_info_t {
  proc_stat_t stat;              // /proc/PID/stat
  proc_status_t status;          // /proc/PID/status
  proc_resource_t resources;     // limits (prlimit64) + rusage approximation
  ucap_set_t caps;               // capability bitmasks
  runtime_t runtime;             // [stack] and [heap] addresses from /proc/PID/maps
  // Process identity
  pid_t pid;
  pid_t ppid;
  char exe[posix::path_max + 1];     // resolved /proc/PID/exe symlink
};

namespace __impl
{
inline void
read_exe(pid_t pid, char *buf, usize bufsz)
{
  if ( bufsz == 0 )
    return;
  buf[0] = '\0';
  auto path = __impl::proc_path(pid, "exe");
  // readlink syscall
  max_t n = micron::readlink(path.c_str(), buf, bufsz - 1);
  if ( n > 0 )
    buf[n] = '\0';
}

};     // namespace __impl

inline proc_info_t
get_proc_info(pid_t pid)
{
  proc_info_t info{};
  info.pid = pid;
  info.stat = read_proc_stat(pid);
  info.status = read_proc_status(pid);
  info.ppid = info.stat.ppid;
  info.resources.stat = info.stat;
  info.resources.status = info.status;
  info.resources.limits = posix::limits_t(pid);
  info.resources.usage = get_rusage_proc(pid);

  info.caps = ucap_set_t(info.status.cap_eff, info.status.cap_prm, info.status.cap_inh, info.status.cap_bnd, info.status.cap_amb);

  info.runtime = load_stack_heap_pid(pid);
  __impl::read_exe(pid, info.exe, sizeof(info.exe));
  return info;
}

inline proc_info_t
this_proc_info()
{
  pid_t pid = posix::getpid();
  proc_info_t info{};
  info.pid = pid;
  info.stat = read_proc_stat(0);
  info.status = read_proc_status(0);
  info.ppid = info.stat.ppid;

  info.resources.stat = info.stat;
  info.resources.status = info.status;
  info.resources.limits = posix::limits_t(0);
  posix::getrusage(posix::rusage_self, info.resources.usage);

  info.caps = get_caps(0);
  info.runtime = load_stack_heap();
  __impl::read_exe(0, info.exe, sizeof(info.exe));
  return info;
}

inline proc_info_t
enrich(const uprocess_t &proc)
{
  return get_proc_info(proc.pids.pid);
}

struct child_result_t {
  pid_t pid;
  int exit_code;
  bool exited_normally;
  bool signaled;
  int term_signal;
  posix::rusage_t usage;
  proc_resource_t resources;
};

inline child_result_t
wait_and_collect(pid_t pid)
{
  child_result_t r{};
  r.pid = pid;

  int wstatus = 0;
  // peek with WNOHANG first so /proc/PID/ is still readable
  micron::wait4(pid, &wstatus, wnohang, &r.usage);

  r.resources = get_process_resources(pid);

  // reap
  micron::wait4(pid, &wstatus, 0, &r.usage);

  if ( WIFEXITED(wstatus) ) {
    r.exited_normally = true;
    r.exit_code = WEXITSTATUS(wstatus);
  } else if ( WIFSIGNALED(wstatus) ) {
    r.signaled = true;
    r.term_signal = WTERMSIG(wstatus);
    r.exit_code = -r.term_signal;
  }
  return r;
}

template <is_cap_set Caps>
void
run_processes(process_list_t &n, const Caps &caps)
{
  for ( auto &t : n ) {
    micron::svector<char *> argv;
    for ( usize i = 0; i < t.argv.size(); i++ )
      argv.push_back(&t.argv[i][0]);
    argv.push_back(nullptr);
    t.pids.uid = posix::getuid();
    t.pids.gid = posix::getgid();
    if ( spawn(t.pids.pid, t.path.c_str(), &argv[0], environ, caps) )
      exc<except::system_error>("micron process failed to start spawn");
  }
}

template <is_limits_set Lims>
void
run_processes_limited(process_list_t &n, const Lims &lims)
{
  for ( auto &t : n ) {
    micron::svector<char *> argv;
    for ( usize i = 0; i < t.argv.size(); i++ )
      argv.push_back(&t.argv[i][0]);
    argv.push_back(nullptr);
    t.pids.uid = posix::getuid();
    t.pids.gid = posix::getgid();
    // Each child gets a fresh copy of the same limits
    if ( spawn(t.pids.pid, t.path.c_str(), &argv[0], environ, lims) )
      exc<except::system_error>("micron process failed to start spawn");
  }
}

micron::fvector<child_result_t>
wait_and_collect(micron::fvector<uprocess_t> &procs)
{
  micron::fvector<child_result_t> results;
  for ( auto &p : procs )
    results.emplace_back(wait_and_collect(p.pids.pid));
  return results;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// additional spawn overloads

inline int
spawn(uprocess_t &t)
{
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  int err = spawn(t.pids.pid, t.path.c_str(), &argv[0], environ);
  t.status = err;
  return err;
}

template <is_cap_set Caps>
inline int
spawn(uprocess_t &t, const Caps &caps)
{
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  int err = spawn(t.pids.pid, t.path.c_str(), &argv[0], environ, caps);
  t.status = err;
  return err;
}

template <is_limits_set Lims>
inline int
spawn(uprocess_t &t, const Lims &lims)
{
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  int err = spawn(t.pids.pid, t.path.c_str(), &argv[0], environ, lims);
  t.status = err;
  return err;
}

template <is_cap_set Caps, is_limits_set Lims>
inline int
spawn(uprocess_t &t, const Caps &caps, const Lims &lims)
{
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  int err = spawn(t.pids.pid, t.path.c_str(), &argv[0], environ, caps, lims);
  t.status = err;
  return err;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// executes
// execute in a new process, don't replace current

template <is_cap_set Caps, bool W = exec_continue>
status_t
execute(uprocess_t &t, const Caps &caps)
{
  if ( spawn(t, caps) )
    exc<except::system_error>("micron process failed to start");
  status_t status;
  status.pid = t.pids.pid;
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_limits_set Lims>
status_t
execute(uprocess_t &t, const Lims &lims)
{
  if ( spawn(t, lims) )
    exc<except::system_error>("micron process failed to start");
  status_t status;
  status.pid = t.pids.pid;
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_cap_set Caps, is_limits_set Lims>
status_t
execute(uprocess_t &t, const Caps &caps, const Lims &lims)
{
  if ( spawn(t, caps, lims) )
    exc<except::system_error>("micron process failed to start");
  status_t status;
  status.pid = t.pids.pid;
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_string T, is_cap_set Caps>
status_t
execute(const T &t, const Caps &caps)
{
  status_t status;
  micron::vector<char *> argv = { const_cast<char *>(t.c_str()), nullptr };
  if ( spawn(status.pid, t.c_str(), &argv[0], environ, caps) )
    exc<except::system_error>("micron process failed to start");
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_string T, is_limits_set Lims>
status_t
execute(const T &t, const Lims &lims)
{
  status_t status;
  micron::vector<char *> argv = { const_cast<char *>(t.c_str()), nullptr };
  if ( spawn(status.pid, t.c_str(), &argv[0], environ, lims) )
    exc<except::system_error>("micron process failed to start");
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_string T, is_cap_set Caps, is_limits_set Lims>
status_t
execute(const T &t, const Caps &caps, const Lims &lims)
{
  status_t status;
  micron::vector<char *> argv = { const_cast<char *>(t.c_str()), nullptr };
  if ( spawn(status.pid, t.c_str(), &argv[0], environ, caps, lims) )
    exc<except::system_error>("micron process failed to start");
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_string T, is_string... R, is_cap_set Caps>
status_t
execute_with_caps(const T &t, const Caps &caps, const R &...args)
{
  status_t status;
  micron::vector<char *> argv = { const_cast<char *>(t.c_str()), nullptr };
  (argv.push_back(const_cast<char *>(args.c_str())), ...);
  argv.push_back(nullptr);
  if ( spawn(status.pid, t.c_str(), &argv[0], environ, caps) )
    exc<except::system_error>("micron process failed to start");
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

template <bool W = exec_continue, is_string T, is_string... R, is_limits_set Lims>
status_t
execute_with_limits(const T &t, const Lims &lims, const R &...args)
{
  status_t status;
  micron::vector<char *> argv = { const_cast<char *>(t.c_str()), nullptr };
  (argv.push_back(const_cast<char *>(args.c_str())), ...);
  argv.push_back(nullptr);
  if ( spawn(status.pid, t.c_str(), &argv[0], environ, lims) )
    exc<except::system_error>("micron process failed to start");
  if constexpr ( W )
    micron::waitpid(status);
  return status;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rexecutes
// replace current image with new proc

template <is_string T, is_cap_set Caps>
__attribute__((noreturn)) void
rexecute(const T &path, const Caps &caps)
{
  pid_t pid;
  apply_caps_child(caps);
  micron::vector<char *> argv = { const_cast<char *>(path.c_str()), nullptr };
  micron::inplace_spawn(pid, path.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

template <is_cap_set Caps>
__attribute__((noreturn)) void
rexecute(uprocess_t &t, const Caps &caps)
{
  apply_caps_child(caps);
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  posix::execve(t.path.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

template <is_limits_set Lims>
__attribute__((noreturn)) void
rexecute(uprocess_t &t, const Lims &lims)
{
  child_apply_limits(lims);
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  posix::execve(t.path.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

template <is_cap_set Caps, is_limits_set Lims>
__attribute__((noreturn)) void
rexecute(uprocess_t &t, const Caps &caps, const Lims &lims)
{
  child_apply_limits(lims);     // limits first
  apply_caps_child(caps);
  micron::svector<char *> argv;
  for ( usize i = 0; i < t.argv.size(); ++i )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  posix::execve(t.path.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

};     // namespace micron
