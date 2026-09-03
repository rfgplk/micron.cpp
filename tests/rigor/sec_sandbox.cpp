//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::sandbox end to end.
//
// The claim under test is not "it runs" -- an empty sandbox runs. It is that every stage the
// caller asked for ACTUALLY TOOK EFFECT, in the right order. So the confined body probes its own
// state (namespaces, uid, no_new_privs, seccomp mode, landlock reachability, signal mask) and
// returns a bitmask the parent requires EXACTLY. A stage that silently did nothing flips its bit
// off and fails the case, where a "did it crash?" test would pass.
//
// The second half drives failures on purpose and requires the error pipe to name the stage.

#include "../../src/std.hpp"

#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;

namespace
{

constexpr const char *work_dir = "/var/tmp/mc_sec_sandbox";
constexpr const char *work_file = "/var/tmp/mc_sec_sandbox/inside.txt";
constexpr const char *outside_file = "/var/tmp/mc_sec_sandbox_outside.txt";

// probe bits the confined body reports
constexpr i32 bit_userns = 1 << 0;
constexpr i32 bit_uid_root = 1 << 1;
constexpr i32 bit_nnp = 1 << 2;
constexpr i32 bit_seccomp = 1 << 3;
constexpr i32 bit_landlock_in = 1 << 4;
constexpr i32 bit_landlock_out = 1 << 5;
constexpr i32 bit_pidns = 1 << 6;
constexpr i32 bit_mask_clear = 1 << 7;

bool
write_file(const char *path, const char *what)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0644));
  if ( fd < 0 ) return false;
  usize n = 0;
  while ( what[n] ) ++n;
  const bool ok = mc::posix::write(fd, what, n) == static_cast<max_t>(n);
  (void)mc::posix::close(fd);
  return ok;
}

