#pragma once

#include "config.hh"

#include "io/io.hpp"
#include "linux/process/exec.hpp"
#include "linux/sys/limits.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// user-mode qemu routing for cross targets

namespace recipes
{
namespace gnu
{

// qemu == nullptr means "runs natively here, exec it directly"
struct emu_t {
  const char *qemu = nullptr;
  const char *sysroot = nullptr;
};

constexpr const char *__qemu_arm = "/usr/bin/qemu-arm-static";
constexpr const char *__qemu_arm64 = "/usr/bin/qemu-aarch64-static";
constexpr const char *__sysroot_arm = "/usr/gcc-linaro/arm-none-linux-gnueabihf/libc";
constexpr const char *__sysroot_arm64 = "/usr/gcc-linaro-aarch64/aarch64-none-linux-gnu/libc";

inline emu_t
emulator_for(const config_t &conf)
{
  if ( conf.arch == __arch::arm ) return emu_t{ __qemu_arm, __sysroot_arm };
  if ( conf.arch == __arch::arm64 ) return emu_t{ __qemu_arm64, __sysroot_arm64 };
  return emu_t{};      // x86 / amd64 / i386: native
}

inline bool
needs_emulation(const config_t &conf)
{
  return emulator_for(conf).qemu != nullptr;
}

inline bool
target_runnable(const config_t &conf)
{
  if ( !mc::posix::exists(conf.compiler_path.c_str()) ) return false;
  const emu_t e = emulator_for(conf);
  if ( e.qemu == nullptr ) return true;
  return mc::posix::exists(e.qemu) and mc::posix::exists(e.sysroot);
}

inline string_type
__missing_for(const config_t &conf)
{
  if ( !mc::posix::exists(conf.compiler_path.c_str()) ) return conf.compiler_path;
  const emu_t e = emulator_for(conf);
  if ( e.qemu != nullptr and !mc::posix::exists(e.qemu) ) return string_type{ e.qemu };
  if ( e.qemu != nullptr and !mc::posix::exists(e.sysroot) ) return string_type{ e.sysroot };
  return string_type{};
}

// a crashing test shouldn't leave core files behind
// micron reserves 256 GiB of VA (abcmalloc) and
// qemu-user can dump the targets core into the cwd, so an unbounded core is worth avoiding
inline void
__suppress_core_dumps()
{
  static bool done = false;
  if ( done ) return;
  done = true;
  mc::posix::rlimit64_t rl{};
  rl.rlim_cur = 0;
  rl.rlim_max = 0;
  (void)mc::posix::prlimit_in(0, mc::posix::rlimit_core, rl);
}

template<bool Wait = mc::exec_wait>
inline mc::status_t
run_target(const config_t &conf)
{
  __suppress_core_dumps();
  const emu_t e = emulator_for(conf);
  if ( e.qemu == nullptr ) return mc::execute<Wait>(conf.target_out);

  string_type qemu{ e.qemu };
  string_type command{ e.qemu };
  command.append(string_type{ " -L " });
  command.append(string_type{ e.sysroot });
  command.append(string_type{ " " });
  command.append(conf.target_out);
  return mc::execute<Wait>(qemu, command);
}

// snowball codes
//   1   -> the success sentinel
//   6   -> sb::require() failure  (__abort -> sys_exit(6))
//   139 -> SIGSEGV (128+11), usually spawned-thread TLS / stack-canary corruption
//   134 -> SIGABRT (128+6), an uncaught throw reaching terminate
//   0   -> ran off the end without hitting the sentinel
inline constexpr int __snowball_pass = 1;

inline int
verdict_of(int wait_status)
{
  if ( mc::wifsignaled(wait_status) ) return 128 + mc::wtermsig(wait_status);
  return mc::wexitstatus(wait_status);
}

inline const char *
decode_snowball(int rc)
{
  switch ( rc ) {
  case 1:
    return "PASS";
  case 6:
    return "FAIL (require)";
  case 128 + 11:
    return "FAIL (SIGSEGV)";
  case 128 + 6:
    return "FAIL (SIGABRT)";
  case 128 + 4:
    return "FAIL (SIGILL)";
  case 128 + 8:
    return "FAIL (SIGFPE)";
  case 128 + 9:
    return "FAIL (SIGKILL, likely OOM)";
  case 0:
    return "FAIL (no success sentinel)";
  default:
    return "FAIL";
  }
}

};      // namespace gnu
};      // namespace recipes
