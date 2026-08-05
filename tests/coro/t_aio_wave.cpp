//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace posix = micron::posix;

// WARNING: sb::check() only prints; it cannot fail a run on its own. Without this counter and the
// require(FAILS == 0) at the end, a byte-for-byte mismatch would print and main would still hit
// `return 1` -- the PASS sentinel. Same idiom as t_uring_io.cpp
static int FAILS = 0;

static constexpr const char *DIR = "/var/tmp/micron_wave_t";

static void
mkdir_p(const char *p)
{
  micron::syscall(SYS_mkdirat, -100, p, 0755);
}

static void
path_of(char *out, const char *name)
{
  usize k = 0;
  for ( const char *p = DIR; *p; ++p ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; ++p ) out[k++] = *p;
  out[k] = '\0';
}

static bool
mkfile(const char *name, const micron::string &body)
{
  char full[512];
  path_of(full, name);
  const long fd = micron::syscall(SYS_openat, -100, full, posix::o_create | posix::o_rdwr | posix::o_trunc, 0644);
  if ( fd < 0 ) return false;
  usize off = 0;
  while ( off < body.size() ) {
    const long w = micron::syscall(SYS_write, fd, body.c_str() + off, body.size() - off);
    if ( w <= 0 ) break;
    off += static_cast<usize>(w);
  }
  micron::syscall(SYS_close, fd);
  return off == body.size();
}

static micron::string
oracle(const char *name)
{
  char full[512];
  path_of(full, name);
  micron::string out;
  const long fd = micron::syscall(SYS_openat, -100, full, posix::o_rdonly, 0);
  if ( fd < 0 ) return out;
  char buf[8192];
  for ( ;; ) {
    const long r = micron::syscall(SYS_read, fd, buf, sizeof(buf));
    if ( r <= 0 ) break;
    out.append(buf, static_cast<usize>(r));
  }
  micron::syscall(SYS_close, fd);
  return out;
}

static i32
open_dir()
{

  return static_cast<i32>(micron::syscall(SYS_openat, -100, DIR, posix::o_rdonly | posix::o_directory, 0));
}

static micron::string
body_for(u32 i, usize n)
{
  micron::string s;
  s.reserve(n + 1);
  u32 st = 0x9e3779b9u ^ (i * 0x85ebca6bu);
  for ( usize k = 0; k < n; ++k ) {
    st ^= st << 13;
    st ^= st >> 17;
    st ^= st << 5;
    s += static_cast<char>('a' + (st % 26u));
  }
  return s;
}

struct wfile {
  micron::sstr<64> name;
  micron::string body;
};

static micron::task<i32>
wave_matches_oracle(i32 dirfd, const micron::vector<wfile> &sp)
{
  cio::wave w;
  w.begin(dirfd);
  for ( usize i = 0; i < sp.size(); ++i ) {
    if ( w.full() ) break;
    if ( !w.push(sp[i].name.c_str()) ) break;
  }
  const i32 rc = co_await w.run();
  if ( rc != 0 ) co_return -3000;

  i32 verified = 0;
  for ( usize i = 0; i < w.size(); ++i ) {
    const auto &r = w[i];
    if ( r.unstaged ) continue;
    if ( r.err != 0 ) co_return -3002;
    if ( r.partial ) co_return -3003;
    if ( r.len != sp[i].body.size() ) co_return -3004;
    for ( usize k = 0; k < r.len; ++k )
      if ( static_cast<char>(r.data[k]) != sp[i].body[k] ) co_return -3005;
    ++verified;
  }

  if constexpr ( cio::wave_items * cio::wave_sqes + micron::coro::__io_sq_reserve <= MICRON_CORO_URING_ENTRIES ) {
    if ( static_cast<usize>(verified) != w.size() ) co_return -3001;
  }
  co_return verified;
}

static micron::task<i32>
wave_reports_open_failure(i32 dirfd, const char *good)
{
  cio::wave w;
  w.begin(dirfd);
  (void)w.push("definitely_not_here_xyzzy");
  (void)w.push(good);
  const i32 rc = co_await w.run();
  if ( rc != 0 ) co_return -3100;
  if ( w[0].err >= 0 ) co_return -3101;
  if ( w[1].err != 0 || w[1].data == nullptr ) co_return -3102;
  co_return 0;
}

static micron::task<i32>
wave_flags_oversize(i32 dirfd, const char *big)
{
  cio::wave w;
  w.begin(dirfd);
  (void)w.push(big);
  const i32 rc = co_await w.run();
  if ( rc != 0 ) co_return -3200;
  if ( w[0].err != 0 ) co_return -3201;
  if ( !w[0].partial ) co_return -3202;
  if ( w[0].len != cio::wave_cap ) co_return -3203;
  co_return 0;
}

