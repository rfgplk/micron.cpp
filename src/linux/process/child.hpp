//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// child
// functional porcelain interface

#include "process.hpp"
#include "signals.hpp"
#include "spawn.hpp"

#include "../poll.hpp"
#include "../sys/spawn.hpp"

#include "../../function.hpp"
#include "../../string/strings.hpp"
#include "../../sum.hpp"
#include "../../vector/vector.hpp"

namespace micron
{

struct exit_status {
  int code = 0;               // exit code; or -signal when terminated by a signal
  bool exited = false;        // exited normally (code is the exit code)
  bool signaled = false;      // killed by a signal (term_signal set)
  int term_signal = 0;

  [[nodiscard]] bool
  success() const noexcept
  {
    return exited && code == 0;
  }
};

struct comm_result {
  exit_status status;
  micron::string out;
  micron::string err;
};

struct spawn_error {
  enum stage_t : u8 { ok = 0, pipe, fork, spawn };

  int err = 0;      // positive errno
  stage_t stage = ok;
};

enum class redirect : u8 { inherit, pipe, null, to_fd };

struct stdio_spec {
  redirect kind = redirect::inherit;
  int fd = -1;      // target when kind == to_fd
};

struct child_spec {
  micron::sstring<posix::path_max + 1> path;
  micron::svector<micron::string> argv;      // extra args; argv[0] is path, added at spawn
  micron::svector<micron::string> env;       // KEY=VALUE entries
  bool env_inherit = true;
  micron::sstring<posix::path_max + 1> cwd;      // empty => inherit
  stdio_spec in, out, err;
  bool merge_stderr = false;      // 2>&1 onto stdout
  bool new_session = false;       // setsid
  posix::limits_t lims;           // read once at proc(); used iff has_lims
  bool has_lims = false;
  ucap_set_t caps;
  bool has_caps = false;
};

class child
{
  pid_t __pid = -1;
  bool __reaped = false;
  exit_status __status;
  int __in_fd = -1;       // parent writes child's stdin
  int __out_fd = -1;      // parent reads child's stdout
  int __err_fd = -1;      // parent reads child's stderr

  friend struct spawn_fn;

  child(pid_t p, int in_fd, int out_fd, int err_fd) noexcept : __pid(p), __in_fd(in_fd), __out_fd(out_fd), __err_fd(err_fd) { }

  static exit_status
  __decode(int wstatus) noexcept
  {
    exit_status e;
    if ( wifexited(wstatus) ) {
      e.exited = true;
      e.code = wexitstatus(wstatus);
    } else if ( wifsignaled(wstatus) ) {
      e.signaled = true;
      e.term_signal = wtermsig(wstatus);
      e.code = -e.term_signal;
    }
    return e;
  }

  static void
  __write_all(int fd, const byte *src, usize len) noexcept
  {
    usize done = 0;
    while ( done < len ) {
      max_t n = micron::write(fd, src + done, len - done);
      if ( n < 0 ) {
        if ( n == -error::interrupted ) continue;      // retry EINTR rather than truncating
        break;
      }
      if ( n == 0 ) break;
      done += static_cast<usize>(n);
    }
  }

  static usize
  __drain(int &fd, micron::string &out) noexcept
  {
    if ( fd < 0 ) return 0;
    usize total = 0;
    byte tmp[4096];
    for ( ;; ) {
      max_t n = micron::read(fd, tmp, sizeof(tmp));
      if ( n < 0 ) {
        if ( n == -error::interrupted ) continue;      // retry EINTR
        break;
      }
      if ( n == 0 ) break;      // EOF
      out.append(reinterpret_cast<const typename micron::string::value_type *>(tmp), static_cast<usize>(n));
      total += static_cast<usize>(n);
    }
    micron::close(fd);
    fd = -1;
    return total;
  }

  void
  __close_fd(int &fd) noexcept
  {
    if ( fd >= 0 ) {
      micron::close(fd);
      fd = -1;
    }
  }

public:
  ~child()
  {
    __close_fd(__in_fd);
    __close_fd(__out_fd);
    __close_fd(__err_fd);
  }

  child(const child &) = delete;
  child &operator=(const child &) = delete;

