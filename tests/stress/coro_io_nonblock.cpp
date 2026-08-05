//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"

using namespace snowball;
namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace posix = micron::posix;

namespace
{

constexpr const char *DIR = "/var/tmp/micron_stress_io";
constexpr i32 c_eagain = -11;
constexpr u64 ROUNDS = 64;
constexpr u64 LINES = 4096;

bool
mkpipe(i32 &rfd, i32 &wfd, bool nonblock)
{
  i32 fds[2] = { -1, -1 };
  if ( micron::syscall(SYS_pipe2, fds, nonblock ? 04000 : 0) < 0 ) return false;
  rfd = fds[0];
  wfd = fds[1];
  return true;
}

void
path_of(char *out, const char *name)
{
  usize k = 0;
  for ( const char *p = DIR; *p; ++p ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; ++p ) out[k++] = *p;
  out[k] = '\0';
}

void
rmfile(const char *name)
{
  char full[512];
  path_of(full, name);
  micron::syscall(SYS_unlinkat, -100, full, 0);
}

micron::string
body_for(u32 tag, usize n)
{
  micron::string s;
  s.reserve(n + 1);
  u32 st = 0x9E3779B9u ^ (tag * 0x85EBCA6Bu) ^ 0x27D4EB2Fu;
  for ( usize k = 0; k < n; ++k ) {
    st ^= st << 13;
    st ^= st >> 17;
    st ^= st << 5;
    s += static_cast<char>('!' + (st % 90u));
  }
  return s;
}

micron::task<i32>
some_roundtrip(const char *name, const micron::string &body)
{
  char full[512];
  path_of(full, name);

  {

    auto file = co_await cio::open_file(micron::io::path_t{ full }, micron::io::modes::write);
    if ( !file.valid() ) co_return -3000;

    usize off = 0;
    u32 guard = 0;
    while ( off < body.size() ) {
      const max_t w = co_await file.write_some(body.c_str() + off, body.size() - off);
      if ( w < 0 ) co_return static_cast<i32>(w);
      if ( w == 0 ) {
        if ( ++guard > 64 ) co_return -3001;
        continue;
      }
      guard = 0;
      off += static_cast<usize>(w);
    }
    if ( off != body.size() ) co_return -3002;
    const max_t sz = co_await file.sync();
    if ( sz < 0 ) co_return -3003;
  }

  {
    auto file = co_await cio::open_file(micron::io::path_t{ full }, micron::io::modes::read);
    if ( !file.valid() ) co_return -3100;

    micron::buffer back(body.size() + 16);
    usize got = 0;
    u32 guard = 0;
    while ( got < body.size() ) {
      const max_t r = co_await file.read_some(back.data() + got, body.size() - got);
      if ( r < 0 ) co_return static_cast<i32>(r);
      if ( r == 0 ) {
        if ( ++guard > 64 ) co_return -3101;
        continue;
      }
      guard = 0;
      got += static_cast<usize>(r);
    }
    if ( got != body.size() ) co_return -3102;
    for ( usize k = 0; k < body.size(); ++k )
      if ( static_cast<char>(back.data()[k]) != body[k] ) co_return -3103;

    const max_t eof = co_await file.read_some(back.data(), 16);
    if ( eof != 0 ) co_return -3104;
  }
  co_return 0;
}

micron::task<i32>
splice_moves(bool nb_in, bool nb_out, usize n)
{
  i32 ar = -1, aw = -1, br = -1, bw = -1;
  if ( !mkpipe(ar, aw, nb_in) ) co_return -3200;
  if ( !mkpipe(br, bw, nb_out) ) {
    micron::syscall(SYS_close, ar);
    micron::syscall(SYS_close, aw);
    co_return -3201;
  }

  const micron::string payload = body_for(static_cast<u32>(n), n);
  i32 rc = 0;
  const long put = micron::syscall(SYS_write, aw, payload.c_str(), n);
  if ( put != static_cast<long>(n) ) {
    rc = -3202;
  } else {
    const max_t moved = co_await cio::splice(ar, bw, n);
    if ( moved != static_cast<max_t>(n) ) {
      rc = -3203;
    } else {
      micron::buffer back(n + 16);
      const long got = micron::syscall(SYS_read, br, back.data(), n);
      if ( got != static_cast<long>(n) ) {
        rc = -3204;
      } else {
        for ( usize k = 0; k < n; ++k )
          if ( static_cast<char>(back.data()[k]) != payload[k] ) {
            rc = -3205;
            break;
          }
      }
    }
  }

  micron::syscall(SYS_close, ar);
  micron::syscall(SYS_close, aw);
  micron::syscall(SYS_close, br);
  micron::syscall(SYS_close, bw);
  co_return rc;
}

micron::task<i32>
tee_duplicates(usize n)
{
  micron::io::upipe from{};
  micron::io::upipe to{};

  const micron::string payload = body_for(0xFEE0u, n);
  const long put = micron::syscall(SYS_write, from.write_fd(), payload.c_str(), n);
  if ( put != static_cast<long>(n) ) co_return -3300;

  const max_t t = co_await cio::tee(from, to, n);
  if ( t != static_cast<max_t>(n) ) co_return -3301;

  micron::buffer a(n + 16), b(n + 16);
  const long ga = micron::syscall(SYS_read, to.read_fd(), a.data(), n);
  const long gb = micron::syscall(SYS_read, from.read_fd(), b.data(), n);
  if ( ga != static_cast<long>(n) || gb != static_cast<long>(n) ) co_return -3302;
  for ( usize k = 0; k < n; ++k )
    if ( a.data()[k] != b.data()[k] || static_cast<char>(a.data()[k]) != payload[k] ) co_return -3303;
  co_return 0;
}

micron::task<i32>
lines_roundtrip(const char *name, u64 nlines)
{
  char full[512];
  path_of(full, name);

  micron::string content;
  content.reserve(nlines * 24);
  for ( u64 i = 0; i < nlines; ++i ) {
    content += "line-";
    micron::string num;
    u64 v = i;
    do {
      num += static_cast<char>('0' + (v % 10u));
      v /= 10u;
    } while ( v );
    for ( usize k = num.size(); k > 0; --k ) content += num[k - 1];
    content += "-payload";
    content += '\n';
  }

  {
    auto w = co_await cio::write_file(micron::io::path_t{ full }, content);
    if ( w < 0 ) co_return -3400;
  }

  auto file = co_await cio::open_file(micron::io::path_t{ full }, micron::io::modes::read);
  if ( !file.valid() ) co_return -3401;

  u64 seen = 0;
  i32 bad = 0;
  const max_t rc = co_await file.each_line([&seen, &bad](const micron::string &ln) {
    micron::string want("line-");
    u64 v = seen;
    micron::string num;
    do {
      num += static_cast<char>('0' + (v % 10u));
      v /= 10u;
    } while ( v );
    for ( usize k = num.size(); k > 0; --k ) want += num[k - 1];
    want += "-payload";
    if ( ln != want ) bad = 1;
    ++seen;
  });
  if ( rc < 0 ) co_return static_cast<i32>(rc);
  if ( bad ) co_return -3402;
  if ( seen != nlines ) co_return -3403;
  if ( rc != static_cast<max_t>(nlines) ) co_return -3404;
  co_return 0;
}

micron::task<i32>
some_contract(bool nonblock)
{
  i32 rfd = -1, wfd = -1;
  if ( !mkpipe(rfd, wfd, nonblock) ) co_return -3500;
  cio::fd_io in{ rfd };
  cio::fd_io out{ wfd };
  i32 rc = 0;

  if ( nonblock ) {

    u32 sink = 0;
    const max_t r = co_await in.read_some(&sink, sizeof(sink));
    if ( r != static_cast<max_t>(c_eagain) ) rc = -3501;
  }

  if ( rc == 0 ) {
    const u64 v = 0x0123456789ABCDEFull;
    const max_t w = co_await out.write_some(&v, sizeof(v));
    if ( w != static_cast<max_t>(sizeof(v)) ) {
      rc = -3502;
    } else {
      u64 back = 0;
      const max_t r = co_await in.read_some(&back, sizeof(back));
      if ( r != static_cast<max_t>(sizeof(back)) )
        rc = -3503;
      else if ( back != v )
        rc = -3504;
    }
  }

  micron::syscall(SYS_close, rfd);
  micron::syscall(SYS_close, wfd);
  co_return rc;
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO IO: ONE-ATTEMPT AND RETRY SURFACE ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  lines: ", static_cast<usize>(LINES),
            "  scale: ", static_cast<usize>(ltest::stress_scale));

