//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2019-10063  --  Flatpak, CVSS 9.0, CWE-693 (protection mechanism failure)
//
// "A seccomp-based TIOCSTI protection bypass could allow sandboxed applications to inject commands
// outside the sandbox."
//
// THE SHAPE
//
// TIOCSTI ("terminal ioctl: simulate terminal input") pushes a byte into the input queue of a
// terminal the caller has a descriptor to. A sandbox that inherits the launching shell's tty on fd
// 0 can therefore type into that shell -- outside the sandbox, with the launcher's privileges.
// Every serious sandbox blocks it, and Flatpak did:
//
//     seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(ioctl), 1,
//                      SCMP_A1(SCMP_CMP_EQ, (int)TIOCSTI));
//
// That rule is bypassable and the reason is an ABI seam, not a logic error. seccomp compares the
// full 64-bit register the argument arrived in. The kernel's ioctl entry point does not:
//
//     SYSCALL_DEFINE3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg)
//                                          ^^^^^^^^^^^^^^^^
//
// `cmd` is an `unsigned int`. The high 32 bits are discarded before any handler sees them. So
// ioctl(fd, 0x1'0000'5412) is TIOCSTI to the kernel and is NOT 0x5412 to the filter. The fix was
// to compare the low word only:
//
//     SCMP_A1(SCMP_CMP_MASKED_EQ, 0xFFFFFFFF, (int)TIOCSTI)
//
// MICRON'S ANALOGUE -- TWO OF THEM, AND THE SECOND IS THE LIVE ONE
//
// (a) The mechanism is reachable here in exactly one keystroke. src/sec/seccomp.hpp:140-180 offers
//     arg_eq/arg_ne/arg_lt/... as the obvious comparators and arg_masked() as an afterthought, and
//     emit_pred() lowers arg_eq into a full 64-bit compare (seccomp.hpp:345-351) -- correctly, which
//     is the problem. `arg_eq(1, posix::tiocsti)` is what anyone writes and it is the Flatpak bug.
//     Nothing in the header says which syscall arguments the kernel narrows.
//
// (b) micron never gets as far as a bypassable rule, because it has no rule. src/sec/groups.hpp:48:
//
//         struct baseline {
//           static constexpr i32 calls[] = {
//             SYS_exit, SYS_exit_group, SYS_rt_sigreturn, SYS_brk, SYS_munmap, SYS_futex,
//             SYS_sysinfo, SYS_set_tid_address, SYS_set_robust_list, SYS_rseq, SYS_getrandom,
//             SYS_close,
//             SYS_ioctl,          <-- unfiltered, in the group every policy starts from
//             ...
//
//     and sandbox::stdio() (sandbox.hpp:361-375) exists specifically to wire the caller's
//     descriptors onto 0/1/2, which for an interactive launcher is the tty. baseline + stdio is the
//     documented way to build a sandbox here, and it hands the confined process TIOCSTI.
//
// THE DIFFERENTIAL
//
// Against Flatpak post-1.0.8: it denies TIOCSTI with a masked compare. Against micron today: no
// deny at all, and the comparator a reader would reach for to write one is the pre-fix Flatpak
// form. This file pins both halves, because fixing only (b) leaves the next caller to rediscover
// (a) on fcntl, prctl, personality or keyctl -- every one of which narrows its selector the same way.
//
// WHAT THIS PINS
//   1  arg_eq on a narrowed argument IS bypassable -- demonstrated, not asserted
//   2  the masked form is not, for the same inputs
//   3  the two disagree, so (1) is a property of the comparator and not of the probe
//   4  no shipped syscall group may reach TIOCSTI unfiltered
//   5  a filter built from the shipped groups the documented way must not permit TIOCSTI
//   6  the other tty-injection commands (TIOCLINUX, TIOCSETD) are covered by the same rule
//   7  live: with a real tty and a real filter, the injection does not land
//
// POLARITY: inverted. Contracts 4 and 5 FAIL on the tree as it stands -- that failure IS the
// finding -- and pass once SYS_ioctl leaves groups::baseline. Contracts 1-3 pass today and forever;
// they document the seam so the fix for 4/5 is not written with arg_eq.
//
// NEGATIVE CONTROL: self-contained and live. Contract 1 executes the bypass and observes
// SECCOMP_RET_ALLOW; contract 2 runs the identical probe through the masked comparator and observes
// the deny. Neither is a claim about a mutation I made elsewhere -- the divergence is in this file,
// on every run, and if the encoder ever started narrowing arg_eq silently, contract 3 goes red.
//
// CONTROL (ungated): a filter that denies TIOCSTI must still pass TCGETS, which is what isatty()
// issues (io/term, ioctl.hpp:230). A "fix" that denies ioctl outright would break every micron
// program that prints to a terminal, and is not a fix.
//
// Build:
//   duck test tests/adv/adv_cve_2019_10063_tiocsti.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/sys/ioctl.hpp"
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

