//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/cached_file.hpp"
#include "../src/io/file.hpp"
#include "../src/memory_block.hpp"

#include "../src/list.hpp"
#include "../src/strings.hpp"
#include "../src/vector/vector.hpp"

namespace mio = micron::io;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

constexpr const char *DIR = "/dev/shm/mc_io_file_bench";
constexpr const char *P_EXIST = "/dev/shm/mc_io_file_bench/existing.bin";
constexpr const char *P_CREATE = "/dev/shm/mc_io_file_bench/create.bin";
constexpr const char *P_WRITE = "/dev/shm/mc_io_file_bench/write.bin";
constexpr const char *P_READ = "/dev/shm/mc_io_file_bench/read.bin";
constexpr const char *P_CACHE = "/dev/shm/mc_io_file_bench/cache.bin";
constexpr const char *P_ATOMIC = "/dev/shm/mc_io_file_bench/atomic.bin";

bool g_csv = false;
volatile u64 g_sink = 0;

struct pod_t {
  i32 a;
  f32 b;
  u64 c;
};

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

[[gnu::always_inline]] inline u64
splitmix64(u64 &s) noexcept
{
  u64 z = (s += 0x9e3779b97f4a7c15ULL);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
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

inline void
emit_str(const char *p) noexcept
{
  usize n = 0;
  while ( p[n] ) ++n;
  micron::posix::write(1, p, n);
}

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
report(const char *type, const char *op, usize elems, usize payload, f64 ns, f64 memcpy_ns) noexcept
{
  const f64 gibs = (ns > 0) ? (static_cast<f64>(payload) / ns) / 1.073741824 : 0.0;
  const f64 ratio = (ns > 0 && memcpy_ns > 0) ? ns / memcpy_ns : 0.0;
  if ( g_csv ) {
    mio::println(type, ",", op, ",", elems, ",", payload, ",", to_fmt2(ns).whole, ".", to_fmt2(ns).frac, ",", to_fmt2(gibs).whole, ".",
                 to_fmt2(gibs).frac, ",", to_fmt2(ratio).whole, ".", to_fmt2(ratio).frac);
    return;
  }
  line l;
  l.s(type);
  l.col(22);
  l.s(op);
  l.u_at(elems, 40);
  l.u_at(payload, 54);
  l.f2_at(to_fmt2(ns), 66);
  l.f2_at(to_fmt2(gibs), 78);
  l.f2_at(to_fmt2(ratio), 88);
  l.emit();
}

u64
iters_blit(usize bytes) noexcept
{
  if ( bytes <= 4096 ) return 2000;
  if ( bytes <= 65536 ) return 800;
  return 200;
}

u64
iters_framed(usize bytes) noexcept
{
  if ( bytes <= 4096 ) return 800;
  if ( bytes <= 65536 ) return 200;
  return 40;
}

f64
memcpy_baseline(usize bytes, u64 iters) noexcept
{
  const usize n = bytes ? bytes : 1;
  micron::buffer src(n);
  micron::buffer dst(n);
  src.data()[0] = static_cast<byte>(1);
  return measure(
      [&]() -> u64 {
        micron::memcpy(dst.data(), src.data(), bytes);
        return static_cast<u64>(dst.data()[0]);
      },
      iters);
}

void
prepare_file(const char *path, usize bytes) noexcept
{
  mio::file f(path, mio::rwc);
  if ( !f.valid() ) return;
  micron::buffer b(bytes ? bytes : 1);
  b.data()[0] = static_cast<byte>(0x5a);
  f.write(b.data(), bytes);
}

void
bench_open_close() noexcept
{
  prepare_file(P_EXIST, 4096);

  const f64 rd_ns = measure(
      [&]() -> u64 {
        mio::file f(P_EXIST, mio::modes::read);
        return static_cast<u64>(f.raw_fd() + 1);
      },
      2000);
  report("file/open", "existing(rd)", 0, 0, rd_ns, 0.0);

  const f64 cr_ns = measure(
      [&]() -> u64 {
        mio::file f(P_CREATE, mio::rwc);
        return static_cast<u64>(f.raw_fd() + 1);
      },
      2000);
  report("file/open", "create(rwc)", 0, 0, cr_ns, 0.0);

  micron::posix::unlink(P_CREATE);
}

template<typename C>
void
run_write_case(const char *type, const char *op, const C &fixture, usize elems, usize payload, u64 iters, f64 mc_ns) noexcept
{
  mio::file f(P_WRITE, mio::rwc);
  if ( !f.valid() ) {
    mio::println("  (open failed for ", type, "/", op, ")");
    return;
  }
  const f64 ns = measure(
      [&]() -> u64 {
        f.seek(0);
        return static_cast<u64>(f.write(fixture));
      },
      iters);
  report(type, op, elems, payload, ns, mc_ns);
}

void
bench_write() noexcept
{
  const usize sizes[] = { 4096, 65536, 1048576 };
  for ( usize sz : sizes ) {
    const u64 it_b = iters_blit(sz);
    const u64 it_f = iters_framed(sz);
    const f64 mc = memcpy_baseline(sz, it_b);

    micron::string s(sz, 'x');
    run_write_case("file/write", "string", s, sz, sz, it_b, mc);

    {
      u64 rng = 0x1234 ^ sz;
      const usize n = sz / sizeof(u32);
      micron::vector<u32> v;
      v.reserve(n);
      for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<u32>(splitmix64(rng)));
      run_write_case("file/write", "vector<u32>", v, n, n * sizeof(u32), it_b, mc);
    }

    if ( sz <= 65536 ) {
      u64 rng = 0x9abc ^ sz;
      const usize n = sz / sizeof(i32);
      micron::list<i32> l;
      for ( usize i = 0; i < n; ++i ) l.push_back(static_cast<i32>(splitmix64(rng)));
      run_write_case("file/write", "list<i32>(fr)", l, n, n * sizeof(i32), it_f, mc);
    }
  }

  const f64 mc_pod = memcpy_baseline(sizeof(pod_t), 2000);
  pod_t p{ 42, 2.5f, 0xdeadbeefULL };
  run_write_case("file/write", "pod", p, 1, sizeof(pod_t), 2000, mc_pod);
}

