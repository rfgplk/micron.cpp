//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../../src/linux/sys/signal.hpp"
#include "../snowball/snowball.hpp"

// async pipes + stdio: ping-pong, EOF/EPIPE edges, chunk streaming,
// backpressure with the idle-CPU property, dup2'd stdin/stdout interact

namespace coro = micron::coro;
namespace cio = micron::io::coro;
static int FAILS = 0;

static void
msleep(long ms)
{
  micron::timespec_t ts{ ms / 1000, (ms % 1000) * 1000000l };
  micron::syscall(SYS_nanosleep, &ts, nullptr);
}

static long
self_cpu_ticks()
{
  // /proc/self/stat fields 14+15 (utime+stime)
  long fd = micron::syscall(SYS_openat, -100, "/proc/self/stat", 0, 0);
  if ( fd < 0 ) return -1;
  char buf[512];
  long n = micron::syscall(SYS_read, fd, buf, sizeof(buf) - 1);
  micron::syscall(SYS_close, fd);
  if ( n <= 0 ) return -1;
  buf[n] = 0;
  // skip past "comm" (may contain spaces inside parens)
  long i = 0;
  while ( i < n && buf[i] != ')' ) ++i;
  i += 2;
  long field = 3, val = 0, total = 0;
  while ( i < n && field <= 15 ) {
    if ( buf[i] == ' ' ) {
      if ( field == 14 || field == 15 ) total += val;
      val = 0;
      ++field;
    } else if ( buf[i] >= '0' && buf[i] <= '9' ) {
      val = val * 10 + (buf[i] - '0');
    }
    ++i;
  }
  return total;
}

static micron::task<i32>
pingpong(i32 rfd, i32 wfd, u32 rounds)
{
  cio::fd_io r{ rfd }, w{ wfd };
  for ( u32 i = 0; i < rounds; ++i ) {
    u32 v = i * 2654435761u;
    max_t ww = co_await w.write(&v, sizeof(v));
    if ( ww != static_cast<max_t>(sizeof(v)) ) co_return -1000;
    u32 back = 0;
    max_t rr = co_await r.read(&back, sizeof(back));
    if ( rr != static_cast<max_t>(sizeof(back)) || back != v ) co_return -1001;
  }
  co_return 0;
}

static micron::atomic_token<u64> g_bp_written{ 0 };

static micron::task<i32>
bp_writer(i32 wfd, u32 total)
{
  cio::fd_io w{ wfd };
  static char blk[16384];
  for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = static_cast<char>(i * 41u);
  u32 sent = 0;
  while ( sent < total ) {
    u32 want = total - sent;
    if ( want > sizeof(blk) ) want = sizeof(blk);
    max_t n = co_await w.write(blk, want);
    if ( n <= 0 ) co_return -1100;
    sent += static_cast<u32>(n);
    g_bp_written.store(sent, micron::memory_order_release);
  }
  micron::syscall(SYS_close, wfd);
  co_return static_cast<i32>(sent);
}

