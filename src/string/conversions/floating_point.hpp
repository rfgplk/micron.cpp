//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{

namespace __impl
{
namespace __ryu
{

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ryu floats

namespace __f32
{

constexpr u32 __float_mantissa_bits = 23;
constexpr u32 __float_exponent_bits = 8;
constexpr i32 __float_bias = 127;
constexpr i32 __float_pow5_inv_bitcount = 59;
constexpr i32 __float_pow5_bitcount = 61;

// ceil(2^(q + __pow5_inv_BITCOUNT) / 5^q) for q in [0, 30]
inline constexpr u64 __pow5_inv[31]
    = { 576460752303423489u, 461168601842738791u, 368934881474191033u, 295147905179352826u, 472236648286964522u, 377789318629571618u,
        302231454903657294u, 483570327845851671u, 386856262276681337u, 309485009821345069u, 495176015714152111u, 396140812571321689u,
        316912650057057351u, 507060240091291762u, 405648192073033410u, 324518553658426728u, 519229685853482765u, 415383748682786212u,
        332306998946228970u, 531691198313966352u, 425352958651173082u, 340282366920938465u, 544451787073501545u, 435561429658801236u,
        348449143727040989u, 557518629963265582u, 446014903970612466u, 356811923176489973u, 570899077082383957u, 456719261665907166u,
        365375409332725733u };

// ceil(5^i × 2^(__pow5_BITCOUNT - ceil(log2(5^i)))) for i in [0, 46]
inline constexpr u64 __pow5[47]
    = { 1152921504606846976u, 1441151880758558720u, 1801439850948198400u, 2251799813685248000u, 1407374883553280000u, 1759218604441600000u,
        2199023255552000000u, 1374389534720000000u, 1717986918400000000u, 2147483648000000000u, 1342177280000000000u, 1677721600000000000u,
        2097152000000000000u, 1310720000000000000u, 1638400000000000000u, 2048000000000000000u, 1280000000000000000u, 1600000000000000000u,
        2000000000000000000u, 1250000000000000000u, 1562500000000000000u, 1953125000000000000u, 1220703125000000000u, 1525878906250000000u,
        1907348632812500000u, 1192092895507812500u, 1490116119384765625u, 1862645149230957031u, 1164153218269348144u, 1455191522836685180u,
        1818989403545856475u, 2273736754432320594u, 1421085471520200371u, 1776356839400250464u, 2220446049250313080u, 1387778780781445675u,
        1734723475976807094u, 2168404344971008868u, 1355252715606880542u, 1694065894508600678u, 2117582368135750847u, 1323488980084844279u,
        1654361225106055349u, 2067951531382569187u, 1292469707114105741u, 1615587133892632177u, 2019483917365790221u };

//%%%%%%%%%%%%%%%%%%%%%%%%%
// 32×64→96 bit multiply, portable split version
// equivalent to ((u128)m * factor) >> shift, but uses only 32×32→64 ops

inline u32
mul_shift_32(u32 m, u64 factor, i32 shift)
{
  u32 fLo = static_cast<u32>(factor);
  u32 fHi = static_cast<u32>(factor >> 32);
  u64 bits0 = static_cast<u64>(m) * fLo;
  u64 bits1 = static_cast<u64>(m) * fHi;
  u64 sum = (bits0 >> 32) + bits1;
  return static_cast<u32>(sum >> (shift - 32));
}

inline u32
mul_inv(u32 m, u32 q, i32 j)
{
  return mul_shift_32(m, __pow5_inv[q], j);
}

inline u32
mul_fwd(u32 m, u32 i, i32 j)
{
  return mul_shift_32(m, __pow5[i], j);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// returns ceil(log2(5^e)) for e >= 0

inline i32
pow5bits(i32 e)
{
  return static_cast<i32>(((static_cast<u32>(e) * 1217359u) >> 19) + 1);
}

inline i32
log10p2(i32 e)
{
  return static_cast<i32>((static_cast<u64>(e) * 78913) >> 18);
}

inline i32
log10p5(i32 e)
{
  return static_cast<i32>((static_cast<u64>(e) * 732923) >> 20);
}

inline u32
pow5fac(u32 v)
{
  u32 c = 0;
  while ( v > 0 ) {
    u32 q = v / 5, r = v - 5 * q;
    if ( r != 0 )
      break;
    v = q;
    ++c;
  }
  return c;
}

inline bool
mul_of_5(u32 v, u32 p)
{
  return pow5fac(v) >= p;
}

inline bool
mul_of_2(u32 v, u32 p)
{
  return (v & ((1u << p) - 1)) == 0;
}

inline u32
dlen9(u32 v)
{
  if ( v >= 100000000 )
    return 9;
  if ( v >= 10000000 )
    return 8;
  if ( v >= 1000000 )
    return 7;
  if ( v >= 100000 )
    return 6;
  if ( v >= 10000 )
    return 5;
  if ( v >= 1000 )
    return 4;
  if ( v >= 100 )
    return 3;
  if ( v >= 10 )
    return 2;
  return 1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// f2d: decompose f32 to shortest

struct f2d_t {
  u32 m;
  i32 e;
};

inline f2d_t
f2d(u32 ieeeMant, u32 ieeeExp)
{
  i32 e2;
  u32 m2;
  if ( ieeeExp == 0 ) {
    e2 = 1 - __float_bias - __float_mantissa_bits - 2;
    m2 = ieeeMant;
  } else {
    e2 = static_cast<i32>(ieeeExp) - __float_bias - __float_mantissa_bits - 2;
    m2 = (1u << __float_mantissa_bits) | ieeeMant;
  }
  bool even = (m2 & 1) == 0;
  bool acceptBounds = even;

  u32 mv = 4 * m2;
  u32 mp = 4 * m2 + 2;
  u32 mmShift = (ieeeMant != 0 || ieeeExp <= 1) ? 1 : 0;
  u32 mm = 4 * m2 - 1 - mmShift;

  u32 vr, vp, vm;
  i32 e10;
  bool vmTZ = false, vrTZ = false;

  if ( e2 >= 0 ) {
    u32 q = static_cast<u32>(log10p2(e2));
    e10 = static_cast<i32>(q);
    i32 k = __float_pow5_inv_bitcount + pow5bits(static_cast<i32>(q)) - 1;
    i32 i = -e2 + static_cast<i32>(q) + k;
    vr = mul_inv(mv, q, i);
    vp = mul_inv(mp, q, i);
    vm = mul_inv(mm, q, i);
    if ( q <= 9 ) {
      if ( mv % 5 == 0 ) {
        vrTZ = mul_of_5(mv, q);
      } else if ( acceptBounds ) {
        vmTZ = mul_of_5(mm, q);
      } else {
        vp -= mul_of_5(mp, q) ? 1 : 0;
      }
    }
  } else {
    u32 q = static_cast<u32>(log10p5(-e2));
    e10 = static_cast<i32>(q) + e2;
    i32 i = -e2 - static_cast<i32>(q);
    i32 k = pow5bits(i) - __float_pow5_bitcount;
    i32 j = static_cast<i32>(q) - k;
    vr = mul_fwd(mv, static_cast<u32>(i), j);
    vp = mul_fwd(mp, static_cast<u32>(i), j);
    vm = mul_fwd(mm, static_cast<u32>(i), j);
    if ( q <= 1 ) {
      vrTZ = true;
      if ( acceptBounds )
        vmTZ = (mmShift == 1);
      else
        --vp;
    } else if ( q < 31 ) {
      vrTZ = mul_of_2(mv, q - 1);
    }
  }

  // digit removal: find shortest representation
  i32 removed = 0;
  u32 lastDig = 0;
  u32 output;

  if ( vmTZ || vrTZ ) {
    while ( vp / 10 > vm / 10 ) {
      vmTZ &= (vm % 10 == 0);
      vrTZ &= (lastDig == 0);
      lastDig = vr % 10;
      vr /= 10;
      vp /= 10;
      vm /= 10;
      ++removed;
    }
    if ( vmTZ ) {
      while ( vm % 10 == 0 ) {
        vrTZ &= (lastDig == 0);
        lastDig = vr % 10;
        vr /= 10;
        vp /= 10;
        vm /= 10;
        ++removed;
      }
    }
    if ( vrTZ && lastDig == 5 && vr % 2 == 0 )
      lastDig = 4;
    output = vr + ((vr == vm && !(acceptBounds && vmTZ)) || lastDig >= 5 ? 1 : 0);
  } else {
    bool roundUp = false;
    u32 vpD = vp / 100, vmD = vm / 100;
    if ( vpD > vmD ) {
      u32 vrD = vr / 100;
      roundUp = (vr - 100 * vrD) >= 50;
      vr = vrD;
      vp = vpD;
      vm = vmD;
      removed += 2;
    }
    while ( vp / 10 > vm / 10 ) {
      roundUp = (vr % 10) >= 5;
      vr /= 10;
      vp /= 10;
      vm /= 10;
      ++removed;
    }
    output = vr + (vr == vm || roundUp ? 1 : 0);
  }

  return { output, e10 + removed };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// f2s_buffered
// buf must have at least 16 bytes returns length (not null-terminated)

inline usize
f2s_buffered(f32 val, char *buf)
{
  union {
    f32 f;
    u32 u;
  } fu;

  fu.f = val;
  u32 bits = fu.u;

  bool sign = (bits >> 31) != 0;
  u32 ieeeExp = (bits >> __float_mantissa_bits) & ((1u << __float_exponent_bits) - 1);
  u32 ieeeMant = bits & ((1u << __float_mantissa_bits) - 1);

  usize idx = 0;

  // NaN
  if ( ieeeExp == 0xFF && ieeeMant != 0 ) {
    buf[0] = 'N';
    buf[1] = 'a';
    buf[2] = 'N';
    return 3;
  }
  // Inf
  if ( ieeeExp == 0xFF ) {
    if ( sign )
      buf[idx++] = '-';
    buf[idx++] = 'I';
    buf[idx++] = 'n';
    buf[idx++] = 'f';
    return idx;
  }
  // zero
  if ( ieeeExp == 0 && ieeeMant == 0 ) {
    if ( sign )
      buf[idx++] = '-';
    buf[idx++] = '0';
    buf[idx++] = 'E';
    buf[idx++] = '0';
    return idx;
  }

  if ( sign )
    buf[idx++] = '-';

  f2d_t v = f2d(ieeeMant, ieeeExp);
  u32 olength = dlen9(v.m);

  // write mantissa digits in reverse
  char dtmp[10];
  u32 dpos = 0;
  u32 tmp = v.m;
  while ( tmp > 0 ) {
    dtmp[dpos++] = '0' + static_cast<char>(tmp % 10);
    tmp /= 10;
  }

  // format: first_digit.remaining E exponent
  buf[idx++] = dtmp[dpos - 1];
  if ( dpos > 1 ) {
    buf[idx++] = '.';
    for ( u32 j = dpos - 1; j-- > 0; )
      buf[idx++] = dtmp[j];
  }

  i32 exp10 = v.e + static_cast<i32>(olength) - 1;
  buf[idx++] = 'E';
  if ( exp10 < 0 ) {
    buf[idx++] = '-';
    exp10 = -exp10;
  }
  if ( exp10 >= 10 )
    buf[idx++] = '0' + static_cast<char>(exp10 / 10);
  buf[idx++] = '0' + static_cast<char>(exp10 % 10);

  return idx;
}

};     // namespace __f32

};     // namespace __ryu
};     // namespace __impl

// NOTE: consider moving to base format.hpp

template <typename C = char>
inline micron::hstring<C>
float_to_string(f32 val)
{
  char buf[16];
  usize n = __impl::__ryu::__f32::f2s_buffered(val, buf);
  return micron::hstring<C>(buf, buf + n);
}

template <typename C = char>
inline micron::hstring<C>
double_to_string(f64 val)
{
  char buf[26];
  usize n = __impl::__ryu::d2s_buffered(val, buf);
  return micron::hstring<C>(buf, buf + n);
}

template <typename C = char>
inline micron::hstring<C>
float_to_string(f32 val, u32 prec)
{
  char buf[350];
  usize n = __impl::__ryu::d2f_buffered(static_cast<f64>(val), buf, 350, prec);
  return micron::hstring<C>(buf, buf + n);
}

template <typename C = char>
inline micron::hstring<C>
double_to_string(f64 val, u32 prec)
{
  char buf[350];
  usize n = __impl::__ryu::d2f_buffered(val, buf, 350, prec);
  return micron::hstring<C>(buf, buf + n);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_fixed / to_scientific / to_general

template <typename C = char>
inline micron::hstring<C>
to_fixed(f64 val, u32 precision = 6)
{
  char buf[350];
  usize n = __impl::__ryu::d2f_buffered(val, buf, 350, precision);
  return micron::hstring<C>(buf, buf + n);
}

template <typename C = char>
inline micron::hstring<C>
to_scientific(f64 val, u32 precision = 6)
{
  char buf[350];
  usize n = __impl::__ryu::d2e_buffered(val, buf, 350, precision);
  return micron::hstring<C>(buf, buf + n);
}

template <typename C = char>
inline micron::hstring<C>
to_general(f64 val, u32 precision = 6)
{
  char shortest[26];
  usize slen = __impl::__ryu::d2s_buffered(val, shortest);

  // parse exponent from "...E[-]digits" format
  i32 exp10 = 0;
  for ( usize i = 0; i < slen; ++i ) {
    if ( shortest[i] == 'E' ) {
      bool eneg = false;
      usize j = i + 1;
      if ( j < slen && shortest[j] == '-' ) {
        eneg = true;
        ++j;
      }
      while ( j < slen && static_cast<u32>(shortest[j] - '0') <= 9 ) {
        exp10 = exp10 * 10 + (shortest[j] - '0');
        ++j;
      }
      if ( eneg )
        exp10 = -exp10;
      break;
    }
  }

  if ( exp10 < -4 || exp10 >= static_cast<i32>(precision) )
    return to_scientific<C>(val, precision > 0 ? precision - 1 : 0);
  return to_fixed<C>(val, precision);
}

};     // namespace micron
