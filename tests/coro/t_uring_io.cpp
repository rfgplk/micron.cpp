

#define MICRON_CORO_URING

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::task<i32>
file_roundtrip(const char *path)
{

  i32 fd = co_await coro::io::openat(-100 /*AT_FDCWD*/, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  const char msg[] = "uring-aio-roundtrip";
  i32 wr = co_await coro::io::write(fd, msg, sizeof(msg), 0);
  if ( wr != static_cast<i32>(sizeof(msg)) ) co_return -1000;
  i32 fs = co_await coro::io::fsync(fd);
  if ( fs != 0 ) co_return -1001;
  char back[64] = {};
  i32 rd = co_await coro::io::read(fd, back, sizeof(back), 0);
  if ( rd != static_cast<i32>(sizeof(msg)) ) co_return -1002;
  for ( usize i = 0; i < sizeof(msg); ++i )
    if ( back[i] != msg[i] ) co_return -1003;
  i32 cl = co_await coro::io::close(fd);
  if ( cl != 0 ) co_return -1004;
  co_return 0;
}

static micron::task<i32>
pipe_pingpong(i32 rfd, i32 wfd, u32 rounds)
{

  for ( u32 i = 0; i < rounds; ++i ) {
    u32 v = i * 2654435761u;
    i32 wr = co_await coro::io::write(wfd, &v, sizeof(v));
    if ( wr != static_cast<i32>(sizeof(v)) ) co_return -2000;
    if ( (i & 7u) == 0u ) co_await coro::sleep_for_ms(1);
    u32 back = 0;
    i32 rd = co_await coro::io::read(rfd, &back, sizeof(back));
    if ( rd != static_cast<i32>(sizeof(back)) ) co_return -2001;
    if ( back != v ) co_return -2002;
  }
  co_return 0;
}

static micron::task<i32>
poll_then_read(i32 rfd, i32 wfd)
{

  coro::detach([](i32 w) -> micron::task<void> {
    co_await coro::sleep_for_ms(20);
    u32 v = 77;
    co_await coro::io::write(w, &v, sizeof(v));
  }(wfd));
  i32 ev = co_await coro::io::poll(rfd, 0x001 /*POLLIN*/);
  if ( ev < 0 || !(ev & 0x001) ) co_return -3000;
  u32 back = 0;
  i32 rd = co_await coro::io::read(rfd, &back, sizeof(back));
  if ( rd != static_cast<i32>(sizeof(back)) || back != 77 ) co_return -3001;
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); aio tests SKIPPED");
      return 1;
    }
  }

  coro::start_coroutine_runtime();

  sb::test_case("file open/write/fsync/pread/close via co_await");
  {
    i32 r = coro::sync_wait(file_roundtrip("/tmp/micron_t_uring_io.dat"));
    sb::check(r == 0);
  }
  sb::end_test_case();

  sb::test_case("pipe ping-pong x256 interleaved with ring timers");
  {
    long fds[2] = { 0, 0 };
    int pfd[2] = { 0, 0 };
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    (void)fds;
    i32 r = coro::sync_wait(pipe_pingpong(pfd[0], pfd[1], 256));
    sb::check(r == 0);
  }
  sb::end_test_case();

  sb::test_case("poll blocks until a detached writer fires");
  {
    int pfd[2] = { 0, 0 };
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    i32 r = coro::sync_wait(poll_then_read(pfd[0], pfd[1]));
    sb::check(r == 0);
  }
  sb::end_test_case();

  sb::test_case("16 concurrent pipe ping-pongs (fan-out over the shared ring)");
  {
    int pfd[16][2];
    micron::futex_future<i32> futs[16];
    for ( int i = 0; i < 16; ++i ) {
      sb::require(micron::syscall(SYS_pipe2, pfd[i], 0) == 0);
      futs[i] = coro::schedule(pipe_pingpong(pfd[i][0], pfd[i][1], 64));
    }
    for ( int i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL URING AIO TESTS PASSED ===");
  return 1;
}
