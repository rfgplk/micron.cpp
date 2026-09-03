//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2024-21626  --  runc, CVSS 8.6, CWE-403 (exposure of a resource to a wrong sphere)
// CVE-2019-5736   --  runc, CVSS 8.6, CWE-668  (the same descriptor, re-opened through /proc)
//
// 21626: "A leaked file descriptor could allow a container process to obtain a working directory in
//         the host filesystem namespace and enable container escape."   fixed: runc 1.1.12
// 5736:  "A malicious container could overwrite the host runc binary and potentially obtain
//         host-root code execution."
//
// THE SHAPE
//
// A jail built out of pivot_root is a statement about PATH RESOLUTION: from "/", nothing above the
// new root can be named. It is not a statement about descriptors. An open directory descriptor is
// its own resolution root, and openat(dirfd, "..") walks upward from wherever that directory
// actually is -- which, for a descriptor opened before the pivot, is the host.
//
// runc leaked exactly one such descriptor into the container's setup path. The container could
// fchdir() to it and then resolve relative paths in the host's namespace. The fix (1.1.12) was to
// close it, and to verify the working directory was not left outside.
//
// CVE-2019-5736 is the same descriptor viewed through /proc: with procfs mounted, /proc/self/fd/N
// re-opens the object behind descriptor N with FRESH flags -- so a read-only leak becomes a
// writable one, and a descriptor to the running binary becomes a way to overwrite it.
//
// MICRON'S ANALOGUE
//
// The stage order is right. sandbox.hpp:32-46 puts "descriptors" (close everything the child was
// not given) AFTER "filesystem" (pivot_root, chdir("/"), detach), which is the correct order --
// sweeping before the pivot would leave the pivot's own descriptors to be swept. And __close_extra
// (sandbox.hpp:224-279) is careful: close_range where available, a bounded loop where not, and
// -EOVERFLOW rather than a partial sweep it cannot finish.
//
// Two things undo it.
//
// (a) The sweep is OFF by default.
//
//         bool __close_others = false;                          // sandbox.hpp:210
//
//     So the documented way to build a sandbox -- namespaces, root, seccomp, run -- inherits every
//     descriptor the parent had open. That is CVE-2024-21626's precondition, arranged by default.
//
// (b) keep_fd() does not care what it is keeping.
//
//         sandbox & keep_fd(i32 fd) noexcept {                   // sandbox.hpp:643
//           if ( __keep_count >= 8 ) { __config_fail(...); return *this; }
//           __keep_fds[__keep_count++] = fd;
//
//     A kept regular-file descriptor is a channel, which is what the feature is for. A kept
//     DIRECTORY descriptor is a hole in the jail, and nothing distinguishes them.
//
// THE DIFFERENTIAL
//
// tests/rigor/sec_descriptors.cpp:179-198 already tests the leaked-dirfd case -- with the sweep
// explicitly turned ON. It proves __close_extra works. It cannot see (a), because it never runs the
// default configuration, and it never combines the leak with a pivot_root, so it asserts "the fd is
// gone" rather than "the host is unreachable through it". This file runs the default, and pivots.
//
// WHAT THIS PINS
//   1  the descriptor sweep is ON by default
//   2  a sandbox built the documented way does not inherit the parent's descriptors
//   3  a leaked dirfd does not survive INTO a pivot_root'd child  (the combined case)
//   4  ... and if one did, openat(dirfd, "..") would reach the host -- demonstrated, so 3 is not vacuous
//   5  keep_fd() refuses a directory descriptor
//   6  with procfs mounted, no surviving descriptor is re-openable through /proc/self/fd  (5736)
//   7  the child's working directory is inside the new root, not merely nominally "/"
//
// POLARITY: inverted. Contracts 1, 2, 3 and 5 FAIL on the tree as it stands -- __close_others is
// false and keep_fd takes anything. Contract 4 PASSES today and forever: it is the demonstration
// that the escape is real, and it is deliberately run in a sandbox with the sweep disabled so it
// keeps working after the default flips.
//
// NEGATIVE CONTROL: contract 4 IS the negative control, and it is live rather than a mutation log.
// It builds the same jail with close_extra_fds(false), leaks the same descriptor, and requires the
// escape to SUCCEED. If it ever stops succeeding, contract 3 has stopped being a claim about the
// sweep -- something else is closing the descriptor, and 3 would be passing for a reason nobody
// wrote down.
//
// CONTROL (ungated): keep_fd() of a regular file must keep working. It is how a sandbox is given a
// log, a socket, a pipe; a fix that closed those would make the feature useless.
//
// Build:
//   duck test tests/adv/adv_cve_2024_21626_leaked_dirfd.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace ns = micron::sec::ns;

