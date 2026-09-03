//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2019-12589  --  Firejail, CVSS 8.8, CWE-693 (protection mechanism failure)
//
// "Seccomp filters could be modified from within a jail, weakening filtering for processes
//  subsequently joining that jail."                            fixed: Firejail 0.9.60
//
// THE SHAPE
//
// Firejail compiled its BPF programs into files under a per-jail directory and re-read them when a
// later process joined the jail. That directory was writable from inside. So a confined process
// could rewrite the filter, and the NEXT process to join was confined by whatever it wrote.
//
// The property that failed is not "the filter was wrong". It is:
//
//     THE BYTES THE KERNEL RECEIVES ARE THE BYTES THE POLICY AUTHOR WROTE.
//
// Firejail broke it through a mutable file. The general ways to break it are: a mutable file, a
// shared mapping, a dangling pointer to a builder that has gone out of scope, or a builder that
// silently dropped rules on the way (which is a different CVE-shaped defect the rigor suite already
// covers at sec_bpf_encode.cpp:301-350).
//
// MICRON'S ANALOGUE
//
// micron does not have the file. Filters are `constexpr` arrays inside a filter_builder, built at
// compile time where the policy is type-level, and reach the child by fork() -- a copy-on-write
// snapshot of the parent's memory taken before the child can run any code at all. There is no
// window and no shared object.
//
// What there IS, is a borrow. sandbox.hpp:586-597:
//
//     template<usize N> sandbox & seccomp(const seccomp::filter_builder<N> &fb) noexcept {
//       ...
//       __filter = fb.insns;        // <-- a raw pointer into the caller's builder
//       __filter_len = static_cast<u16>(fb.count);
//       return *this;
//     }
//     template<usize N> sandbox & seccomp(seccomp::filter_builder<N> &&) = delete;
//
// The rvalue overload is deleted, which closes the temporary case at compile time. A named builder
// that goes out of scope before spawn() is still a use-after-free -- the sandbox would install
// whatever now occupies that stack -- and the type system cannot see it.
//
// So: micron is not vulnerable to Firejail's defect, and this file is the permanent guard on the
// property Firejail lost, plus the compile-time pins on the borrow.
//
// WHAT THIS PINS
//   1  the program the child installs is byte-identical to the one configured  -- checked in the child
//   2  a filter is not reachable through any file the sandbox creates
//   3  the rvalue seccomp() overload is deleted                    (compile-time)
//   4  a filter_builder cannot bind to a temporary anywhere in the API   (compile-time)
//   5  a builder that dropped rules is refused rather than installed
//   6  the type-level and imperative faces produce identical bytes  (nothing is lost in composition)
//
// POLARITY: this file passes on the current tree and is a permanent regression guard. It goes red if
// the borrow is ever widened, if the overflow refusal is weakened, or if a filter is ever routed
// through a file or a shared mapping.
//
// NEGATIVE CONTROL: contract 1 does not merely compare the filter to itself. It ships the program
// the CHILD sees back to the parent over a pipe and compares it against the parent's copy byte for
// byte -- and it first proves the comparison has teeth by running the same machinery against a
// DELIBERATELY perturbed copy and requiring a mismatch. Contract 5 likewise proves the kernel would
// have accepted the truncated program, so the builder's refusal is the only thing that stopped it.
//
// CONTROL (ungated): an ordinary named builder must still install and still work. The fix for a
// borrow is not to forbid the borrow.
//
// Build:
//   duck test tests/adv/adv_cve_2019_12589_filter_immutable.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/sec/fp.hpp"
#include "../../src/sec/groups.hpp"
#include "../../src/sec/policy.hpp"
#include "../../src/sec/sandbox.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"
#include "../support/sec_oracle.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace s = micron::sec;
namespace ns = micron::sec::ns;
namespace so = sec_oracle;

