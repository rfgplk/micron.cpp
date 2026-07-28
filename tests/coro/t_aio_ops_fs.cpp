//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/tasks/tasks.hpp"

#include "../../src/linux/sys/stat.hpp"
#include "../snowball/snowball.hpp"

// metadata/filesystem op correctness: openat2/statx/mkdirat/renameat/unlinkat/
// ftruncate/fallocate/fadvise/sync_file_range, with per-op supports() gating

namespace coro = micron::coro;
namespace ur = micron::uring;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_fs";

static void
mkpath(char *out, const char *name)
{
  usize k = 0;
  for ( const char *p = DIR; *p; p++ ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; p++ ) out[k++] = *p;
  out[k] = '\0';
}

static ur::ring g_probe;      // supports() oracle

static micron::task<i32>
open_write_close(const char *path, const void *data, u32 n)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  i32 w = co_await coro::io::write(fd, data, n, 0);
  if ( w != static_cast<i32>(n) ) co_return -1000;
  i32 c = co_await coro::io::close(fd);
  co_return c;
}

static micron::task<i32>
open_missing(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0 /*O_RDONLY*/, 0);
  co_return fd;
}

struct open_how_t {
  u64 flags;
  u64 mode;
  u64 resolve;
};

static micron::task<i32>
openat2_ops(i32 dirfd, const char *inside, const char *escape)
{
  open_how_t ok_how{ 0 /*O_RDONLY*/, 0, 0x08 /*RESOLVE_BENEATH*/ };
  i32 fd = co_await coro::io::openat2(dirfd, inside, &ok_how, sizeof(open_how_t));
  if ( fd < 0 ) co_return -1100;
  co_await coro::io::close(fd);
  open_how_t esc_how{ 0, 0, 0x08 };
  i32 bad = co_await coro::io::openat2(dirfd, escape, &esc_how, sizeof(open_how_t));
  if ( bad >= 0 ) co_return -1101;      // escape must be denied
  if ( bad != -18 /*EXDEV*/ ) co_return bad;
  co_return 0;
}

static micron::task<i32>
statx_ops(const char *path, micron::posix::statx_t *out)
{
  i32 r = co_await coro::io::statx(-100, path, 0, 0x7ff /*STATX_BASIC_STATS*/, out);
  co_return r;
}

static micron::task<i32>
mkdir_rename_unlink(const char *d1, const char *d2, const char *f1, const char *f2)
{
  i32 m = co_await coro::io::mkdirat(-100, d1, 0755);
  if ( m != 0 ) co_return -1200;
  micron::posix::statx_t sx{};
  i32 s = co_await coro::io::statx(-100, d1, 0, 0x7ff, &sx);
  if ( s != 0 || (sx.stx_mode & 0170000) != 0040000 /*S_IFDIR*/ ) co_return -1201;
  // file create + rename
  i32 fd = co_await coro::io::openat(-100, f1, 0102 | 02, 0644);
  if ( fd < 0 ) co_return -1202;
  i32 w = co_await coro::io::write(fd, "xyz", 3, 0);
  if ( w != 3 ) co_return -1203;
  co_await coro::io::close(fd);
  i32 rn = co_await coro::io::renameat(-100, f1, -100, f2);
  if ( rn != 0 ) co_return -1204;
  i32 gone = co_await coro::io::statx(-100, f1, 0, 0x7ff, &sx);
  if ( gone != -2 /*ENOENT*/ ) co_return -1205;
  i32 there = co_await coro::io::statx(-100, f2, 0, 0x7ff, &sx);
  if ( there != 0 || sx.stx_size != 3 ) co_return -1206;
  // dir rename
  i32 rd = co_await coro::io::renameat(-100, d1, -100, d2);
  if ( rd != 0 ) co_return -1207;
  // unlink file + dir
  i32 uf = co_await coro::io::unlinkat(-100, f2, 0);
  if ( uf != 0 ) co_return -1208;
  i32 ud = co_await coro::io::unlinkat(-100, d2, 0x200 /*AT_REMOVEDIR*/);
  if ( ud != 0 ) co_return -1209;
  i32 miss = co_await coro::io::unlinkat(-100, f2, 0);
  if ( miss != -2 ) co_return -1210;
  co_return 0;
}

