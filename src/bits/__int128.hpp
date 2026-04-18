// types included here ^^^

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// needed for targets which don't provide a direct __int128 equivalent

#if defined(__micron_arch_width_64)
using uint128_t = unsigned __int128;
using int128_t = __int128;

#else

struct uint128_t;
struct int128_t;

struct uint128_t {
  u64 lo;
  u64 hi;

  constexpr uint128_t() noexcept : lo(0), hi(0) {}

  constexpr uint128_t(u64 v) noexcept : lo(v), hi(0) {}

  constexpr uint128_t(u64 h, u64 l) noexcept : lo(l), hi(h) {}

  constexpr uint128_t(u32 v) noexcept : lo(static_cast<u64>(v)), hi(0) {}

  constexpr uint128_t(int v) noexcept : lo(static_cast<u64>(v)), hi(v < 0 ? ~static_cast<u64>(0) : 0) {}

  explicit constexpr
  operator u64() const noexcept
  {
    return lo;
  }

  explicit constexpr
  operator u32() const noexcept
  {
    return static_cast<u32>(lo);
  }

  explicit constexpr
  operator i64() const noexcept
  {
    return static_cast<i64>(lo);
  }

  explicit constexpr
  operator i32() const noexcept
  {
    return static_cast<i32>(lo);
  }

  explicit constexpr
  operator bool() const noexcept
  {
    return lo != 0 || hi != 0;
  }

  constexpr bool
  operator==(const uint128_t &o) const noexcept
  {
    return hi == o.hi && lo == o.lo;
  }

  constexpr bool
  operator!=(const uint128_t &o) const noexcept
  {
    return hi != o.hi || lo != o.lo;
  }

  constexpr bool
  operator<(const uint128_t &o) const noexcept
  {
    return hi < o.hi || (hi == o.hi && lo < o.lo);
  }

  constexpr bool
  operator>(const uint128_t &o) const noexcept
  {
    return o < *this;
  }

  constexpr bool
  operator<=(const uint128_t &o) const noexcept
  {
    return !(o < *this);
  }

  constexpr bool
  operator>=(const uint128_t &o) const noexcept
  {
    return !(*this < o);
  }

  constexpr uint128_t
  operator~() const noexcept
  {
    return uint128_t(~hi, ~lo);
  }

  constexpr uint128_t
  operator&(const uint128_t &o) const noexcept
  {
    return uint128_t(hi & o.hi, lo & o.lo);
  }

  constexpr uint128_t
  operator|(const uint128_t &o) const noexcept
  {
    return uint128_t(hi | o.hi, lo | o.lo);
  }

  constexpr uint128_t
  operator^(const uint128_t &o) const noexcept
  {
    return uint128_t(hi ^ o.hi, lo ^ o.lo);
  }

  constexpr uint128_t &
  operator&=(const uint128_t &o) noexcept
  {
    hi &= o.hi;
    lo &= o.lo;
    return *this;
  }

  constexpr uint128_t &
  operator|=(const uint128_t &o) noexcept
  {
    hi |= o.hi;
    lo |= o.lo;
    return *this;
  }

  constexpr uint128_t &
  operator^=(const uint128_t &o) noexcept
  {
    hi ^= o.hi;
    lo ^= o.lo;
    return *this;
  }

  constexpr uint128_t
  operator<<(int n) const noexcept
  {
    if ( n == 0 ) return *this;
    if ( n >= 128 ) return uint128_t(0, 0);
    if ( n >= 64 ) return uint128_t(lo << (n - 64), 0);
    return uint128_t((hi << n) | (lo >> (64 - n)), lo << n);
  }

  constexpr uint128_t
  operator>>(int n) const noexcept
  {
    if ( n == 0 ) return *this;
    if ( n >= 128 ) return uint128_t(0, 0);
    if ( n >= 64 ) return uint128_t(0, hi >> (n - 64));
    return uint128_t(hi >> n, (lo >> n) | (hi << (64 - n)));
  }

  constexpr uint128_t &
  operator<<=(int n) noexcept
  {
    *this = *this << n;
    return *this;
  }

