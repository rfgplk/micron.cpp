//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// build: duck build benches/arbint_nussbaumer_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/b
// perf : duck build benches/arbint_nussbaumer_bench.cpp --perf --fp --no-ssp --no-lto -i . --def ARBINT_BENCH_PERF -o bin/b
// phases: add --def ARBINT_NUSSBAUMER_BENCH_PHASES
// profile: add --def ARBINT_NUSSBAUMER_BENCH_PROFILE, then run under perf record/stat
// run  : taskset -c 2 ./bin/b/arbint_nussbaumer_bench

#include "../src/io/console.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/arbint.hpp"
#include "../src/std.hpp"

#if defined(ARBINT_BENCH_PERF)
#include "../external/bbench/bench.hpp"
#endif

namespace
{

namespace mpn = micron::math::mpn;

#if defined(ARBINT_BENCH_PERF)
using arb_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
#endif

#if defined(__micron_arch_width_32)
constexpr usize mul_center = mpn::__nuss_max_balanced_limbs();
constexpr usize sqr_center = mul_center;
#else
constexpr usize mul_center = mpn::threshold::mul_nussbaumer;
constexpr usize sqr_center = mpn::threshold::sqr_nussbaumer;
#endif
constexpr usize max_center = mul_center > sqr_center ? mul_center : sqr_center;
#if defined(ARBINT_NUSSBAUMER_BENCH_SEARCH)
constexpr usize MAX_N = 344002u;
#elif defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
constexpr usize MAX_N = 260002u;
#else
constexpr usize MAX_N = max_center + max_center / 4u + 2u;
#endif

constexpr usize mul_ns_itch = mpn::nussbaumer_itch(MAX_N, MAX_N);
constexpr usize sqr_ns_itch = mpn::sqr_nussbaumer_itch(MAX_N);
constexpr usize mul_t4_itch = mpn::toom4_itch(MAX_N, true);
constexpr usize sqr_t4_itch = mpn::sqr_toom4_itch(MAX_N, true);
constexpr usize MAX_ITCH_0 = mul_ns_itch > sqr_ns_itch ? mul_ns_itch : sqr_ns_itch;
constexpr usize MAX_ITCH_1 = mul_t4_itch > sqr_t4_itch ? mul_t4_itch : sqr_t4_itch;
constexpr usize MAX_ITCH = MAX_ITCH_0 > MAX_ITCH_1 ? MAX_ITCH_0 : MAX_ITCH_1;

[[maybe_unused]] alignas(64) mpn::limb_t g_a[MAX_N];
[[maybe_unused]] alignas(64) mpn::limb_t g_b[MAX_N];
[[maybe_unused]] alignas(64) mpn::limb_t g_r[2u * MAX_N];
alignas(64) mpn::limb_t g_sc[MAX_ITCH];

struct cell {
  u64 cycles;
  u64 ipc_milli;
  u64 branch_miss_ppm;
};

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ull + 1442695040888963407ull;
  return s;
}

[[maybe_unused]] void
fill(mpn::limb_t *p, usize n, u64 seed) noexcept
{
  for ( usize i = 0u; i < n; ++i ) p[i] = static_cast<mpn::limb_t>(lcg_next(seed));
  p[n - 1u] |= mpn::limb_msb;
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[nodiscard]] u64
median3(u64 a, u64 b, u64 c) noexcept
{
  if ( a > b ) {
    const u64 t = a;
    a = b;
    b = t;
  }
  if ( b > c ) {
    const u64 t = b;
    b = c;
    c = t;
  }
  return a > b ? a : b;
}

template<typename Kernel>
cell
measure(u64 reps, Kernel &&kernel) noexcept
{
  kernel();
  u64 cycles[3]{};
  u64 ipc[3]{};
  u64 misses[3]{};
  for ( usize m = 0u; m < 3u; ++m ) {
#if defined(ARBINT_BENCH_PERF)
    arb_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0u; i < reps; ++i ) kernel();
    evs.end();
    const u64 cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const u64 ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const u64 branches = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const u64 branch_misses = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    cycles[m] = cyc / reps;
    ipc[m] = cyc != 0u ? 1000u * ins / cyc : 0u;
    misses[m] = branches != 0u ? 1000000u * branch_misses / branches : 0u;
#else
    asm volatile("" ::: "memory");
    const u64 begin = micron::math::__asm_op::rdtsc64();
    for ( u64 i = 0u; i < reps; ++i ) kernel();
    asm volatile("" ::: "memory");
    cycles[m] = (micron::math::__asm_op::rdtsc64() - begin) / reps;
#endif
  }
  return { median3(cycles[0], cycles[1], cycles[2]), median3(ipc[0], ipc[1], ipc[2]), median3(misses[0], misses[1], misses[2]) };
}

