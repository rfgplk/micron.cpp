//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/tasks/tasks.hpp"

#include "../../src/linux/sys/signal.hpp"
#include "../snowball/snowball.hpp"

// data-path op correctness: offsets, scatter-gather, partial transfers, EOF,
// errno passthrough, EINTR storms, fixed buffers, splice/tee

namespace coro = micron::coro;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_rw";

static void
mkpath(char *out, const char *name)
{
  usize k = 0;
  for ( const char *p = DIR; *p; p++ ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; p++ ) out[k++] = *p;
  out[k] = '\0';
}

static micron::task<i32>
positional_ops(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  i32 w1 = co_await coro::io::write(fd, "AB", 2, 0);
  i32 w2 = co_await coro::io::write(fd, "CD", 2, 2);
  if ( w1 != 2 || w2 != 2 ) co_return -1000;
  char back[8] = {};
  i32 r = co_await coro::io::read(fd, back, 4, 0);
  if ( r != 4 ) co_return -1001;
  if ( back[0] != 'A' || back[1] != 'B' || back[2] != 'C' || back[3] != 'D' ) co_return -1002;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
cursor_ops(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  // off == -1: use + advance the kernel file position
  i32 w1 = co_await coro::io::write(fd, "12", 2);
  i32 w2 = co_await coro::io::write(fd, "34", 2);
  if ( w1 != 2 || w2 != 2 ) co_return -1100;
  char back[8] = {};
  i32 r = co_await coro::io::read(fd, back, 4, 0);
  if ( r != 4 || back[0] != '1' || back[3] != '4' ) co_return -1101;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
vectored_ops(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  static char a[7], b[4096], c[13];
  for ( usize i = 0; i < sizeof(a); ++i ) a[i] = static_cast<char>('a' + i);
  for ( usize i = 0; i < sizeof(b); ++i ) b[i] = static_cast<char>(i * 7u);
  for ( usize i = 0; i < sizeof(c); ++i ) c[i] = static_cast<char>('A' + i);
  micron::uring::iovec iov[3] = { { a, sizeof(a) }, { b, sizeof(b) }, { c, sizeof(c) } };
  constexpr u32 total = 7 + 4096 + 13;
  i32 w = co_await coro::io::writev(fd, iov, 3, 0);
  if ( w != static_cast<i32>(total) ) co_return -1200;
  // contiguous readback equals concatenation
  static char flat[total];
  i32 r = co_await coro::io::read(fd, flat, total, 0);
  if ( r != static_cast<i32>(total) ) co_return -1201;
  for ( usize i = 0; i < sizeof(a); ++i )
    if ( flat[i] != a[i] ) co_return -1202;
  for ( usize i = 0; i < sizeof(b); ++i )
    if ( flat[7 + i] != b[i] ) co_return -1203;
  for ( usize i = 0; i < sizeof(c); ++i )
    if ( flat[7 + 4096 + i] != c[i] ) co_return -1204;
  // scatter readback reassembles byte-identically
  static char a2[7], b2[4096], c2[13];
  micron::uring::iovec iov2[3] = { { a2, sizeof(a2) }, { b2, sizeof(b2) }, { c2, sizeof(c2) } };
  i32 r2 = co_await coro::io::readv(fd, iov2, 3, 0);
  if ( r2 != static_cast<i32>(total) ) co_return -1205;
  for ( usize i = 0; i < sizeof(a); ++i )
    if ( a2[i] != a[i] ) co_return -1206;
  for ( usize i = 0; i < sizeof(b); ++i )
    if ( b2[i] != b[i] ) co_return -1207;
  for ( usize i = 0; i < sizeof(c); ++i )
    if ( c2[i] != c[i] ) co_return -1208;
  co_await coro::io::close(fd);
  co_return 0;
}

// 1 MiB through a default pipe: writer loops short writes, reader drains
static micron::atomic_token<u64> g_pipe_sum_w{ 0 };
static micron::atomic_token<u64> g_pipe_sum_r{ 0 };

static micron::task<i32>
pipe_writer(i32 wfd, u32 total)
{
  static char blk[8192];
  for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = static_cast<char>(i * 13u);
  u32 sent = 0;
  while ( sent < total ) {
    u32 want = total - sent;
    if ( want > sizeof(blk) ) want = sizeof(blk);
    i32 w = co_await coro::io::write(wfd, blk, want);
    if ( w <= 0 ) co_return -1300;
    for ( i32 i = 0; i < w; ++i ) g_pipe_sum_w.fetch_add(static_cast<u8>(blk[i]), micron::memory_order_relaxed);
    sent += static_cast<u32>(w);
  }
  co_await coro::io::close(wfd);
  co_return static_cast<i32>(sent);
}

static micron::task<i32>
pipe_reader(i32 rfd)
{
  static char blk[8192];
  u32 got = 0;
  for ( ;; ) {
    i32 r = co_await coro::io::read(rfd, blk, sizeof(blk));
    if ( r < 0 ) co_return -1301;
    if ( r == 0 ) break;      // writer closed: EOF
    for ( i32 i = 0; i < r; ++i ) g_pipe_sum_r.fetch_add(static_cast<u8>(blk[i]), micron::memory_order_relaxed);
    got += static_cast<u32>(r);
  }
  co_await coro::io::close(rfd);
  co_return static_cast<i32>(got);
}

static micron::task<i32>
short_and_eof(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  static char hundred[100];
  for ( usize i = 0; i < sizeof(hundred); ++i ) hundred[i] = static_cast<char>(i);
  i32 w = co_await coro::io::write(fd, hundred, sizeof(hundred), 0);
  if ( w != 100 ) co_return -1400;
  static char big[8192];
  i32 r = co_await coro::io::read(fd, big, sizeof(big), 0);      // short read: only 100 exist
  if ( r != 100 ) co_return -1401;
  i32 z = co_await coro::io::read(fd, big, sizeof(big), 100);      // at EOF
  if ( z != 0 ) co_return -1402;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
badfd_ops()
{
  static char b[8];
  i32 r = co_await coro::io::read(-1, b, sizeof(b), 0);
  if ( r != -9 ) co_return -1500;
  i32 w = co_await coro::io::write(-1, b, sizeof(b), 0);
  if ( w != -9 ) co_return -1501;
  i32 s = co_await coro::io::fsync(-1);
  if ( s != -9 ) co_return -1502;
  co_return 0;
}

static micron::task<i32>
pingpong(i32 rfd, i32 wfd, u32 rounds)
{
  for ( u32 i = 0; i < rounds; ++i ) {
    u32 v = i * 2654435761u;
    i32 w = co_await coro::io::write(wfd, &v, sizeof(v));
    if ( w != static_cast<i32>(sizeof(v)) ) co_return -1600;
    u32 back = 0;
    i32 r = co_await coro::io::read(rfd, &back, sizeof(back));
    if ( r != static_cast<i32>(sizeof(back)) || back != v ) co_return -1601;
  }
  co_return 0;
}

static micron::task<i32>
fixed_roundtrip(const char *path)
{
  i32 s0 = coro::acquire_fixed();
  i32 s1 = coro::acquire_fixed();
  if ( s0 < 0 || s1 < 0 ) co_return -9000;      // pool unavailable: caller skips
  byte *p0 = coro::fixed_ptr(s0);
  byte *p1 = coro::fixed_ptr(s1);
  for ( usize i = 0; i < 4096; ++i ) p0[i] = static_cast<byte>(i * 31u);
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  i32 w = co_await coro::io::write_fixed(fd, p0, 4096, 0, static_cast<u16>(s0));
  if ( w == -95 || w == -22 || w == -105 || w == -12 ) co_return -9000;      // registration refused: skip
  if ( w != 4096 ) co_return -1700;
  i32 r = co_await coro::io::read_fixed(fd, p1, 4096, 0, static_cast<u16>(s1));
  if ( r != 4096 ) co_return -1701;
  for ( usize i = 0; i < 4096; ++i )
    if ( p1[i] != p0[i] ) co_return -1702;
  // cross-check against a plain read
  static char plain[4096];
  i32 pr = co_await coro::io::read(fd, plain, sizeof(plain), 0);
  if ( pr != 4096 ) co_return -1703;
  for ( usize i = 0; i < 4096; ++i )
    if ( static_cast<byte>(plain[i]) != p0[i] ) co_return -1704;
  co_await coro::io::close(fd);
  coro::release_fixed(s0);
  coro::release_fixed(s1);
  co_return 0;
}

static micron::task<i32>
splice_chain(const char *src_path, const char *dst_path)
{
  constexpr u32 SZ = 65536;
  i32 sfd = co_await coro::io::openat(-100, src_path, 0102 | 02 | 01000, 0644);
  if ( sfd < 0 ) co_return sfd;
  static char blob[SZ];
  for ( usize i = 0; i < sizeof(blob); ++i ) blob[i] = static_cast<char>(i * 97u);
  i32 w = co_await coro::io::write(sfd, blob, SZ, 0);
  if ( w != static_cast<i32>(SZ) ) co_return -1800;
  int pfd[2];
  if ( micron::syscall(SYS_pipe2, pfd, 0) != 0 ) co_return -1801;
  i32 dfd = co_await coro::io::openat(-100, dst_path, 0102 | 02 | 01000, 0644);
  if ( dfd < 0 ) co_return dfd;
  u32 moved = 0;
  while ( moved < SZ ) {
    i32 in = co_await coro::io::splice(sfd, moved, pfd[1], static_cast<u64>(-1), SZ - moved);
    if ( in <= 0 ) co_return -1802;
    u32 drained = 0;
    while ( drained < static_cast<u32>(in) ) {
      i32 out = co_await coro::io::splice(pfd[0], static_cast<u64>(-1), dfd, moved + drained, static_cast<u32>(in) - drained);
      if ( out <= 0 ) co_return -1803;
      drained += static_cast<u32>(out);
    }
    moved += static_cast<u32>(in);
  }
  static char back[SZ];
  i32 r = co_await coro::io::read(dfd, back, SZ, 0);
  if ( r != static_cast<i32>(SZ) ) co_return -1804;
  for ( usize i = 0; i < SZ; ++i )
    if ( back[i] != blob[i] ) co_return -1805;
  co_await coro::io::close(sfd);
  co_await coro::io::close(dfd);
  micron::syscall(SYS_close, pfd[0]);
  micron::syscall(SYS_close, pfd[1]);
  co_return 0;
}

static micron::task<i32>
tee_pair()
{
  int pa[2], pb[2];
  if ( micron::syscall(SYS_pipe2, pa, 0) != 0 || micron::syscall(SYS_pipe2, pb, 0) != 0 ) co_return -1900;
  static char blob[4096];
  for ( usize i = 0; i < sizeof(blob); ++i ) blob[i] = static_cast<char>(i * 53u);
  i32 w = co_await coro::io::write(pa[1], blob, sizeof(blob));
  if ( w != static_cast<i32>(sizeof(blob)) ) co_return -1901;
  i32 t = co_await coro::io::tee(pa[0], pb[1], sizeof(blob));
  if ( t != static_cast<i32>(sizeof(blob)) ) co_return -1902;
  static char ra[4096], rb[4096];
  i32 r1 = co_await coro::io::read(pa[0], ra, sizeof(ra));
  i32 r2 = co_await coro::io::read(pb[0], rb, sizeof(rb));
  if ( r1 != static_cast<i32>(sizeof(ra)) || r2 != static_cast<i32>(sizeof(rb)) ) co_return -1903;
  for ( usize i = 0; i < sizeof(blob); ++i )
    if ( ra[i] != blob[i] || rb[i] != blob[i] ) co_return -1904;
  micron::syscall(SYS_close, pa[0]);
  micron::syscall(SYS_close, pa[1]);
  micron::syscall(SYS_close, pb[0]);
  micron::syscall(SYS_close, pb[1]);
  co_return 0;
}

static micron::task<i32>
disjoint_read(i32 fd, u64 off, u32 seed)
{
  char buf[4096];      // frame-local: 16 of these run concurrently on shared workers
  i32 r = co_await coro::io::read(fd, buf, sizeof(buf), off);
  if ( r != static_cast<i32>(sizeof(buf)) ) co_return -2000;
  // stripe content: byte = (offset + i) * 131
  for ( usize i = 0; i < sizeof(buf); ++i )
    if ( buf[i] != static_cast<char>((off + i) * 131u) ) co_return -2001;
  (void)seed;
  co_return 0;
}

static void
alarm_noop(int)
{
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); rw op tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(DIR, 0755);
  char p1[128], p2[128], p3[128], p4[128], p5[128], p6[128], p7[128];
  mkpath(p1, "positional.dat");
  mkpath(p2, "cursor.dat");
  mkpath(p3, "vectored.dat");
  mkpath(p4, "short.dat");
  mkpath(p5, "fixed.dat");
  mkpath(p6, "splice_src.dat");
  mkpath(p7, "splice_dst.dat");

  coro::start_coroutine_runtime(4);

  sb::test_case("explicit-offset positional writes compose");
  sb::check(coro::sync_wait(positional_ops(p1)) == 0);
  sb::end_test_case();

  sb::test_case("off=-1 advances the kernel cursor");
  sb::check(coro::sync_wait(cursor_ops(p2)) == 0);
  sb::end_test_case();

  sb::test_case("readv/writev 3-iov scatter-gather byte-identity");
  sb::check(coro::sync_wait(vectored_ops(p3)) == 0);
  sb::end_test_case();

  sb::test_case("1 MiB through a pipe: partial writes/reads, EOF on close");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    g_pipe_sum_w.store(0, micron::memory_order_relaxed);
    g_pipe_sum_r.store(0, micron::memory_order_relaxed);
    micron::futex_future<i32> fw = coro::schedule(pipe_writer(pfd[1], 1u << 20));
    micron::futex_future<i32> fr = coro::schedule(pipe_reader(pfd[0]));
    sb::check(fw.get() == static_cast<i32>(1u << 20));
    sb::check(fr.get() == static_cast<i32>(1u << 20));
    sb::check(g_pipe_sum_w.get(micron::memory_order_relaxed) == g_pipe_sum_r.get(micron::memory_order_relaxed));
  }
  sb::end_test_case();

  sb::test_case("short reads + 0-byte EOF");
  sb::check(coro::sync_wait(short_and_eof(p4)) == 0);
  sb::end_test_case();

  sb::test_case("widowed pipe read returns 0");
  {
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::syscall(SYS_close, pfd[1]);
    i32 r = coro::sync_wait([](i32 rfd) -> micron::task<i32> {
      char b[8];
      i32 x = co_await coro::io::read(rfd, b, sizeof(b));
      co_return x;
    }(pfd[0]));
    sb::check(r == 0);
    micron::syscall(SYS_close, pfd[0]);
  }
  sb::end_test_case();

  sb::test_case("bad fd: exact -EBADF passthrough on read/write/fsync");
  sb::check(coro::sync_wait(badfd_ops()) == 0);
  sb::end_test_case();

  sb::test_case("EINTR storm: 1ms SIGALRM barrage during 256 roundtrips");
  {
    micron::posix::sigaction_t sa{};
    sa.sigaction_handler.sa_handler = &alarm_noop;
    micron::posix::sigaction_t old{};
    sb::require(micron::posix::sigaction(14 /*SIGALRM*/, sa, &old) == 0);
    struct itv {
      long isec, iusec, vsec, vusec;
    } timer{ 0, 1000, 0, 1000 };
    micron::syscall(SYS_setitimer, 0 /*ITIMER_REAL*/, &timer, nullptr);
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    sb::check(coro::sync_wait(pingpong(pfd[0], pfd[1], 256)) == 0);
    itv off{ 0, 0, 0, 0 };
    micron::syscall(SYS_setitimer, 0, &off, nullptr);
    micron::posix::sigaction(14, old, nullptr);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("fixed-buffer roundtrip + plain-read cross-check");
  {
    i32 r = coro::sync_wait(fixed_roundtrip(p5));
    if ( r == -9000 )
      sb::print("fixed buffers unavailable (memlock/kernel); case SKIPPED");
    else
      sb::check(r == 0);
  }
  sb::end_test_case();

  sb::test_case("fixed pool exhaustion + reuse");
  {
    i32 slots[8];
    u32 got = 0;
    for ( u32 i = 0; i < 8; ++i ) {
      slots[i] = coro::acquire_fixed();
      if ( slots[i] >= 0 ) ++got;
    }
    if ( got == 8 ) {
      sb::check(coro::acquire_fixed() == -1);      // exhausted
      coro::release_fixed(slots[3]);
      i32 re = coro::acquire_fixed();
      sb::check(re == slots[3]);
      coro::release_fixed(re);
    } else {
      sb::print("fixed pool unavailable; case SKIPPED");
    }
    for ( u32 i = 0; i < 8; ++i )
      if ( slots[i] >= 0 && slots[i] != slots[3] ) coro::release_fixed(slots[i]);
  }
  sb::end_test_case();

  sb::test_case("splice file->pipe->file 64K byte-identical");
  sb::check(coro::sync_wait(splice_chain(p6, p7)) == 0);
  sb::end_test_case();

  sb::test_case("tee duplicates a pipe payload");
  sb::check(coro::sync_wait(tee_pair()) == 0);
  sb::end_test_case();

  sb::test_case("16 concurrent positional reads on ONE fd, disjoint stripes");
  {
    char p8[128];
    mkpath(p8, "shared.dat");
    long fd = micron::syscall(SYS_openat, -100, p8, 0102 | 02 | 01000, 0644);
    sb::require(fd >= 0);
    static char blk[65536];
    for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = static_cast<char>(i * 131u);
    sb::require(micron::syscall(SYS_write, fd, blk, sizeof(blk)) == static_cast<long>(sizeof(blk)));
    micron::futex_future<i32> futs[16];
    for ( u32 i = 0; i < 16; ++i ) futs[i] = coro::schedule(disjoint_read(static_cast<i32>(fd), i * 4096ull, i));
    for ( u32 i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
    micron::syscall(SYS_close, fd);
  }
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO RW OP TESTS PASSED ===");
  return 1;
}
