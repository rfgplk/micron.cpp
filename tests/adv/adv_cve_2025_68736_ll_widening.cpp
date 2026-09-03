//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2025-68736  --  Linux Landlock, CVSS 8.8, CWE-281 (improper preservation of permissions)
//
// "Improper handling of disconnected directories could widen Landlock access rights and permit
//  access beyond intended sandbox policy."
//     fixed: 6.6.143, 6.12.80, 6.18.2, 6.19
//
// THE SHAPE
//
// Landlock is a whitelist expressed as (directory, rights) pairs, and the directory is named by a
// descriptor. The kernel bug was that for a *disconnected* directory -- one whose path to the root
// had been removed out from under it -- the rights check resolved to something wider than the
// policy said. The class the CVE belongs to is broader than the one kernel path: it is
//
//     THE DIRECTORY THE RULE ENDS UP ON IS NOT THE DIRECTORY THE POLICY NAMED.
//
// A kernel bug is one way to get there. Following a symlink while resolving the rule's path is
// another, and that one is entirely userspace's fault.
//
// MICRON'S ANALOGUE -- TWO, AND THE SECOND IS THE LARGER
//
// (a) Rule paths are resolved with symlinks followed. src/sec/landlock.hpp:261-275:
//
//         [[nodiscard]] i32 allow(const char *path, access_fs allowed) noexcept {
//           ...
//           const i32 pf = static_cast<i32>(posix::open(path, posix::o_path | posix::o_cloexec, 0));
//
//     No O_NOFOLLOW, no O_DIRECTORY, no openat2/RESOLVE_BENEATH -- and micron already has all three
//     (posix::o_nofollow, posix::o_directory, posix::openat2 with resolve_no_symlinks at
//     linux/io/sys.hpp:695-713). If any component of `path` is a symlink, the rule is installed on
//     whatever it points at. The policy reads "grant read on /app/data"; the sandbox grants read on
//     wherever /app/data was pointing when the ruleset was built.
//
// (b) `handled` defaults to the union of what was GRANTED. sandbox.hpp:564-571:
//
//         sandbox & landlock(const char *path, landlock::access_fs access) noexcept {
//           ...
//           if ( !__ll_handled_explicit ) __ll_handled |= access;
//
//     Landlock restricts only what the ruleset says it HANDLES; anything unhandled is unrestricted
//     everywhere, in the whole filesystem. So
//
//         box.landlock("/usr", landlock::read_only);
//
//     handles exactly read_file|read_dir. write_file, truncate, remove_file, remove_dir, every
//     make_*, execute, refer and ioctl_dev are all left completely unrestricted -- which is the
//     opposite of what "landlock this path read-only" reads like, and it is the DEFAULT.
//     landlock_handled_all() (sandbox.hpp:581) is the correct spelling and is opt-in.
//
// THE DIFFERENTIAL
//
// tests/rigor/sec_landlock.cpp:110-130 confines to a subtree and checks the outside is EACCES --
// through the ruleset API directly, where the caller states `handled` itself. It never goes through
// sandbox::landlock(), so it cannot see (b). And every path it hands to allow() is a real directory
// with no symlinked component, so it cannot see (a).
//
// WHAT THIS PINS
//   1  a symlinked component does not silently relocate a rule    -- demonstrated, then required
//   2  allow() refuses a path whose final component is a symlink
//   3  allow() refuses a non-directory
//   4  a sandbox told "read-only here" does not leave write unrestricted EVERYWHERE
//   5  ... nor truncate, nor the make_* family, nor remove_*
//   6  an ABI too old for a requested right is reported, not silently dropped
//
// POLARITY: inverted. Contracts 1-5 FAIL on the tree as it stands. Contract 6 fails because
// try_ruleset narrows silently (landlock.hpp:306-308) and only errors when NOTHING survives.
//
// NEGATIVE CONTROL: contract 1 first proves the symlink relocation is real by building the rule and
// showing the OUTSIDE directory became reachable -- in a child, live. Only then does it require the
// refusal. Without that half, "allow() returned an error" could mean the fixture was broken.
//
// CONTROL (ungated): a legitimate deep path with no symlinked component must still install, and a
// read-only confinement must still permit reads. A fix that made allow() reject ordinary paths, or
// that handled everything and granted nothing, would pass 1-5 and be useless.
//
// Build:
//   duck test tests/adv/adv_cve_2025_68736_ll_widening.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/sec/landlock.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace ll = micron::sec::landlock;
namespace ns = micron::sec::ns;

