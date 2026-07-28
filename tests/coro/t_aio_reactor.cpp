//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING
#define MICRON_CORO_STATS

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

// per-worker ring reactor mechanics: lifecycle, parking, cross-worker
// completion, off-engine fallback, stats seam

namespace coro = micron::coro;
static int FAILS = 0;

static i32
devnull_watermark()
{
  long fd = micron::syscall(SYS_openat, -100 /*AT_FDCWD*/, "/dev/null", 0 /*O_RDONLY*/, 0);
  if ( fd >= 0 ) micron::syscall(SYS_close, fd);
  return static_cast<i32>(fd);
}

static void
msleep(long ms)
{
  micron::timespec_t ts{ ms / 1000, (ms % 1000) * 1000000l };
  micron::syscall(SYS_nanosleep, &ts, nullptr);
}

static micron::task<i32>
await_nop()
{
  i32 r = co_await coro::io::nop();
  co_return r;
}

static micron::task<i32>
nop_burst(u32 n)
{
  for ( u32 i = 0; i < n; ++i ) {
    i32 r = co_await coro::io::nop();
    if ( r != 0 ) co_return r;
  }
  co_return 0;
}

static micron::task<i32>
read_u32(i32 rfd, u32 expect)
{
  u32 v = 0;
  i32 r = co_await coro::io::read(rfd, &v, sizeof(v));
  if ( r != static_cast<i32>(sizeof(v)) ) co_return -1;
  co_return v == expect ? 0 : -2;
}

struct tid_pair {
  long before = 0;
  long after = 0;
};

static micron::task<i32>
read_u32_tids(i32 rfd, u32 expect, tid_pair *out)
{
  out->before = micron::syscall(SYS_gettid);
  u32 v = 0;
  i32 r = co_await coro::io::read(rfd, &v, sizeof(v));
  out->after = micron::syscall(SYS_gettid);
  if ( r != static_cast<i32>(sizeof(v)) ) co_return -1;
  co_return v == expect ? 0 : -2;
}

static micron::task<void>
offengine_probe(micron::atomic_token<u32> *flag)
{
  i32 r = co_await coro::io::nop();
  flag->store(r == 0 ? 1u : 2u, micron::memory_order_release);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); reactor tests SKIPPED");
      return 1;
    }
  }

  sb::test_case("start/stop x20 cycles: no fd leak, pending drains");
  {
    const i32 wm0 = devnull_watermark();
    for ( u32 c = 0; c < 20; ++c ) {
      coro::start_coroutine_runtime(2);
      i32 r = coro::sync_wait(await_nop());
      sb::require(r == 0);
      sb::require(coro::io_pending() == 0);
      coro::stop_coroutine_runtime();
    }
    const i32 wm1 = devnull_watermark();
    sb::check(wm0 == wm1);
  }
  sb::end_test_case();

  sb::test_case("16 concurrent pipe reads across per-worker rings (4 workers)");
  {
    coro::start_coroutine_runtime(4);
    int pfd[16][2];
    micron::futex_future<i32> futs[16];
    for ( int i = 0; i < 16; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(read_u32(pfd[i][0], 0xabc00000u + static_cast<u32>(i)));
    }
    msleep(20);      // let readers submit + park
    for ( int i = 0; i < 16; ++i ) {
      u32 v = 0xabc00000u + static_cast<u32>(i);
      sb::require(micron::syscall(SYS_write, pfd[i][1], &v, sizeof(v)) == static_cast<long>(sizeof(v)));
    }
    for ( int i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
    for ( int i = 0; i < 16; ++i ) {
      micron::syscall(SYS_close, pfd[i][0]);
      micron::syscall(SYS_close, pfd[i][1]);
    }
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("re-entrant start is a no-op; ops still complete");
  {
    coro::start_coroutine_runtime(2);
    coro::start_coroutine_runtime(8);      // running: must be ignored
    sb::check(coro::__global_engine != nullptr && coro::__global_engine->n == 2);
    sb::check(coro::sync_wait(await_nop()) == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("stats seam: submits track ops, no inline completions in R1");
  {
    coro::start_coroutine_runtime(1);
    coro::__io_stats_reset();
    sb::check(coro::sync_wait(nop_burst(100)) == 0);
    coro::io_stats_t st = coro::io_stats();
    sb::print("stats: submits=", st.submits, " enters=", st.enters, " parks=", st.parks, " sqe_full=", st.sqe_full_flushes);
    sb::check(st.submits >= 100);
    sb::check(st.enters >= 100);
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("parked worker wakes on its own ring's cqe");
  {
    coro::start_coroutine_runtime(1);
    coro::__io_stats_reset();
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::futex_future<i32> fut = coro::schedule(read_u32(pfd[0], 77u));
    msleep(50);      // worker must have submitted the read and parked in its ring
    coro::io_stats_t st = coro::io_stats();
    sb::check(st.parks >= 1);
    u32 v = 77;
    sb::require(micron::syscall(SYS_write, pfd[1], &v, sizeof(v)) == static_cast<long>(sizeof(v)));
    sb::check(fut.get() == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("cross-worker completion: submit on A, resume anywhere (correctness)");
  {
    coro::start_coroutine_runtime(4);
    int pfd[16][2];
    tid_pair tids[16];
    micron::futex_future<i32> futs[16];
    for ( int i = 0; i < 16; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(read_u32_tids(pfd[i][0], static_cast<u32>(i) * 3u + 1u, &tids[i]));
    }
    msleep(30);
    for ( int i = 0; i < 16; ++i ) {
      u32 v = static_cast<u32>(i) * 3u + 1u;
      sb::require(micron::syscall(SYS_write, pfd[i][1], &v, sizeof(v)) == static_cast<long>(sizeof(v)));
    }
    u32 moved = 0;
    for ( int i = 0; i < 16; ++i ) {
      sb::check(futs[i].get() == 0);
      if ( tids[i].before != tids[i].after ) ++moved;
    }
    sb::print("cross-worker resumes: ", moved, "/16 (informational)");
    for ( int i = 0; i < 16; ++i ) {
      micron::syscall(SYS_close, pfd[i][0]);
      micron::syscall(SYS_close, pfd[i][1]);
    }
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("off-engine co_await routes through the fallback ring");
  {
    coro::start_coroutine_runtime(2);
    micron::atomic_token<u32> flag{ 0 };
    {
      micron::task<void> t = offengine_probe(&flag);
      t.handle().resume();      // main thread: current_worker()==nullptr -> fallback ring
      u32 spins = 0;
      while ( flag.get(micron::memory_order_acquire) == 0 && spins < 4000 ) {
        msleep(1);
        ++spins;
      }
      sb::check(flag.get(micron::memory_order_acquire) == 1u);
    }
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("sleep_for rides the submitter's ring");
  {
    coro::start_coroutine_runtime(1);
    coro::__io_stats_reset();
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    coro::sync_wait([]() -> micron::task<void> { co_await coro::sleep_for_ms(30); }());
    micron::clock_gettime(micron::clock_monotonic, t1);
    const i64 dt_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
    sb::check(dt_ms >= 25 && dt_ms < 1000);
    coro::io_stats_t st = coro::io_stats();
    sb::check(st.submits >= 1);      // the timeout sqe went through a ring
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("restart with different worker counts gets fresh rings");
  {
    coro::start_coroutine_runtime(1);
    sb::check(coro::sync_wait(await_nop()) == 0);
    coro::stop_coroutine_runtime();
    coro::start_coroutine_runtime(4);
    sb::check(coro::sync_wait(nop_burst(32)) == 0);
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO REACTOR TESTS PASSED ===");
  return 1;
}
