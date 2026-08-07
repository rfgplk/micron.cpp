//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// build:  duck build benches/arbint_crossover_bench.cpp --perf --fp --no-ssp --no-lto -o bin/b
// run  :  taskset -c 2 ./bin/b/arbint_crossover_bench
// csv  :  taskset -c 2 ./bin/b/arbint_crossover_bench --csv > benches/results/arbint_crossover.csv

#if defined(ARBINT_BENCH_PERF)
#include "../external/bbench/bench.hpp"
#endif

#include "../src/chrono.hpp"
#include "../src/io/console.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/arbint.hpp"
#include "../src/std.hpp"

namespace
{

namespace mpn = micron::math::mpn;

#if defined(ARBINT_BENCH_PERF)
using arb_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
#endif

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;
constexpr u64 TARGET_LIMB_OPS = 1ull << 24;

constexpr usize MAX_N = 1024;

alignas(64) mpn::limb_t g_a[MAX_N];
alignas(64) mpn::limb_t g_b[MAX_N];
alignas(64) mpn::limb_t g_r[2 * MAX_N + 4];

alignas(64) mpn::limb_t g_sc[64 * MAX_N + 16384];

alignas(64) mpn::limb_t g_dn[MAX_N];
alignas(64) mpn::limb_t g_nn[2 * MAX_N + 4];
alignas(64) mpn::limb_t g_nw[2 * MAX_N + 4];
alignas(64) mpn::limb_t g_q[MAX_N + 4];
mpn::limb_t g_dinv = 0;

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ull + 1442695040888963407ull;
  return s;
}

