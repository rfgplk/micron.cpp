#pragma once

#include "../../memory/mman.hpp"
#include "../io.hpp"
#include "../sys/spawn.hpp"

#include "capabilities.hpp"
#include "fork.hpp"
#include "resource.hpp"
#include "wait.hpp"

#include "../sys/signal.hpp"

namespace micron
{
constexpr static const bool exec_wait = true;
constexpr static const bool exec_continue = false;
template <typename T>
concept is_limits_set = requires(const T &t) {
  t.lim[0];     // posix::rlimit_t
};

template <is_limits_set Lims>
inline void
child_apply_limits(const Lims &lims)
{
  for ( rlim_t i = 0; i < posix::rlimit_nlimits; ++i ) {
    posix::rlimit_t rl = lims.lim[i];     // local copy
    posix::setrlimit(static_cast<posix::limits>(i), rl);
  }
}

__attribute__((noreturn)) void
__inplace_spawn([[maybe_unused]] pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  // TODO: configure
  micron::posix::spawn_ctx ctx = { path, argv, envp, nullptr, nullptr, 0 };
  micron::posix::spawn_process(ctx);
}

__attribute__((noreturn)) void
inplace_spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  __inplace_spawn(pid, path, argv, envp);
}

int
__spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  int pipefd[2];
  if ( micron::pipe2(pipefd, posix::o_cloexec) < 0 )
    return errno;
  micron::posix::spawn_ctx ctx = { path, argv, envp, nullptr, nullptr, pipefd[1] };
  pid = micron::fork();
  if ( pid == 0 ) {
    micron::close(pipefd[0]);
    micron::posix::spawn_process(ctx);
  }

  micron::close(pipefd[1]);

  int err;
  max_t n = micron::read(pipefd[0], &err, sizeof(err));
  micron::close(pipefd[0]);

  if ( n == sizeof(err) ) {
    micron::wait4(pid, nullptr, 0, nullptr);
    return err;
  }

  return 0;
}

template <bool Lim = false, bool Cap = false, typename Lims = posix::limits_t, typename Caps = ucap_set_t>
inline int
__spawn_caps(pid_t &pid, const char *path, char *const *argv, char *const *envp, [[maybe_unused]] const Lims *lims = nullptr,
             [[maybe_unused]] const Caps *caps = nullptr)
{
  int pipefd[2];
  if ( micron::pipe2(pipefd, posix::o_cloexec) < 0 )
    return errno;

  pid = micron::fork();
  if ( pid == 0 ) {
    micron::close(pipefd[0]);

    if constexpr ( Lim ) {
      if ( lims )
        child_apply_limits(*lims);
    }

    if constexpr ( Cap ) {
      if ( caps )
        apply_caps_child(*caps);
    }

    posix::spawn_ctx ctx{ path, argv, envp, nullptr, nullptr, pipefd[1] };
    micron::posix::spawn_process(ctx);
    micron::posix::exit(6);
    __builtin_unreachable();
  }

  micron::close(pipefd[1]);
  int err = 0;
  max_t n = micron::read(pipefd[0], &err, sizeof(err));
  micron::close(pipefd[0]);
  if ( n == static_cast<max_t>(sizeof(err)) ) {
    micron::wait4(pid, nullptr, 0, nullptr);
    return err;
  }
  return 0;
}

// rudimentary spawns are here, additional overloads in process

int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp)
{
  return __spawn(pid, path, argv, envp);
}

template <is_limits_set Lims>
inline int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp, const Lims &lims)
{
  return __spawn_caps<true, false>(pid, path, argv, envp, &lims);
}

template <is_cap_set Caps>
inline int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp, const Caps &caps)
{
  return __spawn_caps<false, true, posix::limits_t, Caps>(pid, path, argv, envp, nullptr, &caps);
}

template <is_cap_set Caps, is_limits_set Lims>
inline int
spawn(pid_t &pid, const char *__restrict path, char *const *argv, char *const *envp, const Caps &caps, const Lims &lims)
{
  return __spawn_caps<true, true, Lims, Caps>(pid, path, argv, envp, &lims, &caps);
}

// WARNING: the following code is currently very unstable/unsafe. stack frames get completely messed up and we have to
// implement a proper trampoline for it to function properly. the stack/activation frame maintains that the stack pointer
// must remain stable, otherwise bad things occur.

namespace unsafe
{
// TODO: fix
int
__spawn(pid_t &pid, const char *__restrict path, const posix::spawn_file_actions_t *__restrict file_actions,
        const posix::spawnattr_t *__restrict attrp, char *const *argv, char *const *envp)
{
  int pipefd[2];
  if ( micron::pipe2(pipefd, posix::o_cloexec) < 0 )
    return errno;

  constexpr usize stack_size = 1 << 20;
  void *stack
      = reinterpret_cast<void *>(micron::mmap(nullptr, stack_size + 4096, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(stack) )
    return errno;

  micron::mprotect(stack, 4096, prot_none);
  constexpr u32 red_zone = 128;
  uintptr_t sp = reinterpret_cast<uintptr_t>(stack) + stack_size + 4096 - red_zone;
  sp &= ~uintptr_t(0xF);

  // micron::posix::spawn_ctx ctx{ path, argv, envp, file_actions, attrp, pipefd[1] };
  auto *ctx = reinterpret_cast<micron::posix::spawn_ctx *>(
      micron::mmap(nullptr, sizeof(micron::posix::spawn_ctx), prot_read | prot_write, map_private | map_anonymous, -1, 0));

  *ctx = { path, argv, envp, file_actions, attrp, pipefd[1] };
  pid_t child = static_cast<pid_t>(micron::posix::clone_kernel(posix::sig_chld, reinterpret_cast<void *>(sp), nullptr, nullptr, 0));

  if ( child == 0 ) {
    micron::close(pipefd[0]);
    micron::posix::spawn_process(ctx);
  }

  micron::close(pipefd[1]);

  int err;
  max_t n = micron::read(pipefd[0], &err, sizeof(err));
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
spawn(pid_t &pid, const char *__restrict path, const posix::spawn_file_actions_t &file_actions, const posix::spawnattr_t &attrp,
      char *const *argv, char *const *envp)
{
  return __spawn(pid, path, &file_actions, &attrp, argv, envp);
}
};     // namespace unsafe

// POSIX compliance, if we need it
};     // namespace micron