static micron::task<i32>
bp_reader_slow(i32 rfd, u32 expect)
{
  cio::fd_io r{ rfd };
  static char blk[16384];
  u32 got = 0;
  u32 gulps = 0;
  for ( ;; ) {
    max_t n = co_await r.read_some(blk, sizeof(blk));
    if ( n < 0 ) co_return -1200;
    if ( n == 0 ) break;
    got += static_cast<u32>(n);
    if ( (++gulps & 3u) == 0u ) co_await coro::sleep_for_ms(1);      // slow consumer
  }
  co_return got == expect ? 0 : -1201;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); pipe tests SKIPPED");
      return 1;
    }
  }

  // widowed-pipe writes must surface -EPIPE, not kill the process
  {
    micron::posix::sigaction_t sa{};
    sa.sigaction_handler.sa_handler = reinterpret_cast<void (*)(int)>(1) /*SIG_IGN*/;
    micron::posix::sigaction(13 /*SIGPIPE*/, sa, nullptr);
  }

  coro::start_coroutine_runtime(2);

  sb::test_case("pipe ping-pong x512 through fd_io");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    sb::check(coro::sync_wait(pingpong(pfd[0], pfd[1], 512)) == 0);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("writer closes -> reader EOF (0)");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::syscall(SYS_close, pfd[1]);
    i32 r = coro::sync_wait([](i32 rfd) -> micron::task<i32> {
      char b[8];
      max_t x = co_await cio::read_some(rfd, b, sizeof(b));
      co_return static_cast<i32>(x);
    }(pfd[0]));
    sb::check(r == 0);
    micron::syscall(SYS_close, pfd[0]);
  }
  sb::end_test_case();

  sb::test_case("reader closes -> writer -EPIPE");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::syscall(SYS_close, pfd[0]);
    i32 r = coro::sync_wait([](i32 wfd) -> micron::task<i32> {
      char b[8] = {};
      max_t x = co_await cio::write_all(wfd, b, sizeof(b));
      co_return static_cast<i32>(x);
    }(pfd[1]));
    sb::check(r == -32);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("each_chunk over uneven detached bursts concatenates exactly");
  {
    micron::io::upipe p;      // io_v3 unidirectional pipe
    sb::require(p.read_fd() >= 0 && p.write_fd() >= 0);
    coro::detach([](micron::io::upipe *pp) -> micron::task<void> {
      static const char b1[] = "first-burst|";
      static const char b2[] = "b2|";
      static const char b3[] = "the-third-and-final-burst";
      i32 w1 = co_await micron::coro::io::write(static_cast<i32>(pp->write_fd()), b1, sizeof(b1) - 1);
      co_await coro::sleep_for_ms(5);
      i32 w2 = co_await micron::coro::io::write(static_cast<i32>(pp->write_fd()), b2, sizeof(b2) - 1);
      co_await coro::sleep_for_ms(5);
      i32 w3 = co_await micron::coro::io::write(static_cast<i32>(pp->write_fd()), b3, sizeof(b3) - 1);
      (void)w1;
      (void)w2;
      (void)w3;
      pp->close_write();
    }(micron::addressof(p)));
    micron::string got;
    max_t n = coro::sync_wait([](micron::io::upipe *pp, micron::string *out) -> micron::task<max_t> {
      max_t r = co_await cio::each_chunk(*pp, [out](const byte *d, usize k) { out->append(reinterpret_cast<const char *>(d), k); });
      co_return r;
    }(micron::addressof(p), micron::addressof(got)));
    sb::check(n == static_cast<max_t>(got.size()));
    sb::check(got == micron::string("first-burst|b2|the-third-and-final-burst"));
  }
  sb::end_test_case();

  sb::test_case("backpressure: 4 MiB through a default pipe, slow reader; idle CPU while parked");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    g_bp_written.store(0, micron::memory_order_relaxed);
    micron::futex_future<i32> fw = coro::schedule(bp_writer(pfd[1], 4u << 20));
    micron::futex_future<i32> fr = coro::schedule(bp_reader_slow(pfd[0], 4u << 20));
    sb::check(fw.get() == static_cast<i32>(4u << 20));
    sb::check(fr.get() == 0);
    micron::syscall(SYS_close, pfd[0]);
    // idle-CPU property: engine up + one pending pipe read, ~0% CPU
    int idle_pfd[2];
    sb::require(micron::syscall(SYS_pipe2, idle_pfd, 0) == 0);
    micron::futex_future<i32> parked = coro::schedule([](i32 rfd) -> micron::task<i32> {
      char b[4];
      max_t r = co_await cio::read_some(rfd, b, sizeof(b));
      co_return static_cast<i32>(r);
    }(idle_pfd[0]));
    msleep(100);      // let it park
    const long t0 = self_cpu_ticks();
    msleep(2000);
    const long t1 = self_cpu_ticks();
    // the watcher keeps a 1ms sweep cadence while io is pending: ~2-3% of one
    // core (4-6 ticks). a spinning engine burns ~200 ticks here - gate at 12
    // (6%) so scheduling jitter can't flake the property being proven.
    sb::print("idle 2s cpu ticks: ", t1 - t0, " (gate < 12)");
    sb::check(t1 - t0 >= 0 && t1 - t0 < 12);
    u32 v = 1;
    micron::syscall(SYS_write, idle_pfd[1], &v, sizeof(v));
    sb::check(parked.get() == 4);
    micron::syscall(SYS_close, idle_pfd[0]);
    micron::syscall(SYS_close, idle_pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("porcelain splice: pipe -> pipe");
  {
    int pa[2], pb[2];
    sb::require(micron::syscall(SYS_pipe2, pa, 0) == 0 && micron::syscall(SYS_pipe2, pb, 0) == 0);
    static const char payload[] = "spliced-through-the-ring";
    sb::require(micron::syscall(SYS_write, pa[1], payload, sizeof(payload) - 1) == static_cast<long>(sizeof(payload) - 1));
    max_t n = coro::sync_wait([](i32 in, i32 out) -> micron::task<max_t> {
      max_t r = co_await cio::splice(in, out, sizeof(payload) - 1);
      co_return r;
    }(pa[0], pb[1]));
    sb::check(n == static_cast<max_t>(sizeof(payload) - 1));
    char back[64] = {};
    sb::require(micron::syscall(SYS_read, pb[0], back, sizeof(back)) == static_cast<long>(sizeof(payload) - 1));
    sb::check(micron::strcmp(back, payload) == 0);
    micron::syscall(SYS_close, pa[0]);
    micron::syscall(SYS_close, pa[1]);
    micron::syscall(SYS_close, pb[0]);
    micron::syscall(SYS_close, pb[1]);
  }
  sb::end_test_case();

  sb::test_case("interact via dup2'd stdin/stdout");
  {
    int in_pipe[2], out_pipe[2];
    sb::require(micron::syscall(SYS_pipe2, in_pipe, 0) == 0 && micron::syscall(SYS_pipe2, out_pipe, 0) == 0);
    static const char feed[] = "hello interact";
    sb::require(micron::syscall(SYS_write, in_pipe[1], feed, sizeof(feed) - 1) == static_cast<long>(sizeof(feed) - 1));
    micron::syscall(SYS_close, in_pipe[1]);      // EOF for the reader
    long save0 = micron::syscall(SYS_dup, 0);
    long save1 = micron::syscall(SYS_dup, 1);
    micron::syscall(SYS_dup2, in_pipe[0], 0);
    micron::syscall(SYS_dup2, out_pipe[1], 1);
    max_t w = coro::sync_wait(cio::interact([](micron::string &&s) {
      micron::string up = micron::move(s);
      for ( usize i = 0; i < up.size(); ++i )
        if ( up[i] >= 'a' && up[i] <= 'z' ) up[i] = static_cast<char>(up[i] - 32);
      return up;
    }));
    micron::syscall(SYS_dup2, save0, 0);
    micron::syscall(SYS_dup2, save1, 1);
    micron::syscall(SYS_close, save0);
    micron::syscall(SYS_close, save1);
    sb::check(w == static_cast<max_t>(sizeof(feed) - 1));
    char back[64] = {};
    sb::require(micron::syscall(SYS_read, out_pipe[0], back, sizeof(back)) == static_cast<long>(sizeof(feed) - 1));
    sb::check(micron::strcmp(back, "HELLO INTERACT") == 0);
    micron::syscall(SYS_close, in_pipe[0]);
    micron::syscall(SYS_close, out_pipe[0]);
    micron::syscall(SYS_close, out_pipe[1]);
  }
  sb::end_test_case();

  sb::test_case("16 concurrent pipe pairs through the porcelain");
  {
    int pfd[16][2];
    micron::futex_future<i32> futs[16];
    for ( int i = 0; i < 16; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(pingpong(pfd[i][0], pfd[i][1], 64));
    }
    for ( int i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
    for ( int i = 0; i < 16; ++i ) {
      micron::syscall(SYS_close, pfd[i][0]);
      micron::syscall(SYS_close, pfd[i][1]);
    }
  }
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL PIPE/STDIO PORCELAIN TESTS PASSED ===");
  return 1;
}
