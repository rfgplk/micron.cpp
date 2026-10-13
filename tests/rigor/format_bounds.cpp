//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/string/format.hpp"

int main()
{
  constexpr const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  const u64 values[] = {0, 1, 9, 10, 99, 100, 99999999, 100000000, 9999999999999999,
                        10000000000000000, u64{1} << 63, ~u64{0}};
  for ( u32 requested = 0; requested <= 37; ++requested ) {
    const u32 base = requested < 2 || requested > 36 ? 10 : requested;
    for ( u64 value : values ) {
      char reversed[64];
      usize count = 0;
      u64 rest = value;
      do {
        reversed[count++] = digits[rest % base];
        rest /= base;
      } while ( rest );
      for ( usize capacity = 0; capacity <= 72; ++capacity ) {
        char actual[74];
        for ( char &c : actual ) c = '!';
        const usize length = micron::format::__impl::fmt_uint_to_buf(actual + 1, capacity, value, requested, false);
        const usize expected = count < capacity ? count : capacity;
        if ( length != expected || actual[0] != '!' ) return 6;
        for ( usize i = 0; i < expected; ++i ) if ( actual[i + 1] != reversed[count - i - 1] ) return 6;
        for ( usize i = expected + 1; i < sizeof(actual); ++i ) if ( actual[i] != '!' ) return 6;
      }
    }
  }
  return 1;
}
