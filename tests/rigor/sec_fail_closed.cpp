//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/std.hpp"

#include "../../src/sec/fp.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;

namespace
{

constexpr const char *work_dir = "/var/tmp/mc_sec_failclosed";
constexpr const char *inside_file = "/var/tmp/mc_sec_failclosed/inside.txt";
constexpr const char *ran_marker = "/var/tmp/mc_sec_failclosed_RAN";
constexpr const char *outside_write = "/var/tmp/mc_sec_failclosed_OUTSIDE";

bool
exists(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

bool
touch(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0644));
  if ( fd < 0 ) return false;
  (void)mc::posix::write(fd, "x", 1);
  (void)mc::posix::close(fd);
  return true;
}

void
clear_marker(void)
{
  (void)mc::posix::unlink(ran_marker);
}

i32
marking_body(void)
{
  return touch(ran_marker) ? 71 : 72;
}

i32
trivial_ok_body(void)
{
  return 61;
}

void
fill_ungated(sc::filter_builder<256> &fb)
{
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  fb.default_errno(1);
}

void
fill_unsealed(sc::filter_builder<256> &fb)
{
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
}

void
fill_overflowed(sc::filter_builder<32> &fb)
{
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::io::count; ++i ) fb.allow(g::io::calls[i]);
  fb.default_errno(1);
}

template<usize N>
void
fill_good(sc::filter_builder<N> &fb)
{
  fb.require_native_arch();
  auto add = [&fb](const i32 *c, usize n) {
    for ( usize i = 0; i < n; ++i ) fb.allow(c[i]);
  };
  add(g::baseline::calls, g::baseline::count);
  add(g::io::calls, g::io::count);
  add(g::memory::calls, g::memory::count);
  add(g::signal::calls, g::signal::count);
  add(g::process::calls, g::process::count);
  add(g::filesystem::calls, g::filesystem::count);
  fb.default_errno(static_cast<u16>(mc::error::permissions));
}

template<typename B>
concept binds_here = requires(s::sandbox &box, B &&fb) { box.seccomp(static_cast<B &&>(fb)); };

static_assert(binds_here<sc::filter_builder<64> &>, "an lvalue builder must still bind");
static_assert(!binds_here<sc::filter_builder<64>>, "a temporary builder must NOT bind: __filter would dangle");

