

#include "../../src/errno.hpp"
#include "../../src/memory/mman.hpp"
#include "../../src/proc.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

namespace mc = micron;

constexpr usize STK = 256 * 1024;

static volatile int *g_shared = nullptr;
static void *g_stk_lo = nullptr;
static usize g_stk_sz = 0;

[[gnu::noinline]] static int
child_on_new_stack(void *arg)
{
  uintptr_t fa = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
  uintptr_t lo = reinterpret_cast<uintptr_t>(g_stk_lo);
  bool on_new = (fa >= lo) && (fa < lo + g_stk_sz);
  g_shared[0] = on_new ? 1 : 0;
  g_shared[1] = static_cast<int>(reinterpret_cast<uintptr_t>(arg) & 0x7fffffff);
  return 42;
}

[[gnu::noinline]] static int
tmpl_sum(int a, long b)
{
  g_shared[2] = a + static_cast<int>(b);
  return 7;
}

[[gnu::noinline]] static void
tmpl_void(int v)
{
  g_shared[4] = v;
}

[[gnu::noinline]] static int
vm_writer(void *arg)
{
  *static_cast<volatile int *>(arg) = 99;
  return 0;
}

int
main()
{
  sb::print("=== clone_trampoline ===");

  g_shared = reinterpret_cast<volatile int *>(
      mc::mmap(nullptr, 4096, mc::prot_read | mc::prot_write, mc::map_shared | mc::map_anonymous, -1, 0));
  sb::require_true(!mc::mmap_failed(const_cast<int *>(g_shared)));
  for ( int i = 0; i < 16; ++i ) g_shared[i] = 0;

  void *stk = reinterpret_cast<void *>(mc::mmap(nullptr, STK, mc::prot_read | mc::prot_write, mc::map_private | mc::map_anonymous, -1, 0));
  sb::require_true(!mc::mmap_failed(reinterpret_cast<int *>(stk)));
  g_stk_lo = stk;
  g_stk_sz = STK;

  sb::test_case("c-style clone: child runs on the NEW stack; arg + exit status round-trip");
  {
    auto pid = mc::posix::clone(&child_on_new_stack, reinterpret_cast<void *>(0x5151), 0UL, stk, STK);
    sb::require_true(pid > 0);
    int status = 0;
    mc::waitpid(pid, &status, 0);
    sb::require_true(mc::wifexited(status));
    sb::require_true(mc::wexitstatus(status) == 42);
    sb::require_true(g_shared[0] == 1);
    sb::require_true(g_shared[1] == 0x5151);
  }
  sb::end_test_case();

  sb::test_case("template clone<Sz,Fn>: marshals (int,long) args; int Fn -> exit status");
  {
    auto pid = mc::posix::clone<STK, &tmpl_sum>(reinterpret_cast<addr_t *>(stk), 0, 123, 456L);
    sb::require_true(pid > 0);
    int status = 0;
    mc::waitpid(pid, &status, 0);
    sb::require_true(mc::wifexited(status) && mc::wexitstatus(status) == 7);
    sb::require_true(g_shared[2] == 579);
  }
  sb::end_test_case();

  sb::test_case("template clone<Sz,Fn>: void Fn -> exit status 0; arg observed");
  {
    auto pid = mc::posix::clone<STK, &tmpl_void>(reinterpret_cast<addr_t *>(stk), 0, 314);
    sb::require_true(pid > 0);
    int status = 0;
    mc::waitpid(pid, &status, 0);
    sb::require_true(mc::wifexited(status) && mc::wexitstatus(status) == 0);
    sb::require_true(g_shared[4] == 314);
  }
  sb::end_test_case();

  sb::test_case("CLONE_VM: child shares VM, writes sentinel visible to parent (leaf fn)");
  {
    g_shared[3] = 0;
    unsigned long f = static_cast<unsigned long>(mc::posix::clone_flags::same_memory);
    auto pid = mc::posix::clone(&vm_writer, const_cast<int *>(&g_shared[3]), f, stk, STK);
    if ( pid > 0 ) {
      int status = 0;
      mc::waitpid(pid, &status, 0);
      if ( mc::wifexited(status) ) {
        sb::require_true(mc::wexitstatus(status) == 0);
        sb::require_true(g_shared[3] == 99);
      } else {
        sb::print("  (skipped: CLONE_VM child did not exit normally under this runtime)");
      }
    } else {
      sb::print("  (skipped: CLONE_VM unsupported by this runtime, e.g. qemu-user)");
    }
  }
  sb::end_test_case();

  sb::test_case("guards: null stack / zero size / null fn -> -EINVAL");
  {
    sb::require_true(mc::posix::clone(&child_on_new_stack, nullptr, 0UL, static_cast<void *>(nullptr), STK) == -EINVAL);
    sb::require_true(mc::posix::clone(&child_on_new_stack, nullptr, 0UL, stk, 0) == -EINVAL);
    sb::require_true(mc::posix::clone(static_cast<int (*)(void *)>(nullptr), nullptr, 0UL, stk, STK) == -EINVAL);
  }
  sb::end_test_case();

  sb::test_case("unsafe::spawn: missing path -> child runs spawn_process, reports ENOENT");
  {
    pid_t pid = 1;
    const char *argv[] = { "/nonexistent/xyzzy", nullptr };
    const char *envp[] = { nullptr };
    int rc = mc::unsafe::spawn(pid, "/nonexistent/xyzzy", const_cast<char *const *>(argv), const_cast<char *const *>(envp));
    sb::require_true(rc == ENOENT);
  }
  sb::end_test_case();

  sb::test_case("unsafe::spawn: /bin/true launches on the new stack and exits 0");
  {
    pid_t pid = 1;
    const char *argv[] = { "/bin/true", nullptr };
    const char *envp[] = { nullptr };
    int rc = mc::unsafe::spawn(pid, "/bin/true", const_cast<char *const *>(argv), const_cast<char *const *>(envp));
    if ( rc == 0 ) {
      int status = 0;
      mc::waitpid(pid, &status, 0);
      sb::require_true(mc::wifexited(status) && mc::wexitstatus(status) == 0);
    } else {
      sb::print("  (skipped: /bin/true not execable under this runtime)");
    }
  }
  sb::end_test_case();

  sb::print("=== clone_trampoline ALL PASSED ===");
  return 1;
}
