

#include "../../src/hash/hash.hpp"

#include "../snowball/snowball.hpp"

using ::sb::print;
using ::sb::require_true;

static u64
next_rand(u64 &s)
{
  u64 z = (s += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

static u32
popcount64(u64 v)
{
  return (u32)__builtin_popcountll(v);
}

struct golden {
  usize len;
  u64 h64;
  u64 out[4];
};

static constexpr golden goldens[] = {
  { 0, 0x4d46fb28dc3d3ad8ull, { 0x3a285fda2c340ec1ull, 0xc479153e85eb2d88ull, 0x6c4b3859f2268637ull, 0xdf5c899587c49fa6ull } },
  { 1, 0xe5d178d864ef5d07ull, { 0x7c1fc626616bd3ebull, 0xc882defa98440331ull, 0x71d8c2a0bf4349e2ull, 0x2094a2a42283c43full } },
  { 8, 0x924676f25a50c531ull, { 0x4c2caf48f3e5d07full, 0x3e292b54e703425dull, 0x878ed7bb312551f8ull, 0x67cd25557f9306ebull } },
  { 31, 0x903630f0a4334fffull, { 0x31377243ceba9c5bull, 0x4b74e9df2d15c6dfull, 0xe1c8d8de5e7d5021ull, 0x0bbd73b219e1455aull } },
  { 32, 0xc3790a80e238b344ull, { 0xbc50c5e35c7e657aull, 0x34a532806beec54cull, 0xd2f3f9be31e5370aull, 0x997f045de44d2478ull } },
  { 33, 0x2247185ac4e3b778ull, { 0x65c32127a2d8be36ull, 0xa61f1704807c1bd2ull, 0xe0ffd8444f52373full, 0x0164f63da91525a3ull } },
  { 64, 0x93818cb2127aa446ull, { 0x36b8c9edc7953701ull, 0x587d035d2b6d30caull, 0x4d2b00ae61807e2aull, 0xb06f46ac9f02dda7ull } },
  { 96, 0xe5165a8c7ba41a84ull, { 0x9d9095fb2879c333ull, 0x08ea132b204f8ef8ull, 0x22fde4837df74f75ull, 0x529138df0e65183aull } },
};

int
main()
{
  print("=== ZZZF FULL-DIFFUSION SPEC ===");

  sb::test_case("golden vectors (algorithm + cross-arch pin)");
  {
    alignas(32) u8 buf[96];
    for ( u64 i = 0; i < 96; ++i ) buf[i] = (u8)(i * 37 + 11);
    for ( const auto &g : goldens ) {
      alignas(32) u64 out[4];
      micron::hashes::zzzf(buf, 0x1234, g.len, out);
      for ( u64 l = 0; l < 4; ++l ) require_true(out[l] == g.out[l]);
      require_true(micron::hashes::zzzf64(buf, 0x1234, g.len) == g.h64);
      auto h128 = micron::hashes::zzzf128(buf, 0x1234, g.len);
      require_true(h128.a == (g.out[0] ^ g.out[1]) && h128.b == (g.out[2] ^ g.out[3]));
    }

    require_true(micron::hashes::zzzf64<0x1234>(buf, 96) == micron::hashes::zzzf64(buf, 0x1234, 96));
  }
  sb::end_test_case();

  sb::test_case("no dead input bits + 64-bit fold avalanche (8/32/33/96B)");
  {
    constexpr usize sizes[] = { 8, 32, 33, 96 };
    constexpr u64 trials = 100;
    u64 rng = 7;
    for ( usize nbytes : sizes ) {
      const u64 nbits = nbytes * 8;
      for ( u64 bit = 0; bit < nbits; ++bit ) {
        u64 changed = 0;
        u64 flips = 0;
        for ( u64 t = 0; t < trials; ++t ) {
          alignas(32) u8 a[96], b[96];
          for ( usize i = 0; i < nbytes; i += 8 ) {
            u64 r = next_rand(rng);
            for ( usize j = 0; j < 8; ++j ) a[i + j] = (u8)(r >> (j * 8));
          }
          for ( usize i = 0; i < nbytes; ++i ) b[i] = a[i];
          b[bit >> 3] ^= (u8)(1u << (bit & 7));
          u64 ha = micron::hashes::zzzf64(a, 99, nbytes);
          u64 hb = micron::hashes::zzzf64(b, 99, nbytes);
          if ( ha != hb ) ++changed;
          flips += popcount64(ha ^ hb);
        }
        require_true(changed == trials);

        require_true(flips >= 30 * trials && flips <= 34 * trials);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("256-bit output: every input bit reaches every lane");
  {
    constexpr u64 trials = 100;
    u64 rng = 17;
    for ( u64 bit = 0; bit < 256; ++bit ) {
      u64 lane_flips[4] = { 0, 0, 0, 0 };
      for ( u64 t = 0; t < trials; ++t ) {
        alignas(32) u8 a[32], b[32];
        for ( usize i = 0; i < 32; i += 8 ) {
          u64 r = next_rand(rng);
          for ( usize j = 0; j < 8; ++j ) a[i + j] = (u8)(r >> (j * 8));
        }
        for ( usize i = 0; i < 32; ++i ) b[i] = a[i];
        b[bit >> 3] ^= (u8)(1u << (bit & 7));
        alignas(32) u64 ha[4], hb[4];
        micron::hashes::zzzf(a, 99, 32, ha);
        micron::hashes::zzzf(b, 99, 32, hb);
        for ( u64 l = 0; l < 4; ++l ) lane_flips[l] += popcount64(ha[l] ^ hb[l]);
      }

      for ( u64 l = 0; l < 4; ++l ) require_true(lane_flips[l] >= 28 * trials && lane_flips[l] <= 36 * trials);
    }
  }
  sb::end_test_case();

  sb::test_case("no constant output bits, seed + length injection");
  {
    u64 rng = 3;
    u64 or_scan[4] = { 0, 0, 0, 0 };
    u64 and_scan[4] = { ~0ull, ~0ull, ~0ull, ~0ull };
    for ( u64 t = 0; t < 8192; ++t ) {
      alignas(32) u8 buf[96];
      for ( usize i = 0; i < 96; i += 8 ) {
        u64 r = next_rand(rng);
        for ( usize j = 0; j < 8; ++j ) buf[i + j] = (u8)(r >> (j * 8));
      }
      alignas(32) u64 out[4];
      micron::hashes::zzzf(buf, (i64)t, 1 + (t % 96), out);
      for ( u64 l = 0; l < 4; ++l ) {
        or_scan[l] |= out[l];
        and_scan[l] &= out[l];
      }
    }
    for ( u64 l = 0; l < 4; ++l ) {
      require_true(or_scan[l] == ~0ull);
      require_true(and_scan[l] == 0ull);
    }

    alignas(32) u8 dummy[1] = { 0 };
    require_true(micron::hashes::zzzf64(dummy, 1, 0) != micron::hashes::zzzf64(dummy, 999999, 0));

    alignas(32) u8 pad[64] = {};
    pad[0] = 'X';
    u64 p1 = micron::hashes::zzzf64(pad, 42, 1);
    u64 p2 = micron::hashes::zzzf64(pad, 42, 2);
    u64 p32 = micron::hashes::zzzf64(pad, 42, 32);
    require_true(p1 != p2 && p1 != p32 && p2 != p32);
  }
  sb::end_test_case();

  print("[ZZZF FULL-DIFFUSION SPEC OK]");
  return 1;
}
