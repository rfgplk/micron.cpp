//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/kernel.hpp"
#include "../src/linux/io/inode.hpp"
#include "../src/linux/io/sys.hpp"
#include "../src/linux/sys/clone.hpp"
#include "../src/linux/sys/poll.hpp"
#include "../src/linux/sys/system.hpp"
#include "../src/memory/mman.hpp"

namespace kn = micron::kernel;
namespace ki = micron::kernel::__impl;
namespace px = micron::posix;
static int FAILS = 0;

static int
__spawn_thunk(void *arg)
{
  micron::atom::store(reinterpret_cast<int *>(arg), 1, micron::atomic_release);
  return 0;
}

static_assert(ki::__parse_release("6.8.0-rc2-generic") == kn::ver(6, 8, 0));
static_assert(ki::__parse_release("5.15.148-tegra") == kn::ver(5, 15, 148));
static_assert(ki::__parse_release("6.12.9-201.fc41.x86_64") == kn::ver(6, 12, 9));
static_assert(ki::__parse_release("7.1.3-201.fc44.x86_64") == kn::ver(7, 1, 3));
static_assert(ki::__parse_release("5.15") == kn::ver(5, 15, 0));
static_assert(ki::__parse_release("6") == kn::ver(6, 0, 0));
static_assert(ki::__parse_release("5.15.0-rc3+") == kn::ver(5, 15, 0));
static_assert(ki::__parse_release("10.0.0") == kn::ver(10, 0, 0));
static_assert(ki::__parse_release("4.19.322") == kn::ver(4, 19, 255));
static_assert(ki::__parse_release("") == 0);
static_assert(ki::__parse_release("linux") == 0);
static_assert(ki::__parse_release(".5.4") == 0);
static_assert(ki::__parse_release("0.1.2") == 0);

static_assert(kn::ver(5, 8) > kn::ver(5, 7, 255));
static_assert(kn::ver(6, 0) > kn::ver(5, 19, 255));
static_assert(kn::ver(5, 11) > kn::ver(5, 8));
static_assert(kn::baseline == kn::ver(5, 0));
static_assert(kn::ver(300, 300, 300) == kn::ver(255, 255, 255));

static_assert(ki::__bucket(ki::__parse_release("4.4.0")) == kn::baseline);
static_assert(ki::__bucket(ki::__parse_release("2.6.32-431.el6")) == kn::baseline);
static_assert(ki::__bucket(0) == kn::baseline);
static_assert(ki::__bucket(kn::ver(6, 1)) == kn::ver(6, 1));
#if !defined(MICRON_MIN_KERNEL_MAJOR)
static_assert(kn::floor_version == kn::baseline);
#endif

