//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING
#define MICRON_CORO_STATS

#define MICRON_CORO_PBUF_ENTRIES 8u

#include "../../src/tasks/tasks.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static u8
fp(usize pos) noexcept
{
  return static_cast<u8>(pos * 31u + 7u);
}

static bool
make_pair(int out[2]) noexcept
{
  return micron::syscall(SYS_socketpair, 1 /*AF_UNIX*/, 1 /*SOCK_STREAM*/, 0, out) == 0;
}

static bool
queue_stream(int wfd, usize total) noexcept
{
  int sz = 1 << 20;
  micron::syscall(SYS_setsockopt, wfd, 1 /*SOL_SOCKET*/, 7 /*SO_SNDBUF*/, &sz, sizeof(sz));
  static u8 chunk[4096];
  usize done = 0;
  while ( done < total ) {
    usize n = total - done;
    if ( n > sizeof(chunk) ) n = sizeof(chunk);
    for ( usize i = 0; i < n; ++i ) chunk[i] = fp(done + i);
    long w = micron::syscall(SYS_sendto, wfd, chunk, n, 0x40 /*MSG_DONTWAIT*/, 0, 0);
    if ( w <= 0 ) return false;
    done += static_cast<usize>(w);
  }
  micron::syscall(SYS_shutdown, wfd, 1 /*SHUT_WR*/);
  return true;
}

static micron::task<i32>
t_stream(int rfd, usize expect, long *rearms_out)
{
  coro::io::mrecv_t mr;
  if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -1;
  usize total = 0;
  long rearms = 0;
  for ( ;; ) {
    coro::__io_mev ev = co_await coro::io::mrecv_next(mr);
    if ( ev.__res > 0 ) {
      const byte *p = coro::io::mrecv_data(ev);
      for ( i32 i = 0; i < ev.__res; ++i )
        if ( static_cast<u8>(p[i]) != fp(total + static_cast<usize>(i)) ) {
          coro::io::mrecv_recycle(ev);
          co_return -2;
        }
      total += static_cast<usize>(ev.__res);
      coro::io::mrecv_recycle(ev);
      if ( (ev.__fl & coro::__io_mev_more) == 0 ) {
        if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -3;
        ++rearms;
      }
      continue;
    }
    if ( ev.__res == 0 ) break;
    if ( ev.__res == -105 ) {
      if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -4;
      ++rearms;
      continue;
    }
    co_return ev.__res;
  }
  *rearms_out = rearms;
  co_return total == expect ? 0 : -5;
}

static micron::task<i32>
t_hold(int rfd, usize expect)
{
  coro::io::mrecv_t mr;
  if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -1;
  coro::__io_mev held[MICRON_CORO_PBUF_ENTRIES];
  u32 nheld = 0;
  usize total = 0;
  bool saw_enobufs = false;
  for ( ;; ) {
    coro::__io_mev ev = co_await coro::io::mrecv_next(mr);
    if ( ev.__res > 0 ) {
      const byte *p = coro::io::mrecv_data(ev);
      for ( i32 i = 0; i < ev.__res; ++i )
        if ( static_cast<u8>(p[i]) != fp(total + static_cast<usize>(i)) ) co_return -2;
      total += static_cast<usize>(ev.__res);
      if ( nheld < MICRON_CORO_PBUF_ENTRIES )
        held[nheld++] = ev;
      else
        coro::io::mrecv_recycle(ev);
      if ( (ev.__fl & coro::__io_mev_more) == 0 ) {
        for ( u32 i = 0; i < nheld; ++i ) coro::io::mrecv_recycle(held[i]);
        nheld = 0;
        if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -3;
      }
      continue;
    }
    if ( ev.__res == 0 ) break;
    if ( ev.__res == -105 ) {
      saw_enobufs = true;
      for ( u32 i = 0; i < nheld; ++i ) coro::io::mrecv_recycle(held[i]);
      nheld = 0;
      if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -4;
      continue;
    }
    co_return ev.__res;
  }
  if ( !saw_enobufs ) co_return -6;
  co_return total == expect ? 0 : -5;
}

static bool
queue_open(int wfd, usize total) noexcept
{
  static u8 chunk[4096];
  usize done = 0;
  while ( done < total ) {
    usize n = total - done;
    if ( n > sizeof(chunk) ) n = sizeof(chunk);
    for ( usize i = 0; i < n; ++i ) chunk[i] = fp(done + i);
    long w = micron::syscall(SYS_sendto, wfd, chunk, n, 0x40 /*MSG_DONTWAIT*/, 0, 0);
    if ( w <= 0 ) return false;
    done += static_cast<usize>(w);
  }
  return true;
}

