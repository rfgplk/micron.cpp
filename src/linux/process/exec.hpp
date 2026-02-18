#pragma once

#include "process.hpp"

#include "../sys/spawn.hpp"

namespace micron
{

constexpr static const bool exec_wait = true;
constexpr static const bool exec_continue = false;

/*void
memexecute(uelf_t &elf)
{
}*/

// replace and execute

__attribute__((noreturn)) void
rexecute(uprocess_t &t)
{
  micron::svector<char *> argv;
  for ( size_t i = 0; i < t.argv.size(); i++ )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  micron::inplace_spawn(t.pids.pid, t.path.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

// run process at path location specified by T
template <is_string T>
__attribute__((noreturn)) void
rexecute(const T &t)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };
  micron::inplace_spawn(pid, t.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

// fork and run process at path location specified by T
template <is_string T, is_string... R>
__attribute__((noreturn)) void
rexecute(const T &t, const R &...args)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };

  (argv.insert(argv.begin() + 1, &args[0]), ...);

  micron::inplace_spawn(pid, t.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

// for passing a preprocessed string to be split up
template <is_string T, is_string A>
__attribute__((noreturn)) void
rexecute(const T &t, A &__argv)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv;
  format::replace_all<0x0>(__argv, " ");
  auto itr = __argv.cbegin();
  auto itr_f = __argv.cbegin();
  while ( itr < __argv.end() ) {
    itr_f = mc::format::find(__argv, itr, char(0x0));
    if ( itr_f == nullptr ) {
      argv.emplace_back(const_cast<char *>(itr));
      break;
    }
    argv.emplace_back(const_cast<char *>(itr));
    itr = ++itr_f;
  }
  micron::inplace_spawn(pid, t.c_str(), &argv[0], environ);
  __builtin_unreachable();
}

__attribute__((noreturn)) void
rexecute(const char *t)
{
  pid_t pid;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };
  micron::inplace_spawn(pid, t, &argv[0], environ);
  __builtin_unreachable();
}

template <typename... R>
__attribute__((noreturn)) void
rexecute(const char *t, R *...args)
{
  pid_t pid;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };

  (argv.insert(argv.begin() + 1, const_cast<char *>(args)), ...);

  micron::inplace_spawn(pid, t, &argv[0], environ);
  __builtin_unreachable();
}

__attribute__((noreturn)) void
rexecute(const char *t, char **args)
{
  pid_t pid;

  micron::inplace_spawn(pid, t, args, environ);
  __builtin_unreachable();
}

// fork and execute

void
execute(uprocess_t &t)
{
  micron::svector<char *> argv;
  for ( size_t i = 0; i < t.argv.size(); i++ )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.pids.uid = posix::getuid();
  t.pids.gid = posix::getgid();
  if ( micron::spawn(t.pids.pid, t.path.c_str(), &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
}

// fork and run process at path location specified by T
template <bool W = exec_continue, is_string T>
pid_t
execute(const T &t)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };
  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

// fork and run process at path location specified by T
template <bool W = exec_continue, is_string T, is_string... R>
pid_t
execute(const T &t, const R &...args)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };

  (argv.insert(argv.begin() + 1, &args[0]), ...);

  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

// for passing a preprocessed string to be split up
template <bool W = exec_continue, is_string T, is_string A>
pid_t
execute(const T &t, A &__argv)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv;
  format::replace_all<0x0>(__argv, " ");
  auto itr = __argv.cbegin();
  auto itr_f = __argv.cbegin();
  while ( itr < __argv.end() ) {
    itr_f = mc::format::find(__argv, itr, char(0x0));
    if ( itr_f == nullptr ) {
      argv.emplace_back(const_cast<char *>(itr));
      break;
    }
    argv.emplace_back(const_cast<char *>(itr));
    itr = ++itr_f;
  }
  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  __argv.clear();
  return pid;
}

template <bool W = exec_continue>
pid_t
execute(const char *t)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };
  if ( micron::spawn(pid, t, &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = exec_continue, typename... R>
pid_t
execute(const char *t, R *...args)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    exc<except::system_error>("micron process failed to init posix::spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };

  (argv.insert(argv.begin() + 1, const_cast<char *>(args)), ...);

  if ( micron::spawn(pid, t, &argv[0], environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = exec_continue>
pid_t
execute(const char *t, char **args)
{
  pid_t pid;
  int status = 0;

  if ( micron::spawn(pid, t, args, environ) ) {
    exc<except::system_error>("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

};
