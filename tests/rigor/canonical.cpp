//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/canonical.hpp"
#include "../../src/io/__serial_core.hpp"
#include "../../src/vector.hpp"
#include "../snowball/snowball.hpp"

namespace wire = micron::io::serialize::canonical;

constexpr bool
constant_roundtrip()
{
  byte data[32]{};
  wire::writer out(data, sizeof(data));
  if ( !out.put_u16(0x1234) || !out.put_u32(0x89abcdef) || !out.put_i64(-17) || !out.put_f64(-0.0) ) return false;
  wire::reader in(data, out.position());
  u16 a = 0;
  u32 b = 0;
  i64 c = 0;
  f64 d = 1;
  return in.get_u16(a) && in.get_u32(b) && in.get_i64(c) && in.get_f64(d) && a == 0x1234 && b == 0x89abcdef && c == -17
         && __builtin_bit_cast(u64, d) == 0x8000000000000000ull && !in.remaining();
}

static_assert(constant_roundtrip());

int
main()
{
  sb::test_case("canonical scalar bytes, alignments, bounds and independent oracle");
  u64 seed = 0x7ee2a497b165d02full;
  byte backing[80]{};
  for ( usize alignment = 0; alignment < 16; ++alignment ) {
    for ( usize iteration = 0; iteration < 128; ++iteration ) {
      seed ^= seed << 13;
      seed ^= seed >> 7;
      seed ^= seed << 17;
      byte *data = backing + alignment;
      wire::writer out(data, 8);
      sb::require(out.put_u64(seed));
      for ( usize i = 0; i < 8; ++i ) sb::require(data[i] == static_cast<byte>(seed >> (8 * i)));
      sb::require(!out.put_u8(9));
      sb::require(out.position() == 8);
      for ( usize length = 0; length < 8; ++length ) {
        u64 value = 0xcafe;
        wire::reader truncated(data, length);
        sb::require(!truncated.get_u64(value));
        sb::require(value == 0xcafe);
        sb::require(truncated.position() == 0);
      }
      wire::reader in(data, 8);
      u64 value = 0;
      sb::require(in.get_u64(value));
      sb::require(value == seed);
    }
  }
  sb::end_test_case();

  sb::test_case("canonical MFR1 byte framing matches the existing sequence representation");
  byte input[300]{};
  byte framed[340]{};
  byte generic[340]{};
  for ( usize n = 0; n <= 300; ++n ) {
    if ( n ) input[n - 1] = static_cast<byte>(n * 17);
    const max_t written = wire::frame_bytes(framed, sizeof(framed), input, n);
    sb::require(written == static_cast<max_t>(n + 16));
    micron::vector<byte> sequence;
    for ( usize i = 0; i < n; ++i ) sequence.push_back(input[i]);
    const max_t original = micron::io::serialize::frame_into(generic, sizeof(generic), sequence);
    sb::require(original == written);
    for ( usize i = 0; i < static_cast<usize>(written); ++i ) sb::require(generic[i] == framed[i]);
    wire::byte_view view;
    sb::require(wire::unframe_bytes(framed, static_cast<usize>(written), view, 300) == written);
    sb::require(view.size == n);
    for ( usize i = 0; i < n; ++i ) sb::require(view.data[i] == input[i]);
    for ( usize truncated = 0; truncated < static_cast<usize>(written); ++truncated ) {
      wire::byte_view untouched{ input, 77 };
      sb::require(wire::unframe_bytes(framed, truncated, untouched) < 0);
      sb::require(untouched.data == input && untouched.size == 77);
    }
    if ( n ) sb::require(wire::unframe_bytes(framed, static_cast<usize>(written), view, n - 1) < 0);
  }
  sb::end_test_case();

  sb::test_case("canonical frame rejects malformed metadata without publishing a view");
  const byte input_byte = 5;
  sb::require(wire::frame_bytes(framed, sizeof(framed), &input_byte, 1) == 17);
  for ( usize i = 0; i < 16; ++i ) {
    const byte saved = framed[i];
    framed[i] ^= 0x80;
    wire::byte_view output{ &input_byte, 99 };
    sb::require(wire::unframe_bytes(framed, 17, output) < 0);
    sb::require(output.data == &input_byte && output.size == 99);
    framed[i] = saved;
  }
  byte untouched[16];
  for ( byte &b : untouched ) b = 0xab;
  sb::require(wire::frame_bytes(untouched, sizeof(untouched), &input_byte, 1) < 0);
  for ( byte b : untouched ) sb::require(b == 0xab);
  sb::require(wire::byte_frame_size(~usize{ 0 }) < 0);
  sb::end_test_case();
  return 1;
}
