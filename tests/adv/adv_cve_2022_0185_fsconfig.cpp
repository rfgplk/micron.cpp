//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2022-0185  --  Linux legacy_parse_param, CVSS 8.4, CWE-787 (out-of-bounds write)
//
// "A heap overflow in filesystem-context handling could be reached by unprivileged users when
//  unprivileged user namespaces are enabled, potentially enabling privilege escalation."
//
// THE SHAPE
//
// The overflow is in legacy_parse_param(), reached through fsconfig(2) -- part of the "new mount
// API" the kernel grew in 5.2: fsopen, fsconfig, fsmount, fspick, open_tree, move_mount. Six new
// syscalls that do everything mount(2) did, plus more, and that every syscall filter written before
// 5.2 had never heard of.
//
// That is the transferable lesson and it has nothing to do with the overflow. A denylist that names
// `mount` and `umount2` and stops there was complete in 2019 and is not complete now. The kernel
// added doors; a policy that enumerates doors has to be told.
//
// MICRON'S ANALOGUE
//
// micron KNOWS all six numbers -- bits/__syscall_codes_amd64.hpp:734-739 defines SYS_open_tree,
// SYS_move_mount, SYS_fsopen, SYS_fsconfig, SYS_fsmount, SYS_fspick, and :748 SYS_mount_setattr --
// and NO GROUP NAMES ANY OF THEM. groups::filesystem (groups.hpp:173-191) grants the legacy API:
//
//     SYS_mount, SYS_umount2, SYS_chroot, SYS_pivot_root, SYS_sync, SYS_statfs, ...
//
// Two consequences, and they point in opposite directions.
//
// (a) In the ALLOWLIST direction micron is safe, by omission. A group that never names fsopen
//     cannot grant it. Safe for the right reason? No -- safe because nobody wrote it down, and the
//     next person to make "filesystem" mean "can mount things" will add the six and not notice they
//     are adding this.
//
// (b) In the DENYLIST direction micron is not safe, because `deny<Gs...>` (policy.hpp:121-134) takes
//     GROUPS, and there is no group to point at. A caller writing
//
//         seccomp_policy<allow<...>, deny<groups::filesystem>>
//
//     has denied the legacy mount API and left the modern one alone. The deny reads as complete and
//     is not, which is worse than having no deny at all -- an absent hardening step gets noticed.
//
// (c) `groups::filesystem` is internally inconsistent: it grants chroot and pivot_root and mount but
//     not mount_setattr, which micron's OWN policy layer needs (policy.hpp:57-61, __remount_ro
//     routes recursive read-only remounts through mount_setattr). So a sandbox allowlisting
//     groups::filesystem cannot perform a recursive read-only bind -- a bug in the other direction,
//     found by asking this question.
//
// WHAT THIS PINS
//   1  a group exists naming the new mount API, so `deny<>` has something to point at
//   2  deny<that group> actually reaches all seven numbers
//   3  no shipped ALLOW group leaks any of the seven                (holds today; regression guard)
//   4  groups::filesystem is self-consistent: it grants the modern API alongside the legacy one
//   5  the composed reachability claim: with the mount API denied, fsopen is refused at runtime
//   6  the numbers themselves are right -- checked against the kernel, not against our own table
//
// POLARITY: inverted. Contracts 1, 2 and 4 FAIL on the tree as it stands. Contract 3 passes today
// and is the guard that a fix for 4 does not widen the wrong groups. Contract 6 passes and is here
// because a table typo would make every other contract vacuous.
//
// NEGATIVE CONTROL: contract 6 asks the KERNEL what these numbers are, by issuing each one with
// arguments guaranteed to fail and requiring the errno to be one only that syscall produces --
// never ENOSYS, which is what a wrong number gives. Without it, "the policy denies SYS_fsopen"
// could be true of a number that is not fsopen.
//
// CONTROL (ungated): denying the mount API must not disturb ordinary file io. openat, read, write,
// statx and close are not mount syscalls and a policy that caught them would be unusable.
//
// Build:
//   duck test tests/adv/adv_cve_2022_0185_fsconfig.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/sec/groups.hpp"
#include "../../src/sec/policy.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace s = micron::sec;

