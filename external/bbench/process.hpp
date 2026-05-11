
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../src/bits/__exceptions.hpp"
#include "../../src/concepts.hpp"
#include "../../src/except.hpp"
#include "../../src/linux/io.hpp"
#include "../../src/linux/sys/exec.hpp"
#include "../../src/linux/sys/fcntl.hpp"
#include "../../src/memory/actions.hpp"
#include "../../src/proc.hpp"
#include "../../src/types.hpp"
#include "../../src/vector.hpp"

namespace bbench
{

template <typename T> using __impl_vector = micron::vector<T>;

using micron::is_string;

// fork and run process at path location specified by T
template <bool W = false, is_string T, is_string... R>
int
process(const T &t, const R &...args)
{
  micron::pid_t pid = 0;
  int status = 0;
  __impl_vector<char *> argv;
  argv.push_back(const_cast<char *>(t.c_str()));
  argv.push_back(nullptr);
  // insert each arg before the trailing nullptr (index == size - 1)
  (argv.insert(argv.size() - 1, const_cast<char *>(args.c_str())), ...);

  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    micron::exc<micron::except::runtime_error>("micron process failed to start spawn");
  }
  if constexpr ( W ) micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = false, typename... R>
int
process(const char *t, const R *...args)
{
  micron::pid_t pid = 0;
  int status = 0;
  __impl_vector<char *> argv;
  argv.push_back(const_cast<char *>(t));
  argv.push_back(nullptr);
  (argv.insert(argv.size() - 1, const_cast<char *>(args)), ...);

  if ( micron::spawn(pid, t, &argv[0], environ) ) {
    micron::exc<micron::except::runtime_error>("micron process failed to start spawn");
  }
  if constexpr ( W ) micron::waitpid(pid, &status, 0);
  return pid;
}

template <typename F>
inline int
process_attach(const char *path, F &&attach_fn)
{
  int sync_pipe[2];
  if ( micron::pipe2(sync_pipe, micron::posix::o_cloexec) < 0 )
    micron::exc<micron::except::runtime_error>("bbench process_attach: pipe2 failed");

  micron::pid_t pid = micron::fork();
  if ( pid < 0 ) {
    micron::close(sync_pipe[0]);
    micron::close(sync_pipe[1]);
    micron::exc<micron::except::runtime_error>("bbench process_attach: fork failed");
  }

  if ( pid == 0 ) {
    micron::close(sync_pipe[1]);
    char c = 0;
    micron::posix::read(sync_pipe[0], &c, 1);
    micron::close(sync_pipe[0]);

    char *argv[2] = { const_cast<char *>(path), nullptr };
    micron::posix::execve(path, argv, environ);
    micron::posix::exit(127);
  }

  micron::close(sync_pipe[0]);
  attach_fn(static_cast<int>(pid));
  char go = 'g';
  micron::posix::write(sync_pipe[1], &go, 1);
  micron::close(sync_pipe[1]);
  return static_cast<int>(pid);
}

};     // namespace bbench