static micron::task<i32>
wave_reuses(i32 dirfd, const micron::vector<wfile> &sp)
{
  cio::wave w;
  usize base = 0;
  usize checked = 0;
  while ( base < sp.size() ) {
    w.begin(dirfd);
    while ( base < sp.size() && !w.full() ) {
      if ( !w.push(sp[base].name.c_str()) ) break;
      ++base;
    }
    const i32 rc = co_await w.run();
    if ( rc != 0 ) co_return -3300;
    for ( usize i = 0; i < w.size(); ++i ) {
      if ( w[i].unstaged ) continue;
      if ( w[i].err != 0 ) co_return -3301;
      ++checked;
    }
    w.clear();
  }
  if ( checked == 0 ) co_return -3302;
  co_return static_cast<i32>(checked);
}

static micron::task<i32>
wave_recycles_slots(i32 dirfd, const micron::vector<wfile> &sp)
{
  cio::wave w;
  const u32 passes = (micron::coro::__io_file_slots / cio::wave_items) * 3u + 5u;
  i32 ok = 0;
  for ( u32 p = 0; p < passes; ++p ) {
    w.begin(dirfd);

    (void)w.push("definitely_not_here_xyzzy");
    (void)w.push("big.dat");
    for ( usize i = 0; i < sp.size() && !w.full(); ++i ) (void)w.push(sp[i].name.c_str());

    const i32 rc = co_await w.run();
    if ( rc != 0 ) co_return -3400;
    for ( usize i = 0; i < w.size(); ++i ) {
      const auto &r = w[i];
      if ( r.unstaged ) continue;
      if ( i == 0 ) {
        if ( r.err >= 0 ) co_return -3401;
        continue;
      }

      if ( r.err != 0 ) co_return -3402 - (static_cast<i32>(p) << 8);
      ++ok;
    }
    w.clear();
  }
  co_return ok;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO IO WAVE ===");

  mkdir_p(DIR);

  micron::vector<wfile> sp;
  for ( u32 i = 0; i < 40; ++i ) {
    wfile s;
    micron::sstr<64> n("f");
    n += static_cast<char>('a' + i % 26u);
    n += static_cast<char>('a' + i / 26u);
    n += ".dat";
    s.name = n;
    s.body = body_for(i, 64u + (i * 137u) % 4096u);
    sb::require(mkfile(s.name.c_str(), s.body));
    sp.push_back(micron::move(s));
  }

  {
    micron::string big = body_for(0xbeef, cio::wave_cap + 4096u);
    sb::require(mkfile("big.dat", big));
  }

  sb::require(oracle(sp[0].name.c_str()) == sp[0].body);

  const i32 dirfd = open_dir();
  sb::require(dirfd >= 0);

  bool have_wave = false;
  coro::start_coroutine_runtime(2);
  have_wave = coro::sync_wait([]() -> micron::task<bool> { co_return cio::wave::available(); }());
  coro::stop_coroutine_runtime();

  if ( !have_wave ) {
    sb::print("no sparse fixed-file table (needs >= 5.19) - wave unavailable, skipping");
    micron::syscall(SYS_close, dirfd);
    sb::print("=== CORO IO WAVE SKIPPED ===");
    return 1;
  }

  sb::test_case("a wave of small files matches the per-file oracle byte for byte");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(wave_matches_oracle(dirfd, sp));
    coro::stop_coroutine_runtime();
    sb::check(rc > 0);
    sb::print("oracle: ", rc, " of ", cio::wave_items, " staged and byte-matched");
  }
  sb::end_test_case();

  sb::test_case("a failed open reports its own errno and spares its neighbour");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(wave_reports_open_failure(dirfd, sp[0].name.c_str()));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
  }
  sb::end_test_case();

  sb::test_case("a file larger than the slab is flagged partial, never silently short");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(wave_flags_oversize(dirfd, "big.dat"));
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
  }
  sb::end_test_case();

  sb::test_case("one wave object drives many batches");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(wave_reuses(dirfd, sp));
    coro::stop_coroutine_runtime();
    sb::check(rc > 0);
    sb::print("reuse: ", rc, " files over ", (sp.size() + cio::wave_items - 1) / cio::wave_items, " batches");
  }
  sb::end_test_case();

  sb::test_case("direct-descriptor slots recycle across a wrapping table");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(wave_recycles_slots(dirfd, sp));
    coro::stop_coroutine_runtime();
    sb::check(rc > 0);
    sb::print("slot recycling: ", rc, " successful opens over ", (micron::coro::__io_file_slots / cio::wave_items) * 3u + 5u,
              " batches (table is ", micron::coro::__io_file_slots, " slots)");
  }
  sb::end_test_case();

  micron::syscall(SYS_close, dirfd);
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO IO WAVE TESTS PASSED ===");
  return 1;
}
