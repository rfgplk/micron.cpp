//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2019-9893  --  libseccomp, CVSS 9.8, CWE-693
//
// "Incorrect seccomp filters for certain 64-bit argument comparisons could allow intended syscall
//  restrictions to be bypassed."                          affected: libseccomp < 2.4.0
//
// THE SHAPE
//
// classic BPF has a 32-bit accumulator. seccomp_data.args[] is 64 bits. So every argument
// comparison a filter generator emits is a LADDER: load the high word, compare, branch; load the
// low word, compare, branch. There is no other way to do it, and it is where filter generators go
// wrong, because the ladder's shape differs per operator and each shape has its own set of
// forward-skip counts.
//
// libseccomp got one of those ladders wrong. The failure mode is the one that matters: the program
// still assembles, still passes the kernel's load-time verifier, still returns an action for every
// input -- and classifies some inputs wrong. Nothing about the emitted instruction array reveals it.
// An argument that should have been denied was allowed.
//
// MICRON'S ANALOGUE
//
// src/sec/seccomp.hpp:338-414, emit_pred(). Seven operators, seven different ladders, sizes 5/5/6/6/
// 7/7/7 declared separately in pred_insns() (:267-284) and consumed by action_if() as the skip count
// over the whole block (:578). Every one of those numbers is an opportunity for this CVE.
//
// I read all seven by hand and they are correct: both halves are compared for every operator, the
// MSW/LSW offsets are endian-aware (:194-217), and pred_insns matches the emitted length exactly.
// This file exists anyway, for two reasons. First, the libjkr rule -- every CVE gets a test whether
// or not we are vulnerable, because a passing test is the permanent regression guard. Second,
// tests/rigor/sec_bpf_encode.cpp:189-231 already runs a 36,000-evaluation differential against the
// oracle, but it draws its arguments from a seeded RNG. Random draws are not the same as pinning
// the exact shapes a half-comparison lets through, and this CVE is precisely about those shapes.
//
// THE DIFFERENTIAL
//
// sec_bpf_encode.cpp asks "does the lowering agree with the predicate on 36,000 sampled points".
// This file asks "does it agree on the six points where a broken lowering MUST differ", for every
// operator, and then proves the question has teeth by running a deliberately broken lowering
// through the same oracle and requiring it to be caught.
//
// The six shapes, for a comparand V with halves (Vh, Vl):
//   A  arg == V                        both halves match
//   B  (Vh, Vl ^ 1)                    high matches, low does not      <- catches "compared high only"
//   C  (Vh ^ 1, Vl)                    low matches, high does not      <- catches "compared low only"
//   D  (Vh ^ 1, Vl ^ 1)                neither matches
//   E  V + 1, V - 1                    the ordering boundary
//   F  V ^ (1 << 32)                   the word boundary itself
//
// WHAT THIS PINS
//   1  all seven operators agree with a 64-bit C++ predicate on all six shapes
//   2  ... for a set of comparands chosen to sit ON the word boundary, not near it
//   3  a lowering that compares only the low word IS caught             (negative control)
//   4  a lowering that compares only the high word IS caught            (negative control)
//   5  a lowering with an off-by-one skip count IS caught               (negative control)
//   6  pred_insns() equals the number of instructions emit_pred actually emits, per operator
//   7  every emitted program passes the kernel's own structural verifier
//
// POLARITY: this one passes on the current tree and is a permanent regression guard. It FAILS the
// moment emit_pred, pred_insns, or the MSW/LSW offsets are touched wrongly.
//
// NEGATIVE CONTROL: contracts 3-5, and they are not a mutation log -- the broken lowerings are
// written out in this file, emitted through the same bpf primitives, and run through the same
// oracle. If the harness could not tell a broken ladder from a correct one, those three go red.
//
// CONTROL (ungated): the arch gate and the syscall dispatch above the predicate must still work --
// a different syscall number must never reach the predicate block at all. A "fix" that made every
// argument comparison deny everything would satisfy 1-2 and be useless.
//
// Build:
//   duck test tests/adv/adv_cve_2019_9893_arg64_split.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"
#include "../support/sec_oracle.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace so = sec_oracle;
namespace b = micron::bpf;

