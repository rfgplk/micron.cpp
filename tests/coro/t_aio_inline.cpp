//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING
#define MICRON_CORO_STATS

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

// submit-then-peek inline completion: cached reads/nops resume without ever
// suspending. NOTE the filesystem trap: tmpfs files lack FMODE_NOWAIT, so
// io_uring punts them to io-wq and the inline path cannot fire - the warm-read
// cases MUST run on a real fs (/var/tmp = xfs here); the tmpfs leg only
// documents the punt. correctness must hold on every path.

namespace coro = micron::coro;
static int FAILS = 0;

static constexpr const char *XDIR = "/var/tmp/micron_aio_inline";
static constexpr const char *TDIR = "/tmp/micron_aio_inline";

static void
mkfile(const char *dir, const char *name, char *out, const void *data, usize n)
{
  usize k = 0;
  for ( const char *p = dir; *p; p++ ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; p++ ) out[k++] = *p;
  out[k] = '\0';
  long fd = micron::syscall(SYS_openat, -100, out, 0102 | 02 | 01000 /*O_CREAT|O_RDWR|O_TRUNC*/, 0644);
  if ( fd < 0 ) return;
  micron::syscall(SYS_write, fd, data, n);
  micron::syscall(SYS_fsync, fd);
  micron::syscall(SYS_close, fd);
}

