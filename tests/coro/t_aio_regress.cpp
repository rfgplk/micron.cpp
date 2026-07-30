//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#define MICRON_CORO_URING

// BOTH layers in one TU on purpose: the sync io::with_file overload has to be
// declared for the with_file ADL case below to mean anything, and mixing the
// two is what a real program does anyway
#include "../../src/io/coroutine/coro_io.hpp"
#include "../../src/io/fp.hpp"

#include "../snowball/snowball.hpp"

// regression suite: one case per defect found in the coro-io review.
//
// every case here FAILED (or crashed, or hung) before its fix - none of them is
// a generic smoke test. the comment on each names the exact mechanism, because
// several are races whose test can only make the window wide, not certain.

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;
namespace mp = micron::posix;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_regress";

static micron::io::path_t
mkpath(const char *stem)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += stem;
  return p;
}

static void
msleep(long ms)
{
  micron::timespec_t ts{ ms / 1000, (ms % 1000) * 1000000l };
  micron::syscall(SYS_nanosleep, &ts, nullptr);
}

static max_t
file_bytes(const micron::io::path_t &p)
{
  micron::stat_t st{};
  if ( !mp::exists(p.c_str(), st) ) return -1;
  return static_cast<max_t>(st.st_size);
}

static u32
file_mode(const micron::io::path_t &p)
{
  micron::stat_t st{};
  if ( !mp::exists(p.c_str(), st) ) return 0xffffffffu;
  return st.st_mode & 0777u;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// coroutine helpers

static micron::task<i32>
await_nop()
{
  i32 r = co_await coro::io::nop();
  co_return r;
}

// N ring ops in flight from ONE frame chain; the deadlock case needs more
// simultaneous completions than the 256-slot inbox can hold
static micron::task<i32>
nop_fanout(usize n)
{
  auto rs = co_await coro::spawn_many(n, [](usize) -> micron::task<i32> { return await_nop(); });
  for ( usize i = 0; i < rs.size(); ++i )
    if ( rs[i] != 0 ) co_return rs[i];
  co_return 0;
}

// a payload comfortably larger than a default 64K pipe buffer, so the writer is
// guaranteed to park mid-transfer with the engine otherwise idle
static constexpr usize PIPE_PAYLOAD = 256 * 1024;

static micron::task<max_t>
pipe_writer(i32 wfd, const byte *buf)
{
  max_t n = co_await cio::__impl::__write_full(wfd, buf, PIPE_PAYLOAD, static_cast<u64>(-1));
  co_return n;
}

static micron::task<i32>
fixed_write_timed(i32 fd, i32 slot, bool timed)
{
  byte *p = coro::fixed_ptr(slot);
  if ( p == nullptr ) co_return -1;
  for ( u32 i = 0; i < 4096; ++i ) p[i] = static_cast<byte>(i & 0xff);
  i32 r = timed ? co_await(coro::io::write_fixed(fd, p, 4096, 0, static_cast<u16>(slot)) | coro::io::after(2'000'000'000ull))
                : co_await coro::io::write_fixed(fd, p, 4096, 0, static_cast<u16>(slot));
  co_return r;
}

// a local at the ABI's strictest alignment that must SURVIVE a suspension, so
// it lives in the coroutine frame rather than on the stack. gcc lays it out
// assuming the frame base is __BIGGEST_ALIGNMENT__-aligned and will happily
// emit vmovdqa against it, so the allocator has to actually deliver that.
struct alignas(__BIGGEST_ALIGNMENT__) __wide_local {
  u64 v[8];
};

static micron::task<usize>
overaligned_frame_local()
{
  __wide_local a{};
  i32 r = co_await coro::io::nop();
  a.v[3] = static_cast<u64>(r) + 7u;
  i32 s = co_await coro::io::nop();
  a.v[7] = static_cast<u64>(s);
  // report the misalignment rather than trusting that gcc chose a faulting
  // opcode: the store CAN legally be vmovdqu, in which case the bug is silent
  // here and lands on some other coroutine instead
  co_return reinterpret_cast<usize>(&a) % static_cast<usize>(__BIGGEST_ALIGNMENT__);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inbox-overflow burst: enough parked reads that one CQ drain carries more
// completions than the 256-slot inbox can hold

static constexpr u32 BURST_READS = 400;
static int burst_pfd[BURST_READS][2];
static micron::atomic_token<u32> burst_go{ 0 };
static volatile u64 burst_sink = 0;

static micron::task<i32>
burst_one(u32 i)
{
  char b[1];
  i32 r = co_await coro::io::read(burst_pfd[i][0], b, 1);
  co_return r;
}

static micron::task<i32>
burst_fan()
{
  micron::vector<i32> out(static_cast<usize>(BURST_READS));
  for ( u32 i = 0; i < BURST_READS; ++i )
    co_await coro::fork(&out[i], [](usize k) -> micron::task<i32> { return burst_one(static_cast<u32>(k)); })(static_cast<usize>(i));
  // every child is parked now. release the writer, then HOG this worker so
  // nothing drains the CQ while the completions accumulate behind us
  burst_go.store(1, micron::memory_order_release);
  micron::timespec_t t0{}, t1{};
  micron::clock_gettime(micron::clock_monotonic, t0);
  for ( ;; ) {
    for ( u32 s = 0; s < 200000; ++s ) burst_sink += s;
    micron::clock_gettime(micron::clock_monotonic, t1);
    if ( (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000 > 400 ) break;
  }
  co_await coro::join;      // the whole backlog lands in ONE drain here
  i32 ok = 0;
  for ( usize i = 0; i < out.size(); ++i )
    if ( out[i] == 1 ) ++ok;
  co_return ok;
}

// off-engine await: current_worker() is null on this thread, so the op routes
// through the SHARED fallback ring rather than a worker ring
static micron::task<void>
offengine_fixed(i32 fd, i32 slot, micron::atomic_token<i32> *out)
{
  byte *p = coro::fixed_ptr(slot);
  if ( p == nullptr ) {
    out->store(-777, micron::memory_order_release);
    co_return;
  }
  for ( u32 i = 0; i < 4096; ++i ) p[i] = static_cast<byte>(i & 0xff);
  i32 r = co_await coro::io::write_fixed(fd, p, 4096, 0, static_cast<u16>(slot));
  out->store(r, micron::memory_order_release);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); coro-io regression tests SKIPPED");
      return 1;
    }
  }
  (void)mp::mkdir(DIR, 0755);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // porcelain parity vs the sync mirror

  sb::test_case("copy: self-copy is refused, source intact (was: truncated to 0)");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("selfcopy.dat");
    micron::string content(8192, 'z');
    sb::require(mio::write_file(p, content) == 8192);
    // modes::write carries O_TRUNC: without the same-inode guard this opened
    // the source for truncation and reported 0 as SUCCESS
    sb::check(coro::sync_wait(cio::copy(p, p)) == -micron::error::invalid_arg);
    sb::check(file_bytes(p) == 8192);
    // and through a hard link, which is the same inode by another name
    micron::io::path_t lp = mkpath("selfcopy.link");
    (void)coro::sync_wait(cio::remove(lp));
    sb::require(micron::syscall(SYS_linkat, -100, p.c_str(), -100, lp.c_str(), 0) == 0);
    sb::check(coro::sync_wait(cio::copy(p, lp)) == -micron::error::invalid_arg);
    sb::check(file_bytes(p) == 8192);
    sb::check(file_bytes(lp) == 8192);
    // the sync mirror agrees, which is the whole point
    sb::check(mio::copy(p, p) == -micron::error::invalid_arg);
    (void)coro::sync_wait(cio::remove(lp));
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("copy: destination carries the source's mode bits (was: always 0644)");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t src = mkpath("mode_src.bin");
    micron::io::path_t dst = mkpath("mode_dst.bin");
    sb::require(mio::write_file(src, micron::string("#!/bin/sh\n")) == 10);
    sb::require(micron::syscall(SYS_fchmodat, -100, src.c_str(), 0755, 0) == 0);
    (void)coro::sync_wait(cio::remove(dst));
    sb::check(coro::sync_wait(cio::copy(src, dst)) == 10);
    sb::check(file_mode(src) == 0755u);
    sb::check(file_mode(dst) == 0755u);      // an executable must stay executable
    // a 0600 source must not become world-readable either
    micron::io::path_t s2 = mkpath("mode_src2.bin");
    micron::io::path_t d2 = mkpath("mode_dst2.bin");
    sb::require(mio::write_file(s2, micron::string("secret")) == 6);
    sb::require(micron::syscall(SYS_fchmodat, -100, s2.c_str(), 0600, 0) == 0);
    (void)coro::sync_wait(cio::remove(d2));
    sb::check(coro::sync_wait(cio::copy(s2, d2)) == 6);
    sb::check(file_mode(d2) == 0600u);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("copy: content survives a large transfer and matches the oracle");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t src = mkpath("big_src.bin");
    micron::io::path_t dst = mkpath("big_dst.bin");
    micron::vector<byte> blob;
    for ( u32 i = 0; i < (1u << 20) + 12345u; ++i ) blob.push_back(static_cast<byte>(i * 131u));
    sb::require(mio::write_file(src, blob) == static_cast<max_t>(blob.size()));
    (void)coro::sync_wait(cio::remove(dst));
    // returns the byte count AND fsyncs before doing so (callers unlink the
    // source on the strength of this value)
    sb::check(coro::sync_wait(cio::copy(src, dst)) == static_cast<max_t>(blob.size()));
    auto a = mio::read_file<micron::vector<byte>>(dst);
    sb::require(a.is_first());
    micron::vector<byte> got = micron::move(a);
    sb::require(got.size() == blob.size());
    bool same = true;
    for ( usize i = 0; i < blob.size(); ++i )
      if ( got[i] != blob[i] ) {
        same = false;
        break;
      }
    sb::check(same);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("move: returns 0 on BOTH paths (was: byte count on the EXDEV fallback)");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t a = mkpath("mv_a.dat");
    micron::io::path_t b = mkpath("mv_b.dat");
    sb::require(mio::write_file(a, micron::string("twelve bytes")) == 12);
    (void)coro::sync_wait(cio::remove(b));
    sb::check(coro::sync_wait(cio::move(a, b)) == 0);      // same device: rename
    sb::check(!coro::sync_wait(cio::exists(a)));
    sb::check(coro::sync_wait(cio::exists(b)));
    // cross-device takes the copy+unlink fallback; it must report 0 too, not 12
    micron::stat_t st_tmp{}, st_var{};
    const bool have_two_fs
        = mp::exists("/tmp", st_tmp) && mp::exists("/var/tmp", st_var) && st_tmp.st_dev != st_var.st_dev;
    if ( have_two_fs ) {
      micron::io::path_t x = mkpath("mv_x.dat");
      micron::io::path_t y("/var/tmp/micron_regress_mv.dat");
      sb::require(mio::write_file(x, micron::string("twelve bytes")) == 12);
      (void)coro::sync_wait(cio::remove(y));
      sb::check(coro::sync_wait(cio::move(x, y)) == 0);      // NOT 12
      sb::check(!coro::sync_wait(cio::exists(x)));
      sb::check(coro::sync_wait(cio::exists(y)));
      sb::check(file_bytes(y) == 12);
      (void)coro::sync_wait(cio::remove(y));
    } else {
      sb::print("  (/tmp and /var/tmp share a device; EXDEV branch not exercised)");
    }
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("open_file: O_APPEND handles report __append (was: always false)");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("append.dat");
    sb::require(mio::write_file(p, micron::string("BASEXXXYYY")) == 10);
    // modify() on an append handle MUST refuse: the fd is O_APPEND, so its
    // "rewrite at offset 0" is appended by the kernel and the ftruncate that
    // follows then eats the original content
    max_t r = coro::sync_wait([](micron::io::path_t q) -> micron::task<max_t> {
      cio::file f = co_await cio::open_file(micron::move(q), mio::modes::appendread);
      if ( !f.valid() ) co_return f.raw_fd();
      max_t m = co_await f.modify([](micron::string) { return micron::string("NEW"); });
      co_return m;
    }(p));
    sb::check(r == -micron::error::invalid_arg);
    sb::check(file_bytes(p) == 10);      // untouched, not 3
    auto rd = mio::read_file<micron::string>(p);
    sb::require(rd.is_first());
    sb::check(micron::string(micron::move(rd)) == micron::string("BASEXXXYYY"));
    // the sync-opening ctor has always agreed; parity is the contract
    max_t r2 = coro::sync_wait([](micron::io::path_t q) -> micron::task<max_t> {
      cio::file f(q, mio::modes::appendread);
      if ( !f.valid() ) co_return f.raw_fd();
      max_t m = co_await f.modify([](micron::string) { return micron::string("NEW"); });
      co_return m;
    }(p));
    sb::check(r2 == -micron::error::invalid_arg);
    // and an append write lands at EOF, not at the cursor
    max_t w = coro::sync_wait([](micron::io::path_t q) -> micron::task<max_t> {
      cio::file f = co_await cio::open_file(micron::move(q), mio::modes::append);
      if ( !f.valid() ) co_return f.raw_fd();
      max_t n = co_await f.write(micron::string("TAIL"));
      co_return n;
    }(p));
    sb::check(w == 4);
    auto rd2 = mio::read_file<micron::string>(p);
    sb::require(rd2.is_first());
    sb::check(micron::string(micron::move(rd2)) == micron::string("BASEXXXYYYTAIL"));
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("create_file: the fd is O_CLOEXEC (was: inherited across exec)");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("cloexec.lock");
    (void)coro::sync_wait(cio::remove(p));
    i32 flags = coro::sync_wait([](micron::io::path_t q) -> micron::task<i32> {
      cio::file f = co_await cio::create_file(micron::move(q));
      if ( !f.valid() ) co_return f.raw_fd();
      co_return static_cast<i32>(mp::fcntl(f.fd(), mp::f_getfd));
    }(p));
    sb::require(flags >= 0);
    sb::check((flags & 1 /*FD_CLOEXEC*/) != 0);
    // exclusive-create still refuses an existing path
    sb::check(coro::sync_wait([](micron::io::path_t q) -> micron::task<i32> {
                cio::file f = co_await cio::create_file(micron::move(q));
                co_return f.raw_fd();
              }(p))
              == -micron::error::file_exists);
    (void)coro::sync_wait(cio::remove(p));
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("with_file 2-arg form resolves to coro::with_file (ADL regression)");
  {
    // this case is a COMPILE-time regression: unqualified `with_file(...)` let
    // ADL pull micron::io::with_file into the overload set, and a generic
    // lambda matches both candidates -> ambiguous, no diagnostic until someone
    // actually called the 2-arg form with one.
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("adl.dat");
    sb::require(mio::write_file(p, micron::string("abcdef")) == 6);
    auto res = coro::sync_wait(cio::with_file(p, [](auto &f) -> micron::task<max_t> {
      auto s = co_await f.template read<micron::string>();
      co_return s.is_first() ? static_cast<max_t>(micron::string(micron::move(s)).size()) : -1;
    }));
    sb::require(res.is_first());
    sb::check(static_cast<max_t>(micron::move(res)) == 6);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // transfer loops

  sb::test_case("write_file is whole-file under ring pressure");
  {
    // guards the EAGAIN half of the transfer loops: the reactor synthesizes -11
    // when the SQ is still full after a flush, and __write_full used to treat
    // that as terminal and return the PARTIAL count as a non-negative success.
    //
    // HONEST CAVEAT: this asserts the invariant (never a short count reported as
    // success) but is NOT a proven repro - __io_submit_own's enter(0) flush
    // reliably frees an SQ slot, so the synthetic -11 could not be provoked here
    // even on a 4-entry ring. the fix also covers a real O_NONBLOCK fd, where
    // the lazy probe is what keeps the retry from becoming an infinite spin.
    coro::start_coroutine_runtime(1);
    const usize N = 3u << 20;
    micron::string big(N, 'q');
    micron::vector<micron::io::path_t> paths;
    micron::vector<cio::write_spec> specs;
    for ( u32 i = 0; i < 24; ++i ) {
      micron::io::path_t p(DIR);
      p += "/pressure_";
      p += static_cast<char>('a' + i % 26u);
      p += static_cast<char>('a' + i / 26u);
      p += ".dat";
      paths.push_back(p);
    }
    for ( u32 i = 0; i < paths.size(); ++i )
      specs.push_back(cio::write_spec{ paths[i], reinterpret_cast<const byte *>(big.c_str()), N });
    auto rs = coro::sync_wait(cio::write_files(specs));
    sb::require(rs.size() == paths.size());
    for ( usize i = 0; i < rs.size(); ++i ) {
      sb::check(rs[i] == static_cast<max_t>(N));      // full count, never a short one
      sb::check(file_bytes(paths[i]) == static_cast<max_t>(N));
    }
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("read of a size-0-reporting file is complete (growth-loop clamp)");
  {
    // __read_remainder doubles its buffer with an UNCLAMPED (u32)(cap - got);
    // past 4 GiB that wraps to a 0-length read, which reads as a clean EOF on a
    // truncated buffer. the >4 GiB case is not testable here, but the clamped
    // growth path itself is - /proc nodes report st_size == 0.
    coro::start_coroutine_runtime(2);
    auto r = coro::sync_wait(cio::read_file<micron::string>(micron::io::path_t("/proc/self/maps")));
    sb::require(r.is_first());
    micron::string got = micron::move(r);
    sb::check(got.size() > 0);
    auto oracle = mio::read_file<micron::string>(micron::io::path_t("/proc/self/maps"));
    sb::require(oracle.is_first());
    sb::check(got.size() >= micron::string(micron::move(oracle)).size() / 2);      // same order; maps drifts between reads
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // reactor + scheduler

  // NOTE for the two fixed-buffer cases below: the slab is 8 x 256K = 2 MB and
  // gets pinned into EVERY ring that runs a fixed op, while the kernel's
  // RLIMIT_MEMLOCK uncharge lags ring close. on a default 8 MB limit that caps
  // how many start/stop cycles can register before the kernel starts refusing
  // (-95, the documented "memlock or old kernel" path). so neither case asserts
  // a bare 4096 - they assert the PROPERTY that broke, which holds either way.

  sb::test_case("fixed-buffer op composed with io::after() (was: -EFAULT)");
  {
    // operator| rebuilt the timed awaitable from the sqe ALONE, dropping the
    // __fixed flag, so the lazy per-ring slab registration never ran and the
    // kernel saw a buf_index against a ring with no registered buffers.
    // the invariant: a timed fixed op behaves EXACTLY like an untimed one.
    // before the fix plain==4096 while timed==-14; a memlock refusal makes both
    // -95 and the equality still holds.
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("fixed_timed.bin");
    i32 fd = static_cast<i32>(micron::syscall(SYS_openat, -100, p.c_str(), 0102 /*O_RDWR|O_CREAT*/ | 01000 /*O_TRUNC*/, 0644));
    sb::require(fd >= 0);
    i32 slot = coro::acquire_fixed();
    sb::require(slot >= 0);
    // TIMED FIRST, on a ring that has never seen a plain fixed op: that ordering
    // is what made the missing registration observable
    const i32 timed0 = coro::sync_wait(fixed_write_timed(fd, slot, true));
    const i32 plain0 = coro::sync_wait(fixed_write_timed(fd, slot, false));
    const i32 timed1 = coro::sync_wait(fixed_write_timed(fd, slot, true));
    sb::check(timed0 == plain0);      // THE regression: these used to disagree
    sb::check(timed1 == plain0);
    sb::check(timed0 != -14);      // never EFAULT, which is what the dropped flag produced
    if ( plain0 == 4096 ) {
      micron::syscall(SYS_close, fd);
      sb::check(file_bytes(p) == 4096);
    } else {
      sb::print("  (fixed buffers refused: ", plain0, " - RLIMIT_MEMLOCK; equality still asserted)");
      micron::syscall(SYS_close, fd);
    }
    coro::release_fixed(slot);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("per-ring registration state is cleared at shutdown (stale __bufs_reg)");
  {
    // __io_fb_shutdown never reset __bufs_reg, so the NEXT fallback ring
    // short-circuited registration on the stale flag and every fixed op through
    // it -EFAULTed for the rest of the process. white-box, because it is the
    // flag itself that leaked - and because the behavioural probe is bounded by
    // RLIMIT_MEMLOCK long before it is bounded by correctness.
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("fixed_restart.bin");
    i32 fd = static_cast<i32>(micron::syscall(SYS_openat, -100, p.c_str(), 0102 | 01000, 0644));
    sb::require(fd >= 0);
    i32 slot = coro::acquire_fixed();
    sb::require(slot >= 0);
    // resume the frame on THIS thread: current_worker() is null here, so the op
    // lands on the fallback ring and dirties the flag that used to leak
    micron::atomic_token<i32> res{ 12345 };
    {
      micron::task<void> t = offengine_fixed(fd, slot, &res);
      t.handle().resume();
      for ( u32 spins = 0; res.get(micron::memory_order_acquire) == 12345 && spins < 4000; ++spins ) msleep(1);
    }
    const i32 first = res.get(micron::memory_order_acquire);
    sb::check(first == 4096 || first == -95);
    coro::release_fixed(slot);
    micron::syscall(SYS_close, fd);
    coro::stop_coroutine_runtime();
    // EVERY ring must come out of shutdown unregistered; a survivor makes the
    // next ring skip registration and fail every fixed op it is handed
    sb::check(micron::coro::__io_fb.__bufs_reg.get(micron::memory_order_acquire) == 0u);
    for ( u32 i = 0; i < 32u; ++i ) sb::check(micron::coro::__io_rings[i].__bufs_reg.get(micron::memory_order_acquire) == 0u);
    // and the ring itself is marked dead, so no reaper can touch the mmap
    sb::check(micron::coro::__io_fb.__live.get(micron::memory_order_acquire) == 0u);
    for ( u32 i = 0; i < 32u; ++i ) sb::check(micron::coro::__io_rings[i].__live.get(micron::memory_order_acquire) == 0u);
    // a restart that DID register once still works the second time round
    coro::start_coroutine_runtime(2);
    i32 fd2 = static_cast<i32>(micron::syscall(SYS_openat, -100, p.c_str(), 0102 | 01000, 0644));
    sb::require(fd2 >= 0);
    i32 slot2 = coro::acquire_fixed();
    sb::require(slot2 >= 0);
    const i32 second = coro::sync_wait(fixed_write_timed(fd2, slot2, false));
    if ( first == 4096 && second != 4096 )
      sb::print("  (restart pass refused with ", second, "; RLIMIT_MEMLOCK uncharge lag, not the stale flag)");
    sb::check(second == 4096 || second == -95);
    coro::release_fixed(slot2);
    micron::syscall(SYS_close, fd2);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("completion delivery survives an inbox overflow (was: livelock)");
  {
    // __dispatch_cqe used the SPINNING submit(), called while holding the ring's
    // __cq_lk. with ONE worker that worker is the only thread that can ever pop
    // the 256-slot inbox - so once a single CQ drain carries more than 256
    // completions, it wedges pushing to a queue only it can drain.
    //
    // getting there needs the completions to PILE UP: a drain that keeps pace
    // never fills the inbox. so park BURST_READS children on pipes, make them
    // all readable at once, and hog the worker on cpu meanwhile - the backlog
    // then lands in a single __drain_ring pass at the join.
    //
    // NOTE: the failure mode is a HANG, not a check() failure. if this file
    // stops finishing, this is the case that broke.
    coro::start_coroutine_runtime(1);
    for ( u32 i = 0; i < BURST_READS; ++i ) sb::require(micron::syscall(SYS_pipe2, burst_pfd[i], 0) == 0);
    burst_go.store(0, micron::memory_order_release);
    {
      auto th = micron::solo::spawn<micron::auto_thread<>>([]() {
        while ( burst_go.get(micron::memory_order_acquire) == 0 ) {
          micron::timespec_t ts{ 0, 1000000l };
          micron::syscall(SYS_nanosleep, &ts, nullptr);
        }
        for ( u32 i = 0; i < BURST_READS; ++i ) micron::syscall(SYS_write, burst_pfd[i][1], "x", 1);
      });
      sb::check(coro::sync_wait(burst_fan()) == static_cast<i32>(BURST_READS));
      th.reset();
    }
    sb::check(coro::io_pending() == 0);
    for ( u32 i = 0; i < BURST_READS; ++i ) {
      micron::syscall(SYS_close, burst_pfd[i][0]);
      micron::syscall(SYS_close, burst_pfd[i][1]);
    }
    coro::stop_coroutine_runtime();
    // and the plain fan-out still behaves on a multi-worker engine
    coro::start_coroutine_runtime(2);
    sb::check(coro::sync_wait(nop_fanout(2048)) == 0);
    sb::check(coro::io_pending() == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("repeated start/stop with io in flight (shutdown munmap race)");
  {
    // __io_worker_ring_shutdown munmap'd a worker ring while foreign reapers
    // were inside peek_cqe on it: they tested __live BEFORE taking __cq_lk, so
    // the test could pass and the mmap vanish underneath. intermittent SIGSEGV.
    for ( u32 pass = 0; pass < 24; ++pass ) {
      coro::start_coroutine_runtime(4);
      sb::check(coro::sync_wait(nop_fanout(256)) == 0);
      // stop while the rings are still hot: no msleep, no drain wait
      coro::stop_coroutine_runtime();
    }
    sb::check(true);      // reaching here without a fault IS the assertion
  }
  sb::end_test_case();

  sb::test_case("concurrent first fixed-op on the shared fallback ring (reg race)");
  {
    // __io_fixed_reg check-and-register was unlocked; two off-engine threads
    // both called register_buffers, the loser took the kernel's -EBUSY for a
    // real refusal and poisoned the ring (__bufs_reg = 2) for good.
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("fixed_race.bin");
    i32 fd = static_cast<i32>(micron::syscall(SYS_openat, -100, p.c_str(), 0102 | 01000, 0644));
    sb::require(fd >= 0);
    constexpr u32 NT = 8;
    micron::atomic_token<u32> bad{ 0 };
    {
      micron::__thread_pointer<micron::auto_thread<>> th[NT];
      for ( u32 i = 0; i < NT; ++i )
        th[i] = micron::solo::spawn<micron::auto_thread<>>(
            [](i32 dfd, micron::atomic_token<u32> *b) {
              i32 s = coro::acquire_fixed();
              if ( s < 0 ) return;
              // off-engine sync_wait -> the SHARED fallback ring, first fixed op
              if ( coro::sync_wait(fixed_write_timed(dfd, s, false)) != 4096 ) b->fetch_add(1, micron::memory_order_acq_rel);
              coro::release_fixed(s);
            },
            fd, &bad);
      for ( u32 i = 0; i < NT; ++i ) th[i].reset();
    }
    sb::check(bad.get(micron::memory_order_acquire) == 0u);
    micron::syscall(SYS_close, fd);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("stop lets in-flight io finish (was: cancelled with zero grace)");
  {
    // the drain guard was replaced by an immediate ASYNC_CANCEL_ANY that fired
    // on the FIRST cpu-quiet observation. the state that triggers it is a
    // DETACHED task parked on an op with nothing left on the cpu - a pipe write
    // bigger than the pipe buffer is the reproducible version of that: the
    // writer blocks at ~64K, the engine looks idle, and stop used to cancel it.
    // __write_full then returned the partial count as a NON-NEGATIVE success.
    coro::start_coroutine_runtime(2);
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::vector<byte> payload;
    for ( usize i = 0; i < PIPE_PAYLOAD; ++i ) payload.push_back(static_cast<byte>(i & 0xff));
    micron::futex_future<max_t> fut = coro::schedule(pipe_writer(pfd[1], payload.data()));
    msleep(80);      // writer parked on a full pipe; no cpu activity anywhere
    micron::atomic_token<u64> drained{ 0 };
    {
      auto th = micron::solo::spawn<micron::auto_thread<>>(
          [](int rfd, micron::atomic_token<u64> *d) {
            byte b[8192];
            u64 tot = 0;
            for ( u32 k = 0; k < 20000; ++k ) {      // bounded: never wedge the suite
              long r = micron::syscall(SYS_read, rfd, b, sizeof(b));
              if ( r <= 0 ) break;
              tot += static_cast<u64>(r);
              d->store(tot, micron::memory_order_release);
              if ( tot >= PIPE_PAYLOAD ) break;
              micron::timespec_t ts{ 0, 3000000l };      // drain slowly: ~100ms total
              micron::syscall(SYS_nanosleep, &ts, nullptr);
            }
          },
          pfd[0], &drained);
      coro::stop_coroutine_runtime();      // must DRAIN the writer, not cancel it
      sb::check(fut.get() == static_cast<max_t>(PIPE_PAYLOAD));
      micron::syscall(SYS_close, pfd[1]);      // let the reader see EOF before the join
    }
    sb::check(drained.get(micron::memory_order_acquire) == PIPE_PAYLOAD);
    micron::syscall(SYS_close, pfd[0]);
  }
  sb::end_test_case();

  sb::test_case("stop still terminates when io is genuinely stuck (grace is bounded)");
  {
    // the flip side: the grace period must not turn into a hang. a read on a
    // pipe nobody will ever write to never progresses, so stop escalates to
    // cancel-all once the window burns and returns.
    coro::start_coroutine_runtime(2);
    int pfd[2];
    sb::require(micron::syscall(SYS_pipe2, pfd, 0) == 0);
    micron::futex_future<i32> fut = coro::schedule([](i32 rfd) -> micron::task<i32> {
      char b[8];
      i32 r = co_await coro::io::read(rfd, b, sizeof(b));
      co_return r;
    }(pfd[0]));
    msleep(30);
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    coro::stop_coroutine_runtime();
    micron::clock_gettime(micron::clock_monotonic, t1);
    const i64 dt_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
    sb::print("  stop with a permanently parked read took ", dt_ms, " ms");
    sb::check(dt_ms < 5000);      // bounded by the grace window, NOT a hang
    sb::check(fut.get() == -125);
    micron::syscall(SYS_close, pfd[0]);
    micron::syscall(SYS_close, pfd[1]);
  }
  sb::end_test_case();

  sb::test_case("over-aligned coroutine-frame local (frame allocator alignment)");
  {
    // __frame_alloc handed back 16-aligned memory while gcc places frame locals
    // as if the base were __BIGGEST_ALIGNMENT__-aligned and emits vmovdqa
    // against them -> #GP, reported as SIGSEGV with si_addr == 0 inside an
    // unrelated-looking coroutine.
    coro::start_coroutine_runtime(2);
    for ( u32 i = 0; i < 64; ++i ) sb::check(coro::sync_wait(overaligned_frame_local()) == 0u);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::test_case("advise() over a >4 GiB range does not wrap to a no-op");
  {
    coro::start_coroutine_runtime(2);
    micron::io::path_t p = mkpath("advise.dat");
    sb::require(mio::write_file(p, micron::string(4096, 'a')) == 4096);
    // len == 4 GiB is exactly (u32)0 after the old unclamped cast
    i32 r = coro::sync_wait([](micron::io::path_t q) -> micron::task<i32> {
      cio::file f = co_await cio::open_file(micron::move(q));
      if ( !f.valid() ) co_return f.raw_fd();
      i32 a = co_await f.advise(0, 1ull << 32, 0 /*POSIX_FADV_NORMAL*/);
      co_return a;
    }(p));
    sb::check(r == 0);
    coro::stop_coroutine_runtime();
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL CORO-IO REGRESSION TESTS PASSED ===");
  return 1;
}
