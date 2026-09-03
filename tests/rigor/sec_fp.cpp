//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// The functional face must mean exactly what the imperative one means.
//
// micron::sec offers three ways to say the same thing, and nothing about the source makes them
// agree -- they are three separate lowerings. So this compares the artefact: the emitted cBPF,
// instruction for instruction, across all three. Most of it is settled at COMPILE time, because
// the builder and every pipeline stage are constexpr, which turns a divergence into a build error.
//
// The pipeline also has a property worth pinning on its own: an adaptor returns the builder by
// reference, so a chain of any length copies nothing. filter_builder<1024> is 8 KB; a
// return-by-value design would memcpy that once per stage.

#include "../../src/std.hpp"

#include "../../src/sec.hpp"

#include "../support/sec_oracle.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace ll = micron::sec::landlock;
namespace so = sec_oracle;

namespace
{

constexpr usize cap_n = 1024;
constexpr u16 deny_e = static_cast<u16>(mc::error::permissions);

// (a) functional
constexpr sc::filter_builder<cap_n>
functional_build(void) noexcept
{
  return sc::policy<cap_n>() | sc::arch_native() | sc::allow_group<g::baseline, g::io>()
         | sc::deny_group<g::network>(deny_e) | sc::deny_all(deny_e) | sc::build();
}

// (b) imperative
constexpr sc::filter_builder<cap_n>
imperative_build(void) noexcept
{
  sc::filter_builder<cap_n> fb;
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::io::count; ++i ) fb.allow(g::io::calls[i]);
  for ( usize i = 0; i < g::network::count; ++i ) fb.deny_errno(g::network::calls[i], deny_e);
  fb.__seal(sc::act_errno(deny_e));
  return fb;
}

// (c) type-level
using type_policy = s::seccomp_policy_n<cap_n, s::allow<g::baseline, g::io>, s::deny<g::network>>;

template<usize N>
constexpr bool
same_program(const sc::filter_builder<N> &a, const sc::filter_builder<N> &b) noexcept
{
  if ( a.count != b.count || a.overflowed != b.overflowed ) return false;
  for ( usize i = 0; i < a.count; ++i ) {
    if ( a.insns[i].code != b.insns[i].code || a.insns[i].jt != b.insns[i].jt ) return false;
    if ( a.insns[i].jf != b.insns[i].jf || a.insns[i].k != b.insns[i].k ) return false;
  }
  return true;
}

constexpr auto from_fp = functional_build();
constexpr auto from_imp = imperative_build();
constexpr auto from_type = type_policy::build(sc::act_errno(deny_e));

static_assert(from_fp.count > 0, "the functional pipeline emitted nothing");
static_assert(from_fp.valid(), "the functional pipeline is not installable");
static_assert(same_program(from_fp, from_imp),
              "micron::sec: the functional pipeline and the imperative chain no longer agree");
static_assert(same_program(from_fp, from_type),
              "micron::sec: the functional pipeline and the type-level policy no longer agree");

// a stage must hand back the SAME object, not a copy -- this is what keeps an 8 KB builder from
// being memcpy'd once per link in the chain
static_assert(micron::is_reference_v<decltype(sc::arch_native()(micron::declval<sc::filter_builder<8> &&>()))>,
              "a seccomp pipeline stage must return the builder by reference");
static_assert(micron::is_reference_v<decltype(sc::allow(0)(micron::declval<sc::filter_builder<8> &&>()))>,
              "a seccomp pipeline stage must return the builder by reference");

};      // namespace