#if !defined(ARBINT_NUSSBAUMER_BENCH_SEARCH) && !defined(ARBINT_NUSSBAUMER_BENCH_PHASES) && !defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
[[nodiscard]] u64
reps_for(usize n) noexcept
{
  if ( n >= max_center ) return 1u;
  if ( n >= max_center / 2u ) return 2u;
  return 4u;
}
#endif

[[maybe_unused]] void
emit_plan(const char *band, usize n, const mpn::__nuss_plan &p, usize itch) noexcept
{
  usize k0 = p.log_n;
  usize k1 = k0 > mpn::threshold::nussbaumer_leaf_log ? k0 - k0 / 2u : 0u;
  usize k2 = k1 > mpn::threshold::nussbaumer_leaf_log ? k1 - k1 / 2u : 0u;
  usize k3 = k2 > mpn::threshold::nussbaumer_leaf_log ? k2 - k2 / 2u : 0u;
  micron::io::println("plan,", band, ",", static_cast<u64>(n), ",q=", static_cast<u64>(p.digit_bits), ",N=", static_cast<u64>(p.n),
                      ",shape=", static_cast<u64>(k0), "->", static_cast<u64>(k1), "->", static_cast<u64>(k2), "->", static_cast<u64>(k3),
                      ",scratch_bytes=", static_cast<u64>(itch * sizeof(mpn::limb_t)));
}

[[maybe_unused]] void
emit(const char *band, const char *tier, usize n, const cell &c) noexcept
{
  micron::io::println("sample,", band, ",", tier, ",", static_cast<u64>(n), ",cycles=", c.cycles, ",ipc_milli=", c.ipc_milli,
                      ",branch_miss_ppm=", c.branch_miss_ppm);
}

#if defined(ARBINT_NUSSBAUMER_BENCH_PHASES)
void
fill_coeffs(mpn::__nuss_coeffs p, u64 seed) noexcept
{
  for ( usize i = 0u; i < p.n; ++i ) {
    mpn::__nuss_coeff v{ static_cast<mpn::limb_t>(lcg_next(seed) & 0x3fffffffu), 0u };
    if ( (lcg_next(seed) & 1u) != 0u ) v = mpn::__nuss_coeff_neg(v);
    mpn::__nuss_coeff_store(p, i, v);
  }
}

