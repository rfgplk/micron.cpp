// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include "../../../types.hpp"

namespace abc
{

struct __tcache_chunk {
  byte *ptr;
  u32 size;
};

template<u32 Slots> struct alignas(64) __tier_tcache {
  static constexpr u32 __cache_slots = Slots;

  u32 _size[Slots];
  byte *_ptr[Slots];
  u32 _count;

  constexpr __tier_tcache() noexcept : _size{}, _ptr{}, _count(0) { }

  [[nodiscard, gnu::always_inline]] inline i32
  probe(u32 sz) const noexcept
  {
    if ( _count == 0 ) [[unlikely]]
      return -1;
#pragma GCC unroll 8
    for ( u32 i = _count; i > 0; --i ) {
      if ( _size[i - 1] == sz ) [[likely]]
        return static_cast<i32>(i - 1);
    }
    return -1;
  }

  [[nodiscard, gnu::always_inline]] inline i32
  probe_ge(u32 sz) const noexcept
  {
    if ( _count == 0 ) [[unlikely]]
      return -1;
#pragma GCC unroll 8
    for ( u32 i = _count; i > 0; --i ) {
      if ( _size[i - 1] >= sz ) [[likely]]
        return static_cast<i32>(i - 1);
    }
    return -1;
  }

  [[gnu::always_inline]] inline bool
  push(byte *p, u32 sz) noexcept
  {
    if ( _count >= Slots ) return false;
    _ptr[_count] = p;
    _size[_count] = sz;
    ++_count;
    return true;
  }

  // true iff p is already parked in this cache
  [[nodiscard, gnu::always_inline]] inline bool
  contains(const byte *p) const noexcept
  {
    for ( u32 i = 0; i < _count; ++i )
      if ( _ptr[i] == p ) return true;
    return false;
  }

  [[gnu::always_inline]] inline __tcache_chunk
  pop_at(u32 idx) noexcept
  {
    __tcache_chunk out = { _ptr[idx], _size[idx] };
    --_count;
    if ( idx != _count ) {
      _ptr[idx] = _ptr[_count];
      _size[idx] = _size[_count];
    }
    return out;
  }

  [[gnu::always_inline]] inline void
  invalidate_range(const byte *lo, const byte *hi) noexcept
  {
    u32 w = 0;
    for ( u32 r = 0; r < _count; ++r ) {
      if ( _ptr[r] < lo || _ptr[r] >= hi ) {
        if ( w != r ) {
          _ptr[w] = _ptr[r];
          _size[w] = _size[r];
        }
        ++w;
      }
    }
    _count = w;
  }

  template<typename Fn>
  [[gnu::always_inline]] inline void
  drain(Fn &&fn) noexcept
  {
    for ( u32 i = 0; i < _count; ++i ) fn(_ptr[i], _size[i]);
    _count = 0;
  }
};

// emulating arena design but for the precise class, helps maintain 0% bmiss on rapid repeated allocs
struct alignas(64) __slab_tcache {
  static constexpr u32 __num_classes = 16;
  static constexpr u32 __slots_per = 8;
  static constexpr u32 __class_step = 32;
  static constexpr u32 __class_min_payload = 64;                                                            // class 0
  static constexpr u32 __class_max_payload = __class_min_payload + (__num_classes - 1) * __class_step;      // 544
  static constexpr u32 __class_max_user = __num_classes * __class_step;                                     // 512
  static constexpr u32 __cache_slots = __num_classes * __slots_per;                                         // 128

  byte *_buckets[__num_classes][__slots_per];
  u8 _counts[__num_classes];
  u32 _occupied;

  constexpr __slab_tcache() noexcept : _buckets{}, _counts{}, _occupied(0) { }

  [[nodiscard, gnu::always_inline]] static inline i32
  class_for_user(u32 user_n) noexcept
  {
    if ( user_n == 0 or user_n > __class_max_user ) return -1;
    return static_cast<i32>((user_n - 1) >> 5);      // /32
  }

  [[nodiscard, gnu::always_inline]] static inline i32
  class_for_payload(u32 payload) noexcept
  {
    if ( payload < __class_min_payload or payload > __class_max_payload ) return -1;
    if ( (payload & 31u) != 0 ) return -1;
    return static_cast<i32>((payload - __class_min_payload) >> 5);
  }

  [[gnu::always_inline]] static inline i32
  encode_hit(u32 c, u32 s) noexcept
  {
    return static_cast<i32>((c << 16) | (s & 0xffffu));
  }

