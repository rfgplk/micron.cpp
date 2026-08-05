//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: splice()'s -EAGAIN retry.
//
// -EAGAIN out of a two-ended op does not say WHICH end blocked, and the old code guessed: if in_fd
// was nonblocking it ALWAYS armed POLLIN on in_fd. With a full sink and a source sitting on data
// that is precisely backwards -- the poll answers "ready" immediately, the splice re-issues, gets
// -EAGAIN again, and the whole spin budget burns without ever waiting on writability. The caller
// then gets -EAGAIN or a short count and silently drops the rest of the stream.
//
// The other half: when in_fd was BLOCKING the retry went to __retry_after with out_fd, and that
// function's blocking-fd branch did reschedule_fair() and returned "retry" without touching the
// spin counter -- an unbounded loop, i.e. a task that never completes and a worker at 100%.

#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace posix = micron::posix;

static int FAILS = 0;

static constexpr usize SINK_SZ = 4096;      // one page, the smallest a pipe can be
static constexpr usize PAYLOAD = 8192;      // twice the sink, so it CANNOT fit without a reader

static bool
mkpipe(i32 &r, i32 &w, i32 flags)
{
  i32 fds[2] = { -1, -1 };
  if ( micron::syscall(SYS_pipe2, fds, flags) < 0 ) return false;
  r = fds[0];
  w = fds[1];
  return true;
}

static usize
fill(i32 fd, usize n)
{
  byte buf[1024];
  for ( usize i = 0; i < sizeof(buf); ++i ) buf[i] = static_cast<byte>('a' + (i % 26));
  usize done = 0;
  while ( done < n ) {
    const usize want = (n - done) > sizeof(buf) ? sizeof(buf) : (n - done);
    const long w = micron::syscall(SYS_write, fd, buf, want);
    if ( w <= 0 ) break;
    done += static_cast<usize>(w);
  }
  return done;
}

// drain the sink in the background so the splice has somewhere to put the payload
static micron::task<i32>
drains(i32 rfd, usize total)
{
  co_await coro::sleep_for_ms(30);      // make the splice meet a FULL sink first
  byte buf[1024];
  usize got = 0;
  while ( got < total ) {
    const max_t r = co_await cio::read_some(rfd, buf, sizeof(buf));
    if ( r < 0 ) {
      if ( r == -11 ) {
        co_await coro::sleep_for_ms(1);
        continue;
      }
      co_return static_cast<i32>(r);
    }
    if ( r == 0 ) break;
    got += static_cast<usize>(r);
  }
  co_return static_cast<i32>(got);
}

static micron::task<i32>
splice_into_full_sink(i32 src_r, i32 dst_r, i32 dst_w)
{
  coro::detach(drains(dst_r, SINK_SZ + PAYLOAD));
  const max_t moved = co_await cio::splice(src_r, dst_w, PAYLOAD);
  if ( moved < 0 ) co_return static_cast<i32>(moved);
  co_return static_cast<i32>(moved);
}

// __retry_after must SURRENDER on a blocking fd rather than reschedule forever. Calling it directly
// is the only way to pin that: the ring rarely produces -EAGAIN for a blocking fd on a modern
// kernel, which is exactly why the unbounded branch went unnoticed.
static micron::task<i32>
retry_after_is_bounded(i32 blocking_fd)
{
  i32 nb = cio::__impl::__nb_unknown;
  u32 spun = 0;
  for ( u32 i = 0; i < 4u * cio::__impl::__ready_spins + 8u; ++i ) {
    const i32 a = co_await cio::__impl::__retry_after(-11 /*EAGAIN*/, blocking_fd, micron::uring::poll_out, nb, spun);
    if ( a != 0 ) co_return static_cast<i32>(i);      // surrendered on round i
  }
  co_return -1;      // never gave up: the hang
}

static micron::task<i32>
retry_after2_is_bounded(i32 ready_in, i32 blocking_out)
{
  u32 spun = 0;
  u32 turn = 0;
  for ( u32 i = 0; i < 8u * cio::__impl::__ready_spins + 16u; ++i ) {
    const i32 a = co_await cio::__impl::__retry_after2(-11, ready_in, blocking_out, spun, turn);
    if ( a != 0 ) co_return static_cast<i32>(i);
  }
  co_return -1;
}

