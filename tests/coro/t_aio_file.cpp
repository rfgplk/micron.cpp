//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#define MICRON_CORO_URING

#include "../../src/io/coroutine/coro_io.hpp"

#include "../../src/list.hpp"
#include "../snowball/snowball.hpp"

// the central equivalence lever: every marshalling tier and the line policy
// must round-trip BYTE-IDENTICALLY against the sync io::file, both directions

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;
static int FAILS = 0;

static constexpr const char *DIR = "/tmp/micron_aio_file";

static micron::io::path_t
mkpath(const char *name)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += name;
  return p;
}

struct pod_t {
  u64 a;
  u32 b;
  u16 c;
  u8 d[10];
};

static i32
files_identical(const micron::io::path_t &a, const micron::io::path_t &b)
{
  auto ra = mio::read_file<micron::vector<byte>>(a);
  auto rb = mio::read_file<micron::vector<byte>>(b);
  if ( !ra.is_first() || !rb.is_first() ) return -1;
  micron::vector<byte> va = micron::move(ra);
  micron::vector<byte> vb = micron::move(rb);
  if ( va.size() != vb.size() ) return -2;
  for ( usize i = 0; i < va.size(); ++i )
    if ( va[i] != vb[i] ) return -3;
  return 0;
}

// value equality is proven via serialized bytes: async-write == sync-write
// artifact, then async-read-back re-serializes to the same bytes again.
// (avoids needing operator== on containers and unary& on hstrings.)
template<typename T>
static i32
tier_roundtrip(const char *name, const T &value)
{
  micron::io::path_t pa = mkpath(name);
  max_t w = coro::sync_wait([](micron::io::path_t p, const T *v) -> micron::task<max_t> {
    max_t r = co_await cio::write_file(micron::move(p), *v);
    co_return r;
  }(pa, micron::addressof(value)));
  if ( w <= 0 ) return -10;
  micron::io::path_t ps = mkpath("sync_twin.dat");
  {
    mio::file sf = mio::open_file(ps, mio::modes::write);
    if ( !sf.valid() ) return -11;
    if ( sf.write(value) != w ) return -12;
  }
  if ( i32 e = files_identical(pa, ps); e != 0 ) return -20 + e;
  // async read-back through the porcelain tiers, then re-serialize sync
  T back{};
  max_t r = coro::sync_wait([](micron::io::path_t p, T *out) -> micron::task<max_t> {
    cio::file f = co_await cio::open_file(micron::move(p));
    if ( !f.valid() ) co_return f.raw_fd();
    max_t x = co_await f.read(*out);
    co_return x;
  }(ps, micron::addressof(back)));
  if ( r < 0 ) return -30;
  micron::io::path_t pb = mkpath("readback_twin.dat");
  {
    mio::file sf = mio::open_file(pb, mio::modes::write);
    if ( !sf.valid() ) return -31;
    if ( sf.write(back) != w ) return -32;
  }
  if ( i32 e = files_identical(pb, ps); e != 0 ) return -40 + e;
  return 0;
}

static micron::task<i32>
fn_layer(micron::io::path_t p)
{
  cio::file f = co_await cio::open_file(micron::move(p), micron::io::modes::readwritecreate);
  if ( !f.valid() ) co_return f.raw_fd();
  micron::string payload("producer-written");
  max_t w = co_await f.write([&payload]() { return payload; });
  if ( w != static_cast<max_t>(payload.size()) ) co_return -2000;
  f.rewind();
  auto got = co_await f.read([](micron::string s) { return s.size(); });
  if ( !got.is_first() ) co_return -2001;
  usize n = got;
  if ( n != payload.size() ) co_return -2002;
  // modify: uppercase first char, same length (no truncate path)
  f.rewind();
  max_t m = co_await f.modify([](micron::string s) {
    if ( s.size() ) s[0] = 'P';
    return s;
  });
  if ( m != static_cast<max_t>(payload.size()) ) co_return -2003;
  f.rewind();
  auto after = co_await f.read<micron::string>();
  if ( !after.is_first() ) co_return -2004;
  micron::string s2 = micron::move(after);
  if ( s2.size() != payload.size() || s2[0] != 'P' ) co_return -2005;
  co_return 0;
}