[[gnu::cold]] void
fill(mpn::limb_t *p, usize n, u64 seed) noexcept
{
  for ( usize i = 0; i < n; ++i ) p[i] = static_cast<mpn::limb_t>(lcg_next(seed));
  if ( n ) p[n - 1u] |= mpn::limb_msb;
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

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : buf{}, pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else
      while ( v ) {
        tmp[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
      }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = static_cast<char>('0' + (w % 10));
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = static_cast<char>('0' + (f.frac_x100 / 10));
    buf[pos++] = static_cast<char>('0' + (f.frac_x100 % 10));
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

struct cell {
  f64 cyc_per_op;
  f64 cyc_per_n2;
  f64 ipc;
  f64 bmiss;
};

template<typename Kernel>
cell
measure(usize n, u64 reps, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cyc_s[K_MEASUREMENTS];
  f64 ipc_s[K_MEASUREMENTS];
  f64 bm_s[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
#if defined(ARBINT_BENCH_PERF)
    arb_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    evs.end();
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    ipc_s[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_s[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
#else

    asm volatile("" ::: "memory");
    const u64 t0 = micron::math::__asm_op::rdtsc64();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    asm volatile("" ::: "memory");
    const u64 cyc = micron::math::__asm_op::rdtsc64() - t0;
    ipc_s[m] = 0.0;
    bm_s[m] = 0.0;
#endif
    cyc_s[m] = static_cast<f64>(cyc) / static_cast<f64>(reps);
  }

  const f64 per_op = median_f64(cyc_s, K_MEASUREMENTS);
  const f64 n2 = static_cast<f64>(n) * static_cast<f64>(n);
  return cell{ per_op, per_op / n2, median_f64(ipc_s, K_MEASUREMENTS), median_f64(bm_s, K_MEASUREMENTS) };
}

[[nodiscard]] u64
reps_for_gcd(usize n) noexcept
{
  const u64 work = static_cast<u64>(n) * static_cast<u64>(n) * static_cast<u64>(mpn::limb_bits);
  u64 r = work ? TARGET_LIMB_OPS / work : 64;
  if ( r < 4 ) r = 4;
  if ( r > 4096 ) r = 4096;
  return r;
}

[[nodiscard]] u64
reps_for(usize n) noexcept
{
  const u64 work = static_cast<u64>(n) * static_cast<u64>(n);
  u64 r = work ? TARGET_LIMB_OPS / work : 64;
  if ( r < 16 ) r = 16;
  if ( r > (1ull << 20) ) r = 1ull << 20;
  return r;
}

constexpr usize SIZES[]
    = { 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 40, 48, 64, 96, 128, 160, 192, 256, 320, 384, 512, 640, 768, 896, 1024 };
constexpr usize N_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);

bool g_csv = false;

[[gnu::cold]] void
emit(const char *op, const char *tier, usize n, const cell &c)
{
  if ( g_csv ) {
    line l;
    l.s(op);
    l.s(",");
    l.s(tier);
    l.s(",");
    l.u_at(n, 0);
    l.s(",");
    l.f2_at(to_fmt2(c.cyc_per_op), 0);
    l.s(",");
    l.f2_at(to_fmt2(c.cyc_per_n2 * 100.0), 0);
    l.s(",");
    l.f2_at(to_fmt2(c.ipc), 0);
    l.s(",");
    l.f2_at(to_fmt2(c.bmiss * 100.0), 0);
    micron::io::println(l.str());
    return;
  }
  line l;
  l.s("  ");
  l.s_at(tier, 14);
  l.u_at(n, 22);
  l.f2_at(to_fmt2(c.cyc_per_op), 36);
  l.f2_at(to_fmt2(c.cyc_per_n2 * 100.0), 50);
  l.f2_at(to_fmt2(c.ipc), 58);
  l.f2_at(to_fmt2(c.bmiss * 100.0), 68);
  micron::io::println(l.str());
}

[[gnu::cold]] void
header(const char *title)
{
  if ( g_csv ) return;
  micron::io::println("");
  micron::io::println("[", title, "]");
  line h;
  h.s("  ");
  h.s_at("tier", 14);
  h.s_at("limbs", 22);
  h.s_at("cyc/op", 36);
  h.s_at("cyc/n^2 x100", 50);
  h.s_at("IPC", 58);
  h.s_at("bmiss%", 68);
  micron::io::println(h.str());
  micron::io::println("  ----------------------------------------------------------------------");
}

}      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    const char *a = argv[i];
    if ( a[0] == '-' && a[1] == '-' && a[2] == 'c' ) g_csv = true;
  }

  fill(g_a, MAX_N, 0x1234567ull);
  fill(g_b, MAX_N, 0x89ABCDEFull);

  if ( g_csv ) {
    micron::io::println("# arbint tier crossover. cyc_n2_x100 is (cycles / n^2) * 100 -- flat means quadratic.");
    micron::io::println("op,tier,limbs,cyc_per_op,cyc_n2_x100,ipc,bmiss_pct");
  } else {
    micron::io::println("=== arbint tier crossover ===");
    micron::io::println("limb width: ", static_cast<u64>(mpn::limb_bits), " bits    measurements: ", static_cast<u64>(K_MEASUREMENTS),
                        " (median)");
    micron::io::println("read cyc/n^2: a quadratic tier is flat, a subquadratic one falls. the crossover is where they cross.");
    micron::io::println("current thresholds (limbs) -- comba ", static_cast<u64>(mpn::threshold::mul_comba), ", karatsuba ",
                        static_cast<u64>(mpn::threshold::mul_karatsuba), ", toom3 ", static_cast<u64>(mpn::threshold::mul_toom3),
                        ", toom4 ", static_cast<u64>(mpn::threshold::mul_toom4), ", nussbaumer ",
                        static_cast<u64>(mpn::threshold::mul_nussbaumer));
  }

  header("kernel, cycles per limb (cyc/op column is per-limb here)");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 8 || n > 1024 ) continue;
    const u64 reps = 4096;
    emit("kernel", "lshift_portable", n, measure(n, reps, [n] {
           (void)mpn::__portable::lshift(g_r, g_a, n, 17);
           clobber(g_r);
         }));
    emit("kernel", "lshift_simd", n, measure(n, reps, [n] {
           (void)mpn::lshift(g_r, g_a, n, 17);
           clobber(g_r);
         }));
    emit("kernel", "rshift_portable", n, measure(n, reps, [n] {
           (void)mpn::__portable::rshift(g_r, g_a, n, 17);
           clobber(g_r);
         }));
    emit("kernel", "rshift_simd", n, measure(n, reps, [n] {
           (void)mpn::rshift(g_r, g_a, n, 17);
           clobber(g_r);
         }));
    emit("kernel", "addmul_1_portable", n, measure(n, reps, [n] {
           (void)mpn::__portable::addmul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
    emit("kernel", "addmul_1_asm", n, measure(n, reps, [n] {
           (void)mpn::addmul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
    emit("kernel", "submul_1_portable", n, measure(n, reps, [n] {
           (void)mpn::__portable::submul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
    emit("kernel", "submul_1_asm", n, measure(n, reps, [n] {
           (void)mpn::submul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
    emit("kernel", "mul_1_portable", n, measure(n, reps, [n] {
           (void)mpn::__portable::mul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
    emit("kernel", "mul_1_asm", n, measure(n, reps, [n] {
           (void)mpn::mul_1(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
#if defined(__micron_arbint_have_simd_mul_experiment)

    emit("kernel", "mul_1_avx2_expt", n, measure(n, reps, [n] {
           (void)mpn::__kern::mul_1_avx2(g_r, g_a, n, 0x9e3779b97f4a7c15ull);
           clobber(g_r);
         }));
#endif
  }

  header("mul, balanced n x n");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    const u64 reps = reps_for(n);
    emit("mul", "basecase", n, measure(n, reps, [n] {
           mpn::mul_with<mpn::algo::basecase>(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
    emit("mul", "comba", n, measure(n, reps, [n] {
           mpn::mul_with<mpn::algo::comba>(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
    emit("mul", "karatsuba", n, measure(n, reps, [n] {
           mpn::mul_with<mpn::algo::karatsuba>(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
    if ( mpn::toom3_applies(n, n) )
      emit("mul", "toom3", n, measure(n, reps, [n] {
             mpn::mul_with<mpn::algo::toom3>(g_r, g_a, n, g_b, n, g_sc);
             clobber(g_r);
           }));
    if ( mpn::toom4_applies(n, n) )
      emit("mul", "toom4", n, measure(n, reps, [n] {
             mpn::mul_with<mpn::algo::toom4>(g_r, g_a, n, g_b, n, g_sc);
             clobber(g_r);
           }));
    emit("mul", "dispatch", n, measure(n, reps, [n] {
           mpn::mul(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
  }

  header("sqr, n x n");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    const u64 reps = reps_for(n);
    emit("sqr", "basecase", n, measure(n, reps, [n] {
           mpn::sqr_with<mpn::algo::basecase>(g_r, g_a, n, g_sc);
           clobber(g_r);
         }));
    emit("sqr", "comba", n, measure(n, reps, [n] {
           mpn::sqr_with<mpn::algo::comba>(g_r, g_a, n, g_sc);
           clobber(g_r);
         }));
    emit("sqr", "karatsuba", n, measure(n, reps, [n] {
           mpn::sqr_with<mpn::algo::karatsuba>(g_r, g_a, n, g_sc);
           clobber(g_r);
         }));
    if ( mpn::sqr_toom3_applies(n) )
      emit("sqr", "toom3", n, measure(n, reps, [n] {
             mpn::sqr_with<mpn::algo::toom3>(g_r, g_a, n, g_sc);
             clobber(g_r);
           }));
    if ( mpn::sqr_toom4_applies(n) )
      emit("sqr", "toom4", n, measure(n, reps, [n] {
             mpn::sqr_with<mpn::algo::toom4>(g_r, g_a, n, g_sc);
             clobber(g_r);
           }));
    emit("sqr", "dispatch", n, measure(n, reps, [n] {
           mpn::sqr(g_r, g_a, n, g_sc);
           clobber(g_r);
         }));
  }

  header("mul, fixed width unrolled vs the same size dispatched");
  {
    const auto one = [&]<usize N>() {
      const u64 reps = reps_for(N);
      emit("mul_fixed", "comba_fixed", N, measure(N, reps, [] {
             mpn::mul_comba_fixed<N, N>(g_r, g_a, g_b);
             clobber(g_r);
           }));
      emit("mul_fixed", "comba", N, measure(N, reps, [] {
             mpn::mul_with<mpn::algo::comba>(g_r, g_a, N, g_b, N, g_sc);
             clobber(g_r);
           }));
      emit("mul_fixed", "basecase", N, measure(N, reps, [] {
             mpn::mul_with<mpn::algo::basecase>(g_r, g_a, N, g_b, N, g_sc);
             clobber(g_r);
           }));
    };
    one.template operator()<2>();
    one.template operator()<4>();
    one.template operator()<8>();
    one.template operator()<12>();
    one.template operator()<16>();
  }

  header("sqr, fixed width unrolled vs the same size dispatched");
  {
    const auto one = [&]<usize N>() {
      const u64 reps = reps_for(N);
      emit("sqr_fixed", "sqr_comba_fixed", N, measure(N, reps, [] {
             mpn::sqr_comba_fixed<N>(g_r, g_a);
             clobber(g_r);
           }));
      emit("sqr_fixed", "sqr_comba", N, measure(N, reps, [] {
             mpn::sqr_with<mpn::algo::comba>(g_r, g_a, N, g_sc);
             clobber(g_r);
           }));
      emit("sqr_fixed", "sqr_basecase", N, measure(N, reps, [] {
             mpn::sqr_with<mpn::algo::basecase>(g_r, g_a, N, g_sc);
             clobber(g_r);
           }));
    };
    one.template operator()<2>();
    one.template operator()<4>();
    one.template operator()<8>();
    one.template operator()<12>();
    one.template operator()<16>();
  }

  header("divrem, 2n limbs by n limbs");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize dn = SIZES[si];
    if ( dn < 6 || dn > MAX_N / 2u ) continue;
    const usize nn = 2u * dn;
    fill(g_dn, dn, 0x2468ACEull + dn);
    g_dn[dn - 1u] |= mpn::limb_msb;
    fill(g_nn, nn, 0x13579BDull + dn);
    g_dinv = mpn::invert_pi1(g_dn[dn - 1u], g_dn[dn - 2u]);
    const u64 reps = reps_for(dn);

    emit("divrem", "schoolbook", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::sbpi1_div_qr(g_q, g_nw, nn, g_dn, dn, g_dinv);
           clobber(g_q);
         }));
    emit("divrem", "divconquer", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::dc_div_qr(g_q, g_nw, nn, g_dn, dn, g_dinv, g_sc);
           clobber(g_q);
         }));

    emit("divrem", "barrett", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::mu_div_qr(g_q, g_nw, nn, g_dn, dn, g_sc);
           clobber(g_q);
         }));
  }

  header("divrem, 4n limbs by n limbs -- where mu has something to amortize");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize dn = SIZES[si];
    if ( dn < 6 || dn > MAX_N / 4u ) continue;
    const usize nn = 4u * dn;
    fill(g_dn, dn, 0x2468ACEull + dn);
    g_dn[dn - 1u] |= mpn::limb_msb;
    fill(g_nn, nn, 0x13579BDull + dn);
    g_dinv = mpn::invert_pi1(g_dn[dn - 1u], g_dn[dn - 2u]);
    const u64 reps = reps_for(2u * dn);

    emit("divrem4", "schoolbook", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::sbpi1_div_qr(g_q, g_nw, nn, g_dn, dn, g_dinv);
           clobber(g_q);
         }));
    emit("divrem4", "divconquer", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::dc_div_qr(g_q, g_nw, nn, g_dn, dn, g_dinv, g_sc);
           clobber(g_q);
         }));
    emit("divrem4", "barrett", dn, measure(dn, reps, [nn, dn] {
           mpn::copyi(g_nw, g_nn, nn);
           (void)mpn::mu_div_qr(g_q, g_nw, nn, g_dn, dn, g_sc);
           clobber(g_q);
         }));
  }

  header("invert_n, the reciprocal mu builds once per call");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 2 || n > MAX_N / 2u ) continue;
    fill(g_dn, n, 0x0FACADE0ull + n);
    g_dn[n - 1u] |= mpn::limb_msb;
    emit("invert_n", "basecase", n, measure(n, reps_for(n), [n] {
           mpn::invert_n_basecase(g_q, g_dn, n, g_sc);
           clobber(g_q);
         }));
  }

  header("gcd, n by n limbs");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 1 || n > 1024u ) continue;
    fill(g_a, n, 0xACE0F5AD0ull + n);
    fill(g_b, n, 0xB0BB1E50ull + n);
    g_a[n - 1u] |= 1u;
    g_b[n - 1u] |= 1u;
    const u64 reps = reps_for_gcd(n);

    if ( n <= 256u )
      emit("gcd", "binary", n, measure(n, reps, [n] {
             (void)mpn::gcd_with<mpn::gcd_algo::binary>(g_r, g_a, n, g_b, n, g_sc);
             clobber(g_r);
           }));
    emit("gcd", "lehmer", n, measure(n, reps, [n] {
           (void)mpn::gcd_with<mpn::gcd_algo::lehmer>(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
    if ( mpn::gcd_itch_with<mpn::gcd_algo::dc>(n, n) + n < sizeof(g_sc) / sizeof(g_sc[0]) )
      emit("gcd", "dc", n, measure(n, reps, [n] {
             (void)mpn::gcd_with<mpn::gcd_algo::dc>(g_r, g_a, n, g_b, n, g_sc);
             clobber(g_r);
           }));
    emit("gcd", "dispatch", n, measure(n, reps, [n] {
           (void)mpn::gcd(g_r, g_a, n, g_b, n, g_sc);
           clobber(g_r);
         }));
  }

  header("hgcd, one reduction of an n-limb pair");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 4 || n > 1024u ) continue;
    if ( mpn::hgcd_itch(n) >= sizeof(g_sc) / sizeof(g_sc[0]) ) continue;
    if ( mpn::hgcd_mat_itch(n) >= sizeof(g_nw) / sizeof(g_nw[0]) ) continue;
    fill(g_a, n, 0x5EEDBEEF0ull + n);
    fill(g_b, n, 0xFACEB00Cull + n);
    g_a[n - 1u] |= mpn::limb_msb;
    g_b[n - 1u] &= static_cast<mpn::limb_t>(mpn::limb_msb - 1u);

    emit("hgcd", "reduce", n, measure(n, reps_for_gcd(n), [n] {
           mpn::copyi(g_nn, g_a, n);
           mpn::copyi(g_nn + n, g_b, n);
           mpn::hgcd_mat M{};
           mpn::hgcd_mat_init(M, n, g_nw);
           (void)mpn::hgcd(g_nn, g_nn + n, n, M, g_sc);
           clobber(g_nn);
         }));
  }

  header("modmul, one a*b mod m");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 1 || n > 512u ) continue;
    fill(g_dn, n, 0x00DDD00Dull + n);
    g_dn[n - 1u] |= mpn::limb_msb;
    g_dn[0] |= 1u;
    fill(g_a, n, 0x1111AAAull + n);
    fill(g_b, n, 0x2222BBBull + n);
    for ( usize i = 0; i < n; ++i ) {
      if ( g_a[i] > g_dn[i] ) g_a[i] = g_dn[i] - 1u;
      if ( g_b[i] > g_dn[i] ) g_b[i] = g_dn[i] - 1u;
    }
    const u64 reps = reps_for(n);

    const mpn::mont_ctx mc = mpn::mont_make(g_dn, n);
    emit("modmul", "montgomery", n, measure(n, reps, [n, &mc] {
           mpn::mont_mul(g_r, g_a, g_b, mc, g_sc);
           clobber(g_r);
         }));

    mpn::limb_t *const bmw = g_nw;
    mpn::limb_t *const biw = g_nw + n;
    const mpn::barrett_ctx bc = mpn::barrett_make(bmw, biw, g_dn, n, g_sc);
    emit("modmul", "barrett", n, measure(n, reps, [n, &bc] {
           mpn::barrett_mul(g_r, g_a, g_b, bc, g_sc);
           clobber(g_r);
         }));

    emit("modmul", "divrem", n, measure(n, reps, [n] {
           mpn::mul(g_nn, g_a, n, g_b, n, g_sc);
           mpn::divrem(g_q, g_r, g_nn, 2u * n, g_dn, n, g_sc);
           clobber(g_r);
         }));
  }

  header("powm, 256-bit exponent (read cyc/op, not cyc/n^2)");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 1 || n > 128u ) continue;
    fill(g_dn, n, 0x0B0FA0Dull + n);
    g_dn[n - 1u] |= mpn::limb_msb;
    g_dn[0] |= 1u;
    fill(g_a, n, 0x3333CCCull + n);
    const usize en = (256u / mpn::limb_bits) != 0 ? (256u / mpn::limb_bits) : 1u;
    fill(g_b, en, 0x4444DDDull + n);
    g_b[en - 1u] |= mpn::limb_msb;

    u64 reps = TARGET_LIMB_OPS / (256ull * static_cast<u64>(n) * static_cast<u64>(n) + 1ull);
    if ( reps < 1 ) reps = 1;
    if ( reps > 256 ) reps = 256;

    emit("powm", "dispatch", n, measure(n, reps, [n, en] {
           mpn::powm(g_r, g_a, n, g_b, en, g_dn, n, g_sc);
           clobber(g_r);
         }));
  }

  header("invmod, n by n limbs");
  for ( usize si = 0; si < N_SIZES; ++si ) {
    const usize n = SIZES[si];
    if ( n < 1 || n > 128u ) continue;
    fill(g_dn, n, 0x1DEA0FF1ull + n);
    g_dn[n - 1u] |= mpn::limb_msb;
    g_dn[0] |= 1u;
    fill(g_a, n, 0x5555EEEull + n);
    const u64 reps = reps_for_gcd(n);
    emit("invmod", "gcdext", n, measure(n, reps, [n] {
           usize rn = 0;
           (void)mpn::invmod(g_r, rn, g_a, n, g_dn, n, g_sc);
           clobber(g_r);
         }));
  }

  if ( !g_csv ) micron::io::println("");
  return 0;
}
