//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/bin.hpp"
#include "../src/memory_block.hpp"
#include "../src/strings.hpp"
#include "../src/vector/vector.hpp"

namespace mio = micron::io;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

constexpr usize FILE_SZ = 16u * 1024u * 1024u;
constexpr usize WINDOW_SZ = 64u * 1024u;
constexpr usize MiB = 1024u * 1024u;
const char *const PATH = "/dev/shm/mc_io_binary_bench.bin";

constexpr u64 SEARCH_ITERS = 200;
constexpr u64 FIND_ALL_ITERS = 6;
constexpr u64 ANALYSE1_ITERS = 60;
constexpr u64 ANALYSE16_ITERS = 8;
constexpr u64 ENTROPY_ITERS = 8;
constexpr u64 READ_WIN_ITERS = 3000;
constexpr u64 READ_AT_ITERS = 400;
constexpr u64 SLICE_ITERS = 400;
constexpr u64 MEMCPY1_ITERS = 60;
constexpr u64 MEMCPY16_ITERS = 8;

bool g_csv = false;
volatile u64 g_sink = 0;

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
report(const char *type, const char *op, usize elems, usize payload, f64 ns, f64 base_ns) noexcept
{
  const f64 gibs = (ns > 0 && payload > 0) ? (static_cast<f64>(payload) / ns) / 1.073741824 : 0.0;
  const f64 ratio = (ns > 0 && base_ns > 0) ? ns / base_ns : 0.0;
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

void
bench_search(mio::binary<micron::string> &b, f64 mc_file_ns) noexcept
{
  const auto &cb = b;
  const mio::bin_range_t win = cb.as_range();
  const byte *hay = win.ptr;
  const usize hlen = win.len;

  const usize patlens[] = { 4, 16, 256 };
  for ( usize pl : patlens ) {
    byte miss[256];
    byte hit[256];
    for ( usize i = 0; i < pl; ++i ) {
      miss[i] = static_cast<byte>(0xAB);
      hit[i] = hay[i];
    }

    const f64 mc_ns = measure(
        [&, k = 0u]() mutable -> u64 {
          const usize s = (k++ & 63u);
          const byte *p = micron::memchr(hay + s, static_cast<byte>(miss[0]), hlen - 64u);
          return p ? static_cast<u64>(p - hay) + 1u : 0u;
        },
        SEARCH_ITERS);
    report("search", "memchr(b0)", pl, hlen, mc_ns, mc_ns);

    const f64 s_ns = measure(
        [&]() -> u64 {
          const mio::bin_match_t m = cb.search(miss, pl);
          return static_cast<u64>(m.found) + m.offset + m.length;
        },
        SEARCH_ITERS);
    report("search", "kmp(miss)", pl, hlen, s_ns, mc_ns);

    const f64 h_ns = measure(
        [&]() -> u64 {
          const mio::bin_match_t m = cb.search(hit, pl);
          return static_cast<u64>(m.found) + m.offset + m.length;
        },
        SEARCH_ITERS);
    report("search", "kmp(hit@0)", pl, pl, h_ns, mc_ns);

    const f64 fa_ns = measure(
        [&]() -> u64 {
          micron::vector<mio::bin_match_t> v = b.find_all(miss, pl);
          return static_cast<u64>(v.size()) + 1u;
        },
        FIND_ALL_ITERS);
    report("find_all", "kmp(file)", pl, FILE_SZ, fa_ns, mc_file_ns);
  }
}

void
bench_analyse(mio::binary<micron::string> &b, micron::buffer &src, usize len, f64 base_ns, u64 iters) noexcept
{
  const auto &cb = b;
  byte *p = src.begin();
  const f64 a_ns = measure(
      [&, k = 0u]() mutable -> u64 {
        p[(k++) & 0xffffu] ^= 1u;
        const mio::bin_stats_t st = cb.analyse(mio::bin_range_t{ p, len });
        return st.total_bytes + static_cast<u64>(st.entropy * 1000.0) + st.freq[0];
      },
      iters);
  report("analyse", "histogram", 0, len, a_ns, base_ns);
}

void
bench_file_entropy(mio::binary<micron::string> &b, f64 base_ns) noexcept
{
  const f64 e_ns = measure(
      [&]() -> u64 {
        const double h = b.file_entropy();
        return static_cast<u64>(h * 1000000.0) + 1u;
      },
      ENTROPY_ITERS);
  report("entropy", "file(pread)", 0, FILE_SZ, e_ns, base_ns);
}

void
bench_reads(mio::binary<micron::string> &b) noexcept
{
  const auto &cb = b;

  const f64 w16le = measure([&, k = 0u]() mutable -> u64 { return cb.read_u16le((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);
  const f64 w16be = measure([&, k = 0u]() mutable -> u64 { return cb.read_u16be((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);
  const f64 w32le = measure([&, k = 0u]() mutable -> u64 { return cb.read_u32le((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);
  const f64 w32be = measure([&, k = 0u]() mutable -> u64 { return cb.read_u32be((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);
  const f64 w64le = measure([&, k = 0u]() mutable -> u64 { return cb.read_u64le((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);
  const f64 w64be = measure([&, k = 0u]() mutable -> u64 { return cb.read_u64be((k++ * 8u) & (WINDOW_SZ - 64u)); }, READ_WIN_ITERS);

  report("read_win", "u16le", 2, 0, w16le, w16le);
  report("read_win", "u16be", 2, 0, w16be, w16le);
  report("read_win", "u32le", 4, 0, w32le, w32le);
  report("read_win", "u32be", 4, 0, w32be, w32le);
  report("read_win", "u64le", 8, 0, w64le, w64le);
  report("read_win", "u64be", 8, 0, w64be, w64le);

  const f64 a16le = measure(
      [&, k = 0u]() mutable -> u64 { return cb.read_u16le_at(static_cast<micron::posix::off_t>((k++ * 4096u) & (FILE_SZ - 4096u))); },
      READ_AT_ITERS);
  const f64 a32le = measure(
      [&, k = 0u]() mutable -> u64 { return cb.read_u32le_at(static_cast<micron::posix::off_t>((k++ * 4096u) & (FILE_SZ - 4096u))); },
      READ_AT_ITERS);
  const f64 a64le = measure(
      [&, k = 0u]() mutable -> u64 { return cb.read_u64le_at(static_cast<micron::posix::off_t>((k++ * 4096u) & (FILE_SZ - 4096u))); },
      READ_AT_ITERS);

  report("read_at", "u16le_at", 2, 0, a16le, w16le);
  report("read_at", "u32le_at", 4, 0, a32le, w32le);
  report("read_at", "u64le_at", 8, 0, a64le, w64le);
}

void
bench_slice(mio::binary<micron::string> &b) noexcept
{
  const auto &cb = b;
  constexpr usize len = 4096;
  micron::buffer reuse(len);
  byte *rp = reuse.begin();

  const f64 t_ns = measure(
      [&, k = 0u]() mutable -> u64 {
        const micron::posix::off_t off = static_cast<micron::posix::off_t>((k++ * 4096u) & (FILE_SZ - 8192u));
        const max_t n = cb.try_slice(reuse, off, len);
        return static_cast<u64>(n) + rp[0];
      },
      SLICE_ITERS);
  report("slice", "try_slice", 0, len, t_ns, t_ns);

  const f64 s_ns = measure(
      [&, k = 0u]() mutable -> u64 {
        const micron::posix::off_t off = static_cast<micron::posix::off_t>((k++ * 4096u) & (FILE_SZ - 8192u));
        micron::buffer out = cb.slice(off, len);
        return static_cast<u64>(out.size()) + out.begin()[0];
      },
      SLICE_ITERS);
  report("slice", "slice(alloc)", 0, len, s_ns, t_ns);
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    micron::string a(argv[i]);
    if ( a == micron::string("--csv") ) g_csv = true;
  }

  micron::buffer data(FILE_SZ);
  {
    u64 rng = 0xB1A5C0DE5EED1234ULL;
    byte *d = data.begin();
    usize i = 0;
    for ( ; i + 8 <= FILE_SZ; i += 8 ) {
      const u64 r = splitmix64(rng);
      micron::memcpy(d + i, &r, 8);
    }
    for ( ; i < FILE_SZ; ++i ) d[i] = static_cast<byte>(splitmix64(rng));
  }
  {
    mio::binary<micron::string> w(PATH, mio::rwc, WINDOW_SZ);
    usize done = 0;
    while ( done < FILE_SZ ) {
      const max_t n = w.write_at(data.begin() + done, FILE_SZ - done, static_cast<micron::posix::off_t>(done));
      if ( n <= 0 ) break;
      done += static_cast<usize>(n);
    }
  }

  mio::binary<micron::string> b(PATH, mio::rd, WINDOW_SZ);
  b.fill(0);

  micron::buffer scratch(FILE_SZ);
  micron::buffer hist_src(FILE_SZ);
  micron::memcpy(hist_src.begin(), data.begin(), FILE_SZ);

  const f64 mc16_ns = measure(
      [&]() -> u64 {
        micron::memcpy(scratch.begin(), data.begin(), FILE_SZ);
        return scratch.begin()[0];
      },
      MEMCPY16_ITERS);
  const f64 mc1_ns = measure(
      [&]() -> u64 {
        micron::memcpy(scratch.begin(), data.begin(), MiB);
        return scratch.begin()[0];
      },
      MEMCPY1_ITERS);

  if ( g_csv ) {
    mio::println("# bench=io_binary");
    mio::println("# file=", (u64)FILE_SZ, " window=", (u64)WINDOW_SZ, " k=", (u64)K_MEASUREMENTS, " warmup=", (u64)WARMUP_REPS);
    mio::println("type,op,elems,bytes,ns_per_op,gib_s,x_base");
  } else {
    mio::println("=== MICRON io_v3 BINARY analysis (median-of-5) ===");
    mio::println("file=16MiB  window=64KiB  (/dev/shm)");
    mio::println("type                  op            elems       bytes   ns/op   GiB/s   x-bs");
    mio::println("--------------------------------------------------------------------------------");
  }

  mio::fflush(mio::stdout);

  report("memcpy", "raw(1MiB)", 0, MiB, mc1_ns, mc1_ns);
  report("memcpy", "raw(16MiB)", 0, FILE_SZ, mc16_ns, mc16_ns);
  if ( !g_csv ) micron::posix::write(1, "\n", 1);

  bench_search(b, mc16_ns);
  if ( !g_csv ) micron::posix::write(1, "\n", 1);

  bench_analyse(b, hist_src, MiB, mc1_ns, ANALYSE1_ITERS);
  bench_analyse(b, hist_src, FILE_SZ, mc16_ns, ANALYSE16_ITERS);
  bench_file_entropy(b, mc16_ns);
  if ( !g_csv ) micron::posix::write(1, "\n", 1);

  bench_reads(b);
  if ( !g_csv ) micron::posix::write(1, "\n", 1);

  bench_slice(b);

  mio::fflush(mio::stdout);
  micron::posix::unlink(PATH);

  if ( g_sink == 0xffffffffffffffffULL ) mio::println("");
  return 0;
}
