//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Shared oracle + generators for the exhaustive, adversarial vector rigor
// suites (rigor_vector / rigor_svector / rigor_fvector / rigor_ivector /
// rigor_pvector / rigor_convector / rigor_circle_vector). Modeled on
// tests/support/string_rigor.hpp.
//
// The oracle `ref_vec` is a flat, obviously-correct (linear, no SIMD, no
// allocator games) reference against which micron's vector classes are diffed
// per operation. STL is forbidden (CLAUDE.md) so everything is hand-rolled on
// top of micron::types only.
//
// Key-projection design: vector elements may be non-trivial / move-only /
// throwing, so (unlike the string oracle which stored T directly) the oracle
// stores an integer "key" (u64) per element, and every element type E provides
// elem<E>::make(key)->E and elem<E>::key(const E&)->u64. The comparator diffs
// elem<E>::key(v[i]) against the oracle's stored key. make() truncates to E and
// key() reads it back, so a round-trip is exact for every element type.
//
// Conventions:
//   * VREF_NPOS == ~usize(0) is the oracle's "not found" sentinel (the classes
//     report a miss as nullptr, so callers translate before diffing).
//   * generators draw from mtest::prng (oracles.hpp); seed for reproducibility.

#include "../../src/types.hpp"

#include "oracles.hpp"
#include "tracked_types.hpp"

namespace mtest
{

inline constexpr usize VREF_NPOS = ~static_cast<usize>(0);

enum class band : int { full, small };

struct big {
  u64 a, b, c;

  bool
  operator==(const big &o) const noexcept
  {
    return a == o.a && b == o.b && c == o.c;
  }

  bool
  operator!=(const big &o) const noexcept
  {
    return !(*this == o);
  }

  bool
  operator>(const big &o) const noexcept
  {
    return a > o.a;
  }

  bool
  operator<(const big &o) const noexcept
  {
    return a < o.a;
  }

  bool
  operator>=(const big &o) const noexcept
  {
    return a >= o.a;
  }

  bool
  operator<=(const big &o) const noexcept
  {
    return a <= o.a;
  }
};

struct move_only {
  int v = 0;

  move_only() noexcept : v(0) { }

  explicit move_only(int x) noexcept : v(x) { }

  move_only(const move_only &) = delete;
  move_only &operator=(const move_only &) = delete;

  move_only(move_only &&o) noexcept : v(o.v) { o.v = 0; }

  move_only &
  operator=(move_only &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  ~move_only() = default;

  bool
  operator==(const move_only &o) const noexcept
  {
    return v == o.v;
  }

  bool
  operator>(const move_only &o) const noexcept
  {
    return v > o.v;
  }

  bool
  operator<(const move_only &o) const noexcept
  {
    return v < o.v;
  }
};

template<typename E> struct elem;

template<typename E>
  requires micron::is_integral_v<E>
struct elem<E> {
  static E
  make(u64 k) noexcept
  {
    return static_cast<E>(k);
  }

  static u64
  key(const E &e) noexcept
  {
    return static_cast<u64>(e);
  }

  template<typename V>
  static void
  emplace(V &v, u64 k)
  {
    v.emplace_back(static_cast<E>(k));
  }

