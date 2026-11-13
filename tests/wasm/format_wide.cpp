// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/io/echo.hpp"

static_assert(__LDBL_MANT_DIG__ == 113);

struct capture_sink {
  char bytes[80]{};
  usize size = 0;

  max_t
  put(const char *p, usize n)
  {
    if ( n > sizeof(bytes) - size ) return -1;
    for ( usize i = 0; i < n; ++i ) bytes[size++] = p[i];
    return n;
  }

  max_t
  put(char c)
  {
    return put(&c, 1);
  }

  max_t
  flush()
  {
    return 0;
  }
};

static bool
equal(const char *a, usize n, const char *b)
{
  for ( usize i = 0; i < n; ++i )
    if ( !b[i] || a[i] != b[i] ) return false;
  return b[n] == '\0';
}

int
main()
{
  using U = unsigned __int128;
  constexpr U one = U(16383) << 112;

  struct sample {
    U bits;
    const char *text;
    bool hexadecimal;
  };

  constexpr sample samples[] = {
    { one + 1, "0x10000000000000000000000000001p-112", true },
    { one + (U(1) << 111), "1.5", false },
    { U(26383) << 112, "0x1p+10000", true },
    { U(1) << 112, "0x1p-16382", true },
    { 1, "0x1p-16494", true },
    { (U(1) << 127) | 1, "-0x1p-16494", true },
    { (U(32767) << 112) - 1, "0x1ffffffffffffffffffffffffffffp+16271", true },
  };
  for ( const auto &sample : samples ) {
    const volatile long double source = __builtin_bit_cast(long double, sample.bits);
    char bytes[80];
    for ( auto &byte : bytes ) byte = '!';
    const usize n = micron::io::__impl::arith_to_buf(bytes + 1, sizeof(bytes) - 2, source);
    if ( !equal(bytes + 1, n, sample.text) || bytes[0] != '!' || bytes[n + 1] != '!' ) return 6;

    capture_sink sink;
    if ( micron::io::printk(sink, static_cast<long double>(source)) != static_cast<max_t>(n) || !equal(sink.bytes, sink.size, sample.text) )
      return 6;

    if ( sample.hexadecimal ) {
      for ( auto &byte : bytes ) byte = '!';
      if ( micron::io::__impl::arith_to_buf(bytes + 1, n - 1, source) != 0 ) return 6;
      for ( const auto byte : bytes )
        if ( byte != '!' ) return 6;
      if ( micron::io::__impl::arith_to_buf(bytes + 1, n, source) != n || !equal(bytes + 1, n, sample.text) || bytes[0] != '!'
           || bytes[n + 1] != '!' )
        return 6;
    }
  }
  return 1;
}