static micron::task<i32>
size_ops(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  i32 w = co_await coro::io::write(fd, "0123456789", 10, 0);
  if ( w != 10 ) co_return -1300;
  micron::posix::statx_t sx{};
  // ftruncate grow (6.9+; skip-mapped by caller on -EINVAL)
  i32 tg = co_await coro::io::ftruncate(fd, 4096);
  if ( tg == -22 ) co_return -9000;      // op unsupported on this kernel
  if ( tg != 0 ) co_return -1301;
  i32 s1 = co_await coro::io::statx(-100, path, 0, 0x7ff, &sx);
  if ( s1 != 0 || sx.stx_size != 4096 ) co_return -1302;
  // shrink
  i32 ts = co_await coro::io::ftruncate(fd, 5);
  if ( ts != 0 ) co_return -1303;
  i32 s2 = co_await coro::io::statx(-100, path, 0, 0x7ff, &sx);
  if ( s2 != 0 || sx.stx_size != 5 ) co_return -1304;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
fallocate_ops(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  i32 fa = co_await coro::io::fallocate(fd, 0, 0, 65536);
  if ( fa != 0 ) co_return -1400;
  micron::posix::statx_t sx{};
  i32 s = co_await coro::io::statx(-100, path, 0, 0x7ff, &sx);
  if ( s != 0 || sx.stx_size != 65536 ) co_return -1401;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
advise_and_range(const char *path)
{
  i32 fd = co_await coro::io::openat(-100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) co_return fd;
  static char blk[16384];
  i32 w = co_await coro::io::write(fd, blk, sizeof(blk), 0);
  if ( w != static_cast<i32>(sizeof(blk)) ) co_return -1500;
  i32 fa = co_await coro::io::fadvise(fd, 0, sizeof(blk), 2 /*POSIX_FADV_SEQUENTIAL*/);
  if ( fa != 0 ) co_return -1501;
  i32 fd2 = co_await coro::io::fadvise(fd, 0, sizeof(blk), 4 /*POSIX_FADV_DONTNEED*/);
  if ( fd2 != 0 ) co_return -1502;
  i32 sr = co_await coro::io::sync_file_range(fd, sizeof(blk), 0, 0);
  if ( sr != 0 ) co_return -1503;
  i32 fs = co_await coro::io::fsync(fd, micron::uring::fsync_datasync);
  if ( fs != 0 ) co_return -1504;
  co_await coro::io::close(fd);
  co_return 0;
}

static micron::task<i32>
link_ops(const char *target, const char *sym, const char *hard)
{
  i32 s = co_await coro::io::symlinkat(target, -100, sym);
  if ( s != 0 ) co_return -1600;
  micron::posix::statx_t sx{};
  i32 st = co_await coro::io::statx(-100, sym, 0x100 /*AT_SYMLINK_NOFOLLOW*/, 0x7ff, &sx);
  if ( st != 0 || (sx.stx_mode & 0170000) != 0120000 /*S_IFLNK*/ ) co_return -1601;
  i32 h = co_await coro::io::linkat(-100, target, -100, hard, 0);
  if ( h != 0 ) co_return -1602;
  i32 st2 = co_await coro::io::statx(-100, hard, 0, 0x7ff, &sx);
  if ( st2 != 0 || sx.stx_nlink < 2 ) co_return -1603;
  co_await coro::io::unlinkat(-100, sym, 0);
  co_await coro::io::unlinkat(-100, hard, 0);
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  if ( int rc = g_probe.init(8); rc != 0 ) {
    sb::print("io_uring unavailable (rc=", rc, "); fs op tests SKIPPED");
    return 1;
  }

  (void)micron::posix::mkdir(DIR, 0755);
  coro::start_coroutine_runtime(2);

  char f_base[128], f_missing[128], d1[128], d2[128], f1[128], f2[128], f_sz[128], f_fa[128], f_adv[128], f_sym[128], f_hard[128];
  mkpath(f_base, "base.dat");
  mkpath(f_missing, "does_not_exist.dat");
  mkpath(d1, "dir_a");
  mkpath(d2, "dir_b");
  mkpath(f1, "ren_from.dat");
  mkpath(f2, "ren_to.dat");
  mkpath(f_sz, "sized.dat");
  mkpath(f_fa, "falloc.dat");
  mkpath(f_adv, "advise.dat");
  mkpath(f_sym, "sym.lnk");
  mkpath(f_hard, "hard.lnk");

  sb::test_case("openat create + write + close; missing path is -ENOENT");
  {
    sb::check(coro::sync_wait(open_write_close(f_base, "hello", 5)) == 0);
    sb::check(coro::sync_wait(open_missing(f_missing)) == -2);
  }
  sb::end_test_case();

  sb::test_case("openat2 RESOLVE_BENEATH allows inside, denies escape");
  {
    if ( g_probe.supports(ur::op_openat2) ) {
      long dfd = micron::syscall(SYS_openat, -100, DIR, 0200000 /*O_DIRECTORY*/, 0);
      sb::require(dfd >= 0);
      sb::check(coro::sync_wait(openat2_ops(static_cast<i32>(dfd), "base.dat", "../micron_aio_fs/base.dat")) == 0);
      micron::syscall(SYS_close, dfd);
    } else {
      sb::print("op_openat2 unsupported; case SKIPPED");
    }
  }
  sb::end_test_case();

  sb::test_case("statx agrees with statx(2) on size/mode/mtime");
  {
    if ( g_probe.supports(ur::op_statx) ) {
      micron::posix::statx_t via_ring{};
      sb::check(coro::sync_wait(statx_ops(f_base, &via_ring)) == 0);
      micron::posix::statx_t oracle{};
      sb::require(micron::syscall(SYS_statx, -100, f_base, 0, 0x7ff, &oracle) == 0);
      sb::check(via_ring.stx_size == oracle.stx_size && via_ring.stx_size == 5);
      sb::check(via_ring.stx_mode == oracle.stx_mode);
      sb::check(via_ring.stx_mtime.tv_sec == oracle.stx_mtime.tv_sec);
    } else {
      sb::print("op_statx unsupported; case SKIPPED");
    }
  }
  sb::end_test_case();

  sb::test_case("mkdirat/renameat/unlinkat lifecycle");
  {
    if ( g_probe.supports(ur::op_mkdirat) && g_probe.supports(ur::op_renameat) && g_probe.supports(ur::op_unlinkat) )
      sb::check(coro::sync_wait(mkdir_rename_unlink(d1, d2, f1, f2)) == 0);
    else
      sb::print("mkdirat/renameat/unlinkat unsupported; case SKIPPED");
  }
  sb::end_test_case();

  sb::test_case("ftruncate grow + shrink, statx sizes agree");
  {
    if ( g_probe.supports(ur::op_ftruncate) ) {
      i32 r = coro::sync_wait(size_ops(f_sz));
      if ( r == -9000 )
        sb::print("op_ftruncate rejected at runtime; case SKIPPED");
      else
        sb::check(r == 0);
    } else {
      sb::print("op_ftruncate unsupported (<6.9); case SKIPPED");
    }
  }
  sb::end_test_case();

  sb::test_case("fallocate 64K reflected in statx size");
  sb::check(coro::sync_wait(fallocate_ops(f_fa)) == 0);
  sb::end_test_case();

  sb::test_case("fadvise + sync_file_range + fdatasync return 0");
  sb::check(coro::sync_wait(advise_and_range(f_adv)) == 0);
  sb::end_test_case();

  sb::test_case("symlinkat + linkat + NOFOLLOW statx");
  {
    if ( g_probe.supports(ur::op_symlinkat) && g_probe.supports(ur::op_linkat) )
      sb::check(coro::sync_wait(link_ops(f_base, f_sym, f_hard)) == 0);
    else
      sb::print("symlinkat/linkat unsupported; case SKIPPED");
  }
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  // cleanup
  micron::syscall(SYS_unlinkat, -100, f_base, 0);
  micron::syscall(SYS_unlinkat, -100, f_sz, 0);
  micron::syscall(SYS_unlinkat, -100, f_fa, 0);
  micron::syscall(SYS_unlinkat, -100, f_adv, 0);

  sb::require(FAILS == 0);
  sb::print("=== ALL AIO FS OP TESTS PASSED ===");
  return 1;
}
