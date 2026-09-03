//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::landlock against a live kernel.
//
// Enforcement is irreversible and inherited across fork/exec, so every case that calls
// restrict_self() does it in a CHILD and reports through the exit status. The parent stays
// unconfined and can keep testing.
//
// Fixtures live under /var/tmp, never /tmp: a landlock rule is attached to an inode, and the point
// of the suite is that reads inside a granted subtree succeed while reads outside it are EACCES.

#include "../../src/std.hpp"

#include "../../src/exit.hpp"
#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/linux/sys/fcntl.hpp"
#include "../../src/sec/landlock.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace ll = micron::sec::landlock;

namespace
{

constexpr i32 ok_code = 31;
constexpr i32 bad_code = 32;
constexpr i32 setup_failed = 33;

constexpr const char *root_dir = "/var/tmp/mc_sec_landlock";
constexpr const char *inside_dir = "/var/tmp/mc_sec_landlock/in";
constexpr const char *inside_file = "/var/tmp/mc_sec_landlock/in/allowed.txt";
constexpr const char *outside_dir = "/var/tmp/mc_sec_landlock/out";
constexpr const char *outside_file = "/var/tmp/mc_sec_landlock/out/denied.txt";

bool
write_fixture(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0644));
  if ( fd < 0 ) return false;
  const char payload[] = "micron";
  const bool ok = mc::posix::write(fd, payload, sizeof(payload) - 1) == static_cast<max_t>(sizeof(payload) - 1);
  (void)mc::posix::close(fd);
  return ok;
}

bool
build_fixtures(void)
{
  (void)mc::posix::mkdir(root_dir, 0755);
  (void)mc::posix::mkdir(inside_dir, 0755);
  (void)mc::posix::mkdir(outside_dir, 0755);
  return write_fixture(inside_file) && write_fixture(outside_file);
}

// can this process still read `path`?
bool
can_read(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

i32
open_errno(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd >= 0 ) {
    (void)mc::posix::close(fd);
    return 0;
  }
  return fd;
}

i32
in_child(void (*fn)(void))
{
  const int pid = mc::try_fork();
  if ( pid < 0 ) return -1;
  if ( pid == 0 ) {
    fn();
    mc::sys_group_exit(bad_code);
  }
  int status = 0;
  (void)mc::waitpid(pid, &status, 0);
  return status;
}

