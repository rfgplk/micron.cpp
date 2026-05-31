// child_spawn.cpp
// Rigor tests for micron::child -- the functional porcelain over execute/rexecute.
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/errno.hpp"
#include "../../src/proc.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

namespace mc = micron;

static bool
contains(const mc::string &h, const char *n)
{
  usize hn = h.size();
  usize nn = 0;
  while ( n[nn] ) ++nn;
  if ( nn == 0 ) return true;
  if ( hn < nn ) return false;
  for ( usize i = 0; i + nn <= hn; ++i ) {
    usize j = 0;
    for ( ; j < nn; ++j )
      if ( h[i + j] != n[j] ) break;
    if ( j == nn ) return true;
  }
  return false;
}

int
main()
{
  sb::print("=== child_spawn ===");

  sb::test_case("functional pipe: echo hi -> communicate captures stdout");
  {
    auto r = mc::proc("/bin/echo") | mc::arg("hi") | mc::pipe_stdout() | mc::launch | mc::communicate_with("");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == 3);
    sb::require_true(cr.out[0] == 'h' && cr.out[1] == 'i' && cr.out[2] == '\n');
  }
  sb::end_test_case();

  sb::test_case("args(...): echo a b -> 'a b\\n'");
  {
    auto r = mc::proc("/bin/echo") | mc::args("a", "b") | mc::pipe_stdout() | mc::launch | mc::communicate_with("");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == 4 && cr.out[0] == 'a' && cr.out[1] == ' ' && cr.out[2] == 'b' && cr.out[3] == '\n');
  }
  sb::end_test_case();

  sb::test_case("exit codes: /bin/false -> code 1, /bin/true -> success");
  {
    auto rf = mc::proc("/bin/false") | mc::launch;
    sb::require_true(rf.is_first());
    auto st = rf.cast<mc::child>().wait();
    sb::require_true(st.exited && st.code == 1 && !st.success());

    auto rt = mc::proc("/bin/true") | mc::launch;
    sb::require_true(rt.is_first());
    sb::require_true(rt.cast<mc::child>().wait().success());
  }
  sb::end_test_case();

  sb::test_case("merge_stderr: sh -c 'echo out; echo err 1>&2' merged on stdout");
  {
    auto r = mc::proc("/bin/sh") | mc::arg("-c") | mc::arg("echo out; echo err 1>&2") | mc::pipe_stdout() | mc::merge_stderr() | mc::launch
             | mc::communicate_with("");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(contains(cr.out, "out"));
    sb::require_true(contains(cr.out, "err"));
  }
  sb::end_test_case();

  sb::test_case("pipe_stdin: cat echoes stdin back via communicate");
  {
    auto r = mc::proc("/bin/cat") | mc::pipe_stdin() | mc::pipe_stdout() | mc::launch | mc::communicate_with("hello");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == 5 && cr.out[0] == 'h' && cr.out[4] == 'o');
  }
  sb::end_test_case();

  sb::test_case("terminate(): sleep 5 -> wait reports signaled (SIGTERM)");
  {
    auto r = mc::proc("/bin/sleep") | mc::arg("5") | mc::launch;
    sb::require_true(r.is_first());
    auto &kid = r.cast<mc::child>();
    sb::require_true(kid.valid());
    kid.terminate();
    auto st = kid.wait();
    sb::require_true(st.signaled && st.term_signal == 15 && !st.exited);
  }
  sb::end_test_case();

  sb::test_case("null_stdout: output discarded, child still succeeds");
  {
    auto r = mc::proc("/bin/echo") | mc::arg("hi") | mc::null_stdout() | mc::launch;
    sb::require_true(r.is_first());
    sb::require_true(r.cast<mc::child>().wait().success());
  }
  sb::end_test_case();

  sb::test_case("error branch: missing binary -> option holds spawn_error ENOENT");
  {
    auto r = mc::proc("/no/such/binary_xyz") | mc::launch;
    sb::require_true(r.is_second());
    auto &e = r.cast<mc::spawn_error>();
    sb::require_true(e.err == ENOENT);
    sb::require_true(e.stage == mc::spawn_error::spawn);
  }
  sb::end_test_case();

  sb::test_case("spawn_or_throw + read_stdout");
  {
    auto kid = mc::spawn_or_throw(mc::proc("/bin/echo") | mc::arg("xy") | mc::pipe_stdout());
    mc::string out;
    kid.read_stdout(out);
    auto st = kid.wait();
    sb::require_true(st.success());
    sb::require_true(out.size() == 3 && out[0] == 'x' && out[1] == 'y' && out[2] == '\n');
  }
  sb::end_test_case();

  sb::test_case("drop policy: dropping an unwaited child does not block or abort");
  {
    {
      auto r = mc::proc("/bin/sleep") | mc::arg("1") | mc::launch;
      sb::require_true(r.is_first());
    }
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::print("=== child_spawn ALL PASSED ===");
  return 1;
}
