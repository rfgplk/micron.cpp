//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING
#define MICRON_CORO_URING_ENTRIES 4u
#define MICRON_CORO_STATS

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

// SQ/CQ backpressure with deliberately tiny (4-entry) rings: queue depth far
// beyond the ring, CQ-overflow backlog flushing, and the SQ-full flush path.
// nothing may be lost and -EAGAIN must never escape at these depths.

namespace coro = micron::coro;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_press";

static void
mkpath(char *out, const char *stem, u32 i)
{
  usize k = 0;
  for ( const char *p = DIR; *p; p++ ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = stem; *p; p++ ) out[k++] = *p;
  out[k++] = static_cast<char>('a' + (i / 26u) % 26u);
  out[k++] = static_cast<char>('a' + i % 26u);
  out[k] = '\0';
}

static void
msleep(long ms)
{
  micron::timespec_t ts{ ms / 1000, (ms % 1000) * 1000000l };
  micron::syscall(SYS_nanosleep, &ts, nullptr);
}

static micron::task<i32>
read_u32(i32 rfd, u32 expect)
{
  u32 v = 0;
  i32 r = co_await coro::io::read(rfd, &v, sizeof(v));
  if ( r != static_cast<i32>(sizeof(v)) ) co_return -1;
  co_return v == expect ? 0 : -2;
}

static micron::task<i32>
file_churn(const char *path)
{
  i32 fd = co_await coro::io::openat(-100 /*AT_FDCWD*/, path, 0102 | 02 | 01000 /*O_CREAT|O_RDWR|O_TRUNC*/, 0644);
  if ( fd < 0 ) co_return fd;
  u64 payload = 0x6d6963726f6e0a00ull;
  i32 wr = co_await coro::io::write(fd, &payload, sizeof(payload), 0);
  if ( wr != static_cast<i32>(sizeof(payload)) ) co_return -1000;
  i32 fs = co_await coro::io::fsync(fd);
  if ( fs != 0 ) co_return -1001;
  u64 back = 0;
  i32 rd = co_await coro::io::read(fd, &back, sizeof(back), 0);
  if ( rd != static_cast<i32>(sizeof(back)) || back != payload ) co_return -1002;
  i32 cl = co_await coro::io::close(fd);
  if ( cl != 0 ) co_return -1003;
  co_return 0;
}

static micron::task<i32>
sleepy_nops(u32 rounds)
{
  for ( u32 i = 0; i < rounds; ++i ) {
    co_await coro::sleep_for_ms(1 + (i % 5));
    i32 r = co_await coro::io::nop();
    if ( r != 0 ) co_return r;
  }
  co_return 0;
}

static void
pipe_storm(u32 nworkers)
{
  coro::start_coroutine_runtime(nworkers);
  int pfd[64][2];
  micron::futex_future<i32> futs[64];
  for ( u32 i = 0; i < 64; ++i ) {
    sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
    futs[i] = coro::schedule(read_u32(pfd[i][0], 0x5000u + i));
  }
  msleep(30);      // 64 in-flight reads vs 4-entry rings (CQ backlog territory)
  for ( u32 i = 0; i < 64; ++i ) {
    u32 v = 0x5000u + i;
    sb::require(micron::syscall(SYS_write, pfd[i][1], &v, sizeof(v)) == static_cast<long>(sizeof(v)));
  }
  for ( u32 i = 0; i < 64; ++i ) sb::check(futs[i].get() == 0);
  for ( u32 i = 0; i < 64; ++i ) {
    micron::syscall(SYS_close, pfd[i][0]);
    micron::syscall(SYS_close, pfd[i][1]);
  }
  sb::check(coro::io_pending() == 0);
  coro::io_stats_t st = coro::io_stats();
  sb::print("storm(", nworkers, "w): submits=", st.submits, " sqe_full_flushes=", st.sqe_full_flushes);
  coro::stop_coroutine_runtime();
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); ring-pressure tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(DIR, 0755);

  sb::test_case("64 concurrent pipe reads vs 4-entry rings (4 workers)");
  pipe_storm(4);
  sb::end_test_case();

  sb::test_case("64 concurrent pipe reads vs a 4-entry ring (1 worker: submitter == only reaper)");
  pipe_storm(1);
  sb::end_test_case();

  sb::test_case("64 openat/write/fsync/read/close chains, queue depth >> ring");
  {
    coro::start_coroutine_runtime(4);
    micron::futex_future<i32> futs[64];
    char paths[64][128];
    for ( u32 i = 0; i < 64; ++i ) {
      mkpath(paths[i], "churn_", i);
      futs[i] = coro::schedule(file_churn(paths[i]));
    }
    for ( u32 i = 0; i < 64; ++i ) sb::check(futs[i].get() == 0);
    sb::check(coro::io_pending() == 0);
    for ( u32 i = 0; i < 64; ++i ) micron::syscall(SYS_unlinkat, -100, paths[i], 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("timers + io interleaved over tiny rings: no deadlock, all complete");
  {
    coro::start_coroutine_runtime(2);
    micron::futex_future<i32> futs[32];
    for ( u32 i = 0; i < 32; ++i ) futs[i] = coro::schedule(sleepy_nops(8));
    for ( u32 i = 0; i < 32; ++i ) sb::check(futs[i].get() == 0);
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO RING-PRESSURE TESTS PASSED ===");
  return 1;
}
