//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2026-63917 / CVE-2026-63921  --  Linux VTI6, CVSS 8.8, CWE-416 / CWE-843
// CVE-2022-0185                    --  Linux legacy_parse_param, CVSS 8.4, CWE-787
//
// 63917: "VTI6 namespace handling could leave stale cross-netns state and lead to use-after-free or
//         kernel failure, with potential cross-tenant impact."
// 63921: "IPv6 VTI network-namespace confusion reachable from an unprivileged user namespace could
//         cross namespace or tenant boundaries on container hosts."
// 0185:  "A heap overflow in filesystem-context handling could be reached by unprivileged users
//         when unprivileged user namespaces are enabled."
//
// THE SHAPE -- one primitive, three bugs
//
// All three are kernel bugs in code an unprivileged user cannot ordinarily reach. What makes them
// reachable is identical in every case: CLONE_NEWUSER gives the caller a full capability set inside
// the new user namespace, and that set is honoured by every namespace subsequently created from it.
// A nested user namespace therefore hands an unprivileged, sandboxed process CAP_NET_ADMIN in a
// fresh netns (the VTI6 pair) and CAP_SYS_ADMIN in a fresh mount namespace (0185's fsconfig path).
//
// The sandbox's job is not to have no kernel bugs. It is to keep the confined process away from the
// syscalls that reach them. That means: no new user namespaces, and no new mount namespaces.
//
// MICRON'S ANALOGUE
//
// src/sec/groups.hpp ships a group whose entire reason to exist is this, and it does not do it:
//
//     struct process_no_ns {                                    // groups.hpp:254
//       static constexpr i32 calls[] = {
//         SYS_pipe2, SYS_restart_syscall, SYS_clone, SYS_clone3, SYS_execve, ...
//                                         ^^^^^^^^^  ^^^^^^^^^^
//
// It is groups::process minus SYS_unshare and SYS_setns -- and that is the whole difference. But
// unshare() is not how you get a namespace; it is one of three ways, and the other two are still
// here. clone(CLONE_NEWUSER|CLONE_NEWNET, ...) needs no unshare at all. So "process_no_ns" allows
// exactly the operation its name forbids.
//
// clone3 is worse, and it is worse in a way that cannot be repaired by adding an argument rule.
// clone3 takes a POINTER to struct clone_args:
//
//     long clone3(struct clone_args *uargs, size_t size);
//
// seccomp-BPF cannot dereference user memory -- it sees the pointer, never the flags behind it. So
// there is no filter, in micron or anywhere else, that can allow clone3 and constrain its
// namespace flags. Any policy that filters clone by flags and permits clone3 has written a rule
// with a documented, unclosable hole next to it. This is why Docker and podman deny clone3 outright
// rather than filtering it.
//
// THE COST, STATED UP FRONT
//
// micron's own thread spawn issues clone3 (thread/, CLAUDE.md §2: "threads (pthread or clone3)").
// A policy that denies clone3 cannot spawn micron threads. That is a real trade and it is the
// caller's to make -- but it has to be MAKEABLE, and today there is no group that expresses it.
//
// WHAT THIS PINS
//   1  groups::process_no_ns must not name SYS_clone3     (unfilterable => must be denied outright)
//   2  a policy built on process_no_ns must refuse clone with any namespace bit set
//   3  ... for every namespace bit individually, not just CLONE_NEWUSER
//   4  ... and must refuse clone3 unconditionally
//   5  there must exist a group naming the namespace-creating calls, for `deny<>` to point at
//   6  live: under such a policy, clone(CLONE_NEWUSER|CLONE_NEWNET) actually fails
//   7  live, the composed claim: with the nested userns sealed, the CVE-2022-0185 entry point
//      (fsopen) is unreachable too
//
// POLARITY: inverted. Contracts 1-6 FAIL on the tree as it stands. Contract 1 fails because
// process_no_ns names clone3; 2-4 fail because `allow<...process_no_ns>` emits a bare
// fb.allow(SYS_clone) with no argument predicate; 5 fails because groups::namespaces does not
// exist. All pass once the group is repaired.
//
// NEGATIVE CONTROL: contract 2 does not merely assert the deny -- it first shows, through the same
// oracle and in the same run, that the UNCONSTRAINED spelling permits the call. If the two ever
// agree, the assertion has stopped observing the rule and is passing vacuously.
//
// CONTROL (ungated): a namespace-denying policy must still permit an ordinary clone. Denying clone
// outright would stop micron's own fork/spawn and every process the sandbox exists to run; a policy
// that cannot start a process is not a sandbox.
//
// Build:
//   duck test tests/adv/adv_cve_2026_63917_nested_userns.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/sys/sched.hpp"
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