namespace
{

// /var/tmp, not /tmp: the rigor suite made this choice for landlock inode reasons
// (sec_landlock.cpp:13) and two suites disagreeing about where fixtures live is how one test starts
// depending on another's leftovers.
constexpr const char *jail_root = "/var/tmp/mc_adv_21626";
constexpr const char *jail_old = "/var/tmp/mc_adv_21626/old";
constexpr const char *host_marker = "/var/tmp/mc_adv_21626_host_marker";

// the descriptor the parent leaks. A fixed number keeps the child's arithmetic trivial under a
// filter that may deny fcntl.
constexpr i32 leaked = 90;

// child verdicts beyond adv_kit's
constexpr i32 escaped_via_dirfd = 61;
constexpr i32 escaped_via_procfd = 62;
constexpr i32 cwd_outside = 63;

bool
make_fixture(void)
{
  (void)mc::posix::mkdir(jail_root, 0700);
  (void)mc::posix::mkdir(jail_old, 0700);

  // a file only reachable from OUTSIDE the jail. Reaching it from inside is the escape.
  const i32 fd = static_cast<i32>(mc::posix::open(host_marker, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0600));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "HOST", 4);
  (void)mc::posix::close(fd);

  // and one only reachable from inside, so a probe that finds nothing anywhere is distinguishable
  // from a probe that is correctly confined
  char inside[128];
  adv::scratch_path(inside, sizeof(inside), "mc_adv_21626", "inside.txt");
  const i32 in = static_cast<i32>(mc::posix::open(inside, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0600));
  if ( in < 0 ) return false;
  (void)mc::posix::write(in, "IN", 2);
  (void)mc::posix::close(in);

  // the jail must be a mount point of its own for pivot_root to accept it
  return true;
}

// Open /var/tmp as a directory and park it on `leaked`. This is the runc descriptor: a directory
// ABOVE the new root, held open across the pivot.
[[nodiscard]] bool
leak_host_dirfd(void)
{
  const i32 d = static_cast<i32>(mc::posix::open("/var/tmp", mc::posix::o_rdonly | mc::posix::o_directory, 0));
  if ( d < 0 ) return false;
  if ( d != leaked ) {
    if ( mc::posix::dup2(d, leaked) < 0 ) {
      (void)mc::posix::close(d);
      return false;
    }
    (void)mc::posix::close(d);
  }
  // deliberately NOT cloexec and deliberately not closed: that is the defect being modelled
  return true;
}

