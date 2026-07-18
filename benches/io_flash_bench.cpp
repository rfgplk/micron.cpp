//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// io::flash (io_uring-native file io) latency + throughput bench.
//
// the headline metric here is NOT wall time -- it is `sqe` (submission-queue entries consumed) and
// `rt` (ring round-trips == io_uring_enter calls). flash is synchronous-completing, so a layer that
// batches badly is invisible in wall-clock on a hot page cache while burning N syscalls where it
// should burn 1. sqe/rt == 1.00 means every op paid its own syscall.
//
// counting is in-process ring-state differencing (perf tracepoints are unavailable at
// perf_event_paranoid=2). ground truth:
//   strace -c -f -e trace=io_uring_enter ./bin/io_flash_bench/io_flash_bench --only=porcelain --reps=1
// counts only from a traced run -- ptrace inflates wall time 50-200x.
//
// build: duck build benches/io_flash_bench.cpp --no-lto --no-ssp
// run  : ./bin/io_flash_bench/io_flash_bench [--csv] [--cold] [--only=<group>] [--reps=N]

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/flash.hpp"
#include "../src/io/fsys.hpp"
#include "../src/linux/sys/fcntl.hpp"
#include "../src/memory_block.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector/vector.hpp"

namespace mio = micron::io;
namespace mf = micron::io::flash;
namespace px = micron::posix;
namespace mc = micron;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// preamble (idiom shared with benches/io_file_bench.cpp)

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

bool g_csv = false;
bool g_cold = false;
bool g_tmpfs = false;
u32 g_reps = K_MEASUREMENTS;
volatile u64 g_sink = 0;

// group selector bits
constexpr u32 G_LATENCY = 1u << 0;
constexpr u32 G_OPEN = 1u << 1;
constexpr u32 G_PORCELAIN = 1u << 2;
constexpr u32 G_BATCH = 1u << 3;
constexpr u32 G_QUEUE = 1u << 4;
constexpr u32 G_SEQ = 1u << 5;
constexpr u32 G_FIXED = 1u << 6;
u32 g_only = 0xffffffffu;

// DEFAULT IS A REAL FILESYSTEM, DELIBERATELY.
//
// tmpfs (/dev/shm, /tmp) does not set FMODE_NOWAIT on its files, so io_uring cannot complete an op
// inline in the submitting task and punts EVERY one to an io-wq kernel worker. Measured on this box
// (kernel 7.1.3): a 4-byte pread costs 1054 ns on xfs and 12847 ns on tmpfs at an identical rt=1,
// and the tmpfs run spawns 2 iou-wrk threads where the xfs run spawns none. Benchmarking flash on
// tmpfs measures that thread handoff, not this layer. --tmpfs is kept only to document the effect.
constexpr const char *DIR_DISK = "/var/tmp/mc_io_flash_bench";      // on /, xfs, real NVMe
constexpr const char *DIR_TMPFS = "/dev/shm/mc_io_flash_bench";
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// counter layer: in-process ring-state differencing.
//
//   sqe -- delta *sq_tail. advance_sq only ever adds, so it is a free-running monotone counter.
//   cqe -- delta *cq_head. peek_cqe release-stores it per entry. cqe != sqe -> strays/leaks.
//   rt  -- delta next_seq(). __rw/__oneshot bump it exactly once per submit_wait(1), i.e. per
//          io_uring_enter. the probe itself consumes one value, hence the -1.
//
// rt does NOT see submit_wait calls made internally without a next_seq bump (batch wave 1,
// chain::run). those are modelled analytically per-case and added into `rt`.

[[gnu::always_inline]] inline u32
c_sqe(mf::engine &e) noexcept
{
  return *e.ring().sq_tail;
}

[[gnu::always_inline]] inline u32
c_cqe(mf::engine &e) noexcept
{
  return *e.ring().cq_head;
}

[[gnu::always_inline]] inline u64
c_rt(mf::engine &e) noexcept
{
  return e.next_seq();
}

struct rec {
  u64 n = 0;            // logical ops (files, blocks) per measured iteration
  usize bytes = 0;      // payload bytes per measured iteration
  f64 ns = 0;           // median ns per measured iteration
  u64 sqe = 0;
  u64 cqe = 0;
  u64 rt = 0;
  bool counted = false;
};

// a timed pass should take roughly this long regardless of whether the op costs 300 ns or 3 ms.
// without this the device-bound cases (fsync ~360 us, whole-file write ~3 ms) dominate the suite.
constexpr u64 TARGET_PASS_NS = 20000000ULL;      // 20 ms

