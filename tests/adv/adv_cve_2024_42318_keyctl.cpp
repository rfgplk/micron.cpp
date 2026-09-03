//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2024-42318  --  Linux Landlock, CVSS 8.8 (Linux CNA) / 5.5 (older NVD), CWE-noinfo
//
// "KEYCTL_SESSION_TO_PARENT could cause Landlock credentials or restrictions to be lost."
//     fixed: 5.15.165, 6.1.103, 6.6.44, 6.10.3, 6.11+
//
// THE SHAPE
//
// Landlock lives on the credential structure. KEYCTL_SESSION_TO_PARENT replaces the parent's session
// keyring, which meant building a fresh cred -- and the fresh cred did not carry the Landlock domain
// across. A confined process could therefore ask the kernel for a credential swap and come out the
// other side unconfined. Nothing about the sandbox looked different; the restriction was simply gone.
//
// The generalisation is the useful part: A CONFINEMENT THAT LIVES ON A CREDENTIAL IS ONLY AS DURABLE
// AS EVERY PATH THAT REBUILDS THAT CREDENTIAL. There is no way for userspace to enumerate those
// paths, so the defence is not to enumerate them: it is to deny the syscalls that reach them, and to
// not depend on a single LSM for the whole boundary.
//
// The keyring family is three syscalls -- keyctl, add_key, request_key -- and no ordinary sandboxed
// workload wants any of them.
//
// MICRON'S ANALOGUE
//
// No group in src/sec/groups.hpp names keyctl, add_key or request_key. Under an allowlist that is
// the right outcome: a group that never names a syscall cannot grant it, and every shipped policy in
// the tree is an allowlist with a deny default (policy.hpp:180-184 seals with act_errno(EPERM)).
//
// So micron is not vulnerable here -- and this file exists anyway, for two reasons.
//
// (a) The libjkr rule: every CVE gets a test whether or not we are vulnerable, because a passing
//     test is the permanent regression guard. The way this stops being true is somebody adding
//     `SYS_keyctl` to groups::process because a workload wanted a session keyring, and nothing
//     currently says why they should not.
//
// (b) The DENY direction has the same gap as the mount API (see adv_cve_2022_0185_fsconfig.cpp):
//     `deny<Gs...>` takes groups, and there is no group naming the keyring family to point at.
//     "Safe because nobody wrote it down" is a different property from "safe because we said so",
//     and only the second one survives an edit.
//
// WHAT THIS PINS
//   1  no shipped allow-group names keyctl / add_key / request_key      (holds today)
//   2  no assembled policy from the shipped groups permits them
//   3  a group exists naming the keyring family, so a deny can be written
//   4  live: a Landlock domain survives a KEYCTL_SESSION_TO_PARENT attempt   (the kernel's half)
//   5  a Landlock domain survives the credential changes a sandbox itself performs
//
// POLARITY: mixed, stated per contract. 1, 2 and 5 pass today and are regression guards. 3 FAILS on
// the tree as it stands. 4 depends on the kernel: this box runs 7.1.10, far past every fix listed
// above, so 4 is expected to pass and is a check on the ENVIRONMENT, not on micron -- it is reported
// as such rather than being allowed to look like a micron result.
//
// NEGATIVE CONTROL: contract 4 first establishes that the Landlock domain is really in force by
// requiring a denied open to be EACCES BEFORE the keyctl call. Without that half, "still denied
// afterwards" could mean the ruleset never applied.
//
// CONTROL (ungated): a policy that denies the keyring must not disturb the credential syscalls a
// sandbox legitimately performs -- setgroups/setgid/setuid are stage 10 of sandbox's own sequence
// (sandbox.hpp:43), and a policy catching them would kill every sandbox that drops privilege.
//
// Build:
//   duck test tests/adv/adv_cve_2024_42318_keyctl.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/sec/groups.hpp"
#include "../../src/sec/landlock.hpp"
#include "../../src/sec/policy.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace ll = micron::sec::landlock;
namespace s = micron::sec;

