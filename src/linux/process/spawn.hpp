#pragma once

#include "../../memory/mman.hpp"
#include "../io.hpp"
#include "../sys/spawn.hpp"

#include "fork.hpp"
#include "wait.hpp"

#include "../sys/signal.hpp"

namespace micron
{

int
__spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  int pipefd[2];
  if ( micron::pipe2(pipefd, o_cloexec) < 0 )
    return errno;
  micron::posix::spawn_ctx ctx = { path, argv, envp, nullptr, nullptr, pipefd[1] };
  pid = fork();
  if ( pid == 0 ) {
    micron::close(pipefd[0]);
    micron::posix::spawn_process(ctx);
  }

  micron::close(pipefd[1]);

  int err;
  ssize_t n = micron::read(pipefd[0], &err, sizeof(err));
  micron::close(pipefd[0]);

  if ( n == sizeof(err) ) {
    micron::wait4(pid, nullptr, 0, nullptr);
    return err;
  }

  return 0;
}
int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, argv, envp);
}

// WARNING: the following code is currently very unstable/unsafe. stack frames get completely messed up and we have to implement a proper trampoline for it to function properly. the stack/activation frame maintains that the stack pointer must remain stable, otherwise bad things occur.

namespace unsafe
{
// TODO: fix
int
__spawn(pid_t &pid, const char *__restrict path, const posix::spawn_file_actions_t *__restrict file_actions,
        const posix::spawnattr_t *__restrict attrp, char *const *argv, char *const *envp)
{
  int pipefd[2];
  if ( micron::pipe2(pipefd, o_cloexec) < 0 )
    return errno;

  constexpr size_t stack_size = 1 << 20;
  void *stack = reinterpret_cast<void *>(
      micron::mmap(nullptr, stack_size + 4096, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(stack) )
    return errno;

  micron::mprotect(stack, 4096, prot_none);
  constexpr u32 red_zone = 128;
  uintptr_t sp = reinterpret_cast<uintptr_t>(stack) + stack_size + 4096 - red_zone;
  sp &= ~uintptr_t(0xF);

  // micron::posix::spawn_ctx ctx{ path, argv, envp, file_actions, attrp, pipefd[1] };
  auto *ctx = reinterpret_cast<micron::posix::spawn_ctx *>(micron::mmap(
      nullptr, sizeof(micron::posix::spawn_ctx), prot_read | prot_write, map_private | map_anonymous, -1, 0));

  *ctx = { path, argv, envp, file_actions, attrp, pipefd[1] };
  pid_t child
      = static_cast<pid_t>(micron::posix::clone_kernel(sig_chld, reinterpret_cast<void *>(sp), nullptr, nullptr, 0));

  if ( child == 0 ) {
    micron::close(pipefd[0]);
    micron::posix::spawn_process(ctx);
  }

  micron::close(pipefd[1]);

  int err;
  ssize_t n = micron::read(pipefd[0], &err, sizeof(err));
  micron::close(pipefd[0]);

  if ( n == sizeof(err) ) {
    micron::wait4(child, nullptr, 0, nullptr);
    return err;
  }

  if ( pid )
    pid = child;

  return 0;
}

int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, nullptr, nullptr, argv, envp);
}

int
spawn(pid_t &pid, const char *__restrict path, const posix::spawnattr_t &attrp, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, nullptr, &attrp, argv, envp);
}

int
spawn(pid_t &pid, const char *__restrict path, const posix::spawn_file_actions_t &file_actions,
      const posix::spawnattr_t &attrp, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, &file_actions, &attrp, argv, envp);
}
};
// POSIX compliance, if we need it
};
