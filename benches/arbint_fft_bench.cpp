//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// duck build benches/arbint_fft_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/arbint/fft-bench
// taskset -c 2 bin/arbint/fft-bench/arbint_fft_bench

#include "../src/io/console.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/arbint.hpp"

namespace mpn = micron::math::mpn;
using limbs = micron::vector<mpn::limb_t, micron::allocator_serial<>, false>;

template<typename F>
u64
measure(F fn, usize reps)
{
  u64 samples[5];
  fn();
  for ( usize sample = 0u; sample < 5u; ++sample ) {
    asm volatile("" ::: "memory");
    const u64 start = micron::math::__asm_op::rdtsc64();
    for ( usize rep = 0u; rep < reps; ++rep ) fn();
    asm volatile("" ::: "memory");
    samples[sample] = (micron::math::__asm_op::rdtsc64() - start) / reps;
  }
  for ( usize i = 1u; i < 5u; ++i )
    for ( usize j = i; j > 0u && samples[j] < samples[j - 1u]; --j ) {
      const auto t = samples[j];
      samples[j] = samples[j - 1u];
      samples[j - 1u] = t;
    }
  return samples[2];
}

void
run(usize an, usize bn, bool square)
{
  limbs a(an), b(bn), out(an + bn), ref(an + bn);
  u64 state = 0x243f6a8885a308d3ull;
  for ( auto &v : a ) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    v = mpn::limb_t(state);
  }
  for ( auto &v : b ) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    v = mpn::limb_t(state);
  }
  if ( square ) b = a;
  usize itch = mpn::fft_itch(an, bn);
  const usize toom = mpn::mul_itch_with<mpn::algo::toom4>(an, bn);
  const usize sto = mpn::sqr_itch_with<mpn::algo::toom4>(an);
  if ( toom > itch ) itch = toom;
  if ( sto > itch ) itch = sto;
  limbs work(itch);
  mpn::mul_with<mpn::algo::toom4>(ref.data(), a.data(), an, b.data(), bn, work.data());
#if defined(__micron_arbint_fft)
  const bool fft_ok = square ? mpn::__fft_run<true>(out.data(), a.data(), an, a.data(), an, work.data())
                             : mpn::__fft_run<false>(out.data(), a.data(), an, b.data(), bn, work.data());
  if ( !fft_ok ) micron::exc<micron::except::runtime_error>("arbint FFT benchmark: unexpected exact fallback");
#endif
  for ( unsigned tier = 0u; tier < 4u; ++tier ) {
    auto kernel = [&] {
      if ( square ) {
        if ( tier == 0u ) mpn::sqr_with<mpn::algo::toom4>(out.data(), a.data(), an, work.data());
        if ( tier == 1u ) mpn::sqr_with<mpn::algo::nussbaumer>(out.data(), a.data(), an, work.data());
        if ( tier == 2u ) mpn::sqr_with<mpn::algo::fft>(out.data(), a.data(), an, work.data());
        if ( tier == 3u ) mpn::sqr(out.data(), a.data(), an, work.data());
      } else {
        if ( tier == 0u ) mpn::mul_with<mpn::algo::toom4>(out.data(), a.data(), an, b.data(), bn, work.data());
        if ( tier == 1u ) mpn::mul_with<mpn::algo::nussbaumer>(out.data(), a.data(), an, b.data(), bn, work.data());
        if ( tier == 2u ) mpn::mul_with<mpn::algo::fft>(out.data(), a.data(), an, b.data(), bn, work.data());
        if ( tier == 3u ) mpn::mul(out.data(), a.data(), an, b.data(), bn, work.data());
      }
      asm volatile("" : : "r"(out.data()) : "memory");
    };
    const u64 ticks = measure(kernel, an < 2048u ? 7u : (an < 16384u ? 3u : 1u));
    for ( usize i = 0u; i < an + bn; ++i )
      if ( ref[i] != out[i] ) micron::exc<micron::except::runtime_error>("arbint FFT benchmark: incorrect product");
    const char *names[] = { "toom4", "nussbaumer", "fft", "automatic" };
    const auto shape = mpn::__fft_pick_shape(an, bn);
    const auto selected = square ? mpn::clamp_to(mpn::pick_sqr(an), mpn::sqr_tier_cap) : mpn::clamp_algo(mpn::pick_mul(an, bn));
    micron::io::println(square ? "sqr," : "mul,", mpn::limb_bits, ",", an, ",", bn, ",", names[tier], ",", ticks, ",", shape.plan.n, ",",
                        shape.plan.digit_bits, ",", mpn::fft_itch(an, bn) * mpn::limb_bytes, ",", unsigned(selected));
  }
}

int
main()
{
  micron::io::println("operation,limb_bits,an,bn,tier,tsc_ticks,fft_n,digit_bits,scratch_bytes,automatic_tier");
  for ( usize n :
        { 256u,  512u,  1024u, 1536u,  2048u,  2560u,  2816u,  2817u,  3072u,  4095u,  4096u,  4097u,  5632u,  5633u,  8192u,
          8639u, 8640u, 8641u, 12288u, 15359u, 15360u, 15361u, 16384u, 16385u, 30719u, 30720u, 32768u, 32769u, 53759u, 53760u } ) {
    run(n, n, false);
    run(n, n, true);
  }
  for ( usize n : { 2048u, 4096u, 8192u } ) {
    run(n + 1u, n, false);
    run(2u * n, n, false);
    run(8u * n, n, false);
  }
#if defined(ARBINT_FFT_BENCH_LARGE)
  const usize large[] = { mpn::__fft_block_limbs, mpn::__fft_block_limbs + 1u, 131072u, 180000u, 262144u };
  for ( usize n : large ) {
    run(n, n, false);
    run(n, n, true);
  }
#endif
  return 0;
}