  child(child &&o) noexcept
      : __pid(o.__pid), __reaped(o.__reaped), __status(o.__status), __in_fd(o.__in_fd), __out_fd(o.__out_fd), __err_fd(o.__err_fd)
  {
    o.__pid = -1;
    o.__reaped = true;
    o.__in_fd = -1;
    o.__out_fd = -1;
    o.__err_fd = -1;
  }

  child &
  operator=(child &&o) noexcept
  {
    if ( this == &o ) return *this;
    __close_fd(__in_fd);
    __close_fd(__out_fd);
    __close_fd(__err_fd);
    __pid = o.__pid;
    __reaped = o.__reaped;
    __status = o.__status;
    __in_fd = o.__in_fd;
    __out_fd = o.__out_fd;
    __err_fd = o.__err_fd;
    o.__pid = -1;
    o.__reaped = true;
    o.__in_fd = -1;
    o.__out_fd = -1;
    o.__err_fd = -1;
    return *this;
  }

  [[nodiscard]] pid_t
  pid() const noexcept
  {
    return __pid;
  }

  [[nodiscard]] bool
  valid() const noexcept
  {
    return __pid > 0;
  }

  exit_status
  wait()
  {
    if ( __reaped ) return __status;
    int wstatus = 0;
    micron::wait4(__pid, &wstatus, 0, nullptr);
    __status = __decode(wstatus);
    __reaped = true;
    return __status;
  }

  bool
  try_wait(exit_status &out)
  {
    if ( __reaped ) {
      out = __status;
      return true;
    }
    int wstatus = 0;
    pid_t r = micron::wait4(__pid, &wstatus, wnohang, nullptr);
    if ( r == __pid ) {
      __status = __decode(wstatus);
      __reaped = true;
      out = __status;
      return true;
    }
    return false;
  }

  child_result_t
  wait_and_collect()
  {
    child_result_t r = micron::wait_and_collect(__pid);
    __reaped = true;
    __status = exit_status{};
    if ( r.exited_normally ) {
      __status.exited = true;
      __status.code = r.exit_code;
    } else if ( r.signaled ) {
      __status.signaled = true;
      __status.term_signal = r.term_signal;
      __status.code = -r.term_signal;
    }
    return r;
  }

  // controls

  int
  signal(micron::signal sig) noexcept
  {
    return posix::kill(__pid, static_cast<int>(sig));
  }

  int
  terminate() noexcept
  {
    return signal(signal::terminate);
  }

  int
  kill() noexcept
  {
    return signal(signal::kill9);
  }

  int
  cont() noexcept
  {
    return signal(signal::cont);
  }

  int
  stop() noexcept
  {
    return signal(signal::stop);
  }

  // io
  [[nodiscard]] int
  stdin_fd() const noexcept
  {
    return __in_fd;
  }

  [[nodiscard]] int
  stdout_fd() const noexcept
  {
    return __out_fd;
  }

  [[nodiscard]] int
  stderr_fd() const noexcept
  {
    return __err_fd;
  }

  void
  close_stdin() noexcept
  {
    __close_fd(__in_fd);
  }

  void
  write_stdin(const micron::string &in) noexcept
  {
    if ( __in_fd < 0 ) return;
    __write_all(__in_fd, reinterpret_cast<const byte *>(in.c_str()), in.size() * sizeof(typename micron::string::value_type));
  }

  usize
  read_stdout(micron::string &out) noexcept
  {
    return __drain(__out_fd, out);
  }

  usize
  read_stderr(micron::string &out) noexcept
  {
    return __drain(__err_fd, out);
  }