void
bench_read() noexcept
{
  const usize sizes[] = { 4096, 65536, 1048576 };
  for ( usize sz : sizes ) {
    const u64 it = iters_blit(sz);
    const f64 mc = memcpy_baseline(sz, it);
    prepare_file(P_READ, sz);

    mio::file f(P_READ, mio::modes::read);
    if ( !f.valid() ) {
      mio::println("  (open failed for file/read sz=", sz, ")");
      continue;
    }

    const usize nu = sz / sizeof(u32);
    const f64 rv_ns = measure(
        [&]() -> u64 {
          f.seek(0);
          auto v = f.read<micron::vector<u32>>();
          return static_cast<u64>(v.size());
        },
        it);
    report("file/read", "vector<u32>", nu, sz, rv_ns, mc);

    const f64 rs_ns = measure(
        [&]() -> u64 {
          f.seek(0);
          auto s = f.read<micron::string>();
          return static_cast<u64>(s.size());
        },
        it);
    report("file/read", "string", sz, sz, rs_ns, mc);
  }
}

void
bench_cached() noexcept
{
  const usize sz = 65536;
  const u64 it = iters_blit(sz);
  const f64 mc = memcpy_baseline(sz, it);

  prepare_file(P_CACHE, sz);
  mio::cached_file<micron::string> cf(P_CACHE, mio::rw);
  if ( !cf.valid() ) {
    mio::println("  (open failed for cached_file)");
    return;
  }

  const f64 ld_ns = measure([&]() -> u64 { return static_cast<u64>(cf.load()); }, it);
  report("cached/load", "string", sz, sz, ld_ns, mc);

  micron::string payload(sz, 'q');
  const f64 fl_ns = measure(
      [&]() -> u64 {
        cf.set_start();
        cf.push_copy(payload);
        return static_cast<u64>(cf.flush());
      },
      it);
  report("cached/flush", "string(+stage)", sz, sz, fl_ns, mc);

  const usize apn = 4096;
  const f64 mc_ap = memcpy_baseline(apn, 500);
  micron::string small(apn, 'a');
  const f64 ap_ns = measure([&]() -> u64 { return static_cast<u64>(cf.append(small)); }, 500);
  report("cached/append", "string", apn, apn, ap_ns, mc_ap);
}

void
bench_atomic() noexcept
{
  const usize sz = 65536;
  const f64 mc = memcpy_baseline(sz, 300);

  prepare_file(P_ATOMIC, sz);
  mio::file f(P_ATOMIC, mio::rwc);
  if ( !f.valid() ) {
    mio::println("  (open failed for atomic_replace)");
    return;
  }

  micron::string payload(sz, 'z');
  const f64 ns = measure([&]() -> u64 { return static_cast<u64>(f.atomic_replace(payload.c_str(), payload.size())) + 1u; }, 300);
  report("atomic/replace", "ptr,len", 1, sz, ns, mc);
}

void
cleanup() noexcept
{
  micron::posix::unlink(P_EXIST);
  micron::posix::unlink(P_CREATE);
  micron::posix::unlink(P_WRITE);
  micron::posix::unlink(P_READ);
  micron::posix::unlink(P_CACHE);
  micron::posix::unlink(P_ATOMIC);
  micron::posix::rmdir(DIR);
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    micron::string a(argv[i]);
    if ( a == micron::string("--csv") ) g_csv = true;
  }

  micron::posix::mkdir(DIR, 0755u);

  if ( g_csv ) {
    mio::println("# bench=io_file (io_v3 file data-path)");
    mio::println("# k=", (u64)K_MEASUREMENTS, " warmup=", (u64)WARMUP_REPS, " dir=", DIR);
    mio::println("type,op,elems,bytes,ns_per_op,gib_s,vs_memcpy");
  } else {

    emit_str("=== MICRON io_v3 FILE DATA-PATH (median-of-5, wall-ns, tmpfs) ===\n");
    emit_str("type                  op            elems       bytes   ns/op   GiB/s   x-mc\n");
    emit_str("--------------------------------------------------------------------------------\n");
  }

  try {
    bench_open_close();
    if ( !g_csv ) emit_str("\n");
    bench_write();
    if ( !g_csv ) emit_str("\n");
    bench_read();
    if ( !g_csv ) emit_str("\n");
    bench_cached();
    if ( !g_csv ) emit_str("\n");
    bench_atomic();
  } catch ( ... ) {
    emit_str("  (benchmark aborted: an io op threw)\n");
  }

  cleanup();
  mio::fflush(mio::stdout);
  if ( g_sink == 0xffffffffffffffffULL ) mio::println("");
  return 0;
}