// both ends must actually get probed; turn advancing once per call is what makes that true
static micron::task<i32>
retry_after2_alternates(i32 ready_in, i32 blocking_out)
{
  u32 spun = 0;
  u32 turn = 0;
  (void)co_await cio::__impl::__retry_after2(-11, ready_in, blocking_out, spun, turn);
  if ( turn != 1u ) co_return -1;      // probed in
  if ( spun != 0u ) co_return -2;      // an "in" probe does not charge the budget
  (void)co_await cio::__impl::__retry_after2(-11, ready_in, blocking_out, spun, turn);
  if ( turn != 2u ) co_return -3;      // probed out
  if ( spun != 1u ) co_return -4;      // one round charged per PAIR
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO SPLICE RETRY ===");

  bool have_ring = false;
  coro::start_coroutine_runtime(2);
  have_ring = coro::sync_wait([]() -> micron::task<bool> { co_return cio::available(); }());
  coro::stop_coroutine_runtime();

  sb::test_case("__retry_after gives up on a blocking fd instead of rescheduling forever");
  {
    i32 r = -1, w = -1;
    sb::require(mkpipe(r, w, 0));      // blocking
    coro::start_coroutine_runtime(2);
    const i32 round = coro::sync_wait(retry_after_is_bounded(w));
    coro::stop_coroutine_runtime();
    sb::check(round >= 0);
    sb::check(round <= static_cast<i32>(cio::__impl::__ready_spins));
    sb::print("  surrendered on round ", round, " (budget ", cio::__impl::__ready_spins, ")");
    micron::syscall(SYS_close, r);
    micron::syscall(SYS_close, w);
  }
  sb::end_test_case();

  sb::test_case("__retry_after2 probes both ends and charges one round per pair");
  {
    i32 nr = -1, nw = -1, br = -1, bw = -1;
    sb::require(mkpipe(nr, nw, 04000 /*O_NONBLOCK*/));
    sb::require(mkpipe(br, bw, 0));
    sb::require(fill(nw, 64) == 64);      // make the "in" end genuinely readable
    coro::start_coroutine_runtime(2);
    const i32 alt = coro::sync_wait(retry_after2_alternates(nr, bw));
    const i32 round = coro::sync_wait(retry_after2_is_bounded(nr, bw));
    coro::stop_coroutine_runtime();
    sb::check(alt == 0);
    sb::check(round >= 0);
    micron::syscall(SYS_close, nr);
    micron::syscall(SYS_close, nw);
    micron::syscall(SYS_close, br);
    micron::syscall(SYS_close, bw);
  }
  sb::end_test_case();

  if ( !have_ring ) {
    sb::print("no live io_uring ring - skipping the end-to-end splice case");
    sb::require(FAILS == 0);
    sb::print("=== CORO SPLICE RETRY PARTIALLY SKIPPED ===");
    return 1;
  }

  sb::test_case("splice waits on the SINK, not the source, when the sink is what is full");
  {
    i32 src_r = -1, src_w = -1, dst_r = -1, dst_w = -1;
    sb::require(mkpipe(src_r, src_w, 04000 /*O_NONBLOCK: the exact shape that misrouted the poll*/));
    sb::require(mkpipe(dst_r, dst_w, 0));

    // shrink the sink and stuff it full, so the ONLY way forward is waiting for writability
    micron::syscall(SYS_fcntl, dst_w, posix::f_setpipe_sz, SINK_SZ);
    sb::require(fill(dst_w, SINK_SZ) == SINK_SZ);
    sb::require(fill(src_w, PAYLOAD) == PAYLOAD);
    micron::syscall(SYS_close, src_w);

    coro::start_coroutine_runtime(2);
    const i32 moved = coro::sync_wait(splice_into_full_sink(src_r, dst_r, dst_w));
    coro::stop_coroutine_runtime();

    sb::print("  spliced ", moved, " of ", PAYLOAD, " through a sink that started full");
    sb::check(moved == static_cast<i32>(PAYLOAD));

    micron::syscall(SYS_close, src_r);
    micron::syscall(SYS_close, dst_r);
    micron::syscall(SYS_close, dst_w);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== CORO SPLICE RETRY PASSED ===");
  return 1;
}
