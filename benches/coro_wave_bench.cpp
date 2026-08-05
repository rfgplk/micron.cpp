//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#define MICRON_CORO_URING

#include "../src/io/coroutine/coro_io.hpp"

#include "../src/io/console.hpp"
#include "../src/io/fsys.hpp"
#include "../src/linux/sys/time.hpp"

// io::coro::wave against the per-op coro route, many small files, one worker
//
// benches syscalls per file
//
// build:  duck benches/coro_wave_bench.cpp --uring --perf --fp --no-ssp --no-lto -o bin/b
// run  :  taskset -c 2 ./bin/b/coro_wave_bench
//
// WARNING: /var/tmp, never /tmp. tmpfs has no FMODE_NOWAIT, so io_uring punts every read to an
// io-wq thread and both ring routes measure the punt rather than themselves

namespace coro = micron::coro;
namespace cio = micron::io::coro;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u32 N_FILES = 2048;
constexpr usize FILE_SZ = 1024;
constexpr const char *DIR = "/var/tmp/mc_coro_wave_bench";

volatile u64 g_sink = 0;

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

void
name_of(char *out, u32 i)
{
  usize k = 0;
  const char *p = "f";
  while ( *p ) out[k++] = *p++;
  out[k++] = static_cast<char>('a' + (i % 26u));
  out[k++] = static_cast<char>('a' + ((i / 26u) % 26u));
  out[k++] = static_cast<char>('a' + ((i / 676u) % 26u));
  out[k++] = static_cast<char>('a' + ((i / 17576u) % 26u));
  out[k++] = '.';
  out[k++] = 'd';
  out[k] = '\0';
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
make_corpus()
{
  micron::syscall(SYS_mkdirat, -100, DIR, 0755);
  micron::string body(FILE_SZ, 'x');
  char nm[32];
  char full[512];
  for ( u32 i = 0; i < N_FILES; ++i ) {
    name_of(nm, i);
    path_of(full, nm);
    const long fd = micron::syscall(SYS_openat, -100, full, 0102 | 02 | 01000, 0644);
    if ( fd < 0 ) continue;
    usize off = 0;
    while ( off < body.size() ) {
      const long w = micron::syscall(SYS_write, fd, body.c_str() + off, body.size() - off);
      if ( w <= 0 ) break;
      off += static_cast<usize>(w);
    }
    micron::syscall(SYS_close, fd);
  }
}

u64
run_syscalls(i32 dirfd)
{
  char nm[32];
  micron::buffer buf(FILE_SZ * 2);
  u64 bytes = 0;
  for ( u32 i = 0; i < N_FILES; ++i ) {
    name_of(nm, i);
    const long fd = micron::syscall(SYS_openat, dirfd, nm, 0, 0);
    if ( fd < 0 ) continue;
    const long r = micron::syscall(SYS_read, fd, buf.data(), buf.size());
    if ( r > 0 ) bytes += static_cast<u64>(r);
    micron::syscall(SYS_close, fd);
  }
  return bytes;
}

micron::task<u64>
run_perop(i32 dirfd)
{
  char nm[32];
  micron::buffer buf(FILE_SZ * 2);
  u64 bytes = 0;
  for ( u32 i = 0; i < N_FILES; ++i ) {
    name_of(nm, i);
    const i32 fd = co_await coro::io::openat(dirfd, nm, 0 /*O_RDONLY*/, 0);
    if ( fd < 0 ) continue;
    const i32 r = co_await coro::io::read(fd, buf.data(), static_cast<u32>(buf.size()), 0);
    if ( r > 0 ) bytes += static_cast<u64>(r);
    (void)co_await coro::io::close(fd);
  }
  co_return bytes;
}

micron::task<u64>
run_wave(i32 dirfd)
{
  cio::wave w;
  char nm[32];
  u64 bytes = 0;
  u32 i = 0;
  while ( i < N_FILES ) {
    w.begin(dirfd);
    while ( i < N_FILES && !w.full() ) {
      name_of(nm, i);
      if ( !w.push(nm) ) break;
      ++i;
    }
    if ( co_await w.run() != 0 ) break;
    for ( usize k = 0; k < w.size(); ++k )
      if ( !w[k].unstaged && w[k].err == 0 ) bytes += w[k].len;
    w.clear();
  }
  co_return bytes;
}

void
report(const char *label, f64 ns_total, f64 floor_ns)
{
  const f64 per = ns_total / static_cast<f64>(N_FILES);
  micron::io::print(label, ": ", static_cast<u64>(per), " ns/file");
  if ( floor_ns > 0.0 ) {
    const f64 ratio = floor_ns / ns_total;
    micron::io::print("  vs syscall floor: ", static_cast<u64>(ratio * 100.0), "/100 x");
  }
  micron::io::println("");
}

};      // namespace

int
main()
{
  make_corpus();
  const i32 dirfd = static_cast<i32>(micron::syscall(SYS_openat, -100, DIR, 0 | 0200000, 0));
  if ( dirfd < 0 ) {
    micron::io::println("cannot open ", DIR);
    return 1;
  }

  f64 s_syscall[K_MEASUREMENTS];
  f64 s_perop[K_MEASUREMENTS];
  f64 s_wave[K_MEASUREMENTS];

  g_sink += run_syscalls(dirfd);

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    const u64 t0 = now_ns();
    g_sink += run_syscalls(dirfd);
    s_syscall[m] = static_cast<f64>(now_ns() - t0);
  }

  coro::start_coroutine_runtime(1);
  {
    bool ok = coro::sync_wait([]() -> micron::task<bool> { co_return cio::wave::available(); }());
    if ( !ok ) micron::io::println("NOTE: wave unavailable on this kernel (needs >= 5.19); its row will be empty");

    for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
      const u64 t0 = now_ns();
      g_sink += coro::sync_wait(run_perop(dirfd));
      s_perop[m] = static_cast<f64>(now_ns() - t0);
    }
    for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
      const u64 t0 = now_ns();
      g_sink += coro::sync_wait(run_wave(dirfd));
      s_wave[m] = static_cast<f64>(now_ns() - t0);
    }
  }
  coro::stop_coroutine_runtime();

  const f64 f_sys = median_f64(s_syscall, K_MEASUREMENTS);
  const f64 f_per = median_f64(s_perop, K_MEASUREMENTS);
  const f64 f_wav = median_f64(s_wave, K_MEASUREMENTS);

  micron::io::println("coro wave bench: ", N_FILES, " x ", FILE_SZ, " B on ", DIR, ", one worker, warm");
  micron::io::println("");
  report("syscalls  (openat+read+close)", f_sys, 0.0);
  report("coro per-op (3 enters/file) ", f_per, f_sys);
  report("coro wave   (1 enter/batch) ", f_wav, f_sys);

#if defined(MICRON_CORO_STATS)
  {
    coro::start_coroutine_runtime(1);
    const auto a = coro::io_stats();
    g_sink += coro::sync_wait(run_perop(dirfd));
    const auto b = coro::io_stats();
    g_sink += coro::sync_wait(run_wave(dirfd));
    const auto c = coro::io_stats();
    coro::stop_coroutine_runtime();
    micron::io::println("");
    micron::io::println("enters per file  per-op: ", (b.enters - a.enters) * 100u / N_FILES, "/100");
    micron::io::println("enters per file  wave  : ", (c.enters - b.enters) * 100u / N_FILES, "/100");
    micron::io::println("sqes   per file  wave  : ", (c.submits - b.submits) * 100u / N_FILES, "/100");
  }
#endif

  micron::syscall(SYS_close, dirfd);
  return 0;
}
