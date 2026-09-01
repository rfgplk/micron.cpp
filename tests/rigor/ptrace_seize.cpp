//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// the ptrace wrapper's live gate: seize a real child, stop it without a signal, read and write
// one word of its memory, detach, and require that the CHILD saw the write
//
// nothing here needs privilege: a parent tracing its own child is what ptrace_scope 1 allows

#include "../../src/io/console.hpp"

#include "../../src/exit.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/linux/sys/ptrace.hpp"
#include "../../src/linux/sys/time.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr u64 seed_word = 0x1111111111111111ull;
constexpr u64 poke_word = 0x2222222222222222ull;

// the child spins on this; fork gives it the same address, so &probe is the child's address too
volatile u64 probe = seed_word;

constexpr i32 child_saw_poke = 7;
constexpr i32 child_timed_out = 8;

void
nap_ms(u64 ms)
{
  micron::timespec_t ts{};
  ts.tv_sec = static_cast<i64>(ms / 1000);
  ts.tv_nsec = static_cast<i64>((ms % 1000) * 1000000ull);
  micron::nanosleep(ts);
}

// poll waitpid rather than blocking: micron's wait4 swallows EINTR internally, so a blocking
// wait cannot be broken by anything -- timeout's own recorded trap
bool
wait_stop(i32 pid, i32 &status, u64 deadline_ms)
{
  for ( u64 waited = 0; waited < deadline_ms; waited += 2 ) {
    const i32 r = static_cast<i32>(micron::waitpid(pid, &status, micron::wnohang | micron::__wall));
    if ( r == pid ) return true;
    if ( r < 0 ) return false;
    nap_ms(2);
  }
  return false;
}

};      // namespace

int
main(void)
{
  sb::print("=== PTRACE SEIZE ===");

  test_case("a peeked word is the word the child holds, and a poked one reaches it");
  {
    const int pid = micron::try_fork();
    require_true(pid >= 0);

    if ( pid == 0 ) {
      // child: spin until the word changes, then report which it saw
      for ( u64 i = 0; i < 4000 && probe == seed_word; i++ ) nap_ms(2);
      micron::group_exit(probe == poke_word ? child_saw_poke : child_timed_out);
    }

    // seize does NOT stop the child; it keeps running until we ask
    require(micron::posix::seize(pid), 0);
    require(micron::posix::interrupt(pid), 0);

    i32 st = 0;
    require_true(wait_stop(pid, st, 4000));
    require_true(micron::wifstopped(st));
    // an interrupt on a seize'd tracee is a ptrace-stop, never a real group-stop
    require(micron::posix::ptrace_event_of(st), micron::posix::ptrace_event_stop);

    const u64 at = reinterpret_cast<u64>(const_cast<u64 *>(&probe));

    unsigned long got = 0;
    require(micron::posix::peekdata(pid, at, got), 0);
    require(static_cast<u64>(got), seed_word);

    require(micron::posix::pokedata(pid, at, static_cast<unsigned long>(poke_word)), 0);

    got = 0;
    require(micron::posix::peekdata(pid, at, got), 0);
    require(static_cast<u64>(got), poke_word);

    // the parent's own copy is untouched -- the poke went to the CHILD's address space
    require(static_cast<u64>(probe), seed_word);

    require(micron::posix::detach(pid), 0);

    i32 fin = 0;
    require_true(wait_stop(pid, fin, 8000));
    require_true(micron::wifexited(fin));
    require(micron::wexitstatus(fin), child_saw_poke);
  }
  end_test_case();

  test_case("seizing a pid that does not exist is -ESRCH, not a crash");
  {
    const i32 r = micron::posix::seize(0x7ffffffe);
    require_true(r < 0);
  }
  end_test_case();

  test_case("the option and request codes are the kernel's");
  {
    static_assert(micron::posix::ptrace_seize == 0x4206);
    static_assert(micron::posix::ptrace_interrupt == 0x4207);
    static_assert(micron::posix::ptrace_listen == 0x4208);
    static_assert(micron::posix::ptrace_detach == 17);
    static_assert(micron::posix::ptrace_peekdata == 2);
    static_assert(micron::posix::ptrace_pokedata == 5);
    static_assert(micron::posix::ptrace_o_exitkill == 0x00100000u);
    static_assert(micron::posix::ptrace_o_suspend_seccomp == 0x00200000u);
    static_assert(micron::posix::ptrace_o_traceclone == 0x00000008u);
    static_assert(micron::__wall == 0x40000000);
    static_assert(micron::posix::ptrace_word == sizeof(unsigned long));
    // a wait status carrying SIGTRAP | (EVENT_STOP << 8)
    static_assert(micron::posix::ptrace_event_of((micron::posix::ptrace_event_stop << 16) | 0x057f) == micron::posix::ptrace_event_stop);
    static_assert(micron::posix::ptrace_event_of(0x057f) == 0);
    require_true(true);
  }
  end_test_case();

  sb::print("=== PTRACE SEIZE PASSED ===");
  return 1;
}
