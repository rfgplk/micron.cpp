//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2019-5736  --  runc, CVSS 8.6, CWE-668 (exposure to a wrong sphere)
//
// "A malicious container could overwrite the host runc binary and potentially obtain host-root code
//  execution."
//
// THE SHAPE
//
// procfs is not a directory of names, it is a directory of HANDLES. /proc/<pid>/exe and
// /proc/<pid>/fd/<n> are magic links: opening one does not resolve a path, it re-opens the object
// the kernel is holding -- with whatever flags the opener asks for, from wherever the opener is,
// regardless of mount namespace or chroot.
//
// runc exec'd itself inside the container to do setup work. For the moment that process was alive,
// /proc/self/exe inside the container was a handle on the HOST's runc binary. A malicious image
// replaced the entrypoint with a symlink to /proc/self/exe, waited for runc to open it, and wrote
// through the handle. The next container start ran the attacker's code as host root.
//
// Two properties of the magic link make this work and both matter here:
//   * it ignores the mount namespace -- the jail's root is irrelevant to what the handle points at
//   * it grants FRESH access -- a descriptor opened O_RDONLY is re-openable O_RDWR through /proc
//
// MICRON'S ANALOGUE
//
// sandbox has an exec path (stage 13, sandbox.hpp:445-451) and a procfs mount
// (sandbox.hpp:554-560), so the ingredients are present. The question is what is reachable through
// /proc once the child is in the jail, and it comes down to what descriptors survive stage 5.
//
// The relevant default is the same one CVE-2024-21626 turns on:
//
//     bool __close_others = false;                          // sandbox.hpp:210
//
// With the sweep off, every descriptor the parent had open is in the child's table -- and with
// procfs mounted, each of them is re-openable through /proc/self/fd/<n> at any access the child
// asks for. A read-only handle on a host file becomes a writable one. adv_cve_2024_21626 covers the
// DIRECTORY case (a resolution root); this file covers the ACCESS-UPGRADE case, which is a
// different property with a different fix and needs its own assertions.
//
// /proc/self/exe deserves separate treatment because it is not swept by anything: it exists as long
// as procfs is mounted, and it points at whatever binary the child is running -- which, before an
// execve, is the sandbox's own host binary.
//
// WHAT THIS PINS
//   1  procfs mounted in a jail is nosuid,nodev,noexec       (holds today; regression guard)
//   2  no inherited descriptor is re-openable through /proc/self/fd
//   3  ... and specifically not at a HIGHER access than it was opened with  (the upgrade)
//   4  /proc/self/exe is not writable from inside the jail
//   5  /proc/self/exe does not resolve to a path the jail can also reach by name
//   6  a jail with no procfs has no /proc at all                (the simplest containment)
//
// POLARITY: inverted. Contracts 2 and 3 FAIL on the tree as it stands, for the same root cause as
// adv_cve_2024_21626 -- the sweep default. 1, 4, 5, 6 pass today and are regression guards.
//
// NEGATIVE CONTROL: contract 3 first demonstrates the upgrade against a jail with the sweep
// explicitly OFF -- opening a read-only inherited handle for WRITING through /proc and requiring it
// to succeed. Only then does it require the default configuration to seal it. Without that half,
// "the open failed" could mean procfs was not mounted.
//
// CONTROL (ungated): a jail WITH procfs must still be able to read its own /proc entries. procfs is
// mounted because something needs it; a fix that made /proc unreadable would break the reason it is
// there.
//
// Build:
//   duck test tests/adv/adv_cve_2019_5736_procexe.cpp -o bin/adv --timeout 120 -f

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

constexpr const char *jail_root = "/var/tmp/mc_adv_5736";
constexpr const char *jail_old = "/var/tmp/mc_adv_5736/old";
constexpr const char *jail_proc = "/var/tmp/mc_adv_5736/proc";
constexpr const char *host_file = "/var/tmp/mc_adv_5736_host.txt";

// a fixed descriptor for the inherited read-only handle
constexpr i32 leaked = 91;

constexpr i32 reopened = 141;
constexpr i32 upgraded = 142;
constexpr i32 exe_writable = 143;
constexpr i32 proc_present = 144;
constexpr i32 proc_unreadable = 145;
constexpr i32 mount_flags_wrong = 146;

