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
