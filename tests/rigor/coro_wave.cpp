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

static constexpr const char *DIR = "/var/tmp/micron_rigor_wave";
static constexpr u64 SEED = 0xc0ffee1234abcdefull;
static constexpr u32 ROUNDS = 24;

static u64 __st = SEED;

static u64
rnd()
{
  __st ^= __st << 13;
  __st ^= __st >> 7;
  __st ^= __st << 17;
  return __st;
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

static micron::string
body_for(u32 tag, usize n)
{
  micron::string s;
  s.reserve(n + 1);
  u32 st = 0x9e3779b9u ^ (tag * 0x85ebca6bu) ^ 0x27d4eb2fu;
  for ( usize k = 0; k < n; ++k ) {
    st ^= st << 13;
    st ^= st >> 17;
    st ^= st << 5;
    s += static_cast<char>('!' + (st % 90u));
  }
  return s;
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

static void
rmfile(const char *name)
{
  char full[512];
  path_of(full, name);
  micron::syscall(SYS_unlinkat, -100, full, 0);
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

struct item {
  micron::sstr<64> name;
  micron::string body;
  bool exists = true;
};

static micron::task<i32>
round_check(i32 dirfd, const micron::vector<item> &its)
{
  cio::wave w;
  w.begin(dirfd);
  usize pushed = 0;
  for ( usize i = 0; i < its.size() && !w.full(); ++i ) {
    if ( !w.push(its[i].name.c_str()) ) break;
    ++pushed;
  }
  const i32 rc = co_await w.run();
  if ( rc != 0 ) co_return -4000;
  if ( w.size() != pushed ) co_return -4001;

  for ( usize i = 0; i < w.size(); ++i ) {
    const auto &r = w[i];
    const item &it = its[i];

    if ( r.name == nullptr ) co_return -4002;

    if ( r.unstaged ) continue;

    if ( !it.exists ) {
      if ( r.err >= 0 ) co_return -4003;
      continue;
    }
    if ( r.err != 0 ) co_return -4004;

    const bool over = it.body.size() >= cio::wave_cap;
    if ( over != r.partial ) co_return -4005;

    const usize want = over ? cio::wave_cap : it.body.size();
    if ( r.len != want ) co_return -4006;
    for ( usize k = 0; k < want; ++k )
      if ( static_cast<char>(r.data[k]) != it.body[k] ) co_return -4007;
  }
  co_return 0;
}

int
main()
{
  sb::print("=== RIGOR: io::coro::wave vs open/read/close oracle ===");
  micron::syscall(SYS_mkdirat, -100, DIR, 0755);

  const i32 dirfd = static_cast<i32>(micron::syscall(SYS_openat, -100, DIR, posix::o_rdonly | posix::o_directory, 0));
  sb::require(dirfd >= 0);

  bool have_wave = false;
  coro::start_coroutine_runtime(2);
  have_wave = coro::sync_wait([]() -> micron::task<bool> { co_return cio::wave::available(); }());
  coro::stop_coroutine_runtime();
  if ( !have_wave ) {
    sb::print("wave unavailable (needs a sparse fixed-file table, >= 5.19) - nothing to fuzz");
    micron::syscall(SYS_close, dirfd);
    return 1;
  }

  u32 rounds_run = 0;
  u32 files_checked = 0;

  for ( u32 r = 0; r < ROUNDS; ++r ) {
    const u32 n = 1u + static_cast<u32>(rnd() % (cio::wave_items + 4u));
    micron::vector<item> its;
    for ( u32 i = 0; i < n; ++i ) {
      item it;
      micron::sstr<64> nm("r");
      nm += static_cast<char>('a' + r % 26u);
      nm += static_cast<char>('a' + i % 26u);
      nm += static_cast<char>('a' + i / 26u);
      nm += ".bin";
      it.name = nm;

      const u64 roll = rnd() % 100u;
      if ( roll < 10 ) {
        it.exists = false;
        rmfile(it.name.c_str());
      } else {

        usize sz;
        if ( roll < 20 )
          sz = cio::wave_cap + (rnd() % 4096u);
        else if ( roll < 26 )
          sz = cio::wave_cap;
        else if ( roll < 32 )
          sz = cio::wave_cap - 1u;
        else
          sz = static_cast<usize>(rnd() % 8192u);
        it.body = body_for(r * 1000u + i, sz);
        sb::require(mkfile(it.name.c_str(), it.body));

        if ( sz < 4096 ) sb::require(oracle(it.name.c_str()) == it.body);
      }
      its.push_back(micron::move(it));
    }

    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(round_check(dirfd, its));
    coro::stop_coroutine_runtime();

    if ( rc != 0 ) sb::print("round ", r, " failed with ", rc);
    sb::require(rc == 0);
    ++rounds_run;
    files_checked += n;

    for ( usize i = 0; i < its.size(); ++i ) rmfile(its[i].name.c_str());
  }

  sb::print("rounds=", rounds_run, " files=", files_checked, " seed=", SEED);
  micron::syscall(SYS_close, dirfd);
  sb::print("=== RIGOR WAVE PASSED ===");
  return 1;
}
