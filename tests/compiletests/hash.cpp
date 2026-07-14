//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/hash/checksum.hpp"

static constexpr u8 k_digits[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
static_assert(micron::crc32_gzip_refl(0u, k_digits, 9) == 0xCBF43926u);
static_assert(micron::adler32(1u, k_digits, 9) == 0x091E01DEu);
static_assert(micron::hashes::xxhash32(k_digits, 9) == 0x937BAD67u);

int
main(int argc, char **)
{
  static u8 buf[8192];
  for ( usize i = 0; i < 8192; ++i ) buf[i] = static_cast<u8>(i * 37 + 11);

  const usize n = static_cast<usize>(argc) * 4096;

  u32 r = 0;
  r ^= micron::crc32_gzip_refl(0u, buf, n);
  r ^= micron::adler32(1u, buf, n);
  r ^= micron::hashes::xxhash32(buf, n);

  r ^= micron::crc::__crc32_refl_bytewise(buf, n, 0xFFFFFFFFu) ^ 0xFFFFFFFFu;
  r ^= micron::crc::__crc32_refl_slice8(buf, n, 0xFFFFFFFFu) ^ 0xFFFFFFFFu;
  r ^= micron::adler::__adler32_scalar(buf, n, 1u);

  return static_cast<int>(r) & 0x7f;
}
