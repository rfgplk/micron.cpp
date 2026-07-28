//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

// per-op timeouts (linked sqes) + cancellation-token wiring to in-flight ops
// + the stop-drain contract (stop cancels parked io instead of hanging)

namespace coro = micron::coro;
static int FAILS = 0;

#if defined(__SANITIZE_ADDRESS__)
static constexpr u32 HAMMER_ITERS = 2000;
#else
static constexpr u32 HAMMER_ITERS = 10000;
#endif

static void
msleep(long ms)
{
  micron::timespec_t ts{ ms / 1000, (ms % 1000) * 1000000l };
  micron::syscall(SYS_nanosleep, &ts, nullptr);
}

static i32
devnull_watermark()
{
  long fd = micron::syscall(SYS_openat, -100, "/dev/null", 0, 0);
  if ( fd >= 0 ) micron::syscall(SYS_close, fd);
  return static_cast<i32>(fd);
}

static micron::task<i32>
timed_read(i32 rfd, u64 timeout_ns)
{
  char b[8];
  i32 r = co_await (coro::io::read(rfd, b, sizeof(b)) | coro::io::after(timeout_ns));
  co_return r;
}

static micron::task<i32>
timed_nop_burst(u32 n)
{
  for ( u32 i = 0; i < n; ++i ) {
    i32 r = co_await (coro::io::nop() | coro::io::after(50'000'000ull));
    if ( r != 0 ) co_return r;
  }
  co_return 0;
}

static micron::task<i32>
token_read(i32 rfd)
{
  u32 v = 0;
  i32 r = co_await coro::io::read(rfd, &v, sizeof(v));
  co_return r;
}

static micron::task<i32>
canceller_side(coro::cancellation_source *src)
{
  co_await coro::sleep_for_ms(20);
  src->cancel();
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); cancel tests SKIPPED");
      return 1;
    }
  }

  coro::start_coroutine_runtime(4);

  sb::test_case("link_timeout fires: silent pipe read resumes -ECANCELED");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    i32 r = coro::sync_wait(timed_read(pfd[0], 20'000'000ull));
    micron::clock_gettime(micron::clock_monotonic, t1);
    const i64 dt_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
    sb::check(r == -125);
    sb::check(dt_ms >= 15 && dt_ms < 500);
    sb::check(coro::io_pending() == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("timeout does NOT fire x1000: fast ops unharmed, ltimer cqes drained");
  {
    sb::check(coro::sync_wait(timed_nop_burst(1000)) == 0);
    sb::check(coro::io_pending() == 0);
  }
  sb::end_test_case();

  sb::test_case("zero timeout on a silent pipe cancels immediately");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    i32 r = coro::sync_wait(timed_read(pfd[0], 1ull));      // 1ns: effectively immediate
    sb::check(r == -125);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("token cancels an in-flight read from another coroutine");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    coro::cancellation_source src;
    micron::task<i32> t = token_read(pfd[0]);
    src.bind(t);
    micron::futex_future<i32> fut = coro::schedule(micron::move(t));
    coro::detach(canceller_side(&src));
    sb::check(fut.get() == -125);
    sb::check(coro::io_pending() == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("token cancel from a non-engine thread (main)");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    coro::cancellation_source src;
    micron::task<i32> t = token_read(pfd[0]);
    src.bind(t);
    micron::futex_future<i32> fut = coro::schedule(micron::move(t));
    msleep(30);      // reader must be parked in the kernel
    src.cancel();
    sb::check(fut.get() == -125);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("pre-cancelled token short-circuits without submitting");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    coro::cancellation_source src;
    src.cancel();
    micron::task<i32> t = token_read(pfd[0]);
    src.bind(t);
    sb::check(coro::sync_wait(micron::move(t)) == -125);
    sb::check(coro::io_pending() == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("cancel-vs-complete hammer: exactly-once, no leaks");
  {
    const i32 wm0 = devnull_watermark();
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    u32 cancelled = 0, completed = 0, other = 0;
    for ( u32 i = 0; i < HAMMER_ITERS; ++i ) {
      coro::cancellation_source src;
      micron::task<i32> t = token_read(pfd[0]);
      src.bind(t);
      micron::futex_future<i32> fut = coro::schedule(micron::move(t));
      // jitter the order: even iterations write first, odd cancel first
      u32 v = i;
      if ( (i & 1u) == 0u ) {
        micron::syscall(SYS_write, pfd[1], &v, sizeof(v));
        src.cancel();
      } else {
        src.cancel();
        micron::syscall(SYS_write, pfd[1], &v, sizeof(v));
      }
      i32 r = fut.get();
      if ( r == static_cast<i32>(sizeof(u32)) ) {
        ++completed;
      } else if ( r == -125 ) {
        ++cancelled;
        // cancelled => the payload was never consumed; drain exactly it
        char junk[sizeof(u32)];
        micron::syscall(SYS_read, pfd[0], junk, sizeof(junk));
      } else {
        ++other;
      }
      if ( coro::io_pending() != 0 ) {
        ++other;
        break;
      }
    }
    sb::print("hammer: completed=", completed, " cancelled=", cancelled, " other=", other);
    sb::check(other == 0);
    sb::check(completed + cancelled == HAMMER_ITERS);
    sb::check(coro::io_pending() == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
    const i32 wm1 = devnull_watermark();
    sb::check(wm0 == wm1);
  }
  sb::end_test_case();

  sb::test_case("timeout + token cancel racing the same op: terminal, exactly-once");
  {
    for ( u32 i = 0; i < 200; ++i ) {
      int pfd[2];
      sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
      coro::cancellation_source src;
      micron::task<i32> t = [](i32 rfd) -> micron::task<i32> {
        char b[8];
        i32 r = co_await (coro::io::read(rfd, b, sizeof(b)) | coro::io::after(1'000'000ull));
        co_return r;
      }(pfd[0]);
      src.bind(t);
      micron::futex_future<i32> fut = coro::schedule(micron::move(t));
      src.cancel();
      i32 r = fut.get();
      sb::check(r == -125);
      micron::syscall(SYS_close, pfd[0]);
      micron::syscall(SYS_close, pfd[1]);
    }
    sb::check(coro::io_pending() == 0);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();

  sb::test_case("stop with parked reads: cancelled + drained, engine restartable");
  {
    const i32 wm0 = devnull_watermark();
    coro::start_coroutine_runtime(2);
    int pfd[8][2];
    micron::futex_future<i32> futs[8];
    for ( u32 i = 0; i < 8; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(token_read(pfd[i][0]));      // no writer will ever come
    }
    msleep(30);      // all 8 parked in the kernel
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    coro::stop_coroutine_runtime();      // must cancel-drain, not hang
    micron::clock_gettime(micron::clock_monotonic, t1);
    const i64 dt_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
    sb::print("stop with 8 parked reads took ", dt_ms, " ms");
    sb::check(dt_ms < 5000);
    for ( u32 i = 0; i < 8; ++i ) sb::check(futs[i].get() == -125);
    for ( u32 i = 0; i < 8; ++i ) {
      micron::syscall(SYS_close, pfd[i][0]);
      micron::syscall(SYS_close, pfd[i][1]);
    }
    // restartable afterward
    coro::start_coroutine_runtime(2);
    sb::check(coro::sync_wait([]() -> micron::task<i32> {
      i32 r = co_await coro::io::nop();
      co_return r;
    }()) == 0);
    coro::stop_coroutine_runtime();
    const i32 wm1 = devnull_watermark();
    sb::check(wm0 == wm1);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO CANCEL/TIMEOUT TESTS PASSED ===");
  return 1;
}
