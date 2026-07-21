//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/echo.hpp"

#include "../src/maps/heap_swiss.hpp"
#include "../src/strings.hpp"
#include "../src/vector/vector.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

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

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
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
report(const char *type, const char *op, usize elems, usize payload, f64 ns, f64 raw_ns) noexcept
{
  const f64 gibs = (ns > 0) ? (static_cast<f64>(payload) / ns) / 1.073741824 : 0.0;
  const f64 ratio = (ns > 0 && raw_ns > 0) ? ns / raw_ns : 0.0;
  if ( g_csv ) {
    micron::io::println(type, ",", op, ",", elems, ",", payload, ",", to_fmt2(ns).whole, ".", to_fmt2(ns).frac, ",", to_fmt2(gibs).whole,
                        ".", to_fmt2(gibs).frac, ",", to_fmt2(ratio).whole, ".", to_fmt2(ratio).frac);
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

template<int SZ, int CK, typename FmtFn>
void
run_fmt_case(micron::io::stream_sink<SZ, CK> &sink, micron::io::stream<SZ, CK> &st, const char *type, usize elems, u64 iters,
             FmtFn &&fmt_fn) noexcept
{
  st.rewind();
  (void)fmt_fn();
  const usize len = static_cast<usize>(st.size());
  unsigned char pre[8192];
  micron::memcpy(pre, st.data(), len < sizeof(pre) ? len : sizeof(pre));

  const f64 raw_ns = measure(
      [&]() -> u64 {
        st.rewind();
        sink.put(reinterpret_cast<const char *>(pre), len);
        clobber(st.data());
        return static_cast<u64>(st.size()) ^ static_cast<u64>(st.data()[0]);
      },
      iters);
  report(type, "raw(put)", elems, len, raw_ns, raw_ns);

  const f64 fmt_ns = measure(
      [&]() -> u64 {
        st.rewind();
        max_t r = fmt_fn();
        clobber(st.data());
        return static_cast<u64>(r) ^ static_cast<u64>(st.data()[0]);
      },
      iters);
  report(type, "format", elems, len, fmt_ns, raw_ns);
}

void
bench_all(u64 iters_scalar, u64 iters_container) noexcept
{
  constexpr int SZ = 32768;
  constexpr int CK = 4096;
  micron::io::stream<SZ, CK> st;
  micron::io::stream_sink<SZ, CK> sink{ st };

  static volatile int vi = 123456;
  static volatile f64 vf = 3.14159265358979;

  micron::string str("hello, micron world");

  u64 rng = 0xC0FFEE;
  micron::vector<int> vec;
  vec.reserve(16);
  for ( int i = 0; i < 16; ++i ) vec.push_back(1000 + static_cast<int>(splitmix64(rng) % 9000));

  micron::hswiss<u64, u64> map;
  for ( u64 i = 1; i <= 16; ++i ) map.insert(i, i * 2654435761ULL);

  run_fmt_case(sink, st, "int", 1, iters_scalar, [&]() -> max_t {
    int x = vi;
    return micron::io::printk(sink, x);
  });

  run_fmt_case(sink, st, "f64(ryu)", 1, iters_scalar, [&]() -> max_t {
    f64 x = vf;
    return micron::io::printk(sink, x);
  });

  run_fmt_case(sink, st, "string", 1, iters_scalar, [&]() -> max_t {
    clobber(str.c_str());
    return micron::io::printk(sink, str);
  });

  run_fmt_case(sink, st, "vector<int>", 16, iters_container, [&]() -> max_t {
    clobber(vec.data());
    return micron::io::printk(sink, vec);
  });

  run_fmt_case(sink, st, "hswiss<u64,u64>", 16, iters_container, [&]() -> max_t {
    clobber(&map);
    return micron::io::printk(sink, map);
  });

  run_fmt_case(sink, st, "echo-concat", 6, iters_scalar, [&]() -> max_t {
    int a = vi;
    f64 b = vf;
    return micron::io::__echo_impl::run(sink, false, "id=", a, " v=", b, " ok=", true);
  });

  run_fmt_case(sink, st, "literal-heavy", 2, iters_scalar, [&]() -> max_t {
    int a = vi;
    int b = vi + 7;
    return micron::io::__echo_impl::format_to_sink(sink, "prefix {} middle {} suffix", a, b);
  });

  run_fmt_case(sink, st, "pad{:>40}", 1, iters_scalar, [&]() -> max_t {
    int a = static_cast<int>(vi % 100);
    return micron::io::__echo_impl::format_to_sink(sink, "{:>40}", a);
  });
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    micron::string a(argv[i]);
    if ( a == micron::string("--csv") ) g_csv = true;
  }

  if ( g_csv ) {
    micron::io::println("# bench=io_echo_fmt (io_v3 printk/echof -> memory sink)");
    micron::io::println("# k=", (u64)K_MEASUREMENTS, " warmup=", (u64)WARMUP_REPS);
    micron::io::println("type,op,elems,out_bytes,ns_per_op,gib_s,vs_raw");
  } else {
    micron::io::println("=== MICRON io_v3 ECHO/PRINTK FORMAT -> memory sink (median-of-5) ===");
    micron::io::println("type                  op            elems       bytes   ns/op   GiB/s   x-raw");
    micron::io::println("--------------------------------------------------------------------------------");
  }

  micron::io::fflush(micron::io::stdout);

  bench_all(50000, 10000);

  micron::io::fflush(micron::io::stdout);

  if ( g_sink == 0xffffffffffffffffULL ) micron::io::println("");
  return 0;
}
