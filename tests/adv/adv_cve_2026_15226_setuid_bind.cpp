//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2026-15226  --  snapd, CVSS 8.4, CWE-732 (incorrect permission assignment)
//
// "The default seccomp policy did not adequately restrict creation or execution of setuid binaries
//  inside strict confinement, potentially bypassing sandbox assumptions."
//
// THE SHAPE
//
// A confined process that can create a set-user-ID file on a filesystem the host can also see has
// manufactured a privilege-escalation primitive that outlives the sandbox. NO_NEW_PRIVS protects
// the confined process itself -- it will never gain privilege by exec'ing that file -- and that is
// exactly what makes the defect easy to miss: from inside, everything looks contained. The escape
// is that the file is still there afterwards, on a mount somebody else will execute from.
//
// snapd's seccomp policy governed the syscalls. It did not govern the MOUNT, and a mount that
// honours set-user-ID is what turns `chmod u+s` from a no-op into a weapon. The durable fix for
// this class is not a syscall rule at all: it is MS_NOSUID on everything the sandbox can write to,
// so the bit is inert no matter who sets it or who runs the file later.
//
// MICRON'S ANALOGUE
//
// micron mounts tmpfs and procfs correctly and binds incorrectly. All three, side by side:
//
//     // sandbox.hpp:545-552  tmpfs
//     mount_spec{ "tmpfs", dst, "tmpfs", options,
//                 posix::ms_nosuid | posix::ms_nodev | posix::ms_strictatime, false };
//
//     // sandbox.hpp:554-560  procfs
//     mount_spec{ "proc", dst, "proc", nullptr,
//                 posix::ms_nosuid | posix::ms_nodev | posix::ms_noexec, nullptr };
//
//     // sandbox.hpp:536-543  bind          <-- neither flag
//     const unsigned long fl = recursive ? (posix::ms_bind | posix::ms_rec) : posix::ms_bind;
//
// and identically in the type-level face at policy.hpp:392-402. A bind mount takes its per-mount
// flags from the source mount, so binding any part of a normally-mounted host filesystem (`/`,
// `/usr`, `/var` -- all suid,dev on a stock Fedora box) carries suid and dev semantics into the
// jail. The bind is also the ONLY one of the three that shares its inodes with the host, which is
// what makes it the one that matters.
//
// The second half is the capability set. sandbox.hpp:207-210:
//
//     bool __drop_all_caps = false;
//
// Under a user namespace the child is uid 0 with a full capability set in that namespace. It has
// CAP_FSETID, so `chmod u+s` on a file it owns is not stripped; it has CAP_SYS_ADMIN, so it can
// mount; it has CAP_MKNOD, so on a dev-honouring mount it can try for a device node. Every one of
// those is available in the default configuration.
//
// WHAT THIS PINS
//   1  a bind mount is MS_NOSUID          -- asked of the kernel, via statfs f_flags, not of the config
//   2  a bind mount is MS_NODEV
//   3  a set-user-ID bit set inside the jail is inert on that mount
//   4  the securebits are locked, so the credential drop cannot be walked back
//   5  capabilities are dropped by default
//   6  tmpfs and procfs stay correct     (they already are; this is the regression guard)
//
// POLARITY: inverted. Contracts 1-5 FAIL on the tree as it stands. Contract 6 passes today and is
// here so a fix that unifies the three mount paths cannot quietly weaken the two that were right.
//
// NEGATIVE CONTROL: contract 1 reads the flag back OUT OF THE KERNEL with statfs(2) rather than
// inspecting what the builder recorded. Those are different claims -- a mount_spec can carry
// MS_NOSUID and the mount can still fail to apply it, which is precisely what happens with
// MS_REMOUNT|MS_BIND and MS_REC (policy.hpp:53, and the reason __remount_ro routes through
// mount_setattr). Contract 3 then makes it behavioural: it sets the bit and requires the mount to
// refuse it. And contract 1 is bracketed by the same probe run against a mount that is NOT nosuid,
// so a statfs that always answered "nosuid" would be caught.
//
// CONTROL (ungated): a nosuid,nodev bind must still be readable, writable and EXECUTABLE. MS_NOEXEC
// is deliberately not asserted -- a sandbox that cannot execute the program it exists to run is not
// a fix -- and this control is what stops the obvious over-correction.
//
// Build:
//   duck test tests/adv/adv_cve_2026_15226_setuid_bind.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/sys/statfs.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace ns = micron::sec::ns;

