//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/std.hpp"

#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;

namespace
{

constexpr const char *probe_file = "/var/tmp/mc_sec_fds_probe.txt";

constexpr i32 fd_low = 7;
constexpr i32 fd_high = 2048;
constexpr i32 fd_high_kept = 3000;

constexpr i32 bit_low_closed = 1 << 0;
constexpr i32 bit_high_closed = 1 << 1;
constexpr i32 bit_kept_open = 1 << 2;
constexpr i32 bit_stdio_open = 1 << 3;

bool
is_open(i32 fd)
{
  mc::posix::stat_t st{};
  return mc::posix::fstat(mc::posix::fd_t{ fd }, st) == 0;
}

i32
open_probe(void)
{
  return static_cast<i32>(mc::posix::open(probe_file, mc::posix::o_rdonly, 0));
}

bool
touch(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0644));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "probe", 5);
  (void)mc::posix::close(fd);
  return true;
}

i32
fd_probe_body(void)
{
  i32 bits = 0;
  if ( !is_open(fd_low) ) bits |= bit_low_closed;
  if ( !is_open(fd_high) ) bits |= bit_high_closed;
  if ( is_open(fd_high_kept) ) bits |= bit_kept_open;
  if ( is_open(0) && is_open(1) && is_open(2) ) bits |= bit_stdio_open;
  return bits;
}

i32
inherit_probe_body(void)
{
  i32 bits = 0;
  if ( is_open(fd_low) ) bits |= bit_low_closed;
  if ( is_open(fd_high) ) bits |= bit_high_closed;
  return bits;
}

i32
wait_status_of(s::sandbox::child &c)
{
  int status = 0;
  (void)mc::waitpid(c.pid, &status, 0);
  return status;
}

u64
raise_nofile(void)
{
  mc::posix::rlimit64_t rl{};
  if ( mc::posix::get_process_limits(0, mc::posix::rlimit_nofile, rl) < 0 ) return 0;
  if ( rl.rlim_max <= 4096 ) {
    rl.rlim_cur = rl.rlim_max;
  } else {
    rl.rlim_cur = 4096;
  }
  mc::posix::rlimit64_t want = rl;
  if ( mc::posix::set_process_limits(0, mc::posix::rlimit_nofile, want) < 0 ) return 0;
  mc::posix::rlimit64_t now{};
  if ( mc::posix::get_process_limits(0, mc::posix::rlimit_nofile, now) < 0 ) return 0;
  return now.rlim_cur;
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC DESCRIPTORS ===");

  sb::require_true(touch(probe_file));

  const u64 nofile = raise_nofile();
  sb::print("  RLIMIT_NOFILE soft: ", static_cast<i64>(nofile));

  if ( nofile <= static_cast<u64>(fd_high_kept) ) {

    sb::print("  this box will not permit a descriptor past ", static_cast<i64>(fd_high_kept), "; skipping");
    sb::print("=== SEC DESCRIPTORS PASSED (skipped) ===");
    return 1;
  }

  const i32 base = open_probe();
  sb::require_true(base >= 0);
  sb::require(mc::posix::dup2(base, fd_low), fd_low);
  sb::require(mc::posix::dup2(base, fd_high), fd_high);
  sb::require(mc::posix::dup2(base, fd_high_kept), fd_high_kept);
  sb::require_true(is_open(fd_low) && is_open(fd_high) && is_open(fd_high_kept));

  sb::test_case("close_extra_fds() closes descriptors ABOVE the old 1024 bound");
  {
    s::sandbox box;
    box.close_extra_fds().keep_fd(fd_high_kept);
    sb::require_true(box.configured());

    s::sandbox::child c = box.run(fd_probe_body);
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());

    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));

    const i32 want = bit_low_closed | bit_high_closed | bit_kept_open | bit_stdio_open;
    const i32 got = mc::wexitstatus(st);
    if ( got != want )
      sb::print("  fd bits got=", static_cast<i64>(got), " want=", static_cast<i64>(want), "  LEAKED/LOST=", static_cast<i64>(want ^ got));
    sb::require(got, want);
  }
  sb::end_test_case();

  sb::test_case("a keep_fd list with gaps on both sides survives the sweep intact");
  {

    const i32 k1 = 11, k2 = 12, k3 = 1500;
    sb::require(mc::posix::dup2(base, k1), k1);
    sb::require(mc::posix::dup2(base, k2), k2);
    sb::require(mc::posix::dup2(base, k3), k3);

    s::sandbox box;
    box.close_extra_fds().keep_fd(k3).keep_fd(k1).keep_fd(k2);
    s::sandbox::child c = box.run([]() -> i32 {
      i32 b = 0;
      if ( is_open(11) && is_open(12) && is_open(1500) ) b |= 1;
      if ( !is_open(10) && !is_open(13) && !is_open(1499) && !is_open(1501) ) b |= 2;
      if ( !is_open(fd_high) ) b |= 4;
      return b;
    });
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 7);

    for ( i32 fd : { k1, k2, k3 } ) (void)mc::posix::close(fd);
  }
  sb::end_test_case();

  sb::test_case("the sweep is ON by default, and close_extra_fds(false) is what restores inheritance");
  {

    s::sandbox on;
    s::sandbox::child a = on.run(inherit_probe_body);
    sb::require_true(a.ok());
    sb::require(mc::wexitstatus(wait_status_of(a)), 0);

    s::sandbox off;
    off.close_extra_fds(false);
    s::sandbox::child b = off.run(inherit_probe_body);
    sb::require_true(b.ok());
    sb::require(mc::wexitstatus(wait_status_of(b)), bit_low_closed | bit_high_closed);
  }
  sb::end_test_case();

  sb::test_case("a leaked DIRECTORY descriptor is what makes this a confinement bug, and it is gone");
  {

    const i32 dirfd = static_cast<i32>(mc::posix::open("/", mc::posix::o_rdonly | mc::posix::o_directory, 0));
    sb::require_true(dirfd >= 0);
    sb::require(mc::posix::dup2(dirfd, fd_high), fd_high);
    (void)mc::posix::close(dirfd);
    sb::require_true(is_open(fd_high));

    s::sandbox box;
    box.close_extra_fds();
    s::sandbox::child c = box.run([]() -> i32 {
      if ( is_open(fd_high) ) return 91;
      if ( mc::posix::fchdir(mc::posix::fd_t{ fd_high }) == 0 ) return 92;
      return 93;
    });
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 93);
  }
  sb::end_test_case();

  sb::test_case("the runner still holds everything it opened");
  {
    sb::require_true(is_open(0) && is_open(1) && is_open(2));
    sb::require_true(is_open(base));
    sb::require_true(is_open(fd_high));
    sb::require_true(is_open(fd_high_kept));
    for ( i32 fd : { base, fd_low, fd_high, fd_high_kept } ) (void)mc::posix::close(fd);
    (void)mc::posix::unlink(probe_file);
  }
  sb::end_test_case();

  sb::print("=== SEC DESCRIPTORS PASSED ===");
  return 1;
}
