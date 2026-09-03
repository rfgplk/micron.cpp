//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// The dual-interface gate.
//
// micron::sec exposes the same confinement three ways: a type-level policy, an imperative builder
// chain, and a functional pipeline. They are only useful if they mean the same thing, and nothing
// about the source makes that true -- they are three separate lowerings of one intent.
//
// So this file compares the machine code they produce: the emitted cBPF, instruction for
// instruction. Most of it runs at COMPILE time, because filter_builder is constexpr all the way
// down, which makes a divergence a build error rather than a test failure.

#include "../../src/std.hpp"

#include "../../src/sec/policy.hpp"

#include "../support/sec_oracle.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace so = sec_oracle;

namespace
{

constexpr usize cap_n = 1024;
constexpr u16 deny_errno = static_cast<u16>(mc::error::permissions);

// the policy, as a type
using type_policy = s::seccomp_policy_n<cap_n, s::allow<g::baseline, g::io>, s::deny<g::network>>;

// the same policy, written out by hand through the imperative builder
constexpr sc::filter_builder<cap_n>
imperative_build(void) noexcept
{
  sc::filter_builder<cap_n> fb;
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::io::count; ++i ) fb.allow(g::io::calls[i]);
  for ( usize i = 0; i < g::network::count; ++i ) fb.deny_errno(g::network::calls[i], deny_errno);
  fb.__seal(sc::act_errno(deny_errno));
  return fb;
}

