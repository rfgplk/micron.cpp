// control.cpp
// micron's process control utilities (src/control.hpp).
// Thin wrappers around Linux signals and syscalls — no libc.
//
// See also:
//   examples/exceptions.cpp — error/exception types
//   examples/mutex.cpp      — synchronisation primitives

#include "../src/control.hpp"
#include "../src/io/console.hpp"

static bool got_user1 = false;
static bool got_user2 = false;

int
main()
{
  // --- ignore_pipe ---
  // Suppress SIGPIPE so writes to a closed socket/pipe don't kill the process.
  // Call once at startup in any network or IPC program.
  micron::ignore_pipe();
  micron::io::println("SIGPIPE ignored");

  // --- mark ---
  // A no-op annotated noinline, -O0 function — useful as a GDB breakpoint
  // target. Calling it in production has no functional effect.
  micron::mark();

  // --- on_user_signals ---
  // Register handlers for SIGUSR1 and SIGUSR2.
  // Both handlers receive the signal number as an int (same pattern as on_terminate).
  micron::on_user_signals(
      [](int) { got_user1 = true; micron::io::println("SIGUSR1 received"); },
      [](int) { got_user2 = true; micron::io::println("SIGUSR2 received"); });

  // --- on_terminate ---
  // Register a single handler for SIGINT, SIGTERM, SIGHUP, SIGQUIT.
  // Use for graceful shutdown: flush buffers, close sockets, etc.
  // NOTE: on_terminate takes ownership of the lambda — call once per program.
  // on_terminate handler receives the signal number as an int argument
  micron::on_terminate([](int signum) {
    micron::io::println("shutdown: sig=", signum, " (flush, close, etc.)");
  });

  // --- cont ---
  // Send SIGCONT to a given PID. Used when you've previously sent SIGSTOP
  // to a child process and want to resume it.
  // micron::cont(child_pid);   // not called here — no child to resume

  // --- stop / halt ---
  // micron::stop()  — send SIGSTOP to self (suspend until SIGCONT)
  // micron::halt()  — send SIGTERM then SIGKILL to self (graceful exit)
  // These are not called in the example since they terminate/suspend the process.

  // --- crash<N> ---
  // Force an immediate abnormal termination — useful in irrecoverable error
  // paths where you want a core dump rather than a clean exit.
  //   crash<0>()  — execute HLT instruction
  //   crash<1>()  — raise SIGSEGV
  //   crash<2+>() — null-deref loop (always crashes)
  // Not called here.

  micron::io::println("control example done");
  return 0;
}