  comm_result
  communicate(const micron::string &input = {})
  {
    comm_result r;

    // WARNING: writing all of input up front (and only then reading) deadlocks when both the input and the child's output exceed the pipe
    // buffer
    const byte *in_data = reinterpret_cast<const byte *>(input.c_str());
    const usize in_len = input.size() * sizeof(typename micron::string::value_type);
    usize in_off = 0;
    bool in_open = (__in_fd >= 0);
    if ( in_open && in_len == 0 ) {
      __close_fd(__in_fd);      // nothing to send -> immediate EOF for the child's stdin
      in_open = false;
    } else if ( in_open ) {
      int fl = static_cast<int>(posix::fcntl(__in_fd, posix::f_getfl, 0));
      if ( fl >= 0 ) posix::fcntl(__in_fd, posix::f_setfl, fl | posix::o_nonblock);
    }

    bool out_open = __out_fd >= 0;
    bool err_open = __err_fd >= 0;
    byte tmp[4096];

    while ( out_open || err_open || in_open ) {
      micron::pollfd pfds[3];
      int n = 0;
      int oidx = -1, eidx = -1, iidx = -1;
      if ( out_open ) {
        pfds[n] = micron::make_poll(__out_fd, posix::poll_in);
        oidx = n++;
      }
      if ( err_open ) {
        pfds[n] = micron::make_poll(__err_fd, posix::poll_in);
        eidx = n++;
      }
      if ( in_open ) {
        pfds[n] = micron::make_poll(__in_fd, posix::poll_out);
        iidx = n++;
      }

      int pr = micron::poll_for(pfds, static_cast<micron::nfds_t>(n), -1);
      if ( pr < 0 ) {
        if ( pr == -error::interrupted ) continue;      // interrupted -> re-poll
        break;
      }

      if ( oidx >= 0 && pfds[oidx].revents != 0 ) {
        max_t k = micron::read(__out_fd, tmp, sizeof(tmp));
        if ( k > 0 )
          r.out.append(reinterpret_cast<const typename micron::string::value_type *>(tmp), static_cast<usize>(k));
        else if ( k != -error::interrupted && k != -error::try_again ) {
          __close_fd(__out_fd);
          out_open = false;
        }
      }
      if ( eidx >= 0 && pfds[eidx].revents != 0 ) {
        max_t k = micron::read(__err_fd, tmp, sizeof(tmp));
        if ( k > 0 )
          r.err.append(reinterpret_cast<const typename micron::string::value_type *>(tmp), static_cast<usize>(k));
        else if ( k != -error::interrupted && k != -error::try_again ) {
          __close_fd(__err_fd);
          err_open = false;
        }
      }
      if ( iidx >= 0 && pfds[iidx].revents != 0 ) {
        max_t k = micron::write(__in_fd, in_data + in_off, in_len - in_off);
        if ( k > 0 ) {
          in_off += static_cast<usize>(k);
          if ( in_off >= in_len ) {
            __close_fd(__in_fd);      // all input sent -> EOF for the child's stdin
            in_open = false;
          }
        } else if ( k != -EINTR && k != -EAGAIN ) {
          __close_fd(__in_fd);      // EPIPE etc: child closed its stdin
          in_open = false;
        }
      }
    }

    r.status = wait();
    return r;
  }
};

// seed
inline child_spec
proc(const char *path)
{
  child_spec s;
  s.path = path;
  return s;
}

template<is_string T>
inline child_spec
proc(const T &path)
{
  child_spec s;
  s.path = path.c_str();
  return s;
}

inline auto
arg(micron::string a)
{
  return [a = micron::move(a)](child_spec s) mutable -> child_spec {
    s.argv.push_back(micron::move(a));
    return s;
  };
}

template<typename... A>
inline auto
args(A... a)
{
  return [=](child_spec s) -> child_spec {
    (s.argv.push_back(micron::string(a)), ...);
    return s;
  };
}

inline auto
env(micron::string k, micron::string v)
{
  return [k = micron::move(k), v = micron::move(v)](child_spec s) mutable -> child_spec {
    micron::string kv = k;
    kv += '=';
    kv += v;
    s.env.push_back(micron::move(kv));
    return s;
  };
}

inline auto
clear_env()
{
  return [](child_spec s) -> child_spec {
    s.env_inherit = false;
    return s;
  };
}

inline auto
cwd(micron::string p)
{
  return [p = micron::move(p)](child_spec s) mutable -> child_spec {
    s.cwd = p.c_str();
    return s;
  };
}

inline auto
pipe_stdin()
{
  return [](child_spec s) -> child_spec {
    s.in.kind = redirect::pipe;
    return s;
  };
}

inline auto
pipe_stdout()
{
  return [](child_spec s) -> child_spec {
    s.out.kind = redirect::pipe;
    return s;
  };
}

inline auto
pipe_stderr()
{
  return [](child_spec s) -> child_spec {
    s.err.kind = redirect::pipe;
    return s;
  };
}

inline auto
null_stdin()
{
  return [](child_spec s) -> child_spec {
    s.in.kind = redirect::null;
    return s;
  };
}

inline auto
null_stdout()
{
  return [](child_spec s) -> child_spec {
    s.out.kind = redirect::null;
    return s;
  };
}

inline auto
null_stderr()
{
  return [](child_spec s) -> child_spec {
    s.err.kind = redirect::null;
    return s;
  };
}

inline auto
stdin_from(int fd)
{
  return [fd](child_spec s) -> child_spec {
    s.in.kind = redirect::to_fd;
    s.in.fd = fd;
    return s;
  };
}

inline auto
stdout_to(int fd)
{
  return [fd](child_spec s) -> child_spec {
    s.out.kind = redirect::to_fd;
    s.out.fd = fd;
    return s;
  };
}

inline auto
stderr_to(int fd)
{
  return [fd](child_spec s) -> child_spec {
    s.err.kind = redirect::to_fd;
    s.err.fd = fd;
    return s;
  };
}

inline auto
merge_stderr()
{
  return [](child_spec s) -> child_spec {
    s.merge_stderr = true;
    return s;
  };
}

inline auto
new_session()
{
  return [](child_spec s) -> child_spec {
    s.new_session = true;
    return s;
  };
}

inline auto
with_limits(const posix::limits_t &l)
{
  return [l](child_spec s) -> child_spec {
    s.lims = l;
    s.has_lims = true;
    return s;
  };
}

inline auto
with_caps(const ucap_set_t &c)
{
  return [c](child_spec s) -> child_spec {
    s.caps = c;
    s.has_caps = true;
    return s;
  };
}

struct spawn_fn {