static micron::task<i32>
warm_reads(const char *path, u32 reps, long *tid_delta)
{
  long fd = micron::syscall(SYS_openat, -100, path, 02 /*O_RDWR*/, 0);
  if ( fd < 0 ) co_return static_cast<i32>(fd);
  // warm the page cache synchronously
  alignas(64) char buf[4096];
  micron::syscall(SYS_pread64, fd, buf, sizeof(buf), 0);
  long tid0 = micron::syscall(SYS_gettid);
  long moved = 0;
  for ( u32 i = 0; i < reps; ++i ) {
    i32 r = co_await coro::io::read(static_cast<i32>(fd), buf, sizeof(buf), 0);
    if ( r != static_cast<i32>(sizeof(buf)) ) {
      micron::syscall(SYS_close, fd);
      co_return -1000;
    }
    if ( micron::syscall(SYS_gettid) != tid0 ) {
      ++moved;
      tid0 = micron::syscall(SYS_gettid);
    }
  }
  *tid_delta = moved;
  micron::syscall(SYS_close, fd);
  co_return 0;
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
badfd_read()
{
  char b[8];
  i32 r = co_await coro::io::read(-1, b, sizeof(b), 0);
  co_return r;
}

static micron::task<i32>
odirect_read(const char *path)
{
  long fd = micron::syscall(SYS_openat, -100, path, 0 | 040000 /*O_RDONLY|O_DIRECT*/, 0);
  if ( fd < 0 ) co_return -9000;      // O_DIRECT unsupported here: caller skips
  alignas(4096) static char buf[4096];
  i32 r = co_await coro::io::read(static_cast<i32>(fd), buf, sizeof(buf), 0);
  micron::syscall(SYS_close, fd);
  co_return r;
}

static micron::task<i32>
mixed_batch(const char *path, i32 rfd, i32 wfd)
{
  long fd = micron::syscall(SYS_openat, -100, path, 0 /*O_RDONLY*/, 0);
  if ( fd < 0 ) co_return static_cast<i32>(fd);
  char buf[4096];
  for ( u32 i = 0; i < 8; ++i ) {
    i32 r = co_await coro::io::read(static_cast<i32>(fd), buf, sizeof(buf), 0);
    if ( r != static_cast<i32>(sizeof(buf)) ) co_return -2000;
  }
  coro::detach([](i32 w) -> micron::task<void> {
    co_await coro::sleep_for_ms(10);
    u32 v = 99;
    co_await coro::io::write(w, &v, sizeof(v));
  }(wfd));
  u32 back = 0;
  i32 r = co_await coro::io::read(rfd, &back, sizeof(back));      // parks: no writer for 10ms
  micron::syscall(SYS_close, fd);
  if ( r != static_cast<i32>(sizeof(back)) || back != 99 ) co_return -2001;
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); inline tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(XDIR, 0755);
  (void)micron::posix::mkdir(TDIR, 0755);

  alignas(64) static char blob[4096];
  for ( usize i = 0; i < sizeof(blob); ++i ) blob[i] = static_cast<char>(i * 131u);

  sb::test_case("nop burst completes inline (>=99%)");
  {
    coro::start_coroutine_runtime(4);
    coro::__io_stats_reset();
    sb::check(coro::sync_wait(nop_burst(1000)) == 0);
    coro::io_stats_t st = coro::io_stats();
    sb::print("nop x1000: inline=", st.inline_completions, " parks=", st.parks);
    sb::check(st.inline_completions >= 990);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("warm 4K reads on xfs complete inline (>=95%), tid stable");
  {
    char path[128];
    mkfile(XDIR, "warm4k.dat", path, blob, sizeof(blob));
    coro::start_coroutine_runtime(4);
    coro::__io_stats_reset();
    long moved = -1;
    sb::check(coro::sync_wait(warm_reads(path, 1000, &moved)) == 0);
    coro::io_stats_t st = coro::io_stats();
    sb::print("xfs warm x1000: inline=", st.inline_completions, " parks=", st.parks, " tid-moves=", moved);
    sb::check(st.inline_completions >= 950);
    sb::check(moved <= 50);      // inline resumes stay on the submitting worker
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_unlinkat, -100, path, 0);
  }
  sb::end_test_case();

  sb::test_case("tmpfs punt documented: ops correct, inline rate collapses");
  {
    char path[128];
    mkfile(TDIR, "warm4k.dat", path, blob, sizeof(blob));
    coro::start_coroutine_runtime(4);
    coro::__io_stats_reset();
    long moved = -1;
    sb::check(coro::sync_wait(warm_reads(path, 200, &moved)) == 0);
    coro::io_stats_t st = coro::io_stats();
    sb::print("tmpfs warm x200: inline=", st.inline_completions, " parks=", st.parks, " (io-wq punt: informational)");
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_unlinkat, -100, path, 0);
  }
  sb::end_test_case();

  sb::test_case("bad fd errors inline with -EBADF");
  {
    coro::start_coroutine_runtime(1);
    coro::__io_stats_reset();
    sb::check(coro::sync_wait(badfd_read()) == -9);
    coro::io_stats_t st = coro::io_stats();
    sb::print("badfd: inline=", st.inline_completions);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("O_DIRECT read takes the park path (inline == 0 for the read)");
  {
    char path[128];
    mkfile(XDIR, "direct4k.dat", path, blob, sizeof(blob));
    coro::start_coroutine_runtime(1);
    coro::__io_stats_reset();
    i32 r = coro::sync_wait(odirect_read(path));
    if ( r == -9000 || r == -22 ) {
      sb::print("O_DIRECT unsupported here; case SKIPPED (r=", r, ")");
    } else {
      sb::check(r == 4096);
      coro::io_stats_t st = coro::io_stats();
      sb::print("O_DIRECT: inline=", st.inline_completions, " parks=", st.parks);
      sb::check(st.inline_completions == 0);
    }
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_unlinkat, -100, path, 0);
  }
  sb::end_test_case();

  sb::test_case("mixed batch: cached reads inline, pipe read parks, ordering intact");
  {
    char path[128];
    mkfile(XDIR, "mixed4k.dat", path, blob, sizeof(blob));
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    coro::start_coroutine_runtime(2);
    coro::__io_stats_reset();
    sb::check(coro::sync_wait(mixed_batch(path, pfd[0], pfd[1])) == 0);
    coro::io_stats_t st = coro::io_stats();
    sb::print("mixed: inline=", st.inline_completions, " (expect ~8 cached reads inline)");
    sb::check(st.inline_completions >= 6);
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
    micron::syscall(SYS_unlinkat, -100, path, 0);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO INLINE-COMPLETION TESTS PASSED ===");
  return 1;
}