bool
make_fixture(void)
{
  (void)mc::posix::mkdir(jail_root, 0700);
  (void)mc::posix::mkdir(jail_old, 0700);
  (void)mc::posix::mkdir(jail_proc, 0555);
  const i32 fd = static_cast<i32>(mc::posix::open(host_file, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0600));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "HOSTDATA", 8);
  (void)mc::posix::close(fd);
  return true;
}

// Leave a READ-ONLY handle on a host file in the child's table. Read-only is the point: the upgrade
// is what /proc gives you, and a handle that was already writable would prove nothing.
[[nodiscard]] bool
leak_readonly_handle(void)
{
  const i32 d = static_cast<i32>(mc::posix::open(host_file, mc::posix::o_rdonly, 0));
  if ( d < 0 ) return false;
  if ( d != leaked ) {
    if ( mc::posix::dup2(d, leaked) < 0 ) {
      (void)mc::posix::close(d);
      return false;
    }
    (void)mc::posix::close(d);
  }
  return true;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2019-5736 (procfs magic links ignore the jail) ===");

  sb::test_case("fixture");
  if ( !make_fixture() ) {
    sb::skip("cannot build the /var/tmp fixture in this environment");
    sb::print("=== ADV CVE-2019-5736 SKIPPED ===");
    return 1;
  }
  sb::require_true(mc::posix::geteuid() != 0);

  if ( !adv::have_userns() ) {
    sb::test_case("live jail cases");
    sb::skip("this kernel refuses an unprivileged user namespace; the jail cannot be built here");
    sb::print("=== ADV CVE-2019-5736 SKIPPED ===");
    return 1;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  the procfs mount itself

  {
    sb::test_case("procfs in a jail is nosuid, nodev and noexec");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      mc::posix::statfs_t st{};
      if ( mc::posix::statfs("/proc", st) < 0 ) return adv::setup_failed;
      constexpr u64 st_noexec = 0x0008;
      const u64 f = static_cast<u64>(st.f_flags);
      if ( (f & mc::posix::st_nosuid) == 0 ) return mount_flags_wrong;
      if ( (f & mc::posix::st_nodev) == 0 ) return mount_flags_wrong;
      if ( (f & st_noexec) == 0 ) return mount_flags_wrong;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  NEGATIVE CONTROL FIRST -- demonstrate the access upgrade
  //
  // A read-only handle, re-opened for WRITING through /proc/self/fd. This is the whole of
  // CVE-2019-5736 in one open() call, and it has to be shown working before "it does not work" means
  // anything.

  {
    sb::test_case("negative control: with the sweep off, /proc/self/fd UPGRADES a read-only handle");
    sb::require_true(leak_readonly_handle());

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    box.close_extra_fds(false);      // the pre-fix default, said out loud
    box.keep_fd(leaked);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // the inherited handle is read-only. Writing THROUGH it must fail...
      if ( mc::posix::write(leaked, "X", 1) >= 0 ) return adv::setup_failed;
      // ...and re-opening it through /proc must not care
      const i32 up = static_cast<i32>(mc::posix::open("/proc/self/fd/91", mc::posix::o_wronly, 0));
      if ( up < 0 ) return adv::ok_code;
      const auto n = mc::posix::write(up, "PWNED!!!", 8);
      (void)mc::posix::close(up);
      return n > 0 ? upgraded : reopened;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    sb::print("  sweep off -> /proc/self/fd/91 reopened for writing: ",
              code == upgraded ? "YES, and the write landed on the host file" : "no");
    sb::require(code, upgraded);

    (void)mc::posix::close(leaked);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2 + 3  and the default must seal it

  {
    sb::test_case("a default-configured jail leaves nothing re-openable through /proc/self/fd");
    sb::require_true(leak_readonly_handle());

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    // nothing else: the documented way
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // every descriptor above stdio, through the magic link, at the highest access we can ask for
      for ( i32 i = 3; i < 256; ++i ) {
        char p[32];
        adv::scratch_path(p, sizeof(p), "", "");      // reset
        // build "/proc/self/fd/<i>" without allocating
        usize k = 0;
        const char pre[] = "/proc/self/fd/";
        for ( usize j = 0; j + 1 < sizeof(pre); ++j ) p[k++] = pre[j];
        if ( i >= 100 ) p[k++] = static_cast<char>('0' + (i / 100));
        if ( i >= 10 ) p[k++] = static_cast<char>('0' + ((i / 10) % 10));
        p[k++] = static_cast<char>('0' + (i % 10));
        p[k] = '\0';

        const i32 f = static_cast<i32>(mc::posix::open(p, mc::posix::o_rdonly, 0));
        if ( f >= 0 ) {
          (void)mc::posix::close(f);
          return reopened;
        }
      }
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == reopened ) sb::print("  an inherited descriptor was re-openable through /proc/self/fd");
    sb::require(code, adv::ok_code);

    (void)mc::posix::close(leaked);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4 + 5  /proc/self/exe
  //
  // Not swept by anything -- it exists for as long as procfs is mounted. Before the child execve's,
  // it is a handle on the SANDBOX's own binary, which lives on the host outside the jail.

  {
    sb::test_case("/proc/self/exe is not writable from inside the jail");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // ETXTBSY is the kernel refusing to open a running binary for writing. That is a real defence
      // and it is the one runc was relying on -- what defeated runc was doing this from a process
      // that was NOT the one running the binary. Either way, it must not succeed here.
      const i32 w = static_cast<i32>(mc::posix::open("/proc/self/exe", mc::posix::o_wronly, 0));
      if ( w >= 0 ) {
        (void)mc::posix::close(w);
        return exe_writable;
      }
      const i32 rw = static_cast<i32>(mc::posix::open("/proc/self/exe", mc::posix::o_rdwr, 0));
      if ( rw >= 0 ) {
        (void)mc::posix::close(rw);
        return exe_writable;
      }
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == exe_writable ) sb::print("  /proc/self/exe opened for writing from inside the jail");
    sb::require(code, adv::ok_code);
  }

  {
    sb::test_case("/proc/self/exe does not resolve to anything nameable inside the jail");
    // The handle points at the host binary. readlink() reports the host PATH, which the jail cannot
    // resolve -- so a readlink that produced a name the jail CAN open would mean the binary is
    // inside the jail, and writable by whatever wrote the jail.
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      char buf[512]{};
      const auto n = mc::posix::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
      if ( n <= 0 ) return adv::ok_code;      // cannot even read the link: fine
      buf[n] = '\0';
      // the reported path must not be openable for writing by name from in here
      const i32 f = static_cast<i32>(mc::posix::open(buf, mc::posix::o_wronly, 0));
      if ( f >= 0 ) {
        (void)mc::posix::close(f);
        return exe_writable;
      }
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  the simplest containment of all

  {
    sb::test_case("a jail with no procfs has no /proc");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    // note: no .procfs()
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      if ( static_cast<i32>(mc::posix::open("/proc/self/exe", mc::posix::o_rdonly, 0)) >= 0 ) return proc_present;
      if ( static_cast<i32>(mc::posix::open("/proc/self/fd/0", mc::posix::o_rdonly, 0)) >= 0 ) return proc_present;
      if ( static_cast<i32>(mc::posix::open("/proc/self/maps", mc::posix::o_rdonly, 0)) >= 0 ) return proc_present;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == proc_present ) sb::print("  /proc is reachable in a jail that never mounted it");
    sb::require(code, adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: a jail WITH procfs can still read its own /proc entries");
    // procfs is mounted because something wants it. A fix that made /proc unreadable would satisfy
    // contracts 2-5 by removing the feature.
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(jail_proc);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      const i32 st = static_cast<i32>(mc::posix::open("/proc/self/stat", mc::posix::o_rdonly, 0));
      if ( st < 0 ) return proc_unreadable;
      (void)mc::posix::close(st);
      const i32 mp = static_cast<i32>(mc::posix::open("/proc/self/maps", mc::posix::o_rdonly, 0));
      if ( mp < 0 ) return proc_unreadable;
      (void)mc::posix::close(mp);
      // and its own stdio, which is what /proc/self/fd is legitimately for
      const i32 own = static_cast<i32>(mc::posix::open("/proc/self/fd/1", mc::posix::o_wronly, 0));
      if ( own < 0 ) return proc_unreadable;
      (void)mc::posix::close(own);
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == proc_unreadable ) sb::print("  a jail with procfs mounted cannot read its own /proc");
    sb::require(code, adv::ok_code);
  }

  sb::print("=== ADV CVE-2019-5736 PASSED ===");
  return 1;
}