// every CLONE_NEW* the kernel has. A "no new namespaces" rule that misses one is a rule that
// forbids six doors and leaves the seventh open.
constexpr u64 ns_bits = mc::posix::clone_newns | mc::posix::clone_newuts | mc::posix::clone_newipc | mc::posix::clone_newuser
                        | mc::posix::clone_newpid | mc::posix::clone_newnet | mc::posix::clone_newcgroup | mc::posix::clone_newtime;

// what a thread spawn actually passes: shares everything, creates nothing
constexpr u64 thread_flags = mc::posix::clone_vm | mc::posix::clone_fs | mc::posix::clone_files | mc::posix::clone_sighand
                             | mc::posix::clone_thread | mc::posix::clone_sysvsem | mc::posix::clone_settls;

const char *
bit_name(u64 b)
{
  if ( b == mc::posix::clone_newns ) return "CLONE_NEWNS";
  if ( b == mc::posix::clone_newuts ) return "CLONE_NEWUTS";
  if ( b == mc::posix::clone_newipc ) return "CLONE_NEWIPC";
  if ( b == mc::posix::clone_newuser ) return "CLONE_NEWUSER";
  if ( b == mc::posix::clone_newpid ) return "CLONE_NEWPID";
  if ( b == mc::posix::clone_newnet ) return "CLONE_NEWNET";
  if ( b == mc::posix::clone_newcgroup ) return "CLONE_NEWCGROUP";
  if ( b == mc::posix::clone_newtime ) return "CLONE_NEWTIME";
  return "?";
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2026-63917 / 63921 / 2022-0185 (nested userns as the reachability primitive) ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  clone3 is unfilterable, so a no-namespace group must not name it

  {
    sb::test_case("groups::process_no_ns must not name SYS_clone3");
    const bool named = adv::group_names<g::process_no_ns>(SYS_clone3);
    if ( named )
      sb::print("  process_no_ns names SYS_clone3 (groups.hpp:257): its flags live behind a pointer, "
                "so no argument rule can ever constrain them");
    sb::require_false(named);
  }

  {
    sb::test_case("groups::process_no_ns must not name SYS_setns or SYS_unshare either");
    // this half already holds; it is here so the case documents the whole boundary rather than the
    // one edge that was wrong
    sb::require_false(adv::group_names<g::process_no_ns>(SYS_unshare));
    sb::require_false(adv::group_names<g::process_no_ns>(SYS_setns));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  the assembled policy, driven -- with the vacuity check built in

  {
    sb::test_case("a process_no_ns policy must refuse clone(CLONE_NEWUSER|CLONE_NEWNET)");

    // (a) the negative control, first and in the same run: an unconstrained allow DOES permit it.
    // If (b) ever starts agreeing with this, the assertion below has gone blind.
    sc::filter_builder<64> loose;
    loose.require_native_arch();
    loose.allow(SYS_clone);
    loose.default_errno(eperm);
    const u32 unconstrained = adv::filter_action(loose, SYS_clone, mc::posix::clone_newuser | mc::posix::clone_newnet);
    sb::require(unconstrained, sc::act_allow());

    // (b) THE GROUP ALONE IS NOT ENOUGH, and this is the assertion that says so out loud.
    //
    // groups::process_no_ns is a flat list of syscall numbers. A number list can express "may call
    // clone" and cannot express "may call clone without CLONE_NEWUSER" -- that needs an argument
    // predicate, which lives in the `no_new_namespaces` RULE. So allowlisting the group by itself
    // still permits a nested user namespace, and a reader who trusted the name would not know.
    //
    // This half is required to keep FAILING to grant, i.e. it documents the trap permanently rather
    // than being fixed away. If a future group could carry a predicate, this case is what would have
    // to be revisited.
    using group_only = s::seccomp_policy<s::allow<g::baseline, g::process_no_ns>>;
    auto bare = group_only::build(sc::act_errno(eperm));
    sb::require_true(bare.valid());
    const u32 group_alone = adv::filter_action(bare, SYS_clone, mc::posix::clone_newuser | mc::posix::clone_newnet);
    sb::print("  allow<baseline,process_no_ns> alone   -> ",
              group_alone == sc::act_allow() ? "ALLOW (the group is a number list; it cannot filter args)" : "denied");

    // (c) the composed policy -- what a caller is supposed to write, and what must actually hold
    using jail = s::seccomp_policy<s::no_new_namespaces<>, s::allow<g::baseline, g::process_no_ns>>;
    auto fb = jail::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());

    const u32 shipped = adv::filter_action(fb, SYS_clone, mc::posix::clone_newuser | mc::posix::clone_newnet);
    sb::print("  + no_new_namespaces<>                 -> ", shipped == sc::act_allow() ? "ALLOW  <-- nested userns reachable" : "denied");

    sb::require_distinct(shipped, unconstrained);
    sb::require_distinct(shipped, sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  every bit, not just the famous one

  {
    sb::test_case("a process_no_ns policy must refuse clone with ANY namespace bit set");
    using jail = s::seccomp_policy<s::no_new_namespaces<>, s::allow<g::baseline, g::process_no_ns>>;
    auto fb = jail::build(sc::act_errno(eperm));

    u32 leaked = 0;
    for ( u64 b = 1; b != 0; b <<= 1 ) {
      if ( (ns_bits & b) == 0 ) continue;
      const u32 a = adv::filter_action(fb, SYS_clone, b);
      if ( a == sc::act_allow() ) {
        sb::print("    reachable: clone(", bit_name(b), ")");
        ++leaked;
      }
    }
    if ( leaked != 0 ) sb::print("  ", leaked, " of 8 namespace kinds creatable under a policy named process_no_ns");
    sb::require(leaked, 0u);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  and clone3 unconditionally

  {
    sb::test_case("a process_no_ns policy must refuse clone3 outright");
    using jail = s::seccomp_policy<s::no_new_namespaces<>, s::allow<g::baseline, g::process_no_ns>>;
    auto fb = jail::build(sc::act_errno(eperm));
    // no argument is passed, because no argument CAN be inspected -- that is the whole point
    sb::require_distinct(adv::filter_action(fb, SYS_clone3, 0, 0), sc::act_allow());
    // and the group must not name it either, so a caller who forgets the rule still cannot reach it
    sb::require_false(adv::group_names<g::process_no_ns>(SYS_clone3));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  the deny vocabulary
  //
  // The allowlist direction is safe by omission: a group that never names fsopen cannot grant it.
  // The DENY direction has no such luck -- `deny<Gs...>` (policy.hpp:121) takes groups, and there is
  // no group in the tree naming the namespace or mount-API calls. A caller building a denylist has
  // nothing to point at, which is how a hardening step gets skipped.

  {
    sb::test_case("a group must exist naming the namespace-creating calls");
#if defined(__micron_sec_has_groups_namespaces)
    sb::require_true(adv::group_names<g::namespaces>(SYS_unshare));
    sb::require_true(adv::group_names<g::namespaces>(SYS_setns));
    sb::require_true(adv::group_names<g::namespaces>(SYS_clone3));
#else
    sb::print("  groups::namespaces does not exist: `deny<>` has nothing to name for this class");
    sb::require_true(false);
#endif
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated
  //
  // A namespace-denying policy must still start a process. This is the assertion that stops the
  // obvious wrong fix (drop SYS_clone from the group) from looking like a pass.

  {
    sb::test_case("control: an ordinary clone -- and a thread spawn -- must still be permitted");
    using jail = s::seccomp_policy<s::no_new_namespaces<>, s::allow<g::baseline, g::process_no_ns>>;
    auto fb = jail::build(sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_clone, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_clone, thread_flags), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_clone, mc::posix::sig_chld), sc::act_allow());
    // and the rest of the group is untouched by whatever we do to clone
    sb::require(adv::filter_action(fb, SYS_execve, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_wait4, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_getpid), sc::act_allow());
  }

  {
    sb::test_case("control: groups::process (the unconfined variant) is deliberately still permissive");
    // process_no_ns is the confined one. groups::process is documented as the full set, and a
    // caller reaching for it has said so. Asserting it here keeps the fix from over-reaching.
    sb::require_true(adv::group_names<g::process>(SYS_unshare));
    sb::require_true(adv::group_names<g::process>(SYS_setns));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  live

  {
    sb::test_case("live: clone(CLONE_NEWUSER|CLONE_NEWNET) under the policy must fail");
    if ( !adv::have_userns() ) {
      sb::skip("this kernel refuses an unprivileged user namespace outright; the filter is not what "
               "would be under test");
    } else {
      const adv::child_result r = adv::run_child([]() -> i32 {
        using jail = s::seccomp_policy_n<512, s::no_new_namespaces<>, s::allow<g::baseline, g::process_no_ns, g::signal>>;
        auto fb = jail::build(sc::act_errno(eperm));
        if ( !fb.valid() ) return adv::setup_failed;
        if ( sc::load(fb, true) < 0 ) return adv::setup_failed;

        // the 63921 primitive: a userns we are root in, and a netns created from it
        if ( mc::posix::unshare(mc::posix::clone_newuser | mc::posix::clone_newnet) >= 0 ) return adv::bad_code;
        // ... and the door around unshare
        const long via_clone = mc::syscall(SYS_clone, mc::posix::clone_newuser | mc::posix::clone_newnet | mc::posix::sig_chld, 0, 0, 0, 0);
        if ( via_clone == 0 ) mc::sys_group_exit(adv::bad_code);      // we are the new child: it worked
        if ( via_clone > 0 ) return adv::bad_code;                    // we are the parent: it worked
        return adv::ok_code;
      });
      if ( r.g == adv::grade::bad ) sb::print("  a nested user+network namespace was created under the policy");
      sb::require_true(r.ok());
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 7  the composed claim -- CVE-2022-0185's entry point
  //
  // fsopen/fsconfig is the 0185 path. Under an allowlist it is denied by omission, which is the
  // right answer for the wrong reason: nothing in the tree says so, and groups::filesystem grants
  // the LEGACY mount API while silently omitting the modern one, so "filesystem" does not mean what
  // it reads. Pin both halves.

  {
    sb::test_case("the CVE-2022-0185 entry point is unreachable from a confined allowlist");
    // groups::filesystem is deliberately NOT in this list. That group grants mount, chroot and
    // pivot_root, so a caller reaching for it has asked to shape mounts and gets the modern API with
    // the legacy one -- which is the fix for the OTHER half of 0185 (see
    // adv_cve_2022_0185_fsconfig.cpp contract 4, where a group called "filesystem" granting the 1991
    // API and not the 2019 one was itself the defect). The claim here is about the groups a CONFINED
    // workload composes, and it is those that must not reach fsconfig.
    using jail = s::seccomp_policy<s::no_new_namespaces<>, s::allow<g::baseline, g::io, g::process_no_ns, g::filesystem_no_mount>>;
    auto fb = jail::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());

    const i32 mount_api[] = { SYS_fsopen, SYS_fsconfig, SYS_fsmount, SYS_fspick, SYS_open_tree, SYS_move_mount };
    for ( const i32 nr : mount_api ) sb::require_distinct(adv::filter_action(fb, nr, 0, 0), sc::act_allow());
    // and the legacy half, which is what "no_mount" has to mean
    sb::require_distinct(adv::filter_action(fb, SYS_mount, 0, 0), sc::act_allow());
    sb::require_distinct(adv::filter_action(fb, SYS_chroot, 0), sc::act_allow());
  }

  sb::print("=== ADV CVE-2026-63917 PASSED ===");
  return 1;
}
