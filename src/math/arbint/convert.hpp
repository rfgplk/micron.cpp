//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../strings.hpp"
#include "../../types.hpp"
#include "div.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "signed.hpp"
#include "unsigned.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// base 2..36, in and out
//
// a power of two base is bit slicing; every other base divides by the
// largest power of the base that fits one limb (10^19 on a 64-bit limb)
// one divrem_1 yields 19 decimal digits instead of one

namespace micron
{
namespace math
{
namespace __arb
{

inline constexpr const char digits_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
inline constexpr const char digits_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

struct base_pow {
  mpn::limb_t big;      // the largest power of base that still fits a limb
  u32 per;              // how many digits that power is worth
};

[[nodiscard, gnu::flatten]] inline constexpr base_pow
largest_power(u32 base) noexcept
{
  mpn::limb_t big = base;
  u32 per = 1;
  while ( big <= mpn::limb_max / base ) {
    big = static_cast<mpn::limb_t>(big * base);
    ++per;
  }
  return { big, per };
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
digit_value(char c) noexcept
{
  if ( c >= '0' && c <= '9' ) return static_cast<u32>(c - '0');
  if ( c >= 'a' && c <= 'z' ) return static_cast<u32>(c - 'a') + 10u;
  if ( c >= 'A' && c <= 'Z' ) return static_cast<u32>(c - 'A') + 10u;
  return 0xFFFFFFFFu;
}

// a squared tower cannot need more entries than a value has bits, so 64 is past any real input
inline constexpr usize max_tower = 64;

[[nodiscard, gnu::always_inline]] inline constexpr bool
is_pow2_base(u32 base) noexcept
{
  return base >= 2u && (base & (base - 1u)) == 0u;
}

[[gnu::flatten]] inline char *
emit_digits(char *end, mpn::limb_t *work, usize n, u32 base, bool upper) noexcept
{
  const char *const tab = upper ? digits_upper : digits_lower;
  char *p = end;

  if ( is_pow2_base(base) ) {
    u32 bits = 0;
    for ( u32 b = base; b > 1u; b >>= 1 ) ++bits;
    const usize total = mpn::bitlen(work, n);
    for ( usize at = 0; at < total; at += bits ) {
      const usize i = at / mpn::limb_bits;
      const usize off = at % mpn::limb_bits;
      mpn::limb_t v = static_cast<mpn::limb_t>(work[i] >> off);
      if ( off + bits > mpn::limb_bits && i + 1u < n ) v |= static_cast<mpn::limb_t>(work[i + 1u] << (mpn::limb_bits - off));
      *--p = tab[static_cast<usize>(v & static_cast<mpn::limb_t>(base - 1u))];
    }
    return p;
  }

  const base_pow bp = largest_power(base);
  while ( n > 0 ) {
    const mpn::limb_t rem = mpn::divrem_1(work, work, n, bp.big);
    n = mpn::normalize(work, n);
    mpn::limb_t v = rem;
    if ( n == 0 ) {
      do {
        *--p = tab[static_cast<usize>(v % base)];
        v /= base;
      } while ( v != 0 );
    } else {
      for ( u32 k = 0; k < bp.per; ++k ) {
        *--p = tab[static_cast<usize>(v % base)];
        v /= base;
      }
    }
  }
  return p;
}

};      // namespace __arb

// converter fns
template<usize B, arb_solver S, class A>
[[nodiscard]] inline usize
to_chars(char *buf, usize cap, const arbuint<B, S, A> &a, u32 base = 10u, bool upper = false)
{
  if ( base < 2u || base > 36u || cap == 0 ) return 0;
  if ( a.is_zero() ) {
    if ( cap < 1u ) return 0;
    buf[0] = '0';
    return 1;
  }
  const usize n = a.size();
  const usize want = a.bit_length();
  if ( cap < want ) return 0;

  __arb::scratch<(B != 0) ? arbuint<B, S, A>::cap_limbs : 1u, A, arbuint<B, S, A>::bounded> sc(n);
  mpn::limb_t *work = sc.get();
  mpn::copyi(work, a.limbs(), n);

  char *const end = buf + want;
  char *const first = __arb::emit_digits(end, work, n, base, upper);
  const usize len = static_cast<usize>(end - first);
  for ( usize i = 0; i < len; ++i ) buf[i] = first[i];
  return len;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline usize
to_chars(char *buf, usize cap, const arbint<B, S, A> &a, u32 base = 10u, bool upper = false)
{
  if ( !a.negative() ) return to_chars(buf, cap, a.magnitude(), base, upper);
  if ( cap == 0 ) return 0;
  buf[0] = '-';
  const usize k = to_chars(buf + 1, cap - 1u, a.magnitude(), base, upper);
  return k == 0 ? 0 : k + 1u;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline bool
from_chars_basecase(arbuint<B, S, A> &out, const char *p, usize n, u32 base = 10u)
{
  if ( base < 2u || base > 36u || n == 0 ) return false;
  out.set_zero();

  const __arb::base_pow bp = __arb::largest_power(base);
  usize i = 0;
  const usize head = n % bp.per;

  if ( head != 0 ) {
    mpn::limb_t chunk = 0;
    for ( ; i < head; ++i ) {
      const u32 d = __arb::digit_value(p[i]);
      if ( d >= base ) return false;
      chunk = static_cast<mpn::limb_t>(chunk * base + d);
    }
    out.__mul_add_1(1, chunk);
  }
  while ( i < n ) {
    mpn::limb_t chunk = 0;
    for ( u32 k = 0; k < bp.per; ++k, ++i ) {
      const u32 d = __arb::digit_value(p[i]);
      if ( d >= base ) return false;
      chunk = static_cast<mpn::limb_t>(chunk * base + d);
    }
    out.__mul_add_1(bp.big, chunk);
  }
  return true;
}

namespace __arb
{

template<class U>
[[nodiscard]] inline bool
parse_dc(U &out, const char *p, usize n, u32 base, const U *tower, const usize *digits, usize level)
{
  if ( level == 0 || n <= digits[0] || n < mpn::threshold::set_str_dc ) return from_chars_basecase(out, p, n, base);

  usize lv = level;
  while ( lv > 0 && digits[lv] >= n ) --lv;      // the largest split strictly inside the string
  if ( lv == 0 && digits[0] >= n ) return from_chars_basecase(out, p, n, base);

  const usize low = digits[lv];
  U hi, lo;
  if ( !parse_dc(hi, p, n - low, base, tower, digits, lv) ) return false;
  if ( !parse_dc(lo, p + (n - low), low, base, tower, digits, lv) ) return false;
  hi *= tower[lv];
  hi += lo;
  out = hi;
  return true;
}

};      // namespace __arb

template<usize B, arb_solver S, class A>
[[nodiscard]] inline bool
from_chars(arbuint<B, S, A> &out, const char *p, usize n, u32 base = 10u)
{
  using U = arbuint<B, S, A>;
  if ( base < 2u || base > 36u || n == 0 ) return false;
  if ( n < mpn::threshold::set_str_dc || __arb::is_pow2_base(base) ) return from_chars_basecase(out, p, n, base);

  // reject before building anything: a tower is wasted work if the input is malformed
  for ( usize i = 0; i < n; ++i )
    if ( __arb::digit_value(p[i]) >= base ) return false;

  U tower[__arb::max_tower];
  usize digits[__arb::max_tower];
  const __arb::base_pow bp = __arb::largest_power(base);
  tower[0] = U(bp.big);
  digits[0] = bp.per;
  usize top = 0;
  while ( top + 1u < __arb::max_tower && digits[top] * 2u < n ) {
    tower[top + 1u] = micron::math::sqr(tower[top]);
    digits[top + 1u] = digits[top] * 2u;
    ++top;
  }
  return __arb::parse_dc(out, p, n, base, tower, digits, top);
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline bool
from_chars(arbint<B, S, A> &out, const char *p, usize n, u32 base = 10u)
{
  if ( n == 0 ) return false;
  bool neg = false;
  usize i = 0;
  if ( p[0] == '-' || p[0] == '+' ) {
    neg = p[0] == '-';
    i = 1;
    if ( n == 1 ) return false;
  }
  typename arbint<B, S, A>::mag_type mag;
  if ( !from_chars(mag, p + i, n - i, base) ) return false;
  out = arbint<B, S, A>(mag, neg);
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// divide and conquer formatting
//
// tower[i] = base^(digits[i]) with digits[i] = digits[0] * 2^i
namespace __arb
{

inline void
emit_padded_basecase(char *out, usize width, mpn::limb_t *work, usize n, u32 base, bool upper) noexcept
{
  char *const end = out + width;
  char *const first = emit_digits(end, work, n, base, upper);
  for ( char *p = out; p < first; ++p ) *p = '0';
}

template<class U>
inline void
emit_padded(char *out, usize width, const U &x, u32 base, bool upper, const U *tower, const usize *digits, usize level)
{
  if ( level == 0 || x.size() < mpn::threshold::get_str_dc ) {
    micron::vector<mpn::limb_t, typename U::allocator_type, false> work(x.size() + 1u);
    mpn::copyi(work.data(), x.limbs(), x.size());
    emit_padded_basecase(out, width, work.data(), x.size(), base, upper);
    return;
  }
  const usize low = digits[level];
  if ( low >= width ) {      // the split cannot help at this width; drop a level
    emit_padded(out, width, x, base, upper, tower, digits, level - 1u);
    return;
  }
  const auto qr = divmod(x, tower[level]);
  emit_padded(out, width - low, qr.quot, base, upper, tower, digits, level - 1u);
  emit_padded(out + (width - low), low, qr.rem, base, upper, tower, digits, level - 1u);
}

};      // namespace __arb

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// string porcelain

template<usize B, arb_solver S, class A>
[[nodiscard]] inline micron::string
to_string(const arbuint<B, S, A> &a, u32 base = 10u, bool upper = false)
{
  using U = arbuint<B, S, A>;
  const usize cap = a.bit_length() + 3u;
  micron::vector<char, A, false> buf(cap);

  // below the threshold, or in a base that is pure bit slicing
  if ( a.size() < mpn::threshold::get_str_dc || __arb::is_pow2_base(base) || base < 2u || base > 36u ) {
    const usize len = to_chars(buf.data(), cap - 1u, a, base, upper);
    buf.data()[len] = '\0';
    return micron::string(buf.data());
  }

  // square a tower of base powers until one exceeds the value
  U tower[__arb::max_tower];
  usize digits[__arb::max_tower];
  const __arb::base_pow bp = __arb::largest_power(base);
  tower[0] = U(bp.big);
  digits[0] = bp.per;
  usize top = 0;
  while ( top + 1u < __arb::max_tower && tower[top] <= a ) {
    tower[top + 1u] = micron::math::sqr(tower[top]);
    digits[top + 1u] = digits[top] * 2u;
    ++top;
  }
  // tower[top] > a, so the whole value fits inside digits[top] characters
  const usize width = digits[top];
  if ( width + 1u > cap ) {      // cannot happen for base >= 2, but never write past the buffer
    const usize len = to_chars(buf.data(), cap - 1u, a, base, upper);
    buf.data()[len] = '\0';
    return micron::string(buf.data());
  }

  __arb::emit_padded(buf.data(), width, a, base, upper, tower, digits, top);

  // strip the one leading run of zeros the padding introduced
  usize lead = 0;
  while ( lead + 1u < width && buf.data()[lead] == '0' ) ++lead;
  for ( usize i = 0; i + lead < width; ++i ) buf.data()[i] = buf.data()[i + lead];
  buf.data()[width - lead] = '\0';
  return micron::string(buf.data());
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline micron::string
to_string(const arbint<B, S, A> &a, u32 base = 10u, bool upper = false)
{
  const usize cap = a.bit_length() + 3u;
  micron::vector<char, A, false> buf(cap);
  const usize len = to_chars(buf.data(), cap - 1u, a, base, upper);
  buf.data()[len] = '\0';
  return micron::string(buf.data());
}

};      // namespace math
};      // namespace micron
