//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// spawning with no /proc at all
//
// build this with -DMICRON_TLS_NO_PROC: __tls_capture_from_proc_auxv() then hard-returns false, which is exactly what a chroot, a minimal
// container or a mount namespace without /proc looks like from inside micron. before the __ehdr_start fallback this configuration could not
// spawn a single thread -- __link_launch() raised except::thread_error and killed the whole process group
//
//   duck test tests/rigor/tls_no_proc.cpp -o bin/t -f --timeout 60 --def MICRON_TLS_NO_PROC
//
// NOTE: without the define this still builds and runs; it just exercises the ordinary path, so the define is what gives it teeth

#include "../../src/linux/sys/micthread/tls.hpp"
#include "../../src/sync/pause.hpp"
#include "../../src/thread/cpu.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/threads.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

constexpr int __n_threads = 8;

micron::atomic_token<u32> __ran{ 0 };

// a thread_local the child has to touch: this is the whole point of the TLS template, and it faults if the frame is wrong rather than merely
// absent, so it catches a bad load bias that a spawn-only test would sail past
thread_local u64 __tl_scratch = 0;

int
worker(int n)
{
  __tl_scratch = static_cast<u64>(n) * 7u + 1u;
  __ran.fetch_add(1, micron::memory_order_acq_rel);
  if ( __tl_scratch != static_cast<u64>(n) * 7u + 1u ) return -1;
  return n;
}

}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== TLS NO-PROC RIGOR ===");

#if defined(MICRON_TLS_NO_PROC)
  sb::print("MICRON_TLS_NO_PROC is ON: the /proc capture is forced to fail");
#else
  sb::print("MICRON_TLS_NO_PROC is OFF: ordinary path (build with --def MICRON_TLS_NO_PROC for the real case)");
#endif

  test_case("the /proc capture really is unavailable under the define");
  {
#if defined(MICRON_TLS_NO_PROC)
    require_false(__tls_capture_from_proc_auxv());
#else
    sb::print("not built with MICRON_TLS_NO_PROC; SKIPPED");
#endif
  }
  end_test_case();

  test_case("a template is still obtainable");
  {
    require_true(__tls_ensure_template());
    require_true(__micron_tls_template.valid);
    require_true(__micron_tls_template.image != nullptr);
    require_true(__micron_tls_template.memsz != 0);
  }
  end_test_case();

  test_case("threads_available reports true with no /proc");
  {
    // WARNING: this is the assertion that kills the old downstream workaround. gating a thread pool on open("/proc/self/auxv") reports false
    // here, where threads in fact work perfectly well
    require_true(threads_available());
  }
  end_test_case();

  test_case("spawn + join, and each child touches a thread_local");
  {
    __ran.store(0, memory_order_release);
    for ( int i = 0; i < __n_threads; ++i ) {
      auto t = solo::spawn<auto_thread<>>(worker, i);
      int got = t->result<int>();
      require(got, i);
      solo::join(t);
    }
    require(__ran.get(memory_order_acquire), static_cast<u32>(__n_threads));
  }
  end_test_case();

  test_case("the main thread's own thread_local survived all of it");
  {
    __tl_scratch = 0xdeadbeefu;
    require(__tl_scratch, static_cast<u64>(0xdeadbeefu));
  }
  end_test_case();

  test_case("cpu_count never reports 0");
  {
    // the sibling of the /proc bug above: cpu_count() parses a /sys node, and the reader yields an empty buffer -- so a count of 0 -- when
    // /sys is not mounted, which is the same stripped container. 0 is not merely useless, it is a crash: parallel/for.hpp computes
    // (size + cn - 1) / cn, so cn == 0 is integer division by zero. pin the floor
    require_true(cpu_count() >= 1u);
    // and the empty-buffer parse really is where the 0 comes from -- if this ever stops returning 0 the clamp above is guarding nothing
    require(posix::sysfs::__impl::__parse_range_count(""), static_cast<u32>(0));
  }
  end_test_case();

  sb::print("=== ALL TLS NO-PROC TESTS PASSED ===");
  return 1;
}
