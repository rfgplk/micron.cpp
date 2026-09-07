// Copyright (c) 2026 David Lucius Severus
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

#include "../../../type_traits.hpp"
#include "../../../types.hpp"

namespace bb
{

enum class ret_flag : u8 { invalid = 0, ok = 1, failure = 2, out_of_space = 3 };

constexpr static const u8 __tag_none = 0x00;
constexpr static const u8 __tag_alloc = 0x40;
constexpr static const u8 __tag_free = 0x80;
constexpr static const u8 __tag_sheet = 0xC0;
constexpr static const u8 __tag_kind = 0xC0;
constexpr static const u8 __tag_order = 0x3F;

constexpr usize native_alignment = 16;
constexpr usize header_size = 16;

constexpr static const u32 __null_off = 0xFFFFFFFFu;
constexpr static const u32 __tlsf_alloc = 1u;
constexpr static const u32 __tlsf_sentinel = 2u;
constexpr static const u32 __tlsf_flags = 0xFu;

struct __attribute__((__may_alias__)) tlsf_hdr {
  u32 bsize_flags;
  u32 prev_phys;
  u32 next_free;
  u32 prev_free;
};

static_assert(sizeof(tlsf_hdr) == header_size, "bb: tlsf_hdr must be 16 bytes");

namespace __bits
{

constexpr usize __usize_max = static_cast<usize>(-1);

[[gnu::always_inline]] constexpr inline bool
__is_pow2(usize v) noexcept
{
  return v != 0 && (v & (v - 1)) == 0;
}

[[gnu::always_inline]] constexpr inline usize
__align_up(usize v, usize a) noexcept
{
  return (v + a - 1) & ~(a - 1);
}

[[gnu::always_inline]] constexpr inline uintptr_t
__align_up_ptr(uintptr_t v, usize a) noexcept
{
  return (v + a - 1) & ~(static_cast<uintptr_t>(a) - 1);
}

constexpr inline i32
__log2(usize v) noexcept
{
  i32 r = 0;
  while( v > 1 ) {
    v >>= 1;
    ++r;
  }
  return r;
}

[[gnu::always_inline]] inline i32
__fls(usize v) noexcept
{
  if constexpr( sizeof(usize) == 8 )
    return 63 - __builtin_clzll(static_cast<unsigned long long>(v));
  else
    return 31 - __builtin_clz(static_cast<unsigned int>(v));
}

[[gnu::always_inline]] inline i32
__fls32(u32 v) noexcept
{
  return 31 - __builtin_clz(v);
}

[[gnu::always_inline]] inline i32
__ctz32(u32 v) noexcept
{
  return __builtin_ctz(v);
}

[[gnu::always_inline]] inline i32
__ctz64(u64 v) noexcept
{
  return __builtin_ctzll(static_cast<unsigned long long>(v));
}

[[gnu::always_inline]] inline usize
__next_pow2(usize v) noexcept
{
  if( v <= 1 )
    return 1;
  const i32 top = __fls(v - 1) + 1;
  if( top >= static_cast<i32>(sizeof(usize) * 8) )
    return __usize_max;
  return static_cast<usize>(1) << top;
}

[[gnu::always_inline]] inline usize
__round_up_sat(usize v, usize a) noexcept
{
  const usize r = (v + a - 1) & ~(a - 1);
  return r < v ? __usize_max : r;
}

};

};
