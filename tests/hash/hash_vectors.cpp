//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/hash/crc.hpp"
#include "../../src/hash/hash.hpp"

#include "../snowball/snowball.hpp"

using ::sb::print;
using ::sb::require_true;

static void
fill(byte *b, usize n)
{
  for ( usize i = 0; i < n; ++i ) b[i] = (byte)(i * 37 + 11);
}

int
main()
{
  print("=== MICRON HASH KNOWN-ANSWER VECTORS ===");
  alignas(64) byte buf[512];
  fill(buf, 512);
  const byte *s9 = reinterpret_cast<const byte *>("123456789");

  sb::test_case("CRC check values (\"123456789\")");
  {
    require_true(micron::crc16_t10dif(0, s9, 9) == 0xD0DB);
    require_true(micron::crc32_gzip_refl(0, s9, 9) == 0xCBF43926u);
    require_true(micron::crc32_iscsi(0, s9, 9) == 0xE3069283u);
    require_true(micron::crc64_ecma_norm(0, s9, 9) == 0x6C40DF5F0B497347ull);
  }
  sb::end_test_case();

  sb::test_case("FNV-1a canonical vectors");
  {
    require_true(micron::fnv1a_32<0>(reinterpret_cast<const byte *>(""), 0) == 0x811c9dc5u);
    require_true(micron::fnv1a_32<0>(reinterpret_cast<const byte *>("a"), 1) == 0xe40c292cu);
    require_true(micron::fnv1a_32<0>(reinterpret_cast<const byte *>("foobar"), 6) == 0xbf9cf968u);
    require_true(micron::fnv1a_64<0>(reinterpret_cast<const byte *>(""), 0) == 0xcbf29ce484222325ull);
    require_true(micron::fnv1a_64<0>(reinterpret_cast<const byte *>("foobar"), 6) == 0x85944171f73967e8ull);
  }
  sb::end_test_case();

  sb::test_case("Murmur3-x64-128 vs canonical reference (seed 0)");
  {
    struct m128 {
      usize len;
      u64 a, b;
    };

    static const m128 gold[] = {
      { 0, 0x0000000000000000ull, 0x0000000000000000ull },  { 1, 0x932fc7cce617f1e7ull, 0x7a434b816c4508dcull },
      { 15, 0x2906f047b67f83ffull, 0x49ca338fe7701facull }, { 16, 0xda9c66580c5ef0fbull, 0x885aae87bb6c5ff7ull },
      { 17, 0x78b8ee9a775e07d1ull, 0xe36790301698fce0ull }, { 31, 0xb1ca061ed4c5532full, 0xcb6926489e7b3763ull },
      { 32, 0xaf7374eb8efe799bull, 0x3a5200924dcdd6ffull }, { 64, 0x497867a35161e107ull, 0x9a6c3536f9d78a8eull },
      { 96, 0xa0e81018ddeb9b7eull, 0x45a4fa7b2517d914ull }, { 256, 0x72651697728573d3ull, 0x437c9f8cb0bef786ull },
    };
    for ( const auto &g : gold ) {
      auto h = micron::hashes::murmur<0>(buf, g.len);
      require_true(h.a == g.a && h.b == g.b);
    }

    require_true(micron::hashes::murmur<0x1234u>(buf, 40).a == micron::hashes::murmur(buf, 40, 0x1234u).a);
  }
  sb::end_test_case();

  sb::test_case("xxHash64 vs canonical reference (seed 0)");
  {
    struct xrow {
      usize len;
      u64 h;
    };

    static const xrow gold[] = {
      { 0, 0xef46db3751d8e999ull },  { 1, 0xf592c0c7639c4cb6ull },  { 8, 0x57cb2b7521f3e21aull },
      { 31, 0xe4a0e629e519a4aeull }, { 32, 0xcc6b8aaada790b2dull }, { 33, 0x35ec49850475a832ull },
      { 64, 0x155ccce4bf32befcull }, { 96, 0x7c874b795fa7ab0cull }, { 256, 0x43c92f09cb3e28cfull },
    };
    for ( const auto &g : gold ) require_true(micron::hashes::xxhash64<0>(buf, g.len) == g.h);

    require_true(micron::hashes::xxhash64_rtseed(buf, 0x369bd65914cf0616ull, 96) == 0x09e0ecf39c6da741ull);
  }
  sb::end_test_case();

  sb::test_case("rapidhash vs upstream V3 (seed 0)");
  {
    struct r64 {
      usize len;
      u64 h;
    };

    static const r64 gold[] = {
      { 0, 0x0338dc4be2cecdaeull },  { 1, 0x1b8b997858cd243aull },   { 8, 0x48374b6735e2878eull },  { 16, 0x8d62e2179a38046full },
      { 31, 0xd2c452675b458c41ull }, { 32, 0xddcad65e2d0c8b73ull },  { 33, 0xf347f406daa80e85ull }, { 64, 0xda1a1bb5fa78999bull },
      { 96, 0xa98a47334c57b55aull }, { 256, 0x0ad2e0219a3bfdb6ull },
    };
    for ( const auto &g : gold ) require_true(micron::hashes::rapidhash<0>(buf, g.len) == g.h);

    require_true(micron::hashes::rapidhash_micro<0>(buf, 96) == 0x58c3682ec38dfc63ull);
    require_true(micron::hashes::rapidhash_nano<0>(buf, 96) == 0xb69d09bdf813618full);
    require_true(micron::hashes::rapidhash<0>(buf, 16) == micron::hashes::rapidhash(buf, 0ull, 16));
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("meow_hash vs upstream MeowHash (seed 0)");
  {
    struct m128 {
      usize len;
      u64 a, b;
    };

    static const m128 gold[] = {
      { 0, 0x657ca02a5859c045ull, 0x75a7b5550383265eull },  { 1, 0x9cb4bd359451a0c8ull, 0x32d6709a1d229caaull },
      { 8, 0xb41efe09684f9627ull, 0xe322f553bfbef27dull },  { 16, 0xa0930a7011bd8addull, 0xbc0e952a24bcd9dbull },
      { 31, 0x1f4fa1425801fab0ull, 0xd2a9f9103e1bf00full }, { 32, 0x7bcc4ca143d3ab77ull, 0x703adbd4b09dae85ull },
      { 33, 0x873a6209b5baaa67ull, 0x0025e1e202b8f4f0ull }, { 64, 0x0f39462d67082fefull, 0xb635503d9c1fad8cull },
      { 96, 0x2ede92b2681331deull, 0xa3b2a23f300f4c2bull }, { 256, 0x6f685ec0128f20b9ull, 0xadfc7ac247eef0c6ull },
    };
    for ( const auto &g : gold ) {
      auto h = micron::hashes::meowhash128<0>(buf, g.len);
      require_true(h.a == g.a && h.b == g.b);
    }

    require_true(micron::hashes::meowhash64<0>(buf, 64) == (gold[7].a ^ gold[7].b));
    require_true(micron::hashes::meowhash128<0>(buf, 40).a == micron::hashes::meowhash128(buf, 0ull, 40).a);
  }
  sb::end_test_case();
#endif

  sb::test_case("zzz-family self-consistency");
  {
    require_true(micron::hashes::zzzf64<0>(buf, 32) == 0xd1485c41af329b34ull);
    require_true(micron::hashes::zzz64<0>(buf, 32) == 0x00006b9116cddb11ull);
    require_true(micron::hashes::zzzf64<0x99u>(buf, 96) == micron::hashes::zzzf64(buf, 0x99, 96));
  }
  sb::end_test_case();

  print("[HASH KNOWN-ANSWER VECTORS OK]");
  return 1;
}