namespace
{

constexpr const char *base = "/var/tmp/mc_adv_68736";
constexpr const char *inside = "/var/tmp/mc_adv_68736/in";
constexpr const char *outside = "/var/tmp/mc_adv_68736/out";
constexpr const char *link_to_out = "/var/tmp/mc_adv_68736/in_link";      // -> out
constexpr const char *inside_file = "/var/tmp/mc_adv_68736/in/ok.txt";
constexpr const char *outside_file = "/var/tmp/mc_adv_68736/out/secret.txt";
constexpr const char *elsewhere = "/var/tmp/mc_adv_68736_elsewhere.txt";

constexpr i32 wrote_outside = 81;
constexpr i32 truncated_outside = 82;
constexpr i32 made_outside = 83;
constexpr i32 removed_outside = 84;
constexpr i32 reached_via_symlink = 85;

bool
touch(const char *p, const char *what)
{
  const i32 fd = static_cast<i32>(mc::posix::open(p, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0600));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, what, 4);
  (void)mc::posix::close(fd);
  return true;
}

bool
make_fixture(void)
{
  (void)mc::posix::mkdir(base, 0700);
  (void)mc::posix::mkdir(inside, 0700);
  (void)mc::posix::mkdir(outside, 0700);
  (void)mc::posix::unlink(link_to_out);
  // the relocation: a name inside the confined tree that resolves outside it
  if ( mc::posix::symlink("out", link_to_out) < 0 && !adv::path_exists(link_to_out) ) return false;
  if ( !touch(inside_file, "IN__") ) return false;
  if ( !touch(outside_file, "OUT_") ) return false;
  if ( !touch(elsewhere, "ELSE") ) return false;
  return true;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2025-68736 (landlock rights wider than the policy names) ===");

  sb::test_case("fixture");
  if ( !make_fixture() ) {
    sb::skip("cannot build the /var/tmp fixture in this environment");
    sb::print("=== ADV CVE-2025-68736 SKIPPED ===");
    return 1;
  }
  sb::require_true(mc::posix::geteuid() != 0);

  if ( !adv::have_landlock(1) ) {
    sb::test_case("landlock cases");
    sb::skip("landlock is not available on this kernel (abi <= 0); every contract here is about a "
             "landlock ruleset");
    sb::print("=== ADV CVE-2025-68736 SKIPPED ===");
    return 1;
  }
  sb::print("  landlock abi = ", adv::landlock_abi());

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  NEGATIVE CONTROL FIRST: prove the symlink actually relocates the rule

  {
    sb::test_case("negative control: a rule added on a symlink DOES scope to its target");
    const adv::child_result r = adv::run_child([]() -> i32 {
      // handle everything, grant read only through the symlinked name
      ll::ruleset rs = ll::try_ruleset(ll::full_dir | ll::access_fs::execute);
      if ( !rs.valid() ) return adv::setup_failed;
      if ( rs.allow(link_to_out, ll::read_only) < 0 ) return adv::unsupported;      // already refused: good
      if ( rs.restrict_self() < 0 ) return adv::setup_failed;

      // if the rule landed on `out`, the file under `out` is readable and nothing else is
      const i32 f = static_cast<i32>(mc::posix::open(outside_file, mc::posix::o_rdonly, 0));
      if ( f >= 0 ) {
        (void)mc::posix::close(f);
        return reached_via_symlink;
      }
      return adv::ok_code;
    });
    if ( r.g == adv::grade::unsupported )
      sb::print("  allow() already refuses a symlinked path; the relocation cannot be demonstrated");
    else if ( r.code == reached_via_symlink )
      sb::print("  confirmed: naming the symlink granted rights on its TARGET, not on the name");
    // either outcome is informative; what must not happen is a fixture failure
    sb::require_distinct(r.g, adv::grade::setup);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2 + 3  and so allow() must not resolve one

  {
    sb::test_case("ruleset::allow() must refuse a path whose final component is a symlink");
    ll::ruleset rs = ll::try_ruleset(ll::read_only);
    sb::require_true(rs.valid());
    const i32 got = rs.allow(link_to_out, ll::read_only);
    if ( got >= 0 )
      sb::print("  allow() followed the symlink (landlock.hpp:269 opens O_PATH|O_CLOEXEC with no "
                "O_NOFOLLOW): the rule is on ",
                outside, ", not on the name the policy gave");
    sb::require_true(got < 0);
  }

  {
    sb::test_case("ruleset::allow() must refuse a non-directory");
    // a rule on a regular file is not meaningful for a path_beneath rule, and accepting one hides
    // the caller's mistake rather than reporting it
    ll::ruleset rs = ll::try_ruleset(ll::read_only);
    sb::require_true(rs.valid());
    sb::require_true(rs.allow(inside_file, ll::read_only) < 0);
  }

  {
    sb::test_case("control: an ordinary directory path still installs");
    // the assertion that stops "refuse everything" from looking like a fix
    ll::ruleset rs = ll::try_ruleset(ll::read_only);
    sb::require_true(rs.valid());
    sb::require_true(rs.allow(inside, ll::read_only) >= 0);
    sb::require_true(rs.allow(base, ll::read_only) >= 0);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4 + 5  the default `handled` mask
  //
  // This is the larger of the two findings. It needs no symlink, no kernel bug and no unusual
  // configuration -- just the documented way to ask a sandbox for a read-only path.

  if ( !adv::have_userns() ) {
    sb::test_case("sandbox landlock cases");
    sb::skip("this kernel refuses an unprivileged user namespace; sandbox::run cannot be exercised here");
    sb::print("=== ADV CVE-2025-68736 PASSED (sandbox half skipped) ===");
    return 1;
  }

  {
    sb::test_case("box.landlock(path, read_only) must not leave write unrestricted everywhere");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user);
    box.landlock(inside, ll::read_only);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // a path the policy never mentioned, outside the confined subtree entirely
      const i32 w = static_cast<i32>(mc::posix::open(elsewhere, mc::posix::o_wronly | mc::posix::o_trunc, 0));
      if ( w >= 0 ) {
        (void)mc::posix::write(w, "PWND", 4);
        (void)mc::posix::close(w);
        return wrote_outside;
      }
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == wrote_outside )
      sb::print("  wrote to ", elsewhere,
                " -- write_file was never HANDLED, so landlock does not restrict it anywhere "
                "(sandbox.hpp:568 defaults handled to the union of what was granted)");
    sb::require(code, adv::ok_code);
  }

  {
    sb::test_case("... nor truncate, make_* or remove_*");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user);
    box.landlock(inside, ll::read_only);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      if ( mc::posix::truncate(elsewhere, 0) >= 0 ) return truncated_outside;
      if ( mc::posix::mkdir("/var/tmp/mc_adv_68736_made", 0700) >= 0 ) {
        (void)mc::posix::rmdir("/var/tmp/mc_adv_68736_made");
        return made_outside;
      }
      if ( mc::posix::unlink(outside_file) >= 0 ) return removed_outside;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == truncated_outside ) sb::print("  truncate() outside the confined tree succeeded");
    if ( code == made_outside ) sb::print("  mkdir() outside the confined tree succeeded");
    if ( code == removed_outside ) sb::print("  unlink() outside the confined tree succeeded");
    sb::require(code, adv::ok_code);
  }

  {
    sb::test_case("control: a read-only confinement must still permit reads inside it");
    // the over-correction guard for 4/5: handling everything and granting nothing would pass both
    s::sandbox box;
    box.namespaces(ns::ns_kind::user);
    box.landlock(inside, ll::read_only);
    box.landlock(base, ll::read_only);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      const i32 f = static_cast<i32>(mc::posix::open(inside_file, mc::posix::o_rdonly, 0));
      if ( f < 0 ) return adv::bad_code;
      char b[8]{};
      const auto n = mc::posix::read(f, b, sizeof(b) - 1);
      (void)mc::posix::close(f);
      return n >= 4 && b[0] == 'I' ? adv::ok_code : adv::bad_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  silent ABI narrowing
  //
  // try_ruleset intersects the requested rights with what the running ABI supports and only fails
  // when NOTHING survives. Asking for `truncate` (abi 3) or `ioctl_dev` (abi 5) on an older kernel
  // therefore returns a VALID ruleset that does not restrict the thing you asked about, and the only
  // way to notice is to re-read rs.handled() and compare -- which no caller does.

  {
    sb::test_case("a right the running ABI cannot express is reportable, not silently dropped");
    const i32 abi = adv::landlock_abi();
    constexpr ll::access_fs impossible = static_cast<ll::access_fs>(u64(1) << 40);

    // The DEFAULT posture is still `narrow`, and that is deliberate: intersecting with the ABI mask
    // is what lets one binary run across kernels, and changing it would break every existing caller
    // on an older box. What was missing is any way to FIND OUT, so both halves are pinned here.

    // (a) narrow still narrows -- and now says what it dropped
    ll::ruleset lenient = ll::try_ruleset(ll::read_only | impossible);
    sb::require_true(lenient.valid());
    const ll::access_fs lost = ll::dropped_fs(ll::read_only | impossible, lenient);
    sb::print("  abi ", abi, ": narrow posture dropped ", any(lost) ? "a requested right, and reports it" : "nothing");
    sb::require_true(any(lost));
    sb::require_true(any(lost & impossible));

    // (b) strict refuses outright, which is what a caller wants when the right IS the defence
    ll::ruleset firm = ll::try_ruleset(ll::read_only | impossible, ll::access_net::none, ll::scope::none, ll::abi_policy::strict);
    if ( firm.valid() )
      sb::print("  strict posture accepted a right this kernel cannot express: try_ruleset narrows to "
                "the ABI mask and only errors when NOTHING survives");
    sb::require_false(firm.valid());

    // (c) and strict must NOT refuse a request the kernel can honour, or it is unusable
    ll::ruleset ok = ll::try_ruleset(ll::read_only, ll::access_net::none, ll::scope::none, ll::abi_policy::strict);
    sb::require_true(ok.valid());
    sb::require_false(any(ll::dropped_fs(ll::read_only, ok)));
  }

  sb::print("=== ADV CVE-2025-68736 PASSED ===");
  return 1;
}