void
run_phases() noexcept
{
  constexpr usize r = 1024u;
  constexpr usize outer_n = 32u;
  constexpr usize total = outer_n * r;
  auto transform = mpn::__nuss_coeff_span(g_sc, total);
  auto transposed = mpn::__nuss_coeff_span(g_sc + 2u * total, total);
  auto butterfly_a = mpn::__nuss_coeff_span(g_sc + 4u * total, r);
  auto butterfly_b = mpn::__nuss_coeff_span(g_sc + 4u * total + 2u * r, r);
  auto temp = mpn::__nuss_coeff_span(g_sc + 4u * total + 4u * r, r);
  auto leaf_a = mpn::__nuss_coeff_span(g_sc + 4u * total + 6u * r, 16u);
  auto leaf_b = mpn::__nuss_coeff_span(g_sc + 4u * total + 6u * r + 32u, 16u);
  auto leaf_r = mpn::__nuss_coeff_span(g_sc + 4u * total + 6u * r + 64u, 16u);
  mpn::limb_t *const leaf_scratch = g_sc + 4u * total + 6u * r + 96u;

  fill_coeffs(transform, 0x243f6a8885a308d3ull);
  fill_coeffs(butterfly_a, 0x13198a2e03707344ull);
  fill_coeffs(butterfly_b, 0xa4093822299f31d0ull);
  fill_coeffs(leaf_a, 0x082efa98ec4e6c89ull);
  fill_coeffs(leaf_b, 0x452821e638d01377ull);

  emit("phase", "butterfly_1024", r, measure(4096u, [&] {
         mpn::__nuss_butterfly(butterfly_a, butterfly_b);
         clobber(butterfly_a.lo);
       }));
  emit("phase", "twiddle_1024", r, measure(4096u, [&] {
         mpn::__nuss_twiddle_copy(temp, mpn::__nuss_as_const(butterfly_a), 341u);
         clobber(temp.lo);
       }));
  emit("phase", "transpose_32x1024", total, measure(128u, [&] {
         mpn::__nuss_transpose(transposed, mpn::__nuss_as_const(transform), outer_n, r);
         clobber(transposed.lo);
       }));
  emit("phase", "forward_inverse", total, measure(8u, [&] {
         mpn::__nuss_forward(transform, outer_n, r, temp);
         mpn::__nuss_inverse(transform, outer_n, r, temp);
         clobber(transform.lo);
       }));
  emit("phase", "mul_leaf_8", 8u, measure(131072u, [&] {
         mpn::__nuss_mul_leaf(mpn::__nuss_coeff_subspan(leaf_r, 0u, 8u), mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_a), 0u, 8u),
                              mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_b), 0u, 8u), leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "sqr_leaf_8", 8u, measure(131072u, [&] {
         mpn::__nuss_sqr_leaf(mpn::__nuss_coeff_subspan(leaf_r, 0u, 8u), mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_a), 0u, 8u),
                              leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "mul_leaf_4", 4u, measure(262144u, [&] {
         mpn::__nuss_mul_leaf(mpn::__nuss_coeff_subspan(leaf_r, 0u, 4u), mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_a), 0u, 4u),
                              mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_b), 0u, 4u), leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "sqr_leaf_4", 4u, measure(262144u, [&] {
         mpn::__nuss_sqr_leaf(mpn::__nuss_coeff_subspan(leaf_r, 0u, 4u), mpn::__nuss_coeff_subspan(mpn::__nuss_as_const(leaf_a), 0u, 4u),
                              leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "mul_leaf_16", 16u, measure(32768u, [&] {
         mpn::__nuss_mul_leaf(leaf_r, mpn::__nuss_as_const(leaf_a), mpn::__nuss_as_const(leaf_b), leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "sqr_leaf_16", 16u, measure(32768u, [&] {
         mpn::__nuss_sqr_leaf(leaf_r, mpn::__nuss_as_const(leaf_a), leaf_scratch);
         clobber(leaf_r.lo);
       }));
  emit("phase", "divexact", total, measure(128u, [&] {
         (void)mpn::__nuss_divexact_coeffs(transform, 5u);
         clobber(transform.lo);
       }));
}
#endif

#if defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
void
run_profile() noexcept
{
  constexpr usize n = 260000u;
  fill(g_a, n, 0x123456789abcdef0ull);
  fill(g_b, n, 0xfedcba9876543210ull);
  for ( usize i = 0u; i < 8u; ++i ) {
#if defined(ARBINT_NUSSBAUMER_BENCH_PROFILE_SQR)
    mpn::sqr_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_sc);
#else
    mpn::mul_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_b, n, g_sc);
#endif
    clobber(g_r);
  }
}
#endif

#if !defined(ARBINT_NUSSBAUMER_BENCH_SEARCH) && !defined(ARBINT_NUSSBAUMER_BENCH_PHASES) && !defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
void
run_size(const char *band, usize n) noexcept
{
  fill(g_a, n, 0x123456789abcdef0ull + n);
  fill(g_b, n, 0xfedcba9876543210ull + n);
  const mpn::__nuss_plan p = mpn::__nuss_make_plan(n, n);
  emit_plan(band, n, p, mpn::nussbaumer_itch(n, n));
  const u64 reps = reps_for(n);

  emit(band, "mul_toom4", n, measure(reps, [n] {
         mpn::mul_with<mpn::algo::toom4>(g_r, g_a, n, g_b, n, g_sc);
         clobber(g_r);
       }));
  emit(band, "mul_nussbaumer", n, measure(reps, [n] {
         mpn::mul_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_b, n, g_sc);
         clobber(g_r);
       }));
  emit(band, "mul_dispatch", n, measure(reps, [n] {
         mpn::mul(g_r, g_a, n, g_b, n, g_sc);
         clobber(g_r);
       }));
  emit(band, "sqr_toom4", n, measure(reps, [n] {
         mpn::sqr_with<mpn::algo::toom4>(g_r, g_a, n, g_sc);
         clobber(g_r);
       }));
  emit(band, "sqr_nussbaumer", n, measure(reps, [n] {
         mpn::sqr_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_sc);
         clobber(g_r);
       }));
  emit(band, "sqr_dispatch", n, measure(reps, [n] {
         mpn::sqr(g_r, g_a, n, g_sc);
         clobber(g_r);
       }));
}
#endif

#if defined(ARBINT_NUSSBAUMER_BENCH_SEARCH)
void
run_search_size(usize n) noexcept
{
  fill(g_a, n, 0x123456789abcdef0ull + n);
  fill(g_b, n, 0xfedcba9876543210ull + n);
  const mpn::__nuss_plan p = mpn::__nuss_make_plan(n, n);
  emit_plan("search", n, p, mpn::nussbaumer_itch(n, n));
  emit("search", "mul_toom4", n, measure(1u, [n] {
         mpn::mul_with<mpn::algo::toom4>(g_r, g_a, n, g_b, n, g_sc);
         clobber(g_r);
       }));
  emit("search", "mul_nussbaumer", n, measure(1u, [n] {
         mpn::mul_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_b, n, g_sc);
         clobber(g_r);
       }));
  emit("search", "sqr_toom4", n, measure(1u, [n] {
         mpn::sqr_with<mpn::algo::toom4>(g_r, g_a, n, g_sc);
         clobber(g_r);
       }));
  emit("search", "sqr_nussbaumer", n, measure(1u, [n] {
         mpn::sqr_with<mpn::algo::nussbaumer>(g_r, g_a, n, g_sc);
         clobber(g_r);
       }));
}
#endif

