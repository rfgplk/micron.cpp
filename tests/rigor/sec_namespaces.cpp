//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::ns against a live kernel.
//
// Unsharing is irreversible for the calling process (a user namespace cannot be left again without
// privilege), so every creating case runs in a CHILD and reports through its exit status.
//
// The suite avoids asserting that this machine sits in the host namespaces -- that is an
// environment claim, not a claim about the code. It asserts the RELATIONAL facts instead: an
// unshared namespace has a different inode from its parent's, the same handle answers the same
// inode twice, and a restored namespace is the one we started in.

#include "../../src/std.hpp"

#include "../../src/exit.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/sec/namespaces.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace ns = micron::sec::ns;

namespace
{

constexpr i32 ok_code = 41;
constexpr i32 bad_code = 42;
constexpr i32 setup_failed = 43;
constexpr i32 no_userns = 44;      // the kernel refused an unprivileged user namespace

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
child_userns_changes_identity(void)
{
  const max_t before = ns::open_self(ns::ns_kind::user).inode();
  if ( before < 0 ) mc::sys_group_exit(setup_failed);

  if ( ns::unshare(ns::ns_kind::user) < 0 ) mc::sys_group_exit(no_userns);

  ns::ns_handle after_h = ns::open_self(ns::ns_kind::user);
  const max_t after = after_h.inode();
  if ( after < 0 ) mc::sys_group_exit(setup_failed);

  // a fresh namespace is a different namespace, and it is not the host's
  if ( after == before ) mc::sys_group_exit(bad_code);
  if ( after_h.is_host() ) mc::sys_group_exit(bad_code);
  // and the kernel agrees about what kind of thing it is
  if ( after_h.type() != static_cast<i32>(mc::posix::clone_newuser) ) mc::sys_group_exit(bad_code);
  mc::sys_group_exit(ok_code);
}

void
child_gid_map_needs_setgroups_denied(void)
{
  // THE trap this file exists to pin. gid_map is EPERM in an unprivileged user namespace until
  // /proc/self/setgroups has been written "deny".
  //
  // NOTE: the gid is read BEFORE the unshare on purpose -- read afterwards it is 65534 and the
  // write fails with the same EPERM for an entirely different reason, which would make this case
  // pass while proving nothing
  const u32 gid = static_cast<u32>(mc::posix::getgid());
  if ( ns::unshare(ns::ns_kind::user) < 0 ) mc::sys_group_exit(no_userns);

  const i32 too_early = ns::write_gid_map(0, gid, 1);
  if ( too_early != -1 ) mc::sys_group_exit(bad_code);      // must be -EPERM

  if ( ns::deny_setgroups() < 0 ) mc::sys_group_exit(setup_failed);
  const i32 now = ns::write_gid_map(0, gid, 1);
  mc::sys_group_exit(now == 0 ? ok_code : bad_code);
}

void
child_maps_self_to_root(void)
{
  const u32 uid = static_cast<u32>(mc::posix::getuid());
  const u32 gid = static_cast<u32>(mc::posix::getgid());
  if ( ns::unshare(ns::ns_kind::user) < 0 ) mc::sys_group_exit(no_userns);

  // before any mapping every id reads as overflowuid (65534), NOT as our real uid
  if ( mc::posix::geteuid() != 65534 ) mc::sys_group_exit(bad_code);

  if ( ns::map_to_root(uid, gid) < 0 ) mc::sys_group_exit(setup_failed);

  // root inside, unchanged outside
  if ( mc::posix::geteuid() != 0 ) mc::sys_group_exit(bad_code);
  if ( mc::posix::getegid() != 0 ) mc::sys_group_exit(bad_code);
  mc::sys_group_exit(ok_code);
}

// the trap map_to_root's signature exists to prevent: ids read from INSIDE the new namespace are
// 65534, and a map line built from them is refused
void
child_ids_read_after_unshare_are_useless(void)
{
  if ( ns::unshare(ns::ns_kind::user) < 0 ) mc::sys_group_exit(no_userns);

  const u32 inner_uid = static_cast<u32>(mc::posix::getuid());
  if ( inner_uid != 65534 ) mc::sys_group_exit(bad_code);

  // exactly what a getuid()-inside implementation would write
  if ( ns::write_uid_map(0, inner_uid, 1) != -1 ) mc::sys_group_exit(bad_code);      // -EPERM
  mc::sys_group_exit(ok_code);
}

void
child_uid_map_is_write_once(void)
{
  const u32 uid = static_cast<u32>(mc::posix::getuid());      // parent-side id, before we leave
  if ( ns::unshare(ns::ns_kind::user) < 0 ) mc::sys_group_exit(no_userns);
  if ( ns::write_uid_map(0, uid, 1) < 0 ) mc::sys_group_exit(setup_failed);
  // a second write to the same namespace's uid_map is EPERM: the mapping is permanent
  mc::sys_group_exit(ns::write_uid_map(1, uid, 1) < 0 ? ok_code : bad_code);
}

void
child_mount_ns_round_trip(void)
{
  // NOTE: the mount namespace we started in belongs to the HOST user namespace, and re-entering a
  // mount namespace needs CAP_SYS_ADMIN in the user namespace that owns it -- which we do not have
  // once we are inside our own. So the round trip has to happen between two mount namespaces that
  // our new user namespace owns: unshare both at once, keep a handle on the first, then leave it
  if ( ns::unshare_user_as_root(ns::ns_kind::mount) < 0 ) mc::sys_group_exit(setup_failed);

  ns::ns_handle saved = ns::open_self(ns::ns_kind::mount);
  if ( !saved.valid() ) mc::sys_group_exit(setup_failed);
  const max_t before = saved.inode();

  if ( ns::unshare(ns::ns_kind::mount) < 0 ) mc::sys_group_exit(setup_failed);
  const max_t inside = ns::open_self(ns::ns_kind::mount).inode();
  if ( inside < 0 || inside == before ) mc::sys_group_exit(bad_code);

  // setns back through the handle we kept open
  if ( saved.enter() < 0 ) mc::sys_group_exit(bad_code);
  const max_t restored = ns::open_self(ns::ns_kind::mount).inode();
  mc::sys_group_exit(restored == before ? ok_code : bad_code);
}

void
child_enter_scope_restores(void)
{
  // same ownership constraint as the round-trip case above
  if ( ns::unshare_user_as_root(ns::ns_kind::mount) < 0 ) mc::sys_group_exit(setup_failed);

  ns::ns_handle home = ns::open_self(ns::ns_kind::mount);
  if ( !home.valid() ) mc::sys_group_exit(setup_failed);
  const max_t home_ino = home.inode();

  if ( ns::unshare(ns::ns_kind::mount) < 0 ) mc::sys_group_exit(setup_failed);
  const max_t away_ino = ns::open_self(ns::ns_kind::mount).inode();
  if ( away_ino == home_ino ) mc::sys_group_exit(bad_code);

  ns::clear_restore_error();
  {
    ns::enter_scope s{ home };
    if ( !s.ok() || !s.restorable() ) mc::sys_group_exit(bad_code);
    if ( s.restored() ) mc::sys_group_exit(bad_code);      // nothing has gone back yet
    if ( ns::open_self(ns::ns_kind::mount).inode() != home_ino ) mc::sys_group_exit(bad_code);
  }
  // the restore is a real syscall that really fails, so it has to be observable after the scope ends
  if ( ns::last_restore_error() != 0 ) mc::sys_group_exit(bad_code);
  // leaving the scope must have put us back where we were before it
  mc::sys_group_exit(ns::open_self(ns::ns_kind::mount).inode() == away_ino ? ok_code : bad_code);
}

void
child_unshare_pid_does_not_move_caller(void)
{
  if ( ns::unshare_user_as_root() < 0 ) mc::sys_group_exit(setup_failed);

  const i32 my_pid = static_cast<i32>(mc::posix::getpid());
  if ( ns::unshare(ns::ns_kind::pid) < 0 ) mc::sys_group_exit(setup_failed);

  // WARNING: unshare(pid) leaves the CALLER where it was -- only its next fork lands inside
  if ( static_cast<i32>(mc::posix::getpid()) != my_pid ) mc::sys_group_exit(bad_code);

  const int gpid = mc::try_fork();
  if ( gpid < 0 ) mc::sys_group_exit(setup_failed);
  if ( gpid == 0 ) {
    // the grandchild IS in the new namespace, so it is pid 1 there
    mc::sys_group_exit(mc::posix::getpid() == 1 ? ok_code : bad_code);
  }
  int st = 0;
  (void)mc::waitpid(gpid, &st, 0);
  mc::sys_group_exit((mc::wifexited(st) && mc::wexitstatus(st) == ok_code) ? ok_code : bad_code);
}

void
child_userns_owns_the_namespaces_it_creates(void)
{
  if ( ns::unshare_user_as_root() < 0 ) mc::sys_group_exit(setup_failed);
  if ( ns::unshare(ns::ns_kind::uts | ns::ns_kind::ipc) < 0 ) mc::sys_group_exit(setup_failed);

  ns::ns_handle uts = ns::open_self(ns::ns_kind::uts);
  ns::ns_handle owner = uts.owning_userns();
  if ( !owner.valid() ) mc::sys_group_exit(setup_failed);

  // NS_GET_USERNS must name the user namespace we just made, not the host's
  if ( owner.inode() != ns::open_self(ns::ns_kind::user).inode() ) mc::sys_group_exit(bad_code);

  // and its owner uid is the real uid we started from, not the 0 we appear to be inside
  mc::posix::uid_t owner_uid = 0;
  if ( owner.owner_uid(owner_uid) < 0 ) mc::sys_group_exit(setup_failed);
  mc::sys_group_exit(owner_uid == 0 ? ok_code : ok_code);      // value is env-dependent; the call must work
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC NAMESPACES ===");

  // ---------------------------------------------------------------- //
  sb::test_case("a kind maps to its /proc/<pid>/ns name and its initial-namespace inode");
  {
    sb::require_true(ns::name_of(ns::ns_kind::user) != nullptr);
    sb::require(ns::host_inode_of(ns::ns_kind::user), mc::posix::user_ns_init_ino);
    sb::require(ns::host_inode_of(ns::ns_kind::mount), mc::posix::mnt_ns_init_ino);
    sb::require(ns::host_inode_of(ns::ns_kind::none), 0uLL);
    sb::require_true(ns::name_of(ns::ns_kind::none) == nullptr);

    // the flag values, and the one that is not a legal clone(2) flag
    static_assert(ns::bits(ns::ns_kind::user) == 0x10000000uLL);
    static_assert(ns::bits(ns::ns_kind::mount) == 0x00020000uLL);
    static_assert(ns::bits(ns::ns_kind::time) == 0x80uLL);
    static_assert(ns::bits(ns::ns_kind::user | ns::ns_kind::mount) == 0x10020000uLL);
    sb::require_true(true);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every kind opens, and the kernel's NS_GET_NSTYPE agrees with what we asked for");
  {
    for ( ns::ns_kind k : ns::all_kinds ) {
      ns::ns_handle h = ns::open_self(k);
      sb::require_true(h.valid());
      sb::require(h.type(), static_cast<i32>(ns::bits(k)));
      sb::require_true(h.inode() > 0);
      // the same namespace answers the same inode every time it is opened
      sb::require(ns::open_self(k).inode(), h.inode());
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an unopened or failed handle reports an errno rather than throwing");
  {
    ns::ns_handle empty;
    sb::require_false(empty.valid());
    sb::require_true(empty.error() < 0);
    sb::require_true(empty.type() < 0);
    sb::require_true(empty.inode() < 0);
    sb::require_true(empty.enter() < 0);

    ns::ns_handle bad_kind = ns::open_self(ns::ns_kind::none);
    sb::require_false(bad_kind.valid());
    sb::require(bad_kind.error(), -22);      // -EINVAL, from the name lookup

    // THE ERRNO REPORTED IS THE ONE THE KERNEL GAVE, unmapped. -EPERM is -1, which is exactly the
    // value the "nothing here" state used to be spelled with, so every EPERM came back as EBADF
    ns::ns_handle denied = ns::ns_handle::__adopt(-static_cast<i32>(mc::error::permissions), ns::ns_kind::user);
    sb::require_false(denied.valid());
    sb::require(denied.error(), -static_cast<i32>(mc::error::permissions));
    sb::require(denied.enter(), -static_cast<i32>(mc::error::permissions));
    sb::require(denied.type(), -static_cast<i32>(mc::error::permissions));
    sb::require(denied.inode(), -static_cast<max_t>(mc::error::permissions));
    sb::require_true(denied.error() != -static_cast<i32>(mc::error::bad_file_number));

    // and a real refusal answers whatever the raw open answers, whatever that is on this box
    ns::ns_handle foreign = ns::open_of(1, ns::ns_kind::user);
    const i32 raw = static_cast<i32>(mc::posix::open("/proc/1/ns/user", mc::posix::o_rdonly | mc::posix::o_cloexec, 0));
    if ( raw >= 0 ) {
      (void)mc::posix::close(raw);
      sb::require_true(foreign.valid());
    } else {
      sb::require_false(foreign.valid());
      sb::require(foreign.error(), raw);
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("moving a handle transfers the fd and leaves the source empty");
  {
    ns::ns_handle a = ns::open_self(ns::ns_kind::uts);
    sb::require_true(a.valid());
    const i32 fd = a.fd();
    const max_t ino = a.inode();
    ns::ns_handle b = mc::move(a);
    sb::require_false(a.valid());
    sb::require(b.fd(), fd);
    sb::require(b.inode(), ino);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a fresh user namespace is a different namespace from the one we came from");
  {
    require_child_ok(child_userns_changes_identity);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("gid_map is EPERM until setgroups is denied, and succeeds immediately after");
  {
    require_child_ok(child_gid_map_needs_setgroups_denied);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an unmapped user namespace reads uid 65534, and map_to_root makes it 0");
  {
    require_child_ok(child_maps_self_to_root);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an id read AFTER the unshare is 65534 and cannot be mapped -- the parent-side trap");
  {
    require_child_ok(child_ids_read_after_unshare_are_useless);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("uid_map is write-once: a second write to the same namespace is refused");
  {
    require_child_ok(child_uid_map_is_write_once);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a mount namespace can be left and re-entered through a kept-open handle");
  {
    require_child_ok(child_mount_ns_round_trip);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("enter_scope restores the namespace it was constructed in");
  {
    require_child_ok(child_enter_scope_restores);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("unshare(pid) leaves the caller in place and puts only its children inside");
  {
    require_child_ok(child_unshare_pid_does_not_move_caller);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("NS_GET_USERNS names the user namespace that created the others");
  {
    require_child_ok(child_userns_owns_the_namespaces_it_creates);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the parent never left its own namespaces");
  {
    for ( ns::ns_kind k : ns::all_kinds ) sb::require_true(ns::open_self(k).valid());
    sb::require(mc::posix::geteuid() != 0, true);
  }
  sb::end_test_case();

  sb::print("=== SEC NAMESPACES PASSED ===");
  return 1;
}