namespace
{

constexpr const char *src_dir = "/var/tmp/mc_adv_15226_src";
constexpr const char *jail_root = "/var/tmp/mc_adv_15226";
constexpr const char *jail_old = "/var/tmp/mc_adv_15226/old";
constexpr const char *bind_at = "/var/tmp/mc_adv_15226/data";

// inside the jail, after the pivot
constexpr const char *inner_bind = "/data";
constexpr const char *inner_file = "/data/payload";

constexpr i32 not_nosuid = 71;
constexpr i32 not_nodev = 72;
constexpr i32 suid_bit_stuck = 73;
constexpr i32 caps_retained = 74;
constexpr i32 securebits_unlocked = 75;

bool
make_fixture(void)
{
  (void)mc::posix::mkdir(src_dir, 0700);
  (void)mc::posix::mkdir(jail_root, 0700);
  (void)mc::posix::mkdir(jail_old, 0700);
  (void)mc::posix::mkdir(bind_at, 0700);

  char f[160];
  adv::scratch_path(f, sizeof(f), "mc_adv_15226_src", "seed");
  const i32 fd = static_cast<i32>(mc::posix::open(f, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0700));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "SEED", 4);
  (void)mc::posix::close(fd);
  return true;
}

// Ask the KERNEL what the mount is, not the builder what it meant to say.
[[nodiscard]] bool
mount_has(const char *path, u64 bit)
{
  mc::posix::statfs_t st{};
  if ( mc::posix::statfs(path, st) < 0 ) return false;
  return (static_cast<u64>(st.f_flags) & bit) != 0;
}

i32
probe_bind_flags(void)
{
  if ( !mount_has(inner_bind, mc::posix::st_nosuid) ) return not_nosuid;
  if ( !mount_has(inner_bind, mc::posix::st_nodev) ) return not_nodev;
  return adv::ok_code;
}

// the snapd shape, end to end: make a file, make it setuid, and see whether the bit survives on a
// mount the host also sees
i32
probe_setuid_creation(void)
{
  const i32 fd = static_cast<i32>(mc::posix::open(inner_file, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0700));
  if ( fd < 0 ) return adv::setup_failed;
  (void)mc::posix::write(fd, "PAYLOAD", 7);
  (void)mc::posix::close(fd);

  // 04000 = S_ISUID. Under a user namespace we are uid 0 with CAP_FSETID, so this is not stripped.
  (void)mc::posix::chmod(inner_file, 04755);

  // The bit may well be recorded in the inode -- that is not the question. The question is whether
  // the MOUNT will honour it, because a nosuid mount makes the bit inert for every execve through
  // any path on that mount, here and on the host.
  if ( !mount_has(inner_bind, mc::posix::st_nosuid) ) return suid_bit_stuck;
  return adv::ok_code;
}