  static posix::spawn_action
  __dup2(int src, int dst) noexcept
  {
    posix::spawn_action a{};
    a.type = posix::SPAWN_ACTION_DUP2;
    a.fd = src;
    a.newfd = dst;
    return a;
  }

  static posix::spawn_action
  __open(const char *path, int oflag, int target) noexcept
  {
    posix::spawn_action a{};
    a.type = posix::SPAWN_ACTION_OPEN;
    a.fd = target;
    a.path = path;
    a.oflag = oflag;
    a.mode = 0;
    return a;
  }

  option<child, spawn_error>
  operator()(child_spec spec) const
  {
    micron::vector<char *> argv;
    argv.push_back(const_cast<char *>(spec.path.c_str()));
    for ( usize i = 0; i < spec.argv.size(); ++i ) argv.push_back(const_cast<char *>(spec.argv[i].c_str()));
    argv.push_back(nullptr);

    char *const *envp = environ;
    micron::vector<char *> envv;      // kept alive across the spawn() call
    if ( !(spec.env_inherit && spec.env.empty()) ) {
      if ( spec.env_inherit )
        for ( char **e = environ; *e; ++e ) envv.push_back(*e);
      for ( usize i = 0; i < spec.env.size(); ++i ) envv.push_back(const_cast<char *>(spec.env[i].c_str()));
      envv.push_back(nullptr);
      envp = &envv[0];
    }

    posix::spawn_action acts[8];
    int nact = 0;

    int in_pipe[2] = { -1, -1 };
    int out_pipe[2] = { -1, -1 };
    int err_pipe[2] = { -1, -1 };

    auto close_all = [&]() noexcept {
      if ( in_pipe[0] >= 0 ) micron::close(in_pipe[0]);
      if ( in_pipe[1] >= 0 ) micron::close(in_pipe[1]);
      if ( out_pipe[0] >= 0 ) micron::close(out_pipe[0]);
      if ( out_pipe[1] >= 0 ) micron::close(out_pipe[1]);
      if ( err_pipe[0] >= 0 ) micron::close(err_pipe[0]);
      if ( err_pipe[1] >= 0 ) micron::close(err_pipe[1]);
    };

    if ( spec.in.kind == redirect::pipe ) {
      int pr = micron::pipe2(in_pipe, posix::o_cloexec);
      if ( pr < 0 ) {
        return option<child, spawn_error>{ spawn_error{ micron::syscall_errno(pr), spawn_error::pipe } };
      }
      acts[nact++] = __dup2(in_pipe[0], 0);
    } else if ( spec.in.kind == redirect::null ) {
      acts[nact++] = __open("/dev/null", posix::o_rdonly, 0);
    } else if ( spec.in.kind == redirect::to_fd ) {
      acts[nact++] = __dup2(spec.in.fd, 0);
    }

    int out_dest = 1;
    if ( spec.out.kind == redirect::pipe ) {
      int pr = micron::pipe2(out_pipe, posix::o_cloexec);
      if ( pr < 0 ) {
        close_all();
        return option<child, spawn_error>{ spawn_error{ micron::syscall_errno(pr), spawn_error::pipe } };
      }
      acts[nact++] = __dup2(out_pipe[1], 1);
      out_dest = out_pipe[1];
    } else if ( spec.out.kind == redirect::null ) {
      acts[nact++] = __open("/dev/null", posix::o_wronly, 1);
    } else if ( spec.out.kind == redirect::to_fd ) {
      acts[nact++] = __dup2(spec.out.fd, 1);
      out_dest = spec.out.fd;
    }

    if ( spec.merge_stderr ) {
      acts[nact++] = __dup2(out_dest, 2);
    } else if ( spec.err.kind == redirect::pipe ) {
      int pr = micron::pipe2(err_pipe, posix::o_cloexec);
      if ( pr < 0 ) {
        close_all();
        return option<child, spawn_error>{ spawn_error{ micron::syscall_errno(pr), spawn_error::pipe } };
      }
      acts[nact++] = __dup2(err_pipe[1], 2);
    } else if ( spec.err.kind == redirect::null ) {
      acts[nact++] = __open("/dev/null", posix::o_wronly, 2);
    } else if ( spec.err.kind == redirect::to_fd ) {
      acts[nact++] = __dup2(spec.err.fd, 2);
    }

    posix::spawn_file_actions_t fa{};
    fa.allocated = nact;
    fa.used = nact;
    fa.__actions = acts;

    posix::spawnattr_t attr;
    posix::spawnattr_init(attr);
    bool use_attr = false;
    if ( spec.new_session ) {
      attr.__flags |= posix_spawn_setsid;
      use_attr = true;
    }

    const posix::limits_t *limp = spec.has_lims ? &spec.lims : nullptr;
    const ucap_set_t *capp = spec.has_caps ? &spec.caps : nullptr;
    const char *cwdp = spec.cwd.empty() ? nullptr : spec.cwd.c_str();

    pid_t pid = -1;
    int rc = micron::spawn(pid, spec.path.c_str(), &argv[0], envp, nact ? &fa : nullptr, use_attr ? &attr : nullptr, cwdp, limp, capp);

    if ( rc != 0 ) {
      close_all();
      return option<child, spawn_error>{ spawn_error{ rc, spawn_error::spawn } };
    }

    if ( in_pipe[0] >= 0 ) micron::close(in_pipe[0]);
    if ( out_pipe[1] >= 0 ) micron::close(out_pipe[1]);
    if ( err_pipe[1] >= 0 ) micron::close(err_pipe[1]);

    return option<child, spawn_error>{ child{ pid, in_pipe[1], out_pipe[0], err_pipe[0] } };
  }
};

inline constexpr spawn_fn launch{};

// option<child,E> -> option<exit_status,E>
inline auto
wait_for()
{
  return [](option<child, spawn_error> o) -> option<exit_status, spawn_error> {
    if ( o.is_first() ) return option<exit_status, spawn_error>{ o.template cast<child>().wait() };
    return option<exit_status, spawn_error>{ o.template cast<spawn_error>() };
  };
}

// option<child,E> -> option<comm_result,E>
inline auto
communicate_with(micron::string input = {})
{
  return [input = micron::move(input)](option<child, spawn_error> o) mutable -> option<comm_result, spawn_error> {
    if ( o.is_first() ) return option<comm_result, spawn_error>{ o.template cast<child>().communicate(input) };
    return option<comm_result, spawn_error>{ o.template cast<spawn_error>() };
  };
}

inline child
spawn_or_throw(child_spec spec)
{
  auto o = launch(micron::move(spec));
  if ( !o.is_first() ) exc<except::system_error>("micron::child failed to spawn");
  return micron::move(o.template cast<child>());
}

};      // namespace micron