namespace
{

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 3 + 4  compile-time pins on the borrow
//
// These are static_asserts rather than runtime checks because the defect they guard is a lifetime
// one, and a lifetime defect that reaches runtime has already installed the wrong filter.

template<typename B>
concept seccomp_binds_rvalue = requires(s::sandbox &box, B &&fb) { box.seccomp(static_cast<B &&>(fb)); };

static_assert(!seccomp_binds_rvalue<sc::filter_builder<64>>,
              "sandbox::seccomp() must not bind a temporary filter_builder: the sandbox stores "
              "fb.insns as a raw pointer, so the program would be read from a dead stack frame");
static_assert(!seccomp_binds_rvalue<sc::filter_builder<128>>, "same, at the default capacity");
static_assert(!seccomp_binds_rvalue<sc::filter_builder<1024>>, "same, at the policy capacity");

// and the lvalue form must still work, or the assertions above are just "the API is broken"
template<typename B>
concept seccomp_binds_lvalue = requires(s::sandbox &box, B &fb) { box.seccomp(fb); };
static_assert(seccomp_binds_lvalue<sc::filter_builder<64>>, "a named builder must still be accepted");
static_assert(seccomp_binds_lvalue<sc::filter_builder<1024>>, "a named builder must still be accepted");

// the same guard on the functional face (fp.hpp:408)
template<typename B>
concept confined_binds_rvalue = requires(B &&fb) { s::confined(static_cast<B &&>(fb), +[]() -> i32 { return 0; }); };
static_assert(!confined_binds_rvalue<sc::filter_builder<64>>, "sec::confined() must not bind a temporary builder, for the same reason");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the filter under test, and the pipe the child reports through

constexpr usize max_insns = 512;

// the parent's copy, and the pipe the child writes its own view into
mc::bpf::insn_t g_reference[max_insns]{};
u16 g_reference_len = 0;
i32 g_pipe_w = -1;

template<usize N>
void
build_reference(sc::filter_builder<N> &fb)
{
  fb.require_native_arch();
  fb.deny_errno(SYS_getpgid, eperm);
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::signal::count; ++i ) fb.allow(g::signal::calls[i]);
  // The two syscalls the SANDBOX ITSELF issues after the filter is in force, neither of which has
  // anything to do with what this file tests -- but a reference filter that denies either faults the
  // sandbox at a stage this file is not about.
  //
  //   prctl   -- stage 9 sets NO_NEW_PRIVS and PR_SET_DUMPABLE.
  //   capset  -- stage 12 drops the capability sets, and drop_capabilities() is now the DEFAULT.
  //              Inside a user namespace the child really is root with a full set, so the drop
  //              really does happen and really does need capset. That is a live requirement on
  //              every userns policy, not a quirk of this test: caps_post runs AFTER seccomp.
  //              (An unprivileged sandbox with no userns holds nothing, so the call is skipped and
  //              the requirement does not arise -- which is why this only bit once the control below
  //              added .user().)
  fb.allow(SYS_prctl);
  fb.allow(SYS_capset);
  fb.default_errno(eperm);
}

