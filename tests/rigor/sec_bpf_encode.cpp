//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::seccomp -- the encoder's gate.
//
// Every filter the builder produces is executed on an independent cBPF interpreter
// (tests/support/sec_oracle.hpp) and its verdict compared against evaluating the same predicate
// directly in 64-bit C++. That is the only check that can see through emit_pred(): its lt/le/gt/ge
// lowerings are 6-to-8 instruction ladders comparing two 32-bit halves in sequence, and a wrong
// skip count there yields a filter that loads cleanly and misclassifies a subset of inputs.
//
// Nothing here installs a filter, so nothing here needs to fork.

#include "../../src/std.hpp"

#include "../../src/sec/groups.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../support/sec_oracle.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace so = sec_oracle;
namespace g = micron::sec::groups;

namespace
{

// fixed seed. NEVER time-based
constexpr u64 seed = 0xC0FFEE5EED1234ull;

constexpr i32 test_nr = 111;
constexpr i32 other_nr = 222;
constexpr u16 hit_errno = 42;

const u32 act_hit = sc::act_errno(hit_errno);
const u32 act_miss = sc::act_allow();

so::probe
make_probe(i32 nr, u32 argn, u64 val)
{
  so::probe p{};
  p.nr = nr;
  p.arch = static_cast<u32>(sc::native_arch);
  p.ip = 0xdeadbeefcafe0000ull;
  p.args[argn] = val;
  return p;
}

// run a builder's program on the oracle
template<usize N>
so::outcome
exec(sc::filter_builder<N> &fb, const so::probe &p)
{
  return so::run(fb.insns, fb.count, p);
}

so::cmp_kind
kind_of(sc::cmp c)
{
  switch ( c ) {
  case sc::cmp::eq: return so::cmp_kind::eq;
  case sc::cmp::ne: return so::cmp_kind::ne;
  case sc::cmp::lt: return so::cmp_kind::lt;
  case sc::cmp::le: return so::cmp_kind::le;
  case sc::cmp::gt: return so::cmp_kind::gt;
  case sc::cmp::ge: return so::cmp_kind::ge;
  case sc::cmp::masked_eq: return so::cmp_kind::masked;
  }
  return so::cmp_kind::eq;
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC BPF ENCODE ===");

  // ---------------------------------------------------------------- //
  sb::test_case("the native-arch gate passes the running arch and kills every other one");
  {
    sc::filter_builder<64> fb;
    fb.require_native_arch().allow(test_nr).default_errno(1);

    so::probe good = make_probe(test_nr, 0, 0);
    so::outcome r = exec(fb, good);
    sb::require_true(r.ok());
    sb::require(r.action, act_miss);      // allow(test_nr) fired

    // a foreign arch must die before the syscall number is ever examined
    for ( u32 foreign : { 0xC00000B7u, 0x40000003u, 0x40000028u, 0x00000000u } ) {
      if ( foreign == static_cast<u32>(sc::native_arch) ) continue;
      so::probe bad = make_probe(test_nr, 0, 0);
      bad.arch = foreign;
      so::outcome br = exec(fb, bad);
      sb::require_true(br.ok());
      sb::require(br.action, mc::posix::seccomp_ret_kill_process);
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("require_native_arch closes the x32 bypass, require_native_arch_raw does not");
  {
    sc::filter_builder<64> gated;
    gated.require_native_arch().allow(test_nr).default_errno(1);

    sc::filter_builder<64> raw;
    raw.require_native_arch_raw().allow(test_nr).default_errno(1);

    // an x32 task reports AUDIT_ARCH_X86_64 and carries __X32_SYSCALL_BIT in its syscall number
    so::probe x32 = make_probe(static_cast<i32>(0x40000000 | test_nr), 0, 0);
    so::outcome g = exec(gated, x32);
    so::outcome w = exec(raw, x32);
    sb::require_true(g.ok() && w.ok());

#if defined(__micron_arch_amd64)
    // gated: killed by the range deny. raw: falls through to the default, which is the bypass
    sb::require(g.action, mc::posix::seccomp_ret_kill_process);
    sb::require_distinct(g.action == w.action, true);
#endif
    // and an ordinary syscall number is untouched by the range on either
    so::probe plain = make_probe(test_nr, 0, 0);
    sb::require(exec(gated, plain).action, act_miss);
    sb::require(exec(raw, plain).action, act_miss);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("allow / deny_errno / trap_syscall each reach exactly their own syscall");
  {
    sc::filter_builder<64> fb;
    fb.require_native_arch()
        .allow(test_nr)
        .deny_errno(other_nr, 7)
        .trap_syscall(333)
        .default_kill();

    sb::require(exec(fb, make_probe(test_nr, 0, 0)).action, sc::act_allow());
    sb::require(exec(fb, make_probe(other_nr, 0, 0)).action, sc::act_errno(7));
    sb::require(exec(fb, make_probe(333, 0, 0)).action, sc::act_trap());
    sb::require(exec(fb, make_probe(444, 0, 0)).action, mc::posix::seccomp_ret_kill_process);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("allow_range and deny_range cover exactly the closed interval [lo, hi]");
  {
    constexpr i32 lo = 900, hi = 910;
    sc::filter_builder<64> fb;
    fb.require_native_arch().allow_range(lo, hi).default_errno(5);

    for ( i32 nr = lo - 6; nr <= hi + 6; ++nr ) {
      so::outcome r = exec(fb, make_probe(nr, 0, 0));
      sb::require_true(r.ok());
      const u32 want = (nr >= lo && nr <= hi) ? sc::act_allow() : sc::act_errno(5);
      sb::require(r.action, want);
    }

    sc::filter_builder<64> db;
    db.require_native_arch().deny_range(lo, hi, sc::act_errno(9)).default_allow();
    for ( i32 nr = lo - 6; nr <= hi + 6; ++nr ) {
      so::outcome r = exec(db, make_probe(nr, 0, 0));
      sb::require_true(r.ok());
      const u32 want = (nr >= lo && nr <= hi) ? sc::act_errno(9) : sc::act_allow();
      sb::require(r.action, want);
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("allow_batch is equivalent, instruction for instruction, to a run of allow()");
  {
    sc::filter_builder<128> batched;
    batched.require_native_arch().allow_batch<10, 20, 30, 40>().default_errno(1);

    for ( i32 nr : { 10, 20, 30, 40 } ) sb::require(exec(batched, make_probe(nr, 0, 0)).action, sc::act_allow());
    for ( i32 nr : { 11, 21, 0, 41 } ) sb::require(exec(batched, make_probe(nr, 0, 0)).action, sc::act_errno(1));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every emit_pred lowering agrees with a 64-bit predicate, over edge-seeking inputs");
  {
    so::rng rg{ seed };

    const sc::cmp ops[] = { sc::cmp::eq, sc::cmp::ne, sc::cmp::lt, sc::cmp::le,
                            sc::cmp::gt, sc::cmp::ge, sc::cmp::masked_eq };

    usize checked = 0;
    for ( usize trial = 0; trial < 3000; ++trial ) {
      const sc::cmp op = ops[rg.below(7)];
      const u32 argn = rg.below(6);
      const u64 val = rg.edgy(rg.next());
      const u64 mask = rg.edgy(0xffffffffull);

      sc::arg_cmp_t ac = (op == sc::cmp::masked_eq) ? sc::arg_masked(argn, mask, val)
                                                    : sc::arg_cmp_t{ argn, op, val, 0 };

      sc::filter_builder<64> fb;
      fb.require_native_arch();
      fb.action_if(test_nr, ac, act_hit);
      fb.default_allow();

      // the program must be structurally acceptable to the kernel before it can be right
      sb::require(static_cast<u32>(so::verify(fb.insns, fb.count)), static_cast<u32>(so::fault::none));

      for ( usize probe_i = 0; probe_i < 12; ++probe_i ) {
        const u64 arg = rg.edgy(val);
        so::outcome got = exec(fb, make_probe(test_nr, argn, arg));
        sb::require_true(got.ok());

        const bool want_hit = so::predicate(kind_of(op), arg, val, mask);
        sb::require(got.action, want_hit ? act_hit : act_miss);
        ++checked;
      }

      // a different syscall number must never reach the predicate block at all
      so::outcome miss = exec(fb, make_probe(other_nr, argn, val));
      sb::require_true(miss.ok());
      sb::require(miss.action, act_miss);
    }
    sb::print("  predicate evaluations checked: ", static_cast<u64>(checked));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a predicate is scoped to its own argument index and leaves the others alone");
  {
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.action_if(test_nr, sc::arg_eq(3, 0x1234'5678'9abcull), act_hit);
    fb.default_allow();

    sb::require(exec(fb, make_probe(test_nr, 3, 0x1234'5678'9abcull)).action, act_hit);
    for ( u32 n = 0; n < 6; ++n ) {
      if ( n == 3 ) continue;
      sb::require(exec(fb, make_probe(test_nr, n, 0x1234'5678'9abcull)).action, act_miss);
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a filter built from every syscall group is structurally valid");
  {
    namespace g = micron::sec::groups;
    sc::filter_builder<2048> fb;
    fb.require_native_arch();

    auto add = [&fb](const i32 *calls, usize n) {
      for ( usize i = 0; i < n; ++i ) fb.allow(calls[i]);
    };
    add(g::baseline::calls, g::baseline::count);
    add(g::memory::calls, g::memory::count);
    add(g::io::calls, g::io::count);
    add(g::process_no_ns::calls, g::process_no_ns::count);
    add(g::signal::calls, g::signal::count);
    add(g::time::calls, g::time::count);
    fb.default_errno(1);

    sb::require_true(fb.valid());
    sb::require(static_cast<u32>(so::verify(fb.insns, fb.count)), static_cast<u32>(so::fault::none));
    sb::require_true(fb.count < 2048);

    // every syscall named in those groups must actually be allowed by the program we built
    auto allows = [&fb](i32 nr) { return exec(fb, make_probe(nr, 0, 0)).action == sc::act_allow(); };
    for ( usize i = 0; i < g::baseline::count; ++i ) sb::require_true(allows(g::baseline::calls[i]));
    for ( usize i = 0; i < g::io::count; ++i ) sb::require_true(allows(g::io::calls[i]));

    // and a syscall in none of them must not be
    sb::require_true(!allows(SYS_mount));
    sb::require_true(!allows(SYS_ptrace));
    sb::print("  full-group filter instructions: ", static_cast<u64>(fb.count));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("valid() refuses a filter with no arch gate, and sealing stops further rules");
  {
    sc::filter_builder<64> ungated;
    ungated.allow(test_nr).default_kill();
    sb::require_false(ungated.valid());

    sc::filter_builder<64> gated;
    gated.require_native_arch().default_allow();
    sb::require_true(gated.valid());

    const usize sealed_at = gated.count;
    gated.allow(other_nr).deny_errno(333, 3);
    sb::require(gated.count, sealed_at);      // sealed: nothing can be appended after a default
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an overflowed builder is refused rather than silently truncated");
  {
    // an over-large policy drops rules. The dangerous shape is NOT the one where the default
    // action is dropped too -- that leaves a conditional jumping past the end, which the kernel
    // rejects with EINVAL on its own. It is the truncation that stops one or two slots short: the
    // seal still fits, the program still ends in a return, every jump is still in range, and the
    // kernel takes it. That installs a policy nobody wrote, fail-OPEN wherever the surviving
    // default allows. The builder has to be what catches it.
    sc::filter_builder<16> tiny;
    tiny.require_native_arch();
    for ( i32 nr = 0; nr < 500; ++nr ) tiny.allow(nr);
    tiny.default_kill();

    sb::require_true(tiny.count <= 16);
    sb::require_true(tiny.overflowed);
    sb::require_false(tiny.valid());      // <- what stops load() from installing it

    // the truncated program is now perfectly well formed, which is exactly why the flag is load
    // bearing: the kernel would have accepted this one
    sb::require(static_cast<u32>(so::verify(tiny.insns, tiny.count)), static_cast<u32>(so::fault::none));

    // a builder that fits is untouched by any of this
    sc::filter_builder<64> roomy;
    roomy.require_native_arch().allow(test_nr).default_kill();
    sb::require_false(roomy.overflowed);
    sb::require_true(roomy.valid());
    sb::require(static_cast<u32>(so::verify(roomy.insns, roomy.count)), static_cast<u32>(so::fault::none));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the exact residual-1 shape from the report: a blocklist that silently kept 4 of 18 rules");
  {
    // policy<20> | arch_native | deny_group<network> | allow_all. On amd64 the gate costs 7 and
    // each deny 3, so the 5th deny finds one slot left, the seal fits in it, and the filter used
    // to install with fourteen network syscalls falling through to ALLOW -- while install()
    // reported success.
    sc::filter_builder<20> fb;
    fb.require_native_arch();
    for ( usize i = 0; i < g::network::count; ++i ) fb.deny_errno(g::network::calls[i], 1);
    fb.default_allow();

    sb::require_true(fb.overflowed);
    sb::require_false(fb.valid());
    // the program the old builder would have handed the kernel is structurally impeccable
    sb::require(static_cast<u32>(so::verify(fb.insns, fb.count)), static_cast<u32>(so::fault::none));
    sb::require_true(g::network::count > 5);      // the case is only meaningful if rules were lost
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("across EVERY capacity, a valid() filter means exactly what was written");
  {
    // the invariant, swept rather than sampled. For each Max, build the same rule list twice: once
    // into a builder that cannot overflow, once into filter_builder<Max>. Either the small one is
    // flagged, or it must answer identically to the reference for every syscall probed. There is
    // no third outcome -- and "silently dropped a rule but still valid()" is precisely that third
    // outcome.
    constexpr i32 rules[] = { 3, 9, 17, 42, 60, 77, 91, 100, 113, 128, 140, 155, 161, 179, 200, 211 };
    constexpr usize nrules = sizeof(rules) / sizeof(rules[0]);

    sc::filter_builder<512> ref;
    ref.require_native_arch();
    for ( usize i = 0; i < nrules; ++i ) ref.allow(rules[i]);
    ref.default_errno(hit_errno);
    sb::require_true(ref.valid());

    usize checked = 0, flagged = 0, agreed = 0;

    // a compile-time list of capacities that straddles every residual class
    auto sweep = [&]<usize Max>(micron::integral_constant<usize, Max>) {
      sc::filter_builder<Max> fb;
      fb.require_native_arch();
      for ( usize i = 0; i < nrules; ++i ) fb.allow(rules[i]);
      fb.default_errno(hit_errno);
      ++checked;

      if ( !fb.valid() ) {
        ++flagged;
        // a rejected builder must be rejected FOR A REASON, not by accident
        sb::require_true(fb.overflowed || !fb.sealed || fb.count == 0);
        return;
      }
      ++agreed;
      // it claims to be the policy that was written; hold it to that for every probe
      for ( i32 nr = 0; nr < 256; ++nr ) {
        so::probe p = make_probe(nr, 0, 0);
        so::outcome a = so::run(fb.insns, fb.count, p);
        so::outcome b = so::run(ref.insns, ref.count, p);
        sb::require_true(a.ok() && b.ok());
        if ( a.action != b.action ) {
          sb::print("  capacity ", static_cast<i64>(Max), " diverges on nr ", static_cast<i64>(nr));
          sb::require_true(false);
        }
      }
    };

    // 8..79 covers residual 0/1/2 many times over on every arch, plus the degenerate sizes where
    // even the arch gate does not fit
    [&]<usize... Is>(micron::index_sequence<Is...>) {
      (sweep(micron::integral_constant<usize, Is + 8>{}), ...);
    }(micron::make_index_sequence<72>{});

    sb::print("  capacities swept: ", static_cast<i64>(checked), "  refused: ", static_cast<i64>(flagged),
              "  exact: ", static_cast<i64>(agreed));
    sb::require(checked, usize(72));
    sb::require_true(flagged > 0);      // small capacities must be refused
    sb::require_true(agreed > 0);       // large ones must be accepted and exact
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the arch gate is refused anywhere but instruction [0]");
  {
    // a gate emitted after a rule is only reached when nothing above returned; emitted after a
    // seal it is never reached at all. Either way the filter carries no effective arch check while
    // arch_ok would have claimed one -- so both are refused, and refused loudly
    sc::filter_builder<64> after_rule;
    after_rule.allow(test_nr).require_native_arch().default_kill();
    sb::require_true(after_rule.overflowed);
    sb::require_false(after_rule.valid());

    sc::filter_builder<64> after_seal;
    after_seal.default_allow().require_native_arch();
    sb::require_true(after_seal.overflowed);
    sb::require_false(after_seal.valid());

    // and prove the danger was real: the gate the old builder appended is dead code
    sb::require(so::run(after_seal.insns, after_seal.count, [] {
                  so::probe p{};
                  p.nr = test_nr;
                  p.arch = 0xDEADBEEFu;      // an arch that is emphatically not ours
                  return p;
                }())
                    .action,
                sc::act_allow());

    // a gate first is accepted, and kills that same foreign arch
    sc::filter_builder<64> proper;
    proper.require_native_arch().allow(test_nr).default_kill();
    sb::require_false(proper.overflowed);
    sb::require_true(proper.valid());
    so::probe foreign = make_probe(test_nr, 0, 0);
    foreign.arch = 0xDEADBEEFu;
    sb::require(so::run(proper.insns, proper.count, foreign).action, mc::posix::seccomp_ret_kill_process);

    // asking twice is idempotent, not an error
    sc::filter_builder<64> twice;
    twice.require_native_arch().require_native_arch().allow(test_nr).default_kill();
    sb::require_false(twice.overflowed);
    sb::require_true(twice.valid());
    sb::require(twice.count, proper.count);

    // the same invariant on require_arch() and require_native_arch_raw(), which reach the gate
    // without going through require_native_arch()'s own placement check. On every arch but amd64
    // these ARE the whole path, so a guard that lived only in the wrapper would protect nothing
    // there
    {
      sc::filter_builder<64> late_raw;
      late_raw.allow(test_nr).require_native_arch_raw().default_kill();
      sb::require_true(late_raw.overflowed);
      sb::require_false(late_raw.arch_ok);
      sb::require_false(late_raw.valid());
      // the gate really was appended nowhere useful: [0] is still the syscall-number load
      sb::require(late_raw.insns[0].k, mc::posix::seccomp_data_nr_off);
    }
    {
      sc::filter_builder<64> late_explicit;
      late_explicit.allow(test_nr).require_arch(sc::native_arch).default_kill();
      sb::require_true(late_explicit.overflowed);
      sb::require_false(late_explicit.arch_ok);
      sb::require_false(late_explicit.valid());
    }
    {
      sc::filter_builder<64> sealed_first;
      sealed_first.default_allow().require_arch(sc::native_arch);
      sb::require_true(sealed_first.overflowed);
      sb::require_false(sealed_first.arch_ok);
      sb::require_false(sealed_first.valid());
    }
    // and first is still fine on both spellings
    {
      sc::filter_builder<64> ok_raw;
      ok_raw.require_native_arch_raw().allow(test_nr).default_kill();
      sb::require_false(ok_raw.overflowed);
      sb::require_true(ok_raw.arch_ok);
      sb::require_true(ok_raw.valid());
      sb::require(ok_raw.insns[0].k, mc::posix::seccomp_data_arch_off);
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("valid() reads the emitted program, not the builder's own bookkeeping");
  {
    // arch_ok is a claim about what the builder was asked to do. valid() has to check what it
    // actually emitted, so that no future path can set the flag without the gate being there
    sc::filter_builder<64> fb;
    fb.require_native_arch().allow(test_nr).default_kill();
    sb::require_true(fb.valid());

    // move the gate's load off seccomp_data.arch and valid() must notice
    const u32 saved_k = fb.insns[0].k;
    fb.insns[0].k = mc::posix::seccomp_data_nr_off;
    sb::require_false(fb.valid());
    fb.insns[0].k = saved_k;
    sb::require_true(fb.valid());

    // turn it into something that is not a word load at all
    const u16 saved_code = fb.insns[0].code;
    fb.insns[0].code = static_cast<u16>(mc::bpf::ld | mc::bpf::b | mc::bpf::abs);
    sb::require_false(fb.valid());
    fb.insns[0].code = saved_code;
    sb::require_true(fb.valid());

    // the same structural test, exposed for programs that never came from a builder
    mc::bpf::fprog_t p{ static_cast<u16>(fb.count), fb.insns };
    sb::require_true(sc::prog_is_arch_gated(p));

    sc::filter_builder<64> ungated;
    ungated.allow(test_nr).default_kill();
    mc::bpf::fprog_t up{ static_cast<u16>(ungated.count), ungated.insns };
    sb::require_false(sc::prog_is_arch_gated(up));

    mc::bpf::fprog_t empty{ 0, nullptr };
    sb::require_false(sc::prog_is_arch_gated(empty));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every rule emitter flags the capacity it could not honour");
  {
    // one case per emitter. Each is sized so the rule does not fit but the seal does -- the shape
    // that used to pass valid()
    {
      sc::filter_builder<9> b;
      b.require_native_arch().allow(test_nr);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<9> b;
      b.require_native_arch().deny(test_nr);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<9> b;
      b.require_native_arch().deny_errno(test_nr, 1);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<9> b;
      b.require_native_arch().trap_syscall(test_nr);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<10> b;
      b.require_native_arch().allow_range(1, 9);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<10> b;
      b.require_native_arch().deny_range(1, 9);
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<11> b;
      b.require_native_arch().allow_batch<1, 2, 3>();
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<14> b;
      b.require_native_arch().allow_if(test_nr, sc::arg_eq(0, 1));      // 5 + 2 slots
      sb::require_true(b.overflowed);
    }
    {
      sc::filter_builder<15> b;
      b.require_native_arch().deny_if(test_nr, sc::arg_masked(0, 0xf, 0x3));      // 7 + 2 slots
      sb::require_true(b.overflowed);
    }
    // and the gate itself, when even that does not fit
    {
      sc::filter_builder<3> b;
      b.require_native_arch();
#if defined(__micron_arch_amd64)
      sb::require_true(b.overflowed);      // needs 7 with the x32 guard
      sb::require_false(b.arch_ok);
#else
      sb::require_false(b.overflowed);
#endif
    }
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the seal always has a slot, so a rule can never cost the default action");
  {
    // the reserve is what makes `overflowed` trustworthy: without it a rule could consume the last
    // instruction, and only THAT case set the flag
    auto seals = [&]<usize Max>(micron::integral_constant<usize, Max>) {
      sc::filter_builder<Max> fb;
      fb.require_native_arch();
      for ( i32 nr = 0; nr < 400; ++nr ) fb.allow(nr);
      const usize before = fb.count;
      fb.default_kill();
      sb::require_true(fb.sealed);
      sb::require(fb.count, before + 1);      // the seal ALWAYS lands
      sb::require_true(fb.count <= Max);
      sb::require((fb.insns[fb.count - 1].code & 0x07u), static_cast<u16>(mc::bpf::ret));
      sb::require_false(fb.valid());      // it overflowed, so it is still refused
    };
    [&]<usize... Is>(micron::index_sequence<Is...>) {
      (seals(micron::integral_constant<usize, Is + 8>{}), ...);
    }(micron::make_index_sequence<72>{});
  }
  sb::end_test_case();

  sb::print("=== SEC BPF ENCODE PASSED ===");
  return 1;
}
