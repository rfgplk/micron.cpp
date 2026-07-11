//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"

#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

extern "C" {
void *memcpy(void *, const void *, __SIZE_TYPE__) noexcept;
void *memmove(void *, const void *, __SIZE_TYPE__) noexcept;
void *memset(void *, int, __SIZE_TYPE__) noexcept;
}

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u32 NUM_TESTS = 16384;
constexpr u32 SIZE_NUM = 65536;
constexpr u32 ALIGN_NUM = 1024;
constexpr u64 MAX_WS = 1ull << 20;
constexpr u64 SLACK = 8192;

static bool g_csv = false;
static volatile u64 g_vsink = 0;

alignas(4096) static u8 g_sbuf[MAX_WS + SLACK];
alignas(4096) static u8 g_dbuf[MAX_WS + SLACK];

struct freq_data {
  u16 size;
  u16 freq;
};

struct align_data {
  u8 align;
  u16 freq;
};

// SPEC2017 memcpy size distribution (gnu style)
static const freq_data size_freq[] = {
  { 32, 22320 }, { 16, 9554 }, { 8, 8915 },  { 152, 5327 }, { 4, 2159 },   { 292, 2035 }, { 12, 1608 }, { 24, 1343 }, { 1152, 895 },
  { 144, 813 },  { 884, 733 }, { 284, 721 }, { 120, 661 },  { 2, 649 },    { 882, 550 },  { 5, 475 },   { 7, 461 },   { 108, 460 },
  { 10, 361 },   { 9, 361 },   { 6, 334 },   { 3, 326 },    { 464, 308 },  { 2048, 303 }, { 1, 298 },   { 64, 250 },  { 11, 197 },
  { 296, 194 },  { 68, 187 },  { 15, 185 },  { 192, 184 },  { 1764, 183 }, { 13, 173 },   { 560, 126 }, { 160, 115 }, { 288, 96 },
  { 104, 96 },   { 1144, 83 }, { 18, 80 },   { 23, 78 },    { 40, 77 },    { 19, 68 },    { 48, 63 },   { 17, 57 },   { 72, 54 },
  { 1280, 51 },  { 20, 49 },   { 28, 47 },   { 22, 46 },    { 640, 45 },   { 25, 41 },    { 14, 40 },   { 56, 37 },   { 27, 35 },
  { 35, 33 },    { 384, 33 },  { 29, 32 },   { 80, 30 },    { 4095, 22 },  { 232, 22 },   { 36, 19 },   { 184, 17 },  { 21, 17 },
  { 256, 16 },   { 44, 15 },   { 26, 15 },   { 31, 14 },    { 88, 14 },    { 176, 13 },   { 33, 12 },   { 1024, 12 }, { 208, 11 },
  { 62, 11 },    { 128, 10 },  { 704, 10 },  { 324, 10 },   { 96, 10 },    { 60, 9 },     { 136, 9 },   { 124, 9 },   { 34, 8 },
  { 30, 8 },     { 480, 8 },   { 1344, 8 },  { 273, 7 },    { 520, 7 },    { 112, 6 },    { 52, 6 },    { 344, 6 },   { 336, 6 },
  { 504, 5 },    { 168, 5 },   { 424, 5 },   { 0, 4 },      { 76, 3 },     { 200, 3 },    { 512, 3 },   { 312, 3 },   { 240, 3 },
  { 960, 3 },    { 264, 2 },   { 672, 2 },   { 38, 2 },     { 328, 2 },    { 84, 2 },     { 39, 2 },    { 216, 2 },   { 42, 2 },
  { 37, 2 },     { 1608, 2 },  { 70, 2 },    { 46, 2 },     { 536, 2 },    { 280, 1 },    { 248, 1 },   { 47, 1 },    { 1088, 1 },
  { 1288, 1 },   { 224, 1 },   { 41, 1 },    { 50, 1 },     { 49, 1 },     { 808, 1 },    { 360, 1 },   { 440, 1 },   { 43, 1 },
  { 45, 1 },     { 78, 1 },    { 968, 1 },   { 392, 1 },    { 54, 1 },     { 53, 1 },     { 59, 1 },    { 376, 1 },   { 664, 1 },
  { 58, 1 },     { 272, 1 },   { 66, 1 },    { 2688, 1 },   { 472, 1 },    { 568, 1 },    { 720, 1 },   { 51, 1 },    { 63, 1 },
  { 86, 1 },     { 496, 1 },   { 776, 1 },   { 57, 1 },     { 680, 1 },    { 792, 1 },    { 122, 1 },   { 760, 1 },   { 824, 1 },
  { 552, 1 },    { 67, 1 },    { 456, 1 },   { 984, 1 },    { 74, 1 },     { 408, 1 },    { 75, 1 },    { 92, 1 },    { 576, 1 },
  { 116, 1 },    { 65, 1 },    { 117, 1 },   { 82, 1 },     { 352, 1 },    { 55, 1 },     { 100, 1 },   { 90, 1 },    { 696, 1 },
  { 111, 1 },    { 880, 1 },   { 79, 1 },    { 488, 1 },    { 61, 1 },     { 114, 1 },    { 94, 1 },    { 1032, 1 },  { 98, 1 },
  { 87, 1 },     { 584, 1 },   { 85, 1 },    { 648, 1 },    { 0, 0 },
};

