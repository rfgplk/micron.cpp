//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING

#include "../src/io/coroutine/coro_io.hpp"

#include "../src/io/console.hpp"
#include "../src/io/fsys.hpp"
#include "../src/linux/sys/time.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace mio = micron::io;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;
volatile u64 g_sink = 0;

constexpr u32 G_READ = 1u << 0;
constexpr u32 G_LINES = 1u << 1;
constexpr u32 G_FANOUT = 1u << 2;
constexpr u32 G_PIPE = 1u << 3;
u32 g_only = 0xffffffffu;

constexpr const char *DIR = "/var/tmp/mc_coro_file_bench";

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

struct fmt2 {
  u64 whole;
  u32 frac;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * 100.0 + 0.5);
  return { s / 100, static_cast<u32>(s % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  col(u32 c) noexcept
  {
    while ( pos < c ) buf[pos++] = ' ';
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char t[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      t[n++] = '0';
    else
      while ( w ) {
        t[n++] = static_cast<char>('0' + (w % 10));
        w /= 10;
      }
    while ( pos + n + 3 < end_col ) buf[pos++] = ' ';
    while ( n ) buf[pos++] = t[--n];
    buf[pos++] = '.';
    buf[pos++] = static_cast<char>('0' + (f.frac / 10));
    buf[pos++] = static_cast<char>('0' + (f.frac % 10));
  }

  void
  emit() noexcept
  {
    buf[pos++] = '\n';
    micron::posix::write(1, buf, pos);
  }
};

inline void
emit_str(const char *p) noexcept
{
  usize n = 0;
  while ( p[n] ) ++n;
  micron::posix::write(1, p, n);
}

void
report(const char *name, f64 v, const char *unit) noexcept
{
  line l;
  l.s("  ");
  l.s(name);
  l.col(46);
  l.f2_at(to_fmt2(v), 60);
  l.s(" ");
  l.s(unit);
  l.emit();
}

micron::io::path_t
mkpath(const char *stem, u32 i = 0xffffffffu)
{
  micron::io::path_t p(DIR);
  p += "/";
  p += stem;
  if ( i != 0xffffffffu ) {
    char sfx[4]
        = { static_cast<char>('a' + (i / 26u / 26u) % 26u), static_cast<char>('a' + (i / 26u) % 26u), static_cast<char>('a' + i % 26u), 0 };
    p += sfx;
  }
  return p;
}

void
make_sized(const micron::io::path_t &p, usize n)
{
  micron::string content(n, 'q');
  for ( usize i = 199; i < n; i += 200 ) content[i] = '\n';
  (void)mio::write_file(p, content);
}

template<typename Fn>
f64
med_of(Fn &&fn, u32 reps)
{
  f64 xs[K_MEASUREMENTS];
  for ( u64 w = 0; w < WARMUP_REPS; ++w ) g_sink += fn(reps / 4 + 1);
  for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
    const u64 t0 = now_ns();
    g_sink += fn(reps);
    xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(reps);
  }
  return median_f64(xs, K_MEASUREMENTS);
}

micron::task<u64>
read_file_loop(micron::io::path_t p, u32 reps)
{
  u64 total = 0;
  for ( u32 i = 0; i < reps; ++i ) {
    auto r = co_await cio::read_file<micron::string>(p);
    if ( r.is_first() ) {
      micron::string s = micron::move(r);
      total += s.size();
    }
  }
  co_return total;
}

micron::task<u64>
each_line_once(micron::io::path_t p)
{
  cio::file f = co_await cio::open_file(micron::move(p));
  u64 lines_n = 0;
  max_t n = co_await f.each_line([&lines_n](const micron::string &l) { lines_n += l.size() ? 1 : 1; });
  (void)n;
  co_return lines_n;
}

micron::task<i32>
pingpong_fd(i32 rfd, i32 wfd, u32 rounds, u32 sz)
{
  char buf[1024];
  for ( u32 i = 0; i < rounds; ++i ) {
    i32 w = co_await micron::coro::io::write(wfd, buf, sz);
    if ( w != static_cast<i32>(sz) ) co_return -1;
    u32 got = 0;
    while ( got < sz ) {
      i32 r = co_await micron::coro::io::read(rfd, buf, sz - got);
      if ( r <= 0 ) co_return -2;
      got += static_cast<u32>(r);
    }
  }
  co_return 0;
}

micron::task<i32>
echo_side(i32 rfd, i32 wfd, u32 rounds, u32 sz, bool sock)
{
  char buf[1024];
  for ( u32 i = 0; i < rounds; ++i ) {
    u32 got = 0;
    while ( got < sz ) {
      i32 r = sock ? co_await micron::coro::io::recv(rfd, buf, sz - got) : co_await micron::coro::io::read(rfd, buf, sz - got);
      if ( r <= 0 ) co_return -1;
      got += static_cast<u32>(r);
    }
    i32 w = sock ? co_await micron::coro::io::send(wfd, buf, sz) : co_await micron::coro::io::write(wfd, buf, sz);
    if ( w != static_cast<i32>(sz) ) co_return -2;
  }
  co_return 0;
}

micron::task<u64>
solo_rtt(i32 c_r, i32 c_w, i32 s_r, i32 s_w, u32 rounds, u32 sz, bool sock)
{
  char buf[1024];
  const u64 t0 = now_ns();
  for ( u32 i = 0; i < rounds; ++i ) {
    i32 w1 = sock ? co_await micron::coro::io::send(c_w, buf, sz) : co_await micron::coro::io::write(c_w, buf, sz);
    if ( w1 != static_cast<i32>(sz) ) co_return 0;
    i32 r1 = sock ? co_await micron::coro::io::recv(s_r, buf, sz) : co_await micron::coro::io::read(s_r, buf, sz);
    if ( r1 != static_cast<i32>(sz) ) co_return 0;
    i32 w2 = sock ? co_await micron::coro::io::send(s_w, buf, sz) : co_await micron::coro::io::write(s_w, buf, sz);
    if ( w2 != static_cast<i32>(sz) ) co_return 0;
    i32 r2 = sock ? co_await micron::coro::io::recv(c_r, buf, sz) : co_await micron::coro::io::read(c_r, buf, sz);
    if ( r2 != static_cast<i32>(sz) ) co_return 0;
  }
  co_return now_ns() - t0;
}

micron::task<u64>
rtt_side(i32 rfd, i32 wfd, u32 rounds, u32 sz, bool sock)
{
  char buf[1024];
  const u64 t0 = now_ns();
  for ( u32 i = 0; i < rounds; ++i ) {
    i32 w = sock ? co_await micron::coro::io::send(wfd, buf, sz) : co_await micron::coro::io::write(wfd, buf, sz);
    if ( w != static_cast<i32>(sz) ) co_return 0;
    u32 got = 0;
    while ( got < sz ) {
      i32 r = sock ? co_await micron::coro::io::recv(rfd, buf, sz - got) : co_await micron::coro::io::read(rfd, buf, sz - got);
      if ( r <= 0 ) co_return 0;
      got += static_cast<u32>(r);
    }
  }
  co_return now_ns() - t0;
}

};      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    const char *a = argv[i];
    auto eq = [&](const char *s) {
      const char *p = a, *q = s;
      while ( *p && *q && *p == *q ) {
        ++p;
        ++q;
      }
      return *p == 0 && *q == 0;
    };
    if ( eq("--only=read") ) g_only = G_READ;
    if ( eq("--only=lines") ) g_only = G_LINES;
    if ( eq("--only=fanout") ) g_only = G_FANOUT;
    if ( eq("--only=pipe") ) g_only = G_PIPE;
  }

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      emit_str("io_uring unavailable; bench aborted\n");
      return 0;
    }
  }
  (void)micron::posix::mkdir(DIR, 0755);

  emit_str("== coro_file_bench (io::coro porcelain) ==\n");
  coro::start_coroutine_runtime(0);

  if ( g_only & G_READ ) {
    emit_str("[read] whole-file value reads (warm)\n");
    micron::io::path_t p4 = mkpath("r4k.dat");
    micron::io::path_t p1m = mkpath("r1m.dat");
    make_sized(p4, 4096);
    make_sized(p1m, 1u << 20);
    {
      f64 v = med_of(
          [&p4](u32 reps) {
            return static_cast<u64>([&] {
              u64 total = 0;
              for ( u32 i = 0; i < reps; ++i ) {
                auto r = mio::read_file<micron::string>(p4);
                if ( r.is_first() ) {
                  micron::string s = micron::move(r);
                  total += s.size();
                }
              }
              return total;
            }());
          },
          2000);
      report("sync io::read_file 4K", v, "ns/op");
    }
    {
      f64 v = med_of([&p4](u32 reps) { return coro::sync_wait(read_file_loop(p4, reps)); }, 2000);
      report("coro read_file 4K", v, "ns/op");
    }
    {
      f64 v = med_of([&p1m](u32 reps) { return coro::sync_wait(read_file_loop(p1m, reps)); }, 100);
      report("coro read_file 1M", v, "ns/op");
    }
  }

  if ( g_only & G_LINES ) {
    emit_str("[lines] 100k-line file\n");
    micron::io::path_t pl = mkpath("lines.dat");
    make_sized(pl, 100000 * 200);
    {
      f64 xs[K_MEASUREMENTS];
      for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
        const u64 t0 = now_ns();
        u64 n = coro::sync_wait(each_line_once(pl));
        xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(n ? n : 1);
        g_sink += n;
      }
      report("coro each_line", median_f64(xs, K_MEASUREMENTS), "ns/line");
    }
    {
      f64 xs[K_MEASUREMENTS];
      for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
        mio::file sf = mio::open_file(pl);
        u64 n = 0;
        const u64 t0 = now_ns();
        (void)sf.each_line([&n](const micron::string &) { ++n; });
        xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(n ? n : 1);
        g_sink += n;
      }
      report("sync each_line", median_f64(xs, K_MEASUREMENTS), "ns/line");
    }
  }

  if ( g_only & G_FANOUT ) {
    emit_str("[fanout] 64 x 256K whole-file loads (the scaling headline)\n");
    micron::vector<micron::io::path_t> paths;
    for ( u32 i = 0; i < 64; ++i ) {
      micron::io::path_t p = mkpath("fan", i);
      make_sized(p, 256 * 1024);
      paths.push_back(p);
    }
    {
      f64 xs[K_MEASUREMENTS];
      for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
        const u64 t0 = now_ns();
        auto res = coro::sync_wait(cio::read_files<micron::string>(paths));
        xs[k] = static_cast<f64>(now_ns() - t0) / 1e6;
        g_sink += res.size();
      }
      report("coro read_files (concurrent)", median_f64(xs, K_MEASUREMENTS), "ms/batch");
    }
    {
      f64 xs[K_MEASUREMENTS];
      for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
        const u64 t0 = now_ns();
        u64 total = 0;
        for ( u32 i = 0; i < 64; ++i ) {
          auto r = mio::read_file<micron::string>(paths[i]);
          if ( r.is_first() ) {
            micron::string s = micron::move(r);
            total += s.size();
          }
        }
        xs[k] = static_cast<f64>(now_ns() - t0) / 1e6;
        g_sink += total;
      }
      report("sync serial loop", median_f64(xs, K_MEASUREMENTS), "ms/batch");
    }
  }

  if ( g_only & G_PIPE ) {
    emit_str("[pipe] local ping-pong RTT (64B); socketpair = corosio comparable\n");

    {
      int a[2], b[2];
      micron::syscall(SYS_pipe2, a, 0);
      micron::syscall(SYS_pipe2, b, 0);
      constexpr u32 ROUNDS = 20000;
      coro::detach(echo_side(a[0], b[1], ROUNDS, 64, false));
      u64 ns = coro::sync_wait(rtt_side(b[0], a[1], ROUNDS, 64, false));
      report("pipe RTT 64B", static_cast<f64>(ns) / ROUNDS, "ns/rt");
      micron::syscall(SYS_close, a[0]);
      micron::syscall(SYS_close, a[1]);
      micron::syscall(SYS_close, b[0]);
      micron::syscall(SYS_close, b[1]);
    }
    {
      int sv[2];
      if ( micron::syscall(SYS_socketpair, 1 /*AF_UNIX*/, 1 /*SOCK_STREAM*/, 0, sv) == 0 ) {
        constexpr u32 ROUNDS = 20000;
        coro::detach(echo_side(sv[1], sv[1], ROUNDS, 64, true));
        u64 ns = coro::sync_wait(rtt_side(sv[0], sv[0], ROUNDS, 64, true));
        report("socketpair RTT 64B", static_cast<f64>(ns) / ROUNDS, "ns/rt");
        micron::syscall(SYS_close, sv[0]);
        micron::syscall(SYS_close, sv[1]);
        int sv2[2];
        micron::syscall(SYS_socketpair, 1, 1, 0, sv2);
        constexpr u32 R1 = 20000;
        coro::detach(echo_side(sv2[1], sv2[1], R1, 1, true));
        u64 n1 = coro::sync_wait(rtt_side(sv2[0], sv2[0], R1, 1, true));
        report("socketpair RTT 1B", static_cast<f64>(n1) / R1, "ns/rt");
        micron::syscall(SYS_close, sv2[0]);
        micron::syscall(SYS_close, sv2[1]);
      }
    }

    {
      const u32 sizes[3] = { 1, 64, 1024 };
      for ( u32 si = 0; si < 3; ++si ) {
        int sv[2];
        if ( micron::syscall(SYS_socketpair, 1, 1, 0, sv) != 0 ) break;
        constexpr u32 ROUNDS = 20000;
        f64 best = 1e30;
        for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
          u64 ns = coro::sync_wait(solo_rtt(sv[0], sv[0], sv[1], sv[1], ROUNDS, sizes[si], true));
          const f64 v = static_cast<f64>(ns) / ROUNDS;
          if ( v < best ) best = v;
        }
        line l;
        l.s("  micron solo socketpair RTT ");
        char t[16];
        u32 n = 0;
        u32 v = sizes[si];
        while ( v ) {
          t[n++] = static_cast<char>('0' + v % 10);
          v /= 10;
        }
        while ( n ) l.buf[l.pos++] = t[--n];
        l.s("B");
        l.col(46);
        l.f2_at(to_fmt2(best), 60);
        l.s(" ns/rt");
        l.emit();
        micron::syscall(SYS_close, sv[0]);
        micron::syscall(SYS_close, sv[1]);
      }
    }
  }

  coro::stop_coroutine_runtime();
  emit_str("done\n");
  return 0;
}
