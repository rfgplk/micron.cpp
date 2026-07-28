//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_CORO_URING
#define MICRON_CORO_STATS

#include "../src/tasks/tasks.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"

namespace coro = micron::coro;
namespace cio = micron::coro::io;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

bool g_csv = false;
bool g_tmpfs = false;
volatile u64 g_sink = 0;

constexpr u32 G_NULL = 1u << 0;
constexpr u32 G_INLINE = 1u << 1;
constexpr u32 G_PARK = 1u << 2;
constexpr u32 G_TPUT = 1u << 3;
constexpr u32 G_SPAWN = 1u << 4;
u32 g_only = 0xffffffffu;

constexpr const char *DIR_DISK = "/var/tmp/mc_coro_aio_bench";
constexpr const char *DIR_TMPFS = "/dev/shm/mc_coro_aio_bench";
const char *g_dir = DIR_DISK;

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
  u_at(u64 v, u32 end_col) noexcept
  {
    char t[24];
    u32 n = 0;
    if ( v == 0 )
      t[n++] = '0';
    else
      while ( v ) {
        t[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
      }
    while ( pos + n < end_col ) buf[pos++] = ' ';
    while ( n ) buf[pos++] = t[--n];
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
report(const char *name, f64 ns_per_op, u64 inline_hits, u64 total_ops) noexcept
{
  if ( g_csv ) {
    line l;
    l.s(name);
    l.s(",");
    l.f2_at(to_fmt2(ns_per_op), l.pos + 12);
    l.s(",");
    l.u_at(total_ops != 0 ? inline_hits * 100 / total_ops : 0, l.pos + 4);
    l.emit();
    return;
  }
  line l;
  l.s("  ");
  l.s(name);
  l.col(44);
  l.f2_at(to_fmt2(ns_per_op), 58);
  l.s(" ns/op");
  if ( total_ops != 0 ) {
    l.u_at(inline_hits * 100 / total_ops, 72);
    l.s("% inl");
  }
  l.emit();
}

void
report_tput(const char *name, f64 gbps) noexcept
{
  line l;
  l.s("  ");
  l.s(name);
  l.col(44);
  l.f2_at(to_fmt2(gbps), 58);
  l.s(" GB/s");
  l.emit();
}

void
mkpath(char *out, const char *name)
{
  usize k = 0;
  for ( const char *p = g_dir; *p; p++ ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = name; *p; p++ ) out[k++] = *p;
  out[k] = '\0';
}

long
make_file(const char *path, usize bytes)
{
  long fd = micron::syscall(SYS_openat, -100, path, 0102 | 02 | 01000, 0644);
  if ( fd < 0 ) return fd;
  alignas(4096) static char blk[65536];
  for ( usize i = 0; i < sizeof(blk); ++i ) blk[i] = static_cast<char>(i * 131u);
  usize left = bytes;
  while ( left != 0 ) {
    usize n = left > sizeof(blk) ? sizeof(blk) : left;
    if ( micron::syscall(SYS_write, fd, blk, n) != static_cast<long>(n) ) break;
    left -= n;
  }
  micron::syscall(SYS_fsync, fd);
  return fd;
}

micron::task<u64>
body_resched(u64 reps)
{
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) co_await coro::reschedule();
  co_return now_ns() - t0;
}

micron::task<u64>
body_nop(u64 reps)
{
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 r = co_await cio::nop();
    g_sink += static_cast<u64>(r);
  }
  co_return now_ns() - t0;
}

micron::task<u64>
body_read4k(i32 fd, u64 reps)
{
  alignas(64) static char buf[4096];
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 r = co_await cio::read(fd, buf, sizeof(buf), 0);
    g_sink += static_cast<u64>(r);
  }
  co_return now_ns() - t0;
}

micron::task<u64>
body_chain(const char *path, u64 reps)
{
  alignas(64) static char buf[4096];
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 fd = co_await cio::openat(-100, path, 0 /*O_RDONLY*/, 0);
    i32 r = co_await cio::read(fd, buf, sizeof(buf), 0);
    i32 c = co_await cio::close(fd);
    g_sink += static_cast<u64>(r) + static_cast<u64>(c);
  }
  co_return now_ns() - t0;
}