static micron::task<i32>
t_cancel(int rfd)
{
  coro::io::mrecv_t mr;
  if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -1;

  coro::__io_mev ev = co_await coro::io::mrecv_next(mr);
  if ( ev.__res <= 0 ) co_return -2;
  if ( (ev.__fl & coro::__io_mev_more) == 0 ) co_return -3;
  coro::io::mrecv_recycle(ev);
  if ( !coro::io::mrecv_live(mr) ) co_return -4;

  coro::io::mrecv_cancel(mr);
  if ( coro::io::mrecv_live(mr) ) co_return -5;

  coro::__io_mev after = co_await coro::io::mrecv_next(mr);
  if ( after.__res != -61 ) co_return -6;

  coro::io::mrecv_cancel(mr);
  if ( coro::io::mrecv_live(mr) ) co_return -7;

  if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -8;
  if ( !coro::io::mrecv_live(mr) ) co_return -9;
  coro::io::mrecv_cancel(mr);
  if ( coro::io::mrecv_live(mr) ) co_return -10;
  co_return 0;
}

static micron::task<i32>
t_guards(int rfd)
{
  coro::io::mrecv_t cold;
  coro::__io_mev ev = co_await coro::io::mrecv_next(cold);
  if ( ev.__res != -61 ) co_return -1;

  coro::io::mrecv_t mr;
  if ( coro::io::mrecv_arm(mr, rfd) != 0 ) co_return -2;
  if ( coro::io::mrecv_arm(mr, rfd) != -114 ) co_return -3;

  micron::syscall(SYS_shutdown, rfd, 2 /*SHUT_RDWR*/);
  for ( ;; ) {
    coro::__io_mev e2 = co_await coro::io::mrecv_next(mr);
    if ( e2.__res > 0 ) {
      coro::io::mrecv_recycle(e2);
      if ( (e2.__fl & coro::__io_mev_more) != 0 ) continue;
    }
    break;
  }
  co_return 0;
}

int
main()
{
  sb::test_case("multishot recv: byte-exact stream, exhaustion re-arm cycles, EOF");
  {
    int fds[2];
    sb::require(make_pair(fds));
    constexpr usize N = 256 * 1024;
    sb::require(queue_stream(fds[1], N));
    coro::start_coroutine_runtime(2);
    long rearms = 0;
    sb::check(coro::sync_wait(t_stream(fds[0], N, &rearms)) == 0);
    sb::print("stream: ", N, " bytes, re-arms=", rearms);
    sb::check(rearms >= 1);
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_close, fds[0]);
    micron::syscall(SYS_close, fds[1]);
  }
  sb::end_test_case();

  sb::test_case("multishot recv: held buffers dry the pool -> -ENOBUFS terminal -> recover");
  {
    int fds[2];
    sb::require(make_pair(fds));
    constexpr usize N = 128 * 1024;
    sb::require(queue_stream(fds[1], N));
    coro::start_coroutine_runtime(2);
    sb::check(coro::sync_wait(t_hold(fds[0], N)) == 0);
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_close, fds[0]);
    micron::syscall(SYS_close, fds[1]);
  }
  sb::end_test_case();

  sb::test_case("multishot recv: misuse guards (-61 unarmed, -114 double-arm, -95 off-worker)");
  {
    int fds[2];
    sb::require(make_pair(fds));
    coro::start_coroutine_runtime(2);
    coro::io::mrecv_t off;
    sb::check(coro::io::mrecv_arm(off, fds[0]) == -95);
    sb::check(coro::sync_wait(t_guards(fds[0])) == 0);
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_close, fds[0]);
    micron::syscall(SYS_close, fds[1]);
  }
  sb::end_test_case();

  sb::test_case("multishot recv: mrecv_cancel tears down an ARMED op (live -> dead, re-armable)");
  {
    coro::io::mrecv_t never;
    coro::io::mrecv_cancel(never);
    sb::require(!coro::io::mrecv_live(never));

    int fds[2];
    sb::require(make_pair(fds));

    sb::require(queue_open(fds[1], 1024));
    coro::start_coroutine_runtime(2);
    sb::require(coro::sync_wait(t_cancel(fds[0])) == 0);
    coro::stop_coroutine_runtime();
    micron::syscall(SYS_close, fds[0]);
    micron::syscall(SYS_close, fds[1]);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL MULTISHOT RECV TESTS PASSED ===");
  return 1;
}