  constexpr uint128_t &
  operator>>=(int n) noexcept
  {
    *this = *this >> n;
    return *this;
  }

  constexpr uint128_t
  operator+(const uint128_t &o) const noexcept
  {
    u64 r_lo = lo + o.lo;
    u64 carry = (r_lo < lo) ? 1ULL : 0ULL;
    u64 r_hi = hi + o.hi + carry;
    return uint128_t(r_hi, r_lo);
  }

  constexpr uint128_t &
  operator+=(const uint128_t &o) noexcept
  {
    *this = *this + o;
    return *this;
  }

  constexpr uint128_t &
  operator++() noexcept
  {
    *this += uint128_t(1);
    return *this;
  }

  constexpr uint128_t
  operator++(int) noexcept
  {
    uint128_t tmp = *this;
    ++*this;
    return tmp;
  }

  constexpr uint128_t
  operator-(const uint128_t &o) const noexcept
  {
    u64 r_lo = lo - o.lo;
    u64 borrow = (lo < o.lo) ? 1ULL : 0ULL;
    u64 r_hi = hi - o.hi - borrow;
    return uint128_t(r_hi, r_lo);
  }

  constexpr uint128_t &
  operator-=(const uint128_t &o) noexcept
  {
    *this = *this - o;
    return *this;
  }

  constexpr uint128_t &
  operator--() noexcept
  {
    *this -= uint128_t(1);
    return *this;
  }

  constexpr uint128_t
  operator--(int) noexcept
  {
    uint128_t tmp = *this;
    --*this;
    return tmp;
  }

  static constexpr uint128_t
  __mul64(u64 a, u64 b) noexcept
  {
    const u64 a_lo = static_cast<u32>(a);
    const u64 a_hi = a >> 32;
    const u64 b_lo = static_cast<u32>(b);
    const u64 b_hi = b >> 32;

    const u64 p0 = a_lo * b_lo;
    const u64 p1 = a_lo * b_hi;
    const u64 p2 = a_hi * b_lo;
    const u64 p3 = a_hi * b_hi;

    const u64 mid = (p0 >> 32) + static_cast<u32>(p1) + static_cast<u32>(p2);

    u64 r_lo = (static_cast<u32>(p0)) | (mid << 32);
    u64 r_hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);

    return uint128_t(r_hi, r_lo);
  }

  constexpr uint128_t
  operator*(const uint128_t &o) const noexcept
  {
    uint128_t r = __mul64(lo, o.lo);
    r.hi += lo * o.hi + hi * o.lo;
    return r;
  }

  constexpr uint128_t &
  operator*=(const uint128_t &o) noexcept
  {
    *this = *this * o;
    return *this;
  }

  static constexpr int
  __clz128(uint128_t v) noexcept
  {
    if ( v.hi != 0 ) {
      int n = 0;
      u64 x = v.hi;
      if ( !(x & 0xFFFFFFFF00000000ULL) ) {
        n += 32;
        x <<= 32;
      }
      if ( !(x & 0xFFFF000000000000ULL) ) {
        n += 16;
        x <<= 16;
      }
      if ( !(x & 0xFF00000000000000ULL) ) {
        n += 8;
        x <<= 8;
      }
      if ( !(x & 0xF000000000000000ULL) ) {
        n += 4;
        x <<= 4;
      }
      if ( !(x & 0xC000000000000000ULL) ) {
        n += 2;
        x <<= 2;
      }
      if ( !(x & 0x8000000000000000ULL) ) {
        n += 1;
      }
      return n;
    }
    if ( v.lo != 0 ) {
      int n = 0;
      u64 x = v.lo;
      if ( !(x & 0xFFFFFFFF00000000ULL) ) {
        n += 32;
        x <<= 32;
      }
      if ( !(x & 0xFFFF000000000000ULL) ) {
        n += 16;
        x <<= 16;
      }
      if ( !(x & 0xFF00000000000000ULL) ) {
        n += 8;
        x <<= 8;
      }
      if ( !(x & 0xF000000000000000ULL) ) {
        n += 4;
        x <<= 4;
      }
      if ( !(x & 0xC000000000000000ULL) ) {
        n += 2;
        x <<= 2;
      }
      if ( !(x & 0x8000000000000000ULL) ) {
        n += 1;
      }
      return 64 + n;
    }
    return 128;
  }

  static constexpr void
  __divmod(uint128_t num, uint128_t den, uint128_t &q_out, uint128_t &r_out) noexcept
  {
    if ( !den ) {
      q_out = uint128_t(~0ULL, ~0ULL);
      r_out = uint128_t(0);
      return;
    }

    if ( num < den ) {
      q_out = uint128_t(0);
      r_out = num;
      return;
    }

    if ( num == den ) {
      q_out = uint128_t(1);
      r_out = uint128_t(0);
      return;
    }

    int shift = __clz128(den) - __clz128(num);
    uint128_t d = den << shift;
    uint128_t q(0);

    for ( int i = 0; i <= shift; i++ ) {
      q <<= 1;
      if ( num >= d ) {
        num -= d;
        q.lo |= 1;
      }
      d >>= 1;
    }

    q_out = q;
    r_out = num;
  }

  constexpr uint128_t
  operator/(const uint128_t &o) const noexcept
  {
    uint128_t q, r;
    __divmod(*this, o, q, r);
    return q;
  }

  constexpr uint128_t
  operator%(const uint128_t &o) const noexcept
  {
    uint128_t q, r;
    __divmod(*this, o, q, r);
    return r;
  }

  constexpr uint128_t &
  operator/=(const uint128_t &o) noexcept
  {
    *this = *this / o;
    return *this;
  }

  constexpr uint128_t &
  operator%=(const uint128_t &o) noexcept
  {
    *this = *this % o;
    return *this;
  }
};