  // non redzone probe
  [[nodiscard, gnu::always_inline]] inline i32
  probe_ge(u32 user_n) const noexcept
  {
    const i32 c = class_for_user(user_n);
    if ( c < 0 ) [[unlikely]]
      return -1;
    if ( _counts[c] != 0 ) [[likely]]
      return encode_hit(static_cast<u32>(c), static_cast<u32>(_counts[c] - 1));
    const u32 m = _occupied & (~0u << static_cast<u32>(c));
    if ( m == 0 ) return -1;
    const u32 hit = static_cast<u32>(__builtin_ctz(m));
    return encode_hit(hit, static_cast<u32>(_counts[hit] - 1));
  }

  // redzoned probe
  [[nodiscard, gnu::always_inline]] inline i32
  probe(u32 user_n) const noexcept
  {
    const i32 c = class_for_user(user_n);
    if ( c < 0 or _counts[c] == 0 ) return -1;
    return encode_hit(static_cast<u32>(c), static_cast<u32>(_counts[c] - 1));
  }

  [[gnu::always_inline]] inline bool
  push(byte *p, u32 sz) noexcept
  {
    const i32 c = class_for_payload(sz);
    if ( c < 0 ) return false;
    const u32 cu = static_cast<u32>(c);
    if ( _counts[cu] >= __slots_per ) return false;
    _buckets[cu][_counts[cu]] = p;
    if ( _counts[cu] == 0 ) _occupied |= (1u << cu);
    ++_counts[cu];
    return true;
  }

  [[gnu::always_inline]] inline __tcache_chunk
  pop_at(u32 enc) noexcept
  {
    const u32 c = enc >> 16;
    const u32 s = enc & 0xffffu;
    __tcache_chunk out = { _buckets[c][s], __class_min_payload + c * __class_step };
    --_counts[c];
    if ( s != _counts[c] ) _buckets[c][s] = _buckets[c][_counts[c]];
    if ( _counts[c] == 0 ) _occupied &= ~(1u << c);
    return out;
  }

  // true iff p is parked in any size-class bucket
  [[nodiscard, gnu::always_inline]] inline bool
  contains(const byte *p) const noexcept
  {
    for ( u32 c = 0; c < __num_classes; ++c )
      for ( u32 i = 0; i < _counts[c]; ++i )
        if ( _buckets[c][i] == p ) return true;
    return false;
  }

  [[gnu::always_inline]] inline void
  invalidate_range(const byte *lo, const byte *hi) noexcept
  {
    for ( u32 c = 0; c < __num_classes; ++c ) {
      u32 w = 0;
      for ( u32 r = 0; r < _counts[c]; ++r ) {
        if ( _buckets[c][r] < lo or _buckets[c][r] >= hi ) {
          if ( w != r ) _buckets[c][w] = _buckets[c][r];
          ++w;
        }
      }
      _counts[c] = static_cast<u8>(w);
      if ( w == 0 ) _occupied &= ~(1u << c);
    }
  }

  template<typename Fn>
  [[gnu::always_inline]] inline void
  drain(Fn &&fn) noexcept
  {
    for ( u32 c = 0; c < __num_classes; ++c ) {
      const u32 sz = __class_min_payload + c * __class_step;
      for ( u32 i = 0; i < _counts[c]; ++i ) fn(_buckets[c][i], sz);
      _counts[c] = 0;
    }
    _occupied = 0;
  }
};

// NOTE: zero-slot specialization; needs such that every fn becomes a fn; compiler culls most of these at comptime
template<> struct alignas(4) __tier_tcache<0> {
  static constexpr u32 __cache_slots = 0;
  u32 _count;

  constexpr __tier_tcache() noexcept : _count(0) { }

  [[nodiscard, gnu::always_inline]] inline i32
  probe(u32) const noexcept
  {
    return -1;
  }

  [[nodiscard, gnu::always_inline]] inline i32
  probe_ge(u32) const noexcept
  {
    return -1;
  }

  [[gnu::always_inline]] inline bool
  push(byte *, u32) noexcept
  {
    return false;
  }

  [[gnu::always_inline]] inline __tcache_chunk
  pop_at(u32) noexcept
  {
    return { nullptr, 0 };
  }

  [[nodiscard, gnu::always_inline]] inline bool
  contains(const byte *) const noexcept
  {
    return false;
  }

  [[gnu::always_inline]] inline void
  invalidate_range(const byte *, const byte *) noexcept
  {
  }

  template<typename Fn>
  [[gnu::always_inline]] inline void
  drain(Fn &&) noexcept
  {
  }
};

};      // namespace abc
