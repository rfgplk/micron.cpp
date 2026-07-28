//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../snowball/snowball.hpp"

// porcelain core: raw io::coro::file surface, cursor semantics, virtual
// files, concurrent positional readers, no-ring error, move semantics

namespace coro = micron::coro;
namespace cio = micron::io::coro;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_acore";

static micron::io::path_t
mkpath(const char *name)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += name;
  return p;
}

static micron::task<i32>
raw_roundtrip(micron::io::path_t p)
{
  cio::file f = co_await cio::open_file(micron::move(p), micron::io::modes::readwritecreate);
  if ( !f.valid() ) co_return f.raw_fd();
  static char blob[10000];
  for ( usize i = 0; i < sizeof(blob); ++i ) blob[i] = static_cast<char>(i * 37u);
  max_t w = co_await f.write(static_cast<const void *>(blob), sizeof(blob));
  if ( w != static_cast<max_t>(sizeof(blob)) ) co_return -1000;
  if ( f.tell() != sizeof(blob) ) co_return -1001;      // cursor advanced
  f.rewind();
  static char back[10000];
  max_t r = co_await f.read(back, sizeof(back));
  if ( r != static_cast<max_t>(sizeof(back)) ) co_return -1002;
  for ( usize i = 0; i < sizeof(blob); ++i )
    if ( back[i] != blob[i] ) co_return -1003;
  // positional does not move the cursor
  const u64 cur = f.tell();
  char four[4];
  max_t p4 = co_await f.read_at(100, four, sizeof(four));
  if ( p4 != 4 || f.tell() != cur ) co_return -1004;
  if ( four[0] != blob[100] ) co_return -1005;
  co_return 0;
}

static micron::task<i32>
proc_virtual(void)
{
  auto r = co_await cio::read_file<micron::string>(micron::io::path_t("/proc/self/status"));
  if ( !r.is_first() ) co_return -1100;
  micron::string s = micron::move(r);
  if ( s.size() == 0 ) co_return -1101;
  // must contain "Pid:"
  bool found = false;
  for ( usize i = 0; i + 4 <= s.size(); ++i )
    if ( s[i] == 'P' && s[i + 1] == 'i' && s[i + 2] == 'd' && s[i + 3] == ':' ) {
      found = true;
      break;
    }
  co_return found ? 0 : -1102;
}

static micron::task<i32>
stripe_check(cio::file *f, u64 off, u32 n)
{
  char buf[512];
  max_t r = co_await f->read_at(off, buf, n);
  if ( r != static_cast<max_t>(n) ) co_return -1200;
  for ( u32 i = 0; i < n; ++i )
    if ( buf[i] != static_cast<char>((off + i) * 37u) ) co_return -1201;
  co_return 0;
}

static micron::task<void>
noring_probe(micron::atomic_token<i32> *out)
{
  char b[4];
  max_t r = co_await cio::__impl::__read_full(0, b, sizeof(b), 0);      // any op: no ring -> -38
  out->store(static_cast<i32>(r), micron::memory_order_release);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); acore tests SKIPPED");
      return 1;
    }
  }

  sb::test_case("no engine, no ring: ops resolve -ENOSYS inline");
  {
    micron::atomic_token<i32> out{ 0 };
    micron::task<void> t = noring_probe(&out);
    t.handle().resume();      // no runtime started: resolves fully inline
    sb::check(out.get(micron::memory_order_acquire) == -38);
    sb::check(!cio::available());
  }
  sb::end_test_case();

  (void)micron::posix::mkdir(DIR, 0755);
  coro::start_coroutine_runtime(4);
  sb::check(cio::available());

  sb::test_case("raw roundtrip: cursor + positional semantics");
  sb::check(coro::sync_wait(raw_roundtrip(mkpath("raw.dat"))) == 0);
  sb::end_test_case();

  sb::test_case("virtual file (/proc) reads grow past fstat size 0");
  sb::check(coro::sync_wait(proc_virtual()) == 0);
  sb::end_test_case();

  sb::test_case("16 concurrent read_at on ONE porcelain file");
  {
    micron::io::path_t p = mkpath("raw.dat");
    cio::file f(p, micron::io::modes::read);
    sb::require(f.valid());
    micron::futex_future<i32> futs[16];
    for ( u32 i = 0; i < 16; ++i ) futs[i] = coro::schedule(stripe_check(&f, i * 512ull, 512));
    for ( u32 i = 0; i < 16; ++i ) sb::check(futs[i].get() == 0);
  }
  sb::end_test_case();

  sb::test_case("move semantics transfer the fd exactly once");
  {
    cio::file a(mkpath("raw.dat"), micron::io::modes::read);
    sb::require(a.valid());
    const i32 fd = a.raw_fd();
    cio::file b = micron::move(a);
    sb::check(!a.valid() && b.valid() && b.raw_fd() == fd);
    cio::file c(mkpath("raw.dat"), micron::io::modes::read);
    c = micron::move(b);
    sb::check(!b.valid() && c.valid() && c.raw_fd() == fd);
  }
  sb::end_test_case();

  sb::test_case("adopted invalid handle carries -errno; ops report it");
  {
    i32 r = coro::sync_wait([]() -> micron::task<i32> {
      cio::file f(micron::fd_t{ -2 /*-ENOENT recorded*/ }, "ghost");
      char b[4];
      max_t x = co_await f.read(b, sizeof(b));
      co_return static_cast<i32>(x);
    }());
    sb::check(r == -2);
  }
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL ACORE TESTS PASSED ===");
  return 1;
}