// calibrate an iteration count from one untimed sample; `hint` is the ceiling.
template<typename Fn>
u64
calibrate(Fn &&fn, u64 hint) noexcept
{
  const u64 t0 = now_ns();
  g_sink += static_cast<u64>(fn());
  const u64 dt = now_ns() - t0;
  if ( dt == 0 ) return hint;
  u64 n = TARGET_PASS_NS / dt;
  if ( n < 1 ) n = 1;
  return n > hint ? hint : n;
}

// warmup -> ONE untimed counted iteration -> K timed iterations. counter reads never enter the
// timed loop. `iters` is a ceiling; the real count is calibrated to TARGET_PASS_NS.
template<typename Fn>
rec
measure_counted(Fn &&fn, u64 iters_max, u64 n, usize bytes, mf::engine &eng, u64 extra_rt = 0) noexcept
{
  rec r;
  r.n = n;
  r.bytes = bytes;
  const u64 iters = calibrate(fn, iters_max);

  for ( u64 w = 0; w < WARMUP_REPS; ++w )
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());

  {
    const u32 s0 = c_sqe(eng);
    const u32 q0 = c_cqe(eng);
    const u64 t0 = c_rt(eng);
    g_sink += static_cast<u64>(fn());
    r.sqe = static_cast<u64>(c_sqe(eng) - s0);
    r.cqe = static_cast<u64>(c_cqe(eng) - q0);
    r.rt = (c_rt(eng) - t0) - 1 + extra_rt;
    r.counted = true;
  }

  f64 ns_s[16];
  const u32 k = g_reps > 16 ? 16 : g_reps;
  for ( u32 m = 0; m < k; ++m ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());
    const u64 t1 = now_ns();
    ns_s[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(iters);
  }
  r.ns = median_f64(ns_s, k);
  return r;
}

// posix / control cases: same shape, no ring counters
template<typename Fn>
rec
measure_plain(Fn &&fn, u64 iters_max, u64 n, usize bytes) noexcept
{
  rec r;
  r.n = n;
  r.bytes = bytes;
  const u64 iters = calibrate(fn, iters_max);

  for ( u64 w = 0; w < WARMUP_REPS; ++w )
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());

  f64 ns_s[16];
  const u32 k = g_reps > 16 ? 16 : g_reps;
  for ( u32 m = 0; m < k; ++m ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < iters; ++i ) g_sink += static_cast<u64>(fn());
    const u64 t1 = now_ns();
    ns_s[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(iters);
  }
  r.ns = median_f64(ns_s, k);
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reporting

void
report(const char *layer, const char *cs, const rec &r, f64 base_ns) noexcept
{
  const f64 gibs = (r.ns > 0) ? (static_cast<f64>(r.bytes) / r.ns) / 1.073741824 : 0.0;
  const f64 ratio = (r.ns > 0 && base_ns > 0) ? r.ns / base_ns : 0.0;
  if ( g_csv ) {
    mio::println(layer, ",", cs, ",", r.n, ",", (u64)r.bytes, ",", to_fmt2(r.ns).whole, ".", to_fmt2(r.ns).frac, ",", to_fmt2(gibs).whole,
                 ".", to_fmt2(gibs).frac, ",", r.sqe, ",", r.cqe, ",", r.rt, ",", to_fmt2(ratio).whole, ".", to_fmt2(ratio).frac);
    return;
  }
  line l;
  l.s(layer);
  l.col(20);
  l.s(cs);
  l.u_at(r.n, 44);
  l.u_at(static_cast<u64>(r.bytes), 57);
  l.f2_at(to_fmt2(r.ns), 70);
  l.f2_at(to_fmt2(gibs), 80);
  if ( r.counted ) {
    l.u_at(r.sqe, 87);
    l.u_at(r.rt, 94);
  } else {
    l.col(94);
    l.s("   -      -");
    l.pos = 94;
  }
  l.f2_at(to_fmt2(ratio), 103);
  if ( r.counted && r.cqe != r.sqe ) l.s("  !");
  l.emit();
}

void
note(const char *s) noexcept
{
  if ( g_csv ) return;
  emit_str(s);
}

