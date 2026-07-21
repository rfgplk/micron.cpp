//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/filesystem.hpp"
#include "../src/io/fsys.hpp"

#include "../src/string/strings.hpp"
#include "../src/vector/vector.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

constexpr const char *DIR = "/dev/shm/mc_io_fs_bench";
constexpr u32 NFILES = 64;
constexpr usize HOT_BYTES = 4096;
constexpr usize COPY_SMALL = 65536;
constexpr usize COPY_LARGE = 4u << 20;
constexpr usize WRITE_BYTES = 4096;

bool g_csv = false;
volatile u64 g_sink = 0;

micron::vector<micron::io::path_t> g_paths;
micron::io::path_t g_hot;
micron::io::path_t g_copy_src_small;
micron::io::path_t g_copy_src_large;
micron::io::path_t g_copy_dst;
micron::io::path_t g_write_target;

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

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
  char buf[192];
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

template<typename Fn>
f64
measure(Fn &&fn, u64 iters) noexcept
{
  for ( u64 w = 0; w < WARMUP_REPS; ++w )
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());
  f64 ns_s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());
    const u64 t1 = now_ns();
    ns_s[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(iters);
  }
  return median_f64(ns_s, K_MEASUREMENTS);
}

void
report(const char *cat, const char *op, u64 n, usize payload, f64 ns, f64 base_ns) noexcept
{
  const f64 gibs = (ns > 0 && payload > 0) ? (static_cast<f64>(payload) / ns) / 1.073741824 : 0.0;
  const f64 ratio = (ns > 0 && base_ns > 0) ? ns / base_ns : 0.0;
  if ( g_csv ) {
    micron::io::println(cat, ",", op, ",", n, ",", (u64)payload, ",", to_fmt2(ns).whole, ".", to_fmt2(ns).frac, ",", to_fmt2(gibs).whole,
                        ".", to_fmt2(gibs).frac, ",", to_fmt2(ratio).whole, ".", to_fmt2(ratio).frac);
    return;
  }
  line l;
  l.s(cat);
  l.col(22);
  l.s(op);
  l.u_at(n, 40);
  l.u_at(payload, 54);
  l.f2_at(to_fmt2(ns), 66);
  l.f2_at(to_fmt2(gibs), 78);
  l.f2_at(to_fmt2(ratio), 88);
  l.emit();
}

micron::io::path_t
data_path(u32 n) noexcept
{
  char buf[64];
  usize i = 0;
  for ( const char *d = DIR; *d; ++d ) buf[i++] = *d;
  buf[i++] = '/';
  buf[i++] = 'f';
  buf[i++] = static_cast<char>('0' + (n / 10) % 10);
  buf[i++] = static_cast<char>('0' + n % 10);
  buf[i++] = '.';
  buf[i++] = 'd';
  buf[i++] = 'a';
  buf[i++] = 't';
  buf[i] = '\0';
  return micron::io::path_t(buf);
}

micron::io::path_t
sub_path(const char *name) noexcept
{
  char buf[128];
  usize i = 0;
  for ( const char *d = DIR; *d; ++d ) buf[i++] = *d;
  buf[i++] = '/';
  for ( const char *p = name; *p; ++p ) buf[i++] = *p;
  buf[i] = '\0';
  return micron::io::path_t(buf);
}

bool
make_file(const micron::io::path_t &p, usize n) noexcept
{
  micron::string s;
  s.reserve(n + 1);
  for ( usize i = 0; i < n; ++i ) s.push_back(static_cast<char>('a' + (i & 15)));
  const max_t r = micron::io::write_file(p, s);
  return r >= 0 && static_cast<usize>(r) == n;
}