  micron::syscall(SYS_mkdirat, -100, DIR, 0755);
  const i32 wm0 = ltest::fd_watermark();

  coro::start_coroutine_runtime(2);
  const bool have_ring = coro::sync_wait([]() -> micron::task<bool> { co_return cio::available(); }());
  coro::stop_coroutine_runtime();

  test_case("*_some contract on pipes: nonblocking says -EAGAIN, both flavours round-trip");
  {
    coro::start_coroutine_runtime(2);
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) {
      const i32 a = coro::sync_wait(some_contract(true));
      if ( a != 0 ) sb::print("     nonblocking round ", static_cast<usize>(i), " rc=", a);
      require(a, 0);
    }
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  if ( !have_ring ) {
    sb::print("no live io_uring ring (qemu-user provides none) - ring-backed cases SKIPPED");
    sb::print("=== CORO IO NONBLOCK PARTIALLY SKIPPED ===");
    return 1;
  }

  test_case("blocking pipe: *_some still round-trips through the ring");
  {
    coro::start_coroutine_runtime(2);
    const u64 n = ltest::scaled(ROUNDS);
    for ( u64 i = 0; i < n; ++i ) require(coro::sync_wait(some_contract(false)), 0);
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("file::write_some / read_some: chunked round-trip, EOF answers 0");
  {
    coro::start_coroutine_runtime(4);
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 i = 0; i < n; ++i ) {
      const usize sz = 1u + static_cast<usize>((i * 2654435761u) % 65536u);
      const micron::string body = body_for(static_cast<u32>(i), sz);
      const i32 rc = coro::sync_wait(some_roundtrip("some_rt.bin", body));
      if ( rc != 0 ) sb::print("     round ", static_cast<usize>(i), " size ", static_cast<usize>(sz), " rc=", rc);
      require(rc, 0);
    }
    coro::stop_coroutine_runtime();
    rmfile("some_rt.bin");
    sb::print("     chunked round-trips=", static_cast<usize>(n));
  }
  end_test_case();

