// activation_record.cpp

#include "../../src/bits/ar.hpp"
#include "../../src/memory/mman.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

namespace mc = micron;
namespace ar = micron::ar;

static usize
absdiff(uintptr_t a, uintptr_t b) noexcept
{
  return a > b ? a - b : b - a;
}

static volatile uintptr_t g_inner_sp = 0;
static void *g_alt_lo = nullptr;
static usize g_alt_sz = 0;

[[gnu::noinline]] static void
on_alt(void *arg)
{
  g_inner_sp = reinterpret_cast<uintptr_t>(ar::stack_pointer());
  *static_cast<volatile int *>(arg) = 7;
}

int
main()
{
  sb::print("=== activation_record ===");

  sb::test_case("stack_pointer(): non-null and adjacent to a local variable");
  {
    int local = 0;
    uintptr_t sp = reinterpret_cast<uintptr_t>(ar::stack_pointer());
    sb::require_true(sp != 0);
    sb::require_true(absdiff(sp, reinterpret_cast<uintptr_t>(&local)) < 65536);
  }
  sb::end_test_case();

  sb::test_case("frame_pointer() / program_counter() / frame_address<0>() non-null");
  {
    sb::require_true(ar::frame_pointer() != nullptr);
    sb::require_true(ar::program_counter() != nullptr);
    sb::require_true(ar::frame_address<0>() != nullptr);
    sb::require_true(ar::return_address<0>() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("program_counter(): two reads are close (same function body)");
  {
    uintptr_t a = reinterpret_cast<uintptr_t>(ar::program_counter());
    uintptr_t b = reinterpret_cast<uintptr_t>(ar::program_counter());
    sb::require_true(a != 0 && b != 0);
    sb::require_true(absdiff(a, b) < 4096);
  }
  sb::end_test_case();

  sb::test_case("thread_pointer(): non-null (TCB present)");
  {
    sb::require_true(ar::thread_pointer() != nullptr);
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)

  sb::test_case("x86 segments: cs/ss readable and nonzero; ds/es/fs/gs readable");
  {
    sb::require_true(ar::code_segment() != 0);
    sb::require_true(ar::stack_segment() != 0);
    (void)ar::data_segment();
    (void)ar::extra_segment();
    (void)ar::fs_segment();
    (void)ar::gs_segment();
    sb::require_true(true);
  }
  sb::end_test_case();
#endif

  sb::test_case("here() snapshot is self-consistent; caller() walks without crashing");
  {
    ar::record r = ar::here();
    sb::require_true(r.sp != nullptr && r.fp != nullptr && r.pc != nullptr);
    int hops = 0;
    while ( hops < 32 && ar::caller(r) ) ++hops;
    sb::require_true(hops >= 0);
  }
  sb::end_test_case();

  sb::test_case("call_on_stack(): fn runs on the foreign stack; caller SP restored");
  {
    constexpr usize SZ = 128 * 1024;
    void *alt = reinterpret_cast<void *>(mc::mmap(nullptr, SZ, mc::prot_read | mc::prot_write, mc::map_private | mc::map_anonymous, -1, 0));
    sb::require_true(!mc::mmap_failed(reinterpret_cast<int *>(alt)));
    g_alt_lo = alt;
    g_alt_sz = SZ;

    uintptr_t before = reinterpret_cast<uintptr_t>(ar::stack_pointer());
    int sentinel = 0;
    g_inner_sp = 0;
    ar::call_on_stack(reinterpret_cast<char *>(alt) + SZ, &on_alt, &sentinel);
    uintptr_t after = reinterpret_cast<uintptr_t>(ar::stack_pointer());

    bool executed = (sentinel == 7);

    bool on_foreign = (g_inner_sp >= reinterpret_cast<uintptr_t>(alt)) && (g_inner_sp <= reinterpret_cast<uintptr_t>(alt) + SZ);
    bool restored = (before == after);
    mc::munmap(reinterpret_cast<addr_t *>(alt), SZ);

    sb::require_true(executed);
    sb::require_true(on_foreign);
    sb::require_true(restored);
  }
  sb::end_test_case();

  sb::print("=== activation_record ALL PASSED ===");
  return 1;
}
