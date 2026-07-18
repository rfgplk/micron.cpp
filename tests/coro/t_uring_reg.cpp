//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/kernel.hpp"
#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/sys/stat.hpp"
#include "../../src/linux/sys/uring.hpp"

#include "../snowball/snowball.hpp"

namespace ur = micron::uring;
namespace px = micron::posix;
namespace kn = micron::kernel;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_uring_reg";

static void
mkpath(char *out, const char *name)
{
  usize i = 0;
  for ( const char *p = DIR; *p; p++ ) out[i++] = *p;
  out[i++] = '/';
  for ( const char *p = name; *p; p++ ) out[i++] = *p;
  out[i] = '\0';
}

static ur::cqe
one(ur::ring &r)
{
  ur::cqe c{};
  (void)r.wait_cqe(&c);
  return c;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  (void)px::mkdir(DIR, 0755);

  ur::ring r;
  if ( int rc = r.init(32); rc != 0 ) {
    sb::print("io_uring unavailable (init rc=", rc, "); register tests SKIPPED");
    return 1;
  }

  sb::test_case("probe: nop and read report supported on a live ring");
  {
    sb::check(r.supports(ur::op_nop));
    if ( kn::has(kn::feature::uring_op_read) ) sb::check(r.supports(ur::op_read));
    sb::check(!r.supports(250));
  }
  sb::end_test_case();

  if ( kn::has(kn::feature::uring_op_read) ) {
    sb::test_case("register_buffers + write_fixed/read_fixed roundtrip");
    {
      char path[128];
      mkpath(path, "/fixed.bin");
      i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, path, px::o_rdwr | px::o_create | px::o_trunc, px::mode_file));
      sb::require(fd >= 0);

      alignas(4096) static byte slab[8192];
      ur::iovec iov{ slab, sizeof(slab) };
      long rr = r.register_buffers(&iov, 1);
      if ( rr == 0 ) {
        for ( usize i = 0; i < 4096; i++ ) slab[i] = static_cast<byte>('A' + (i % 26));
        ur::sqe *s = r.get_sqe();
        ur::prep_write_fixed(s, fd, slab, 4096, 0, 0);
        s->user_data = 1;
        r.advance_sq();
        ur::cqe c = one(r);
        sb::check(c.user_data == 1 && c.res == 4096);

        byte verify[4096];
        px::pread(fd, verify, sizeof(verify), 0);
        bool ok = true;
        for ( usize i = 0; i < 4096; i++ )
          if ( verify[i] != static_cast<byte>('A' + (i % 26)) ) ok = false;
        sb::check(ok);

        for ( usize i = 4096; i < 8192; i++ ) slab[i] = 0;
        ur::sqe *s2 = r.get_sqe();
        ur::prep_read_fixed(s2, fd, slab + 4096, 4096, 0, 0);
        s2->user_data = 2;
        r.advance_sq();
        ur::cqe c2 = one(r);
        sb::check(c2.user_data == 2 && c2.res == 4096);
        bool ok2 = true;
        for ( usize i = 0; i < 4096; i++ )
          if ( slab[4096 + i] != static_cast<byte>('A' + (i % 26)) ) ok2 = false;
        sb::check(ok2);
        sb::check(r.unregister_buffers() == 0);
      } else {
        sb::print("register_buffers rc=", static_cast<i32>(rr), " (RLIMIT_MEMLOCK?); fixed-io SKIPPED");
      }
      px::close(fd);
      px::unlinkat(px::at_fdcwd, path, 0);
    }
    sb::end_test_case();
  }

  if ( kn::has(kn::feature::uring_op_read) ) {
    sb::test_case("register_files + fixed-file read via sqe_fixed_file");
    {
      char path[128];
      mkpath(path, "/regfile.txt");
      i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, path, px::o_rdwr | px::o_create | px::o_trunc, px::mode_file));
      sb::require(fd >= 0);
      const char msg[] = "registered-file payload";
      px::pwrite(fd, msg, sizeof(msg) - 1, 0);

      i32 fds[1] = { fd };
      long rf = r.register_files(fds, 1);
      sb::check(rf == 0);
      if ( rf == 0 ) {
        char buf[64] = {};
        ur::sqe *s = r.get_sqe();
        ur::prep_read(s, 0 /*slot 0*/, buf, sizeof(msg) - 1, 0);
        ur::sqe_set_fixed_file(s);
        s->user_data = 7;
        r.advance_sq();
        ur::cqe c = one(r);
        sb::check(c.user_data == 7 && c.res == static_cast<i32>(sizeof(msg) - 1));
        bool ok = true;
        for ( usize i = 0; i < sizeof(msg) - 1; i++ )
          if ( buf[i] != msg[i] ) ok = false;
        sb::check(ok);
        sb::check(r.unregister_files() == 0);
      }
      px::close(fd);
      px::unlinkat(px::at_fdcwd, path, 0);
    }
    sb::end_test_case();
  }

  if ( kn::has(kn::feature::uring_files_sparse) && r.supports(ur::op_openat) ) {
    sb::test_case("linked openat_direct -> read(fixed) -> close_direct in one submission");
    {
      char path[128];
      mkpath(path, "/chain.txt");
      i32 fd0 = static_cast<i32>(px::openat(px::at_fdcwd, path, px::o_wronly | px::o_create | px::o_trunc, px::mode_file));
      sb::require(fd0 >= 0);
      const char msg[] = "one-submission whole-file read";
      px::pwrite(fd0, msg, sizeof(msg) - 1, 0);
      px::close(fd0);

      long rs = r.register_files_sparse(4);
      sb::check(rs == 0);
      if ( rs == 0 ) {
        char buf[64] = {};
        ur::sqe *s0 = r.get_sqe();
        ur::prep_openat_direct(s0, px::at_fdcwd, path, px::o_rdonly, 0, 0 /*slot 0*/);
        s0->flags |= ur::sqe_io_link;
        s0->user_data = 10;
        r.advance_sq();
        ur::sqe *s1 = r.get_sqe();
        ur::prep_read(s1, 0, buf, sizeof(msg) - 1, 0);
        ur::sqe_set_fixed_file(s1);
        s1->flags |= ur::sqe_io_link;
        s1->user_data = 11;
        r.advance_sq();
        ur::sqe *s2 = r.get_sqe();
        ur::prep_close_direct(s2, 0);
        s2->user_data = 12;
        r.advance_sq();

        i32 open_res = -999, read_res = -999, close_res = -999;
        for ( int k = 0; k < 3; k++ ) {
          ur::cqe c = one(r);
          if ( c.user_data == 10 )
            open_res = c.res;
          else if ( c.user_data == 11 )
            read_res = c.res;
          else if ( c.user_data == 12 )
            close_res = c.res;
        }
        sb::check(open_res == 0);
        sb::check(read_res == static_cast<i32>(sizeof(msg) - 1));
        sb::check(close_res == 0);
        bool ok = true;
        for ( usize i = 0; i < sizeof(msg) - 1; i++ )
          if ( buf[i] != msg[i] ) ok = false;
        sb::check(ok);
        (void)r.unregister_files();
      }
      px::unlinkat(px::at_fdcwd, path, 0);
    }
    sb::end_test_case();
  }

  if ( r.features & ur::feat_ext_arg ) {
    sb::test_case("wait_cqe_timeout returns -62 (ETIME) on an idle ring");
    {
      ur::cqe c{};
      ur::ktimespec ts{ 0, 20000000 };
      int rc = r.wait_cqe_timeout(&c, &ts);
      sb::check(rc == -62);
    }
    sb::end_test_case();
  }

  if ( r.supports(ur::op_statx) ) {
    sb::test_case("statx op reports the size of a written file");
    {
      char path[128];
      mkpath(path, "/sx.txt");
      i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, path, px::o_wronly | px::o_create | px::o_trunc, px::mode_file));
      sb::require(fd >= 0);
      const char blob[100] = {};
      px::pwrite(fd, blob, sizeof(blob), 0);
      px::close(fd);

      px::statx_t sx{};
      ur::sqe *s = r.get_sqe();
      ur::prep_statx(s, px::at_fdcwd, path, 0, px::statx_size, &sx);
      s->user_data = 20;
      r.advance_sq();
      ur::cqe c = one(r);
      sb::check(c.user_data == 20 && c.res == 0);
      sb::check(sx.stx_size == 100);
      px::unlinkat(px::at_fdcwd, path, 0);
    }
    sb::end_test_case();
  }

  if ( r.supports(ur::op_mkdirat) && r.supports(ur::op_renameat) && r.supports(ur::op_unlinkat) ) {
    sb::test_case("mkdirat / renameat / unlinkat ops via the ring");
    {
      char a[128], b[128];
      mkpath(a, "/sub_a");
      mkpath(b, "/sub_b");
      (void)px::unlinkat(px::at_fdcwd, a, px::at_removedir);
      (void)px::unlinkat(px::at_fdcwd, b, px::at_removedir);

      ur::sqe *s = r.get_sqe();
      ur::prep_mkdirat(s, px::at_fdcwd, a, 0755);
      s->user_data = 30;
      r.advance_sq();
      ur::cqe c = one(r);
      sb::check(c.user_data == 30 && c.res == 0);

      ur::sqe *s2 = r.get_sqe();
      ur::prep_renameat(s2, px::at_fdcwd, a, px::at_fdcwd, b);
      s2->user_data = 31;
      r.advance_sq();
      ur::cqe c2 = one(r);
      sb::check(c2.user_data == 31 && c2.res == 0);

      ur::sqe *s3 = r.get_sqe();
      ur::prep_unlinkat(s3, px::at_fdcwd, b, px::at_removedir);
      s3->user_data = 32;
      r.advance_sq();
      ur::cqe c3 = one(r);
      sb::check(c3.user_data == 32 && c3.res == 0);

      px::statx_t sx{};
      sb::check(px::statx(px::at_fdcwd, b, 0, px::statx_basic_stats, sx) != 0);
    }
    sb::end_test_case();
  }

  if ( r.supports(ur::op_fallocate) ) {
    sb::test_case("fallocate op grows a file");
    {
      char path[128];
      mkpath(path, "/alloc.bin");
      i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, path, px::o_rdwr | px::o_create | px::o_trunc, px::mode_file));
      sb::require(fd >= 0);
      ur::sqe *s = r.get_sqe();
      ur::prep_fallocate(s, fd, 0, 0, 65536);
      s->user_data = 40;
      r.advance_sq();
      ur::cqe c = one(r);
      sb::check(c.user_data == 40 && c.res == 0);
      px::statx_t sx{};
      px::statx(px::at_fdcwd, path, 0, px::statx_size, sx);
      sb::check(sx.stx_size == 65536);
      px::close(fd);
      px::unlinkat(px::at_fdcwd, path, 0);
    }
    sb::end_test_case();
  }

  if ( kn::has(kn::feature::uring_reg_ring_fd) ) {
    sb::test_case("register_ring_fd then nop through the registered index");
    {
      int rr = r.register_ring_fd();
      sb::check(rr == 0);
      if ( rr == 0 ) {
        ur::sqe *s = r.get_sqe();
        ur::prep_nop(s);
        s->user_data = 50;
        r.advance_sq();
        ur::cqe c = one(r);
        sb::check(c.user_data == 50 && c.res == 0);
        sb::check(r.unregister_ring_fd() == 0);
      }
    }
    sb::end_test_case();
  }

  sb::test_case("init_best strips unsupported setup flags and still yields a working ring");
  {
    ur::ring rb;

    u32 want = ur::setup_single_issuer | ur::setup_defer_taskrun | ur::setup_coop_taskrun;
    int rc = rb.init_best(8, want);
    sb::check(rc == 0);
    if ( rc == 0 ) {
      ur::sqe *s = rb.get_sqe();
      ur::prep_nop(s);
      s->user_data = 60;
      rb.advance_sq();
      ur::cqe c{};
      sb::check(rb.wait_cqe(&c) == 0 && c.user_data == 60);

      sb::check((rb.setup_flags & ~want) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("sqe_mixed keeps the 64B stride; 64B and 128B ops share one ring");
  {
    if ( !kn::has(kn::feature::uring_sqe_mixed) ) {
      sb::print("  kernel < 7.1; sqe_mixed SKIPPED");
    } else {
      ur::ring rm;
      int rc = rm.init(64, ur::setup_sqe_mixed);
      if ( rc != 0 ) {
        sb::print("  sqe_mixed rejected by this kernel (rc=", rc, "); SKIPPED");
      } else {

        sb::check((rm.setup_flags & ur::setup_sqe_mixed) != 0);
        sb::check(rm.__sqes_len == static_cast<usize>(rm.__sq_entries) * sizeof(ur::sqe));

        ur::sqe *s = rm.get_sqe();
        sb::require(s != nullptr);
        ur::prep_nop(s);
        s->user_data = 0x64;
        rm.advance_sq();
        ur::cqe c{};
        sb::check(rm.wait_cqe(&c) == 0 && c.user_data == 0x64 && c.res == 0);

        ur::sqe *w = rm.get_sqe128();
        sb::require(w != nullptr);
        sb::check(reinterpret_cast<usize>(w) >= reinterpret_cast<usize>(rm.sqes)
                  && reinterpret_cast<usize>(w) + 128 <= reinterpret_cast<usize>(rm.sqes) + rm.__sqes_len);
        ur::prep_nop128(w);
        w->user_data = 0x128;
        rm.advance_sq(2);
        ur::cqe c2{};
        sb::check(rm.wait_cqe(&c2) == 0 && c2.user_data == 0x128);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("resize_rings re-maps sqes so post-resize submissions still reach the kernel");
  {
    if ( !kn::has(kn::feature::uring_resize_rings) ) {
      sb::print("  kernel lacks resize_rings; SKIPPED");
    } else {
      ur::ring rz;

      int rc = rz.init(8, ur::setup_defer_taskrun | ur::setup_single_issuer);
      if ( rc != 0 ) {
        sb::print("  defer_taskrun ring unavailable (rc=", rc, "); SKIPPED");
      } else {
        ur::params p{};
        p.sq_entries = 64;
        p.cq_entries = 128;
        int rr = rz.resize_rings(p);
        sb::check(rr == 0);
        if ( rr == 0 ) {

          sb::check(rz.sqes != nullptr);
          sb::check(rz.__sqes_len == static_cast<usize>(rz.__sq_entries) * sizeof(ur::sqe));

          ur::sqe *s = rz.get_sqe();
          sb::require(s != nullptr);
          ur::prep_nop(s);
          s->user_data = 0x5150;
          rz.advance_sq();
          ur::cqe c{};
          sb::check(rz.wait_cqe(&c) == 0 && c.user_data == 0x5150 && c.res == 0);

          ur::sqe *top = rz.peek_sqe(rz.__sq_entries - 1);
          sb::check(top != nullptr && reinterpret_cast<usize>(top) + sizeof(ur::sqe) <= reinterpret_cast<usize>(rz.sqes) + rz.__sqes_len);
        }
      }
    }
  }
  sb::end_test_case();

  (void)px::unlinkat(px::at_fdcwd, DIR, px::at_removedir);

  sb::require(FAILS == 0);
  sb::print("=== ALL URING REGISTER TESTS PASSED ===");
  return 1;
}