namespace
{

constexpr i32 probe_nr = SYS_getpgid;
constexpr i32 other_nr = SYS_getsid;      // must never reach the predicate
constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

struct op_case {
  sc::cmp op;
  so::cmp_kind kind;
  const char *name;
  usize size;
};

constexpr op_case ops[] = {
  { sc::cmp::eq, so::cmp_kind::eq, "eq", 5 },
  { sc::cmp::ne, so::cmp_kind::ne, "ne", 5 },
  { sc::cmp::lt, so::cmp_kind::lt, "lt", 7 },
  { sc::cmp::le, so::cmp_kind::le, "le", 7 },
  { sc::cmp::gt, so::cmp_kind::gt, "gt", 6 },
  { sc::cmp::ge, so::cmp_kind::ge, "ge", 6 },
  { sc::cmp::masked_eq, so::cmp_kind::masked, "masked_eq", 7 },
};

// Comparands that sit ON the 32-bit word boundary rather than near it. A generator that compared
// one half only would agree with the truth for most values; these are the ones where it cannot.
constexpr u64 comparands[] = {
  0x0000'0000'0000'0000ull,      // both halves zero
  0x0000'0000'0000'0001ull,      // low only
  0x0000'0000'FFFF'FFFFull,      // low saturated, high zero
  0x0000'0001'0000'0000ull,      // high only -- invisible to a low-word compare
  0x0000'0001'0000'0001ull,      // both halves set
  0xFFFF'FFFF'0000'0000ull,      // high saturated, low zero
  0x7FFF'FFFF'FFFF'FFFFull,      // signed max
  0x8000'0000'0000'0000ull,      // sign bit
  0xFFFF'FFFF'FFFF'FFFFull,      // all ones
  0x0000'0000'8000'0000ull,      // low-word sign bit -- the classic sign-extension trap
};

// the six shapes, derived from a comparand
void
shapes_of(u64 v, u64 (&out)[8])
{
  const u64 hi = v & 0xFFFF'FFFF'0000'0000ull;
  const u64 lo = v & 0x0000'0000'FFFF'FFFFull;
  out[0] = v;                                      // A  both match
  out[1] = hi | (lo ^ 1ull);                       // B  high matches, low does not
  out[2] = (hi ^ (1ull << 32)) | lo;               // C  low matches, high does not
  out[3] = (hi ^ (1ull << 32)) | (lo ^ 1ull);      // D  neither
  out[4] = v + 1ull;                               // E  ordering boundary
  out[5] = v - 1ull;                               // E
  out[6] = v ^ (1ull << 32);                       // F  the word boundary itself
  out[7] = v ^ (1ull << 31);                       // F' the top of the LOW word
}

// build a one-rule filter around `ac` and hand back the action for a given argument
template<usize N>
u32
drive(sc::filter_builder<N> &fb, i32 nr, u64 arg)
{
  auto p = fb.prog();
  so::probe pr{};
  pr.nr = nr;
  pr.arch = static_cast<u32>(sc::native_arch);
  pr.args[0] = arg;
  return so::run(p.filter, p.len, pr).action;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// THE NEGATIVE CONTROLS
//
// Three lowerings that are wrong in exactly the ways CVE-2019-9893 was wrong. They are emitted from
// the same bpf primitives the real encoder uses and run through the same oracle, so if the harness
// could not distinguish a broken ladder from a correct one, these would agree with the predicate and
// the contracts above would be worthless.

enum class broken { low_only, high_only, off_by_one };

// an `eq` predicate lowered wrongly. The correct one is seccomp.hpp:345-351.
usize
emit_broken_eq(b::insn_t *out, broken how, u64 datum)
{
  const u32 hi = sc::msw(datum);
  const u32 lo = sc::lsw(datum);
  usize n = 0;
  switch ( how ) {
  case broken::low_only:
    // the CVE shape: never look at the high word at all
    out[n++] = sc::load_arg_lsw(0);
    out[n++] = sc::jeq_k(lo, 0, 1);
    out[n++] = sc::ret_k(sc::act_errno(eperm));
    break;
  case broken::high_only:
    out[n++] = sc::load_arg_msw(0);
    out[n++] = sc::jeq_k(hi, 0, 1);
    out[n++] = sc::ret_k(sc::act_errno(eperm));
    break;
  case broken::off_by_one:
    // Both halves ARE compared. The only defect is that the low-word mismatch branch skips one too
    // FEW, landing on the deny return instead of past it.
    //
    // This is the shape that matters, and it is worth being precise about why. An off-by-one that
    // skips one too MANY lands past the end of the program, and the kernel's load-time verifier
    // rejects that outright -- a loud failure, at install time, that nobody ships. One too few
    // stays in range: every jump target is valid, the program ends in a return, seccomp(2) accepts
    // it without complaint, and it answers some inputs wrong for the life of the process. That is
    // CVE-2019-9893's actual failure mode and it is the one this control has to model.
    out[n++] = sc::load_arg_lsw(0);
    out[n++] = sc::jeq_k(lo, 0, 2);      // correct is 3
    out[n++] = sc::load_arg_msw(0);
    out[n++] = sc::jeq_k(hi, 0, 1);
    out[n++] = sc::ret_k(sc::act_errno(eperm));
    break;
  }
  return n;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2019-9893 (64-bit argument comparison lowered to a 32-bit machine) ===");

  usize checks = 0;

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3 + 4 + 5  NEGATIVE CONTROLS FIRST -- prove the method can see a broken ladder

  {
    sb::test_case("negative control: a low-word-only lowering is caught");
    // 0x1_0000_0000 differs from 0 only in the HIGH word, so a low-only compare says "equal"
    b::insn_t prog[16]{};
    usize n = 0;
    prog[n++] = sc::load_arch();
    prog[n++] = sc::jeq_k(static_cast<u32>(sc::native_arch), 1, 0);
    prog[n++] = sc::ret_k(mc::posix::seccomp_ret_kill_process);
    prog[n++] = sc::load_syscall_nr();
    prog[n++] = sc::jeq_k(static_cast<u32>(probe_nr), 0, 3);
    n += emit_broken_eq(prog + n, broken::low_only, 0ull);
    prog[n++] = sc::ret_k(sc::act_allow());
    sb::require(static_cast<i32>(so::verify(prog, n)), static_cast<i32>(so::fault::none));

    so::probe pr{};
    pr.arch = static_cast<u32>(sc::native_arch);
    pr.nr = probe_nr;
    pr.args[0] = u64(1) << 32;      // NOT equal to 0
    const u32 got = so::run(prog, n, pr).action;
    const bool truth = so::predicate(so::cmp_kind::eq, pr.args[0], 0ull);
    sb::print("  low-only lowering on arg=2^32 vs 0: says ", got == sc::act_errno(eperm) ? "MATCH" : "no match", ", truth says ",
              truth ? "match" : "no match");
    sb::require_false(truth);
    sb::require(got, sc::act_errno(eperm));      // the broken one matches; that is the bug, reproduced
  }

  {
    sb::test_case("negative control: a high-word-only lowering is caught");
    b::insn_t prog[16]{};
    usize n = 0;
    prog[n++] = sc::load_arch();
    prog[n++] = sc::jeq_k(static_cast<u32>(sc::native_arch), 1, 0);
    prog[n++] = sc::ret_k(mc::posix::seccomp_ret_kill_process);
    prog[n++] = sc::load_syscall_nr();
    prog[n++] = sc::jeq_k(static_cast<u32>(probe_nr), 0, 3);
    n += emit_broken_eq(prog + n, broken::high_only, 0ull);
    prog[n++] = sc::ret_k(sc::act_allow());
    sb::require(static_cast<i32>(so::verify(prog, n)), static_cast<i32>(so::fault::none));

    so::probe pr{};
    pr.arch = static_cast<u32>(sc::native_arch);
    pr.nr = probe_nr;
    pr.args[0] = 1ull;      // differs from 0 only in the LOW word
    sb::require_false(so::predicate(so::cmp_kind::eq, pr.args[0], 0ull));
    sb::require(so::run(prog, n, pr).action, sc::act_errno(eperm));
  }

  {
    sb::test_case("negative control: an off-by-one skip count is caught");
    b::insn_t prog[16]{};
    usize n = 0;
    prog[n++] = sc::load_arch();
    prog[n++] = sc::jeq_k(static_cast<u32>(sc::native_arch), 1, 0);
    prog[n++] = sc::ret_k(mc::posix::seccomp_ret_kill_process);
    prog[n++] = sc::load_syscall_nr();
    prog[n++] = sc::jeq_k(static_cast<u32>(probe_nr), 0, 5);
    n += emit_broken_eq(prog + n, broken::off_by_one, 0ull);
    prog[n++] = sc::ret_k(sc::act_allow());

    // THE KERNEL WOULD TAKE THIS PROGRAM. Every jump is in range, it ends in a return, and
    // seccomp(2) would install it without a word. That is the whole danger of this CVE class: a
    // wrong skip count is not a load-time error, it is a wrong answer that ships.
    sb::require(static_cast<i32>(so::verify(prog, n)), static_cast<i32>(so::fault::none));

    so::probe pr{};
    pr.arch = static_cast<u32>(sc::native_arch);
    pr.nr = probe_nr;

    // an exact match still denies -- the defect is not on this path
    pr.args[0] = 0ull;
    sb::require_true(so::predicate(so::cmp_kind::eq, 0ull, 0ull));
    sb::require(so::run(prog, n, pr).action, sc::act_errno(eperm));

    // a LOW-word mismatch is where the misplaced skip shows: truth says no match, so the correct
    // answer is the trailing allow. The broken ladder falls onto the deny return instead.
    pr.args[0] = 1ull;
    sb::require_false(so::predicate(so::cmp_kind::eq, 1ull, 0ull));
    const u32 got = so::run(prog, n, pr).action;
    sb::print("  off-by-one lowering on arg=1 vs 0: says ", got == sc::act_errno(eperm) ? "MATCH" : "no match", ", truth says no match");
    sb::require(got, sc::act_errno(eperm));      // the broken one disagrees; that is the bug, reproduced

    // and the correct encoder, on the identical question, must not
    sc::filter_builder<64> good;
    good.require_native_arch();
    good.deny_if_errno(probe_nr, sc::arg_eq(0, 0ull), eperm);
    good.default_allow();
    sb::require(drive(good, probe_nr, 1ull), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  pred_insns() must equal what emit_pred emits
  //
  // action_if() uses pred_insns() as the jf skip over the whole predicate block (seccomp.hpp:578).
  // If the two ever disagree, a non-matching syscall lands INSIDE the predicate -- reading a
  // half-loaded accumulator and returning whatever that compares to.

  {
    sb::test_case("pred_insns() equals the emitted block length, per operator");
    for ( const op_case &oc : ops ) {
      sc::filter_builder<64> fb;
      fb.require_native_arch();
      const usize before = fb.count;
      fb.action_if(probe_nr, sc::arg_cmp_t{ 0, oc.op, 0x1234'5678'9ABC'DEF0ull, 0xFFFF'FFFF'FFFF'FFFFull }, sc::act_errno(eperm));
      const usize emitted = fb.count - before;
      // 2 for the load-nr + jeq dispatch, then the predicate block
      sb::require(emitted, oc.size + 2u);
      sb::require(sc::pred_insns(oc.op), oc.size);
      ++checks;
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1 + 2 + 7  the claim: every operator, every comparand, every shape

  {
    sb::test_case("every operator agrees with a 64-bit predicate on every word-boundary shape");
    usize disagreements = 0;

    for ( const op_case &oc : ops ) {
      for ( const u64 v : comparands ) {
        // masked_eq needs a mask that actually spans the boundary, or the case degenerates
        const u64 mask = (oc.op == sc::cmp::masked_eq) ? 0xFFFF'FFFF'FFFF'FFFFull : 0ull;

        sc::filter_builder<64> fb;
        fb.require_native_arch();
        fb.action_if(probe_nr, sc::arg_cmp_t{ 0, oc.op, v, mask }, sc::act_errno(eperm));
        fb.default_allow();
        sb::require_true(fb.valid());

        // 7: the kernel's own verifier must accept it
        auto pg = fb.prog();
        sb::require(static_cast<i32>(so::verify(pg.filter, pg.len)), static_cast<i32>(so::fault::none));

        u64 sh[8];
        shapes_of(v, sh);
        for ( const u64 a : sh ) {
          const u32 got = drive(fb, probe_nr, a);
          const bool want = so::predicate(oc.kind, a, v, mask);
          const u32 expect = want ? sc::act_errno(eperm) : sc::act_allow();
          if ( got != expect ) {
            sb::print("    DISAGREE ", oc.name, " comparand=", v, " arg=", a);
            ++disagreements;
          }
          ++checks;
        }
      }
    }
    if ( disagreements != 0 ) sb::print("  ", disagreements, " lowering/predicate disagreements");
    sb::require(disagreements, static_cast<usize>(0));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: a different syscall never reaches the predicate block");
    // the jf skip over the predicate is the other half of pred_insns being right. If it is wrong,
    // an unrelated syscall falls into a half-built comparison.
    for ( const op_case &oc : ops ) {
      const u64 mask = (oc.op == sc::cmp::masked_eq) ? 0xFFFF'FFFF'FFFF'FFFFull : 0ull;
      sc::filter_builder<64> fb;
      fb.require_native_arch();
      fb.action_if(probe_nr, sc::arg_cmp_t{ 0, oc.op, 0ull, mask }, sc::act_errno(eperm));
      fb.default_allow();
      // whatever the argument, another syscall must fall straight through to the default
      for ( const u64 v : comparands ) {
        sb::require(drive(fb, other_nr, v), sc::act_allow());
        ++checks;
      }
    }
  }

  {
    sb::test_case("control: the arch gate still fires ahead of everything");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.action_if(probe_nr, sc::arg_eq(0, 0ull), sc::act_errno(eperm));
    fb.default_allow();
    auto pg = fb.prog();
    so::probe pr{};
    pr.arch = 0xDEAD'BEEFu;      // not this machine
    pr.nr = probe_nr;
    sb::require(so::run(pg.filter, pg.len, pr).action, mc::posix::seccomp_ret_kill_process);
  }

  sb::print("  lowering/predicate comparisons checked: ", checks);
  sb::print("=== ADV CVE-2019-9893 PASSED ===");
  return 1;
}