void
require_child_ok(void (*fn)(void))
{
  const i32 status = in_child(fn);
  sb::require_true(status >= 0);
  sb::require_true(mc::wifexited(status));
  sb::require(mc::wexitstatus(status), ok_code);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// children

void
child_confines_to_subtree(void)
{
  ll::ruleset r = ll::try_ruleset(ll::read_only | ll::access_fs::write_file);
  if ( !r.valid() ) mc::sys_group_exit(setup_failed);
  if ( r.allow(inside_dir, ll::read_only) < 0 ) mc::sys_group_exit(setup_failed);
  if ( r.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);

  // inside the granted subtree: still readable
  if ( !can_read(inside_file) ) mc::sys_group_exit(bad_code);
  // outside it: EACCES, and specifically EACCES rather than ENOENT
  if ( open_errno(outside_file) != -13 ) mc::sys_group_exit(bad_code);
  // a right the ruleset HANDLES but never granted is denied even inside the subtree
  const i32 wfd
      = static_cast<i32>(mc::posix::open(inside_file, mc::posix::o_wronly, 0));
  if ( wfd >= 0 ) {
    (void)mc::posix::close(wfd);
    mc::sys_group_exit(bad_code);
  }
  mc::sys_group_exit(ok_code);
}

void
child_unhandled_right_is_untouched(void)
{
  // handle ONLY read_file/read_dir. write_file is not handled, so landlock leaves writing entirely
  // to the rest of the system -- this is the distinction between "handled and not granted" (denied)
  // and "not handled" (not landlock's business)
  ll::ruleset r = ll::try_ruleset(ll::read_only);
  if ( !r.valid() ) mc::sys_group_exit(setup_failed);
  if ( r.allow(inside_dir, ll::read_only) < 0 ) mc::sys_group_exit(setup_failed);
  if ( r.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);

  if ( !can_read(inside_file) ) mc::sys_group_exit(bad_code);
  const i32 wfd = static_cast<i32>(mc::posix::open(outside_file, mc::posix::o_wronly, 0));
  if ( wfd < 0 ) mc::sys_group_exit(bad_code);      // write was never handled -> must still work
  (void)mc::posix::close(wfd);
  mc::sys_group_exit(ok_code);
}

void
child_domain_survives_fork(void)
{
  ll::ruleset r = ll::try_ruleset(ll::read_only);
  if ( !r.valid() ) mc::sys_group_exit(setup_failed);
  if ( r.allow(inside_dir, ll::read_only) < 0 ) mc::sys_group_exit(setup_failed);
  if ( r.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);

  const int gpid = mc::try_fork();
  if ( gpid < 0 ) mc::sys_group_exit(setup_failed);
  if ( gpid == 0 ) {
    // the grandchild inherited the domain and never opted in to it
    const bool in_ok = can_read(inside_file);
    const bool out_blocked = (open_errno(outside_file) == -13);
    mc::sys_group_exit((in_ok && out_blocked) ? ok_code : bad_code);
  }
  int st = 0;
  (void)mc::waitpid(gpid, &st, 0);
  mc::sys_group_exit((mc::wifexited(st) && mc::wexitstatus(st) == ok_code) ? ok_code : bad_code);
}

void
child_restrict_sets_nnp(void)
{
  ll::ruleset r = ll::try_ruleset(ll::read_only);
  if ( !r.valid() ) mc::sys_group_exit(setup_failed);
  if ( r.allow(root_dir, ll::read_only) < 0 ) mc::sys_group_exit(setup_failed);
  if ( mc::prctl(mc::PR_GET_NO_NEW_PRIVS) != 0 ) mc::sys_group_exit(bad_code);
  if ( r.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);
  // restrict_self() must have set NNP itself; without it the kernel would have said EPERM
  mc::sys_group_exit(mc::prctl(mc::PR_GET_NO_NEW_PRIVS) == 1 ? ok_code : bad_code);
}

void
child_second_ruleset_only_narrows(void)
{
  // stacking a domain can only ever remove access. the second ruleset grants the OUTSIDE dir, but
  // the first already forbade it, so the result stays forbidden
  ll::ruleset a = ll::try_ruleset(ll::read_only);
  if ( !a.valid() || a.allow(inside_dir, ll::read_only) < 0 || a.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);

  ll::ruleset b = ll::try_ruleset(ll::read_only);
  if ( !b.valid() || b.allow(root_dir, ll::read_only) < 0 || b.restrict_self() < 0 ) mc::sys_group_exit(setup_failed);

  const bool in_ok = can_read(inside_file);
  const bool out_blocked = (open_errno(outside_file) == -13);
  mc::sys_group_exit((in_ok && out_blocked) ? ok_code : bad_code);
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC LANDLOCK ===");

  sb::require_true(build_fixtures());

  // ---------------------------------------------------------------- //
  sb::test_case("the ABI probe answers, and the supported masks follow the level it reports");
  {
    const i32 abi = ll::abi_level();
    sb::print("  landlock ABI: ", static_cast<i64>(abi));
    sb::require_true(ll::available());
    sb::require_true(abi >= 1);

    // every supported mask must be a subset of what the header knows, and monotone in the ABI
    sb::require_true(ll::bits(ll::supported_fs()) == mc::posix::landlock_fs_mask_for(abi));
    sb::require_true((ll::bits(ll::supported_fs()) & ~0x1ffffuLL) == 0);
    sb::require(ll::bits(ll::supported_net()) != 0, abi >= 4);
    sb::require(ll::bits(ll::supported_scope()) != 0, abi >= 6);

    // the probe is cached; a second call must agree
    sb::require(ll::abi_level(), abi);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a ruleset narrows its handled mask to the running ABI instead of failing EINVAL");
  {
    // ask for every right the header knows, including ones a 5.13 kernel has never heard of
    const ll::access_fs everything = static_cast<ll::access_fs>(0x1ffffuLL);
    ll::ruleset r = ll::try_ruleset(everything);
    sb::require_true(r.valid());
    sb::require(ll::bits(r.handled()), mc::posix::landlock_fs_mask_for(ll::abi_level()));
    sb::require_true(ll::bits(r.handled()) <= ll::bits(everything));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a rule may not grant a right the ruleset does not handle");
  {
    ll::ruleset r = ll::try_ruleset(ll::access_fs::read_file);
    sb::require_true(r.valid());
    // execute is not handled here, so a rule granting only execute has nothing to say
    sb::require(r.allow(inside_dir, ll::access_fs::execute), -22);      // -EINVAL
    // and one granting read_file is accepted
    sb::require(r.allow(inside_dir, ll::access_fs::read_file), 0);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a bad path is reported, not thrown, and does not poison the ruleset");
  {
    ll::ruleset r = ll::try_ruleset(ll::read_only);
    sb::require_true(r.valid());
    sb::require_true(r.allow("/var/tmp/mc_sec_landlock/definitely_absent", ll::read_only) < 0);
    sb::require_true(r.valid());
    sb::require(r.allow(inside_dir, ll::read_only), 0);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("moving a ruleset transfers the fd and leaves the source closed");
  {
    ll::ruleset a = ll::try_ruleset(ll::read_only);
    sb::require_true(a.valid());
    const i32 fd = a.fd();
    ll::ruleset b = mc::move(a);
    sb::require_false(a.valid());
    sb::require_true(b.valid());
    sb::require(b.fd(), fd);
    sb::require(b.allow(inside_dir, ll::read_only), 0);      // the moved-to handle still works
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a restricted child reads inside its subtree and gets EACCES outside it");
  {
    require_child_ok(child_confines_to_subtree);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a right the ruleset never handled is left alone entirely");
  {
    require_child_ok(child_unhandled_right_is_untouched);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the domain is inherited by a child that never opted into it");
  {
    require_child_ok(child_domain_survives_fork);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("restrict_self sets NO_NEW_PRIVS itself, which is what makes it legal unprivileged");
  {
    require_child_ok(child_restrict_sets_nnp);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("stacking a second ruleset can only narrow, never widen");
  {
    require_child_ok(child_second_ruleset_only_narrows);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a failed ruleset reports the errno it was handed, not a sentinel collision");
  {
    // -EPERM is -1, which is what "no ruleset fd" used to be spelled with -- so a create refused by
    // an outer LSM or an existing filter came back to the caller as EBADF
    ll::ruleset denied = ll::ruleset::__failed(-static_cast<i32>(mc::error::permissions));
    sb::require_false(denied.valid());
    sb::require(denied.error(), -static_cast<i32>(mc::error::permissions));
    sb::require(denied.allow("/usr", ll::read_only), -static_cast<i32>(mc::error::permissions));
    sb::require(denied.restrict_self(), -static_cast<i32>(mc::error::permissions));

    // a non-negative argument is not an errno, and is reported as EINVAL rather than as an fd
    ll::ruleset bogus = ll::ruleset::__failed(0);
    sb::require_false(bogus.valid());
    sb::require(bogus.error(), -static_cast<i32>(mc::error::invalid_arg));

    // and abi_level() never answers 0 or -1: an absent landlock is ENOSYS, a real errno
    const i32 a = ll::abi_level();
    sb::require_true(a > 0 || a < -1);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the parent was never confined by any of the above");
  {
    sb::require_true(can_read(inside_file));
    sb::require_true(can_read(outside_file));
    sb::require(mc::prctl(mc::PR_GET_NO_NEW_PRIVS), 0);
  }
  sb::end_test_case();

  sb::print("=== SEC LANDLOCK PASSED ===");
  return 1;
}