namespace
{

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

struct named_nr {
  i32 nr;
  const char *name;
};

constexpr named_nr keyring[] = {
  { SYS_keyctl, "keyctl" },
  { SYS_add_key, "add_key" },
  { SYS_request_key, "request_key" },
};

// the operation the CVE is named for
constexpr u64 keyctl_session_to_parent = 18;
constexpr u64 keyctl_get_keyring_id = 0;

constexpr const char *base = "/var/tmp/mc_adv_42318";
constexpr const char *allowed_dir = "/var/tmp/mc_adv_42318/in";
constexpr const char *denied_file = "/var/tmp/mc_adv_42318_denied.txt";

constexpr i32 domain_lost = 121;
constexpr i32 domain_never_applied = 122;

bool
make_fixture(void)
{
  (void)mc::posix::mkdir(base, 0700);
  (void)mc::posix::mkdir(allowed_dir, 0700);
  const i32 fd = static_cast<i32>(mc::posix::open(denied_file, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0600));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "DENY", 4);
  (void)mc::posix::close(fd);
  return true;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2024-42318 (a confinement that lives on a credential) ===");

  sb::test_case("fixture");
  if ( !make_fixture() ) {
    sb::skip("cannot build the /var/tmp fixture in this environment");
    sb::print("=== ADV CVE-2024-42318 SKIPPED ===");
    return 1;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1 + 2  the keyring is out of reach of every shipped allowlist

  {
    sb::test_case("no shipped allow-group names the keyring family");
    for ( const named_nr &c : keyring ) {
      sb::require_false(adv::group_names<g::baseline>(c.nr));
      sb::require_false(adv::group_names<g::io>(c.nr));
      sb::require_false(adv::group_names<g::process>(c.nr));
      sb::require_false(adv::group_names<g::process_no_ns>(c.nr));
      sb::require_false(adv::group_names<g::capabilities>(c.nr));
      sb::require_false(adv::group_names<g::ipc>(c.nr));
      sb::require_false(adv::group_names<g::filesystem>(c.nr));
      sb::require_false(adv::group_names<g::network>(c.nr));
      sb::require_false(adv::group_names<g::memory>(c.nr));
      sb::require_false(adv::group_names<g::signal>(c.nr));
      sb::require_false(adv::group_names<g::time>(c.nr));
      sb::require_false(adv::group_names<g::io_multiplexing>(c.nr));
    }
  }

  {
    sb::test_case("the broadest policy the shipped groups can build still refuses the keyring");
    using everything = s::seccomp_policy_n<2048, s::allow<g::baseline, g::io, g::memory, g::signal, g::time, g::ipc, g::network,
                                                          g::io_multiplexing, g::process, g::filesystem, g::capabilities>>;
    auto fb = everything::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    for ( const named_nr &c : keyring ) {
      const u32 a = adv::filter_action(fb, c.nr, keyctl_session_to_parent, 0);
      if ( a == sc::act_allow() ) sb::print("    reachable: ", c.name);
      sb::require_distinct(a, sc::act_allow());
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  and the deny direction has a name to use

  {
    sb::test_case("a group must exist naming the keyring family");
#if defined(__micron_sec_has_groups_keyring)
    for ( const named_nr &c : keyring ) {
      if ( !adv::group_names<g::keyring>(c.nr) ) sb::print("    groups::keyring omits ", c.name);
      sb::require_true(adv::group_names<g::keyring>(c.nr));
    }
    using hardened = s::seccomp_policy<s::deny<g::keyring>, s::allow<g::baseline, g::io, g::process>>;
    auto fb = hardened::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    for ( const named_nr &c : keyring ) sb::require_distinct(adv::filter_action(fb, c.nr, keyctl_session_to_parent, 0), sc::act_allow());
#else
    sb::print("  groups::keyring does not exist. micron is safe from this by OMISSION -- no group "
              "names keyctl -- which is a different property from being safe by decision, and only "
              "the second survives somebody adding SYS_keyctl to groups::process");
    sb::require_true(false);
#endif
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: denying the keyring leaves the credential syscalls a sandbox needs");
    // stage 10 of sandbox's own sequence is setgroups -> setgid -> setuid (sandbox.hpp:420-428).
    // A keyring deny that caught those would kill every sandbox that drops privilege.
    using p = s::seccomp_policy<s::allow<g::baseline, g::capabilities>>;
    auto fb = p::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    sb::require(adv::filter_action(fb, SYS_setgroups, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_setgid, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_setuid, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_capset, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_prctl, 0, 0), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  the kernel's half
  //
  // This contract is about THIS BOX, not about micron. Every kernel listed in the advisory as fixed
  // predates anything that can run this suite, so a pass here is expected and says nothing about
  // src/sec. It is worth running anyway: if it ever fails, the deny above stops being defence in
  // depth and becomes the only thing standing between a sandbox and an unconfining syscall.

  {
    sb::test_case("environment: a landlock domain survives KEYCTL_SESSION_TO_PARENT");
    if ( !adv::have_landlock(1) ) {
      sb::skip("landlock is unavailable on this kernel; there is no domain to lose");
    } else {
      const adv::child_result r = adv::run_child([]() -> i32 {
        ll::ruleset rs = ll::try_ruleset(ll::full_dir | ll::access_fs::execute);
        if ( !rs.valid() ) return adv::setup_failed;
        if ( rs.allow(allowed_dir, ll::read_only) < 0 ) return adv::setup_failed;
        if ( rs.restrict_self() < 0 ) return adv::setup_failed;

        // NEGATIVE CONTROL: the domain must be in force BEFORE the keyctl call, or "still denied
        // afterwards" would be true of a ruleset that never applied
        const i32 before = static_cast<i32>(mc::posix::open(denied_file, mc::posix::o_rdonly, 0));
        if ( before >= 0 ) {
          (void)mc::posix::close(before);
          return domain_never_applied;
        }

        // the CVE's operation. It may well fail -- EPERM, ENOSYS, no session keyring -- and that is
        // fine: what matters is what the domain looks like afterwards either way.
        (void)mc::syscall(SYS_keyctl, keyctl_session_to_parent, 0, 0, 0, 0);
        (void)mc::syscall(SYS_keyctl, keyctl_get_keyring_id, 0, 0, 0, 0);

        const i32 after = static_cast<i32>(mc::posix::open(denied_file, mc::posix::o_rdonly, 0));
        if ( after >= 0 ) {
          (void)mc::posix::close(after);
          return domain_lost;
        }
        return adv::ok_code;
      });
      if ( r.code == domain_never_applied )
        sb::print("  the landlock domain was not in force to begin with; contract 4 proves nothing here");
      if ( r.code == domain_lost ) sb::print("  THE LANDLOCK DOMAIN WAS LOST across keyctl(KEYCTL_SESSION_TO_PARENT)");
      sb::require_distinct(r.code, domain_never_applied);
      sb::require_true(r.ok());
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  the credential changes micron itself makes
  //
  // The same question, asked about the paths the sandbox actually walks. setuid/setgid/setgroups all
  // build a new credential, and stage 10 runs AFTER stage 6 (landlock) by design -- so if a cred
  // rebuild dropped the domain, micron's own stage ordering would be the thing that triggered it.

  {
    sb::test_case("a landlock domain survives the credential drop micron performs");
    if ( !adv::have_landlock(1) ) {
      sb::skip("landlock is unavailable on this kernel");
    } else {
      const adv::child_result r = adv::run_child([]() -> i32 {
        ll::ruleset rs = ll::try_ruleset(ll::full_dir | ll::access_fs::execute);
        if ( !rs.valid() ) return adv::setup_failed;
        if ( rs.allow(allowed_dir, ll::read_only) < 0 ) return adv::setup_failed;
        if ( rs.restrict_self() < 0 ) return adv::setup_failed;

        if ( static_cast<i32>(mc::posix::open(denied_file, mc::posix::o_rdonly, 0)) >= 0 ) return domain_never_applied;

        // the same order sandbox::__child uses at stage 10. Unprivileged, so these mostly fail --
        // which is the point: a FAILED cred change must not drop the domain either.
        (void)mc::syscall(SYS_setgroups, 0, 0);
        (void)mc::posix::setgid(mc::posix::getgid());
        (void)mc::posix::setuid(mc::posix::getuid());
        (void)mc::prctl(mc::PR_SET_DUMPABLE, 0UL);
        (void)mc::clear_ambient();

        const i32 after = static_cast<i32>(mc::posix::open(denied_file, mc::posix::o_rdonly, 0));
        if ( after >= 0 ) {
          (void)mc::posix::close(after);
          return domain_lost;
        }
        return adv::ok_code;
      });
      if ( r.code == domain_lost )
        sb::print("  the landlock domain was lost across the credential stage -- stage 6 runs before "
                  "stage 10 (sandbox.hpp:39-43), so this would make that ordering unsafe");
      sb::require_distinct(r.code, domain_never_applied);
      sb::require_true(r.ok());
    }
  }

  sb::print("=== ADV CVE-2024-42318 PASSED ===");
  return 1;
}