struct int128_t {
  uint128_t v;

  constexpr int128_t() noexcept : v() {}

  constexpr int128_t(uint128_t u) noexcept : v(u) {}

  constexpr int128_t(i64 x) noexcept : v(x < 0 ? ~0ULL : 0ULL, static_cast<u64>(x)) {}

  constexpr int128_t(u64 x) noexcept : v(x) {}

  constexpr int128_t(i32 x) noexcept : int128_t(static_cast<i64>(x)) {}

  constexpr int128_t(u32 x) noexcept : v(static_cast<u64>(x)) {}

  constexpr bool
  __is_negative() const noexcept
  {
    return (v.hi & (1ULL << 63)) != 0;
  }

  static constexpr uint128_t
  __negate_u(uint128_t x) noexcept
  {
    return ~x + uint128_t(1);
  }

  static constexpr int128_t
  __negate(int128_t x) noexcept
  {
    return int128_t(__negate_u(x.v));
  }

  constexpr uint128_t
  __abs() const noexcept
  {
    return __is_negative() ? __negate_u(v) : v;
  }

  explicit constexpr
  operator u64() const noexcept
  {
    return v.lo;
  }

  explicit constexpr
  operator i64() const noexcept
  {
    return static_cast<i64>(v.lo);
  }

  explicit constexpr
  operator u32() const noexcept
  {
    return static_cast<u32>(v.lo);
  }

  explicit constexpr
  operator i32() const noexcept
  {
    return static_cast<i32>(v.lo);
  }

  explicit constexpr
  operator bool() const noexcept
  {
    return static_cast<bool>(v);
  }

  explicit constexpr
  operator uint128_t() const noexcept
  {
    return v;
  }

  constexpr bool
  operator==(const int128_t &o) const noexcept
  {
    return v == o.v;
  }

  constexpr bool
  operator!=(const int128_t &o) const noexcept
  {
    return v != o.v;
  }

  constexpr bool
  operator<(const int128_t &o) const noexcept
  {
    bool a_neg = __is_negative();
    bool b_neg = o.__is_negative();
    if ( a_neg != b_neg ) return a_neg;
    return v < o.v;
  }

  constexpr bool
  operator>(const int128_t &o) const noexcept
  {
    return o < *this;
  }

  constexpr bool
  operator<=(const int128_t &o) const noexcept
  {
    return !(o < *this);
  }

  constexpr bool
  operator>=(const int128_t &o) const noexcept
  {
    return !(*this < o);
  }

