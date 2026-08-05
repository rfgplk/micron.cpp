//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: what fd classification decides, and the two things it got wrong.
//
// (1) -ESPIPE. file::read_some/write_some route through __read_once/__write_once with off =
//     __cursor, which selected pread/pwrite. On a pipe, FIFO, socket or tty that is -ESPIPE
//     forever: the read never makes progress. The ring path the change replaced worked, because
//     IORING_OP_READ takes the stream form for a non-seekable file.
//
// (2) A blocked worker. The bare-syscall bypass fired for EVERY O_NONBLOCK fd -- but O_NONBLOCK is
//     a no-op on a regular file, which always reports ready and still waits on the disk. A file
//     opened defensively with .nonblock therefore ran a synchronous pread on a coroutine worker,
//     stalling every other task queued on it (and, on a defer_taskrun ring, that ring's pumping).
//
// The predicate is the fix, so the predicate is what gets pinned here: "nonblocking" has to mean
// nonblocking AND pollable, and a positional op is only legal on something seekable.

#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace posix = micron::posix;

static int FAILS = 0;

static constexpr i32 c_espipe = -29;
static constexpr const char *REG = "/var/tmp/micron_stream_some.dat";
static constexpr const char *FIFO = "/var/tmp/micron_stream_some.fifo";

static bool
mkpipe(i32 &r, i32 &w, i32 flags)
{
  i32 fds[2] = { -1, -1 };
  if ( micron::syscall(SYS_pipe2, fds, flags) < 0 ) return false;
  r = fds[0];
  w = fds[1];
  return true;
}

