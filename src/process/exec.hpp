#pragma once

#include "process.hpp"

namespace micron
{

void 
memexecute(uelf_t& elf)
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
  if ( ::posix_spawn(&t.pid, t.path.c_str(), NULL, &t.flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  // if ( t.wait )
  //   ::waitpid(t.pid, &t.status, 0);
}

// fork and run process at path location specified by T
template <bool W = false, is_string T>
pid_t
execute(const T &t)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };
  if ( ::posix_spawn(&pid, t.c_str(), NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

// fork and run process at path location specified by T
template <bool W = false, is_string T>
pid_t
execute(const T &t)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };
  if ( ::posix_spawn(&pid, t.c_str(), NULL, &flags, &argv[0], environ) ) {
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
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { &t[0], nullptr };

  (argv.insert(argv.begin() + 1, &args[0]), ...);

  if ( ::posix_spawn(&pid, t.c_str(), NULL, &flags, &argv[0], environ) ) {
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
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };
  if ( ::posix_spawn(&pid, t, NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

template <bool W = false, typename... R>
pid_t
execute(const char *t, const R *...args)
{
  pid_t pid;
  int status = 0;
  posix_spawnattr_t flags;
  if ( posix_spawnattr_init(&flags) != 0 ) {
    throw except::system_error("micron process failed to init spawnattrs");
  }
  micron::vector<char *> argv = { const_cast<char *>(t), nullptr };

  (argv.insert(argv.begin() + 1, const_cast<char *>(args)), ...);

  if ( ::posix_spawn(&pid, t, NULL, &flags, &argv[0], environ) ) {
    throw except::system_error("micron process failed to start posix_spawn");
  }
  if constexpr ( W )
    micron::waitpid(pid, &status, 0);
  return pid;
}

};