void
hdr(const char *s) noexcept
{
  if ( g_csv ) return;
  emit_str("\n");
  emit_str(s);
  emit_str("\n");
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fixture. every path_t is built HERE -- io::path_t is sstr<4096>, so constructing one inside a
// measured lambda would measure a 4 KiB memset.

constexpr usize LAT_SZ = 4u << 20;
constexpr usize SEQ_SZ = 16u << 20;
constexpr u32 BATCH_MAX = 512;
constexpr usize BATCH_SMALL = 4096;
constexpr usize BATCH_LARGE = 65536;
constexpr usize QUEUE_BUF = 8u << 20;

struct fixture {
  mio::path_t lat;
  mio::path_t seq_r, seq_w;
  mio::path_t churn;
  mio::path_t whole_r[3], whole_w[3];
  mc::vector<mio::path_t> small;      // BATCH_MAX x 4 KiB
  mc::vector<mio::path_t> large;      // 128 x 64 KiB
  i32 lat_fd = -1, seq_fd = -1, seqw_fd = -1;
  bool ok = false;
};

const usize WHOLE_SZ[3] = { 1024, 65536, 1048576 };

mio::path_t
mk_path(const char *stem, i32 idx) noexcept
{
  char buf[256];
  usize i = 0;
  for ( const char *p = g_dir; *p; ++p ) buf[i++] = *p;
  buf[i++] = '/';
  for ( const char *p = stem; *p; ++p ) buf[i++] = *p;
  if ( idx >= 0 ) {
    char num[16];
    i32 j = 15;
    num[j--] = '\0';
    if ( idx == 0 ) num[j--] = '0';
    for ( i32 x = idx; x > 0; x /= 10 ) num[j--] = static_cast<char>('0' + (x % 10));
    for ( const char *p = &num[j + 1]; *p; ++p ) buf[i++] = *p;
  }
  buf[i++] = '.';
  buf[i++] = 'b';
  buf[i++] = 'i';
  buf[i++] = 'n';
  buf[i] = '\0';
  return mio::path_t(buf);
}

// write `bytes` of pseudo-random content via raw posix (never through flash -- fixture setup must
// not perturb the ring counters or the page-cache state we are about to measure)
bool
stage(const mio::path_t &p, usize bytes) noexcept
{
  i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_wronly | px::o_create | px::o_trunc, px::mode_file));
  if ( fd < 0 ) return false;
  micron::buffer b(bytes ? bytes : 1);
  u64 rng = 0x5eed ^ static_cast<u64>(bytes);
  byte *d = b.data();
  for ( usize i = 0; i + 8 <= bytes; i += 8 ) {
    const u64 v = splitmix64(rng);
    for ( usize k = 0; k < 8; ++k ) d[i + k] = static_cast<byte>((v >> (k * 8)) & 0xff);
  }
  usize done = 0;
  while ( done < bytes ) {
    const max_t w = px::pwrite(fd, d + done, bytes - done, static_cast<px::off64_t>(done));
    if ( w <= 0 ) break;
    done += static_cast<usize>(w);
  }
  px::close(fd);
  return done == bytes;
}

// best-effort page-cache eviction for --cold. fadv_dontneed drops only clean, unreferenced pages --
// it is NOT drop_caches, which needs root. issued through posix so it never perturbs ring counters.
// the ~3 syscalls it costs are inside the timed region; against a cold NVMe read (~100 us) that is
// ~2%, and it is charged identically to every layer under comparison.
[[gnu::always_inline]] inline void
evict(const mio::path_t &p) noexcept
{
  if ( !g_cold ) return;
  const i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_rdonly, 0u));
  if ( fd < 0 ) return;
  px::fadvise(fd, 0, 0, px::fadv_dontneed);
  px::close(fd);
}

bool
setup(fixture &fx) noexcept
{
  px::mkdir(g_dir, 0755u);

  fx.lat = mk_path("lat", -1);
  fx.seq_r = mk_path("seqr", -1);
  fx.seq_w = mk_path("seqw", -1);
  fx.churn = mk_path("churn", -1);
  for ( i32 i = 0; i < 3; ++i ) {
    fx.whole_r[i] = mk_path("wr", i);
    fx.whole_w[i] = mk_path("ww", i);
  }

  if ( !stage(fx.lat, LAT_SZ) ) return false;
  if ( !stage(fx.seq_r, SEQ_SZ) ) return false;
  if ( !stage(fx.seq_w, SEQ_SZ) ) return false;
  if ( !stage(fx.churn, 4096) ) return false;
  for ( i32 i = 0; i < 3; ++i )
    if ( !stage(fx.whole_r[i], WHOLE_SZ[i]) ) return false;

  fx.small.reserve(BATCH_MAX);
  for ( u32 i = 0; i < BATCH_MAX; ++i ) {
    mio::path_t p = mk_path("s", static_cast<i32>(i));
    if ( !stage(p, BATCH_SMALL) ) return false;
    fx.small.push_back(p);
  }
  fx.large.reserve(128);
  for ( u32 i = 0; i < 128; ++i ) {
    mio::path_t p = mk_path("l", static_cast<i32>(i));
    if ( !stage(p, BATCH_LARGE) ) return false;
    fx.large.push_back(p);
  }

  fx.lat_fd = static_cast<i32>(px::openat(px::at_fdcwd, fx.lat.c_str(), px::o_rdwr, 0u));
  fx.seq_fd = static_cast<i32>(px::openat(px::at_fdcwd, fx.seq_r.c_str(), px::o_rdonly, 0u));
  fx.seqw_fd = static_cast<i32>(px::openat(px::at_fdcwd, fx.seq_w.c_str(), px::o_rdwr, 0u));
  if ( fx.lat_fd < 0 || fx.seq_fd < 0 || fx.seqw_fd < 0 ) return false;

  fx.ok = true;
  return true;
}

