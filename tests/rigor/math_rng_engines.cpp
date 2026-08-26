// Fixed-seed differential fuzzing for every math::rng software engine.

#include "../../src/math/rng.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

namespace
{

template<typename Engine>
[[nodiscard]] Engine
make_engine(u64 seed) noexcept
{
  return Engine::from_seed(seed);
}

template<typename Engine>
[[nodiscard]] bool
bulk_equivalent(Engine scalar, Engine bulk, u64 size_seed) noexcept
{
  using T = typename Engine::result_type;
  rng::splitmix64 sizes(size_seed);
  T out[259];
  constexpr T guard = T(~T(0));
  for ( usize round = 0; round < 192; ++round ) {
    const usize n = usize(sizes.next() % 257);
    out[0] = guard;
    out[n + 1] = guard;
    bulk.generate(out + 1, n);
    if ( out[0] != guard || out[n + 1] != guard ) return false;
    for ( usize i = 0; i < n; ++i )
      if ( out[i + 1] != scalar.next() ) return false;
    if ( bulk.next() != scalar.next() ) return false;
  }
  return true;
}

struct x256_ref {
  u64 s[4];

  [[nodiscard]] u64
  next() noexcept
  {
    const u64 result = micron::math::bits::rol64(s[1] * 5, 7) * 9;
    const u64 t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = micron::math::bits::rol64(s[3], 45);
    return result;
  }
};

struct x128_ref {
  u32 s[4];

  [[nodiscard]] u32
  next() noexcept
  {
    const u32 result = micron::math::bits::rol32(s[1] * 5u, 7) * 9u;
    const u32 t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = micron::math::bits::rol32(s[3], 11);
    return result;
  }
};

[[nodiscard]] u64
pcg_output(u128 old) noexcept
{
  const u64 x = u64(old >> 64) ^ u64(old);
  const u32 r = u32(u64(old >> 122));
  return micron::math::bits::ror64(x, int(r));
}

[[nodiscard]] bool
oracle_sequences() noexcept
{
  rng::splitmix64 seeds(0xD1B54A32D192ED03ULL);
  for ( usize trial = 0; trial < 64; ++trial ) {
    const u64 seed = seeds.next();

    rng::splitmix64 sm(seed);
    u64 sm_state = seed;
    for ( usize i = 0; i < 257; ++i ) {
      sm_state += rng::splitmix64::increment;
      if ( sm.next() != rng::splitmix64::__mix(sm_state) ) return false;
    }

    auto x256 = rng::xoshiro256ss::from_seed(seed);
    x256_ref r256{ { x256.s[0], x256.s[1], x256.s[2], x256.s[3] } };
    for ( usize i = 0; i < 257; ++i )
      if ( x256.next() != r256.next() ) return false;

    auto x128 = rng::xoshiro128ss::from_seed(seed);
    x128_ref r128{ { x128.s[0], x128.s[1], x128.s[2], x128.s[3] } };
    for ( usize i = 0; i < 257; ++i )
      if ( x128.next() != r128.next() ) return false;

    const u64 stream = seeds.next();
    auto pcg = rng::pcg64::make(seed, stream);
    u128 state = 0;
    const u128 inc = (u128(stream) << 1) | u128(1);
    constexpr u128 multiplier = (u128(0x2360ED051FC65DA4ULL) << 64) | u128(0x4385DF649FCCF645ULL);
    state = state * multiplier + inc;
    state += u128(seed);
    state = state * multiplier + inc;
    for ( usize i = 0; i < 257; ++i ) {
      const u64 expected = pcg_output(state);
      state = state * multiplier + inc;
      if ( pcg.next() != expected ) return false;
    }

    auto mwc = rng::mwc64::from_seed(seed);
    u64 high = mwc.hi;
    u64 low = mwc.lo;
    for ( usize i = 0; i < 257; ++i ) {
      high = rng::mwc64::A_HI * u64(u32(high)) + (high >> 32);
      low = rng::mwc64::A_LO * u64(u32(low)) + (low >> 32);
      const u64 expected = (u64(u32(high)) << 32) | u32(low);
      if ( mwc.next() != expected ) return false;
    }

    rng::lcg64 lcg(seed);
    u64 lcg_state = seed;
    for ( usize i = 0; i < 257; ++i ) {
      lcg_state = rng::lcg64::A * lcg_state + rng::lcg64::C;
      if ( lcg.next() != lcg_state ) return false;
    }
  }
  return true;
}

template<bool LongJump, typename Engine, typename Word, usize Words, typename Ref>
[[nodiscard]] bool
jump_matches(Engine &engine, const Word (&polynomial)[Words], Ref &reference) noexcept
{
  Word accumulated[4]{};
  for ( usize i = 0; i < Words; ++i ) {
    for ( usize bit = 0; bit < sizeof(Word) * 8; ++bit ) {
      if ( (polynomial[i] >> bit) & Word(1) ) {
        accumulated[0] ^= reference.s[0];
        accumulated[1] ^= reference.s[1];
        accumulated[2] ^= reference.s[2];
        accumulated[3] ^= reference.s[3];
      }
      (void)reference.next();
    }
  }
  if constexpr ( LongJump )
    engine.long_jump();
  else
    engine.jump();
  for ( usize i = 0; i < 4; ++i )
    if ( engine.s[i] != accumulated[i] ) return false;
  return true;
}

struct scripted32 {
  using result_type = u32;
  u32 value;
  u32 draws;