static const align_data src_align_freq[] = { { 8, 300 }, { 16, 292 }, { 32, 168 }, { 64, 153 }, { 4, 79 }, { 2, 14 }, { 1, 18 }, { 0, 0 } };
static const align_data dst_align_freq[] = { { 8, 265 }, { 16, 263 }, { 64, 209 }, { 32, 174 }, { 4, 90 }, { 2, 10 }, { 1, 13 }, { 0, 0 } };

static u16 size_arr[SIZE_NUM];
static u8 src_align_arr[ALIGN_NUM];
static u8 dst_align_arr[ALIGN_NUM];

struct rop {
  u32 d_off;
  u32 s_off;
  u32 len;
  u32 src_in_dst;
};

static rop copy_arr[NUM_TESTS];
static rop move_arr[NUM_TESTS];

static u64 rng_state = 0x369bd65914cf0616ULL;

[[gnu::always_inline]] inline u64
xrand() noexcept
{
  u64 x = rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  rng_state = x;
  return x;
}

static void
init_distributions() noexcept
{
  u32 n = 0;
  for ( u32 i = 0; size_freq[i].freq != 0 && n < SIZE_NUM; ++i )
    for ( u32 j = 0; j < size_freq[i].freq && n < SIZE_NUM; ++j ) size_arr[n++] = size_freq[i].size;
  while ( n < SIZE_NUM ) size_arr[n++] = 32;
  n = 0;
  for ( u32 i = 0; src_align_freq[i].freq != 0 && n < ALIGN_NUM; ++i )
    for ( u32 j = 0; j < src_align_freq[i].freq && n < ALIGN_NUM; ++j ) src_align_arr[n++] = src_align_freq[i].align - 1;
  n = 0;
  for ( u32 i = 0; dst_align_freq[i].freq != 0 && n < ALIGN_NUM; ++i )
    for ( u32 j = 0; j < dst_align_freq[i].freq && n < ALIGN_NUM; ++j ) dst_align_arr[n++] = dst_align_freq[i].align - 1;
}

static u64
gen_ops(u64 ws) noexcept
{
  u64 total = 0;
  for ( u32 i = 0; i < NUM_TESTS; ++i ) {
    u32 d = static_cast<u32>(xrand() & (ws - 1));
    d &= ~static_cast<u32>(dst_align_arr[xrand() & (ALIGN_NUM - 1)]);
    u32 s = static_cast<u32>(xrand() & (ws - 1));
    s &= ~static_cast<u32>(src_align_arr[xrand() & (ALIGN_NUM - 1)]);
    const u32 len = size_arr[xrand() & (SIZE_NUM - 1)];
    copy_arr[i] = { d, s, len, 0 };

    rop m = copy_arr[i];
    if ( (xrand() & 3) == 0 && len > 1 ) {
      const u32 half = len / 2;
      m.s_off = (xrand() & 1) ? m.d_off + half : (m.d_off > half ? m.d_off - half : 0);
      m.src_in_dst = 1;
    }
    move_arr[i] = m;
    total += len;
  }
  return total;
}