  template<typename IV>
  static IV
  emplace_iv(const IV &v, u64 k)
  {
    return v.emplace_back(static_cast<E>(k));
  }
};

template<> struct elem<big> {
  static big
  make(u64 k) noexcept
  {
    return big{ k, k ^ 0x9e3779b97f4a7c15ull, ~k };
  }

  static u64
  key(const big &e) noexcept
  {
    return e.a;
  }

  template<typename V>
  static void
  emplace(V &v, u64 k)
  {
    v.emplace_back(k, k ^ 0x9e3779b97f4a7c15ull, ~k);
  }

  template<typename IV>
  static IV
  emplace_iv(const IV &v, u64 k)
  {
    return v.emplace_back(k, k ^ 0x9e3779b97f4a7c15ull, ~k);
  }
};

template<> struct elem<move_only> {
  static move_only
  make(u64 k) noexcept
  {
    return move_only(static_cast<int>(k & 0xffffffu));
  }

  static u64
  key(const move_only &e) noexcept
  {
    return static_cast<u64>(static_cast<u32>(e.v));
  }

  template<typename V>
  static void
  emplace(V &v, u64 k)
  {
    v.emplace_back(static_cast<int>(k & 0xffffffu));
  }

  template<typename IV>
  static IV
  emplace_iv(const IV &v, u64 k)
  {
    return v.emplace_back(static_cast<int>(k & 0xffffffu));
  }
};

template<int Tag> struct elem<Tracked<Tag>> {
  static Tracked<Tag>
  make(u64 k) noexcept
  {
    return Tracked<Tag>(static_cast<int>(k & 0xffffffu));
  }

  static u64
  key(const Tracked<Tag> &e) noexcept
  {
    return static_cast<u64>(static_cast<u32>(e.v));
  }

  template<typename V>
  static void
  emplace(V &v, u64 k)
  {
    v.emplace_back(static_cast<int>(k & 0xffffffu));
  }

  template<typename IV>
  static IV
  emplace_iv(const IV &v, u64 k)
  {
    return v.emplace_back(static_cast<int>(k & 0xffffffu));
  }
};

template<throw_on Trait, int Tag> struct elem<Throwing<Trait, Tag>> {
  static Throwing<Trait, Tag>
  make(u64 k)
  {
    return Throwing<Trait, Tag>(static_cast<int>(k & 0xffffffu));
  }

  static u64
  key(const Throwing<Trait, Tag> &e) noexcept
  {
    return static_cast<u64>(static_cast<u32>(e.v));
  }

  template<typename V>
  static void
  emplace(V &v, u64 k)
  {
    v.emplace_back(static_cast<int>(k & 0xffffffu));
  }

  template<typename IV>
  static IV
  emplace_iv(const IV &v, u64 k)
  {
    return v.emplace_back(static_cast<int>(k & 0xffffffu));
  }
};

inline u64
gen_raw(prng &rng, band b) noexcept
{
  const u64 r = rng.next();
  return (b == band::small) ? (r % 8u) : r;
}

inline void
gen_count(prng &rng, usize &n, usize maxlen) noexcept
{
  n = (maxlen == 0) ? 0 : static_cast<usize>(rng.next() % (maxlen + 1));
}

inline constexpr usize kVecLens[] = { 0, 1, 2, 3, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 1023 };
inline constexpr usize kVecLensCount = sizeof(kVecLens) / sizeof(kVecLens[0]);

template<usize Cap = 4096> struct ref_vec {
  u64 buf[Cap];
  usize len = 0;

  void
  clear(void) noexcept
  {
    len = 0;
  }

  usize
  size(void) const noexcept
  {
    return len;
  }

  bool
  empty(void) const noexcept
  {
    return len == 0;
  }

  static constexpr usize
  capacity(void) noexcept
  {
    return Cap;
  }

  u64 &
  operator[](usize i) noexcept
  {
    return buf[i];
  }

  u64
  operator[](usize i) const noexcept
  {
    return buf[i];
  }

  u64
  front(void) const noexcept
  {
    return buf[0];
  }

  u64
  back(void) const noexcept
  {
    return buf[len - 1];
  }

  void
  push_back(u64 k) noexcept
  {
    if ( len < Cap ) buf[len++] = k;
  }

  void
  push_front(u64 k) noexcept
  {
    if ( len >= Cap ) return;
    for ( usize i = len; i > 0; --i ) buf[i] = buf[i - 1];
    buf[0] = k;
    ++len;
  }

  void
  pop_back(void) noexcept
  {
    if ( len ) --len;
  }

  void
  set(usize idx, u64 k) noexcept
  {
    if ( idx < len ) buf[idx] = k;
  }

  void
  insert(usize idx, u64 k) noexcept
  {
    if ( idx > len ) idx = len;
    if ( len >= Cap ) return;
    for ( usize i = len; i > idx; --i ) buf[i] = buf[i - 1];
    buf[idx] = k;
    ++len;
  }

  void
  insert(usize idx, u64 k, usize cnt) noexcept
  {
    for ( usize c = 0; c < cnt; ++c ) insert(idx + c, k);
  }

  void
  insert_seq(usize idx, const u64 *p, usize n) noexcept
  {
    if ( idx > len ) idx = len;
    if ( len + n > Cap ) n = Cap - len;
    for ( usize i = len; i > idx; --i ) buf[i + n - 1] = buf[i - 1];
    for ( usize i = 0; i < n; ++i ) buf[idx + i] = p[i];
    len += n;
  }

  void
  erase(usize idx) noexcept
  {
    if ( idx >= len ) return;
    for ( usize i = idx; i + 1 < len; ++i ) buf[i] = buf[i + 1];
    --len;
  }

  void
  erase_range(usize from, usize to) noexcept
  {
    if ( from >= to || to > len ) return;
    const usize cnt = to - from;
    for ( usize i = to; i < len; ++i ) buf[i - cnt] = buf[i];
    len -= cnt;
  }

  void
  assign(usize cnt, u64 k) noexcept
  {
    if ( cnt > Cap ) cnt = Cap;
    for ( usize i = 0; i < cnt; ++i ) buf[i] = k;
    len = cnt;
  }

  void
  append(const u64 *p, usize n) noexcept
  {
    for ( usize i = 0; i < n && len < Cap; ++i ) buf[len++] = p[i];
  }

  void
  fill_all(u64 k) noexcept
  {
    for ( usize i = 0; i < len; ++i ) buf[i] = k;
  }

  void
  resize(usize n) noexcept
  {
    if ( n <= len ) {
      len = n;
      return;
    }
    if ( n > Cap ) n = Cap;
    for ( usize i = len; i < n; ++i ) buf[i] = 0;
    len = n;
  }

  void
  resize(usize n, u64 k) noexcept
  {
    if ( n <= len ) {
      len = n;
      return;
    }
    if ( n > Cap ) n = Cap;
    for ( usize i = len; i < n; ++i ) buf[i] = k;
    len = n;
  }

  void
  resize_grow_only(usize n) noexcept
  {
    if ( n <= len ) return;
    if ( n > Cap ) n = Cap;
    for ( usize i = len; i < n; ++i ) buf[i] = 0;
    len = n;
  }

  void
  remove_val(u64 k) noexcept
  {
    usize w = 0;
    for ( usize i = 0; i < len; ++i )
      if ( buf[i] != k ) buf[w++] = buf[i];
    len = w;
  }

  usize
  find(u64 k) const noexcept
  {
    for ( usize i = 0; i < len; ++i )
      if ( buf[i] == k ) return i;
    return VREF_NPOS;
  }

  bool
  contains(u64 k) const noexcept
  {
    return find(k) != VREF_NPOS;
  }

  void
  sort(void) noexcept
  {
    for ( usize i = 1; i < len; ++i ) {
      u64 key = buf[i];
      usize j = i;
      while ( j > 0 && buf[j - 1] > key ) {
        buf[j] = buf[j - 1];
        --j;
      }
      buf[j] = key;
    }
  }

  u64
  sum(void) const noexcept
  {
    u64 s = 0;
    for ( usize i = 0; i < len; ++i ) s += buf[i];
    return s;
  }

  u64
  product(void) const noexcept
  {
    u64 p = 1;
    for ( usize i = 0; i < len; ++i ) p *= buf[i];
    return p;
  }

  u64
  vmin(void) const noexcept
  {
    u64 m = buf[0];
    for ( usize i = 1; i < len; ++i )
      if ( buf[i] < m ) m = buf[i];
    return m;
  }

  u64
  vmax(void) const noexcept
  {
    u64 m = buf[0];
    for ( usize i = 1; i < len; ++i )
      if ( buf[i] > m ) m = buf[i];
    return m;
  }
};

template<usize N> struct ref_ring {
  u64 buf[N];
  usize len = 0;

  void
  clear(void) noexcept
  {
    len = 0;
  }

  usize
  size(void) const noexcept
  {
    return len;
  }

  bool
  empty(void) const noexcept
  {
    return len == 0;
  }

  bool
  full(void) const noexcept
  {
    return len == N;
  }

  static constexpr usize
  capacity(void) noexcept
  {
    return N;
  }

  u64
  operator[](usize i) const noexcept
  {
    return buf[i];
  }

  u64
  front(void) const noexcept
  {
    return buf[0];
  }

  u64
  back(void) const noexcept
  {
    return buf[len - 1];
  }

  void
  push(u64 k) noexcept
  {
    if ( len < N ) {
      buf[len++] = k;
    } else {
      for ( usize i = 1; i < N; ++i ) buf[i - 1] = buf[i];
      buf[N - 1] = k;
    }
  }

  void
  pop_front(void) noexcept
  {
    if ( !len ) return;
    for ( usize i = 1; i < len; ++i ) buf[i - 1] = buf[i];
    --len;
  }
};

template<typename E, typename V, usize Cap>
inline bool
vec_eq(const V &v, const ref_vec<Cap> &r) noexcept
{
  if ( static_cast<usize>(v.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( elem<E>::key(v[i]) != r.buf[i] ) return false;
  return true;
}

template<typename E, typename CV, usize N>
inline bool
ring_eq(const CV &c, const ref_ring<N> &r) noexcept
{
  if ( static_cast<usize>(c.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( elem<E>::key(c[i]) != r.buf[i] ) return false;
  return true;
}

};      // namespace mtest