bool
can_read(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

// the confined body. every bit it sets is a stage that demonstrably happened
i32
probe_body(void)
{
  i32 bits = 0;

  if ( !s::ns::open_self(s::ns::ns_kind::user).is_host() ) bits |= bit_userns;
  if ( mc::posix::geteuid() == 0 ) bits |= bit_uid_root;
  if ( mc::prctl(mc::PR_GET_NO_NEW_PRIVS) == 1 ) bits |= bit_nnp;
  if ( mc::prctl(mc::PR_GET_SECCOMP) == static_cast<i32>(mc::posix::seccomp_mode_filter) ) bits |= bit_seccomp;
  if ( can_read(work_file) ) bits |= bit_landlock_in;
  if ( !can_read(outside_file) ) bits |= bit_landlock_out;
  if ( mc::posix::getpid() == 1 ) bits |= bit_pidns;

  mc::posix::sigset_t cur{};
  if ( mc::posix::sigprocmask(mc::posix::sig_setmask, cur, &cur) == 0 ) bits |= bit_mask_clear;

  return bits;
}

i32
trivial_body(void)
{
  return 5;
}

// a seccomp filter that permits everything the probe body needs
template<usize N>
void
fill_runtime_filter(s::seccomp::filter_builder<N> &fb)
{
  fb.require_native_arch();
  auto add = [&fb](const i32 *c, usize n) {
    for ( usize i = 0; i < n; ++i ) fb.allow(c[i]);
  };
  add(s::groups::baseline::calls, s::groups::baseline::count);
  add(s::groups::io::calls, s::groups::io::count);
  add(s::groups::memory::calls, s::groups::memory::count);
  add(s::groups::signal::calls, s::groups::signal::count);
  add(s::groups::process::calls, s::groups::process::count);
  add(s::groups::capabilities::calls, s::groups::capabilities::count);
  add(s::groups::filesystem::calls, s::groups::filesystem::count);
  fb.default_errno(static_cast<u16>(mc::error::permissions));
}

i32
wait_status_of(s::sandbox::child &c)
{
  int status = 0;
  (void)mc::waitpid(c.pid, &status, 0);
  return status;
}

// the gate the asynchrony case parks its body on. fork() copies it, so the child reads the same fd
i32 g_gate_read = -1;

i32
gated_body(void)
{
  char b = 0;
  return mc::posix::read(g_gate_read, &b, 1) == 1 ? 21 : 22;
}

// reads three bytes from fd 0, writes two to fd 1
i32
stdio_probe(void)
{
  char b[4] = {};
  if ( mc::posix::read(0, b, 3) != 3 || b[0] != 'S' ) return 24;
  return mc::posix::write(1, "OK", 2) == 2 ? 23 : 25;
}

bool
file_is(const char *path, const char *want)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return false;
  char b[64] = {};
  const max_t n = mc::posix::read(fd, b, sizeof(b) - 1);
  (void)mc::posix::close(fd);
  if ( n < 0 ) return false;
  usize i = 0;
  while ( want[i] && b[i] == want[i] ) ++i;
  return want[i] == '\0' && static_cast<usize>(n) == i;
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC SANDBOX ===");

  (void)mc::posix::mkdir(work_dir, 0755);
  sb::require_true(write_file(work_file, "inside"));
  sb::require_true(write_file(outside_file, "outside"));

  // ---------------------------------------------------------------- //
  sb::test_case("an empty sandbox runs a body and hands back its exit status");
  {
    s::sandbox box;
    s::sandbox::child c = box.run(trivial_body);
    sb::require_true(c.ok());
    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));
    sb::require(mc::wexitstatus(st), 5);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("EVERY configured stage actually took effect inside the child");
  {
    s::seccomp::filter_builder<1024> fb;
    fill_runtime_filter(fb);
    sb::require_true(fb.valid());

    s::sandbox box;
    box.user()
        .mount_ns()
        .pid_ns()
        .net()
        .uts()
        .ipc()
        .landlock(work_dir, s::landlock::read_only)
        .no_new_privs()
        .seccomp(fb);

    sb::require_true(s::ns::any(box.kinds() & s::ns::ns_kind::user));
    sb::require_true(box.has_seccomp());
    sb::require(box.landlock_rule_count(), usize(1));

    s::sandbox::child c = box.run(probe_body);
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));

    // EXACT equality: an extra bit would mean a stage fired that was not asked for, a missing one
    // that a stage was silently skipped
    const i32 want = bit_userns | bit_uid_root | bit_nnp | bit_seccomp | bit_landlock_in | bit_landlock_out
                     | bit_pidns | bit_mask_clear;
    const i32 got = mc::wexitstatus(st);
    if ( got != want )
      sb::print("  probe bits got=", static_cast<i64>(got), " want=", static_cast<i64>(want),
                " missing=", static_cast<i64>(want & ~got), " extra=", static_cast<i64>(got & ~want));
    sb::require(got, want);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("landlock confines the child even with no seccomp filter at all");
  {
    s::sandbox box;
    box.user().landlock(work_dir, s::landlock::read_only);
    s::sandbox::child c = box.run([]() -> i32 {
      i32 b = 0;
      if ( can_read(work_file) ) b |= bit_landlock_in;
      if ( !can_read(outside_file) ) b |= bit_landlock_out;
      return b;
    });
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), bit_landlock_in | bit_landlock_out);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("seccomp is installed BEFORE the credential drop, so the drop still works");
  {
    // this is the ordering claim. the filter denies nothing the drop needs, and the drop is
    // requested -- if seccomp ran last, or if the policy lost setuid, the child would fault at the
    // credentials stage instead of reaching its body
    s::seccomp::filter_builder<1024> fb;
    fill_runtime_filter(fb);

    s::sandbox box;
    box.user().seccomp(fb).as_user(0, 0).drop_capabilities();

    s::sandbox::child c = box.run([]() -> i32 { return mc::posix::geteuid() == 0 ? 11 : 12; });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 11);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a failing stage is named by the error pipe rather than guessed from an exit code");
  {
    // pivot_root into a directory that is not a mount point: EINVAL, at the filesystem stage
    s::sandbox box;
    box.user().mount_ns().root("/var/tmp/mc_sec_sandbox_absent", "/var/tmp/mc_sec_sandbox_absent/old");
    s::sandbox::child c = box.run(trivial_body);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::filesystem));
    sb::require_true(c.fault.err < 0);
    sb::require_true(c.fault.stage_name() != nullptr);
    sb::print("  faulted at stage: ", c.fault.stage_name(), "  errno ", static_cast<i64>(-c.fault.err));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a landlock rule naming a path that does not exist faults at the landlock stage");
  {
    s::sandbox box;
    box.user().landlock("/var/tmp/mc_sec_sandbox_absent_path", s::landlock::read_only);
    s::sandbox::child c = box.run(trivial_body);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::landlock));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a credential change that cannot take faults at the credentials stage");
  {
    // no user namespace, so an unprivileged process cannot become uid 0 -- and the sandbox must
    // notice rather than run the body believing it dropped
    s::sandbox box;
    box.as_user(0, 0);
    s::sandbox::child c = box.run(trivial_body);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::credentials));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("spawn() execs a real binary inside the confinement");
  {
    s::seccomp::filter_builder<1024> fb;
    fill_runtime_filter(fb);

    char arg0[] = "/bin/true";
    char *argv[] = { arg0, nullptr };
    char *envp[] = { nullptr };

    s::sandbox box;
    box.user().pid_ns().net().no_new_privs().seccomp(fb);
    s::sandbox::child c = box.spawn("/bin/true", argv, envp);
    sb::require_true(c.ok());
    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));
    sb::require(mc::wexitstatus(st), 0);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a spawn of a path that does not exist is reported as an exec fault, not a hang");
  {
    char arg0[] = "/nonexistent/mc_sec";
    char *argv[] = { arg0, nullptr };
    char *envp[] = { nullptr };
    s::sandbox box;
    s::sandbox::child c = box.spawn("/nonexistent/mc_sec", argv, envp);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::exec));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("run() is a SPAWN: it returns while the body is still running");
  {
    // the body parks on a pipe nobody has written to yet. run() must come back anyway -- if the
    // child still held the fault pipe across the body, __launch's read() could not return until the
    // body exited, and the two would deadlock. That is a HANG pre-fix, which the --timeout grades
    i32 gate[2] = { -1, -1 };
    sb::require(mc::posix::pipe2(gate, 0), 0);
    g_gate_read = gate[0];

    s::sandbox box;
    // NOTE (CVE audit): the gate descriptor has to be KEPT now. The descriptor sweep is on by
    // default -- an inherited dirfd ignores pivot_root, and with procfs mounted an inherited
    // anything is re-openable through /proc/self/fd (CVE-2024-21626 / CVE-2019-5736) -- so a body
    // that expects to inherit a pipe has to say so. Which is the feature working: this is a channel
    // the caller deliberately hands the child, and keep_fd is how that is spelled.
    box.keep_fd(gate[0]);
    s::sandbox::child c = box.run(gated_body);
    sb::require_true(c.ok());
    sb::require_true(c.pid > 0);

    // still running: nothing has unblocked it
    int peek = 0;
    sb::require(mc::waitpid(c.pid, &peek, mc::wnohang), 0);

    sb::require(mc::posix::write(gate[1], "g", 1), 1);
    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));
    sb::require(mc::wexitstatus(st), 21);

    (void)mc::posix::close(gate[0]);
    (void)mc::posix::close(gate[1]);
    g_gate_read = -1;
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("stdio() sources below 3 are lifted clear instead of being clobbered mid-sequence");
  {
    // stdio(in, 0, 2) asks for "stdout goes where the caller's fd 0 currently goes". Assigning in
    // fixed 0,1,2 order does dup2(in, 0) first and then duplicates the NEW fd 0 onto 1, which wires
    // the child's stdout to its own input file
    const char *in_path = "/var/tmp/mc_sec_sandbox/stdio_in.txt";
    const char *out_path = "/var/tmp/mc_sec_sandbox/stdio_out.txt";
    sb::require_true(write_file(in_path, "SRC"));
    sb::require_true(write_file(out_path, ""));

    const i32 saved0 = mc::posix::dup(0);
    sb::require_true(saved0 >= 3);
    const i32 outf = static_cast<i32>(mc::posix::open(out_path, mc::posix::o_wronly | mc::posix::o_trunc, 0));
    sb::require_true(outf >= 0);
    sb::require(mc::posix::dup2(outf, 0), 0);
    (void)mc::posix::close(outf);

    const i32 inf = static_cast<i32>(mc::posix::open(in_path, mc::posix::o_rdonly, 0));
    sb::require_true(inf >= 3);

    s::sandbox box;
    box.stdio(inf, 0, 2);
    s::sandbox::child c = box.run(stdio_probe);
    const bool launched = c.ok();
    const i32 st = launched ? wait_status_of(c) : 0;

    // the runner's own stdin goes back before anything is asserted
    sb::require(mc::posix::dup2(saved0, 0), 0);
    (void)mc::posix::close(saved0);
    (void)mc::posix::close(inf);

    sb::require_true(launched);
    sb::require_true(mc::wifexited(st));
    sb::require(mc::wexitstatus(st), 23);
    sb::require_true(file_is(out_path, "OK"));
    sb::require_true(file_is(in_path, "SRC"));      // the input file was never written through
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("run_to_completion() hands back a TYPED status, not a bare integer");
  {
    auto r = s::sandbox{}.run_to_completion(trivial_body);
    sb::require_true(r.is_first());
    const s::sandbox::exit_status es = r.cast<s::sandbox::exit_status>();
    sb::require_true(es.exited());
    sb::require_false(es.signaled());
    sb::require(es.code(), 5);
    sb::require(es.raw(), 5 << 8);      // and raw() is the encoded status, which is NOT the code
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the parent is untouched by every sandbox above");
  {
    sb::require_true(can_read(work_file));
    sb::require_true(can_read(outside_file));
    sb::require(mc::prctl(mc::PR_GET_NO_NEW_PRIVS), 0);
    sb::require(mc::prctl(mc::PR_GET_SECCOMP), 0);
    sb::require_true(s::ns::open_self(s::ns::ns_kind::user).valid());
    sb::require_true(mc::posix::geteuid() != 0);
  }
  sb::end_test_case();

  sb::print("=== SEC SANDBOX PASSED ===");
  return 1;
}
