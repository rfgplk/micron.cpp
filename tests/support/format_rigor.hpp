//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// format_rigor.hpp — shared infrastructure for the rigor_format_* suite.
//
// Provides:
//   * adversarial char/string patterns (control, all-digits, all-spaces,
//     mixed, near-max-length, embedded-nul)
//   * naive reference implementations of ctype predicates (matching the
//     ASCII spec but written as obvious loops, used as oracles)
//   * naive integer/float-to-string and parse routines for round-trip
//     property tests
//   * random-int / random-float / random-string generators
//   * string container factories (hstring, sstring<N>, istring)
//   * prop_round_trip wrappers
//
// Header-only, no STL / libc; depends only on micron + snowball + oracles.

#include "../../src/concepts.hpp"
#include "../../src/math/generic.hpp"
#include "../../src/std.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/type_traits.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

#include "oracles.hpp"

namespace mtest::format_rigor
{

// ─────────────────────────────────────────────────────────────────────────
// Naive ctype references — bit-for-bit matches the ASCII spec.
//
// Used as the ground-truth oracle for property tests of micron's
// format::is* predicates.
// ─────────────────────────────────────────────────────────────────────────

namespace ref
{

inline constexpr bool
isupper(char c)
{
  return c >= 'A' && c <= 'Z';
}

inline constexpr bool
islower(char c)
{
  return c >= 'a' && c <= 'z';
}

inline constexpr bool
isalpha(char c)
{
  return isupper(c) || islower(c);
}

inline constexpr bool
isdigit(char c)
{
  return c >= '0' && c <= '9';
}

inline constexpr bool
isalnum(char c)
{
  return isalpha(c) || isdigit(c);
}

inline constexpr bool
isxdigit(char c)
{
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline constexpr bool
isspace(char c)
{
  return c == ' ' || (c >= 0x09 && c <= 0x0D);
}

inline constexpr bool
isblank(char c)
{
  return c == ' ' || c == '\t';
}

inline constexpr bool
iscntrl(char c)
{
  // Treat as unsigned to avoid sign-extension surprises on platforms
  // where char is signed.
  unsigned char u = static_cast<unsigned char>(c);
  return u <= 0x1F || u == 0x7F;
}

inline constexpr bool
isprint(char c)
{
  unsigned char u = static_cast<unsigned char>(c);
  return u >= 0x20 && u <= 0x7E;
}

inline constexpr bool
isgraph(char c)
{
  unsigned char u = static_cast<unsigned char>(c);
  return u >= 0x21 && u <= 0x7E;
}

inline constexpr bool
ispunct(char c)
{
  return isprint(c) && !isalnum(c) && !isspace(c);
}

inline constexpr bool
isascii(char c)
{
  unsigned char u = static_cast<unsigned char>(c);
  return u <= 0x7F;
}

inline constexpr char
to_upper(char c)
{
  return islower(c) ? static_cast<char>(c - 32) : c;
}

inline constexpr char
to_lower(char c)
{
  return isupper(c) ? static_cast<char>(c + 32) : c;
}

// ── integer → string (decimal, signed) ────────────────────────────────
inline usize
naive_i64_to_dec(i64 v, char *buf, usize cap)
{
  if ( cap == 0 ) return 0;
  bool neg = v < 0;
  u64 uv;
  if ( neg ) {
    // safe negation for INT64_MIN via two's-complement bit math
    uv = static_cast<u64>(-(v + 1)) + 1u;
  } else {
    uv = static_cast<u64>(v);
  }
  char tmp[32];
  usize n = 0;
  if ( uv == 0 ) tmp[n++] = '0';
  while ( uv > 0 ) {
    tmp[n++] = static_cast<char>('0' + (uv % 10));
    uv /= 10;
  }
  usize out = 0;
  if ( neg && out < cap ) buf[out++] = '-';
  while ( n > 0 && out < cap ) buf[out++] = tmp[--n];
  return out;
}

inline usize
naive_u64_to_dec(u64 v, char *buf, usize cap)
{
  char tmp[32];
  usize n = 0;
  if ( v == 0 ) tmp[n++] = '0';
  while ( v > 0 ) {
    tmp[n++] = static_cast<char>('0' + (v % 10));
    v /= 10;
  }
  usize out = 0;
  while ( n > 0 && out < cap ) buf[out++] = tmp[--n];
  return out;
}

inline usize
naive_u64_to_hex(u64 v, char *buf, usize cap, bool upper = false)
{
  char tmp[32];
  usize n = 0;
  if ( v == 0 ) tmp[n++] = '0';
  const char a = upper ? 'A' : 'a';
  while ( v > 0 ) {
    u8 nib = static_cast<u8>(v & 0xfu);
    tmp[n++] = nib < 10 ? static_cast<char>('0' + nib) : static_cast<char>(a + (nib - 10));
    v >>= 4;
  }
  usize out = 0;
  while ( n > 0 && out < cap ) buf[out++] = tmp[--n];
  return out;
}

inline usize
naive_u64_to_oct(u64 v, char *buf, usize cap)
{
  char tmp[32];
  usize n = 0;
  if ( v == 0 ) tmp[n++] = '0';
  while ( v > 0 ) {
    tmp[n++] = static_cast<char>('0' + (v & 7u));
    v >>= 3;
  }
  usize out = 0;
  while ( n > 0 && out < cap ) buf[out++] = tmp[--n];
  return out;
}

inline usize
naive_u64_to_bin(u64 v, char *buf, usize cap)
{
  char tmp[64];
  usize n = 0;
  if ( v == 0 ) tmp[n++] = '0';
  while ( v > 0 ) {
    tmp[n++] = static_cast<char>('0' + (v & 1u));
    v >>= 1;
  }
  usize out = 0;
  while ( n > 0 && out < cap ) buf[out++] = tmp[--n];
  return out;
}

// ── string → integer (parse) ────────────────────────────────────────────
// Parses [s, s+n) as decimal, with optional leading '-' or '+'. Returns
// the parsed value and (via out_consumed) the number of chars consumed.
// Stops on first non-digit; partial parses set ok=false only if no digit
// was seen.
inline i64
naive_parse_i64(const char *s, usize n, bool *ok = nullptr)
{
  usize i = 0;
  bool neg = false;
  if ( i < n && (s[i] == '-' || s[i] == '+') ) {
    neg = (s[i] == '-');
    ++i;
  }
  bool any = false;
  u64 val = 0;
  while ( i < n && isdigit(s[i]) ) {
    val = val * 10u + static_cast<u64>(s[i] - '0');
    ++i;
    any = true;
  }
  if ( ok ) *ok = any;
  if ( neg ) return -static_cast<i64>(val);
  return static_cast<i64>(val);
}

inline u64
naive_parse_u64(const char *s, usize n, bool *ok = nullptr)
{
  usize i = 0;
  bool any = false;
  u64 val = 0;
  while ( i < n && isdigit(s[i]) ) {
    val = val * 10u + static_cast<u64>(s[i] - '0');
    ++i;
    any = true;
  }
  if ( ok ) *ok = any;
  return val;
}

inline u64
naive_parse_hex_u64(const char *s, usize n, bool *ok = nullptr)
{
  usize i = 0;
  bool any = false;
  u64 val = 0;
  while ( i < n && isxdigit(s[i]) ) {
    int nib;
    char c = s[i];
    if ( c >= '0' && c <= '9' )
      nib = c - '0';
    else if ( c >= 'a' && c <= 'f' )
      nib = c - 'a' + 10;
    else
      nib = c - 'A' + 10;
    val = (val << 4) | static_cast<u64>(nib);
    ++i;
    any = true;
  }
  if ( ok ) *ok = any;
  return val;
}

};      // namespace ref

// ─────────────────────────────────────────────────────────────────────────
// String / character generators
// ─────────────────────────────────────────────────────────────────────────

inline char
rand_ascii_print(prng &rng)
{
  // [0x20, 0x7E]
  return static_cast<char>(0x20 + (rng.next() % (0x7F - 0x20)));
}

inline char
rand_alpha(prng &rng)
{
  u64 r = rng.next() & 0x1f;      // 0..31
  if ( r >= 26 ) return rand_alpha(rng);
  bool upper = (rng.next() & 1u);
  return static_cast<char>(upper ? ('A' + r) : ('a' + r));
}

inline char
rand_digit(prng &rng)
{
  return static_cast<char>('0' + (rng.next() % 10));
}

inline char
rand_alnum(prng &rng)
{
  u64 r = rng.next() % 62u;
  if ( r < 10 ) return static_cast<char>('0' + r);
  if ( r < 36 ) return static_cast<char>('A' + (r - 10));
  return static_cast<char>('a' + (r - 36));
}

inline char
rand_byte(prng &rng)
{
  return static_cast<char>(rng.next() & 0xff);
}

inline i64
rand_i64(prng &rng)
{
  u64 r = rng.next();
  return static_cast<i64>(r);
}

inline i32
rand_i32(prng &rng)
{
  u32 r = static_cast<u32>(rng.next());
  return static_cast<i32>(r);
}

inline u64
rand_u64(prng &rng)
{
  return rng.next();
}

inline u32
rand_u32(prng &rng)
{
  return static_cast<u32>(rng.next());
}

inline f64
rand_f64(prng &rng)
{
  // produce a wide range of doubles by xor-mixing the bits
  u64 bits = rng.next();
  // mask out NaN/inf exponent (all-ones) to keep tests away from those
  // edge cases by default
  bits &= 0x7ff7'ffff'ffff'ffffULL;
  f64 v;
  __builtin_memcpy(&v, &bits, 8);
  return v;
}

inline f64
rand_f64_small(prng &rng, f64 lo = -1e6, f64 hi = 1e6)
{
  u64 r = rng.next();
  f64 u = static_cast<f64>(static_cast<u32>(r) & 0xffffff) / static_cast<f64>(0x1000000);
  return lo + u * (hi - lo);
}

inline void
fill_random_string(char *buf, usize n, prng &rng)
{
  for ( usize i = 0; i < n; ++i ) buf[i] = rand_ascii_print(rng);
}

inline void
fill_random_alpha(char *buf, usize n, prng &rng)
{
  for ( usize i = 0; i < n; ++i ) buf[i] = rand_alpha(rng);
}

inline void
fill_random_digits(char *buf, usize n, prng &rng)
{
  for ( usize i = 0; i < n; ++i ) buf[i] = rand_digit(rng);
}

inline void
fill_random_bytes(char *buf, usize n, prng &rng)
{
  for ( usize i = 0; i < n; ++i ) buf[i] = rand_byte(rng);
}

// ─────────────────────────────────────────────────────────────────────────
// Adversarial char patterns
// ─────────────────────────────────────────────────────────────────────────

inline constexpr char kAllAsciiBoundaries[] = {
  static_cast<char>(0x00), static_cast<char>(0x07), static_cast<char>(0x08), static_cast<char>(0x09), static_cast<char>(0x0A),
  static_cast<char>(0x0D), static_cast<char>(0x1F), static_cast<char>(0x20), static_cast<char>(0x2F), static_cast<char>(0x30),
  static_cast<char>(0x39), static_cast<char>(0x3A), static_cast<char>(0x40), static_cast<char>(0x41), static_cast<char>(0x46),
  static_cast<char>(0x47), static_cast<char>(0x5A), static_cast<char>(0x5B), static_cast<char>(0x60), static_cast<char>(0x61),
  static_cast<char>(0x66), static_cast<char>(0x67), static_cast<char>(0x7A), static_cast<char>(0x7B), static_cast<char>(0x7E),
  static_cast<char>(0x7F), static_cast<char>(0x80), static_cast<char>(0xFE), static_cast<char>(0xFF),
};

inline constexpr usize kAllAsciiBoundariesCount = sizeof(kAllAsciiBoundaries) / sizeof(kAllAsciiBoundaries[0]);

inline constexpr usize kAdversarialStringSizes[]
    = { 0u, 1u, 2u, 3u, 7u, 8u, 15u, 16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 127u, 128u, 129u, 255u, 256u, 1023u };
inline constexpr usize kAdversarialStringSizesCount = sizeof(kAdversarialStringSizes) / sizeof(kAdversarialStringSizes[0]);

// Common integer-boundary values for round-trip testing.
inline constexpr i64 kSignedBoundaries[] = {
  0,
  1,
  -1,
  2,
  -2,
  9,
  10,
  -9,
  -10,
  99,
  100,
  -99,
  -100,
  999,
  1000,
  -999,
  9999,
  -9999,
  65535,
  -65535,
  65536,
  -65536,
  0x7fffffffLL,
  -0x7fffffffLL,
  0x7ffffffffffffffeLL,
  -0x7ffffffffffffffeLL,
  0x7fffffffffffffffLL,
  -0x7fffffffffffffffLL - 1,      // INT64_MAX / MIN
};
inline constexpr usize kSignedBoundariesCount = sizeof(kSignedBoundaries) / sizeof(kSignedBoundaries[0]);

inline constexpr u64 kUnsignedBoundaries[] = {
  0u,
  1u,
  9u,
  10u,
  99u,
  100u,
  999u,
  1000u,
  65535u,
  65536u,
  0xffffffffu,
  0x100000000ULL,
  0xfffffffffffffffeULL,
  0xffffffffffffffffULL,      // UINT64_MAX
};
inline constexpr usize kUnsignedBoundariesCount = sizeof(kUnsignedBoundaries) / sizeof(kUnsignedBoundaries[0]);

inline constexpr f64 kFloatBoundaries[] = {
  0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 0.1, -0.1, 3.14159265358979, -3.14159265358979, 1e-10, 1e10, 1e-100, 1e100, 1e300, -1e300,
};
inline constexpr usize kFloatBoundariesCount = sizeof(kFloatBoundaries) / sizeof(kFloatBoundaries[0]);

// ─────────────────────────────────────────────────────────────────────────
// String container builders
// ─────────────────────────────────────────────────────────────────────────

using hstr = micron::hstring<schar>;
template<usize N> using sstr = micron::sstring<N, schar>;

inline hstr
mk_hstring(const char *p, usize n)
{
  hstr s;
  for ( usize i = 0; i < n; ++i ) s += p[i];
  return s;
}

inline hstr
mk_hstring(const char *p)
{
  return hstr(p);
}

template<usize N>
inline sstr<N>
mk_sstring(const char *p, usize n)
{
  sstr<N> s;
  usize lim = n < N - 1 ? n : N - 1;
  for ( usize i = 0; i < lim; ++i ) s += p[i];
  return s;
}

// Compare two byte ranges for exact match.
inline bool
bytes_equal(const char *a, usize an, const char *b, usize bn)
{
  if ( an != bn ) return false;
  for ( usize i = 0; i < an; ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

inline bool
hstr_equal_bytes(const hstr &s, const char *bytes, usize n)
{
  if ( s.size() != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( s[i] != bytes[i] ) return false;
  return true;
}

inline bool
hstr_equal_cstr(const hstr &s, const char *cstr)
{
  usize n = micron::strlen(cstr);
  return hstr_equal_bytes(s, cstr, n);
}

};      // namespace mtest::format_rigor