  [[nodiscard]] u32
  next() noexcept
  {
    ++draws;
    const u32 result = value;
    value += 0x9E3779B9u;
    return result;
  }
};

};      // namespace

int
main()
{
  print("=== RNG ENGINE DIFFERENTIAL FUZZ ===");

  test_case("result widths and valid defaults");
  {
    static_assert(rng::splitmix64::result_bits == 64 && sizeof(rng::splitmix64::result_type) == 8);
    static_assert(rng::xoshiro256ss::result_bits == 64 && sizeof(rng::xoshiro256ss::result_type) == 8);
    static_assert(rng::xoshiro128ss::result_bits == 32 && sizeof(rng::xoshiro128ss::result_type) == 4);
    static_assert(rng::pcg64::result_bits == 64 && sizeof(rng::pcg64::result_type) == 8);
    static_assert(rng::mt19937::result_bits == 32 && sizeof(rng::mt19937::result_type) == 4);
    static_assert(rng::mwc64::result_bits == 64 && sizeof(rng::mwc64::result_type) == 8);
    static_assert(rng::lcg64::result_bits == 64 && sizeof(rng::lcg64::result_type) == 8);

    rng::xoshiro256ss x256;
    rng::xoshiro128ss x128;
    rng::mt19937 mt;
    require_true(x256.next() != 0 || x256.next() != 0);
    require_true(x128.next() != 0 || x128.next() != 0);
    require_true(mt.next() == 3499211612u);

    rng::xoshiro256ss zero256(0, 0, 0, 0);
    rng::xoshiro128ss zero128(0, 0, 0, 0);
    const u64 z256a = zero256.next();
    const u64 z256b = zero256.next();
    const u32 z128a = zero128.next();
    const u32 z128b = zero128.next();
    require_true(z256a != 0 || z256b != 0);
    require_true(z128a != 0 || z128b != 0);
  }
  end_test_case();

  test_case("scalar engines match independent recurrences");
  require_true(oracle_sequences());
  end_test_case();

  test_case("SplitMix64 known-answer vector");
  {
    constexpr u64 expected[]
        = { 0xE220A8397B1DCDAFULL, 0x6E789E6AA1B965F4ULL, 0x06C45D188009454FULL, 0xF88BB8A8724C81ECULL, 0x1B39896A51A8749BULL };
    rng::splitmix64 sm(0);
    bool ok = true;
    for ( usize i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i ) ok &= sm.next() == expected[i];
    require_true(ok);
  }
  end_test_case();

  test_case("MT19937 known-answer vector");
  {
    constexpr u32 expected[]
        = { 3499211612u, 581869302u, 3890346734u, 3586334585u, 545404204u, 4161255391u, 3922919429u, 949333985u, 2715962298u, 1323567403u };
    rng::mt19937 mt(5489u);
    bool ok = true;
    for ( usize i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i ) ok &= mt.next() == expected[i];
    u32 last = expected[9];
    for ( usize i = sizeof(expected) / sizeof(expected[0]); i < 10000; ++i ) last = mt.next();
    ok &= last == 4123659995u;
    require_true(ok);
  }
  end_test_case();

  test_case("bulk generation is sequence preserving");
  {
    rng::splitmix64 seeds(0x8CB92BA72F3D8DD7ULL);
    bool ok = true;
    for ( usize trial = 0; trial < 12; ++trial ) {
      const u64 seed = seeds.next();
      ok &= bulk_equivalent(make_engine<rng::splitmix64>(seed), make_engine<rng::splitmix64>(seed), seed ^ 1);
      ok &= bulk_equivalent(make_engine<rng::xoshiro256ss>(seed), make_engine<rng::xoshiro256ss>(seed), seed ^ 2);
      ok &= bulk_equivalent(make_engine<rng::xoshiro128ss>(seed), make_engine<rng::xoshiro128ss>(seed), seed ^ 3);
      const auto p0 = rng::pcg64::make(seed, trial + 3);
      ok &= bulk_equivalent(p0, p0, seed ^ 4);
      ok &= bulk_equivalent(make_engine<rng::mt19937>(seed), make_engine<rng::mt19937>(seed), seed ^ 5);
      ok &= bulk_equivalent(make_engine<rng::mwc64>(seed), make_engine<rng::mwc64>(seed), seed ^ 6);
      ok &= bulk_equivalent(make_engine<rng::lcg64>(seed), make_engine<rng::lcg64>(seed), seed ^ 7);
    }
    require_true(ok);
  }
  end_test_case();

  test_case("xoshiro jump polynomials");
  {
    constexpr u64 j256[] = { 0x180EC6D33CFD0ABAULL, 0xD5A61266F0C9392CULL, 0xA9582618E03FC9AAULL, 0x39ABDC4529B1661CULL };
    constexpr u64 lj256[] = { 0x76E15D3EFEFDCBBFULL, 0xC5004E441C522FB3ULL, 0x77710069854EE241ULL, 0x39109BB02ACBE635ULL };
    constexpr u32 j128[] = { 0x8764000Bu, 0xF542D2D3u, 0x6FA035C3u, 0x77F2DB5Bu };
    constexpr u32 lj128[] = { 0xB523952Eu, 0x0B6F099Fu, 0xCCF5A0EFu, 0x1C580662u };
    auto x256 = rng::xoshiro256ss::from_seed(0xF1357AEA2E62A9C5ULL);
    auto x128 = rng::xoshiro128ss::from_seed(0xF1357AEA2E62A9C5ULL);
    auto x256_long = x256;
    auto x128_long = x128;
    x256_ref r256{ { x256.s[0], x256.s[1], x256.s[2], x256.s[3] } };
    x128_ref r128{ { x128.s[0], x128.s[1], x128.s[2], x128.s[3] } };
    x256_ref r256_long = r256;
    x128_ref r128_long = r128;
    require_true(jump_matches<false>(x256, j256, r256));
    require_true(jump_matches<false>(x128, j128, r128));
    require_true(jump_matches<true>(x256_long, lj256, r256_long));
    require_true(jump_matches<true>(x128_long, lj128, r128_long));
  }
  end_test_case();

  test_case("32-bit draw composition and real precision");
  {
    scripted32 a{ 0x12345678u, 0 };
    const f32 f = rng::dist::uniform_real<f32>(a);
    require_true(a.draws == 1 && f >= 0.0f && f < 1.0f);

    scripted32 b{ 0x12345678u, 0 };
    const u32 lo = b.value;
    const u32 hi = b.value + 0x9E3779B9u;
    const u64 expected = u64(lo) | (u64(hi) << 32);
    const f64 d = rng::dist::uniform_real<f64>(b);
    require_true(b.draws == 2);
    require_true(d == f64((expected >> 11) * 0x1.0p-53));

    auto x = rng::xoshiro128ss::from_seed(0x94D049BB133111EBULL);
    auto y = x;
    const u64 combined = u64(y.next()) | (u64(y.next()) << 32);
    require_true(x.next64() == combined);

    rng::splitmix64 strong(0x243F6A8885A308D3ULL);
    auto strong_ref = strong;
    require_true(rng::__impl::next32(strong) == u32(strong_ref.next()));
    rng::lcg64 weak(0x13198A2E03707344ULL);
    auto weak_ref = weak;
    require_true(rng::__impl::next32(weak) == u32(weak_ref.next() >> 32));
  }
  end_test_case();

  print("=== RNG ENGINE DIFFERENTIAL FUZZ PASSED ===");
  return 1;
}