static micron::task<i32>
modify_shrink(micron::io::path_t p)
{
  {
    max_t w = co_await cio::write_file(p, micron::string("0123456789"));
    if ( w != 10 ) co_return -2100;
  }
  max_t m = co_await cio::modify(p, [](micron::string s) {
    s.set_size(3);
    return s;
  });
  if ( m == -22 ) co_return -9000;      // ftruncate op unsupported (<6.9): caller skips
  if ( m != 3 ) co_return -2101;
  auto r = co_await cio::read_file<micron::string>(micron::move(p));
  if ( !r.is_first() ) co_return -2102;
  micron::string s = micron::move(r);
  if ( s.size() != 3 || s[0] != '0' || s[2] != '2' ) co_return -2103;
  co_return 0;
}

// crafted line corpus: CRLF, empty lines, long line, final unterminated
static micron::string
line_corpus()
{
  micron::string c;
  c += "first\n";
  c += "\n";
  c += "crlf line\r\n";
  micron::string longline(5000, 'x');
  c += longline;
  c += "\n";
  c += "final-no-newline";
  return c;
}

static micron::task<i32>
lines_equivalence(micron::io::path_t p)
{
  micron::string corpus = line_corpus();
  max_t w = co_await cio::write_file(p, corpus);
  if ( w != static_cast<max_t>(corpus.size()) ) co_return -2200;
  // async collection
  micron::vector<micron::string> async_lines;
  cio::file f = co_await cio::open_file(p);
  if ( !f.valid() ) co_return -2201;
  max_t n = co_await f.each_line([&async_lines](const micron::string &l) { async_lines.push_back(l); });
  if ( n < 0 ) co_return -2202;
  // sync collection through io::file (the oracle)
  micron::vector<micron::string> sync_lines;
  {
    mio::file sf = mio::open_file(p);
    if ( !sf.valid() ) co_return -2203;
    max_t sn = sf.each_line([&sync_lines](const micron::string &l) { sync_lines.push_back(l); });
    if ( sn != n ) co_return -2204;
  }
  if ( async_lines.size() != sync_lines.size() ) co_return -2205;
  for ( usize i = 0; i < async_lines.size(); ++i )
    if ( !(async_lines[i] == sync_lines[i]) ) co_return -2206;
  // fold equivalence: total length
  auto fold = co_await [](micron::io::path_t pp) -> micron::task<micron::option<usize, micron::io::error_t>> {
    cio::file ff = co_await cio::open_file(micron::move(pp));
    auto r = co_await ff.fold_lines(usize{ 0 }, [](usize acc, const micron::string &l) { return acc + l.size(); });
    co_return r;
  }(p);
  if ( !fold.is_first() ) co_return -2207;
  usize total = fold;
  usize expect = 0;
  for ( usize i = 0; i < sync_lines.size(); ++i ) expect += sync_lines[i].size();
  if ( total != expect ) co_return -2208;
  // manual-pull cursor sees the same lines
  micron::vector<micron::string> pulled;
  {
    cio::file pf = co_await cio::open_file(p);
    cio::__aline_cursor cur = pf.lines();
    micron::string ln;
    for ( ;; ) {
      bool more = co_await cur.next(ln);
      if ( !more ) break;
      pulled.push_back(ln);
    }
    if ( cur.error() != 0 ) co_return -2209;
  }
  if ( pulled.size() != sync_lines.size() ) co_return -2210;
  for ( usize i = 0; i < pulled.size(); ++i )
    if ( !(pulled[i] == sync_lines[i]) ) co_return -2211;
  co_return 0;
}