  constexpr int128_t
  operator~() const noexcept
  {
    return int128_t(~v);
  }

  constexpr int128_t
  operator&(const int128_t &o) const noexcept
  {
    return int128_t(v & o.v);
  }

  constexpr int128_t
  operator|(const int128_t &o) const noexcept
  {
    return int128_t(v | o.v);
  }

  constexpr int128_t
  operator^(const int128_t &o) const noexcept
  {
    return int128_t(v ^ o.v);
  }

  constexpr int128_t &
  operator&=(const int128_t &o) noexcept
  {
    v &= o.v;
    return *this;
  }

  constexpr int128_t &
  operator|=(const int128_t &o) noexcept
  {
    v |= o.v;
    return *this;
  }

  constexpr int128_t &
  operator^=(const int128_t &o) noexcept
  {
    v ^= o.v;
    return *this;
  }

  constexpr int128_t
  operator<<(int n) const noexcept
  {
    return int128_t(v << n);
  }

  constexpr int128_t
  operator>>(int n) const noexcept
  {
    if ( n == 0 ) return *this;
    if ( !__is_negative() ) return int128_t(v >> n);
    uint128_t shifted = v >> n;
    uint128_t mask;
    if ( n >= 128 )
      mask = uint128_t(~0ULL, ~0ULL);
    else
      mask = ~(uint128_t(~0ULL, ~0ULL) >> n);
    return int128_t(shifted | mask);
  }

  constexpr int128_t &
  operator<<=(int n) noexcept
  {
    *this = *this << n;
    return *this;
  }

  constexpr int128_t &
  operator>>=(int n) noexcept
  {
    *this = *this >> n;
    return *this;
  }

  constexpr int128_t
  operator+(const int128_t &o) const noexcept
  {
    return int128_t(v + o.v);
  }

  constexpr int128_t
  operator-(const int128_t &o) const noexcept
  {
    return int128_t(v - o.v);
  }

  constexpr int128_t &
  operator+=(const int128_t &o) noexcept
  {
    v += o.v;
    return *this;
  }

  constexpr int128_t &
  operator-=(const int128_t &o) noexcept
  {
    v -= o.v;
    return *this;
  }

  constexpr int128_t &
  operator++() noexcept
  {
    v += uint128_t(1);
    return *this;
  }

  constexpr int128_t
  operator++(int) noexcept
  {
    int128_t t = *this;
    ++*this;
    return t;
  }

  constexpr int128_t &
  operator--() noexcept
  {
    v -= uint128_t(1);
    return *this;
  }

  constexpr int128_t
  operator--(int) noexcept
  {
    int128_t t = *this;
    --*this;
    return t;
  }

  // Unary minus
  constexpr int128_t
  operator-() const noexcept
  {
    return __negate(*this);
  }

  constexpr int128_t
  operator*(const int128_t &o) const noexcept
  {
    return int128_t(v * o.v);
  }

  constexpr int128_t &
  operator*=(const int128_t &o) noexcept
  {
    *this = *this * o;
    return *this;
  }

  constexpr int128_t
  operator/(const int128_t &o) const noexcept
  {
    bool neg_a = __is_negative();
    bool neg_b = o.__is_negative();
    uint128_t ua = neg_a ? __negate_u(v) : v;
    uint128_t ub = neg_b ? __negate_u(o.v) : o.v;
    uint128_t q = ua / ub;
    return (neg_a != neg_b) ? int128_t(__negate_u(q)) : int128_t(q);
  }

  constexpr int128_t
  operator%(const int128_t &o) const noexcept
  {
    bool neg_a = __is_negative();
    bool neg_b = o.__is_negative();
    uint128_t ua = neg_a ? __negate_u(v) : v;
    uint128_t ub = neg_b ? __negate_u(o.v) : o.v;
    uint128_t r = ua % ub;
    return neg_a ? int128_t(__negate_u(r)) : int128_t(r);
  }

  constexpr int128_t &
  operator/=(const int128_t &o) noexcept
  {
    *this = *this / o;
    return *this;
  }

  constexpr int128_t &
  operator%=(const int128_t &o) noexcept
  {
    *this = *this % o;
    return *this;
  }
};

#endif
