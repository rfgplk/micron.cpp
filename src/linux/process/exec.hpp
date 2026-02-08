#pragma once

#include "process.hpp"

#include "../sys/spawn.hpp"

namespace micron
{

void
memexecute(uelf_t &elf)
{
}

void
execute(uprocess_t &t)
{
  micron::svector<char *> argv;
  for ( size_t i = 0; i < t.argv.size(); i++ )
    argv.push_back(&t.argv[i][0]);
  argv.push_back(nullptr);
  t.uid = posix::getuid();
  t.gid = posix::getgid();
  if ( micron::spawn(t.pid, t.path.c_str(), &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
}

// fork and run process at path location specified by T
template <bool W = false, is_string T>
pid_t
execute(const T &t)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };
  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

// fork and run process at path location specified by T
template <bool W = false, is_string T, is_string... R>
pid_t
execute(const T &t, const R &...args)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( spawnattr_init(flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };

  (argv.insert(argv.begin() + 1, &args[0]), ...);

  if ( micron::spawn(pid, t.c_str(), &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = false>
pid_t
execute(const char *t)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( posix::spawnattr_init(flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };
  if ( micron::spawn(pid, t, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = false, typename... R>
pid_t
execute(const char *t, R *...args)
{
  pid_t pid;
  int status = 0;
  posix::spawnattr_t flags;
  if ( spawnattr_init(flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };

  (argv.insert(argv.begin() + 1, const_cast<char *>(args)), ...);

  if ( micron::spawn(pid, t, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = false>
pid_t
execute(const char *t, char **args)
{
  pid_t pid;
  int status = 0;

  if ( micron::spawn(pid, t, args, environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

};
