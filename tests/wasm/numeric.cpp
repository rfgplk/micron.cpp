// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/math/extended.hpp"

namespace w = micron::wasm::math;
using U = unsigned __int128;
static U operands[5]{};

extern "C" __attribute__((export_name("numeric_buffer"))) U *
numeric_buffer()
{
  return operands;
}

extern "C" __attribute__((export_name("numeric_probe"))) void
numeric_probe(unsigned op)
{
  const volatile long double a = w::from<long double>(operands[0]), b = w::from<long double>(operands[1]),
                             c = w::from<long double>(operands[2]);
  long double r;
  switch ( op ) {
  case 0:
    r = a + b;
    break;
  case 1:
    r = a - b;
    break;
  case 2:
    r = a * b;
    break;
  case 3:
    r = a / b;
    break;
  case 4:
    r = w::fma<long double>(a, b, c);
    break;
  case 5:
    r = w::sqrt<long double>(a);
    break;
  case 6:
    r = static_cast<long double>(operands[0]);
    break;
  case 7:
    r = static_cast<long double>(static_cast<__int128>(operands[0]));
    break;
  case 8:
    operands[3] = static_cast<U>(a);
    return;
  case 9:
    operands[3] = static_cast<U>(static_cast<__int128>(a));
    return;
  case 10:
    operands[3] = __builtin_bit_cast(u64, static_cast<double>(a));
    return;
  case 11:
    operands[3] = __builtin_bit_cast(u32, static_cast<float>(a));
    return;
  case 12:
    operands[3] = w::bits(w::fma(w::from<double>(operands[0]), w::from<double>(operands[1]), w::from<double>(operands[2])));
    return;
  case 13:
    operands[3] = w::bits(w::sqrt(w::from<double>(operands[0])));
    return;
  case 14:
    operands[3] = w::bits(w::fma(w::from<float>(operands[0]), w::from<float>(operands[1]), w::from<float>(operands[2])));
    return;
  case 15:
    operands[3] = w::bits(w::sqrt(w::from<float>(operands[0])));
    return;
  case 16:
    operands[3] = w::bits(static_cast<double>(operands[0]));
    return;
  case 17:
    operands[3] = w::bits(static_cast<double>(static_cast<__int128>(operands[0])));
    return;
  case 18:
    operands[3] = w::bits(static_cast<float>(operands[0]));
    return;
  case 19:
    operands[3] = w::bits(static_cast<float>(static_cast<__int128>(operands[0])));
    return;
  case 20:
    operands[3] = static_cast<U>(w::from<double>(operands[0]));
    return;
  case 21:
    operands[3] = static_cast<U>(static_cast<__int128>(w::from<double>(operands[0])));
    return;
  case 22:
    operands[3] = static_cast<U>(w::from<float>(operands[0]));
    return;
  case 23:
    operands[3] = static_cast<U>(static_cast<__int128>(w::from<float>(operands[0])));
    return;
  case 24:
    r = static_cast<long double>(w::from<double>(operands[0]));
    break;
  case 25:
    r = static_cast<long double>(w::from<float>(operands[0]));
    break;
  case 26:
    r = w::fmod(a, b);
    break;
  case 27:
    r = w::remainder(a, b);
    break;
  case 28:
    operands[3] = operands[0] * operands[1];
    return;
  case 29:
    operands[3] = operands[0] / operands[1];
    return;
  case 30:
    operands[3] = operands[0] % operands[1];
    return;
  case 31:
    operands[3] = static_cast<U>(static_cast<__int128>(operands[0]) / static_cast<__int128>(operands[1]));
    return;
  case 32:
    operands[3] = static_cast<U>(static_cast<__int128>(operands[0]) % static_cast<__int128>(operands[1]));
    return;
  case 33:
    operands[3] = operands[0] << unsigned(operands[1]);
    return;
  case 34:
    operands[3] = operands[0] >> unsigned(operands[1]);
    return;
  case 35:
    operands[3] = static_cast<U>(static_cast<__int128>(operands[0]) >> unsigned(operands[1]));
    return;
  case 36: {
    const int shift = static_cast<int>(operands[1]);
    operands[3] = shift >= 0 && shift < 128 ? operands[0] << shift : 0;
    return;
  }
  case 37: {
    const int shift = static_cast<int>(operands[1]);
    operands[3] = shift >= 0 && shift < 128 ? operands[0] >> shift : 0;
    return;
  }
  case 38: {
    const int shift = static_cast<int>(operands[1]);
    operands[3] = shift >= 0 && shift < 128 ? static_cast<U>(static_cast<__int128>(operands[0]) >> shift) : 0;
    return;
  }
  default:
    r = a;
    break;
  }
  operands[3] = w::bits(r);
}

int
main()
{
  constexpr U one = U(16383) << 112, implicit = U(1) << 112, sign = U(1) << 127;
  const U cases[][4] = { { 0, 1, 1, 2 },
                         { 0, implicit - 1, 1, implicit },
                         { 1, implicit, implicit - 1, 1 },
                         { 2, 1, one, 1 },
                         { 3, 1, one, 1 },
                         { 3, 1, one + implicit, 0 },
                         { 3, 3, one + implicit, 2 },
                         { 3, sign | 1, one + implicit, sign },
                         { 2, implicit, one - implicit, implicit >> 1 },
                         { 0, one, U(16270) << 112, one },
                         { 0, one + 1, U(16270) << 112, one + 2 },
                         { 1, one, one, 0 } };
  for ( const auto &v : cases ) {
    operands[0] = v[1];
    operands[1] = v[2];
    numeric_probe(unsigned(v[0]));
    if ( operands[3] != v[3] ) return 6;
  }
  const long double tiny = w::from<long double>(1);
  if ( w::sqrt(tiny) != 0x1p-8247L ) return 6;
  const volatile long double runtime_one = 1.L;
  if ( w::sqrt(runtime_one) != 1.L ) return 6;
  if ( w::fma(0x1.0000000000000000000000000001p0L, 0x1.fffffffffffffffffffffffffffep-1L, -1.L) != -0x1p-224L ) return 6;
  if ( w::fma(0x1.ffffffffffffffffffffffffffffp16383L, 2.L, -0x1.ffffffffffffffffffffffffffffp16383L)
       != 0x1.ffffffffffffffffffffffffffffp16383L )
    return 6;
  if ( w::round(0x1.fffffffffffffp-2) != 0. || w::round(-0x1.fffffffffffffp-2) != -0. ) return 6;
  int e = 0;
  if ( w::frexp(tiny, &e) != 0.5L || e != -16493 ) return 6;
  if ( w::scalbn(1.L, -16494) != tiny || w::scalbn(1.5L, -16494) != w::from<long double>(2) ) return 6;
  return 1;
}