i32
wait_status_of(s::sandbox::child &c)
{
  int status = 0;
  (void)mc::waitpid(c.pid, &status, 0);
  return status;
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC FAIL CLOSED ===");

  (void)mc::posix::mkdir(work_dir, 0755);
  sb::require_true(touch(inside_file));
  clear_marker();
  sb::require_false(exists(ran_marker));

  sb::test_case("a builder valid() refuses is not silently dropped -- the launch is refused");
  {

    {
      sc::filter_builder<256> fb;
      fill_ungated(fb);
      sb::require_false(fb.valid());

      s::sandbox box;
      box.seccomp(fb);
      sb::require_false(box.has_seccomp());
      sb::require_false(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::seccomp));

      clear_marker();
      s::sandbox::child c = box.run(marking_body);
      sb::require_false(c.ok());
      sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::seccomp));
      sb::require(c.fault.err, -static_cast<i32>(mc::error::invalid_arg));
      sb::require_false(exists(ran_marker));
    }
    {
      sc::filter_builder<256> fb;
      fill_unsealed(fb);
      sb::require_false(fb.valid());

      s::sandbox box;
      box.seccomp(fb);
      sb::require_false(box.configured());
      clear_marker();
      s::sandbox::child c = box.run(marking_body);
      sb::require_false(c.ok());
      sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::seccomp));
      sb::require_false(exists(ran_marker));
    }
    {
      sc::filter_builder<32> fb;
      fill_overflowed(fb);
      sb::require_true(fb.overflowed);
      sb::require_false(fb.valid());

      s::sandbox box;
      box.seccomp(fb);
      sb::require_false(box.configured());
      clear_marker();
      s::sandbox::child c = box.run(marking_body);
      sb::require_false(c.ok());
      sb::require_false(exists(ran_marker));
    }
  }
  sb::end_test_case();

  sb::test_case("spawn() refuses on the same fault, and refuses BEFORE forking");
  {
    sc::filter_builder<256> fb;
    fill_ungated(fb);

    char arg0[] = "/bin/true";
    char *argv[] = { arg0, nullptr };
    char *envp[] = { nullptr };

    s::sandbox box;
    box.seccomp(fb);
    s::sandbox::child c = box.spawn("/bin/true", argv, envp);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::seccomp));
    sb::require_true(c.pid < 0);
  }
  sb::end_test_case();

  sb::test_case("a filter that IS installable still installs, so the gate is not simply refusing everything");
  {
    sc::filter_builder<1024> fb;
    fill_good(fb);
    sb::require_true(fb.valid());

    s::sandbox box;
    box.seccomp(fb);
    sb::require_true(box.has_seccomp());
    sb::require_true(box.configured());

    clear_marker();
    s::sandbox::child c = box.run(marking_body);
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 71);
    sb::require_true(exists(ran_marker));
    clear_marker();
  }
  sb::end_test_case();

  sb::test_case("sec::confined() reports the error instead of running the body unconfined");
  {

    {
      sc::filter_builder<256> fb;
      fill_ungated(fb);
      clear_marker();
      auto r = s::confined(fb, marking_body);
      sb::require_true(r.is_second());
      sb::require_false(exists(ran_marker));
    }
    {
      sc::filter_builder<32> fb;
      fill_overflowed(fb);
      clear_marker();
      auto r = s::confined(fb, marking_body);
      sb::require_true(r.is_second());
      sb::require_false(exists(ran_marker));
    }

    {
      sc::filter_builder<1024> fb;
      fill_good(fb);
      clear_marker();
      auto r = s::confined(fb, marking_body);
      sb::require_true(r.is_first());
      sb::require_true(exists(ran_marker));
      sb::require_true(r.cast<s::sandbox::exit_status>().exited());
      sb::require(r.cast<s::sandbox::exit_status>().code(), 71);
      clear_marker();
    }
  }
  sb::end_test_case();

  sb::test_case("every builder setter that runs out of room says so instead of dropping the request");
  {

    {
      s::sandbox box;
      for ( usize i = 0; i < s::sandbox::max_mounts + 3; ++i ) box.bind("/usr", "/mnt/x");
      sb::require_false(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::filesystem));
      clear_marker();
      s::sandbox::child c = box.run(marking_body);
      sb::require_false(c.ok());
      sb::require_false(exists(ran_marker));
    }
    {
      s::sandbox box;
      for ( usize i = 0; i < s::sandbox::max_landlock_rules + 3; ++i ) box.landlock(work_dir, s::landlock::read_only);
      sb::require_false(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::landlock));
    }
    {
      s::sandbox box;
      for ( usize i = 0; i < 11; ++i ) box.rlimit(mc::posix::rlimit_nofile, 64, 64);
      sb::require_false(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::rlimits));
    }
    {
      s::sandbox box;
      for ( i32 fd = 3; fd < 15; ++fd ) box.keep_fd(fd);
      sb::require_false(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::descriptors));
    }

    {
      s::sandbox box;
      for ( usize i = 0; i < s::sandbox::max_mounts + 1; ++i ) box.bind("/usr", "/mnt/x");
      for ( usize i = 0; i < 11; ++i ) box.rlimit(mc::posix::rlimit_nofile, 64, 64);
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::filesystem));
    }

    {
      s::sandbox box;
      box.bind("/usr", "/mnt/x").rlimit(mc::posix::rlimit_nofile, 64, 64).keep_fd(3);
      sb::require_true(box.configured());
      sb::require(static_cast<i32>(box.config_fault().where), static_cast<i32>(s::stage::none));
    }
  }
  sb::end_test_case();

  sb::test_case("landlock mediates what it is TOLD to mediate, not merely what it grants");
  {

    // NOTE (CVE audit): the DEFAULT is now everything-handled, and this case is inverted from how it
    // was written. Landlock restricts only what its ruleset declares it HANDLES, so defaulting
    // `handled` to the union of what was GRANTED made
    //
    //     box.landlock(dir, read_only);
    //
    // handle exactly read_file|read_dir -- leaving write_file, truncate, every make_*, remove_*,
    // execute, refer and ioctl_dev unrestricted across the WHOLE filesystem, which is the opposite of
    // what that line reads like. The old expectation is preserved below as the `narrowed` case, which
    // is what a caller now has to write on purpose to get it.
    s::sandbox def;
    def.landlock(work_dir, s::landlock::read_only);
    sb::require(s::landlock::bits(def.landlock_handled()), s::landlock::bits(s::landlock::supported_fs()));

    s::sandbox narrowed;
    narrowed.landlock_handled(s::landlock::read_only).landlock(work_dir, s::landlock::read_only);
    sb::require(s::landlock::bits(narrowed.landlock_handled()), s::landlock::bits(s::landlock::read_only));

    s::sandbox all;
    all.landlock_handled_all().landlock(work_dir, s::landlock::read_only);
    sb::require(s::landlock::bits(all.landlock_handled()), s::landlock::bits(s::landlock::supported_fs()));

    s::sandbox fixed;
    fixed.landlock_handled(s::landlock::read_only).landlock(work_dir, s::landlock::read_write);
    sb::require(s::landlock::bits(fixed.landlock_handled()), s::landlock::bits(s::landlock::read_only));

    if ( !s::landlock::available() ) {
      sb::print("  landlock unavailable on this kernel; the runtime half is skipped");
    } else {

      (void)mc::posix::unlink(outside_write);

      // the leak is still reachable -- it just has to be ASKED FOR now. Keeping this half is the
      // point: it proves the default is the only thing standing between the caller and the old
      // behaviour, rather than the behaviour having become unreachable.
      s::sandbox leaky;
      leaky.user().landlock_handled(s::landlock::read_only).landlock(work_dir, s::landlock::read_only);
      s::sandbox::child a = leaky.run([]() -> i32 { return touch(outside_write) ? 81 : 82; });
      sb::require_true(a.ok());
      sb::require(mc::wexitstatus(wait_status_of(a)), 81);
      sb::require_true(exists(outside_write));
      (void)mc::posix::unlink(outside_write);

      // and the DEFAULT now behaves the way `tight` had to be spelled out to
      s::sandbox by_default;
      by_default.user().landlock(work_dir, s::landlock::read_only);
      s::sandbox::child d = by_default.run([]() -> i32 { return touch(outside_write) ? 81 : 82; });
      sb::require_true(d.ok());
      sb::require(mc::wexitstatus(wait_status_of(d)), 82);
      sb::require_false(exists(outside_write));
      (void)mc::posix::unlink(outside_write);

      s::sandbox tight;
      tight.user().landlock_handled_all().landlock(work_dir, s::landlock::read_only);
      s::sandbox::child b = tight.run([]() -> i32 {
        const bool wrote = touch(outside_write);
        const bool read_in = exists(inside_file);
        return (!wrote && read_in) ? 83 : 84;
      });
      if ( !b.ok() ) sb::print("  faulted at stage: ", b.fault.stage_name(), " errno ", static_cast<i64>(-b.fault.err));
      sb::require_true(b.ok());
      sb::require(mc::wexitstatus(wait_status_of(b)), 83);
      sb::require_false(exists(outside_write));
    }
  }
  sb::end_test_case();

  sb::test_case("landlock_handled_all() with NO rules is total denial, not a no-op");
  {
    // handle everything, permit nothing. Gating the stage on the RULE COUNT turns the strictest
    // configuration the builder can express into no confinement at all, and config_fault() stays
    // empty while landlock_handled() still reports a full mask -- nothing visible says it lapsed
    s::sandbox seal;
    seal.user().landlock_handled_all();
    sb::require(seal.landlock_rule_count(), usize(0));
    sb::require(s::landlock::bits(seal.landlock_handled()), s::landlock::bits(s::landlock::supported_fs()));
    sb::require_true(seal.configured());

    if ( !s::landlock::available() ) {
      sb::print("  landlock unavailable on this kernel; the runtime half is skipped");
    } else {
      (void)mc::posix::unlink(outside_write);
      s::sandbox::child c = seal.run([]() -> i32 {
        const bool wrote = touch(outside_write);
        const bool read_in = exists(inside_file);
        return (!wrote && !read_in) ? 91 : 92;
      });
      if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
      sb::require_true(c.ok());
      sb::require(mc::wexitstatus(wait_status_of(c)), 91);
      sb::require_false(exists(outside_write));
    }
  }
  sb::end_test_case();

  sb::test_case("a capability drop the kernel REFUSES is reported, not swallowed");
  {
    // PR_CAPBSET_DROP wants CAP_SETPCAP. Without it every drop answers EPERM, and a sandbox that
    // discards those return values execs with the bounding set it promised to narrow.
    //
    // NOTE (CVE audit): there are THREE cases here now, not two, because drop_capabilities() became
    // the DEFAULT. Failing every unprivileged sandbox at this stage would have made that default
    // unusable -- but skipping unconditionally would be the fail-open the case was written to catch.
    // What decides it is NO_NEW_PRIVS: the bounding set gates what a setuid exec could grant, and
    // NNP forbids that transition, so with NNP on there is nothing an un-narrowed bounding set could
    // be used for. With NNP OFF there is, and the refusal has to stand.
    s::sandbox box;
    box.drop_capabilities();
    s::sandbox::child c = box.run(trivial_ok_body);

    if ( mc::has_cap(mc::cap::setpcap) ) {
      sb::print("  runner holds CAP_SETPCAP; the drop is expected to succeed");
      sb::require_true(c.ok());
      sb::require(mc::wexitstatus(wait_status_of(c)), 61);
    } else {
      // NNP is on by default, so the un-narrowable bounding set is inert and the sandbox proceeds
      sb::require_true(c.ok());
      sb::require(mc::wexitstatus(wait_status_of(c)), 61);

      // ... and with NNP turned OFF the same sandbox must refuse, because now it matters
      s::sandbox unsafe;
      unsafe.drop_capabilities().no_new_privs(false);
      s::sandbox::child u = unsafe.run(trivial_ok_body);
      sb::require_false(u.ok());
      sb::require(static_cast<i32>(u.fault.where), static_cast<i32>(s::stage::caps_pre));
      sb::require(u.fault.err, -static_cast<i32>(mc::error::permissions));
    }

    // and inside a user namespace, where CAP_SETPCAP IS held, the same sandbox goes through
    s::sandbox ok;
    ok.user().drop_capabilities();
    s::sandbox::child d = ok.run(trivial_ok_body);
    if ( !d.ok() ) sb::print("  userns faulted at stage: ", d.fault.stage_name(), " errno ", static_cast<i64>(-d.fault.err));
    sb::require_true(d.ok());
    sb::require(mc::wexitstatus(wait_status_of(d)), 61);
  }
  sb::end_test_case();

  sb::test_case("the runner never became confined by any of the above");
  {
    sb::require(mc::prctl(mc::PR_GET_NO_NEW_PRIVS), 0);
    sb::require(mc::prctl(mc::PR_GET_SECCOMP), 0);
    sb::require_true(exists(inside_file));
    sb::require_true(touch(outside_write));
    (void)mc::posix::unlink(outside_write);
    clear_marker();
  }
  sb::end_test_case();

  sb::print("=== SEC FAIL CLOSED PASSED ===");
  return 1;
}
