#include "../../src/errno.hpp"
#include "../../src/proc.hpp"
#include "../../src/string/strings.hpp"

#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/std.hpp"
#include "../../src/linux/sys/sysinfo.hpp"
#include "../../src/linux/users.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

namespace mc = micron;

static int
fork_const_42()
{
  return 42;
}

static int
fork_add(int a, int b)
{
  return a + b;
}

static int
fork_add3(int a, int b, int c)
{
  return a + b + c;
}

int
main()
{
  sb::print("=== linux_procs ===");

  sb::test_case("fork(): child exits with code, parent waitpid decodes it");
  {
    int pid = mc::fork();
    if ( pid == 0 ) {
      mc::syscall(SYS_exit_group, 17);
    }
    sb::require_true(pid > 0);
    int st = 0;
    mc::waitpid(pid, &st, 0);
    sb::require_true(mc::wifexited(st));
    sb::require_true(mc::wexitstatus(st) == 17);
  }
  sb::end_test_case();

  sb::test_case("fork<Fn>(): no-arg function -> child exits 42");
  {
    int pid = mc::fork<fork_const_42>();
    sb::require_true(pid > 0);
    int st = 0;
    mc::waitpid(pid, &st, 0);
    sb::require_true(mc::wifexited(st) && mc::wexitstatus(st) == 42);
  }
  sb::end_test_case();

  sb::test_case("fork<Fn>(args): forwarded args arrive intact in the COW child");
  {
    int pid = mc::fork<fork_add>(5, 9);
    sb::require_true(pid > 0);
    int st = 0;
    mc::waitpid(pid, &st, 0);
    sb::require_true(mc::wifexited(st) && mc::wexitstatus(st) == 14);

    int pid2 = mc::fork<fork_add3>(11, 22, 33);
    sb::require_true(pid2 > 0);
    int st2 = 0;
    mc::waitpid(pid2, &st2, 0);
    sb::require_true(mc::wifexited(st2) && mc::wexitstatus(st2) == 66);
  }
  sb::end_test_case();

  sb::test_case("wfork<Fn>(args): fork+wait returns the child's exit code");
  {
    int rc = mc::wfork<fork_add>(40, 2);
    sb::require_true(rc == 42);
  }
  sb::end_test_case();

  sb::test_case("child: echo hi -> communicate captures 'hi\\n'");
  {
    auto r = mc::proc("/bin/echo") | mc::arg("hi") | mc::pipe_stdout() | mc::launch | mc::communicate_with("");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == 3 && cr.out[0] == 'h' && cr.out[1] == 'i' && cr.out[2] == '\n');
  }
  sb::end_test_case();

  sb::test_case("argv order: echo a b c -> 'a b c\\n' (no reversal)");
  {
    auto r = mc::proc("/bin/echo") | mc::args("a", "b", "c") | mc::pipe_stdout() | mc::launch | mc::communicate_with("");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == 6);
    sb::require_true(cr.out[0] == 'a' && cr.out[2] == 'b' && cr.out[4] == 'c' && cr.out[5] == '\n');
  }
  sb::end_test_case();

  sb::test_case("communicate(): 256 KiB round-trips through cat (no deadlock)");
  {
    mc::string big;
    const char *chunk = "0123456789ABCDEF";
    for ( int i = 0; i < 16 * 1024; ++i ) big += chunk;
    const usize n = big.size();
    sb::require_true(n == 256u * 1024u);

    auto r = mc::proc("/bin/cat") | mc::pipe_stdin() | mc::pipe_stdout() | mc::launch | mc::communicate_with(big);
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.status.success());
    sb::require_true(cr.out.size() == n);
    sb::require_true(cr.out[0] == '0' && cr.out[15] == 'F');
    sb::require_true(cr.out[n - 1] == 'F' && cr.out[n / 2] == big[n / 2]);
  }
  sb::end_test_case();

  sb::test_case("cat echoes small stdin; /bin/false=1, /bin/true=0");
  {
    auto r = mc::proc("/bin/cat") | mc::pipe_stdin() | mc::pipe_stdout() | mc::launch | mc::communicate_with("hello");
    sb::require_true(r.is_first());
    auto &cr = r.cast<mc::comm_result>();
    sb::require_true(cr.out.size() == 5 && cr.out[0] == 'h' && cr.out[4] == 'o');

    auto rf = mc::proc("/bin/false") | mc::launch;
    sb::require_true(rf.is_first() && rf.cast<mc::child>().wait().code == 1);
    auto rt = mc::proc("/bin/true") | mc::launch;
    sb::require_true(rt.is_first() && rt.cast<mc::child>().wait().success());
  }
  sb::end_test_case();

  sb::test_case("terminate(): sleep -> wait reports SIGTERM(15)");
  {
    auto r = mc::proc("/bin/sleep") | mc::arg("5") | mc::launch;
    sb::require_true(r.is_first());
    auto &kid = r.cast<mc::child>();
    kid.terminate();
    auto st = kid.wait();
    sb::require_true(st.signaled && st.term_signal == 15 && !st.exited);
  }
  sb::end_test_case();

  sb::test_case("spawn error: missing binary -> spawn_error ENOENT");
  {
    auto r = mc::proc("/no/such/binary_zz") | mc::launch;
    sb::require_true(r.is_second());
    sb::require_true(r.cast<mc::spawn_error>().err == ENOENT);
  }
  sb::end_test_case();

  sb::test_case("getpagesizelive(): nonzero, power-of-two, cached-stable");
  {
    usize p = mc::getpagesizelive();
    usize p2 = mc::getpagesizelive();
    sb::require_true(p == p2);
    sb::require_true(p >= 4096);
    sb::require_true((p & (p - 1)) == 0);
  }
  sb::end_test_case();

  sb::test_case("sysinfo: totalram > 0, mem_unit > 0");
  {
    mc::sysinfo si;
    sb::require_true(si.totalram > 0);
    sb::require_true(si.mem_unit > 0);
  }
  sb::end_test_case();

  sb::test_case("getuid/geteuid/getgid/getegid: consistent & in range");
  {
    auto uid = mc::getuid();
    auto eu = mc::geteuid();
    auto gid = mc::getgid();
    auto eg = mc::posix::getegid();
    sb::require_true(uid == eu);
    sb::require_true(gid == eg);
    sb::require_true(static_cast<u64>(uid) < 0xFFFFFFFFu);
    sb::require_true(mc::getpid() > 0);
  }
  sb::end_test_case();

  sb::test_case("users: getpwuid(0)=root, getpwnam(root).uid=0, huge uid misses");
  {
    mc::posix::passwd_t pw{};
    bool ok = mc::posix::getpwuid(0, pw);
    sb::require_true(ok);
    sb::require_true(pw.pw_uid == 0);

    mc::posix::passwd_t pw2{};
    bool ok2 = mc::posix::getpwnam("root", pw2);
    sb::require_true(ok2 && pw2.pw_uid == 0);

    mc::posix::passwd_t pw3{};
    bool ok3 = mc::posix::getpwuid(4000000000u, pw3);
    sb::require_true(!ok3);
  }
  sb::end_test_case();

  sb::test_case("stat(/bin/sh): regular file, size > 0");
  {
    mc::posix::stat_t st{};
    int r = mc::posix::stat("/bin/sh", st);
    sb::require_true(r == 0);
    sb::require_true(st.st_size > 0);
    sb::require_true((st.st_mode & 0170000u) == 0100000u);
  }
  sb::end_test_case();

  sb::test_case("32 sequential echo spawns all succeed (no fd/zombie leak)");
  {
    bool all = true;
    for ( int i = 0; i < 32; ++i ) {
      auto r = mc::proc("/bin/true") | mc::launch;
      if ( !r.is_first() || !r.cast<mc::child>().wait().success() ) {
        all = false;
        break;
      }
    }
    sb::require_true(all);
  }
  sb::end_test_case();

  sb::print("=== linux_procs ALL PASSED ===");
  return 1;
}