constexpr u64 tiocsti = mc::posix::tiocsti;          // 0x5412
constexpr u64 tioclinux = mc::posix::tioclinux;      // 0x541c
constexpr u64 tcgets = mc::posix::tcgets;            // 0x5401

// the high-bit twin: identical to the kernel, invisible to a 64-bit compare
constexpr u64 tiocsti_hi = tiocsti | (u64(1) << 32);
constexpr u64 tioclinux_hi = tioclinux | (u64(1) << 32);

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

// the whole low word: what the kernel actually reads out of the cmd register
constexpr u64 narrow_mask = 0xFFFFFFFFull;

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2019-10063 (flatpak TIOCSTI seccomp bypass) ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  the pre-fix Flatpak rule, built with micron's comparators, and driven

  u32 eq_low = 0;
  u32 eq_high = 0;
  {
    sb::test_case("arg_eq(1, TIOCSTI): the exact rule CVE-2019-10063 was filed against");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_ioctl, sc::arg_eq(1, tiocsti), eperm);
    fb.default_allow();
    sb::require_true(fb.valid());

    eq_low = adv::filter_action(fb, SYS_ioctl, 0, tiocsti);
    eq_high = adv::filter_action(fb, SYS_ioctl, 0, tiocsti_hi);

    // the rule does what it says for the value it names
    sb::require(eq_low, sc::act_errno(eperm));

    // ...and nothing at all for the value the kernel treats identically
    sb::print("  ioctl(fd, 0x5412)        -> ", eq_low == sc::act_allow() ? "ALLOW" : "deny");
    sb::print("  ioctl(fd, 0x1'0000'5412) -> ", eq_high == sc::act_allow() ? "ALLOW  <-- the bypass" : "deny");
    sb::require(eq_high, sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  the post-fix form: mask to the width the kernel reads

  u32 masked_low = 0;
  u32 masked_high = 0;
  {
    sb::test_case("arg_masked(1, 0xFFFFFFFF, TIOCSTI): the 1.0.8 fix, expressed here");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tiocsti), eperm);
    fb.default_allow();
    sb::require_true(fb.valid());

    masked_low = adv::filter_action(fb, SYS_ioctl, 0, tiocsti);
    masked_high = adv::filter_action(fb, SYS_ioctl, 0, tiocsti_hi);

    sb::require(masked_low, sc::act_errno(eperm));
    sb::require(masked_high, sc::act_errno(eperm));

    // and every other high-word decoration, not just bit 32
    for ( u32 sh = 32; sh < 64; ++sh ) {
      const u64 decorated = tiocsti | (u64(1) << sh);
      sb::require(adv::filter_action(fb, SYS_ioctl, 0, decorated), sc::act_errno(eperm));
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3  the divergence is the point

  {
    sb::test_case("the two comparators must DISAGREE on the decorated value");
    // if this ever stops being true, one of two things happened: emit_pred started narrowing arg_eq
    // (which would be a silent semantic change to every existing arg_eq user), or the oracle stopped
    // executing the program. Either way contracts 1 and 2 would have gone vacuous without saying so.
    sb::require_distinct(eq_high, masked_high);
    sb::require(eq_low, masked_low);      // they agree on the undecorated value; only the seam differs
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  no shipped group may hand out unfiltered ioctl
  //
  // THIS IS THE FINDING. groups::baseline is the group every policy in the tree starts from --
  // policy.hpp:27 shows it in the header comment, sec_seccomp_live.cpp:49 uses it as "the runtime's
  // own needs". Naming SYS_ioctl there means every sandbox built the documented way can reach
  // TIOCSTI on whatever tty stdio() wired to fd 0.

  {
    sb::test_case("groups::baseline must not name SYS_ioctl");
    const bool named = adv::group_names<g::baseline>(SYS_ioctl);
    if ( named ) sb::print("  groups::baseline names SYS_ioctl (groups.hpp:48) -- unfiltered ioctl in the base group");
    sb::require_false(named);
  }

  {
    sb::test_case("no shipped group may name SYS_ioctl unfiltered");
    // io/ipc/io_multiplexing legitimately want fds; none of them wants raw ioctl either. A group
    // that needs terminal control should name the commands, not the syscall.
    sb::require_false(adv::group_names<g::io>(SYS_ioctl));
    sb::require_false(adv::group_names<g::filesystem>(SYS_ioctl));
    sb::require_false(adv::group_names<g::ipc>(SYS_ioctl));
    sb::require_false(adv::group_names<g::io_multiplexing>(SYS_ioctl));
    sb::require_false(adv::group_names<g::network>(SYS_ioctl));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  the assembled policy, driven

  {
    sb::test_case("a policy built from the shipped groups must not permit TIOCSTI");
    using jail = s::seccomp_policy<s::allow<g::baseline, g::io>>;
    auto fb = jail::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());

    const u32 a_sti = adv::filter_action(fb, SYS_ioctl, 0, tiocsti);
    const u32 a_sti_hi = adv::filter_action(fb, SYS_ioctl, 0, tiocsti_hi);
    const u32 a_lin = adv::filter_action(fb, SYS_ioctl, 0, tioclinux);
    const u32 a_lin_hi = adv::filter_action(fb, SYS_ioctl, 0, tioclinux_hi);

    sb::print("  allow<baseline,io> vs TIOCSTI   -> ", a_sti == sc::act_allow() ? "ALLOW  <-- injection reachable" : "denied");
    sb::print("  allow<baseline,io> vs TIOCLINUX -> ", a_lin == sc::act_allow() ? "ALLOW  <-- injection reachable" : "denied");

    sb::require_distinct(a_sti, sc::act_allow());
    sb::require_distinct(a_sti_hi, sc::act_allow());
    sb::require_distinct(a_lin, sc::act_allow());
    sb::require_distinct(a_lin_hi, sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  the rest of the injection family
  //
  // TIOCSTI is the famous one; it is not the only one. TIOCLINUX subcommand 2 does a console
  // selection paste, and TIOCSETD swaps the line discipline out from under the terminal. A deny
  // list that names only TIOCSTI is the same defect one command along.

  {
    sb::test_case("a tty-injection deny must cover the family, not just TIOCSTI");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tiocsti), eperm);
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tioclinux), eperm);
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, mc::posix::tiocsetd), eperm);
    fb.default_allow();
    sb::require_true(fb.valid());

    sb::require(adv::filter_action(fb, SYS_ioctl, 0, tiocsti), sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_ioctl, 0, tioclinux), sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_ioctl, 0, mc::posix::tiocsetd), sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_ioctl, 0, tiocsti_hi), sc::act_errno(eperm));
    sb::require(adv::filter_action(fb, SYS_ioctl, 0, tioclinux_hi), sc::act_errno(eperm));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated, both halves of the tree
  //
  // A fix that also breaks conforming input is not a fix. micron's own print layer calls
  // ioctl(fd, TCGETS) to decide whether to line-buffer (ioctl.hpp:230); a policy that denies
  // TIOCSTI must leave that alone, or every program that writes to a terminal loses its buffering
  // decision -- and under a KILL default, its life.

  {
    sb::test_case("control: denying the injection family must leave TCGETS (isatty) working");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tiocsti), eperm);
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tioclinux), eperm);
    fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, mc::posix::tiocsetd), eperm);
    fb.allow(SYS_ioctl);
    fb.default_errno(eperm);
    sb::require_true(fb.valid());

    sb::require(adv::filter_action(fb, SYS_ioctl, 1, tcgets), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_ioctl, 0, tiocsti), sc::act_errno(eperm));
    // ordering matters and this is where it shows: the denies were emitted first, so the blanket
    // allow(SYS_ioctl) below them cannot resurrect them. Reversed, it would.
  }

  {
    sb::test_case("control: an allowlist that never mentions ioctl still runs the runtime");
    using jail = s::seccomp_policy<s::allow<g::baseline>>;
    auto fb = jail::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());
    // the calls micron's own startup and teardown issue must survive whatever happens to ioctl
    sb::require(adv::filter_action(fb, SYS_write, 1, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_read, 0, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_exit_group, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_futex, 0), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 7  live
  //
  // dev.tty.legacy_tiocsti defaults to 0 since 6.2, which makes the kernel refuse TIOCSTI outright
  // and this half meaningless. The filter assertions above are the ones that carry the claim; this
  // is the confirmation when the box can give it.

  {
    sb::test_case("live: TIOCSTI through a real filter on a real descriptor");
    if ( !adv::legacy_tiocsti_enabled() ) {
      sb::skip("dev.tty.legacy_tiocsti = 0: the kernel refuses TIOCSTI regardless of any filter");
    } else if ( !mc::posix::isatty(0) ) {
      sb::skip("fd 0 is not a terminal; nothing to inject into");
    } else {
      const adv::child_result r = adv::run_child([]() -> i32 {
        sc::filter_builder<128> fb;
        fb.require_native_arch();
        fb.deny_if_errno(SYS_ioctl, sc::arg_masked(1, narrow_mask, tiocsti), eperm);
        for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
        for ( usize i = 0; i < g::signal::count; ++i ) fb.allow(g::signal::calls[i]);
        fb.default_errno(eperm);
        if ( !fb.valid() ) return adv::setup_failed;
        if ( sc::load(fb, true) < 0 ) return adv::setup_failed;

        char c = 'x';
        // both spellings must be refused; the decorated one is the CVE
        if ( mc::posix::ioctl(0, tiocsti, &c) >= 0 ) return adv::bad_code;
        if ( mc::posix::ioctl(0, tiocsti_hi, &c) >= 0 ) return adv::bad_code;
        return adv::ok_code;
      });
      if ( r.g == adv::grade::bad ) sb::print("  TIOCSTI landed through the filter");
      sb::require_true(r.ok());
    }
  }

  sb::print("=== ADV CVE-2019-10063 PASSED ===");
  return 1;
}
