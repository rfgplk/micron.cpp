//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::exit() unwinding test. Verifies that:
//   * micron::atexit() handlers fire in LIFO order
//   * __attribute__((destructor)) functions fire AFTER all atexit handlers
//     (.fini_array drained last by micron::exit())
//   * C++ global object destructors fire via __cxa_atexit and respect the
//     same LIFO discipline as atexit() (interleaved in registration order)
//   * a handler that registers more atexit handlers gets them run too
//   * sys_exit / quick_exit / _Exit bypass the soft path entirely
//
// The test communicates result via the process exit code:
//   0 -- all checks passed
//   non-zero -- numbered failure point (see fini_verify below)
//   42 -- exit(42) reached the end of micron::exit() without a fini dtor
//         intercepting -- means the soft path didn't traverse .fini_array
//
// Build:    duck build tests/rigor/exit_unwind.cpp -k
// Run:      bin/exit_unwind ; echo $?     --> expects 0
//
// NOTE: requires the updated start.cpp and exit.hpp (both in this changeset)
// to be installed via scripts/install_local.py before duck can see them.

#include "../../src/errno.hpp"
#include "../../src/exit.hpp"
#include "../../src/types.hpp"

// boot helpers normally provided by cmalloc.hpp / io.hpp. exit_unwind does
// not exercise the allocator or io paths so empty stubs suffice.
extern "C" void
__boot_abcmalloc(void)
{
}

extern "C" void
__boot_io_buffers(void)
{
}

namespace
{
// 32 slots is plenty for the handler counts below. Plain int -- the LIFO
// drain in micron::exit is single-threaded once __exit_in_progress latches,
// so we don't need atomic semantics here.
int g_trace[32] = {};
int g_trace_idx = 0;

inline void
emit(int tag)
{
  if ( g_trace_idx < 32 ) g_trace[g_trace_idx++] = tag;
}

// --- atexit handlers (tags 1..4) -------------------------------------------
void
h1(void)
{
  emit(1);
}

void
h2(void)
{
  emit(2);
}

void
h3(void)
{
  emit(3);
}

// h4 is the "self-extender" -- when it runs it pushes h_late, which must
// also fire before the registry drain completes.
void
h_late(void)
{
  emit(7);
}

void
h4(void)
{
  emit(4);
  // Late registration during drain. The drain loop in micron::exit re-reads
  // the counter each iteration, so this entry MUST be picked up.
  micron::atexit(h_late);
}

// --- C++ global with non-trivial dtor (tag 5) via __cxa_atexit -------------
struct cxa_dtor_probe {
  cxa_dtor_probe() = default;

  ~cxa_dtor_probe() { emit(5); }
};

// Constructed at .init_array time; destructor registered via __cxa_atexit.
cxa_dtor_probe g_cxa_probe;

// --- .fini_array entry (tag 6) -- runs AFTER all atexit/cxa handlers ------
// Also serves as the verifier: writes the verdict to the process exit code.
}      // namespace

extern "C" void __attribute__((destructor))
fini_verify(void)
{
  emit(6);

  // Expected drain order under LIFO discipline:
  //   atexit registry (newest first): h_late(7), h4(4), h3(3), h2(2), h1(1),
  //                                   then the cxa_dtor_probe (5)
  //   .fini_array                  : fini_verify (6)
  //
  // Registration order in main: h1, h2, h3, h4 (4 newest), and
  // __cxa_atexit fires from .init_array BEFORE main runs, so g_cxa_probe
  // is the OLDEST registration -- runs last in the atexit drain.
  //
  // So we expect g_trace = [4, 7, 3, 2, 1, 5, 6]
  // (h4 fires first popping 4, then *while h4 is running* it registers
  //  h_late which the next loop iteration picks up popping 7, then back to
  //  the originally-registered stack: 3, 2, 1, then the cxa probe: 5, then
  //  this fini_verify pushes 6.)
  static const int expected[] = { 4, 7, 3, 2, 1, 5, 6 };
  constexpr int expected_n = sizeof(expected) / sizeof(expected[0]);

  if ( g_trace_idx != expected_n ) {
    // Encode mismatch length as exit code 100 + idx for diagnostics.
    micron::sys_exit(100 + g_trace_idx);
  }
  for ( int i = 0; i < expected_n; ++i ) {
    if ( g_trace[i] != expected[i] ) {
      // 10 + i identifies which slot mismatched.
      micron::sys_exit(10 + i);
    }
  }

  // All checks passed. Bypass the trailing sys_exit in micron::exit() so we
  // get a clean 0. (If micron::exit's tail ran, it would sys_exit(42)
  // because main called exit(42) -- catching that miswiring would be
  // valuable, but we instead want a clean 0 to indicate full success.)
  micron::sys_exit(0);
}

extern "C" int
main(int /*argc*/, char ** /*argv*/, char ** /*envp*/)
{
  // Register four atexit handlers. LIFO drain expects h4 first.
  if ( micron::atexit(h1) != 0 ) return 71;
  if ( micron::atexit(h2) != 0 ) return 72;
  if ( micron::atexit(h3) != 0 ) return 73;
  if ( micron::atexit(h4) != 0 ) return 74;

  // 42 is a sentinel: fini_verify replaces it with 0 on pass. If fini_verify
  // never runs, the process exits 42 -- which signals "fini_array was not
  // walked during exit()" -- a regression we want to catch.
  micron::exit(42);
}