template<usize N>
constexpr bool
same_program(const sc::filter_builder<N> &a, const sc::filter_builder<N> &b) noexcept
{
  if ( a.count != b.count ) return false;
  if ( a.overflowed != b.overflowed ) return false;
  for ( usize i = 0; i < a.count; ++i ) {
    if ( a.insns[i].code != b.insns[i].code ) return false;
    if ( a.insns[i].jt != b.insns[i].jt ) return false;
    if ( a.insns[i].jf != b.insns[i].jf ) return false;
    if ( a.insns[i].k != b.insns[i].k ) return false;
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the equivalence, settled at compile time

constexpr auto from_type = type_policy::build(sc::act_errno(deny_errno));
constexpr auto from_hand = imperative_build();

static_assert(from_type.count > 0, "the type-level policy emitted nothing");
static_assert(!from_type.overflowed, "the type-level policy overflowed its builder");
static_assert(from_type.valid(), "the type-level policy is not installable");
static_assert(same_program(from_type, from_hand),
              "micron::sec: the type-level policy and the imperative chain no longer emit the same filter");

// a strict policy differs only in its default action, and in exactly one instruction
using strict_policy = s::seccomp_strict_policy_n<cap_n, s::allow<g::baseline, g::io>, s::deny<g::network>>;
constexpr auto from_strict = strict_policy::build();
static_assert(from_strict.count == from_type.count, "strict and errno policies differ in length");

constexpr bool
differs_only_in_default(void) noexcept
{
  for ( usize i = 0; i + 1 < from_strict.count; ++i )
    if ( from_strict.insns[i].k != from_type.insns[i].k || from_strict.insns[i].code != from_type.insns[i].code ) return false;
  return from_strict.insns[from_strict.count - 1].k != from_type.insns[from_type.count - 1].k;
}

static_assert(differs_only_in_default(), "strict vs errno policy must differ in the default action alone");

};      // namespace

int
main(void)
{
  sb::print("=== SEC POLICY ===");

  // ---------------------------------------------------------------- //
  sb::test_case("the type-level policy and the imperative chain emit an identical program");
  {
    // the static_asserts above already settled this at build time; assert it at runtime too so a
    // failure reads as a test result rather than only as a compiler diagnostic
    sb::require_true(same_program(from_type, from_hand));
    sb::require(from_type.count, from_hand.count);
    sb::print("  policy instructions: ", static_cast<u64>(from_type.count));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("and both mean the same thing when executed, not merely look the same");
  {
    // identical bytes would be worthless if the program were wrong, so run it: everything named by
    // the allowed groups is allowed, everything in the denied group answers EPERM, and anything
    // named by neither lands on the default
    auto verdict = [](const sc::filter_builder<cap_n> &fb, i32 nr) {
      so::probe p{};
      p.nr = nr;
      p.arch = static_cast<u32>(sc::native_arch);
      return so::run(fb.insns, fb.count, p);
    };

    sb::require(static_cast<u32>(so::verify(from_type.insns, from_type.count)), static_cast<u32>(so::fault::none));

    for ( usize i = 0; i < g::baseline::count; ++i ) {
      so::outcome o = verdict(from_type, g::baseline::calls[i]);
      sb::require_true(o.ok());
      sb::require(o.action, sc::act_allow());
      sb::require(verdict(from_hand, g::baseline::calls[i]).action, o.action);
    }

    for ( usize i = 0; i < g::network::count; ++i ) {
      // a syscall named by BOTH an allow and a deny group takes whichever rule was emitted first;
      // only ones unique to network are unambiguously denied
      bool also_allowed = false;
      for ( usize j = 0; j < g::io::count; ++j )
        if ( g::io::calls[j] == g::network::calls[i] ) also_allowed = true;
      for ( usize j = 0; j < g::baseline::count; ++j )
        if ( g::baseline::calls[j] == g::network::calls[i] ) also_allowed = true;
      if ( also_allowed ) continue;

      so::outcome o = verdict(from_type, g::network::calls[i]);
      sb::require_true(o.ok());
      sb::require(o.action, sc::act_errno(deny_errno));
    }

    // named by nothing -> the default
    sb::require(verdict(from_type, SYS_mount).action, sc::act_errno(deny_errno));
    sb::require(verdict(from_strict, SYS_mount).action, mc::posix::seccomp_ret_kill_process);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every syscall group is non-empty and free of duplicates on this arch");
  {
    auto check = [](const char *nm, const i32 *calls, usize n) {
      sb::require_true(n > 0);
      for ( usize i = 0; i < n; ++i )
        for ( usize j = i + 1; j < n; ++j )
          if ( calls[i] == calls[j] ) {
            sb::print("  duplicate syscall in group ", nm);
            sb::require_true(false);
          }
    };
    check("baseline", g::baseline::calls, g::baseline::count);
    check("memory", g::memory::calls, g::memory::count);
    check("io", g::io::calls, g::io::count);
    check("filesystem", g::filesystem::calls, g::filesystem::count);
    check("filesystem_readonly", g::filesystem_readonly::calls, g::filesystem_readonly::count);
    check("filesystem_no_mount", g::filesystem_no_mount::calls, g::filesystem_no_mount::count);
    check("process", g::process::calls, g::process::count);
    check("process_no_ns", g::process_no_ns::calls, g::process_no_ns::count);
    check("signal", g::signal::calls, g::signal::count);
    check("network", g::network::calls, g::network::count);
    check("time", g::time::calls, g::time::count);
    check("ipc", g::ipc::calls, g::ipc::count);
    check("capabilities", g::capabilities::calls, g::capabilities::count);
    check("io_multiplexing", g::io_multiplexing::calls, g::io_multiplexing::count);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("process_no_ns really lacks the namespace primitives, and process really has them");
  {
    auto has = [](const i32 *c, usize n, i32 nr) {
      for ( usize i = 0; i < n; ++i )
        if ( c[i] == nr ) return true;
      return false;
    };
    sb::require_true(has(g::process::calls, g::process::count, SYS_unshare));
    sb::require_true(has(g::process::calls, g::process::count, SYS_setns));
    sb::require_false(has(g::process_no_ns::calls, g::process_no_ns::count, SYS_unshare));
    sb::require_false(has(g::process_no_ns::calls, g::process_no_ns::count, SYS_setns));

    // rt_sigreturn must never be missing from the signal group: denying it turns every delivered
    // signal into a kill
    sb::require_true(has(g::signal::calls, g::signal::count, SYS_rt_sigreturn));

    // the capabilities group must keep what a credential drop needs, since seccomp is installed
    // before that drop
    for ( i32 nr : { SYS_setuid, SYS_setgid, SYS_capset, SYS_prctl } )
      sb::require_true(has(g::capabilities::calls, g::capabilities::count, nr));

    // filesystem_readonly must not be able to reshape the mount tree
    for ( i32 nr : { SYS_mount, SYS_umount2, SYS_chroot, SYS_pivot_root } )
      sb::require_false(has(g::filesystem_readonly::calls, g::filesystem_readonly::count, nr));
    for ( i32 nr : { SYS_mount, SYS_umount2, SYS_chroot, SYS_pivot_root } )
      sb::require_true(has(g::filesystem::calls, g::filesystem::count, nr));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("filesystem_readonly cannot MUTATE the filesystem either, which is what its name says");
  {
    // the name is this constant's entire interface -- a policy author cannot re-derive a
    // per-architecture syscall list from a symbol, and nothing else in the header says what the
    // group means. It used to be `filesystem` minus the mount calls, so allowlisting a group
    // called "readonly" left the confined process able to unlink an audit log, rename a config
    // file out from under a reader, or plant a symlink to redirect a later privileged open
    auto has = [](const i32 *c, usize n, i32 nr) {
      for ( usize i = 0; i < n; ++i )
        if ( c[i] == nr ) return true;
      return false;
    };

    // the *at forms exist on every architecture micron targets
    for ( i32 nr : { SYS_linkat, SYS_symlinkat, SYS_unlinkat, SYS_renameat2, SYS_mkdirat, SYS_mknodat } )
      sb::require_false(has(g::filesystem_readonly::calls, g::filesystem_readonly::count, nr));
#if !defined(__micron_arch_arm64)
    for ( i32 nr : { SYS_link, SYS_symlink, SYS_unlink, SYS_rename, SYS_renameat, SYS_mkdir, SYS_rmdir, SYS_mknod } )
      sb::require_false(has(g::filesystem_readonly::calls, g::filesystem_readonly::count, nr));
#endif

    // and it must still be USEFUL: the non-mutating half is all there
    for ( i32 nr : { SYS_readlinkat, SYS_getdents64, SYS_faccessat, SYS_statfs, SYS_fstatfs, SYS_sync } )
      sb::require_true(has(g::filesystem_readonly::calls, g::filesystem_readonly::count, nr));

    // filesystem_no_mount is where the old contents went, and it is named for what it actually is
    for ( i32 nr : { SYS_mount, SYS_umount2, SYS_chroot, SYS_pivot_root, SYS_mknodat } )
      sb::require_false(has(g::filesystem_no_mount::calls, g::filesystem_no_mount::count, nr));
    for ( i32 nr : { SYS_linkat, SYS_symlinkat, SYS_unlinkat, SYS_renameat2, SYS_mkdirat } )
      sb::require_true(has(g::filesystem_no_mount::calls, g::filesystem_no_mount::count, nr));

    // every readonly member is also in filesystem: the group is a subset, not a divergent list
    for ( usize i = 0; i < g::filesystem_readonly::count; ++i )
      sb::require_true(has(g::filesystem::calls, g::filesystem::count, g::filesystem_readonly::calls[i]));
    for ( usize i = 0; i < g::filesystem_no_mount::count; ++i )
      sb::require_true(has(g::filesystem::calls, g::filesystem::count, g::filesystem_no_mount::calls[i]));

    sb::require_true(g::filesystem_readonly::count < g::filesystem_no_mount::count);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a filter built from filesystem_readonly answers the mutating calls with its default");
  {
    // the group list is what the encoder turns into SECCOMP_RET_ALLOW, so assert on the compiled
    // filter rather than only on the array
    sc::filter_builder<256> fb;
    fb.require_native_arch();
    for ( usize i = 0; i < g::filesystem_readonly::count; ++i ) fb.allow(g::filesystem_readonly::calls[i]);
    for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
    fb.default_errno(deny_errno);
    sb::require_true(fb.valid());

    auto acted = [&fb](i32 nr) {
      so::probe p{};
      p.nr = nr;
      p.arch = static_cast<u32>(sc::native_arch);
      return so::run(fb.insns, fb.count, p).action;
    };

    for ( i32 nr : { SYS_unlinkat, SYS_renameat2, SYS_mkdirat, SYS_linkat, SYS_symlinkat } )
      sb::require(acted(nr), sc::act_errno(deny_errno));
    for ( i32 nr : { SYS_readlinkat, SYS_getdents64, SYS_faccessat } ) sb::require(acted(nr), sc::act_allow());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the non-seccomp composers resolve their masks at compile time");
  {
    // NOTE (CVE audit): ns_kind::pid removed. namespace_policy<...pid...> is a static_assert now --
    // unshare(CLONE_NEWPID) moves the caller's CHILDREN and not the caller, so this face silently
    // did nothing for a pid namespace while reporting success. sandbox does the extra fork.
    using N = s::namespace_policy<s::ns::ns_kind::user, s::ns::ns_kind::mount>;
    static_assert(s::ns::bits(N::kinds) == 0x10020000uLL);      // CLONE_NEWUSER | CLONE_NEWNS
    static_assert(N::has_user);
    static_assert(!s::namespace_policy_none::has_user);

    using L = s::landlock_policy<s::beneath<"/usr", s::landlock::read_execute>,
                                 s::beneath<"/var/tmp", s::landlock::read_write>>;
    static_assert(s::landlock::bits(L::handled)
                  == (s::landlock::bits(s::landlock::read_execute) | s::landlock::bits(s::landlock::read_write)));

    static_assert(s::is_seccomp_policy<type_policy>);
    static_assert(s::is_namespace_policy<N>);
    static_assert(s::is_landlock_policy<L>);
    static_assert(s::is_capability_policy<s::capability_policy_none>);
    static_assert(s::is_capability_policy<s::capability_policy<mc::cap::net_bind_service>>);
    static_assert(s::is_rlimit_policy<s::rlimit_policy<s::limit<mc::posix::rlimit_nofile, 64>>>);
    static_assert(s::is_filesystem_policy<s::filesystem_policy<s::make_private>>);
    sb::require_true(true);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a policy too large for its builder is refused, not truncated");
  {
    // 64 instructions cannot hold the io group; the builder must record the overflow and valid()
    // must refuse, so apply() reports -EINVAL instead of installing a partial policy
    using too_big = s::seccomp_policy_n<64, s::allow<g::baseline, g::io, g::memory>>;
    auto fb = too_big::build(sc::act_allow());
    sb::require_true(fb.overflowed);
    sb::require_false(fb.valid());
    sb::require(too_big::apply(), -22);      // -EINVAL from load()
  }
  sb::end_test_case();

  sb::print("=== SEC POLICY PASSED ===");
  return 1;
}