[[gnu::always_inline]] inline u64
cycles() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  u32 lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
#elif defined(__micron_arch_arm64)
  u64 v;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#else
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
#endif
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

struct fmtd {
  u64 whole;
  u32 frac;
};

[[gnu::always_inline]] inline fmtd
to_fmt(f64 v, u32 scale) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * static_cast<f64>(scale) + 0.5);
  return { s / scale, static_cast<u32>(s % scale) };
}

struct line {
  char buf[320];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  u(u64 v) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else
      while ( v ) {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
      }
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f(fmtd fm, u32 digits) noexcept
  {
    u(fm.whole);
    buf[pos++] = '.';
    u32 div = 1;
    for ( u32 k = 1; k < digits; ++k ) div *= 10;
    for ( u32 k = 0; k < digits; ++k ) {
      buf[pos++] = '0' + static_cast<char>((fm.frac / div) % 10);
      div /= 10;
    }
  }

  void
  lj(const char *p, u32 end_col) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
    while ( pos < end_col ) buf[pos++] = ' ';
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

typedef void (*bfn)(const rop *ops, u32 n);

static void
b_noop(const rop *, u32)
{
}

static void
b_mc_memcpy(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) mc::memcpy(g_dbuf + ops[j].d_off, g_sbuf + ops[j].s_off, ops[j].len);
}

static void
b_libc_memcpy(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) ::memcpy(g_dbuf + ops[j].d_off, g_sbuf + ops[j].s_off, ops[j].len);
}

static void
b_mc_memset(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) mc::memset(g_dbuf + ops[j].d_off, static_cast<byte>(0), ops[j].len);
}

static void
b_libc_memset(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) ::memset(g_dbuf + ops[j].d_off, 0, ops[j].len);
}

static void
b_mc_memmove(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) {
    u8 *s = (ops[j].src_in_dst ? g_dbuf : g_sbuf) + ops[j].s_off;
    mc::memmove(g_dbuf + ops[j].d_off, s, ops[j].len);
  }
}

static void
b_libc_memmove(const rop *ops, u32 n)
{
  for ( u32 j = 0; j < n; ++j ) {
    const u8 *s = (ops[j].src_in_dst ? g_dbuf : g_sbuf) + ops[j].s_off;
    ::memmove(g_dbuf + ops[j].d_off, s, ops[j].len);
  }
}

static bfn volatile g_bf = nullptr;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::noinline]] static void
measure_batch(bfn volatile *pf, const rop *ops, u64 reps, f64 &ns_out, f64 &cyc_out) noexcept
{
  f64 ns_s[K_MEASUREMENTS], cyc_s[K_MEASUREMENTS];
  for ( u32 m = 0; m <= K_MEASUREMENTS; ++m ) {
    bfn f = *pf;
    const u64 t0 = now_ns(), c0 = cycles();
    for ( u64 r = 0; r < reps; ++r ) {
      f(ops, NUM_TESTS);
      clobber(g_dbuf);
    }
    const u64 c1 = cycles(), t1 = now_ns();
    if ( m == 0 ) continue;
    ns_s[m - 1] = static_cast<f64>(t1 - t0) / static_cast<f64>(reps);
    cyc_s[m - 1] = static_cast<f64>(c1 - c0) / static_cast<f64>(reps);
  }
  g_vsink ^= g_dbuf[0];
  ns_out = median_f64(ns_s, K_MEASUREMENTS);
  cyc_out = median_f64(cyc_s, K_MEASUREMENTS);
}