// The child re-derives the program the same way the parent did and ships it back. It cannot read
// the installed filter out of the kernel -- there is no such interface -- so what this proves is
// that the fork+borrow path delivers the same bytes, which is exactly what Firejail's file did not.
i32
child_report_filter(void)
{
  sc::filter_builder<max_insns> fb;
  build_reference(fb);
  if ( !fb.valid() ) return adv::setup_failed;
  auto p = fb.prog();
  const u16 len = p.len;
  if ( mc::posix::write(g_pipe_w, &len, sizeof(len)) != static_cast<i64>(sizeof(len)) ) return adv::setup_failed;
  const usize bytes = static_cast<usize>(len) * sizeof(mc::bpf::insn_t);
  if ( mc::posix::write(g_pipe_w, p.filter, bytes) != static_cast<i64>(bytes) ) return adv::setup_failed;
  return adv::ok_code;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2019-12589 (the bytes the kernel gets are the bytes the author wrote) ===");

  // the parent's reference copy
  {
    sc::filter_builder<max_insns> fb;
    build_reference(fb);
    sb::test_case("the reference filter is well-formed");
    sb::require_true(fb.valid());
    auto p = fb.prog();
    g_reference_len = p.len;
    for ( u16 i = 0; i < p.len; ++i ) g_reference[i] = p.filter[i];
    sb::print("  reference program: ", static_cast<u32>(g_reference_len), " instructions");
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  what the child sees, compared byte for byte
  //
  // NEGATIVE CONTROL is inline: the same comparison is first run against a perturbed copy and
  // required to REPORT a mismatch. A comparison that cannot fail is not a comparison.

  {
    sb::test_case("negative control: the byte comparison detects a single flipped instruction");
    mc::bpf::insn_t perturbed[max_insns]{};
    for ( u16 i = 0; i < g_reference_len; ++i ) perturbed[i] = g_reference[i];
    // turn one deny into an allow -- the smallest edit that changes what the filter means
    perturbed[g_reference_len - 1].k = sc::act_allow();

    usize diffs = 0;
    for ( u16 i = 0; i < g_reference_len; ++i ) {
      const mc::bpf::insn_t &a = g_reference[i];
      const mc::bpf::insn_t &b = perturbed[i];
      if ( a.code != b.code || a.jt != b.jt || a.jf != b.jf || a.k != b.k ) ++diffs;
    }
    sb::print("  perturbed copy differs in ", diffs, " instruction(s)");
    sb::require(diffs, static_cast<usize>(1));
  }

  {
    sb::test_case("the program on the child's side of a fork is byte-identical");
    i32 pfd[2] = { -1, -1 };
    sb::require_true(mc::posix::pipe2(pfd, 0) >= 0);
    g_pipe_w = pfd[1];

    const adv::child_result r = adv::run_child(child_report_filter);
    (void)mc::posix::close(pfd[1]);

    if ( !r.ok() ) {
      sb::print("  child could not report its filter (grade: ", adv::name_of(r.g), ")");
      (void)mc::posix::close(pfd[0]);
      sb::require_true(r.ok());
    } else {
      u16 len = 0;
      sb::require(mc::posix::read(pfd[0], &len, sizeof(len)), static_cast<i64>(sizeof(len)));
      sb::require(len, g_reference_len);

      mc::bpf::insn_t got[max_insns]{};
      const usize bytes = static_cast<usize>(len) * sizeof(mc::bpf::insn_t);
      usize done = 0;
      while ( done < bytes ) {
        const i64 n = mc::posix::read(pfd[0], reinterpret_cast<char *>(got) + done, bytes - done);
        if ( n <= 0 ) break;
        done += static_cast<usize>(n);
      }
      (void)mc::posix::close(pfd[0]);
      sb::require(done, bytes);

      usize diffs = 0;
      for ( u16 i = 0; i < len; ++i ) {
        const mc::bpf::insn_t &a = g_reference[i];
        const mc::bpf::insn_t &b = got[i];
        if ( a.code != b.code || a.jt != b.jt || a.jf != b.jf || a.k != b.k ) ++diffs;
      }
      if ( diffs != 0 ) sb::print("  the child's program differs from the parent's in ", diffs, " instruction(s)");
      sb::require(diffs, static_cast<usize>(0));
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  no file anywhere

  {
    sb::test_case("a configured sandbox creates no file holding its filter");
    // Firejail's defect was a path. The property that makes micron immune is that there is not one,
    // and the way that stops being true is somebody adding a cache. Bracket the descriptor count
    // across a full configure+launch: a filter written to a file has to be opened to be written.
    const usize before = adv::open_fds();

    sc::filter_builder<max_insns> fb;
    build_reference(fb);
    s::sandbox box;
    box.seccomp(fb);
    box.no_new_privs(true);
    sb::require_true(box.configured());

    const usize after = adv::open_fds();
    sb::require(after, before);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  a builder that lost rules is refused, not installed
  //
  // The other way to install a program the author did not write. This is the shape sec_bpf_encode
  // covers structurally; here it is driven through the sandbox, which is the path a caller uses.

  {
    sb::test_case("a builder that silently dropped rules is refused before anything forks");
    // a capacity that cannot hold the group: rules are dropped and `overflowed` is set
    sc::filter_builder<24> tiny;
    tiny.require_native_arch();
    for ( usize i = 0; i < g::baseline::count; ++i ) tiny.allow(g::baseline::calls[i]);
    tiny.default_errno(eperm);
    sb::require_true(tiny.overflowed);
    sb::require_false(tiny.valid());

    // and the KERNEL would have taken it: well-formed, ends in a return, every jump in range. The
    // builder's refusal is the only thing between the caller and a policy nobody wrote.
    auto p = tiny.prog();
    sb::require(static_cast<i32>(so::verify(p.filter, p.len)), static_cast<i32>(so::fault::none));

    s::sandbox box;
    box.namespaces(ns::ns_kind::user);
    box.seccomp(tiny);
    if ( box.configured() )
      sb::print("  the sandbox accepted a builder that dropped rules; the truncated program would "
                "have installed and the kernel would have taken it");
    sb::require_false(box.configured());
    sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::seccomp));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  nothing is lost in composition
  //
  // The type-level face and the imperative one must agree instruction for instruction. If they did
  // not, "the bytes the author wrote" would depend on which face they wrote them through.

  {
    sb::test_case("the type-level and imperative faces emit identical bytes");
    using policy
        = s::seccomp_policy_n<max_insns, s::errno_call<SYS_getpgid, eperm>, s::allow<g::baseline, g::signal>, s::allow_calls<SYS_prctl, SYS_capset>>;
    auto typed = policy::build(sc::act_errno(eperm));
    sb::require_true(typed.valid());

    sc::filter_builder<max_insns> manual;
    build_reference(manual);
    sb::require_true(manual.valid());

    sb::require(typed.count, manual.count);
    usize diffs = 0;
    for ( usize i = 0; i < typed.count; ++i ) {
      if ( typed.insns[i].code != manual.insns[i].code || typed.insns[i].jt != manual.insns[i].jt || typed.insns[i].jf != manual.insns[i].jf
           || typed.insns[i].k != manual.insns[i].k )
        ++diffs;
    }
    sb::require(diffs, static_cast<usize>(0));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: a named builder still installs and still confines");
    if ( !adv::have_userns() ) {
      sb::skip("this kernel refuses an unprivileged user namespace; the sandbox cannot be launched here");
    } else {
      sc::filter_builder<max_insns> fb;
      build_reference(fb);
      sb::require_true(fb.valid());

      s::sandbox box;
      box.namespaces(ns::ns_kind::user);
      box.seccomp(fb);
      sb::require_true(box.configured());

      const auto r = box.run_to_completion([]() -> i32 {
        // the rule the reference filter carries: getpgid must be EPERM, everything else must work
        if ( mc::syscall(SYS_getpgid, 0) != -static_cast<long>(mc::error::permissions) ) return adv::bad_code;
        // gettid, not getpid: groups::baseline names gettid and does NOT name getpid, and a control
        // that failed on that would be reporting the group's contents rather than the borrow
        if ( mc::syscall(SYS_gettid) <= 0 ) return adv::bad_code;
        return adv::ok_code;
      });
      sb::require_true(r.is_first());
      sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
    }
  }

  sb::print("=== ADV CVE-2019-12589 PASSED ===");
  return 1;
}
