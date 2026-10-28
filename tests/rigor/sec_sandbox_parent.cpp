// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/sec/sandbox.hpp"
#include "../snowball/snowball.hpp"

int
park()
{
  for ( ;; ) micron::yield();
}

int
main()
{
  using namespace micron;
  using namespace micron::posix;
  sb::test_case("parent lifetime refuses incompatible transitions before fork");
  sec::sandbox pidns;
  const auto failed = pidns.parent_death_signal().pid_ns().run(park);
  sb::require_true(failed.pid < 0 && failed.fault.where == sec::stage::parent_lifetime);
  sb::end_test_case();

  sb::test_case("sandbox arms parent-death before callback, and parent death kills the confined child");
  sb::require_true(prctl(PR_SET_CHILD_SUBREAPER, 1) == 0);
  int pipe[2];
  sb::require_true(pipe2(pipe, o_cloexec) == 0);
  const int supervisor = try_fork();
  sb::require_true(supervisor >= 0);
  if ( !supervisor ) {
    close(pipe[0]);
    sec::sandbox sandbox;
    const auto child = sandbox.parent_death_signal().run(park);
    sb::require_true(child.ok() && child.pid > 0);
    sb::require_true(write(pipe[1], &child.pid, sizeof(child.pid)) == sizeof(child.pid));
    for ( ;; ) micron::yield();
  }
  close(pipe[1]);
  int child = -1;
  sb::require_true(read(pipe[0], &child, sizeof(child)) == sizeof(child) && child > 0);
  close(pipe[0]);
  sb::require_true(kill(supervisor, sig_kill) == 0);
  int state = 0;
  sb::require_true(waitpid(supervisor, &state, 0) == supervisor && wifsignaled(state) && wtermsig(state) == sig_kill);
  sb::require_true(waitpid(child, &state, 0) == child && wifsignaled(state) && wtermsig(state) == sig_kill);
  sb::end_test_case();

  sb::test_case("failure to arm the parent-death control is reported at its own setup stage");
  const int probe = try_fork();
  sb::require_true(probe >= 0);
  if ( !probe ) {
    sec::seccomp::filter_builder<32> filter;
    filter.require_native_arch().deny_errno(SYS_prctl, error::permissions).default_allow();
    sb::require_true(sec::seccomp::load(filter) == 0);
    sec::sandbox sandbox;
    const auto child = sandbox.parent_death_signal().run(park);
    sb::require_true(child.pid < 0 && child.fault.where == sec::stage::parent_lifetime && child.fault.err == -error::permissions);
    micron::sys_group_exit(1);
  }
  sb::require_true(waitpid(probe, &state, 0) == probe && wifexited(state) && wexitstatus(state) == 1);
  sb::end_test_case();
  return 1;
}