micron::task<u64>
body_wsync(i32 fd, u64 reps)
{
  alignas(64) static char buf[4096];
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 w = co_await cio::write(fd, buf, sizeof(buf), 0);
    i32 s = co_await cio::fsync(fd);
    g_sink += static_cast<u64>(w) + static_cast<u64>(s);
  }
  co_return now_ns() - t0;
}

micron::task<i32>
stripe_reader(i32 fd, u64 base, u64 stripe, u32 chunks)
{
  alignas(4096) char buf[65536];
  for ( u32 i = 0; i < chunks; ++i ) {
    i32 r = co_await cio::read(fd, buf, sizeof(buf), base + static_cast<u64>(i) * 65536ull);
    if ( r <= 0 ) co_return -1;
    g_sink += static_cast<u64>(static_cast<u8>(buf[0]));
  }
  (void)stripe;
  co_return 0;
}

micron::atomic_token<u64> g_tput_errs{ 0 };

micron::task<i32> stripe_reader_counted(i32 fd, u64 base, u64 stripe, u32 chunks);

micron::task<u64>
body_tput_fork(i32 fd, u64 stripe, u32 chunks)
{
  const u64 t0 = now_ns();
  for ( u32 i = 0; i < 32; ++i )
    co_await coro::fork(coro::discard, stripe_reader_counted)(fd, static_cast<u64>(i) * stripe, stripe, chunks);
  co_await coro::join;
  co_return now_ns() - t0;
}

micron::task<i32>
stripe_reader_counted(i32 fd, u64 base, u64 stripe, u32 chunks)
{
  alignas(4096) char buf[65536];
  for ( u32 i = 0; i < chunks; ++i ) {
    i32 r = co_await cio::read(fd, buf, sizeof(buf), base + static_cast<u64>(i) * 65536ull);
    if ( r <= 0 ) {
      g_tput_errs.fetch_add(1, micron::memory_order_relaxed);
      co_return -1;
    }
    g_sink += static_cast<u64>(static_cast<u8>(buf[0]));
  }
  (void)stripe;
  co_return 0;
}

micron::task<i32>
trivial_child()
{
  co_return 1;
}

micron::task<u64>
body_chain_depth1(u64 reps)
{
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 v = co_await trivial_child();
    g_sink += static_cast<u64>(v);
  }
  co_return now_ns() - t0;
}

micron::task<i32>
depth2_child()
{
  i32 v = co_await trivial_child();
  co_return v + 1;
}

micron::task<i32>
depth3_child()
{
  i32 v = co_await depth2_child();
  co_return v + 1;
}

micron::task<u64>
body_chain_depth3(u64 reps)
{
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    i32 v = co_await depth3_child();
    g_sink += static_cast<u64>(v);
  }
  co_return now_ns() - t0;
}

micron::task<u64>
body_forkjoin(u64 reps)
{
  const u64 t0 = now_ns();
  for ( u64 i = 0; i < reps; ++i ) {
    co_await coro::fork(coro::discard, trivial_child)();
    co_await coro::join;
  }
  co_return now_ns() - t0;
}

