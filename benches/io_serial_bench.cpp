//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/io/__serial_core.hpp"
#include "../src/memory_block.hpp"

#include "../src/list.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/strings.hpp"
#include "../src/vector/vector.hpp"

namespace ser = micron::io::serialize;

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

u64
iters_for(usize bytes) noexcept
{
  if ( bytes < 4096 ) return 4000;
  if ( bytes < 65536 ) return 800;
  return 150;
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

template<typename C>
void
run_case(const char *type, const C &fixture, usize elems, usize payload, u64 max_iters = ~0ull) noexcept
{
  const max_t need = ser::framed_size(fixture);
  if ( need <= 0 ) {
    micron::io::println("  (framed_size failed for ", type, ")");
    return;
  }
  const usize cap = static_cast<usize>(need);
  micron::buffer frame_buf(cap);
  micron::buffer copy_dst(cap);

  const max_t wrote = ser::frame_into(frame_buf.data(), cap, fixture);
  if ( wrote <= 0 ) {
    micron::io::println("  (frame_into failed for ", type, ")");
    return;
  }
  const usize framed = static_cast<usize>(wrote);
  u64 it = iters_for(framed);
  if ( it > max_iters ) it = max_iters;

  const f64 mc_ns = measure(
      [&]() -> u64 {
        micron::memcpy(copy_dst.data(), frame_buf.data(), framed);
        return static_cast<u64>(copy_dst.data()[0]);
      },
      it);
  report(type, "memcpy(raw)", elems, payload, mc_ns, mc_ns);

  const f64 fi_ns = measure([&]() -> u64 { return static_cast<u64>(ser::frame_into(frame_buf.data(), cap, fixture)); }, it);
  report(type, "frame_into", elems, payload, fi_ns, mc_ns);

  C out{};
  const f64 uf_ns = measure([&]() -> u64 { return static_cast<u64>(ser::unframe_from(frame_buf.data(), framed, out)); }, it);
  report(type, "unframe_from", elems, payload, uf_ns, mc_ns);
}

void
bench_vector_u32(usize n) noexcept
{
  u64 rng = 0x1234;
  micron::vector<u32> v;
  v.reserve(n);
  for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<u32>(splitmix64(rng)));
  run_case("vector<u32>", v, n, n * sizeof(u32));
}

void
bench_vector_f64(usize n) noexcept
{
  u64 rng = 0x5678;
  micron::vector<f64> v;
  v.reserve(n);
  for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<f64>(splitmix64(rng)) * 1e-6);
  run_case("vector<f64>", v, n, n * sizeof(f64));
}

void
bench_vector_string(usize n) noexcept
{
  u64 rng = 0x9abc;
  micron::vector<micron::string> v;
  v.reserve(n);
  usize bytes = 0;
  for ( usize i = 0; i < n; ++i ) {
    const usize len = 4 + (splitmix64(rng) % 28);
    micron::string s;
    for ( usize j = 0; j < len; ++j ) s.push_back(static_cast<char>('a' + (splitmix64(rng) % 26)));
    bytes += len;
    v.push_back(micron::move(s));
  }
  run_case("vector<string>", v, n, bytes, 120);
}

void
bench_list_i32(usize n) noexcept
{
  u64 rng = 0xdef0;
  micron::list<i32> l;
  for ( usize i = 0; i < n; ++i ) l.push_back(static_cast<i32>(splitmix64(rng)));
  run_case("list<i32>", l, n, n * sizeof(i32), 120);
}

void
bench_hswiss(usize n) noexcept
{
  micron::hswiss<u64, u64> m;
  for ( u64 i = 1; i <= n; ++i ) m.insert(i, i * 2654435761ULL);
  run_case("hswiss<u64,u64>", m, n, n * 2 * sizeof(u64), 120);
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
    micron::io::println("# bench=io_serial (MFR1)");
    micron::io::println("# k=", (u64)K_MEASUREMENTS, " warmup=", (u64)WARMUP_REPS);
    micron::io::println("type,op,elems,framed_bytes,ns_per_op,gib_s,vs_memcpy");
  } else {
    micron::io::println("=== MICRON io_v3 MFR1 SERIALIZE (median-of-5) ===");
    micron::io::println("type                  op            elems       bytes   ns/op   GiB/s   x-mc");
    micron::io::println("--------------------------------------------------------------------------------");
  }

  const usize blit_sizes[] = { 256, 4096, 65536 };
  for ( usize s : blit_sizes ) {
    bench_vector_u32(s);
    bench_vector_f64(s);
  }
  if ( !g_csv ) micron::io::println("");

  const usize node_sizes[] = { 256, 4096 };
  for ( usize s : node_sizes ) {
    bench_vector_string(s);
    bench_list_i32(s);
    bench_hswiss(s);
  }

  micron::io::fflush(micron::io::stdout);

  if ( g_sink == 0xffffffffffffffffULL ) micron::io::println("");
  return 0;
}
