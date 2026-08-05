//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;

// WARNING: sb::check() only prints; it cannot fail a run on its own. Without this counter and the
// require(FAILS == 0) at the end, every case below could report a failure and main would still hit
// `return 1` -- the PASS sentinel -- and grade the suite green. Same idiom as t_uring_io.cpp
static int FAILS = 0;

static constexpr i32 c_eagain = -11;

static bool
nbpipe(i32 &rfd, i32 &wfd)
{
  i32 fds[2] = { -1, -1 };
  const long r = micron::syscall(SYS_pipe2, fds, 04000 /*O_NONBLOCK*/);
  if ( r < 0 ) return false;
  rfd = fds[0];
  wfd = fds[1];
  return true;
}

static bool
nbsocketpair(i32 &a, i32 &b)
{
  i32 fds[2] = { -1, -1 };

  const long r = micron::syscall(SYS_socketpair, 1, 1 | 04000, 0, fds);
  if ( r < 0 ) return false;
  a = fds[0];
  b = fds[1];
  return true;
}

static micron::task<i32>
suspends_until_written(i32 rfd, i32 wfd)
{

  coro::detach([](i32 w) -> micron::task<void> {
    co_await coro::sleep_for_ms(30);
    const u32 v = 0xa5a5a5a5u;
    (void)co_await cio::write_all(w, &v, sizeof(v));
  }(wfd));

  u32 back = 0;
  cio::fd_io io{ rfd };
  const max_t r = co_await io.read(&back, sizeof(back));
  if ( r != static_cast<max_t>(sizeof(back)) ) co_return -2000;
  if ( back != 0xa5a5a5a5u ) co_return -2001;
  co_return 0;
}

static micron::task<i32>
read_some_still_eagain(i32 rfd)
{
  u32 back = 0;
  cio::fd_io io{ rfd };
  const max_t r = co_await io.read_some(&back, sizeof(back));
  co_return r == static_cast<max_t>(c_eagain) ? 0 : -2100;
}

static micron::task<i32>
interleaves(i32 rfd, i32 wfd)
{
  micron::atomic_token<u32> ticks{ 0 };
  micron::atomic_token<u32> done{ 0 };

  coro::detach([](micron::atomic_token<u32> *t, micron::atomic_token<u32> *d) -> micron::task<void> {
    while ( d->get(micron::memory_order_acquire) == 0 ) {
      t->fetch_add(1, micron::memory_order_acq_rel);
      co_await coro::reschedule_fair();
    }
  }(&ticks, &done));

  coro::detach([](i32 w) -> micron::task<void> {
    co_await coro::sleep_for_ms(40);
    const u32 v = 7;
    (void)co_await cio::write_all(w, &v, sizeof(v));
  }(wfd));

  u32 back = 0;
  cio::fd_io io{ rfd };
  const max_t r = co_await io.read(&back, sizeof(back));
  const u32 seen = ticks.get(micron::memory_order_acquire);
  done.store(1, micron::memory_order_release);
  co_await coro::sleep_for_ms(10);

  if ( r != static_cast<max_t>(sizeof(back)) || back != 7 ) co_return -2200;
  if ( seen == 0 ) co_return -2201;
  sb::print("interleave: counter advanced ", seen, " times while the read was parked");
  co_return 0;
}

static micron::task<i32>
write_waits_for_drain(i32 rfd, i32 wfd)
{

  const usize big = 512u << 10;
  micron::string payload(big, 'z');

  coro::detach([](i32 r, usize n) -> micron::task<void> {
    co_await coro::sleep_for_ms(30);
    micron::buffer sink(64u << 10);
    usize got = 0;
    u32 idle = 0;
    cio::fd_io in{ r };
    while ( got < n && idle < 2000 ) {
      const max_t k = co_await in.read_some(sink.data(), sink.size());
      if ( k > 0 ) {
        got += static_cast<usize>(k);
        idle = 0;
      } else if ( k == static_cast<max_t>(c_eagain) ) {
        ++idle;
        co_await coro::sleep_for_ms(1);
      } else {
        break;
      }
    }
  }(rfd, big));

  cio::fd_io out{ wfd };
  const max_t w = co_await out.write(payload.c_str(), big);
  co_return w == static_cast<max_t>(big) ? 0 : -2300;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO NONBLOCKING FD READINESS ===");

  sb::test_case("nonblocking pipe: read_some() reports -EAGAIN verbatim (no ring needed)");
  {
    i32 rfd = -1, wfd = -1;
    sb::require(nbpipe(rfd, wfd));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(read_some_still_eagain(rfd));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    micron::syscall(SYS_close, rfd);
    micron::syscall(SYS_close, wfd);
  }
  sb::end_test_case();

  bool have_ring = false;
  coro::start_coroutine_runtime(2);
  have_ring = coro::sync_wait([]() -> micron::task<bool> { co_return cio::available(); }());
  coro::stop_coroutine_runtime();
  if ( !have_ring ) {
    sb::print("no live io_uring ring (qemu-user provides none) - skipping the suspend/resume cases");
    sb::require(FAILS == 0);      // the no-ring case above still had to pass
    sb::print("=== CORO NONBLOCKING PARTIALLY SKIPPED ===");
    return 1;
  }

  sb::test_case("nonblocking pipe: read() suspends and completes when data arrives");
  {
    i32 rfd = -1, wfd = -1;
    sb::require(nbpipe(rfd, wfd));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(suspends_until_written(rfd, wfd));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    micron::syscall(SYS_close, rfd);
    micron::syscall(SYS_close, wfd);
  }
  sb::end_test_case();

  sb::test_case("nonblocking socketpair: read() suspends and completes");
  {
    i32 a = -1, b = -1;
    sb::require(nbsocketpair(a, b));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(suspends_until_written(a, b));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    micron::syscall(SYS_close, a);
    micron::syscall(SYS_close, b);
  }
  sb::end_test_case();

  sb::test_case("a parked nonblocking read yields its worker (the interleave claim)");
  {
    i32 rfd = -1, wfd = -1;
    sb::require(nbpipe(rfd, wfd));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(interleaves(rfd, wfd));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    micron::syscall(SYS_close, rfd);
    micron::syscall(SYS_close, wfd);
  }
  sb::end_test_case();

  sb::test_case("nonblocking pipe: write() waits on POLLOUT rather than short-counting");
  {
    i32 rfd = -1, wfd = -1;
    sb::require(nbpipe(rfd, wfd));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(write_waits_for_drain(rfd, wfd));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    micron::syscall(SYS_close, rfd);
    micron::syscall(SYS_close, wfd);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL CORO NONBLOCKING TESTS PASSED ===");
  return 1;
}