bool
setup() noexcept
{
  micron::io::mkdir_p(micron::io::path_t(DIR));

  g_paths.reserve(NFILES + 1);
  for ( u32 i = 0; i < NFILES; ++i ) {
    micron::io::path_t p = data_path(i);
    const usize sz = (i == 0) ? HOT_BYTES : (64 + (static_cast<usize>(i) * 37) % 960);
    if ( !make_file(p, sz) ) return false;
    g_paths.push_back(p);
  }
  g_hot = g_paths[0];

  g_copy_src_small = sub_path("copy_src_64k.dat");
  g_copy_src_large = sub_path("copy_src_4m.dat");
  g_copy_dst = sub_path("copy_dst.dat");
  g_write_target = sub_path("oneshot_w.dat");

  if ( !make_file(g_copy_src_small, COPY_SMALL) ) return false;
  if ( !make_file(g_copy_src_large, COPY_LARGE) ) return false;
  if ( !make_file(g_write_target, WRITE_BYTES) ) return false;

  micron::io::filesystem<micron::io::rd> probe;
  if ( !probe.exists(g_hot) || !probe.is_regular_file(g_hot) ) return false;
  return true;
}

void
cleanup() noexcept
{
  for ( u32 i = 0; i < NFILES; ++i ) micron::io::unlink(data_path(i));
  micron::io::unlink(g_copy_src_small);
  micron::io::unlink(g_copy_src_large);
  micron::io::unlink(g_copy_dst);
  micron::io::unlink(g_write_target);
  micron::io::rmdir(micron::io::path_t(DIR));
}

f64
bench_open_fstat_close(u64 iters) noexcept
{
  return measure(
      [&]() -> u64 {
        const i32 fd = static_cast<i32>(micron::posix::open(g_hot.c_str(), micron::posix::o_rdonly | micron::posix::o_cloexec));
        if ( fd < 0 ) return 0;
        micron::stat_t st{};
        micron::posix::fstat(fd, st);
        const u64 info = static_cast<u64>(st.st_size) ^ static_cast<u64>(st.st_mode);
        micron::posix::close(fd);
        return info;
      },
      iters);
}