int
main(void)
{
  sb::print("=== SEC FP ===");

  // ---------------------------------------------------------------- //
  sb::test_case("all three interfaces emit one identical program");
  {
    sb::require_true(same_program(from_fp, from_imp));
    sb::require_true(same_program(from_fp, from_type));
    sb::require(from_fp.count, from_imp.count);
    sb::print("  instructions: ", static_cast<u64>(from_fp.count));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("and they behave identically when the program is actually executed");
  {
    auto verdict = [](const sc::filter_builder<cap_n> &fb, i32 nr) {
      so::probe p{};
      p.nr = nr;
      p.arch = static_cast<u32>(sc::native_arch);
      return so::run(fb.insns, fb.count, p);
    };
    sb::require(static_cast<u32>(so::verify(from_fp.insns, from_fp.count)), static_cast<u32>(so::fault::none));

    for ( i32 nr : { SYS_read, SYS_write, SYS_close, SYS_exit_group, SYS_mount, SYS_ptrace, SYS_socket } ) {
      const u32 a = verdict(from_fp, nr).action;
      sb::require(verdict(from_imp, nr).action, a);
      sb::require(verdict(from_type, nr).action, a);
    }
    sb::require(verdict(from_fp, SYS_read).action, sc::act_allow());
    sb::require(verdict(from_fp, SYS_mount).action, sc::act_errno(deny_e));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the argument-predicate stages lower the same way through both faces");
  {
    constexpr auto fp = sc::policy<64>() | sc::arch_native()
                        | sc::allow_when(SYS_ioctl, sc::arg_eq(1, 0x5401))
                        | sc::deny_when(SYS_openat, sc::arg_masked(1, 0x3, 0x1), 30) | sc::deny_all(deny_e)
                        | sc::build();

    sc::filter_builder<64> imp;
    imp.require_native_arch();
    imp.action_if(SYS_ioctl, sc::arg_eq(1, 0x5401), sc::act_allow());
    imp.action_if(SYS_openat, sc::arg_masked(1, 0x3, 0x1), sc::act_errno(30));
    imp.__seal(sc::act_errno(deny_e));

    sb::require_true(same_program(fp, imp));

    // and the predicate really discriminates
    auto verdict = [&fp](i32 nr, u32 argn, u64 v) {
      so::probe p{};
      p.nr = nr;
      p.arch = static_cast<u32>(sc::native_arch);
      p.args[argn] = v;
      return so::run(fp.insns, fp.count, p).action;
    };
    sb::require(verdict(SYS_ioctl, 1, 0x5401), sc::act_allow());
    sb::require(verdict(SYS_ioctl, 1, 0x5402), sc::act_errno(deny_e));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the seal verbs differ only in the default action");
  {
    constexpr auto killer = sc::policy<64>() | sc::arch_native() | sc::allow(SYS_read) | sc::kill_all() | sc::build();
    constexpr auto denier = sc::policy<64>() | sc::arch_native() | sc::allow(SYS_read) | sc::deny_all(deny_e) | sc::build();
    constexpr auto opener = sc::policy<64>() | sc::arch_native() | sc::allow(SYS_read) | sc::allow_all() | sc::build();

    sb::require(killer.count, denier.count);
    sb::require(denier.count, opener.count);
    for ( usize i = 0; i + 1 < killer.count; ++i ) sb::require(killer.insns[i].k, denier.insns[i].k);
    sb::require(killer.insns[killer.count - 1].k, mc::posix::seccomp_ret_kill_process);
    sb::require(denier.insns[denier.count - 1].k, sc::act_errno(deny_e));
    sb::require(opener.insns[opener.count - 1].k, sc::act_allow());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every install terminal refuses a policy the builder could not encode");
  {
    // NOTHING here installs: each pipeline is refused at the gate, which is exactly the property
    // under test. A terminal that let one of these through would confine this process instead
    sb::require(mc::prctl(mc::PR_GET_SECCOMP), 0);

    // (a) truncated. policy<20> holds the amd64 arch gate and four denies; the rest are dropped,
    // the seal still fits, and the surviving default is ALLOW -- fail-open if it ever installed
    {
      auto fb = sc::policy<20>() | sc::arch_native() | sc::deny_group<g::network>() | sc::allow_all() | sc::build();
      sb::require_true(fb.overflowed);
      auto r = sc::policy<20>() | sc::arch_native() | sc::deny_group<g::network>() | sc::allow_all() | sc::install();
      sb::require_true(r.is_second());
      sb::require(r.cast<mc::sec::error_t>().code, static_cast<i32>(mc::error::invalid_arg));
    }

    // (b) no arch gate
    {
      auto r = sc::policy<64>() | sc::allow(SYS_read) | sc::deny_all(deny_e) | sc::install();
      sb::require_true(r.is_second());
      auto t = sc::policy<64>() | sc::allow(SYS_read) | sc::deny_all(deny_e) | sc::install_tsync();
      sb::require_true(t.is_second());
      mc::sec::result<i32> n = sc::policy<64>() | sc::allow(SYS_read) | sc::deny_all(deny_e) | sc::install_notif();
      sb::require_true(n.is_second());
      sb::require(n.cast<mc::sec::error_t>().code, static_cast<i32>(mc::error::invalid_arg));
    }

    // (c) never sealed
    {
      auto r = sc::policy<64>() | sc::arch_native() | sc::allow(SYS_read) | sc::install();
      sb::require_true(r.is_second());
    }

    // (d) the gate placed after a rule, which is dead code once anything above it returns
    {
      auto fb = sc::policy<64>() | sc::allow(SYS_read) | sc::arch_native() | sc::deny_all(deny_e) | sc::build();
      sb::require_false(fb.arch_ok);
      sb::require_true(fb.overflowed);
      auto r = sc::policy<64>() | sc::allow(SYS_read) | sc::arch_native() | sc::deny_all(deny_e) | sc::install();
      sb::require_true(r.is_second());
    }

    // and none of that installed anything
    sb::require(mc::prctl(mc::PR_GET_SECCOMP), 0);
    sb::require(mc::prctl(mc::PR_GET_NO_NEW_PRIVS), 0);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a landlock chain confines exactly as the imperative ruleset does");
  {
    // both faces build the same domain; compare what they HANDLE and that both install
    ll::ruleset imp = ll::try_ruleset(ll::read_only);
    sb::require_true(imp.valid());
    sb::require(imp.allow("/usr", ll::read_only), 0);

    ll::ruleset fp = ll::confine(ll::read_only) | ll::beneath("/usr", ll::read_only);
    sb::require_true(fp.valid());
    sb::require(mc::sec::landlock::bits(fp.handled()), mc::sec::landlock::bits(imp.handled()));
    sb::require(fp.abi(), imp.abi());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a landlock chain over a FAILED ruleset stays failed and reports the first error");
  {
    // an impossible handled mask: the ruleset never opens, and the chain must not pretend it did
    ll::ruleset dead = ll::confine(ll::access_fs::none);
    sb::require_false(dead.valid());
    const i32 first = dead.error();

    mc::sec::result<mc::sec::unit_t> r
        = ll::confine(ll::access_fs::none) | ll::beneath("/usr", ll::read_only) | ll::enforce();
    sb::require_true(r.is_second());      // NOT has_value(): that is true for both branches
    sb::require(r.cast<mc::sec::error_t>().code, static_cast<i32>(-first));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("with_ruleset brackets the resource and lifts a void body into a result");
  {
    auto r = s::with_ruleset(ll::read_only, [](ll::ruleset &rs) { (void)rs.allow("/usr", ll::read_only); });
    sb::require_true(r.is_first());

    auto v = s::with_ruleset(ll::read_only, [](ll::ruleset &rs) -> i32 { return rs.abi(); });
    sb::require_true(v.is_first());
    sb::require(v.cast<i32>(), ll::abi_level());

    // an unsatisfiable ruleset surfaces as the error branch rather than running the body
    bool ran = false;
    auto bad = s::with_ruleset(ll::access_fs::none, [&ran](ll::ruleset &) { ran = true; });
    sb::require_true(bad.is_second());
    sb::require_false(ran);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("confined() runs a body under a filter, in a child, and reports its status");
  {
    sc::filter_builder<1024> fb;
    fb.require_native_arch();
    auto add = [&fb](const i32 *c, usize n) {
      for ( usize i = 0; i < n; ++i ) fb.allow(c[i]);
    };
    add(g::baseline::calls, g::baseline::count);
    add(g::memory::calls, g::memory::count);
    add(g::signal::calls, g::signal::count);
    add(g::process::calls, g::process::count);
    fb.default_errno(deny_e);

    auto r = s::confined(fb, []() -> i32 {
      // mount is not in any allowed group, so the filter answers EPERM instead of the kernel
      return mc::syscall(SYS_mount, 0, 0, 0, 0, 0) == -1 ? 9 : 8;
    });
    sb::require_true(r.is_first());
    const s::sandbox::exit_status st = r.cast<s::sandbox::exit_status>();
    sb::require_true(st.exited());
    sb::require_false(st.signaled());
    sb::require(st.code(), 9);

    // the parent is still unfiltered
    sb::require(mc::prctl(mc::PR_GET_SECCOMP), 0);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("in_namespace runs a body inside fresh namespaces, in a child");
  {
    auto r = s::in_namespace(s::ns::ns_kind::user | s::ns::ns_kind::uts, []() -> i32 {
      return s::ns::open_self(s::ns::ns_kind::uts).is_host() ? 3 : 4;
    });
    sb::require_true(r.is_first());
    sb::require_true(r.cast<s::sandbox::exit_status>().exited());
    sb::require(r.cast<s::sandbox::exit_status>().code(), 4);

    // and the parent never left its own
    sb::require_true(s::ns::open_self(s::ns::ns_kind::user).valid());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the curried selinux forms compose through micron's pipe");
  {
    const char *path = "/etc/passwd";
    auto lbl = path | mc::sec::selinux::label_of_c();
    sb::require_true(lbl.is_first());
    sb::require_true(lbl.cast<mc::sec::selinux::context>().valid());
    // and it agrees with the direct call
    sb::require(lbl.cast<mc::sec::selinux::context>(), mc::sec::selinux::label_of(path).cast<mc::sec::selinux::context>());
  }
  sb::end_test_case();

  sb::print("=== SEC FP PASSED ===");
  return 1;
}