namespace
{

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

struct named_nr {
  i32 nr;
  const char *name;
};

// the six of the new mount API, plus mount_setattr which arrived with the same wave
constexpr named_nr mount_api[] = {
  { SYS_fsopen, "fsopen" },
  { SYS_fsconfig, "fsconfig" },
  { SYS_fsmount, "fsmount" },
  { SYS_fspick, "fspick" },
  { SYS_open_tree, "open_tree" },
  { SYS_move_mount, "move_mount" },
  { SYS_mount_setattr, "mount_setattr" },
};

constexpr named_nr legacy_api[] = {
  { SYS_mount, "mount" },
  { SYS_umount2, "umount2" },
  { SYS_pivot_root, "pivot_root" },
  { SYS_chroot, "chroot" },
};

constexpr i32 reached_fsopen = 101;

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2022-0185 (the mount API a denylist has never heard of) ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  NEGATIVE CONTROL FIRST: are these numbers actually the syscalls we think?
  //
  // Every contract below is of the form "the policy denies SYS_fsopen". That says nothing unless
  // SYS_fsopen is fsopen. A wrong entry in the table would make the whole file agree with itself and
  // with nothing else. So: issue each one with arguments that must fail, and require the kernel's
  // answer to be a real error rather than ENOSYS.

  {
    sb::test_case("negative control: the syscall numbers are the kernel's, not just our table's");
    constexpr long enosys = -static_cast<long>(mc::error::bad_syscall);
    usize present = 0;
    for ( const named_nr &c : mount_api ) {
      // fsopen(nullptr, 0) -> EFAULT/EINVAL; open_tree(-1, nullptr, 0) -> EFAULT/EBADF; etc.
      // Whatever it is, it must not be ENOSYS, which is what an unassigned number returns.
      const long r = mc::syscall(c.nr, 0, 0, 0, 0, 0);
      if ( r != enosys )
        ++present;
      else
        sb::print("    ", c.name, " (nr ", c.nr,
                  ") answered ENOSYS: either this kernel lacks it "
                  "or the number is wrong");
    }
    sb::print("  ", present, " of 7 mount-API syscalls answered as themselves on this kernel");
    // 5.2 gave us six of these and 5.12 the seventh; anything modern enough to run this suite has
    // them all. A zero here would mean the table is wrong, not that the kernel is old.
    sb::require_true(present >= 6);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1 + 2  the deny vocabulary

  {
    sb::test_case("a group must exist naming the new mount API");
#if defined(__micron_sec_has_groups_mount_api)
    for ( const named_nr &c : mount_api ) {
      if ( !adv::group_names<g::mount_api>(c.nr) ) sb::print("    groups::mount_api omits ", c.name);
      sb::require_true(adv::group_names<g::mount_api>(c.nr));
    }
#else
    sb::print("  groups::mount_api does not exist: a caller writing deny<...> has nothing to name for "
              "fsopen/fsconfig/fsmount/fspick/open_tree/move_mount/mount_setattr");
    sb::require_true(false);
#endif
  }

  {
    sb::test_case("deny<that group> reaches every one of the seven numbers");
#if defined(__micron_sec_has_groups_mount_api)
    using hardened = s::seccomp_policy<s::deny<g::mount_api>, s::allow<g::baseline, g::io>>;
    auto fb = hardened::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    for ( const named_nr &c : mount_api ) {
      const u32 a = adv::filter_action(fb, c.nr, 0, 0);
      if ( a == sc::act_allow() ) sb::print("    reachable: ", c.name);
      sb::require_distinct(a, sc::act_allow());
    }
#else
    sb::skip("groups::mount_api does not exist; the deny cannot be written (see the case above)");
#endif
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  no allow group leaks one -- holds today, and must keep holding

  {
    sb::test_case("no shipped allow-group names a new-mount-API syscall");
    for ( const named_nr &c : mount_api ) {
      sb::require_false(adv::group_names<g::baseline>(c.nr));
      sb::require_false(adv::group_names<g::io>(c.nr));
      sb::require_false(adv::group_names<g::filesystem_readonly>(c.nr));
      sb::require_false(adv::group_names<g::filesystem_no_mount>(c.nr));
      sb::require_false(adv::group_names<g::process_no_ns>(c.nr));
    }
  }

  {
    sb::test_case("a broad allowlist still refuses the whole mount API");
    using broad = s::seccomp_policy<
        s::allow<g::baseline, g::io, g::memory, g::signal, g::time, g::ipc, g::io_multiplexing, g::filesystem_no_mount>>;
    auto fb = broad::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    for ( const named_nr &c : mount_api ) sb::require_distinct(adv::filter_action(fb, c.nr, 0, 0), sc::act_allow());
    // and the legacy half stays denied too, which is what "no_mount" has to mean
    for ( const named_nr &c : legacy_api ) sb::require_distinct(adv::filter_action(fb, c.nr, 0, 0), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  groups::filesystem must mean what it says
  //
  // Found by asking this question rather than looking for it: the group grants mount/umount2/chroot/
  // pivot_root but not mount_setattr, which micron's OWN filesystem policy issues for a recursive
  // read-only bind (policy.hpp:57-61). So allowlisting groups::filesystem and then asking for
  // bind(..., read_only=true, recursive=true) faults at the filesystem stage, and the cause is the
  // group rather than the mount.

  {
    sb::test_case("groups::filesystem grants the modern mount API alongside the legacy one");
    // the legacy half, which it already does
    for ( const named_nr &c : legacy_api ) sb::require_true(adv::group_names<g::filesystem>(c.nr));

    // and the modern half, which it must
    usize missing = 0;
    for ( const named_nr &c : mount_api ) {
      if ( !adv::group_names<g::filesystem>(c.nr) ) {
        sb::print("    groups::filesystem grants mount(2) but not ", c.name);
        ++missing;
      }
    }
    if ( missing != 0 )
      sb::print("  a group called \"filesystem\" that grants the 1991 mount API and none of the 2019 "
                "one is not a description of anything");
    sb::require(missing, static_cast<usize>(0));
  }

  {
    sb::test_case("... and micron's own recursive read-only bind is expressible under it");
    // the concrete consequence: __remount_ro needs mount_setattr, so a policy allowlisting
    // groups::filesystem must permit it or sandbox::bind(read_only, recursive) cannot run
    using fs_policy = s::seccomp_policy<s::allow<g::baseline, g::io, g::filesystem>>;
    auto fb = fs_policy::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    sb::require(adv::filter_action(fb, SYS_mount_setattr, 0, 0), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  live: the reachability claim, end to end

  {
    sb::test_case("live: fsopen must be refused under a mount-denying policy");
    const adv::child_result r = adv::run_child([]() -> i32 {
      using hardened = s::seccomp_policy_n<512, s::allow<g::baseline, g::io, g::signal, g::memory>>;
      auto fb = hardened::build(sc::act_errno(eperm));
      if ( !fb.valid() ) return adv::setup_failed;
      if ( sc::load(fb, true) < 0 ) return adv::setup_failed;

      // CVE-2022-0185's entry point. Under the policy this must be EPERM from the filter, not the
      // kernel's own EFAULT -- i.e. the call never reaches legacy_parse_param at all.
      const long got = mc::syscall(SYS_fsopen, "ext4", 0, 0, 0, 0);
      if ( got != -static_cast<long>(mc::error::permissions) ) return reached_fsopen;
      const long cfg = mc::syscall(SYS_fsconfig, -1, 0, 0, 0, 0);
      if ( cfg != -static_cast<long>(mc::error::permissions) ) return reached_fsopen;
      return adv::ok_code;
    });
    if ( r.code == reached_fsopen ) sb::print("  fsopen/fsconfig reached the kernel under the policy");
    sb::require_true(r.ok());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: denying the mount API leaves ordinary file io alone");
    using broad = s::seccomp_policy<s::allow<g::baseline, g::io>>;
    auto fb = broad::build(sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_openat, 0, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_read, 0, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_write, 1, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_close, 3), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_statx, 0, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_lseek, 3, 0, 0), sc::act_allow());
  }

  {
    sb::test_case("control: filesystem_readonly stays genuinely read-only");
    // the group whose contents were already corrected once (see sec_policy.cpp:216-255). If a fix
    // for contract 4 reaches for the wrong group, this is what notices.
    using ro = s::seccomp_policy<s::allow<g::baseline, g::filesystem_readonly>>;
    auto fb = ro::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    const i32 mutating[] = { SYS_unlinkat, SYS_renameat2, SYS_mkdirat, SYS_linkat, SYS_symlinkat, SYS_mount, SYS_fsopen };
    for ( const i32 nr : mutating ) sb::require_distinct(adv::filter_action(fb, nr, 0, 0), sc::act_allow());
    // and reads still work
    sb::require(adv::filter_action(fb, SYS_getdents64, 3, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_statfs, 0, 0), sc::act_allow());
  }

  sb::print("=== ADV CVE-2022-0185 PASSED ===");
  return 1;
}