template<typename MakeTask>
f64
run_med(MakeTask &&mk, u64 reps) noexcept
{
  f64 xs[K_MEASUREMENTS];
  for ( u64 w = 0; w < WARMUP_REPS; ++w ) g_sink += coro::sync_wait(mk(reps / 4 + 1));
  for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
    u64 ns = coro::sync_wait(mk(reps));
    xs[k] = static_cast<f64>(ns) / static_cast<f64>(reps);
  }
  return median_f64(xs, K_MEASUREMENTS);
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
    if ( eq("--csv") ) g_csv = true;
    if ( eq("--tmpfs") ) {
      g_tmpfs = true;
      g_dir = DIR_TMPFS;
    }
    if ( eq("--only=null") ) g_only = G_NULL;
    if ( eq("--only=inline") ) g_only = G_INLINE;
    if ( eq("--only=park") ) g_only = G_PARK;
    if ( eq("--only=tput") ) g_only = G_TPUT;
    if ( eq("--only=spawn") ) g_only = G_SPAWN;
  }

  {
    micron::uring::ring probe;
    if ( int rc = probe.init(4); rc != 0 ) {
      emit_str("io_uring unavailable; bench aborted\n");
      return 0;
    }
  }
  (void)micron::posix::mkdir(g_dir, 0755);

  emit_str("== coro_aio_bench (per-worker rings, inline completion) ==\n");
  if ( g_tmpfs ) emit_str("!! tmpfs mode: FMODE_NOWAIT absent, every file op punts to io-wq !!\n");

  if ( g_only & G_NULL ) {
    emit_str("[null] scheduler + ring null paths (1 worker)\n");
    coro::start_coroutine_runtime(1);
    {
      f64 v = run_med([](u64 r) { return body_resched(r); }, 1000000);
      report("resched (suspend+queue+resume)", v, 0, 0);
    }
    {
      coro::__io_stats_reset();
      f64 v = run_med([](u64 r) { return body_nop(r); }, 200000);
      coro::io_stats_t st = coro::io_stats();
      report("op_nop qd1 (full ring round trip)", v, st.inline_completions, st.submits);
    }
    coro::stop_coroutine_runtime();
  }

  if ( g_only & G_INLINE ) {
    emit_str("[inline] cached 4K reads (xfs unless --tmpfs)\n");
    char path[160];
    mkpath(path, "warm4k.dat");
    long sfd = make_file(path, 4096);
    if ( sfd >= 0 ) {
      alignas(64) static char buf[4096];
      micron::syscall(SYS_pread64, sfd, buf, sizeof(buf), 0);

      {
        f64 xs[K_MEASUREMENTS];
        const u64 reps = 200000;
        for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
          const u64 t0 = now_ns();
          for ( u64 i = 0; i < reps; ++i ) g_sink += static_cast<u64>(micron::syscall(SYS_pread64, sfd, buf, sizeof(buf), 0));
          xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(reps);
        }
        report("pread(2) sync baseline", median_f64(xs, K_MEASUREMENTS), 0, 0);
      }
      {
        coro::start_coroutine_runtime(1);
        coro::__io_stats_reset();
        i32 fd = static_cast<i32>(sfd);
        f64 v = run_med([fd](u64 r) { return body_read4k(fd, r); }, 200000);
        coro::io_stats_t st = coro::io_stats();
        report("co_await read 4K (inline path)", v, st.inline_completions, st.submits);
        coro::stop_coroutine_runtime();
      }
      {
        coro::start_coroutine_runtime(1);
        coro::__io_stats_reset();
        f64 v = run_med([&path](u64 r) { return body_chain(path, r); }, 50000);
        coro::io_stats_t st = coro::io_stats();
        report("co_await open+read+close chain", v, st.inline_completions, st.submits);
        coro::stop_coroutine_runtime();
      }
      micron::syscall(SYS_close, sfd);
      micron::syscall(SYS_unlinkat, -100, path, 0);
    }
  }

  if ( g_only & G_PARK ) {
    emit_str("[park] uncached / durability paths\n");
    char path[160];
    mkpath(path, "direct4k.dat");
    long sfd = make_file(path, 1u << 20);
    if ( sfd >= 0 ) {
      micron::syscall(SYS_close, sfd);
      long dfd = micron::syscall(SYS_openat, -100, path, 0 | 040000 /*O_RDONLY|O_DIRECT*/, 0);
      if ( dfd >= 0 ) {
        alignas(4096) static char dbuf[4096];
        {
          f64 xs[K_MEASUREMENTS];
          const u64 reps = 2000;
          for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
            const u64 t0 = now_ns();
            for ( u64 i = 0; i < reps; ++i )
              g_sink += static_cast<u64>(micron::syscall(SYS_pread64, dfd, dbuf, sizeof(dbuf), (i % 256) * 4096));
            xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(reps);
          }
          report("O_DIRECT pread(2) baseline", median_f64(xs, K_MEASUREMENTS), 0, 0);
        }
        {
          coro::start_coroutine_runtime(2);
          coro::__io_stats_reset();
          i32 fd = static_cast<i32>(dfd);
          f64 v = run_med(
              [fd](u64 r) -> micron::task<u64> {
                alignas(4096) static char b2[4096];
                const u64 t0 = now_ns();
                for ( u64 i = 0; i < r; ++i ) {
                  i32 rr = co_await cio::read(fd, b2, sizeof(b2), (i % 256) * 4096);
                  g_sink += static_cast<u64>(rr);
                }
                co_return now_ns() - t0;
              },
              2000);
          coro::io_stats_t st = coro::io_stats();
          report("co_await read O_DIRECT (park path)", v, st.inline_completions, st.submits);
          coro::stop_coroutine_runtime();
        }
        micron::syscall(SYS_close, dfd);
      } else {
        emit_str("  O_DIRECT unsupported here; park read leg skipped\n");
      }
    }
    {
      char wpath[160];
      mkpath(wpath, "wsync4k.dat");
      long wfd = make_file(wpath, 4096);
      if ( wfd >= 0 ) {
        alignas(64) static char buf[4096];
        {
          f64 xs[K_MEASUREMENTS];
          const u64 reps = 400;
          for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
            const u64 t0 = now_ns();
            for ( u64 i = 0; i < reps; ++i ) {
              g_sink += static_cast<u64>(micron::syscall(SYS_pwrite64, wfd, buf, sizeof(buf), 0));
              g_sink += static_cast<u64>(micron::syscall(SYS_fsync, wfd));
            }
            xs[k] = static_cast<f64>(now_ns() - t0) / static_cast<f64>(reps);
          }
          report("write+fsync sync baseline", median_f64(xs, K_MEASUREMENTS), 0, 0);
        }
        {
          coro::start_coroutine_runtime(2);
          i32 fd = static_cast<i32>(wfd);
          f64 v = run_med([fd](u64 r) { return body_wsync(fd, r); }, 400);
          report("co_await write+fsync", v, 0, 0);
          coro::stop_coroutine_runtime();
        }
        micron::syscall(SYS_close, wfd);
        micron::syscall(SYS_unlinkat, -100, wpath, 0);
      }
    }
  }

  if ( g_only & G_TPUT ) {
    emit_str("[tput] 256MB warm file, QD32 x 64K reads\n");
    char path[160];
    mkpath(path, "tput256m.dat");
    constexpr u64 FILE_SZ = 256ull << 20;
    long sfd = make_file(path, FILE_SZ);
    if ( sfd >= 0 ) {
      alignas(4096) static char buf[65536];
      for ( u64 off = 0; off < FILE_SZ; off += sizeof(buf) )
        g_sink += static_cast<u64>(micron::syscall(SYS_pread64, sfd, buf, sizeof(buf), off));
      {
        f64 xs[K_MEASUREMENTS];
        for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
          const u64 t0 = now_ns();
          for ( u64 off = 0; off < FILE_SZ; off += sizeof(buf) )
            g_sink += static_cast<u64>(micron::syscall(SYS_pread64, sfd, buf, sizeof(buf), off));
          xs[k] = static_cast<f64>(now_ns() - t0);
        }
        const f64 med = median_f64(xs, K_MEASUREMENTS);
        report_tput("pread(2) sequential baseline", static_cast<f64>(FILE_SZ) / med);
      }
      {
        coro::start_coroutine_runtime(0);
        i32 fd = static_cast<i32>(sfd);
        const u64 stripe = FILE_SZ / 32;
        const u32 chunks = static_cast<u32>(stripe / 65536);
        f64 xs[K_MEASUREMENTS];
        for ( u32 k = 0; k < K_MEASUREMENTS; ++k ) {
          u64 ns = coro::sync_wait(body_tput_fork(fd, stripe, chunks));
          xs[k] = static_cast<f64>(ns);
        }
        const f64 med = median_f64(xs, K_MEASUREMENTS);
        report_tput("coro QD32 fork/join striped", static_cast<f64>(FILE_SZ) / med);
        if ( g_tput_errs.get(micron::memory_order_relaxed) != 0 ) emit_str("  !! stripe reader errors detected !!\n");
        coro::stop_coroutine_runtime();
      }
      micron::syscall(SYS_close, sfd);
      micron::syscall(SYS_unlinkat, -100, path, 0);
    }
  }

  if ( g_only & G_SPAWN ) {
    emit_str("[spawn] task machinery (1 worker)\n");
    coro::start_coroutine_runtime(1);
    {
      f64 v = run_med([](u64 r) { return body_chain_depth1(r); }, 1000000);
      report("await child task depth-1", v, 0, 0);
    }
    {
      f64 v = run_med([](u64 r) { return body_chain_depth3(r); }, 500000);
      report("await child chain depth-3", v, 0, 0);
    }
    {
      f64 v = run_med([](u64 r) { return body_forkjoin(r); }, 500000);
      report("fork(discard)+join pair", v, 0, 0);
    }
    coro::stop_coroutine_runtime();
  }

  emit_str("done\n");
  return 0;
}