static_assert(kn::feature::io_uring == kn::ver(5, 1) && kn::feature::clone3 == kn::ver(5, 3));
static_assert(kn::feature::faccessat2 == kn::ver(5, 8) && kn::feature::epoll_pwait2 == kn::ver(5, 11));
static_assert(kn::feature::uring_defer_taskrun > kn::feature::uring_single_issuer);

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  sb::test_case("live detection: version() parses uname and caches");
  {
    const u32 v = kn::version();
    sb::check(v >= kn::baseline);
    sb::check(kn::version() == v);
    micron::posix::utsname_t u{};
    sb::require(micron::posix::uname(u) == 0);
    sb::check(ki::__bucket(ki::__parse_release(u.release)) == v);
    sb::check(kn::since(5, 0));
    sb::check(kn::since(kn::baseline));
  }
  sb::end_test_case();

  sb::test_case("forced 5.4: dispatch picks legacy buckets");
  {
    kn::__set_version_for_testing(kn::ver(5, 4));
    sb::check(kn::version() == kn::ver(5, 4));
    sb::check(kn::since(5, 3) && kn::since(5, 4) && !kn::since(5, 8));
    sb::check(kn::has(kn::feature::clone3));
    sb::check(!kn::has(kn::feature::faccessat2));
    sb::check(!kn::has(kn::feature::epoll_pwait2));
    sb::check(!kn::has(kn::feature::uring_op_read));
  }
  sb::end_test_case();

  sb::test_case("forced 6.1: modern gates open, newer stay shut");
  {
    kn::__set_version_for_testing(kn::ver(6, 1));
    sb::check(kn::since(5, 8) && kn::since(5, 19) && kn::since(6, 1));
    sb::check(kn::has(kn::feature::uring_defer_taskrun));
    sb::check(kn::has(kn::feature::uring_files_sparse));
    sb::check(!kn::has(kn::feature::uring_futex));
    sb::check(!kn::since(6, 1, 1));
    sb::check(kn::since(6, 0, 255));
  }
  sb::end_test_case();

  sb::test_case("probe_gate: version prior + one-way ENOSYS demotion");
  {
    kn::probe_gate g{};
    kn::__set_version_for_testing(kn::ver(6, 0));
    sb::check(g.open(kn::feature::epoll_pwait2));
    g.demote();
    sb::check(!g.open(kn::feature::epoll_pwait2));
    sb::check(!g.open(kn::feature::io_uring));
    g.restore();
    sb::check(g.open(kn::feature::epoll_pwait2));
    kn::__set_version_for_testing(kn::ver(5, 4));
    sb::check(!g.open(kn::feature::epoll_pwait2));
    sb::check(g.open(kn::feature::clone3));
  }
  sb::end_test_case();

  sb::test_case("reset: 0 sentinel re-detects the live kernel");
  {
    kn::__set_version_for_testing(0);
    const u32 v = kn::version();
    sb::check(v >= kn::baseline);
    micron::posix::utsname_t u{};
    sb::require(micron::posix::uname(u) == 0);
    sb::check(ki::__bucket(ki::__parse_release(u.release)) == v);
  }
  sb::end_test_case();

  sb::test_case("epoll_pwait2: live path and demoted ms-fallback both honor the timeout");
  {
    int ep = px::epoll_create();
    sb::require(ep >= 0);
    px::epoll_event ev[1];
    micron::timespec_t ts{ 0, 1000000 };
    sb::check(px::epoll_pwait2(ep, ev, 1, &ts, nullptr) == 0);
    px::__epoll_pwait2_gate.demote();
    sb::check(px::epoll_pwait2(ep, ev, 1, &ts, nullptr) == 0);
    sb::check(px::epoll_pwait2(ep, ev, 1, &ts, nullptr) != -ENOSYS);
    px::__epoll_pwait2_gate.restore();

    const micron::timespec_t bad[] = {
      micron::timespec_t{ -1, 0 },
      micron::timespec_t{ 0, 1500000000 },
      micron::timespec_t{ 0, -1 },
    };
    for ( const micron::timespec_t &b : bad ) {
      const int live = px::epoll_pwait2(ep, ev, 1, &b, nullptr);
      px::__epoll_pwait2_gate.demote();
      const int demoted = px::epoll_pwait2(ep, ev, 1, &b, nullptr);
      px::__epoll_pwait2_gate.restore();
      sb::check(live == -micron::error::invalid_arg);
      sb::check(demoted == live);
    }
    micron::syscall(SYS_close, ep);
  }
  sb::end_test_case();

  sb::test_case("faccessat family: flagless legacy path everywhere, honest -ENOSYS pre-5.8 with flags");
  {
    sb::check(px::access("/", px::x_ok) == 0);
    sb::check(px::faccessat(px::at_fdcwd, "/", px::r_ok, 0) == 0);
    sb::check(micron::is_readable("/"));
    sb::check(!micron::is_writable("/proc/version"));
    if ( kn::since(kn::feature::faccessat2) )
      sb::check(px::faccessat(px::at_fdcwd, "/", px::r_ok, px::at_eaccess) == 0);
    else
      sb::check(px::faccessat(px::at_fdcwd, "/", px::r_ok, px::at_eaccess) == -ENOSYS);
    kn::__set_version_for_testing(kn::ver(5, 4));
    sb::check(px::faccessat(px::at_fdcwd, "/", px::r_ok, px::at_eaccess) == -ENOSYS);
    sb::check(px::faccessat(px::at_fdcwd, "/", px::r_ok, 0) == 0);
    kn::__set_version_for_testing(0);
    (void)kn::version();
  }
  sb::end_test_case();

  sb::test_case("clone3_spawn: clone3 path and demoted legacy-clone path both run the child");
  {
    constexpr unsigned long __flags = px::clone_vm | px::clone_fs | px::clone_files | px::clone_sighand | px::clone_thread
                                      | px::clone_sysvsem | px::clone_child_cleartid;
    constexpr usize __stksz = 1u << 20;
    for ( int pass = 0; pass < 2; pass++ ) {
      if ( pass == 1 ) px::__clone3_gate.demote();
      void *stk
          = micron::mmap(nullptr, __stksz, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0);
      sb::require(reinterpret_cast<usize>(stk) < static_cast<usize>(-4095));
      int ctid = 0;
      int flag = 0;
      px::pid_t tid = px::clone3_spawn(&__spawn_thunk, &flag, stk, __stksz, 0, nullptr, &ctid, __flags);
      sb::require(tid > 0);
      usize spins = 0;
      while ( micron::atom::load(&ctid, micron::atomic_acquire) != 0 && spins++ < 20000000 ) micron::syscall(SYS_sched_yield);
      sb::check(micron::atom::load(&ctid, micron::atomic_acquire) == 0);
      sb::check(micron::atom::load(&flag, micron::atomic_acquire) == 1);
      micron::munmap(reinterpret_cast<addr_t *>(stk), __stksz);
    }
    px::__clone3_gate.restore();
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL KERNEL DISPATCH TESTS PASSED ===");
  return 1;
}