static bool
mkreg(const char *p, const char *body, usize n)
{
  const long fd = micron::syscall(SYS_openat, -100, p, posix::o_create | posix::o_rdwr | posix::o_trunc, 0644);
  if ( fd < 0 ) return false;
  const long w = micron::syscall(SYS_write, fd, body, n);
  micron::syscall(SYS_close, fd);
  return static_cast<usize>(w) == n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the predicate

struct fdclass {
  bool nonblocking;
  bool seekable;
};

static fdclass
classify(i32 fd)
{
  i32 c = cio::__impl::__nb_unknown;
  const bool nb = cio::__impl::__fd_nonblocking(fd, c);
  const bool sk = cio::__impl::__fd_seekable(fd, c);
  return { nb, sk };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// end to end

static micron::task<i32>
some_on_stream(i32 rfd, i32 wfd)
{
  cio::file f(posix::fd_t{ rfd }, "pipe");
  micron::syscall(SYS_write, wfd, "hello wave", 10);

  byte buf[64]{};
  const max_t r = co_await f.read_some(buf, sizeof(buf));
  if ( r == c_espipe ) co_return -4001;      // the defect, verbatim
  if ( r != 10 ) co_return -4002;
  if ( buf[0] != 'h' || buf[9] != 'e' ) co_return -4003;

  // and again: the cursor advanced past the first read, which on a stream must change nothing
  micron::syscall(SYS_write, wfd, "second", 6);
  const max_t r2 = co_await f.read_some(buf, sizeof(buf));
  if ( r2 == c_espipe ) co_return -4004;
  if ( r2 != 6 ) co_return -4005;
  if ( buf[0] != 's' ) co_return -4006;

  (void)f.release();      // the caller owns these fds
  co_return 0;
}

static micron::task<i32>
full_on_stream(i32 rfd, i32 wfd)
{
  cio::file f(posix::fd_t{ rfd }, "pipe");
  micron::syscall(SYS_write, wfd, "0123456789abcdef", 16);
  byte buf[16]{};
  const max_t r = co_await f.read(buf, sizeof(buf));      // __read_full, same offset hazard
  (void)f.release();
  if ( r == c_espipe ) co_return -4101;
  if ( r != 16 ) co_return -4102;
  if ( buf[15] != 'f' ) co_return -4103;
  co_return 0;
}

static micron::task<i32>
write_some_on_stream(i32 rfd, i32 wfd)
{
  cio::file f(posix::fd_t{ wfd }, "pipe");
  const max_t w = co_await f.write_some("abcdefgh", 8);
  (void)f.release();
  if ( w == c_espipe ) co_return -4201;
  if ( w != 8 ) co_return -4202;
  byte back[8]{};
  if ( micron::syscall(SYS_read, rfd, back, 8) != 8 ) co_return -4203;
  if ( back[0] != 'a' || back[7] != 'h' ) co_return -4204;
  co_return 0;
}

// a regular file must keep its positional semantics: the cursor is the whole point there
static micron::task<i32>
regular_file_keeps_its_cursor()
{
  cio::file f(micron::io::path_t(REG), micron::io::modes::read);
  if ( !f.valid() ) co_return -4301;
  byte a[4]{};
  byte b[4]{};
  const max_t r1 = co_await f.read_some(a, 4);
  const max_t r2 = co_await f.read_some(b, 4);
  if ( r1 != 4 || r2 != 4 ) co_return -4302;
  if ( a[0] != 'A' || b[0] != 'E' ) co_return -4303;      // consecutive, not both from offset 0
  if ( f.tell() != 8 ) co_return -4304;
  co_return 0;
}

// the same file with O_NONBLOCK set: still positional, still correct, and (the fix) still on the
// ring rather than a blocking pread on this worker
static micron::task<i32>
nonblock_regular_file_still_reads()
{
  cio::file f(micron::io::path_t(REG), micron::io::modes::read, micron::io::open_opts{ .nonblock = true });
  if ( !f.valid() ) co_return -4401;
  byte a[4]{};
  byte b[4]{};
  const max_t r1 = co_await f.read_some(a, 4);
  const max_t r2 = co_await f.read_some(b, 4);
  if ( r1 != 4 || r2 != 4 ) co_return -4402;
  if ( a[0] != 'A' || b[0] != 'E' ) co_return -4403;
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO STREAM *_some ===");

  sb::require(mkreg(REG, "ABCDEFGHIJKLMNOP", 16));

  sb::test_case("O_NONBLOCK on a regular file does not make it a nonblocking fd");
  {
    const long rfd = micron::syscall(SYS_openat, -100, REG, posix::o_rdonly, 0);
    sb::require(rfd >= 0);
    const long nfd = micron::syscall(SYS_openat, -100, REG, posix::o_rdonly | posix::o_nonblock, 0);
    sb::require(nfd >= 0);

    const fdclass reg = classify(static_cast<i32>(rfd));
    const fdclass regnb = classify(static_cast<i32>(nfd));

    sb::check(!reg.nonblocking && reg.seekable);
    // THE regression: O_NONBLOCK set, but a regular file is not pollable, so it is not "nonblocking"
    sb::check(!regnb.nonblocking);
    sb::check(regnb.seekable);

    micron::syscall(SYS_close, rfd);
    micron::syscall(SYS_close, nfd);
  }
  sb::end_test_case();

  sb::test_case("a nonblocking pipe IS a nonblocking fd, and is never seekable");
  {
    i32 nr = -1, nw = -1, br = -1, bw = -1;
    sb::require(mkpipe(nr, nw, 04000 /*O_NONBLOCK*/));
    sb::require(mkpipe(br, bw, 0));

    const fdclass nb = classify(nr);
    const fdclass bl = classify(br);

    sb::check(nb.nonblocking && !nb.seekable);
    sb::check(!bl.nonblocking && !bl.seekable);

    micron::syscall(SYS_close, nr);
    micron::syscall(SYS_close, nw);
    micron::syscall(SYS_close, br);
    micron::syscall(SYS_close, bw);
  }
  sb::end_test_case();

  bool have_ring = false;
  coro::start_coroutine_runtime(2);
  have_ring = coro::sync_wait([]() -> micron::task<bool> { co_return cio::available(); }());
  coro::stop_coroutine_runtime();
  if ( !have_ring ) {
    sb::print("no live io_uring ring - skipping the transfer cases");
    sb::require(FAILS == 0);
    sb::print("=== CORO STREAM *_some PARTIALLY SKIPPED ===");
    return 1;
  }

  sb::test_case("file::read_some on an adopted pipe reads, it does not answer -ESPIPE");
  {
    i32 r = -1, w = -1;
    sb::require(mkpipe(r, w, 0));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(some_on_stream(r, w));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    if ( rc != 0 ) sb::print("  rc = ", rc);
    micron::syscall(SYS_close, r);
    micron::syscall(SYS_close, w);
  }
  sb::end_test_case();

  sb::test_case("file::read on an adopted pipe reads too (the __read_full path)");
  {
    i32 r = -1, w = -1;
    sb::require(mkpipe(r, w, 0));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(full_on_stream(r, w));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    if ( rc != 0 ) sb::print("  rc = ", rc);
    micron::syscall(SYS_close, r);
    micron::syscall(SYS_close, w);
  }
  sb::end_test_case();

  sb::test_case("file::write_some on an adopted pipe writes");
  {
    i32 r = -1, w = -1;
    sb::require(mkpipe(r, w, 0));
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(write_some_on_stream(r, w));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    if ( rc != 0 ) sb::print("  rc = ", rc);
    micron::syscall(SYS_close, r);
    micron::syscall(SYS_close, w);
  }
  sb::end_test_case();

  sb::test_case("a regular file still reads positionally, with and without O_NONBLOCK");
  {
    coro::start_coroutine_runtime(2);
    const i32 a = coro::sync_wait(regular_file_keeps_its_cursor());
    const i32 b = coro::sync_wait(nonblock_regular_file_still_reads());
    coro::stop_coroutine_runtime();
    sb::check(a == 0);
    sb::check(b == 0);
    if ( a != 0 || b != 0 ) sb::print("  rc = ", a, " / ", b);
  }
  sb::end_test_case();

  micron::syscall(SYS_unlinkat, -100, REG, 0);
  micron::syscall(SYS_unlinkat, -100, FIFO, 0);
  sb::require(FAILS == 0);
  sb::print("=== CORO STREAM *_some PASSED ===");
  return 1;
}