static void
cell(const char *op, const char *impl, bfn f, const rop *ops, u64 ws, u64 batch_bytes)
{
  u64 reps = (1ull << 25) / (batch_bytes ? batch_bytes : 1);
  if ( reps < 2 ) reps = 2;
  f64 bns, bcyc, ns, cyc;
  g_bf = b_noop;
  measure_batch(&g_bf, ops, reps, bns, bcyc);
  g_bf = f;
  measure_batch(&g_bf, ops, reps, ns, cyc);
  const f64 dcyc = cyc > bcyc ? cyc - bcyc : 0.0;
  const f64 dns = ns > bns ? ns - bns : 0.0;
  const f64 cyc_pb = dcyc / static_cast<f64>(batch_bytes);
  const f64 gibs = dns > 0.0 ? (static_cast<f64>(batch_bytes) * 1e9 / dns) / static_cast<f64>(1ull << 30) : 0.0;
  line ln;
  if ( g_csv ) {
    ln.s(op);
    ln.s(",");
    ln.s(impl);
    ln.s(",");
    ln.u(ws);
    ln.s(",");
    ln.s("-1,-1,rand,-1,");
    ln.f(to_fmt(cyc_pb, 10000), 4);
    ln.s(",");
    ln.f(to_fmt(gibs, 100), 2);
  } else {
    ln.lj(op, 14);
    ln.lj(impl, 24);
    ln.u(ws);
    ln.lj("", ln.pos < 34 ? 34 : ln.pos + 1);
    ln.f(to_fmt(cyc_pb, 10000), 4);
    ln.lj("", ln.pos < 46 ? 46 : ln.pos + 1);
    ln.f(to_fmt(gibs, 100), 2);
  }
  micron::io::println(ln.str());
}

static bool
streq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

static u64
parse_u(const char *p) noexcept
{
  u64 v = 0;
  while ( *p >= '0' && *p <= '9' ) v = v * 10 + static_cast<u64>(*p++ - '0');
  return v;
}

}      // namespace

int
main(int argc, char **argv)
{
  u32 cpu = 0;
  for ( int i = 1; i < argc; ++i ) {
    if ( streq(argv[i], "--csv") )
      g_csv = true;
    else if ( streq(argv[i], "--cpu") && i + 1 < argc )
      cpu = static_cast<u32>(parse_u(argv[++i]));
  }

  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(cpu);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  {
    volatile u64 spin = 0;
    const u64 t0 = now_ns();
    while ( now_ns() - t0 < 1500000000ULL )
      for ( u64 i = 0; i < 65536; ++i ) spin += i;
  }

  for ( u64 i = 0; i < sizeof(g_sbuf); ++i ) {
    g_sbuf[i] = static_cast<u8>(i * 0x9Eu + 0x37u);
    g_dbuf[i] = static_cast<u8>(i * 0x6Bu + 0x9Du);
  }
  init_distributions();

  if ( g_csv ) {
    micron::io::println("# bench=memory_random");
    micron::io::println("# batch=", (u64)NUM_TESTS, " k=", (u64)K_MEASUREMENTS, " cpu=", (u64)cpu);
    micron::io::println("op,impl,size,dst_align,src_align,mode,delta,cyc_per_byte,gib_s");
  } else {
    micron::io::println("=== MICRON MEMORY RANDOM (SPEC2017 distributions, batch=16384) ===");
    line h;
    h.lj("op", 14);
    h.lj("impl", 24);
    h.lj("ws", 34);
    h.lj("cyc/B", 46);
    h.lj("GiB/s", 54);
    micron::io::println(h.str());
    micron::io::println("------------------------------------------------------");
  }

  for ( u64 ws = 32ull << 10; ws <= MAX_WS; ws <<= 1 ) {
    const u64 total = gen_ops(ws);
    cell("memcpy_rand", "micron", b_mc_memcpy, copy_arr, ws, total);
    cell("memcpy_rand", "libc", b_libc_memcpy, copy_arr, ws, total);
    cell("memset_rand", "micron", b_mc_memset, copy_arr, ws, total);
    cell("memset_rand", "libc", b_libc_memset, copy_arr, ws, total);
    cell("memmove_rand", "micron", b_mc_memmove, move_arr, ws, total);
    cell("memmove_rand", "libc", b_libc_memmove, move_arr, ws, total);
  }

  if ( !g_csv ) micron::io::println("[done] sink=", (u64)g_vsink);
  return 0;
}

#pragma GCC pop_options