// the child body: try every way back out through descriptor `leaked`
i32
escape_probe(void)
{
  // 1  is the descriptor even there?
  if ( mc::posix::fcntl(leaked, mc::posix::f_getfd) < 0 ) return adv::ok_code;      // swept: nothing to escape with

  // 2  it is. openat() relative to it resolves in the HOST namespace, above the jail root.
  const i32 f = mc::posix::openat(leaked, "mc_adv_21626_host_marker", mc::posix::o_rdonly, 0);
  if ( f >= 0 ) {
    char b[8]{};
    const auto n = mc::posix::read(f, b, sizeof(b) - 1);
    (void)mc::posix::close(f);
    if ( n >= 4 && b[0] == 'H' ) return escaped_via_dirfd;
  }

  // 3  and ".." from it walks further up still
  const i32 up = mc::posix::openat(leaked, "..", mc::posix::o_rdonly | mc::posix::o_directory, 0);
  if ( up >= 0 ) {
    const i32 etc = mc::posix::openat(up, "etc/hostname", mc::posix::o_rdonly, 0);
    (void)mc::posix::close(up);
    if ( etc >= 0 ) {
      (void)mc::posix::close(etc);
      return escaped_via_dirfd;
    }
  }

  // the descriptor survived but did not resolve anywhere useful. Still a leak, still graded bad --
  // "the escape did not happen to work here" is not confinement.
  return escaped_via_dirfd;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2024-21626 / 2019-5736 (leaked descriptor as a resolution root) ===");

  sb::test_case("fixture");
  if ( !make_fixture() ) {
    sb::skip("cannot build the /var/tmp fixture in this environment");
    sb::print("=== ADV CVE-2024-21626 SKIPPED ===");
    return 1;
  }
  // the whole claim is about UNPRIVILEGED confinement
  sb::require_true(mc::posix::geteuid() != 0);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  the default

  // Asked BEHAVIOURALLY, not by reading __close_others. The flag is private, and more to the point
  // a test that reads the field asserts what the builder recorded rather than what the child got --
  // and those are two different claims. This one runs a child.
  {
    sb::test_case("the descriptor sweep must be ON by default");
    if ( !adv::have_userns() ) {
      sb::skip("this kernel refuses an unprivileged user namespace; a sandbox cannot be built here");
    } else {
      const i32 probe = static_cast<i32>(mc::posix::open(host_marker, mc::posix::o_rdonly, 0));
      sb::require_true(probe >= 0);

      s::sandbox box;
      box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
      sb::require_true(box.configured());

      const auto r = box.run_to_completion([]() -> i32 {
        for ( i32 i = 3; i < 256; ++i )
          if ( mc::posix::fcntl(i, mc::posix::f_getfd) >= 0 ) return adv::bad_code;
        return adv::ok_code;
      });
      sb::require_true(r.is_first());
      if ( r.cast<s::sandbox::exit_status>().code() == adv::bad_code )
        sb::print("  a default-configured sandbox inherited a parent descriptor "
                  "(__close_others defaults to false, sandbox.hpp:210)");
      sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);

      (void)mc::posix::close(probe);
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  keep_fd must reject a directory

  {
    sb::test_case("keep_fd() must refuse a directory descriptor");
    const i32 d = static_cast<i32>(mc::posix::open("/var/tmp", mc::posix::o_rdonly | mc::posix::o_directory, 0));
    sb::require_true(d >= 0);
    sb::require_true(adv::fd_is_dir(d));

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.keep_fd(d);
    if ( box.configured() )
      sb::print("  keep_fd() accepted a directory descriptor: a dirfd is a resolution root, and "
                "openat() through it ignores the jail root entirely");
    sb::require_false(box.configured());
    sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::descriptors));

    (void)mc::posix::close(d);
  }

  {
    sb::test_case("control: keep_fd() of a regular file must still work");
    // this is what the feature is for -- a log, a socket, a pipe handed to the confined body. A fix
    // that closed these would make keep_fd useless, which is not a fix.
    const i32 f = static_cast<i32>(mc::posix::open(host_marker, mc::posix::o_rdonly, 0));
    sb::require_true(f >= 0);
    sb::require_false(adv::fd_is_dir(f));

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.keep_fd(f);
    sb::require_true(box.configured());
    (void)mc::posix::close(f);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  THE NEGATIVE CONTROL -- run first, because 3 means nothing without it
  //
  // Build the jail with the sweep explicitly OFF, leak the descriptor, and require the escape to
  // SUCCEED. This is the proof that contract 3 below is observing the sweep and not some accident.

  if ( !adv::have_userns() ) {
    sb::test_case("live jail cases");
    sb::skip("this kernel refuses an unprivileged user namespace; the jail cannot be built here");
    sb::print("=== ADV CVE-2024-21626 PASSED (live half skipped) ===");
    return 1;
  }

  {
    sb::test_case("negative control: with the sweep off, a leaked dirfd DOES reach the host");
    sb::require_true(leak_host_dirfd());

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    box.close_extra_fds(false);      // the pre-fix default, stated explicitly
    sb::require_true(box.configured());

    const auto r = box.run_to_completion(escape_probe);
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    sb::print("  sweep off -> child reports ", code, code == escaped_via_dirfd ? "  (escaped, as it must)" : "");
    sb::require(code, escaped_via_dirfd);

    (void)mc::posix::close(leaked);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  and now the claim: the DEFAULT jail seals it

  {
    sb::test_case("a default-configured jail must not carry a leaked dirfd into the child");
    sb::require_true(leak_host_dirfd());

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    // NOTHING else. This is the documented way to build a jail, and it is the configuration the
    // finding is about.
    sb::require_true(box.configured());

    const auto r = box.run_to_completion(escape_probe);
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == escaped_via_dirfd ) sb::print("  the leaked /var/tmp descriptor survived into the jail and resolved the host marker");
    sb::require(code, adv::ok_code);

    (void)mc::posix::close(leaked);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  and nothing else came through either

  {
    sb::test_case("a default-configured jail inherits no descriptor above 2");
    // three ordinary descriptors, none of them kept
    const i32 a = static_cast<i32>(mc::posix::open(host_marker, mc::posix::o_rdonly, 0));
    const i32 b = static_cast<i32>(mc::posix::open("/var/tmp", mc::posix::o_rdonly | mc::posix::o_directory, 0));
    sb::require_true(a >= 0 && b >= 0);

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // fds 0/1/2 are the sandbox's own business; anything above them is inheritance
      for ( i32 i = 3; i < 256; ++i )
        if ( mc::posix::fcntl(i, mc::posix::f_getfd) >= 0 ) return adv::bad_code;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);

    (void)mc::posix::close(a);
    (void)mc::posix::close(b);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  CVE-2019-5736: the same descriptor, re-opened through /proc
  //
  // /proc/self/fd/N is not a symlink in the ordinary sense -- opening it re-opens the underlying
  // object with whatever flags you ask for. A descriptor that survives the sweep is therefore not
  // merely readable at the access it was opened with; it is a fresh open of the host object.

  {
    sb::test_case("with procfs mounted, no inherited descriptor is re-openable through /proc/self/fd");
    sb::require_true(leak_host_dirfd());

    char proc_dir[128];
    adv::scratch_path(proc_dir, sizeof(proc_dir), "mc_adv_21626", "proc");
    (void)mc::posix::mkdir(proc_dir, 0555);

    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid);
    box.bind(jail_root, jail_root);
    box.procfs(proc_dir);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // if the sweep did its job the descriptor is gone and /proc/self/fd/90 does not exist
      const i32 f = static_cast<i32>(mc::posix::open("/proc/self/fd/90", mc::posix::o_rdonly, 0));
      if ( f < 0 ) return adv::ok_code;
      (void)mc::posix::close(f);
      return escaped_via_procfd;
    });
    sb::require_true(r.is_first());
    if ( r.cast<s::sandbox::exit_status>().code() == escaped_via_procfd )
      sb::print("  /proc/self/fd/90 re-opened an inherited host descriptor from inside the jail");
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);

    (void)mc::posix::close(leaked);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 7  the working directory
  //
  // The other half of runc's fix. chdir("/") after pivot_root is necessary but says nothing on its
  // own -- "/" is whatever the process's root is. The question is whether "/" is the JAIL.

  {
    sb::test_case("the child's working directory is inside the new root");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::mount);
    box.bind(jail_root, jail_root);
    box.root(jail_root, jail_old);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // the marker that exists only inside the jail must be reachable relative to cwd...
      const i32 in = mc::posix::openat(mc::posix::at_fdcwd, "inside.txt", mc::posix::o_rdonly, 0);
      if ( in < 0 ) return adv::setup_failed;
      (void)mc::posix::close(in);
      // ...and the one that exists only outside must not be, by any number of dot-dots
      const i32 out1 = mc::posix::openat(mc::posix::at_fdcwd, "../mc_adv_21626_host_marker", mc::posix::o_rdonly, 0);
      if ( out1 >= 0 ) return cwd_outside;
      const i32 out2 = mc::posix::openat(mc::posix::at_fdcwd, "../../../../etc/hostname", mc::posix::o_rdonly, 0);
      if ( out2 >= 0 ) return cwd_outside;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  sb::print("=== ADV CVE-2024-21626 PASSED ===");
  return 1;
}