static micron::task<i32>
big_stream(micron::io::path_t p)
{
  constexpr usize SZ = 8u << 20;
  micron::vector<byte> big;
  big.reserve(SZ);
  u64 s = 0x9e3779b97f4a7c15ull;
  for ( usize i = 0; i < SZ; ++i ) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    big.push_back(static_cast<byte>(s));
  }
  max_t w = co_await cio::write_file(p, big);
  if ( w != static_cast<max_t>(SZ) ) co_return -2300;
  auto r = co_await cio::read_file<micron::vector<byte>>(micron::move(p));
  if ( !r.is_first() ) co_return -2301;
  micron::vector<byte> back = micron::move(r);
  if ( back.size() != SZ ) co_return -2302;
  for ( usize i = 0; i < SZ; i += 4097 )
    if ( back[i] != big[i] ) co_return -2303;
  co_return 0;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      sb::print("io_uring unavailable (rc=", rc, "); file porcelain tests SKIPPED");
      return 1;
    }
  }

  (void)micron::posix::mkdir(DIR, 0755);
  coro::start_coroutine_runtime(4);

  sb::test_case("tier (a) string: async<->sync byte-identity");
  {
    micron::string v("the quick brown fox, 0123456789");
    sb::check(tier_roundtrip("tier_a.dat", v) == 0);
  }
  sb::end_test_case();

  sb::test_case("tier (b) vector<u32>: async<->sync byte-identity");
  {
    micron::vector<u32> v;
    for ( u32 i = 0; i < 1000; ++i ) v.push_back(i * 2654435761u);
    sb::check(tier_roundtrip("tier_b.dat", v) == 0);
  }
  sb::end_test_case();

  sb::test_case("tier (c) MFR1 vector<string>: async<->sync byte-identity");
  {
    micron::vector<micron::string> v;
    v.push_back(micron::string("alpha"));
    v.push_back(micron::string(""));
    v.push_back(micron::string("gamma with spaces and \n newline"));
    v.push_back(micron::string(300, 'z'));
    sb::check(tier_roundtrip("tier_c.dat", v) == 0);
  }
  sb::end_test_case();

  sb::test_case("tier (c) MFR1 list<string> node container");
  {
    micron::list<micron::string> v;
    v.push_back(micron::string("one"));
    v.push_back(micron::string("two"));
    v.push_back(micron::string("three"));
    sb::check(tier_roundtrip("tier_c_list.dat", v) == 0);
  }
  sb::end_test_case();

  sb::test_case("tier (d) trivially-copyable object");
  {
    pod_t v{ 0x1122334455667788ull, 0xdeadbeef, 0x1234, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } };
    sb::check(tier_roundtrip("tier_d.dat", v) == 0);
  }
  sb::end_test_case();

  sb::test_case("fn layer: producer write / consumer read / modify");
  sb::check(coro::sync_wait(fn_layer(mkpath("fn.dat"))) == 0);
  sb::end_test_case();

  sb::test_case("modify that shrinks truncates the tail");
  {
    i32 r = coro::sync_wait(modify_shrink(mkpath("shrink.dat")));
    if ( r == -9000 )
      sb::print("ftruncate op unsupported; case SKIPPED");
    else
      sb::check(r == 0);
  }
  sb::end_test_case();

  sb::test_case("line policy equivalence (each_line/fold/cursor vs sync)");
  sb::check(coro::sync_wait(lines_equivalence(mkpath("lines.dat"))) == 0);
  sb::end_test_case();

  sb::test_case("8 MiB streaming roundtrip");
  sb::check(coro::sync_wait(big_stream(mkpath("big.dat"))) == 0);
  sb::end_test_case();

  sb::check(coro::io_pending() == 0);
  coro::stop_coroutine_runtime();

  sb::require(FAILS == 0);
  sb::print("=== ALL FILE PORCELAIN TESTS PASSED ===");
  return 1;
}