  test_case("splice porcelain: every blocking/nonblocking end combination moves the same bytes");
  {
    coro::start_coroutine_runtime(2);
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 i = 0; i < n; ++i ) {
      const usize sz = 1024u + static_cast<usize>((i * 40503u) % 8192u);
      for ( int c = 0; c < 4; ++c ) {
        const bool nin = (c & 1) != 0;
        const bool nout = (c & 2) != 0;
        const i32 rc = coro::sync_wait(splice_moves(nin, nout, sz));
        if ( rc != 0 ) sb::print("     splice in_nb=", nin ? 1 : 0, " out_nb=", nout ? 1 : 0, " size ", static_cast<usize>(sz), " rc=", rc);
        require(rc, 0);
      }
    }
    coro::stop_coroutine_runtime();
    sb::print("     splice combinations=", static_cast<usize>(n * 4));
  }
  end_test_case();

  test_case("tee porcelain: duplicates without consuming the source");
  {
    coro::start_coroutine_runtime(2);
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 i = 0; i < n; ++i ) {
      const i32 rc = coro::sync_wait(tee_duplicates(2048));
      if ( rc != 0 ) sb::print("     tee round ", static_cast<usize>(i), " rc=", rc);
      require(rc, 0);
    }
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("__aline_cursor: state carried across next() stays correct past many chunks");
  {
    coro::start_coroutine_runtime(4);
    const u64 n = ltest::scaled(4);
    for ( u64 i = 0; i < n; ++i ) {
      const i32 rc = coro::sync_wait(lines_roundtrip("lines.txt", LINES));
      if ( rc != 0 ) sb::print("     lines round ", static_cast<usize>(i), " rc=", rc);
      require(rc, 0);
    }
    coro::stop_coroutine_runtime();
    rmfile("lines.txt");
    sb::print("     line round-trips=", static_cast<usize>(n), " x ", static_cast<usize>(LINES), " lines");
  }
  end_test_case();

  test_case("all of the above concurrently: retry paths compete for sq room");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 4);
    micron::vector<micron::futex_future<i32>> fs;
    for ( u64 i = 0; i < n; ++i ) {
      fs.push_back(coro::schedule(some_contract((i & 1) != 0)));
      fs.push_back(coro::schedule(splice_moves((i & 1) != 0, (i & 2) != 0, 4096)));
      fs.push_back(coro::schedule(tee_duplicates(1024)));
    }
    u64 ok = 0;
    for ( usize i = 0; i < fs.size(); ++i ) {
      const i32 rc = fs[i].get();
      if ( rc != 0 ) sb::print("     concurrent op ", static_cast<usize>(i), " rc=", rc);
      require(rc, 0);
      ++ok;
    }
    require(coro::io_pending(), static_cast<u64>(0));
    coro::stop_coroutine_runtime();
    sb::print("     concurrent ops=", static_cast<usize>(ok));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  sb::print("=== ALL CORO IO NONBLOCK TESTS PASSED ===");
  return 1;
}
