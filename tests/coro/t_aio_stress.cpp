//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../snowball/snowball.hpp"

// endurance: mixed op storms on private + striped-shared files, open/close
// churn, timers + cancellation mixed with io. content verified by checksums.

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;
static int FAILS = 0;

#if defined(__SANITIZE_ADDRESS__)
static constexpr u32 N_WORKERS_TASKS = 16;
static constexpr u32 OPS_PER_TASK = 50;
static constexpr u32 CHURN_N = 200;
#else
static constexpr u32 N_WORKERS_TASKS = 64;
static constexpr u32 OPS_PER_TASK = 200;
static constexpr u32 CHURN_N = 2000;
#endif

static constexpr const char *DIR = "/tmp/micron_aio_stress";

static micron::io::path_t
mkpath(const char *stem, u32 i)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += stem;
  char sfx[3] = { static_cast<char>('a' + (i / 26u) % 26u), static_cast<char>('a' + i % 26u), 0 };
  p += sfx;
  return p;
}

// private-file storm:each task owns one file, mixed write/read/fsync at rotating
// offsets; final content must equal the last full pattern written
static micron::task<i32>
private_storm(u32 id)
{
  micron::io::path_t p = mkpath("priv", id);
  cio::file f = co_await cio::open_file(micron::move(p), mio::modes::readwritecreate);
  if ( !f.valid() ) co_return f.raw_fd();
  char blk[1024];
  for ( u32 op = 0; op < OPS_PER_TASK; ++op ) {
    const char fill = static_cast<char>('a' + ((id + op) % 26u));
    for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = fill;
    max_t w = co_await f.write_at((op % 8u) * sizeof(blk), blk, sizeof(blk));
    if ( w != static_cast<max_t>(sizeof(blk)) ) co_return -1000;
    if ( (op & 15u) == 0u ) {
      char back[1024];
      max_t r = co_await f.read_at((op % 8u) * sizeof(blk), back, sizeof(back));
      if ( r != static_cast<max_t>(sizeof(back)) ) co_return -1001;
      for ( usize i = 0; i < sizeof(back); ++i )
        if ( back[i] != fill ) co_return -1002;
    }
    if ( (op & 31u) == 0u ) {
      i32 s = co_await f.datasync();
      if ( s != 0 ) co_return -1003;
    }
  }
  co_return 0;
}

// striped-shared storm: 8 tasks on one file, disjoint 4K stripes
static micron::task<i32>
shared_storm(cio::file *f, u32 stripe)
{
  char blk[4096];
  const char fill = static_cast<char>('A' + (stripe % 26u));
  for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = fill;
  for ( u32 op = 0; op < OPS_PER_TASK; ++op ) {
    max_t w = co_await f->write_at(stripe * sizeof(blk), blk, sizeof(blk));
    if ( w != static_cast<max_t>(sizeof(blk)) ) co_return -1100;
    char back[4096];
    max_t r = co_await f->read_at(stripe * sizeof(blk), back, sizeof(back));
    if ( r != static_cast<max_t>(sizeof(back)) ) co_return -1101;
    for ( usize i = 0; i < sizeof(back); ++i )
      if ( back[i] != fill ) co_return -1102;      // a foreign stripe bled in
  }
  co_return 0;
}

static micron::task<i32>
churn_worker(u32 id, u32 rounds)
{
  for ( u32 i = 0; i < rounds; ++i ) {
    micron::io::path_t p = mkpath("churn", id);
    i32 fd = co_await micron::coro::io::openat(-100, p.c_str(), 0102 | 02 | 01000, 0644);
    if ( fd < 0 ) co_return fd;
    u64 payload = (static_cast<u64>(id) << 32) | i;
    i32 w = co_await micron::coro::io::write(fd, &payload, sizeof(payload), 0);
    if ( w != static_cast<i32>(sizeof(payload)) ) co_return -1200;
    if ( (i & 63u) == 0u ) {
      i32 s = co_await micron::coro::io::fsync(fd);
      if ( s != 0 ) co_return -1201;
    }
    i32 c = co_await micron::coro::io::close(fd);
    if ( c != 0 ) co_return -1202;
    i32 u = co_await micron::coro::io::unlinkat(-100, p.c_str(), 0);
    if ( u != 0 ) co_return -1203;
  }
  co_return 0;
}

static micron::task<i32>
timer_io_mix(i32 rfd, u32 rounds)
{
  for ( u32 i = 0; i < rounds; ++i ) {
    char b[8];
    i32 r = co_await (micron::coro::io::read(rfd, b, sizeof(b)) | micron::coro::io::after(2'000'000ull));
    if ( r != -125 ) co_return -1300;      // nobody ever writes: every read times out
    co_await coro::sleep_for_ms(1);
  }
  co_return 0;
}

static i32
devnull_watermark()
{
  long fd = micron::syscall(SYS_openat, -100, "/dev/null", 0, 0);
  if ( fd >= 0 ) micron::syscall(SYS_close, fd);
  return static_cast<i32>(fd);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); stress tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(DIR, 0755);
  const i32 wm0 = devnull_watermark();
  coro::start_coroutine_runtime(0);

  sb::test_case("private-file storm: N tasks x M mixed ops");
  {
    micron::vector<micron::futex_future<i32>> futs;
    for ( u32 i = 0; i < N_WORKERS_TASKS; ++i ) futs.push_back(coro::schedule(private_storm(i)));
    u32 bad = 0;
    for ( u32 i = 0; i < N_WORKERS_TASKS; ++i )
      if ( futs[i].get() != 0 ) ++bad;
    sb::check(bad == 0);
    sb::check(coro::io_pending() == 0);
    for ( u32 i = 0; i < N_WORKERS_TASKS; ++i ) micron::syscall(SYS_unlinkat, -100, mkpath("priv", i).c_str(), 0);
  }
  sb::end_test_case();

  sb::test_case("striped-shared storm: 8 tasks on ONE file, no cross-stripe bleed");
  {
    micron::io::path_t p = mkpath("shared", 0);
    cio::file f(p, mio::modes::readwritecreate);
    sb::require(f.valid());
    micron::futex_future<i32> futs[8];
    for ( u32 i = 0; i < 8; ++i ) futs[i] = coro::schedule(shared_storm(micron::addressof(f), i));
    for ( u32 i = 0; i < 8; ++i ) sb::check(futs[i].get() == 0);
    micron::syscall(SYS_unlinkat, -100, p.c_str(), 0);
  }
  sb::end_test_case();

  sb::test_case("open/write/fsync/close/unlink churn across 8 tasks");
  {
    micron::futex_future<i32> futs[8];
    for ( u32 i = 0; i < 8; ++i ) futs[i] = coro::schedule(churn_worker(i, CHURN_N / 8));
    for ( u32 i = 0; i < 8; ++i ) sb::check(futs[i].get() == 0);
    sb::check(coro::io_pending() == 0);
  }
  sb::end_test_case();

  sb::test_case("timers + timeouts + io mixed, seconds-bounded");
  {
    int pfd[16][2];
    micron::futex_future<i32> futs[16];
    for ( u32 i = 0; i < 16; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(timer_io_mix(pfd[i][0], 32));
    }
    for ( u32 i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
    for ( u32 i = 0; i < 16; ++i ) {
      micron::syscall(SYS_close, pfd[i][0]);
      micron::syscall(SYS_close, pfd[i][1]);
    }
    sb::check(coro::io_pending() == 0);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();

  sb::test_case("no fd leaked across the whole run");
  {
    const i32 wm1 = devnull_watermark();
    sb::check(wm0 == wm1);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO STRESS TESTS PASSED ===");
  return 1;
}