#if !defined(ARBINT_NUSSBAUMER_BENCH_SEARCH) && !defined(ARBINT_NUSSBAUMER_BENCH_PHASES) && !defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
void
run_band(const char *name, usize center) noexcept
{
  const usize sizes[] = { center * 3u / 4u, center * 7u / 8u, center - 1u, center, center + 1u, center * 9u / 8u, center * 5u / 4u };
  for ( usize i = 0u; i < sizeof(sizes) / sizeof(sizes[0]); ++i ) run_size(name, sizes[i]);
}
#endif

};      // namespace

int
main()
{
  micron::io::println("# arbint pure-Nussbaumer crossover benchmark");
  micron::io::println("# limb_bits=", static_cast<u64>(mpn::limb_bits), ",mul_threshold=", static_cast<u64>(mpn::threshold::mul_nussbaumer),
                      ",sqr_threshold=", static_cast<u64>(mpn::threshold::sqr_nussbaumer),
                      ",leaf_log=", static_cast<u64>(mpn::threshold::nussbaumer_leaf_log),
                      ",max_log=", static_cast<u64>(mpn::threshold::nussbaumer_max_log),
                      ",cache_block_coeffs=", static_cast<u64>(mpn::__nuss_cache_block_coeffs));
#if !defined(ARBINT_BENCH_PERF)
  micron::io::println("# IPC and branch misses are zero in the rdtsc-only build; define ARBINT_BENCH_PERF for perf_event counters.");
#endif
#if defined(ARBINT_NUSSBAUMER_BENCH_PROFILE)
  run_profile();
#elif defined(ARBINT_NUSSBAUMER_BENCH_PHASES)
  run_phases();
#elif defined(ARBINT_NUSSBAUMER_BENCH_SEARCH)
#if defined(ARBINT_NUSSBAUMER_BENCH_NEAR_CAP)
#if defined(__micron_arch_width_32)
  const usize sizes[] = { 60000u, 70000u, 80000u, 90000u, 100000u, 110000u, 120000u, 130000u, 140000u, 150000u, 160000u };
#else
  const usize sizes[] = { 260000u, 270000u, 280000u, 290000u, 300000u, 310000u, 320000u, 330000u, 340000u, 344000u };
#endif
#else
  const usize sizes[] = { 80000u, 90000u, 150000u, 175000u, 250000u, 300000u, 310000u, 320000u, 330000u, 340000u, 344000u };
#endif
  for ( usize i = 0u; i < sizeof(sizes) / sizeof(sizes[0]); ++i ) run_search_size(sizes[i]);
#else
  if constexpr ( mul_center == sqr_center ) {
    run_band("threshold", mul_center);
  } else {
    run_band("sqr_threshold", sqr_center);
    run_band("mul_threshold", mul_center);
  }
#endif
  return 0;
}
