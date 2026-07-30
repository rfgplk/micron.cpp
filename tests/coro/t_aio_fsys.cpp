//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../snowball/snowball.hpp"

// fsys oneshot mirrors + the read_files concurrent fan-out flagship

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_fsys";

static micron::io::path_t
mkpath(const char *stem, u32 i = 0xffffffffu)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += stem;
  if ( i != 0xffffffffu ) {
    char sfx[3] = { static_cast<char>('a' + (i / 26u) % 26u), static_cast<char>('a' + i % 26u), 0 };
    p += sfx;
  }
  return p;
}

static micron::string
pattern_for(u32 i)
{
  micron::string s("file-");
  s += static_cast<char>('A' + (i % 26u));
  s += "-payload-";
  for ( u32 k = 0; k < (i % 7u) + 1u; ++k ) s += static_cast<char>('0' + (i + k) % 10u);
  return s;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); fsys porcelain tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(DIR, 0755);
  coro::start_coroutine_runtime(0);      // default worker count: real fan-out

  sb::test_case("read_file sizes: 0B / 100B / 4K / 1M vs sync oracle");
  {
    const usize sizes[4] = { 0, 100, 4096, 1u << 20 };
    for ( u32 k = 0; k < 4; ++k ) {
      micron::io::path_t p = mkpath("sized", k);
      micron::string content(sizes[k], static_cast<char>('a' + k));
      sb::require(mio::write_file(p, content) == static_cast<max_t>(sizes[k]));
      auto r = coro::sync_wait(cio::read_file<micron::string>(p));
      sb::require(r.is_first());
      micron::string got = micron::move(r);
      sb::check(got.size() == sizes[k]);
      auto oracle = mio::read_file<micron::string>(p);
      sb::require(oracle.is_first());
      micron::string want = micron::move(oracle);
      sb::check(got == want);
    }
  }
  sb::end_test_case();

  sb::test_case("write_file/append_file compose; write_file_sync durable");
  {
    micron::io::path_t p = mkpath("appended.dat");
    sb::check(coro::sync_wait(cio::write_file(p, micron::string("head-"))) == 5);
    sb::check(coro::sync_wait(cio::append_file(p, micron::string("tail"))) == 4);
    auto r = mio::read_file<micron::string>(p);
    sb::require(r.is_first());
    micron::string s = micron::move(r);
    sb::check(s == micron::string("head-tail"));
    sb::check(coro::sync_wait(cio::write_file_sync(mkpath("durable.dat"), micron::string("fsynced"))) == 7);
  }
  sb::end_test_case();

  sb::test_case("copy preserves content; move renames; remove unlinks");
  {
    micron::io::path_t src = mkpath("cp_src.dat");
    micron::io::path_t dst = mkpath("cp_dst.dat");
    micron::vector<byte> blob;
    for ( u32 i = 0; i < (1u << 20); ++i ) blob.push_back(static_cast<byte>(i * 131u));
    sb::require(mio::write_file(src, blob) == static_cast<max_t>(blob.size()));
    sb::check(coro::sync_wait(cio::copy(src, dst)) == static_cast<max_t>(blob.size()));
    auto a = mio::read_file<micron::vector<byte>>(src);
    auto b = mio::read_file<micron::vector<byte>>(dst);
    sb::require(a.is_first() && b.is_first());
    micron::vector<byte> va = micron::move(a), vb = micron::move(b);
    sb::require(va.size() == vb.size());
    bool same = true;
    for ( usize i = 0; i < va.size(); ++i )
      if ( va[i] != vb[i] ) {
        same = false;
        break;
      }
    sb::check(same);
    micron::io::path_t moved = mkpath("cp_moved.dat");
    sb::check(coro::sync_wait(cio::move(dst, moved)) == 0);
    sb::check(!coro::sync_wait(cio::exists(dst)));
    sb::check(coro::sync_wait(cio::exists(moved)));
    sb::check(coro::sync_wait(cio::remove(moved)) == 0);
    sb::check(!coro::sync_wait(cio::exists(moved)));
  }
  sb::end_test_case();

  sb::test_case("mkdir + stat + file_size");
  {
    micron::io::path_t d = mkpath("subdir");
    micron::syscall(SYS_unlinkat, -100, d.c_str(), 0x200 /*AT_REMOVEDIR: clean a prior run*/);
    sb::check(coro::sync_wait(cio::mkdir(d)) == 0);
    auto sx = coro::sync_wait(cio::stat(d));
    sb::require(sx.is_first());
    micron::posix::statx_t st = micron::move(sx);
    sb::check((st.stx_mode & 0170000) == 0040000);
    micron::io::path_t f = mkpath("sized_ck.dat");
    sb::require(mio::write_file(f, micron::string("12345")) == 5);
    sb::check(coro::sync_wait(cio::file_size(f)) == 5);
  }
  sb::end_test_case();

  sb::test_case("error mapping: ENOENT and EISDIR through option<error_t>");
  {
    auto r = coro::sync_wait(cio::read_file<micron::string>(mkpath("missing.dat")));
    sb::check(r.is_second());
    mio::error_t e = micron::move(r);
    sb::check(e.code == 2);      // ENOENT (normalized positive)
    auto d = coro::sync_wait(cio::read_file<micron::string>(micron::io::path_t(DIR)));
    sb::check(d.is_second());
    mio::error_t e2 = micron::move(d);
    sb::check(e2.code == 21);      // EISDIR
  }
  sb::end_test_case();

  sb::test_case("read_files: 64-file fan-out, in-order results, correct contents");
  {
    micron::vector<micron::io::path_t> paths;
    for ( u32 i = 0; i < 64; ++i ) {
      micron::io::path_t p = mkpath("fan", i);
      micron::string content = pattern_for(i);
      sb::require(mio::write_file(p, content) == static_cast<max_t>(content.size()));
      paths.push_back(p);
    }
    auto res = coro::sync_wait(cio::read_files<micron::string>(paths));
    sb::require(res.size() == 64);
    for ( u32 i = 0; i < 64; ++i ) {
      sb::require(res[i].is_first());
      micron::string got = micron::move(res[i]);
      sb::check(got == pattern_for(i));      // in input order, no cross-wiring
    }
  }
  sb::end_test_case();

  sb::test_case("read_files: per-file error isolation (one missing)");
  {
    micron::vector<micron::io::path_t> paths;
    paths.push_back(mkpath("fan", 0));
    paths.push_back(mkpath("never_existed.dat"));
    paths.push_back(mkpath("fan", 2));
    auto res = coro::sync_wait(cio::read_files<micron::string>(paths));
    sb::require(res.size() == 3);
    sb::check(res[0].is_first());
    sb::check(res[1].is_second());
    sb::check(res[2].is_first());
  }
  sb::end_test_case();

  sb::test_case("read_files: 300-file window pressure (crosses the fan window)");
  {
    micron::vector<micron::io::path_t> paths;
    for ( u32 i = 0; i < 300; ++i ) {
      micron::io::path_t p = mkpath("wp", i);
      micron::string content = pattern_for(i * 7u + 3u);
      sb::require(mio::write_file(p, content) == static_cast<max_t>(content.size()));
      paths.push_back(p);
    }
    auto res = coro::sync_wait(cio::read_files<micron::string>(paths));
    sb::require(res.size() == 300);
    u32 bad = 0;
    for ( u32 i = 0; i < 300; ++i ) {
      if ( !res[i].is_first() ) {
        ++bad;
        continue;
      }
      micron::string got = micron::move(res[i]);
      if ( !(got == pattern_for(i * 7u + 3u)) ) ++bad;
    }
    sb::check(bad == 0);
    for ( u32 i = 0; i < 300; ++i ) micron::syscall(SYS_unlinkat, -100, mkpath("wp", i).c_str(), 0);
  }
  sb::end_test_case();

  sb::test_case("write_files + stat_many concurrent");
  {
    static micron::string bodies[8];
    micron::vector<cio::write_spec> specs;
    for ( u32 i = 0; i < 8; ++i ) {
      bodies[i] = pattern_for(i + 40u);
      cio::write_spec ws{};
      ws.path = mkpath("wf", i);
      ws.data = reinterpret_cast<const byte *>(bodies[i].c_str());
      ws.len = bodies[i].size();
      specs.push_back(ws);
    }
    auto ws_res = coro::sync_wait(cio::write_files(specs));
    sb::require(ws_res.size() == 8);
    micron::vector<micron::io::path_t> paths;
    for ( u32 i = 0; i < 8; ++i ) {
      sb::check(ws_res[i] == static_cast<max_t>(bodies[i].size()));
      paths.push_back(mkpath("wf", i));
    }
    auto sx = coro::sync_wait(cio::stat_many(paths));
    sb::require(sx.size() == 8);
    for ( u32 i = 0; i < 8; ++i ) {
      sb::require(sx[i].is_first());
      micron::posix::statx_t st = micron::move(sx[i]);
      sb::check(st.stx_size == bodies[i].size());
    }
  }
  sb::end_test_case();

  sb::test_case("with_file: plain and task-returning callables");
  {
    micron::io::path_t p = mkpath("withfile.dat");
    sb::require(mio::write_file(p, micron::string("bracketed")) == 9);
    auto r1 = coro::sync_wait(cio::with_file(p, [](cio::file &f) { return f.size(); }));
    sb::require(r1.is_first());
    max_t sz = micron::move(r1);
    sb::check(sz == 9);
    auto r2 = coro::sync_wait(cio::with_file(p, [](cio::file &f) -> micron::task<usize> {
      auto s = co_await f.read<micron::string>();
      if ( !s.is_first() ) co_return 0;
      micron::string v = micron::move(s);
      co_return v.size();
    }));
    sb::require(r2.is_first());
    usize n = micron::move(r2);
    sb::check(n == 9);
  }
  sb::end_test_case();

  sb::test_case("read_lines by path matches sync oracle");
  {
    micron::io::path_t p = mkpath("rl.dat");
    sb::require(mio::write_file(p, micron::string("a\nbb\nccc\n")) == 9);
    auto r = coro::sync_wait(cio::read_lines(p));
    sb::require(r.is_first());
    micron::vector<micron::string> lines = micron::move(r);
    sb::require(lines.size() == 3);
    sb::check(lines[0] == micron::string("a") && lines[1] == micron::string("bb") && lines[2] == micron::string("ccc"));
  }
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL FSYS PORCELAIN TESTS PASSED ===");
  return 1;
}