f64
bench_cache_miss() noexcept
{
  for ( u64 w = 0; w < WARMUP_REPS; ++w ) {
    micron::io::filesystem<micron::io::rd> fs;
    for ( u32 i = 0; i < NFILES; ++i ) g_sink += static_cast<u64>(fs.open(g_paths[i]).raw_fd());
  }
  f64 reps[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    micron::io::filesystem<micron::io::rd> fs;
    const u64 t0 = now_ns();
    for ( u32 i = 0; i < NFILES; ++i ) g_sink += static_cast<u64>(fs.open(g_paths[i]).raw_fd());
    const u64 t1 = now_ns();
    reps[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(NFILES);
  }
  return median_f64(reps, K_MEASUREMENTS);
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    micron::string a(argv[i]);
    if ( a == micron::string("--csv") ) g_csv = true;
  }

  if ( !setup() ) {
    micron::io::println("io_fs_bench: fixture setup failed (is /dev/shm writable?)");
    cleanup();
    return 1;
  }

  if ( g_csv ) {
    micron::io::println("# bench=io_fs (filesystem + oneshots)");
    micron::io::println("# k=", (u64)K_MEASUREMENTS, " warmup=", (u64)WARMUP_REPS, " dir=", DIR, " files=", (u64)NFILES);
    micron::io::println("category,op,n,bytes,ns_per_op,gib_s,x_base");
  } else {
    micron::io::println("=== MICRON io_v3 FILESYSTEM (median-of-5, wall-ns) ===");
    micron::io::println("category              op                     N       bytes   ns/op   GiB/s  x-base");
    micron::io::println("--------------------------------------------------------------------------------");

    micron::io::fflush(micron::io::stdout);
  }

  micron::io::filesystem<micron::io::rd> qfs;
  const f64 ns_ofc = bench_open_fstat_close(4000);
  const f64 ns_exists = measure([&]() -> u64 { return qfs.exists(g_hot) ? 1u : 0u; }, 4000);
  const f64 ns_isreg = measure([&]() -> u64 { return qfs.is_regular_file(g_hot) ? 1u : 0u; }, 4000);
  const f64 ns_size = measure([&]() -> u64 { return static_cast<u64>(qfs.file_size(g_hot)); }, 4000);
  report("query", "open+fstat+close", 1, 0, ns_ofc, ns_ofc);
  report("query", "exists", 1, 0, ns_exists, ns_ofc);
  report("query", "is_regular", 1, 0, ns_isreg, ns_ofc);
  report("query", "file_size", 1, 0, ns_size, ns_ofc);
  if ( !g_csv ) {
    micron::io::println("");
    micron::io::fflush(micron::io::stdout);
  }

  const f64 ns_miss = bench_cache_miss();
  f64 ns_hit;
  {
    micron::io::filesystem<micron::io::rd> fs;
    g_sink += static_cast<u64>(fs.open(g_hot).raw_fd());
    ns_hit = measure([&]() -> u64 { return static_cast<u64>(fs.open(g_hot).raw_fd()); }, 4000);
  }
  report("cache", "miss_open", NFILES, 0, ns_miss, ns_miss);
  report("cache", "hit_cached", 1, 0, ns_hit, ns_miss);
  if ( !g_csv ) {
    micron::io::println("");
    micron::io::fflush(micron::io::stdout);
  }

  f64 ns_lru8, ns_lru64;
  {
    micron::io::basic_filesystem<micron::io::rd, micron::null_lock, 8> lru8;
    usize idx8 = 0;
    ns_lru8 = measure(
        [&]() -> u64 {
          const i32 fd = lru8.open(g_paths[idx8]).raw_fd();
          idx8 = (idx8 + 1u) % NFILES;
          return static_cast<u64>(fd);
        },
        1500);
  }
  {
    micron::io::basic_filesystem<micron::io::rd, micron::null_lock, 64> lru64;
    usize idx64 = 0;

    ns_lru64 = measure(
        [&]() -> u64 {
          const i32 fd = lru64.open(g_paths[idx64]).raw_fd();
          idx64 = (idx64 + 1u) % NFILES;
          return static_cast<u64>(fd);
        },
        1500);
  }
  report("lru", "cap8_churn64", NFILES, 0, ns_lru8, ns_miss);
  report("lru", "cap64_hit64", NFILES, 0, ns_lru64, ns_hit);
  if ( !g_csv ) {
    micron::io::println("");
    micron::io::fflush(micron::io::stdout);
  }

  f64 ns_c64, ns_c4m;
  {
    micron::io::filesystem<micron::io::rd> fs;
    ns_c64 = measure([&]() -> u64 { return static_cast<u64>(fs.copy(g_copy_src_small, g_copy_dst)); }, 1500);
    ns_c4m = measure([&]() -> u64 { return static_cast<u64>(fs.copy(g_copy_src_large, g_copy_dst)); }, 250);
  }
  report("copy", "copy_64k", 1, COPY_SMALL, ns_c64, ns_c64);
  report("copy", "copy_4m", 1, COPY_LARGE, ns_c4m, ns_c4m);
  if ( !g_csv ) {
    micron::io::println("");
    micron::io::fflush(micron::io::stdout);
  }

  const f64 ns_rf = measure(
      [&]() -> u64 {
        auto r = micron::io::read_file(g_hot);
        if ( r.is_first() ) return static_cast<u64>(r.cast<micron::string>().size());
        return 0;
      },
      2500);
  micron::string wpayload;
  wpayload.reserve(WRITE_BYTES + 1);
  for ( usize i = 0; i < WRITE_BYTES; ++i ) wpayload.push_back(static_cast<char>('a' + (i & 15)));
  const f64 ns_wf = measure([&]() -> u64 { return static_cast<u64>(micron::io::write_file(g_write_target, wpayload)); }, 1500);
  report("oneshot", "read_file", 1, HOT_BYTES, ns_rf, ns_ofc);
  report("oneshot", "write_file", 1, WRITE_BYTES, ns_wf, ns_wf);

  micron::io::fflush(micron::io::stdout);
  cleanup();
  if ( g_sink == 0xffffffffffffffffULL ) micron::io::println("");
  return 0;
}