i32
probe_caps_and_securebits(void)
{
  // A default sandbox under a user namespace makes the child root IN THAT NAMESPACE. If the
  // bounding set was not narrowed, it holds CAP_FSETID (keep the setuid bit), CAP_SYS_ADMIN (mount
  // something else), CAP_MKNOD (device nodes) -- the whole toolkit this CVE class is built from.
  if ( mc::has_cap(mc::cap::fsetid) ) return caps_retained;
  if ( mc::has_cap(mc::cap::sys_admin) ) return caps_retained;
  if ( mc::has_cap(mc::cap::mknod) ) return caps_retained;
  if ( mc::has_cap(mc::cap::setuid) ) return caps_retained;

  // and the drop has to be one-way. Without SECBIT_NOROOT_LOCKED / NO_SETUID_FIXUP_LOCKED a process
  // that still holds CAP_SETPCAP can undo the securebits it just set.
  const i32 sb = mc::posix::cap_get_securebits();
  if ( sb < 0 ) return securebits_unlocked;
  constexpr i32 want = mc::posix::secbit_noroot | mc::posix::secbit_noroot_locked | mc::posix::secbit_no_setuid_fixup
                       | mc::posix::secbit_no_setuid_fixup_locked;
  if ( (sb & want) != want ) return securebits_unlocked;
  return adv::ok_code;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2026-15226 (setuid creation on a sandbox-writable mount) ===");

  sb::test_case("fixture");
  if ( !make_fixture() ) {
    sb::skip("cannot build the /var/tmp fixture in this environment");
    sb::print("=== ADV CVE-2026-15226 SKIPPED ===");
    return 1;
  }
  sb::require_true(mc::posix::geteuid() != 0);

  if ( !adv::have_userns() ) {
    sb::test_case("live jail cases");
    sb::skip("this kernel refuses an unprivileged user namespace; a bind-mounting jail cannot be "
             "built here, and every contract in this file is about the mount");
    sb::print("=== ADV CVE-2026-15226 SKIPPED ===");
    return 1;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // NEGATIVE CONTROL, first: statfs must be capable of answering "not nosuid"
  //
  // Every contract below reads st_nosuid out of statfs. If statfs answered "nosuid" for everything
  // -- a wrong struct layout, a wrong bit, a mount the kernel reported oddly -- all of them would
  // pass while observing nothing. /var/tmp on a stock box is suid,dev; that is the reading that
  // proves the probe has two possible answers.

  {
    sb::test_case("negative control: the statfs probe can distinguish a suid mount from a nosuid one");
    const bool host_nosuid = mount_has("/var/tmp", mc::posix::st_nosuid);
    sb::print("  /var/tmp reports nosuid=", host_nosuid ? "yes" : "no");
    if ( host_nosuid ) {
      sb::skip("/var/tmp is itself mounted nosuid on this box: the probe cannot be shown to "
               "discriminate here, so contracts 1-3 are reported but not proven non-vacuous");
    } else {
      sb::require_false(host_nosuid);
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1 + 2  the bind mount's flags, read back from the kernel

  {
    sb::test_case("a bind mount inside the jail must be MS_NOSUID and MS_NODEV");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.bind(src_dir, bind_at);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion(probe_bind_flags);
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == not_nosuid )
      sb::print("  the bind carries suid semantics from its source mount "
                "(sandbox.hpp:539 passes only MS_BIND|MS_REC)");
    if ( code == not_nodev ) sb::print("  the bind carries dev semantics from its source mount");
    sb::require(code, adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  the attack itself

  {
    sb::test_case("a set-user-ID file created inside the jail must be inert on that mount");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.bind(src_dir, bind_at);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion(probe_setuid_creation);
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == suid_bit_stuck ) sb::print("  chmod u+s inside the jail landed on a suid-honouring mount the host also sees");
    sb::require(code, adv::ok_code);

    // THE HOST-SIDE VIEW, AND WHAT A NOSUID BIND DOES NOT DO.
    //
    // Mount attributes are PER MOUNT, not per inode. MS_NOSUID on the bind governs access through
    // the bind; the host's own path to the same inode is a different mount and keeps its own flags.
    // So the setuid bit is still set, and still honoured, when the host opens the file by its own
    // name -- and no mount flag anywhere can change that.
    //
    // What makes it harmless is ownership, not the mount: the child is uid 0 only INSIDE its user
    // namespace, which maps to the unprivileged invoking uid outside it, so the file it created is
    // owned by the user who is already running the sandbox. A setuid binary owned by yourself grants
    // you nothing. That is the property to pin, and pinning the mount flag instead would have been
    // asserting something untrue about how mounts work.
    //
    // The case this does NOT cover, and which no part of src/sec can: a sandbox running as real root
    // with a bind of a host directory. There the created file is owned by root and the host path is
    // a live escalation. The answer there is not to bind a writable host directory into a root
    // sandbox, and this comment is the whole of the guidance micron can offer.
    char host_view[192];
    adv::scratch_path(host_view, sizeof(host_view), "mc_adv_15226_src", "payload");
    mc::posix::stat_t st{};
    if ( mc::posix::stat(host_view, st) >= 0 ) {
      const bool suid_on_host = (st.st_mode & 04000u) != 0;
      const bool owned_by_us = st.st_uid == mc::posix::getuid();
      if ( suid_on_host && !owned_by_us ) sb::print("  a setuid file owned by uid ", st.st_uid, " is live on the host at ", host_view);
      sb::require_true(owned_by_us);
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4 + 5  the capability half

  {
    sb::test_case("a default sandbox must drop capabilities and lock the securebits");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion(probe_caps_and_securebits);
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == caps_retained )
      sb::print("  the child kept CAP_FSETID / CAP_SYS_ADMIN / CAP_MKNOD in its user namespace "
                "(__drop_all_caps defaults to false, sandbox.hpp:209)");
    if ( code == securebits_unlocked )
      sb::print("  securebits are unset or unlocked: nothing in src/sec ever calls "
                "cap_set_securebits (capabilities.hpp:175 is dead code)");
    sb::require(code, adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated
  //
  // The over-correction this guards against is MS_NOEXEC. A sandbox exists to run something; a
  // "fix" that made every bind non-executable would break the feature while passing contracts 1-3.

  {
    sb::test_case("control: a nosuid,nodev bind is still readable, writable and executable");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.bind(src_dir, bind_at);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      const i32 rd = static_cast<i32>(mc::posix::open("/data/seed", mc::posix::o_rdonly, 0));
      if ( rd < 0 ) return adv::bad_code;
      (void)mc::posix::close(rd);
      const i32 wr = static_cast<i32>(mc::posix::open("/data/ctl", mc::posix::o_wronly | mc::posix::o_create, 0600));
      if ( wr < 0 ) return adv::bad_code;
      (void)mc::posix::close(wr);
      // the mount must NOT be noexec: a sandbox that cannot exec is not a sandbox
      mc::posix::statfs_t st{};
      if ( mc::posix::statfs("/data", st) < 0 ) return adv::setup_failed;
      constexpr u64 st_noexec = 0x0008;
      if ( (static_cast<u64>(st.f_flags) & st_noexec) != 0 ) return adv::bad_code;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  the two that were already right

  {
    sb::test_case("control: tmpfs and procfs keep their nosuid,nodev");
    char tdir[160];
    char pdir[160];
    adv::scratch_path(tdir, sizeof(tdir), "mc_adv_15226", "scratch");
    adv::scratch_path(pdir, sizeof(pdir), "mc_adv_15226", "proc");
    (void)mc::posix::mkdir(tdir, 0700);
    (void)mc::posix::mkdir(pdir, 0555);

    s::sandbox box;
    // procfs needs a pid namespace: an unprivileged user namespace may not mount a fresh procfs that
    // would report the HOST's pid namespace, so the kernel answers EPERM without one
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.tmpfs(tdir);
    box.procfs(pdir);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      if ( !mount_has("/scratch", mc::posix::st_nosuid) ) return not_nosuid;
      if ( !mount_has("/scratch", mc::posix::st_nodev) ) return not_nodev;
      if ( !mount_has("/proc", mc::posix::st_nosuid) ) return not_nosuid;
      if ( !mount_has("/proc", mc::posix::st_nodev) ) return not_nodev;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  sb::print("=== ADV CVE-2026-15226 PASSED ===");
  return 1;
}