void
cleanup(fixture &fx) noexcept
{
  if ( fx.lat_fd >= 0 ) px::close(fx.lat_fd);
  if ( fx.seq_fd >= 0 ) px::close(fx.seq_fd);
  if ( fx.seqw_fd >= 0 ) px::close(fx.seqw_fd);
  px::unlink(fx.lat.c_str());
  px::unlink(fx.seq_r.c_str());
  px::unlink(fx.seq_w.c_str());
  px::unlink(fx.churn.c_str());
  for ( i32 i = 0; i < 3; ++i ) {
    px::unlink(fx.whole_r[i].c_str());
    px::unlink(fx.whole_w[i].c_str());
  }
  for ( usize i = 0; i < fx.small.size(); ++i ) px::unlink(fx.small[i].c_str());
  for ( usize i = 0; i < fx.large.size(); ++i ) px::unlink(fx.large[i].c_str());
  px::rmdir(g_dir);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group A -- per-op latency floor

void
bench_latency(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- A. per-op latency floor (one ring round-trip vs one syscall) --");
  constexpr u64 IT = 4000;
  const i32 fd = fx.lat_fd;

  // fadvise(NORMAL) is the true floor: cheapest possible sqe, kernel does essentially nothing.
  const rec pf = measure_plain([&]() -> u64 { return static_cast<u64>(px::fadvise(fd, 0, 0, px::fadv_normal) + 1); }, IT, 1, 0);
  report("posix/oneshot", "fadvise(normal)", pf, 0.0);
  const rec ff = measure_counted([&]() -> u64 { return static_cast<u64>(mf::fadvise(fd, 0, 0, px::fadv_normal) + 1); }, IT, 1, 0, eng);
  report("flash/oneshot", "fadvise(normal)", ff, pf.ns);

  const rec ps = measure_plain([&]() -> u64 { return static_cast<u64>(px::fsync(fd) + 1); }, IT, 1, 0);
  report("posix/oneshot", "fsync", ps, 0.0);
  const rec fs = measure_counted([&]() -> u64 { return static_cast<u64>(mf::fsync(fd) + 1); }, IT, 1, 0, eng);
  report("flash/oneshot", "fsync", fs, ps.ns);

  {
    px::statx_t sx{};
    const rec pt = measure_plain(
        [&]() -> u64 { return static_cast<u64>(px::statx(px::at_fdcwd, fx.lat.c_str(), 0, px::statx_basic_stats, sx) + 1); }, IT, 1, 0);
    report("posix/oneshot", "statx(path)", pt, 0.0);
    const rec ft = measure_counted([&]() -> u64 { return static_cast<u64>(mf::statx(fd, sx) + 1); }, IT, 1, 0, eng);
    report("flash/oneshot", "statx(fd)", ft, pt.ns);
  }

  {
    byte b[8];
    const rec pr = measure_plain([&]() -> u64 { return static_cast<u64>(px::pread(fd, b, 4, 0) + 1); }, IT, 1, 4);
    report("posix/rw", "pread 4B", pr, 0.0);
    const rec fr = measure_counted([&]() -> u64 { return static_cast<u64>(mf::pread(fd, b, 4, 0) + 1); }, IT, 1, 4, eng);
    report("flash/rw", "pread 4B", fr, pr.ns);
    // same call with the engine hoisted: isolates the [[gnu::noinline]] default_engine() TLS resolve
    const rec fh = measure_counted([&]() -> u64 { return static_cast<u64>(mf::pread(fd, b, 4, 0, eng) + 1); }, IT, 1, 4, eng);
    report("flash/rw", "pread 4B (eng hoisted)", fh, pr.ns);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group G -- open/close churn + the sstr control

void
bench_open(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- G. open/close churn (both layers embed sstr<4096>; ctrl rows isolate it) --");
  constexpr u64 IT = 2000;

  const rec pr = measure_plain(
      [&]() -> u64 {
        i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, fx.churn.c_str(), px::o_rdonly, 0u));
        if ( fd >= 0 ) px::close(fd);
        return static_cast<u64>(fd + 1);
      },
      IT, 1, 0);
  report("posix/open", "raw openat+close", pr, 0.0);

  const rec pf = measure_plain(
      [&]() -> u64 {
        mio::file f(fx.churn.c_str(), mio::modes::read);
        return static_cast<u64>(f.raw_fd() + 1);
      },
      IT, 1, 0);
  report("posix/open", "io::file", pf, pr.ns);

  const rec fo = measure_counted(
      [&]() -> u64 {
        mf::file f = mf::open_file(fx.churn, mio::modes::read);
        return static_cast<u64>(f.raw_fd() + 1);
      },
      IT, 1, 0, eng);
  report("flash/open", "open_file+close", fo, pr.ns);

  // controls: what fraction of the above is pure memset?
  {
    const char *cs = fx.churn.c_str();
    const rec c1 = measure_plain(
        [&]() -> u64 {
          mc::sstr<4096> s(cs);
          return static_cast<u64>(s.size());
        },
        IT, 1, 0);
    report("ctrl/sstr", "sstr<4096>(cstr)", c1, 0.0);
    const rec c2 = measure_plain(
        [&]() -> u64 {
          mc::sstr<128> s(cs);
          return static_cast<u64>(s.size());
        },
        IT, 1, 0);
    report("ctrl/sstr", "sstr<128>(cstr)", c2, 0.0);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group C -- whole-file porcelain

void
bench_porcelain(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- C. whole-file porcelain (sqe column IS the finding) --");
  const u64 its[3] = { 1500, 600, 120 };

  for ( i32 i = 0; i < 3; ++i ) {
    const usize sz = WHOLE_SZ[i];
    const u64 it = its[i];

    const rec pr = measure_plain(
        [&]() -> u64 {
          evict(fx.whole_r[i]);
          auto r = mio::read_file<mc::string>(fx.whole_r[i]);
          return r.is_first() ? static_cast<u64>(r.template cast<mc::string>().size()) : 1u;
        },
        it, 1, sz);
    report("posix/whole", "read_file", pr, 0.0);

    const rec fr = measure_counted(
        [&]() -> u64 {
          evict(fx.whole_r[i]);
          auto r = mf::read_file<mc::string>(fx.whole_r[i]);
          return r.is_first() ? static_cast<u64>(r.template cast<mc::string>().size()) : 1u;
        },
        it, 1, sz, eng);
    report("flash/whole", "read_file", fr, pr.ns);

    {
      mc::string target;
      const rec ft = measure_counted([&]() -> u64 { return static_cast<u64>(mf::read_file(fx.whole_r[i], target) + 1); }, it, 1, sz, eng);
      report("flash/whole", "read_file(p,target)", ft, pr.ns);
    }

    {
      mc::string payload(sz, 'z');
      const rec pw = measure_plain([&]() -> u64 { return static_cast<u64>(mio::write_file(fx.whole_w[i], payload) + 1); }, it, 1, sz);
      report("posix/whole", "write_file", pw, 0.0);
      const rec fw = measure_counted([&]() -> u64 { return static_cast<u64>(mf::write_file(fx.whole_w[i], payload) + 1); }, it, 1, sz, eng);
      report("flash/whole", "write_file", fw, pw.ns);
      const rec fws
          = measure_counted([&]() -> u64 { return static_cast<u64>(mf::write_file_sync(fx.whole_w[i], payload) + 1); }, it, 1, sz, eng);
      report("flash/whole", "write_file_sync", fws, pw.ns);
    }
  }

  // the control: identical work (open->read->close) in ONE submission. size known up front, so it
  // skips the statx -- fair, since that statx is one of read_file's four enters.
  if ( eng.level() == mf::tier::fixed ) {
    const usize sz = WHOLE_SZ[0];
    micron::buffer b(sz);
    const rec fc = measure_counted(
        [&]() -> u64 {
          mf::chain ch(eng);
          ch.open(fx.whole_r[0].c_str(), px::o_rdonly).read(b.data(), static_cast<u32>(sz), 0).close();
          const i32 rc = ch.run();
          return static_cast<u64>(rc + 1) + static_cast<u64>(b.data()[0]);
        },
        1500, 1, sz, eng, 1 /* chain::run's submit_wait is not a next_seq bump */);
    report("flash/chain", "open->read->close", fc, 0.0);
  } else {
    note("  (chain control skipped: engine is below tier::fixed)\n");
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group D -- batch. the money shot.

void
bench_batch(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- D. batch: read_files(N) vs a serial loop (same layer, same ring) --");
  const u32 Ns[4] = { 8, 32, 128, 512 };
  const u64 its[4] = { 120, 40, 12, 4 };
  const u32 cap = eng.ring().__sq_entries > 8 ? eng.ring().__sq_entries - 4 : 4;

  for ( u32 t = 0; t < 4; ++t ) {
    const u32 N = Ns[t];
    const u64 it = its[t];
    const usize payload = static_cast<usize>(N) * BATCH_SMALL;

    mc::vector<mio::path_t> slice;
    slice.reserve(N);
    for ( u32 i = 0; i < N; ++i ) slice.push_back(fx.small[i]);

    // wave 1 issues ceil(N/cap) submit_waits that never bump next_seq
    const u64 wave1 = static_cast<u64>((N + cap - 1) / cap);

    const rec ps = measure_plain(
        [&]() -> u64 {
          u64 acc = 0;
          for ( u32 i = 0; i < N; ++i ) {
            evict(slice[i]);
            auto r = mio::read_file<mc::string>(slice[i]);
            if ( r.is_first() ) acc += r.template cast<mc::string>().size();
          }
          return acc;
        },
        it, N, payload);
    report("posix/batch", "serial read_file xN", ps, 0.0);

    const rec fserial = measure_counted(
        [&]() -> u64 {
          u64 acc = 0;
          for ( u32 i = 0; i < N; ++i ) {
            evict(slice[i]);
            auto r = mf::read_file<mc::string>(slice[i], eng);
            if ( r.is_first() ) acc += r.template cast<mc::string>().size();
          }
          return acc;
        },
        it, N, payload, eng);
    report("flash/batch", "serial read_file xN", fserial, ps.ns);

    const rec fbatch = measure_counted(
        [&]() -> u64 {
          for ( u32 i = 0; i < N; ++i ) evict(slice[i]);
          auto v = mf::read_files<mc::string>(slice, eng);
          u64 acc = 0;
          for ( usize i = 0; i < v.size(); ++i )
            if ( v[i].is_first() ) acc += v[i].template cast<mc::string>().size();
          return acc;
        },
        it, N, payload, eng, wave1);
    report("flash/batch", "read_files(N)", fbatch, ps.ns);

    // the contrast that proves it is the code and not the kernel: same ring, same machine, but
    // stat_many really does batch.
    const rec pstat = measure_plain(
        [&]() -> u64 {
          u64 acc = 0;
          px::statx_t sx{};
          for ( u32 i = 0; i < N; ++i )
            acc += static_cast<u64>(px::statx(px::at_fdcwd, slice[i].c_str(), 0, px::statx_basic_stats, sx) + 1);
          return acc;
        },
        it, N, 0);
    report("posix/batch", "serial statx xN", pstat, 0.0);

    const rec fstat_s = measure_counted(
        [&]() -> u64 {
          u64 acc = 0;
          for ( u32 i = 0; i < N; ++i ) {
            auto r = mf::stat(slice[i], eng);
            acc += r.is_first() ? 1u : 0u;
          }
          return acc;
        },
        it, N, 0, eng);
    report("flash/batch", "serial stat xN", fstat_s, pstat.ns);

    const rec fstat_b = measure_counted(
        [&]() -> u64 {
          auto v = mf::stat_many(slice, eng);
          u64 acc = 0;
          for ( usize i = 0; i < v.size(); ++i ) acc += v[i].is_first() ? 1u : 0u;
          return acc;
        },
        it, N, 0, eng, wave1);
    report("flash/batch", "stat_many(N)", fstat_b, pstat.ns);

    if ( !g_csv && t + 1 < 4 ) emit_str("\n");
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group E -- what the ring can actually do

void
bench_queue(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- E. queue-layer ceiling (hand-rolled queue_read -> submit_wait -> drain) --");
  const u32 QDs[5] = { 1, 4, 16, 64, 128 };
  const usize blocks[2] = { 4096, 65536 };
  const i32 fd = fx.seq_fd;
  micron::buffer io(QUEUE_BUF);

  for ( u32 b = 0; b < 2; ++b ) {
    const usize blk = blocks[b];
    for ( u32 q = 0; q < 5; ++q ) {
      const u32 QD = QDs[q];
      if ( static_cast<usize>(QD) * blk > QUEUE_BUF ) continue;
      const u64 it = blk <= 4096 ? (QD >= 64 ? 400u : 1200u) : (QD >= 64 ? 60u : 300u);

      const rec r = measure_counted(
          [&]() -> u64 {
            byte *base = io.data();
            for ( u32 i = 0; i < QD; ++i )
              (void)mf::queue_read(eng, fd, base + static_cast<usize>(i) * blk, static_cast<u32>(blk), static_cast<u64>(i) * blk,
                                   static_cast<u64>(i));
            (void)mf::submit_wait(eng, QD);
            u64 acc = 0;
            (void)mf::drain(eng, QD, [&](u64, i32 res) { acc += static_cast<u64>(res); });
            return acc;
          },
          it, QD, static_cast<usize>(QD) * blk, eng, 1 /* the bench's own submit_wait */);
      // report per-op, not per-iteration, so the QD rows are comparable
      rec per = r;
      per.ns = r.ns / static_cast<f64>(QD);
      per.bytes = blk;
      char nm[40];
      usize k = 0;
      const char *pfx = blk == 4096 ? "QD=" : "QD=";
      for ( const char *p = pfx; *p; ++p ) nm[k++] = *p;
      if ( QD >= 100 ) nm[k++] = static_cast<char>('0' + (QD / 100) % 10);
      if ( QD >= 10 ) nm[k++] = static_cast<char>('0' + (QD / 10) % 10);
      nm[k++] = static_cast<char>('0' + QD % 10);
      for ( const char *p = blk == 4096 ? " x 4K" : " x 64K"; *p; ++p ) nm[k++] = *p;
      nm[k] = '\0';
      report("flash/queue", nm, per, 0.0);
    }
    if ( !g_csv && b == 0 ) emit_str("\n");
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group B -- sequential throughput sweep

void
bench_seq(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- B. sequential sweep (where the ring tax amortizes) --");
  const usize sizes[6] = { 4096, 16384, 65536, 262144, 1048576, 4194304 };
  const i32 rfd = fx.seq_fd;
  const i32 wfd = fx.seqw_fd;
  micron::buffer b(4u << 20);
  micron::buffer src(4u << 20);
  src.data()[0] = static_cast<byte>(0x5a);

  for ( u32 i = 0; i < 6; ++i ) {
    const usize sz = sizes[i];
    const u64 it = sz <= 65536 ? 1500u : (sz <= 262144 ? 600u : (sz <= 1048576 ? 300u : 80u));
    const u64 nblk = SEQ_SZ / sz;
    u64 turn = 0;

    const rec mc_ = measure_plain(
        [&]() -> u64 {
          micron::memcpy(b.data(), src.data(), sz);
          return static_cast<u64>(b.data()[0]);
        },
        it, 1, sz);
    report("ctrl/mem", "memcpy", mc_, 0.0);

    turn = 0;
    const rec pr = measure_plain(
        [&]() -> u64 {
          const u64 off = (turn++ % nblk) * sz;
          return static_cast<u64>(px::pread(rfd, b.data(), sz, static_cast<px::off64_t>(off)) + 1);
        },
        it, 1, sz);
    report("posix/rw", "pread", pr, mc_.ns);

    turn = 0;
    const rec fr = measure_counted(
        [&]() -> u64 {
          const u64 off = (turn++ % nblk) * sz;
          return static_cast<u64>(mf::pread(rfd, b.data(), sz, off) + 1);
        },
        it, 1, sz, eng);
    report("flash/rw", "pread", fr, pr.ns);

    turn = 0;
    const rec pw = measure_plain(
        [&]() -> u64 {
          const u64 off = (turn++ % nblk) * sz;
          return static_cast<u64>(px::pwrite(wfd, src.data(), sz, static_cast<px::off64_t>(off)) + 1);
        },
        it, 1, sz);
    report("posix/rw", "pwrite", pw, mc_.ns);

    turn = 0;
    const rec fw = measure_counted(
        [&]() -> u64 {
          const u64 off = (turn++ % nblk) * sz;
          return static_cast<u64>(mf::pwrite(wfd, src.data(), sz, off) + 1);
        },
        it, 1, sz, eng);
    report("flash/rw", "pwrite", fw, pw.ns);

    if ( !g_csv && i + 1 < 6 ) emit_str("\n");
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group F -- registered buffers

void
bench_fixed(fixture &fx, mf::engine &eng) noexcept
{
  hdr("-- F. registered (fixed) buffers vs plain --");
  if ( !eng.has_pool() ) {
    note("  (no fixed-buffer pool on this engine)\n");
    return;
  }
  const usize sizes[3] = { 4096, 65536, 262144 };
  const i32 fd = fx.seq_fd;
  micron::buffer heap(262144);

  for ( u32 i = 0; i < 3; ++i ) {
    const usize sz = sizes[i];
    if ( sz > eng.pool_slab_size() ) continue;
    const u64 it = sz <= 4096 ? 2000u : (sz <= 65536 ? 800u : 300u);

    const rec pl = measure_counted([&]() -> u64 { return static_cast<u64>(mf::pread(fd, heap.data(), sz, 0) + 1); }, it, 1, sz, eng);
    report("flash/rw", "pread (heap buf)", pl, 0.0);

    {
      micron::option<mf::pool_buf, micron::io::error_t> got = mf::acquire_buf(eng);
      mf::pool_buf pb = got.is_first() ? micron::move(got.cast<mf::pool_buf>()) : mf::pool_buf{};
      if ( !pb.valid() ) {
        note("  (pool exhausted)\n");
        continue;
      }
      const rec fx_ = measure_counted([&]() -> u64 { return static_cast<u64>(mf::read_fixed(fd, pb, sz, 0, eng) + 1); }, it, 1, sz, eng);
      report("flash/fixed", "read_fixed", fx_, pl.ns);
    }
  }

  // what does an engine cost to stand up? multiply by thread count.
  const rec ei = measure_plain(
      [&]() -> u64 {
        mf::engine e;
        (void)e.init();
        const u64 v = static_cast<u64>(e.level());
        e.shutdown();
        return v + 1;
      },
      30, 1, 0);
  report("flash/engine", "init+shutdown", ei, 0.0);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void
banner(mf::engine &eng) noexcept
{
  const char *tier = "none";
  switch ( eng.level() ) {
  case mf::tier::none:
    tier = "none";
    break;
  case mf::tier::plumbing:
    tier = "plumbing";
    break;
  case mf::tier::basic:
    tier = "basic";
    break;
  case mf::tier::fixed:
    tier = "fixed";
    break;
  }
  if ( g_csv ) {
    mio::println("# bench=io_flash tier=", tier, " sq_entries=", (u64)eng.ring().__sq_entries,
                 " cq_entries=", (u64)eng.ring().__cq_entries);
    mio::println("# k=", (u64)g_reps, " warmup=", (u64)WARMUP_REPS, " dir=", g_dir, " cold=", g_cold ? 1 : 0);
    mio::println("layer,case,n,bytes,ns_per_op,gib_s,sqe,cqe,rt,x_base");
    return;
  }
  emit_str("=== MICRON io::flash -- latency/throughput (median-of-k, wall-ns) ===\n");
  line l;
  l.s("tier=");
  l.s(tier);
  l.s("  sq_entries=");
  l.u_at(static_cast<u64>(eng.ring().__sq_entries), l.pos + 4);
  l.s("  dir=");
  l.s(g_dir);
  l.emit();
  emit_str("sqe = submission entries consumed; rt = ring round-trips (io_uring_enter).\n");
  emit_str("sqe/rt == 1.00 means every op paid its own syscall.  '!' = cqe != sqe.\n\n");
  emit_str("layer               case                                 n        bytes    ns/op   GiB/s   sqe    rt   x-base\n");
  emit_str("-----------------------------------------------------------------------------------------------------------\n");
}

bool
prefix_eq(const char *s, const char *p) noexcept
{
  while ( *p )
    if ( *s++ != *p++ ) return false;
  return true;
}

bool
str_eq(const char *a, const char *b) noexcept
{
  while ( *a && *b )
    if ( *a++ != *b++ ) return false;
  return *a == *b;
}

u32
parse_only(const char *v) noexcept
{
  if ( str_eq(v, "latency") ) return G_LATENCY;
  if ( str_eq(v, "open") ) return G_OPEN;
  if ( str_eq(v, "porcelain") ) return G_PORCELAIN;
  if ( str_eq(v, "batch") ) return G_BATCH;
  if ( str_eq(v, "queue") ) return G_QUEUE;
  if ( str_eq(v, "seq") ) return G_SEQ;
  if ( str_eq(v, "fixed") ) return G_FIXED;
  return 0xffffffffu;
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    const char *a = argv[i];
    if ( str_eq(a, "--csv") )
      g_csv = true;
    else if ( str_eq(a, "--cold") )
      g_cold = true;
    else if ( str_eq(a, "--tmpfs") )
      g_tmpfs = true;
    else if ( prefix_eq(a, "--only=") )
      g_only = parse_only(a + 7);
    else if ( prefix_eq(a, "--reps=") ) {
      u32 v = 0;
      for ( const char *p = a + 7; *p >= '0' && *p <= '9'; ++p ) v = v * 10 + static_cast<u32>(*p - '0');
      if ( v > 0 ) g_reps = v;
    }
  }
  if ( g_tmpfs ) g_dir = DIR_TMPFS;

  mf::engine &eng = mf::default_engine();
  if ( !eng.live() || eng.level() < mf::tier::basic ) {
    emit_str("io_flash_bench: SKIP (io_uring unavailable or tier < basic)\n");
    return 0;
  }

  banner(eng);

  fixture fx;
  if ( !setup(fx) ) {
    emit_str("io_flash_bench: fixture setup FAILED (disk full?)\n");
    cleanup(fx);
    return 1;
  }

  try {
    if ( g_only & G_LATENCY ) bench_latency(fx, eng);
    if ( g_only & G_OPEN ) bench_open(fx, eng);
    if ( g_only & G_PORCELAIN ) bench_porcelain(fx, eng);
    if ( g_only & G_BATCH ) bench_batch(fx, eng);
    if ( g_only & G_QUEUE ) bench_queue(fx, eng);
    if ( g_only & G_SEQ ) bench_seq(fx, eng);
    if ( g_only & G_FIXED ) bench_fixed(fx, eng);
  } catch ( ... ) {
    emit_str("  (aborted: an io op threw)\n");
  }

  cleanup(fx);
  mio::fflush(mio::stdout);
  if ( g_sink == 0xffffffffffffffffULL ) mio::println("");
  return 0;
}
